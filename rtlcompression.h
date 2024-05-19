#pragma once

/*
    Tips & quirks on using these APIs:

    - COMPRESSION_FORMAT_* and COMPRESSION_ENGINE_* constants are defined in winnt.h, thus immediately usable
    - Compression Ratio: XPRESS_HUFF > XPRESS > LZNT1
    - RtlDecompressBuffer does not require a workspace, but only supports LZNT1 and XPRESS (XPRESS_HUFF fails with C00000E8)
    - RtlDecompressBufferEx requires a workspace and target buffer size must be spot on or it will fail with C0000224
    - RtlDecompressBufferEx2 is not exported via NTDLL, and loading NTOSKRNL by hand will crash with C0000096
    - According to <https://github.com/coderforlife/ms-compress>, RTL impl of the algorithms needs more memory than compressed buffer
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma comment(lib, "ntdll.lib")

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NTSTATUS
#   define __NTSTATUS_NOT_DEFINED
#   define NTSTATUS HRESULT // XXX: They are basically the same thing
#endif

#define NT_RTL_COMPRESSION_API __declspec(dllimport) NTSTATUS WINAPI

NT_RTL_COMPRESSION_API RtlGetCompressionWorkSpaceSize(
    /*[in]*/  USHORT CompressionFormatAndEngine,
    /*[out]*/ PULONG CompressBufferWorkSpaceSize,
    /*[out]*/ PULONG CompressFragmentWorkSpaceSize
);

NT_RTL_COMPRESSION_API RtlCompressBuffer(
    /*[in]*/  USHORT CompressionFormatAndEngine,
    /*[in]*/  PUCHAR UncompressedBuffer,
    /*[in]*/  ULONG  UncompressedBufferSize,
    /*[out]*/ PUCHAR CompressedBuffer,
    /*[in]*/  ULONG  CompressedBufferSize,
    /*[in]*/  ULONG  UncompressedChunkSize,
    /*[out]*/ PULONG FinalCompressedSize,
    /*[in]*/  PVOID  WorkSpace
);

NT_RTL_COMPRESSION_API RtlDecompressBuffer(
    /*[in]*/  USHORT CompressionFormat,
    /*[out]*/ PUCHAR UncompressedBuffer,
    /*[in]*/  ULONG  UncompressedBufferSize,
    /*[in]*/  PUCHAR CompressedBuffer,
    /*[in]*/  ULONG  CompressedBufferSize,
    /*[out]*/ PULONG FinalUncompressedSize
);

NT_RTL_COMPRESSION_API RtlDecompressBufferEx(
    /*[in]*/  USHORT CompressionFormat,
    /*[out]*/ PUCHAR UncompressedBuffer,
    /*[in]*/  ULONG  UncompressedBufferSize,
    /*[in]*/  PUCHAR CompressedBuffer,
    /*[in]*/  ULONG  CompressedBufferSize,
    /*[out]*/ PULONG FinalUncompressedSize,
    /*[in]*/  PVOID  WorkSpace
);

NT_RTL_COMPRESSION_API RtlDecompressBufferEx2(
    /*[in]*/           USHORT CompressionFormat,
    /*[out]*/          PUCHAR UncompressedBuffer,
    /*[in]*/           ULONG  UncompressedBufferSize,
    /*[in]*/           PUCHAR CompressedBuffer,
    /*[in]*/           ULONG  CompressedBufferSize,
    /*[in]*/           ULONG  UncompressedChunkSize,
    /*[out]*/          PULONG FinalUncompressedSize,
    /*[in, optional]*/ PVOID  WorkSpace
);

#ifdef __NTSTATUS_NOT_DEFINED
#   undef NTSTATUS
#   undef __NTSTATUS_NOT_DEFINED
#endif

#ifdef __cplusplus
}
#endif

