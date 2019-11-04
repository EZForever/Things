//g++ -static -s -Os -I$(dirname $ME) $ME -lole32 -o demo.exe
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <IOCTL_RAMDISK.h>
#include <IOCTL_RAMDISK_CREATEDISKDEVICE.h>
#include <IOCTL_RAMDISK_SAFETOREMOVE.h>

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
    CoCreateGuid(&inBuffer->id.guid);
    inBuffer->imageSource = IMAGESOURCE_FILE;
    inBuffer->flags = FLAGS_FIXED;
    inBuffer->imageSize = 128 * 1024 * 1024;
    inBuffer->imageOffset = 0;
    inBuffer->sizeUnk_uint64 = 0; //128 * 1024 * 1024;
    wcscpy(inBuffer->imagePath, IMAGE);
    //wcscpy_s(inBuffer->imagePath, wcslen(IMAGE) + 1, IMAGE); // Use this if VC++ is complaining about unsafe wcscpy()
    
    printf("IOCTL_CODE = %#x\n", IOCTL_RAMDISK_CREATEDISKDEVICE);
    printf("inBufferSize = %d\n", inBufferSize);
    printf("inBuffer = ");
    for (int i = 0; i < inBufferSize; i++)
        printf("%02hhx ", ((char *)inBuffer)[i]);
    putchar('\n');
    
    HANDLE hDevice;
    hDevice = CreateFile(RAMDISK_CONTROLLER, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
	
	// This makes the new device easlier to remove
	IOCTL_RAMDISK_SAFETOREMOVE_DATA inBuffer2 = {20};
	inBuffer2.id.guid = inBuffer->id.guid;
	ret = DeviceIoControl(hDevice, IOCTL_RAMDISK_SAFETOREMOVE, &inBuffer2, sizeof(inBuffer2), outBuffer, sizeof(outBuffer), &rcv, NULL);
    printf("ret = %d, rcv = %d, GetLastError() = %d\n", ret, rcv, GetLastError());
	
    CloseHandle(hDevice);
    delete[] (char *)inBuffer;
    return 0;
}