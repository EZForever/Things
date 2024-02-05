#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <initguid.h>
#include <virtdisk.h>

#include <tuple>
#include <cstdio>

#pragma comment(lib, "virtdisk.lib")

int wmain(int argc, wchar_t** argv)
{
	if (argc != 2)
	{
		fwprintf(stderr, L"usage: mountiso <iso>\n");
		return 1;
	}

	VIRTUAL_STORAGE_TYPE storageType{ VIRTUAL_STORAGE_TYPE_DEVICE_ISO , VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT };
	HANDLE hDisk;
	DWORD ret = OpenVirtualDisk(
		&storageType,
		argv[1],
		VIRTUAL_DISK_ACCESS_ATTACH_RO | VIRTUAL_DISK_ACCESS_DETACH,
		OPEN_VIRTUAL_DISK_FLAG_NONE,
		NULL,
		&hDisk
	);
	if (ret != ERROR_SUCCESS)
	{
		fwprintf(stderr, L"OpenVirtualDisk() failed, error %u\n", ret);
		return 1;
	}

	ret = AttachVirtualDisk(
		hDisk,
		NULL,
		ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY,
		0,
		NULL,
		NULL
	);
	if (ret != ERROR_SUCCESS)
	{
		fwprintf(stderr, L"AttachVirtualDisk() failed, error %u\n", ret);
		CloseHandle(hDisk);
		return 1;
	}

	wprintf(L"ISO mounted: %s\n", argv[1]);
	wprintf(L"Press Enter to unmount...");
	std::ignore = getwchar();

	ret = DetachVirtualDisk(
		hDisk,
		DETACH_VIRTUAL_DISK_FLAG_NONE,
		0
	);
	if (ret != ERROR_SUCCESS)
	{
		fwprintf(stderr, L"DetachVirtualDisk() failed, error %u\n", ret);
		CloseHandle(hDisk);
		return 1;
	}

	CloseHandle(hDisk);
	return 0;
}