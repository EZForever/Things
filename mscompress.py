#!/usr/bin/env python3

__all__ = [
    'WimlibCompressor',
    'CabinetCompressor'
]

# ---

import io
import os
import sys
import zlib
import ctypes
import ctypes.util

from typing import Optional

BLOCK_SIZE = 4 * 1024 * 1024

# ---

# 200 is pretty close to the hard limit
# https://github.com/ebiggers/wimlib/blob/v1.14.3/src/lzms_compress.c#L2141-L2146
COMPRESSION_LEVEL = 200

class WimlibCompressor:
    _dll = None

    def _init_dll(self, path: str) -> None:
        self._dll = ctypes.CDLL(path)
        self._dll.wimlib_get_error_string.argtypes = [ctypes.c_int]
        self._dll.wimlib_get_error_string.restype = ctypes.c_void_p
        self._dll.wimlib_create_compressor.argtypes = [ctypes.c_int, ctypes.c_size_t, ctypes.c_uint, ctypes.POINTER(ctypes.c_void_p)]
        self._dll.wimlib_create_compressor.restype = ctypes.c_int
        self._dll.wimlib_compress.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p]
        self._dll.wimlib_compress.restype = ctypes.c_size_t
        self._dll.wimlib_free_compressor.argtypes = [ctypes.c_void_p]
        self._dll.wimlib_free_compressor.restype = None

    def _wimlib_error(self, code: int) -> Exception:
        # https://wimlib.net/apidoc/index.html#sec_encodings
        ptr = self._dll.wimlib_get_error_string(code)
        if sys.platform == 'win32':
            desc = ctypes.wstring_at(ptr)
        else:
            desc = ctypes.string_at(ptr)
        return OSError(code, desc)

    def __init__(self, path: Optional[str] = None):
        if self._dll is None:
            path = path or ctypes.util.find_library('wim-15')
            path = path or ctypes.util.find_library('libwim-15.dll')
            if path is None:
                # find_library on Windows does not search program base directory
                path = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'libwim-15.dll')
            else:
                # XXX: find_library breaks if PATH contains empty string and returns only the library name
                path = os.path.abspath(path)
            if not os.path.isfile(path):
                raise RuntimeError('Could not locate libwim')
            self._init_dll(path)
        
        self._handle = ctypes.c_void_p()
        ret = self._dll.wimlib_create_compressor(3, BLOCK_SIZE, COMPRESSION_LEVEL, ctypes.byref(self._handle)) # WIMLIB_COMPRESSION_TYPE_LZMS
        if ret != 0:
            raise self._wimlib_error(ret)

    def close(self) -> None:
        if self._handle:
            self._dll.wimlib_free_compressor(self._handle)
            self._handle = None

    def compress(self, data: bytes) -> bytes:
        # dwMagic & wHeaderSize
        header1 = (0xC0E5510A).to_bytes(4, 'little') \
                + (24).to_bytes(2, 'little')
        # bAlgorithm, qwUncompressedDataSize & qwUncompressedBlockSize
        header2 = (5).to_bytes(1, 'little') \
                + len(data).to_bytes(8, 'little') \
                + min(len(data), BLOCK_SIZE).to_bytes(8, 'little')
        crc = zlib.crc32(header1, 0)
        crc = zlib.crc32(header2, crc)

        with io.BytesIO() as f:
            f.write(header1)
            f.write(bytes([crc & 0xFF]))
            f.write(header2)

            retbuf = ctypes.create_string_buffer(BLOCK_SIZE)
            pdata = ctypes.cast(data, ctypes.c_void_p).value
            for offset in range(0, len(data), BLOCK_SIZE):
                block_size = min(len(data) - offset, BLOCK_SIZE)

                if block_size <= 8:
                    # XXX: This does not match CABINET!LzmsEncoderEncode(), but matches testing
                    f.write(block_size.to_bytes(4, 'little'))
                    f.write(data[offset : offset + block_size])
                else:
                    retsize = self._dll.wimlib_compress(pdata + offset, block_size, ctypes.byref(retbuf), BLOCK_SIZE, self._handle)
                    if retsize == 0:
                        raise RuntimeError('wimlib_compress() returned 0')

                    f.write(retsize.to_bytes(4, 'little'))
                    f.write(retbuf.raw[ : retsize])

            return f.getvalue()

# ---

if sys.platform == 'win32':
    import ctypes.wintypes as wt

    SIZE_T = wt.ULARGE_INTEGER if ctypes.sizeof(wt.LPVOID) == ctypes.sizeof(wt.ULARGE_INTEGER) else wt.ULONG
    PSIZE_T = ctypes.POINTER(SIZE_T)

    class CabinetCompressor:
        _dll = None

        def _init_dll(self) -> None:
            self._dll = ctypes.WinDLL('CABINET')
            self._dll.CreateCompressor.argtypes = [wt.DWORD, wt.LPVOID, wt.PHANDLE]
            self._dll.CreateCompressor.restype = wt.BOOL
            self._dll.SetCompressorInformation.argtypes = [wt.HANDLE, ctypes.c_int, wt.LPCVOID, SIZE_T]
            self._dll.SetCompressorInformation.restype = wt.BOOL
            self._dll.Compress.argtypes = [wt.HANDLE, wt.LPCVOID, SIZE_T, wt.LPVOID, SIZE_T, PSIZE_T]
            self._dll.Compress.restype = wt.BOOL
            self._dll.CloseCompressor.argtypes = [wt.HANDLE]
            self._dll.CloseCompressor.restype = wt.BOOL

        def __init__(self):
            if self._dll is None:
                self._init_dll()
            
            self._handle = wt.HANDLE()
            ret = self._dll.CreateCompressor(5, 0, ctypes.byref(self._handle)) # COMPRESS_ALGORITHM_LZMS
            if ret == 0:
                raise ctypes.WinError()
            
            block_size = wt.DWORD(BLOCK_SIZE)
            ret = self._dll.SetCompressorInformation(self._handle, 1, ctypes.byref(block_size), ctypes.sizeof(block_size)) # COMPRESS_INFORMATION_CLASS_BLOCK_SIZE
            if ret == 0:
                raise RuntimeWarning('SetCompressorInformation() failed', ctypes.WinError())

        def close(self) -> None:
            if self._handle:
                self._dll.CloseCompressor(self._handle)
                self._handle = None

        def compress(self, data: bytes) -> bytes:
            retsize = SIZE_T()
            ret = self._dll.Compress(self._handle, data, len(data), 0, 0, ctypes.byref(retsize))
            retexc = ctypes.WinError()
            if ret != 0 or retexc.winerror != 0x7A: # ERROR_INSUFFICIENT_BUFFER
                raise retexc
            
            retbuf = ctypes.create_string_buffer(retsize.value)
            ret = self._dll.Compress(self._handle, data, len(data), ctypes.byref(retbuf), retsize, ctypes.byref(retsize))
            if ret == 0:
                raise ctypes.WinError()
            
            return retbuf.raw[ : retsize.value]

