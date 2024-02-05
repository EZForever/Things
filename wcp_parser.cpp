#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>

#include <isolation.h>

#include <stdio.h>

typedef HRESULT (WINAPI *SetIsolationIMalloc_t)(IMalloc* alloc);
typedef HRESULT (WINAPI *ParseManifestEx_t)(DWORD dwFlags, PCWSTR pszManifestPath, const PVOID callback, REFIID riid, IUnknown** ppManifest, const PULONG disp);

int wmain(int argc, wchar_t** argv)
{
	if (argc < 3)
	{
		fwprintf(stderr, L"usage: wcp_parser <wcp.dll> <manifest>...\n");
		return 1;
	}

	HMODULE hModWCP = LoadLibraryW(argv[1]);
	if (!hModWCP)
	{
		fwprintf(stderr, L"LoadLibrary(wcp) failed, GetLastError() = %08x\n", GetLastError());
		return 1;
	}
	wprintf(L"hModWCP\t\t\t= %p\n", hModWCP);

	auto pSetIsolationIMalloc = (SetIsolationIMalloc_t)GetProcAddress(hModWCP, "SetIsolationIMalloc");
	auto pParseManifestEx = (ParseManifestEx_t)GetProcAddress(hModWCP, "ParseManifestEx");
	if (!pSetIsolationIMalloc || !pParseManifestEx)
	{
		fwprintf(stderr, L"GetProcAddress(...) failed, GetLastError() = %08x\n", GetLastError());
		FreeLibrary(hModWCP);
		return 2;
	}
	wprintf(L"pParseManifestEx\t= %p\n", pParseManifestEx);

	IMalloc* alloc = NULL;
	HRESULT hr = CoGetMalloc(1, &alloc);
	if (FAILED(hr) || !alloc)
	{
		fwprintf(stderr, L"CoGetMalloc() failed, hr = %08x\n", hr);
		FreeLibrary(hModWCP);
		return 3;
	}

	hr = pSetIsolationIMalloc(alloc);
	if (FAILED(hr))
	{
		fwprintf(stderr, L"SetIsolationIMalloc() failed, hr = %08x\n", hr);
		alloc->Release();
		FreeLibrary(hModWCP);
		return 4;
	}

	// WCP should now hold a reference
	alloc->Release();
	alloc = NULL;

#if 0

	constexpr IID IID_IDefinitionIdentity = { 0x587bf538,0x4d90,0x4a3c,{0x9e,0xf1,0x58,0xa2,0x00,0xa8,0xa9,0xe7} };

	IDefinitionIdentity* manifest = NULL;
	hr = pParseManifestEx(1, L"C:\\Windows\\WinSxS\\Manifests\\amd64_microsoft-windows-o..euapcommonproxystub_31bf3856ad364e35_10.0.19041.2075_none_079a529e3569df04.manifest", NULL, IID_IDefinitionIdentity, (IUnknown**)&manifest, NULL);
	if (FAILED(hr) || !manifest)
	{
		fwprintf(stderr, L"ParseManifest(IID_IDefinitionIdentity) failed, hr = %08x\n", hr);
		FreeLibrary(hModWCP);
		return 5;
	}

	IEnumIDENTITY_ATTRIBUTE* enumAttribute = NULL;
	hr = manifest->EnumAttributes(&enumAttribute);
	if (FAILED(hr) || !enumAttribute)
	{
		fwprintf(stderr, L"IDefinitionIdentity::EnumAttributes() failed, hr = %08x\n", hr);
		manifest->Release();
		FreeLibrary(hModWCP);
		return 6;
	}

	IDENTITY_ATTRIBUTE attribute{ NULL };
	unsigned long long sz = 1; // XXX: Stack corruption if actuallly using ULONG
	while (SUCCEEDED(hr = enumAttribute->Next(sz, &attribute, (ULONG*)&sz)) && hr != S_FALSE)
	{
		if(attribute.pszNamespace)
			wprintf(L"%s::", attribute.pszNamespace);
		wprintf(L"%s = %s\n", attribute.pszName, attribute.pszValue);
	}

	enumAttribute->Release();
	manifest->Release();

#else

	for (int i = 2; i < argc; i++)
	{
		wprintf(L"<!-- %s -->\n", argv[i]);

		IStream* manifest = NULL;
		hr = pParseManifestEx(1, argv[i], NULL, IID_IStream, (IUnknown**)&manifest, NULL);
		if (FAILED(hr) || !manifest)
		{
			fwprintf(stderr, L"ParseManifest(IID_IStream) failed, hr = %08x\n", hr);
			FreeLibrary(hModWCP);
			return 5;
		}
	
		BYTE buf[4096] = { 0 };
		ULONG cbRead = 0;
		while (SUCCEEDED(manifest->Read(buf, sizeof(buf), &cbRead)) && cbRead > 0)
		{
			wprintf(L"%.*S", cbRead, buf);
		}
		wprintf(L"\n");

		manifest->Release();
	}

#endif

	FreeLibrary(hModWCP);
	return 0;
}