#ifndef __IOCTL_RAMDISK_CREATEDISKDEVICE_H__
#define __IOCTL_RAMDISK_CREATEDISKDEVICE_H__

#include <IOCTL_RAMDISK.h>

// Create a new ramdisk volume device
const DWORD IOCTL_RAMDISK_CREATEDISKDEVICE = 0x240000;

enum RAMDISK_IMAGESOURCE : uint32_t {
	// Load/save volume from/to a image file
	// Requires imagePath
	IMAGESOURCE_FILE	= 2,
	
	// No actual buffer is attached to the device (?), resulting in a dummy device
	// Requires sizeUnk_uint64 != 0
	IMAGESOURCE_NONE	= 3,
	
	// Fail immediately with STATUS_INVALID_PARAMETER
	// This is for Windows XP era, when the driver will create A SERIES OF disk devices based on registry settings
	// Some of the logic remains in current (Windows 10) version of code
	IMAGESOURCE_INVALID	= 4,
	
	// Create the volume purely from RAM
	// Ignores imageOffset, sizeUnk & imagePath
	// Not supported on (and before) Windows 7
	IMAGESOURCE_RAM		= 5,
};

enum RAMDISK_FLAGS : uint32_t {
	// The volume is write-protected
	FLAGS_READONLY		= 0x01,
	
	// The volume is a fixed drive, instead of a removable one
	FLAGS_FIXED			= 0x02,
	
	// Do not create symlink of device in global namespace,
	// i.e. The symlink from "\\Device\\Ramdisk{ID}" to "\\GLOBAL??\\Ramdisk{ID}"
	FLAGS_NOSYMLINK		= 0x08,
	
	// The volume is a CDROM drive, instead of a hard drive
	// Same effect as the Windows boot parameter "RDEXPORTASCD"
	// Indicates FLAGS_READONLY
	FLAGS_CDROM			= 0x20,
};

// The input buffer for ioctl request, confirmed on Windows 7 x64 & Windows 10 x64
// Windows XP version of this struct has some noticeable differences, see below
struct IOCTL_RAMDISK_CREATEDISKDEVICE_DATA {
	// sizeof / magic
	// Must be 0x40, regardless of actual buffer size
	uint32_t magic;
	
	// Device ID
	RAMDISK_ID id;
	
	// Source of volume image
	RAMDISK_IMAGESOURCE imageSource;
	
	// Bitflag; misc settings
	uint32_t flags;
	//RAMDISK_FLAGS flags; // Enum classes does not get along with bitwise OR
	
	// Unknown; not seen in any cases
	uint32_t unk;
	
	// IMAGESOURCE_RAM: The size of volume in bytes
	// IMAGESOURCE_FILE: Desired length of file to be loaded as disk (RDIMAGELENGTH)
	uint64_t imageSize;
	
	// Desired offset of file to be loaded as disk in bytes (RDIMAGEOFFSET)
	uint64_t imageOffset;
	
	// Size for something unknown
	union {
		// IMAGESOURCE_FILE: ???
		// If invalid, the driver will santize these parameters, or use default values (16, 0x100000)
		// The santized values will be the size factor of a buffer (count:length << 6) gave to the newly-created device
		struct {
			uint32_t count;
			uint32_t length;
		} sizeUnk_view;
		
		// Image sources other than IMAGESOURCE_FILE
		// Set to == imageSize is fine for most cases
		uint64_t sizeUnk_uint64;
	};
	
	// Path *in kernel namespace* to volume image file (RDPATH)
	// Can be longer than 4 `wchar_t`s, but must end with L'\0'
	// e.g. L"\\??\\C:\\path\\to\\file.img"
	// or L"\\GLOBAL??\\C:\\path\\to\\file.img"
	wchar_t imagePath[4];
	
} __attribute__((packed));

// Windows XP specific version of IOCTL_RAMDISK_CREATEDISKDEVICE_DATA
// NOT TESTED, only here for reference
struct IOCTL_RAMDISK_CREATEDISKDEVICE_DATA_XP {
	// Must be 0x38
	uint32_t magic;
	
	RAMDISK_ID id;
	
	// IMAGESOURCE_INVALID still fail with STATUS_INVALID_PARAMETER
	// But the "registry disks" will be created under this value
	RAMDISK_IMAGESOURCE imageSource;
	
	uint32_t flags;
	
	uint32_t unk;
	
	uint64_t imageSize;
	
	uint32_t imageOffset;
		
	union {
		struct {
			uint32_t count;
			uint32_t length;
		} sizeUnk_view;
		
		uint64_t sizeUnk_uint64;
		
		// If IMAGESOURCE_NONE and not FLAGS_NOSYMLINK, another symlink to "\\DosDevices\\%wc:" is made,
		//	 where "%wc" is sizeUnk_diskletter.v
		struct {
			uint32_t _0;
			wchar_t v;
			wchar_t _1;
		} sizeUnk_diskletter;
	};
	
	wchar_t imagePath[2];
	
} __attribute__((packed));

#endif // __IOCTL_RAMDISK_CREATEDISKDEVICE_H__