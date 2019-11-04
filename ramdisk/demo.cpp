//g++ -static -s -Os -I$(dirname $ME) $ME -lole32 -o demo.exe
#include <windows.h>
#include <objbase.h>
#include <cstdint>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <IOCTL_RAMDISK_CREATEDISKDEVICE.h>

/* Hint:
	In user mode, "\\.\\" or "\\?\\" refers to kernel namespace "\\GLOBAL??\\"
		Then "\\.\\GLOBALROOT\\" refers to entire kernel namespace
	In kernel mode, "\\??\\" or "\\DosDevices\\" refers to MSDOS namespace (i.e. all the disk letters)
*/

const wchar_t IMAGE[] = L"\\??\\E:\\rev\\ramdisk\\TEST128.vhd";

int main() {	
    size_t inBufferSize = sizeof(IOCTL_RAMDISK_CREATEDISKDEVICE_DATA) + sizeof(IMAGE) - 4 * sizeof(wchar_t);
    IOCTL_RAMDISK_CREATEDISKDEVICE_DATA* inBuffer = (IOCTL_RAMDISK_CREATEDISKDEVICE_DATA*)new char[inBufferSize];
    memset(inBuffer, 0, sizeof(IOCTL_RAMDISK_CREATEDISKDEVICE_DATA));
	
    inBuffer->magic = 0x40;
    CoCreateGuid(&inBuffer->id_guid);
    inBuffer->imageSource = IMAGESOURCE_FILE;
    inBuffer->flags = FLAGS_FIXED;
    inBuffer->imageSize = 128 * 1024 * 1024;
    inBuffer->imageOffset = 0;
    inBuffer->sizeUnk_uint64 = 0; //128 * 1024 * 1024;
    wcscpy_s(inBuffer->imagePath, wcslen(IMAGE) + 1, IMAGE);

    printf("IOCTL_CODE = %#x\n", IOCTL_RAMDISK_CREATEDISKDEVICE);
    printf("inBufferSize = %d\n", inBufferSize);
    printf("inBuffer = ");
    for (int i = 0; i < inBufferSize; i++)
        printf("%02hhx ", ((char *)inBuffer)[i]);
    putchar('\n');
    
    HANDLE hDevice;
    hDevice = CreateFile("\\\\.\\GLOBALROOT\\Device\\Ramdisk", 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    printf("hDevice = %p, GetLastError() = %d\n", hDevice, GetLastError());
    
    char outBuffer[64]; // Actually there will not be any output
    DWORD rcv = 0; // Sometimes this dos not get initialized by DeviceIoControl()
    BOOL ret;
    ret = DeviceIoControl(hDevice, IOCTL_RAMDISK_CREATEDISKDEVICE, inBuffer, inBufferSize, outBuffer, sizeof(outBuffer), &rcv, NULL);
    printf("ret = %d, rcv = %d, GetLastError() = %d\n", ret, rcv, GetLastError());
    printf("outBuffer = ");
    for (int i = 0; i < rcv; i++)
        printf("%02hhx ", outBuffer[i]);
    putchar('\n');
	
    CloseHandle(hDevice);
    delete[] (char *)inBuffer;
    return 0;
}