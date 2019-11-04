#ifndef __IOCTL_RAMDISK_SAFETOREMOVE_H__
#define __IOCTL_RAMDISK_SAFETOREMOVE_H__

#include <IOCTL_RAMDISK.h>

// Mark the specific disk device as "safe to remove"
// Without this, you can remove device just fine, but all the resources cosumed by the device will not be released, until you disable the controller
// Note the disk device still work after calling this
const DWORD IOCTL_RAMDISK_SAFETOREMOVE = 0x240004;

struct IOCTL_RAMDISK_SAFETOREMOVE_DATA {
	// sizeof / magic
	// Must be 20
	uint32_t magic;
	
	// Device ID
	RAMDISK_ID id;
	
} __attribute__((packed));

#endif // __IOCTL_RAMDISK_SAFETOREMOVE_H__