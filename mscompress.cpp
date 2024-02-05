#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include <vector>

#include <compressapi.h>

#pragma comment(lib, "cabinet.lib")

//constexpr size_t BLOCK_SIZE = 1 * 1024 * 1024;
constexpr size_t BLOCK_SIZE = 4 * 1024 * 1024;

#if 1

struct compressor_handle_t
{
    COMPRESSOR_HANDLE handle;

    compressor_handle_t()
    {
        auto ret = CreateCompressor(COMPRESS_ALGORITHM_LZMS, NULL, &handle);
        assert(ret);

        DWORD blocksize = BLOCK_SIZE;
        SetCompressorInformation(handle, COMPRESS_INFORMATION_CLASS_BLOCK_SIZE, &blocksize, sizeof(blocksize));
    }

    ~compressor_handle_t()
    {
        CloseCompressor(handle);
        handle = NULL;
    }

    operator COMPRESSOR_HANDLE ()
    {
        return handle;
    }
};

static void mscompress(const std::vector<BYTE>& inbuf, std::vector<BYTE>& outbuf)
{
    static compressor_handle_t handle;

    // guesswork
    outbuf.resize(inbuf.size() + 0x1000);

    SIZE_T compressed = 0;
    auto ret = Compress(handle, inbuf.data(), inbuf.size(), outbuf.data(), outbuf.size(), &compressed);
    outbuf.resize(compressed);
    assert(ret && compressed != 0);
}

#else // wimlib produces smaller results, but (obviously) needs a copy of wimlib

#pragma warning(push)
#pragma warning(disable: 4200) // 0-size array in struct
#include "wimlib/wimlib.h"
#pragma warning(pop)

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "wimlib/libwim.lib")

// 200 is pretty close to the hard limit
// https://github.com/ebiggers/wimlib/blob/v1.14.3/src/lzms_compress.c#L2141-L2146
constexpr unsigned int COMPRESSION_LEVEL = 200;

extern "C"
DWORD WINAPI RtlComputeCrc32(DWORD dwInitial, const BYTE* pData, INT iLen);

struct COMPRESS_BUFFER_HEADER {
    DWORD dwMagic; // = 0xC0E5510A
    WORD wHeaderSize; // = 24
    BYTE bHeaderCrc; // LSB of RtlComputeCrc32 of whole hdr, skipping this byte
    BYTE bAlgorithm; // LSB of COMPRESS_ALGORITHM_*; LZMS = 5
    DWORD64 qwUncompressedDataSize;
    DWORD64 qwUncompressedBlockSize; // <= 0x4000000; Unrelated to the LZMS setting
};
static_assert(sizeof(COMPRESS_BUFFER_HEADER) == 24, "sizeof(COMPRESS_BUFFER_HEADER)");

struct compressor_handle_t
{
    wimlib_compressor* handle;

    compressor_handle_t()
    {
        auto ret = wimlib_create_compressor(WIMLIB_COMPRESSION_TYPE_LZMS, BLOCK_SIZE, COMPRESSION_LEVEL, &handle);
        assert(ret == 0);
    }

    ~compressor_handle_t()
    {
        wimlib_free_compressor(handle);
        handle = nullptr;
    }

    operator wimlib_compressor* ()
    {
        return handle;
    }
};

static void mscompress(const std::vector<BYTE>& inbuf, std::vector<BYTE>& outbuf)
{
    static compressor_handle_t handle;
    static std::vector<BYTE> workspace;
    if (workspace.size() == 0)
    {
        auto worksize = wimlib_get_compressor_needed_memory(WIMLIB_COMPRESSION_TYPE_LZMS, BLOCK_SIZE, COMPRESSION_LEVEL);
        workspace.resize(worksize);
    }

    // guesswork
    outbuf.resize(inbuf.size() + 0x1000);

    auto hdr = (COMPRESS_BUFFER_HEADER*)outbuf.data();
    hdr->dwMagic = 0xC0E5510A;
    hdr->wHeaderSize = sizeof(COMPRESS_BUFFER_HEADER);
    //hdr->bHeaderCrc = 0; // temp value
    hdr->bAlgorithm = COMPRESS_ALGORITHM_LZMS;
    hdr->qwUncompressedDataSize = inbuf.size();
    hdr->qwUncompressedBlockSize = BLOCK_SIZE;

    auto crcoff = offsetof(COMPRESS_BUFFER_HEADER, bHeaderCrc);
    auto crc = RtlComputeCrc32(0, outbuf.data(), (INT)crcoff);
    crc = RtlComputeCrc32(crc, outbuf.data() + crcoff + 1, (INT)(sizeof(COMPRESS_BUFFER_HEADER) - crcoff - 1));
    hdr->bHeaderCrc = (BYTE)crc;

    size_t outoff = sizeof(COMPRESS_BUFFER_HEADER);
    for (size_t inoff = 0; inoff < inbuf.size(); inoff += BLOCK_SIZE)
    {
        auto compressed = wimlib_compress(&inbuf[inoff], min(inbuf.size() - inoff, BLOCK_SIZE), &outbuf[outoff] + 4, outbuf.size() - outoff - 4, handle);
        assert(compressed != 0);
        *(DWORD*)&outbuf[outoff] = (DWORD)compressed;
        outoff += 4 + compressed;
    }
    outbuf.resize(outoff);
}

#endif

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 3)
    {
        fwprintf(stderr, L"usage: %s <in> <out>\n", argv[0]);
        return 0;
    }

    FILE* fp = NULL;
    _wfopen_s(&fp, argv[1], L"rb");
    if (!fp)
    {
        return 1;
    }

    size_t insize;
    fseek(fp, 0, SEEK_END);
    insize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    wprintf(L"insize = %zu\n", insize);

    std::vector<BYTE> inbuf(insize);
    if (fread(inbuf.data(), 1, inbuf.size(), fp) != insize)
    {
        return 1;
    }
    fclose(fp);

    // ---

    std::vector<BYTE> outbuf;
    mscompress(inbuf, outbuf);
    wprintf(L"outsize = %zu %5.2f%%\n", outbuf.size(), (100.0 * outbuf.size() / inbuf.size()));

    // ---

    _wfopen_s(&fp, argv[2], L"wb");
    if (!fp)
    {
        return 1;
    }

    if (fwrite(outbuf.data(), 1, outbuf.size(), fp) != outbuf.size())
    {
        return 1;
    }

    // ---

    DECOMPRESSOR_HANDLE handle;
    auto ret = CreateDecompressor(COMPRESS_ALGORITHM_LZMS, NULL, &handle);
    assert(ret);

    ret = Decompress(handle, outbuf.data(), outbuf.size(), inbuf.data(), inbuf.size(), &insize);
    assert(ret && insize == inbuf.size());

    CloseDecompressor(handle);
    return 0;
}

