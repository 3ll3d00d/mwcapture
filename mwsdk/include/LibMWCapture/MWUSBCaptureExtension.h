////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2020 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#pragma pack(push)
#pragma pack(1)

////////////////////////////////////////////////////////////////////////////////
// Magewell USB Capture Extensions

enum _MWCAP_HID_EXTENSION {
	// Product informations (General)
	MWCAP_HID_CHANNEL_INFO = 1,						// G, 	Data: uint8_t, MWCAP_CHANNEL_INFO
	MWCAP_HID_FAMILY_INFO,							// G, 	Data: uint8_t, MWCAP_USB_CAPTURE_INFO

	MWCAP_HID_VIDEO_CAPS,							// G, 	Data: uint8_t, MWCAP_VIDEO_CAPS
	MWCAP_HID_AUDIO_CAPS,							// G,	Data: uint8_t, MWCAP_AUDIO_CAPS

	// Device controls (General)
	MWCAP_HID_LED_MODE,								// S,	Data: uint8_t, uint32_t
	MWCAP_HID_POST_RECONFIG,						// S,	Data: uint8_t, uint32_t

	// Notification (General)
	MWCAP_HID_NOTIFY_ENABLE,						// S,	Data: uint8_t, MWCAP_NOTIFY_ENABLE
	MWCAP_HID_NOITFY_STATUS,						// G,	Data: uint8_t, uint64_t

	// Input source (General)
	MWCAP_HID_VIDEO_INPUT_SOURCE_ARRAY,				// G,	Data: uint8_t, MWCAP_INPUT_SOURCE_ARRAY
	MWCAP_HID_AUDIO_INPUT_SOURCE_ARRAY,				// G,	Data: uint8_t, MWCAP_INPUT_SOURCE_ARRAY

	MWCAP_HID_INPUT_SOURCE_SCAN,					// G/S,	Data: uint8_t, bool_t
	MWCAP_HID_INPUT_SOURCE_SCAN_STATE,				// G,	Data: uint8_t, bool_t
	MWCAP_HID_AV_INPUT_SOURCE_LINK,					// G/S,	Data: uint8_t, bool_t
	MWCAP_HID_VIDEO_INPUT_SOURCE,					// G/S,	Data: uint8_t, uint32_t
	MWCAP_HID_AUDIO_INPUT_SOURCE,					// G/S, Data: uint8_t, uint32_t

	// Signal status (General)
	MWCAP_HID_INPUT_SPECIFIC_STATUS,				// G,   Data: uint8_t, MWCAP_INPUT_SPECIFIC_STATUS
	MWCAP_HID_VIDEO_SIGNAL_STATUS,					// G,   Data: uint8_t, MWCAP_VIDEO_SIGNAL_STATUS
	MWCAP_HID_AUDIO_SIGNAL_STATUS,					// G,	Data: uint8_t, MWCAP_AUDIO_SIGNAL_STATUS

	// Video processing (General)
	MWCAP_HID_VIDEO_INPUT_ASPECT_RATIO,				// G/S,	Data: uint8_t, MWCAP_VIDEO_ASPECT_RATIO
	MWCAP_HID_VIDEO_INPUT_COLOR_FORMAT,				// G/S, Data: uint8_t, uint8_t
	MWCAP_HID_VIDEO_INPUT_QUANTIZATION_RANGE,		// G/S, Data: uint8_t, uint8_t

	MWCAP_HID_VIDEO_CAPTURE_CONNECTION_FORMAT,		// G,	Data: uint8_t, MWCAP_VIDEO_CONNECTION_FORMAT
	MWCAP_HID_VIDEO_CAPTURE_PROCESS_SETTINGS,		// G/S, Data: uint8_t, MWCAP_VIDEO_PROCESS_SETTINGS

	// Video output formats (General)
	MWCAP_HID_VIDEO_OUTPUT_FOURCC,					// G/S,	Data: uint8_t, MWCAP_VIDEO_OUTPUT_FOURCC
	MWCAP_HID_VIDEO_OUTPUT_FRAME_SIZE,				// G/S, Data: uint8_t, MWCAP_VIDEO_OUTPUT_FRAME_SIZE
	MWCAP_HID_VIDEO_OUTPUT_FRAME_INTERVAL,			// G/S, Data: uint8_t, MWCAP_VIDEO_OUTPUT_FRAME_INTERVAL
	MWCAP_HID_AUDIO_DEVICE_DISABLE,                 // G/S, Data: uint8_t, MWCAP_AUDIO_DEVICE_DISABLE

	// UART control (UART)
	MWCAP_HID_UART_OPEN = 0x50,						// S,   Data: uint8_t, MWCAP_UART_OPEN
	MWCAP_HID_UART_CLOSE,							// S,	Data: uint8_t, uint8_t
	MWCAP_HID_UART_DATA,							// G/S,	Data: uint8_t, MWCAP_UART_DATA
	MWCAP_HID_UART_STATUS,							// G,	Data: uint8_t, MWCAP_UART_STATUS

	// Miscs
	MWCAP_HID_CORE_TEMPERATURE = 0x58,				// G,	Data: uint8_t, int32_t (in 0.1 deg C)
	MWCAP_HID_GPIO_IN,								// G, 	Data: uint8_t, uint32_t
	MWCAP_HID_GPIO_OUT_SET,							// S,	Data: uint8_t, uint32_t
	MWCAP_HID_GPIO_OUT_CLR,							// S,	Data: uint8_t, uint32_t
	MWCAP_HID_QUERY_STATUS,							// G/S, Data: uint8_t, uint32_t
	MWCAP_HID_USB_SPEED_SEL,						// G/S, Data: uint8_t, uint8_t[8]

	// VGA/Component timings (Timing) (0x90 - 0x99)
	MWCAP_HID_VIDEO_AUTO_H_ALIGN = 0x90,			// G/S, Data: uint8_t, bool_t
	MWCAP_HID_VIDEO_SAMPLING_PHASE,					// G/S, Data: uint8_t, uint8_t (0-31, 255 triger auto adjust once)
	MWCAP_HID_VIDEO_SAMPLING_PHASE_AUTO,			// G/S, Data: uint8_t, bool_t
	MWCAP_HID_VIDEO_TIMING,							// S,	Data: uint8_t, MWCAP_VIDEO_TIMING
	MWCAP_HID_VIDEO_PREFERRED_TIMING_ARRAY,			// G,	Data: uint8_t, MWCAP_VIDEO_TIMING_ARRAY
	MWCAP_HID_VIDEO_CUSTOM_TIMING,					// S,	Data: uint8_t, MWCAP_VIDEO_CUSTOM_TIMING
	MWCAP_HID_VIDEO_CUSTOM_TIMING_ARRAY,			// G/S, Data: uint8_t, MWCAP_VIDEO_CUSTOM_TIMING_ARRAY
	MWCAP_HID_VIDEO_CUSTOM_RESOLUTION_ARRAY,		// G/S, Data: uint8_t, MWCAP_VIDEO_CUSTOM_RESOLUTION_ARRAY

	// HDMI related (HDMI) (0x9A - 0x9F)
	MWCAP_HID_EDID,									// G/S,	Data: uint8_t, uint8_t[256]
	MWCAP_HID_EDID_LOOP_THROUGH,					// G,	Data: uint8_t, uint8_t[256]
	MWCAP_HID_EDID_MODE,							// G/S, Data: uint8_t, uint8_t

	MWCAP_HID_LOOP_THROUGH,							// G,	Data: uint8_t, bool_t

	// HDMI status (HDMI) (0xA0 - 0xB0)
	MWCAP_HID_HDMI_INFOFRAME_VALID = 0xA0,			// G,	Data: uint8_t, uint32_t
	MWCAP_HID_HDMI_INFOFRAME_AVI,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_AUDIO,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_SPD,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_MS,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_VS,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_ACP,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_ISRC1,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_ISRC2,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	MWCAP_HID_HDMI_INFOFRAME_GAMUT,					// G,	Data: uint8_t, HDMI_INFOFRAME_PACKET
	
	// Audio volumes (Volume) (0xB0 - 0xC0)
	MWCAP_HID_AUDIO_VOLUME_MICROPHONE = 0xB0,		// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME
	MWCAP_HID_AUDIO_VOLUME_HEADPHONE,				// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME
	MWCAP_HID_AUDIO_VOLUME_LINE_IN,					// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME
	MWCAP_HID_AUDIO_VOLUME_LINE_OUT,				// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME
	MWCAP_HID_AUDIO_VOLUME_EMBEDDED_CAPTURE,		// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME
	MWCAP_HID_AUDIO_VOLUME_EMBEDDED_PLAYBACK,		// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME
	MWCAP_HID_AUDIO_VOLUME_USB_CAPTURE,				// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME
	MWCAP_HID_AUDIO_VOLUME_USB_PLAYBACK,			// G/S,	Data: uint8_t, MWCAP_AUDIO_VOLUME

	// Reserved (0xC0 - 0xD0)

	// Reserved for future (0xD0 - 0xE0)

	// Firmware upgrade (General)
	MWCAP_HID_FIRMWARE_STORAGE = 0xF0,				// G, 	Data: uint8_t, MWCAP_FIRMWARE_STORAGE
	MWCAP_HID_FIRMWARE_ERASE,						// S,	Data: uint8_t, MWCAP_FIRMWARE_ERASE
	MWCAP_HID_FIRMWARE_READ_ADDRESS,				// G/S, Data: uint8_t, uint32_t
	MWCAP_HID_FIRMWARE_DATA,						// G/S, Data: uint8_t, MWCAP_FIRMWARE_DATA
	MWCAP_HID_FIRMWARE_WIP,							// G,	Data: uint8_t, bool_t

	// Options
	MWCAP_HID_STATUS_IMAGE_ADDR = 0xFA,				// G/S, Data: uint8_t, MWCAP_STATUS_IMAGE_ADDR
	MWCAP_HID_STATUS_IMAGE_DATA = 0xFB,				// G/S, Data: uint8_t, MWCAP_STATUS_IMAGE_DATA
	MWCAP_HID_STATUS_IMAGE_MODE = 0xFC,				// G/S, Data: uint8_t, uint8_t
	MWCAP_HID_DEVICE_NAME_MODE = 0xFD,				// G/S, Data: uint8_t, uint8_t
	MWCAP_HID_OPTIONS_CONTROL = 0xFE,				// S,	Data: uint8_t, uint8_t

	// Extension sets supported (General)
	MWCAP_HID_EXTENSION_SUPPORTED = 0xFF			// G,	Data: uint8_t, uint32_t
};
////////////////////////////////////////////////////////////////////////////////
// Data structs

// USB additional capabilities
#define MWCAP_USB_VIDEO_CAP_HDMI_LOOP_THROUGH		0x00000001
#define MWCAP_USB_VIDEO_CAP_SDI_LOOP_THROUGH		0x00000002
#define MWCAP_USB_VIDEO_CAP_PLANAR_FORMAT			0x00000004

enum _MWCAP_USB_AUDIO_NODE {
	MWCAP_USB_AUDIO_MICROPHONE,
	MWCAP_USB_AUDIO_HEADPHONE,
	MWCAP_USB_AUDIO_LINE_IN,
	MWCAP_USB_AUDIO_LINE_OUT,
	MWCAP_USB_AUDIO_EMBEDDED_CAPTURE,
	MWCAP_USB_AUDIO_EMBEDDED_PLAYBACK,
	MWCAP_USB_AUDIO_USB_CAPTURE,
	MWCAP_USB_AUDIO_USB_PLAYBACK
};

#define MWCAP_USB_AUDIO_CAP_MICROPHONE				(1 << MWCAP_USB_AUDIO_MICROPHONE)
#define MWCAP_USB_AUDIO_CAP_HEADPHONE				(1 << MWCAP_USB_AUDIO_HEADPHONE)
#define MWCAP_USB_AUDIO_CAP_LINE_IN					(1 << MWCAP_USB_AUDIO_LINE_IN)
#define MWCAP_USB_AUDIO_CAP_LINE_OUT				(1 << MWCAP_USB_AUDIO_LINE_OUT)
#define MWCAP_USB_AUDIO_CAP_EMBEDDED_CAPTURE		(1 << MWCAP_USB_AUDIO_EMBEDDED_CAPTURE)
#define MWCAP_USB_AUDIO_CAP_EMBEDDED_PLAYBACK		(1 << MWCAP_USB_AUDIO_EMBEDDED_PLAYBACK)
#define MWCAP_USB_AUDIO_CAP_USB_CAPTURE				(1 << MWCAP_USB_AUDIO_USB_CAPTURE)
#define MWCAP_USB_AUDIO_CAP_USB_PLAYBACK			(1 << MWCAP_USB_AUDIO_USB_PLAYBACK)

#define MWCAP_NOTIFY_CONNECTION_FORMAT_CHANGED		0x20000ULL

#define MWCAP_NOTIFY_VOLUME_MICROPHONE				(1ULL << (48 + MWCAP_USB_AUDIO_MICROPHONE))
#define MWCAP_NOTIFY_VOLUME_HEADPHONE				(1ULL << (48 + MWCAP_USB_AUDIO_HEADPHONE))
#define MWCAP_NOTIFY_VOLUME_LINE_IN					(1ULL << (48 + MWCAP_USB_AUDIO_LINE_IN))
#define MWCAP_NOTIFY_VOLUME_LINE_OUT				(1ULL << (48 + MWCAP_USB_AUDIO_LINE_OUT))
#define MWCAP_NOTIFY_VOLUME_EMBEDDED_CAPTURE		(1ULL << (48 + MWCAP_USB_AUDIO_EMBEDDED_CAPTURE))
#define MWCAP_NOTIFY_VOLUME_EMBEDDED_PLAYBACK		(1ULL << (48 + MWCAP_USB_AUDIO_EMBEDDED_PLAYBACK))
#define MWCAP_NOTIFY_VOLUME_USB_CAPTURE				(1ULL << (48 + MWCAP_USB_AUDIO_USB_CAPTURE))
#define MWCAP_NOTIFY_VOLUME_USB_PLAYBACK			(1ULL << (48 + MWCAP_USB_AUDIO_USB_PLAYBACK))		

enum _MWCAP_USB_SPEED_TYPE {
	MWCAP_USB_FULL_SPEED							= 0x01,
	MWCAP_USB_HIGH_SPEED							= 0x02,
	MWCAP_USB_SUPER_SPEED_GEN1						= 0x03,
	MWCAP_USB_SUPER_SPEED_GEN2						= 0x04
};

// MWCAP_HID_STATUS_IMAGE_MODE
enum _MWCAP_STATUS_IMAGE_MODE {
	MWCAP_STATUS_IMAGE_DEFAULT,
	MWCAP_STATUS_IMAGE_BLUE,
	MWCAP_STATUS_IMAGE_BLACK,
	MWCAP_STATUS_IMAGE_CUSTOM
};

// MWCAP_HID_DEVICE_NAME_MODE
enum _MWCAP_DEVICE_NAME_MODE {
	MWCAP_DEVICE_NAME_DEFAULT,
	MWCAP_DEVICE_NAME_SERIAL_NUMBER,
	MWCAP_DEVICE_NAME_CUSTOM
};

//MWCAP_PRODUCT_ID_USB_CAPTURE_HDMI_4K_PRO the cost of changing device name was increased from 1 byte to 31 byte
#define MW_DEVICE_NAME_LENGTH  31
//an extra 1 byte represents mode MW_DEVICE_NAME_LENGTH
#define MW_DEVICE_NAME_MODE_LENGTH  32

typedef struct _MWCAP_AUDIO_DEVICE_DISABLE {
	uint8_t                                         byDevEnable;
	uint8_t                                         byDevDisable;
} MWCAP_AUDIO_DEVICE_DISABLE;

// MWCAP_HID_OPTIONS_CONTROL
enum _MWCAP_OPTIONS_CONTROL {
	MWCAP_OPTIONS_RESET,							// Reset options to default value
	MWCAP_OPTIONS_LOAD,								// Load from flash/eeprom storage
	MWCAP_OPTIONS_SAVE								// Save to flash/eeprom storage
};

// MWCAP_HID_EXTENSION_SET
#define MWCAP_HID_EXTENSION_HDMI					0x00000001
#define MWCAP_HID_EXTENSION_TIMING					0x00000002
#define MWCAP_HID_EXTENSION_VOLUME					0x00000004
#define MWCAP_HID_EXTENSION_UART					0x00000008

// Basic types
typedef int8_t										bool_t;

typedef struct _MWSIZE {
	int16_t											cx;
	int16_t											cy;
} MWSIZE;

typedef struct _MWRECT {
	int16_t											left;
	int16_t											top;
	int16_t											right;
	int16_t											bottom;
} MWRECT;

/**
 * @ingroup group_variables_struct
 * @brief MWUSBCAP_CAPTURE_INFO
 * @details Records connection information of USB device.\n
 * 			Related function(s): [MWGetFamilyInfoByIndex](@ref MWGetFamilyInfoByIndex)\n
 * 					[MWGetFamilyInfo](@ref MWGetFamilyInfo)
 */
typedef struct _MWCAP_USB_CAPTURE_INFO {
	uint8_t											byUSBSpeed;			///<USB speed: 1.0, 2.0, 3.0
	uint8_t											byBoardIndex;		///<The value is 0
	uint32_t										cbTotalMemorySize;	///<Total memory size
	uint32_t										cbFreeMemorySize;	///<Free memory size
} MWCAP_USB_CAPTURE_INFO;

typedef struct _MWCAP_NOTIFY_ENABLE {
	bool_t											bInterrupt;
	uint64_t										ullEnableBits;
} MWCAP_NOTIFY_ENABLE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_FIRMWARE_STORAGE_USB
 * @details Records firmware storage information of the USB capture card\n
 * Related data type: #MWCAP_FIRMWARE_STORAGE
*/
typedef struct _MWCAP_FIRMWARE_STORAGE_USB{
	DWORD											cbStorage;								///<Length of firmware storage area
	DWORD											cbEraseBlock;							///<Length of erased area
	DWORD											cbProgramBlock;							///<Length of program block storage area
	DWORD											cbHeaderOffset;							///<Offset of firmware header

	DWORD											cbFirmwareOffset;						///<Firmware offset
	DWORD											cbDriverOffset;							///<Drive offset
	DWORD											cbMRFSOffset;							///<MRFS offset
}MWCAP_FIRMWARE_STORAGE_USB;

typedef struct _MWCAP_FIRMWARE_DATA {
	uint32_t										dwAddress;								// Must write in a 256 page
	uint16_t										cbData;
	uint8_t											abyData[256];
} MWCAP_FIRMWARE_DATA;

// Input source
typedef struct _MWCAP_INPUT_SOURCE_ARRAY {
	uint8_t											byNumInputSource;
	uint32_t										adwInputSources[16];
} MWCAP_INPUT_SOURCE_ARRAY;

typedef struct _MWCAP_VIDEO_TIMING_ARRAY {
	uint8_t											byNumTimings;
	MWCAP_VIDEO_TIMING								aTimings[8];
} MWCAP_VIDEO_TIMING_ARRAY;

typedef struct _MWCAP_VIDEO_CUSTOM_TIMING_ARRAY {
	uint8_t											byNumCustomTimings;
	MWCAP_VIDEO_CUSTOM_TIMING						aCustomTimings[8];
} MWCAP_VIDEO_CUSTOM_TIMING_ARRAY;

typedef struct _MWCAP_VIDEO_CUSTOM_RESOLUTION_ARRAY {
	uint8_t											byNumCustomResolutions;
	MWSIZE											aCustomResolutions[16];
} MWCAP_VIDEO_CUSTOM_RESOLUTION_ARRAY;

#define MWCAP_MAX_NUM_AUDIO_CHANNEL					16

typedef struct _MWCAP_AUDIO_VOLUME {
	uint8_t											byChannels;
	uint8_t											byReserved;
	int16_t											sVolumeMin;
	int16_t											sVolumeMax;
	int16_t											sVolumeStep;

	bool_t											abMute[MWCAP_MAX_NUM_AUDIO_CHANNEL];
	int16_t											asVolume[MWCAP_MAX_NUM_AUDIO_CHANNEL];
} MWCAP_AUDIO_VOLUME;





#define MWCAP_MAX_NUM_VIDEO_OUTPUT_FOURCC			3
#define MWCAP_MAX_NUM_VIDEO_OUTPUT_FRAME_SIZE		24
#define MWCAP_MAX_NUM_VIDEO_OUTPUT_FRAME_INTERVAL	8

typedef struct _MWCAP_VIDEO_OUTPUT_FOURCC {
	uint8_t											byCount;
	uint32_t										adwFOURCCs[MWCAP_MAX_NUM_VIDEO_OUTPUT_FOURCC];
} MWCAP_VIDEO_OUTPUT_FOURCC;

typedef struct _MWCAP_VIDEO_OUTPUT_FRAME_SIZE {
	uint8_t											byCount;
	uint8_t											byDefault;
	MWSIZE											aSizes[MWCAP_MAX_NUM_VIDEO_OUTPUT_FRAME_SIZE];
} MWCAP_VIDEO_OUTPUT_FRAME_SIZE;

typedef struct _MWCAP_VIDEO_OUTPUT_FRAME_INTERVAL {
	uint8_t											byCount;
	uint8_t											byDefault;
	uint32_t										adwIntervals[MWCAP_MAX_NUM_VIDEO_OUTPUT_FRAME_INTERVAL];
} MWCAP_VIDEO_OUTPUT_FRAME_INTERVAL;

typedef struct _MWCAP_UART_OPEN {
	uint32_t										dwBaudrate;
	uint8_t 										byStopBitLen;
	uint8_t 										byParityCheck;
	uint8_t 										byBitCount;
} MWCAP_UART_OPEN;

typedef struct _MWCAP_UART_STATUS {
	uint8_t											bRxOverflow;
	uint8_t											byTxCount;
	uint8_t											byRxCount;
} MWCAP_UART_STATUS;

typedef struct _MWCAP_UART_DATA {
	uint8_t											bRxParityError;
	uint8_t											bRxOverflow;
	uint8_t											cbData;
	uint8_t											abyData[128];
} MWCAP_UART_DATA;

#pragma pack(pop)
