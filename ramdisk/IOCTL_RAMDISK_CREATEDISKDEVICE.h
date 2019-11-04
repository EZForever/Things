#include <windows.h>
#include <objbase.h>
#include <cstdint>

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
	// However, logic corresponding to this exists
	IMAGESOURCE_INVALID	= 4,
	
	// Create the volume purely from RAM
	// Ignores imageOffset, sizeUnk & imagePath
	// Not supported on Windows 7
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

// The input buffer for ioctl request, confirmed on Windows 7 x64 & Windows 10 x64.
// Windows XP version of this struct has sone noticeable differences.
struct IOCTL_RAMDISK_CREATEDISKDEVICE_DATA {
	// sizeof / magic
	// Must be 0x40, regardless of actual buffer size
	uint32_t magic;
	
	// Device ID; will be used as device name
	union {
		// For IMAGESOURCE_RAM or IMAGESOURCE_INVALID
		// "\\Device\\Ramdisk%u"
		struct {
			uint32_t value;
			uint32_t _[3];
		} id_uint32;
		
		// For IMAGESOURCE_FILE or IMAGESOURCE_NONE
		// "\\Device\\Ramdisk{%8x-%4x-%4x-%4x-%12x}"
		_GUID id_guid;
	};
	
	// Source of volume image
	RAMDISK_IMAGESOURCE imageSource;
	
	// Bitflag; misc settings
	RAMDISK_FLAGS flags;
	
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

// Use these if you are using VC++
//#include <pshpack1.h>
//#include <poppack.h>
