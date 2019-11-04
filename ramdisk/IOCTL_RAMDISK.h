#ifndef __IOCTL_RAMDISK_H__
#define __IOCTL_RAMDISK_H__

#include <windows.h>
#include <objbase.h>
#include <cstdint>

// Device path for ramdisk controller
#define RAMDISK_CONTROLLER ("\\\\.\\GLOBALROOT\\Device\\Ramdisk")

// Device ID; will be used as device name
union RAMDISK_ID {
	// For IMAGESOURCE_RAM or IMAGESOURCE_INVALID
	// "\\Device\\Ramdisk%u"
	// Note the unused bytes also get stored as part of ID
	uint32_t uint32;
	
	// For IMAGESOURCE_FILE or IMAGESOURCE_NONE
	// "\\Device\\Ramdisk{%8x-%4x-%4x-%4x-%12x}"
	_GUID guid;
};

// Use these if you are using VC++
//#include <pshpack1.h>
//#include <poppack.h>

#endif // __IOCTL_RAMDISK_H__