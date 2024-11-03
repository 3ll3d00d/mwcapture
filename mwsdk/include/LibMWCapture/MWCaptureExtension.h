////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2020 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#pragma warning(push)
#pragma warning(disable : 4201)
#include <ks.h>
#pragma warning(pop)

#include "MWCommon.h"
#include "MWIEC60958.h"
#include "MWSMPTE.h"
#include "MWHDMIPackets.h"

#pragma pack(push)
#pragma pack(1)

////////////////////////////////////////////////////////////////////////////////
// Magewell Capture Extensions

typedef enum _MWCAP_KSPROPERTY_EXTENSION {
	// Product informations
	MWCAP_KSPROPERTY_CHANNEL_INFO,								// G, 	Prop: KSPROPERTY,								Data: MWCAP_CHANNEL_INFO
	MWCAP_KSPROPERTY_FAMILY_INFO,								// G, 	Prop: KSPROPERTY,								Data: MWCAP_PRO_CAPTURE_INFO, MWCAP_ECO_CAPTURE_INFO

	MWCAP_KSPROPERTY_VIDEO_CAPS,								// G, 	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CAPS
	MWCAP_KSPROPERTY_AUDIO_CAPS,								// G,	Prop: KSPROPERTY,								Data: MWCAP_AUDIO_CAPS

	MWCAP_KSPROPERTY_DEVICE_INSTANCE_ID,						// G,	Prop: KSPROPERTY,								Data: WCHAR[_MAX_PATH]

	// Firmware upgrade
	MWCAP_KSPROPERTY_FIRMWARE_STORAGE,							// G, 	Prop: KSPROPERTY,								Data: MWCAP_FIRMWARE_STORAGE
	MWCAP_KSPROPERTY_FIRMWARE_ERASE,							// S,	Prop: KSPROPERTY,								Data: MWCAP_FIRMWARE_ERASE
	MWCAP_KSPROPERTY_FIRMWARE_DATA,								// G/S, Prop: MWCAP_KSPROPERTY_DWORD_S, 				Data: BYTE[]

	// Device misc controls
	MWCAP_KSPROPERTY_LED_MODE,									// S,	Prop: KSPROPERTY,								Data: DWORD
	MWCAP_KSPROPERTY_POST_RECONFIG,								// S,	Prop: KSPROPERTY,								Data: DWORD

	// Hardware timer
	MWCAP_KSPROPERTY_TIME,										// G/S, Prop: KSPROPERTY,								Data: LONGLONG
	MWCAP_KSPROPERTY_TIME_REGULATION,							// S,	Prop: KSPROPERTY,								Data: LONGLONG

	MWCAP_KSPROPERTY_TIMER_REGISTRATION,						// G,	Prop: MWCAP_KSPROPERTY_PTR64_S,					Data: MWCAP_PTR64
	MWCAP_KSPROPERTY_TIMER_DEREGISTRATION,						// S,	Prop: KSPROPERTY,								Data: MWCAP_PTR64
	MWCAP_KSPROPERTY_TIMER_EXPIRE_TIME,							// S,	Prop: KSPROPERTY,								Data: MWCAP_TIMER_EXPIRE_TIME

	// Notifications
	MWCAP_KSPROPERTY_NOTIFY_REGISTRATION,						// G,	Prop: MWCAP_KSPROPERTY_NOTIFY_REGISTRATION_S,	Data: MWCAP_PTR64
	MWCAP_KSPROPERTY_NOTIFY_DEREGISTRATION,						// S,	Prop: KSPROPERTY,								Data: MWCAP_PTR64
	MWCAP_KSPROPERTY_NOTIFY_STATUS,								// G,	Prop: MWCAP_KSPROPERTY_PTR64_S,					Data: ULONGLONG
	MWCAP_KSPROPERTY_NOTIFY_ENABLE,								// S,	Prop: MWCAP_KSPROPERTY_PTR64_S,					Data: ULONGLONG

	// Video frame information
	MWCAP_KSPROPERTY_VIDEO_BUFFER_INFO,							// G,   Prop: KSPROPERTY,								Data: MWCAP_VIDEO_BUFFER_INFO
	MWCAP_KSPROPERTY_VIDEO_FRAME_INFO,							// G,   Prop: MWCAP_KSPROPERTY_DWORD_S,					Data: MWCAP_VIDEO_FRAME_INFO

	// Video capture
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_OPEN,						// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CAPTURE_OPEN
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_FRAME,						// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CAPTURE_FRAME
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_STATUS,						// G,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CAPTURE_STATUS
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_CLOSE,						// S,	Prop: KSPROPERTY,								Data: None

	// Audio capture
	MWCAP_KSPROPERTY_AUDIO_CAPTURE_OPEN,						// S,	Prop: KSPROPERTY,								Data: None
	MWCAP_KSPROPERTY_AUDIO_CAPTURE_FRAME,						// G,	Prop: KSPROPERTY,								Data: MWCAP_AUDIO_CAPTURE_FRAME
	MWCAP_KSPROPERTY_AUDIO_CAPTURE_CLOSE,						// S,	Prop: KSPROPERTY,								Data: None

	// Input source
	MWCAP_KSPROPERTY_VIDEO_INPUT_SOURCE_COUNT,					// G,	Prop: KSPROPERTY, 								Data: DWORD
	MWCAP_KSPROPERTY_VIDEO_INPUT_SOURCE_ARRAY,					// G,	Prop: KSPROPERTY,								Data: DWORD[]

	MWCAP_KSPROPERTY_AUDIO_INPUT_SOURCE_COUNT,					// G,	Prop: KSPROPERTY, 								Data: DWORD
	MWCAP_KSPROPERTY_AUDIO_INPUT_SOURCE_ARRAY,					// G,	Prop: KSPROPERTY,								Data: DWORD[]

	MWCAP_KSPROPERTY_INPUT_SOURCE_SCAN,							// G/S,	Prop: KSPROPERTY,								Data: BOOLEAN
	MWCAP_KSPROPERTY_INPUT_SOURCE_SCAN_STATE,					// G,	Prop: KSPROPERTY,								Data: BOOLEAN
	MWCAP_KSPROPERTY_AV_INPUT_SOURCE_LINK,						// G/S,	Prop: KSPROPERTY,								Data: BOOLEAN
	MWCAP_KSPROPERTY_VIDEO_INPUT_SOURCE,						// G/S,	Prop: KSPROPERTY,								Data: DWORD
	MWCAP_KSPROPERTY_AUDIO_INPUT_SOURCE,						// G/S, Prop: KSPROPERTY,								Data: DWORD

	// EDID
	MWCAP_KSPROPERTY_EDID,										// G/S,	Prop: KSPROPERTY,								Data: BYTE[]

	// Signal status
	MWCAP_KSPROPERTY_INPUT_SPECIFIC_STATUS,						// G,   Prop: KSPROPERTY,								Data: MWCAP_INPUT_SPECIFIC_STATUS
	MWCAP_KSPROPERTY_VIDEO_SIGNAL_STATUS,						// G,   Prop: KSPROPERTY,								Data: MWCAP_VIDEO_SIGNAL_STATUS
	MWCAP_KSPROPERTY_AUDIO_SIGNAL_STATUS,						// G,	Prop: KSPROPERTY,								Data: MWCAP_AUDIO_SIGNAL_STATUS

	// HDMI status
	MWCAP_KSPROPERTY_HDMI_INFOFRAME_VALID,						// G,	Prop: KSPROPERTY,								Data: DWORD
	MWCAP_KSPROPERTY_HDMI_INFOFRAME_PACKET,						// G,	Prop: MWCAP_KSPROPERTY_DWORD_S,					Data: HDMI_INFOFRAME_PACKET

	// Video processing
	MWCAP_KSPROPERTY_VIDEO_INPUT_ASPECT_RATIO,					// G/S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_ASPECT_RATIO
	MWCAP_KSPROPERTY_VIDEO_INPUT_COLOR_FORMAT,					// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_COLOR_FORMAT
	MWCAP_KSPROPERTY_VIDEO_INPUT_QUANTIZATION_RANGE,			// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_QUANTIZATION_RANGE

	// Video processing (NOT functional for DirectKS)
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_CONNECTION_FORMAT,			// G,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CONNECTION_FORMAT
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_PROCESS_PRESET,				// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_PROCESS_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_PROCESS_SETTINGS,			// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_PROCESS_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_OSD_PRESET,					// G/S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_OSD_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_OSD_SETTINGS,				// G/S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_OSD_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_CAPTURE_OSD_IMAGE,					// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_OSD_IMAGE

	MWCAP_KSPROPERTY_VIDEO_PREVIEW_CONNECTION_FORMAT,			// G,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CONNECTION_FORMAT
	MWCAP_KSPROPERTY_VIDEO_PREVIEW_PROCESS_PRESET,				// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_PROCESS_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_PREVIEW_PROCESS_SETTINGS,			// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_PROCESS_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_PREVIEW_OSD_PRESET,					// G/S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_OSD_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_PREVIEW_OSD_SETTINGS,				// G/S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_OSD_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_PREVIEW_OSD_IMAGE,					// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_OSD_IMAGE

	// Temperature
	MWCAP_KSPROPERTY_CORE_TEMPERATURE,							// G,	Prop: KSPROPERTY,								Data: LONG	(in 0.1 deg C)

	// Image upload, can be use as source image or OSD image
	MWCAP_KSPROPERTY_VIDEO_CREATE_IMAGE,						// G,	Prop: MWCAP_KSPROPERTY_VIDEO_CREATE_IMAGE_S,	Data: MWCAP_PTR64
	MWCAP_KSPROPERTY_VIDEO_OPEN_IMAGE,							// G,	Prop: MWCAP_KSPROPERTY_PTR64_S,					Data: LONG
	MWCAP_KSPROPERTY_VIDEO_CLOSE_IMAGE,							// G,	Prop: MWCAP_KSPROPERTY_PTR64_S,					Data: LONG
	MWCAP_KSPROPERTY_VIDEO_UPLOAD_IMAGE,						// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_UPLOAD_IMAGE

	// VGA/Component timings
	MWCAP_KSPROPERTY_VIDEO_AUTO_H_ALIGN,						// G/S, Prop: KSPROPERTY,								Data: BOOLEAN
	MWCAP_KSPROPERTY_VIDEO_SAMPLING_PHASE,						// G/S, Prop: KSPROPERTY,								Data: BYTE (0-31, 255 triger auto adjust once)
	MWCAP_KSPROPERTY_VIDEO_SAMPLING_PHASE_AUTO,					// G/S, Prop: KSPROPERTY,								Data: BOOLEAN
	MWCAP_KSPROPERTY_VIDEO_TIMING,								// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_TIMING
	MWCAP_KSPROPERTY_VIDEO_PREFERRED_TIMING_ARRAY,				// G,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_TIMING[MWCAP_VIDEO_MAX_NUM_PREFERRED_TIMINGS]
	MWCAP_KSPROPERTY_VIDEO_CUSTOM_TIMING,						// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CUSTOM_TIMING
	MWCAP_KSPROPERTY_VIDEO_CUSTOM_TIMING_COUNT,					// G,	Prop: KSPROPERTY,								Data: DWORD
	MWCAP_KSPROPERTY_VIDEO_CUSTOM_TIMING_ARRAY,					// G/S, Prop: KSPROPERTY,								Data: MWCAP_VIDEO_CUSTOM_TIMING[]
	MWCAP_KSPROPERTY_VIDEO_CUSTOM_RESOLUTION_COUNT,				// G,	Prop: KSPROPERTY,								Data: DWORD
	MWCAP_KSPROPERTY_VIDEO_CUSTOM_RESOLUTION_ARRAY,				// G/S, Prop: KSPROPERTY,								Data: SIZE[]

	// Video capture enhancement
	MWCAP_KSPROPERTY_VIDEO_PIN_BUFFER,							// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_PIN_BUFFER
	MWCAP_KSPROPERTY_VIDEO_UNPIN_BUFFER,						// S,	Prop: KSPROPERTY,								Data: MWCAP_PTR64

	// SDI ANC Packet
	MWCAP_KSPROPERTY_SET_SDI_ANC_TYPE,							// S,	Prop: KSPROPERTY,								Data: MWCAP_SDI_ANC_TYPE
	MWCAP_KSPROPERTY_GET_SDI_ANC_PACKET,						// G,	Prop: KSPROPERTY,								Data: MWCAP_SDI_ANC_PACKET

	// Eco video capture
	MWCAP_KSPROPERTY_VIDEO_ECO_CAPTURE_OPEN,					// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_ECO_CAPTURE_OPEN
	MWCAP_KSPROPERTY_VIDEO_ECO_CAPTURE_SETTINGS,				// S/G,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_ECO_CAPTURE_SETTINGS
	MWCAP_KSPROPERTY_VIDEO_ECO_CAPTURE_FRAME,					// S,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_ECO_CAPTURE_FRAME
	MWCAP_KSPROPERTY_VIDEO_ECO_CAPTURE_STATUS,					// G,	Prop: KSPROPERTY,								Data: MWCAP_VIDEO_ECO_CAPTURE_STATUS
	MWCAP_KSPROPERTY_VIDEO_ECO_CAPTURE_CLOSE					// S,	Prop: KSPROPERTY,								Data: None
} MWCAP_KSPROPERTY_EXTENSION;

////////////////////////////////////////////////////////////////////////////////
// Magewell Video Capture Filter Extension Property Set

#define STATIC_MWCAP_PROPSETID_FILTER \
    0x3a45b930, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("3A45B930-FF5C-42AA-A5A2-43771B93EBEF", MWCAP_PROPSETID_FILTER);
#define MWCAP_PROPSETID_FILTER DEFINE_GUIDNAMED(MWCAP_PROPSETID_FILTER)

////////////////////////////////////////////////////////////////////////////////
// Magewell Video Capture Pro Family Specific Extension Property Set

#define STATIC_MWCAP_PROPSETID_PRO \
    0x3a45b931, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("3A45B931-FF5C-42AA-A5A2-43771B93EBEF", MWCAP_PROPSETID_PRO);
#define MWCAP_PROPSETID_PRO DEFINE_GUIDNAMED(MWCAP_PROPSETID_PRO)

////////////////////////////////////////////////////////////////////////////////
// Magewell Video Capture HDMI Extension Property Set

#define STATIC_MWCAP_PROPSETID_HDMI \
    0x3a45b932, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("3A45B932-FF5C-42AA-A5A2-43771B93EBEF", MWCAP_PROPSETID_HDMI);
#define MWCAP_PROPSETID_HDMI DEFINE_GUIDNAMED(MWCAP_PROPSETID_HDMI)

////////////////////////////////////////////////////////////////////////////////
// Magewell Video Capture Timing Extension Property Set

#define STATIC_MWCAP_PROPSETID_TIMING \
    0x3a45b933, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("3A45B933-FF5C-42AA-A5A2-43771B93EBEF", MWCAP_PROPSETID_TIMING);
#define MWCAP_PROPSETID_TIMING DEFINE_GUIDNAMED(MWCAP_PROPSETID_TIMING)

////////////////////////////////////////////////////////////////////////////////
// Magewell Video Capture OSD Extension Property Set

#define STATIC_MWCAP_PROPSETID_OSD \
    0x3a45b934, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("3A45B934-FF5C-42AA-A5A2-43771B93EBEF", MWCAP_PROPSETID_OSD);
#define MWCAP_PROPSETID_OSD DEFINE_GUIDNAMED(MWCAP_PROPSETID_OSD)

////////////////////////////////////////////////////////////////////////////////
// Magewell Video Capture Video Extension Property Set

#define STATIC_MWCAP_PROPSETID_VIDEO \
    0x3a45b935, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("3A45B935-FF5C-42AA-A5A2-43771B93EBEF", MWCAP_PROPSETID_VIDEO);
#define MWCAP_PROPSETID_VIDEO DEFINE_GUIDNAMED(MWCAP_PROPSETID_VIDEO)

////////////////////////////////////////////////////////////////////////////////
// Media Types
#define STATIC_KSDATAFORMAT_TYPE_SDI_CC708 \
    0x4a45b931, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("4A45B931-FF5C-42AA-A5A2-43771B93EBEF", KSDATAFORMAT_TYPE_SDI_CC708);
#define KSDATAFORMAT_TYPE_SDI_CC708 DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_SDI_CC708)

#define STATIC_KSDATAFORMAT_SUBTYPE_SDI_CC708_Packet \
    0x4a45b932, 0xff5c, 0x42aa, 0xa5, 0xa2, 0x43, 0x77, 0x1b, 0x93, 0xeb, 0xef
DEFINE_GUIDSTRUCT("4A45B932-FF5C-42AA-A5A2-43771B93EBEF", KSDATAFORMAT_SUBTYPE_SDI_CC708_Packet);
#define KSDATAFORMAT_SUBTYPE_SDI_CC708_Packet DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_SDI_CC708_Packet)

////////////////////////////////////////////////////////////////////////////////
// Data structs

// Common definitions
#ifdef _MSC_VER
typedef VOID * __ptr64								MWCAP_PTR64;
#else
typedef uint64_t MWCAP_PTR64;
#endif
/**
 * @ingroup group_variables_macro
 * @brief Defines audio or video input source
 * @param [in]	type	Input source type
 * @param [in]	index	Index of input source
 * @details Defines the input source by the type or index. The audio input source type refers to #MWCAP_AUDIO_INPUT_TYPE. The video input source type refers to #MWCAP_VIDEO_INPUT_TYPE\n
*/													                                            	
#define INPUT_SOURCE(type, index)					(((type) << 8) | ((index) & 0xFF))

/**
 * @ingroup group_variables_macro
 * @brief Gets type variables from input sources 
 * @param[in]	source	Input source 
 * @details Extracting type variables from input sources
*/	
#define INPUT_TYPE(source)							((source) >> 8)

/**
 * @ingroup group_variables_macro
 * @brief Gets input source index
 * @param[in]	source	Input source 
 * @details Extracting index variables from input sources
*/	
#define INPUT_INDEX(source)							((source) & 0xFF)

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_PRODUCT_ID
 * @details Distinguishes capture devices.
*/													
typedef enum _MWCAP_PRODUCT_ID {
	MWCAP_PRODUCT_ID_PRO_CAPTURE_AIO				= 0x00000102,///< Pro Capture AIO, one-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DVI				= 0x00000103,///< Pro Capture DVI, one-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_HDMI				= 0x00000104,///< Pro Capture HDMI, one-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_SDI				= 0x00000105,///< Pro Capture SDI, one-channel HD capture card capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_SDI			= 0x00000106,///< Pro Capture Dual SDI, two-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_DVI			= 0x00000107,///< Pro Capture Dual DVI, two-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_HDMI			= 0x00000108,///< Pro Capture Dual HDMI, two-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_QUAD_SDI			= 0x00000109,///< Pro Capture Quad SDI, Four-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_QUAD_HDMI			= 0x00000110,///< Pro Capture Quad HDMI, Four-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_MINI_HDMI			= 0x00000111,///< Pro Capture Mini HDMI, one-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_HDMI_4K			= 0x00000112,///< Pro Capture HDMI 4K, one-channel 4K capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_MINI_SDI			= 0x00000113,///< Pro Capture Mini SDI, one-channel HD capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_AIO_4K_PLUS		= 0x00000114,///< Pro Capture AIO 4K Plus, one-channel 4K capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_HDMI_4K_PLUS		= 0x00000115,///< Pro Capture HDMI 4K Plus, one-channel 4K capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DVI_4K				= 0x00000116,///< Pro Capture DVI 4K, one-channel 4K capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_AIO_4K				= 0x00000117,///< Pro Capture AIO 4K, one-channel 4K capture card
	MWCAP_PRODUCT_ID_PRO_CAPTURE_SDI_4K_PLUS		= 0x00000118,///< Pro Capture SDI 4K Plus, one-channel 4K capture card

	MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_AIO_BETA		= 0x00000121,///< Pro Capture Dual AIO Beta, reserved
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_AIO			= 0x00000123,///< Pro Capture Dual AIO
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DP_4K_PLUS_TBT		= 0x00000124,///< Pro Capture DP 4K PLUS TBT, reserved 
	MWCAP_PRODUCT_ID_PRO_CAPTURE_HEXA_CVBS			= 0x00000125,///< Pro Capture HEXA CVBS  
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_HDMI_4K_PLUS	= 0x00000126,///< Pro Capture Dual HDMI 4K Plus  
	MWCAP_PRODUCT_ID_PRO_CAPTURE_DUAL_SDI_4K_PLUS	= 0x00000127,///< Pro Capture Dual SDI 4K Plus  
	MWCAP_PRODUCT_ID_ECO_CAPTURE_OCTA_SDI			= 0x00000150,///<ECO CAPTURE OCTA SDI
	MWCAP_PRODUCT_ID_ECO_CAPTURE_DUAL_HDMI_M2		= 0x00000151,///<Eco Capture Dual HDMI M.2
	MWCAP_PRODUCT_ID_ECO_CAPTURE_HDMI_4K_M2			= 0x00000152,///<Eco Capture HDMI 4K M.2
	MWCAP_PRODUCT_ID_ECO_CAPTURE_DUAL_SDI_M2		= 0x00000153,///<Eco Capture Dual SDI M.2
	MWCAP_PRODUCT_ID_ECO_CAPTURE_QUAD_SDI_M2		= 0x00000154,///<Eco Capture Quad SDI M.2
	MWCAP_PRODUCT_ID_ECO_CAPTURE_QL_SDI_4K_M2		= 0x00000155,

	MWCAP_PRODUCT_ID_USB_CAPTURE_HDMI_PLUS 			= 0x00000204,///<USB Capture HDMI Plus
	MWCAP_PRODUCT_ID_USB_CAPTURE_SDI_PLUS 			= 0x00000205,///<USB Capture SDI Plus  
	MWCAP_PRODUCT_ID_USB_CAPTURE_HDMI 				= 0x00000206,///<USB Capture HDMI  
	MWCAP_PRODUCT_ID_USB_CAPTURE_SDI 				= 0x00000207,///<USB Capture SDI  
	MWCAP_PRODUCT_ID_USB_CAPTURE_DVI 				= 0x00000208,///<USB Capture DVI 
	MWCAP_PRODUCT_ID_USB_CAPTURE_HDMI_4K_PLUS 		= 0x00000209,///<USB Capture HDMI 4K Plus  
	MWCAP_PRODUCT_ID_USB_CAPTURE_SDI_4K_PLUS 		= 0x00000210,///<USB Capture SDI 4K Plus 
	MWCAP_PRODUCT_ID_USB_CAPTURE_AIO 				= 0x00000211,///<USB Capture AIO 
	MWCAP_PRODUCT_ID_USB_CAPTURE_AIO_4K 			= 0x00000212 ///<USB Capture AIO 4K  
} MWCAP_PRODUCT_ID;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_INPUT_TYPE
 * @details Defines input interface mask of video channel
*/                     
typedef enum _MWCAP_VIDEO_INPUT_TYPE {          
	MWCAP_VIDEO_INPUT_TYPE_NONE						= 0x00,///<Input interface without signal
	MWCAP_VIDEO_INPUT_TYPE_HDMI						= 0x01,///<HDMI input signal
	MWCAP_VIDEO_INPUT_TYPE_VGA						= 0x02,///<VGA input signal
	MWCAP_VIDEO_INPUT_TYPE_SDI						= 0x04,///<SDI input signal
	MWCAP_VIDEO_INPUT_TYPE_COMPONENT				= 0x08,///<Component input signal
	MWCAP_VIDEO_INPUT_TYPE_CVBS						= 0x10,///<CVBS input signal
	MWCAP_VIDEO_INPUT_TYPE_YC						= 0x20 ///<YC input signal
} MWCAP_VIDEO_INPUT_TYPE;	

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_AUDIO_INPUT_TYPE
 * @details Defines input interface mask of audio channel
*/  
typedef enum _MWCAP_AUDIO_INPUT_TYPE {	
	MWCAP_AUDIO_INPUT_TYPE_NONE						= 0x00,///<input interface without signal
	MWCAP_AUDIO_INPUT_TYPE_HDMI						= 0x01,///<HDMI input signal
	MWCAP_AUDIO_INPUT_TYPE_SDI						= 0x02,///<SDI input signal
	MWCAP_AUDIO_INPUT_TYPE_LINE_IN					= 0x04,///<line in
	MWCAP_AUDIO_INPUT_TYPE_MIC_IN					= 0x08 ///<mic in
} MWCAP_AUDIO_INPUT_TYPE;							

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_PCIE_LINK_TYPE
 * @details Defines the connection type of PCIE and hose
*/  													
typedef enum _MWCAP_PCIE_LINK_TYPE {				
	MWCAP_PCIE_LINK_GEN_1							= 0x01,///<PCI-e 1.0 
	MWCAP_PCIE_LINK_GEN_2							= 0x02,///<PCI-e 2.0 
	MWCAP_PCIE_LINK_GEN_3							= 0x03,///<PCI-e 3.0 
	MWCAP_PCIE_LINK_GEN_4							= 0x04 ///<PCI-e 4.0
} MWCAP_PCIE_LINK_TYPE;								

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_TIMING_TYPE
 * @details Defines video timing type
 */ 													
typedef enum _MWCAP_VIDEO_TIMING_TYPE {					
	MWCAP_VIDEO_TIMING_NONE							= 0x00000000,///<No timing of video channels
	MWCAP_VIDEO_TIMING_LEGACY						= 0x00000001,///<LEGACY timing
	MWCAP_VIDEO_TIMING_DMT							= 0x00000002,///<DMT timing
	MWCAP_VIDEO_TIMING_CEA							= 0x00000004,///<CEA timing
	MWCAP_VIDEO_TIMING_GTF							= 0x00000008,///<GTF timing
	MWCAP_VIDEO_TIMING_CVT							= 0x00000010,///<CVT timing
	MWCAP_VIDEO_TIMING_CVT_RB						= 0x00000020,///<CVT_RB timing
	MWCAP_VIDEO_TIMING_FAILSAFE						= 0x00002000 ///<FAILSAFE timing
} MWCAP_VIDEO_TIMING_TYPE;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_COLOR_FORMAT
 * @details Defines video color format
*/ 	
typedef enum _MWCAP_VIDEO_COLOR_FORMAT {         	
	MWCAP_VIDEO_COLOR_FORMAT_UNKNOWN				= 0x00,///<unknown color format
	MWCAP_VIDEO_COLOR_FORMAT_RGB					= 0x01,///<RGB
	MWCAP_VIDEO_COLOR_FORMAT_YUV601					= 0x02,///<YUV601
	MWCAP_VIDEO_COLOR_FORMAT_YUV709					= 0x03,///<YUV709
	MWCAP_VIDEO_COLOR_FORMAT_YUV2020				= 0x04,///<YUV2020
	MWCAP_VIDEO_COLOR_FORMAT_YUV2020C				= 0x05 ///<YUV2020C
} MWCAP_VIDEO_COLOR_FORMAT;							 	

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_QUANTIZATION_RANGE
 * @details Defines quantization range
*/ 														
typedef enum _MWCAP_VIDEO_QUANTIZATION_RANGE {			
	MWCAP_VIDEO_QUANTIZATION_UNKNOWN				= 0x00,///<the default quantization range
	MWCAP_VIDEO_QUANTIZATION_FULL					= 0x01,///<Full range, which has 8-bit data. The black-white color range is 0-255/1023/4095/65535.
	MWCAP_VIDEO_QUANTIZATION_LIMITED				= 0x02 ///<Limited range, which has 8-bit data. The black-white color range is 16/64/256/4096-235(240)/940(960)/3760(3840)/60160(61440).
} MWCAP_VIDEO_QUANTIZATION_RANGE;					

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_SATURATION_RANGE
 * @details Defines video saturation range
*/ 														
typedef enum _MWCAP_VIDEO_SATURATION_RANGE {		
	MWCAP_VIDEO_SATURATION_UNKNOWN					= 0x00,///<The default saturation range
	MWCAP_VIDEO_SATURATION_FULL						= 0x01,///<Full range, which has 8-bit data. The black-white color range is 0-255/1023/4095/65535
	MWCAP_VIDEO_SATURATION_LIMITED					= 0x02,///<Limited range, which has 8-bit data. The black-white color range is 16/64/256/4096-235(240)/940(960)/3760(3840)/60160(61440)
	MWCAP_VIDEO_SATURATION_EXTENDED_GAMUT			= 0x03 ///<Extended range, which has 8-bit data. The black-white color range is 1/4/16/256-254/1019/4079/65279
} MWCAP_VIDEO_SATURATION_RANGE;						

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_FRAME_TYPE
 * @details Defines video frame type
*/ 	
typedef enum _MWCAP_VIDEO_FRAME_TYPE {
	MWCAP_VIDEO_FRAME_2D							= 0x00,///<2D video frame
	MWCAP_VIDEO_FRAME_3D_TOP_AND_BOTTOM_FULL		= 0x01,///<Top-and-Bottom 3D  video frame at full resolution
	MWCAP_VIDEO_FRAME_3D_TOP_AND_BOTTOM_HALF		= 0x02,///<Top-and-Bottom 3D  video frame at half resolution
	MWCAP_VIDEO_FRAME_3D_SIDE_BY_SIDE_FULL			= 0x03,///<Full side-by-side 3D video frame
	MWCAP_VIDEO_FRAME_3D_SIDE_BY_SIDE_HALF			= 0x04 ///<Half side-by-side 3D video frame
} MWCAP_VIDEO_FRAME_TYPE;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_DEINTERLACE_MODE
 * @details Defines the deinterlace mode
*/ 
typedef enum _MWCAP_VIDEO_DEINTERLACE_MODE {
	MWCAP_VIDEO_DEINTERLACE_WEAVE					= 0x00,///<Weave mode
	MWCAP_VIDEO_DEINTERLACE_BLEND					= 0x01,///<Blend mode
	MWCAP_VIDEO_DEINTERLACE_TOP_FIELD				= 0x02,///<Only uses top subframe data
	MWCAP_VIDEO_DEINTERLACE_BOTTOM_FIELD			= 0x03 ///<Only uses bottom subframe data
} MWCAP_VIDEO_DEINTERLACE_MODE;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE
 * @details Defines the aspect ratio conversion
*/ 
typedef enum _MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE {
	MWCAP_VIDEO_ASPECT_RATIO_IGNORE					= 0x00,///<Ignore: Ignores the original aspect ratio and stretches to full-screen.
	MWCAP_VIDEO_ASPECT_RATIO_CROPPING				= 0x01,///<Cropping: Expands to full-screen and remove parts of the image when necessary to keep the original aspect ratio.
	MWCAP_VIDEO_ASPECT_RATIO_PADDING				= 0x02 ///<Padding: Fits to screen and add black borders to keep the original aspect ratio.
} MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_SYNC_TYPE
 * @details Defines video sync type
 */ 
typedef enum _MWCAP_VIDEO_SYNC_TYPE {
	VIDEO_SYNC_ALL									= 0x07,///<
	VIDEO_SYNC_HS_VS								= 0x01,///<
	VIDEO_SYNC_CS									= 0x02,///<
	VIDEO_SYNC_EMBEDDED								= 0x04 ///<
} MWCAP_VIDEO_SYNC_TYPE;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_EDID_MODE
 * @details Defines EDID
*/ 
typedef enum _MWCAP_EDID_MODE {
	MWCAP_EDID_MODE_FIXED							= 0x01,///<
	MWCAP_EDID_MODE_KEEP_LAST						= 0x02,///<
	MWCAP_EDID_MODE_ADD_AUDIO						= 0x04,///<
	MWCAP_EDID_MODE_LIMIT_PIXEL_CLOCK				= 0x08 ///<
} MWCAP_EDID_MODE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_SYNC_INFO
 * @details Records the sync info of the timing
*/ 
typedef struct _MWCAP_VIDEO_SYNC_INFO	 {
	BYTE											bySyncType;			///<Sync type, for details, see #_MWCAP_VIDEO_SYNC_TYPE.
	BOOLEAN											bHSPolarity;		///<Sync polarity of horizontal 
	BOOLEAN											bVSPolarity;		///<Sync polarity of vertical
	BOOLEAN											bInterlaced;		///<Whether video timing is interlaced
	DWORD											dwFrameDuration;	///<Frame interval
	WORD											wVSyncLineCount;	///<Number of vertical sync scan lines
	WORD											wFrameLineCount;	///<Number of frame scan lines
} MWCAP_VIDEO_SYNC_INFO;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_TIMING
 * @details Records video channel timing\n
 * 			The related function is [MWSetVideoTiming](@ref MWSetVideoTiming )
*/ 
typedef struct _MWCAP_VIDEO_TIMING {
	DWORD											dwType;				///<Type of video timing, for details, see #MWCAP_VIDEO_TIMING_TYPE
	DWORD											dwPixelClock;		///<Pixel clock of video timing
	BOOLEAN											bInterlaced;		///<Whether it is interlaced.
	BYTE											bySyncType;			///<Sync type of video timing
	BOOLEAN											bHSPolarity;		///<Whether horizontal timing (line) Polarity of horizontal sync pulse is positive
	BOOLEAN											bVSPolarity;		///<Whether the polarity of the vertical sync pulse is positive
	WORD											wHActive;			///<Active time of the horizontal timing 
	WORD											wHFrontPorch;		///<Horizontal front porch of video timing
	WORD											wHSyncWidth;		///<Horizontal sync width of video timing
	WORD											wHBackPorch;		///<Horizontal back porch of video timing
	WORD											wVActive;			///<Vertical active time of video timing
	WORD											wVFrontPorch;		///<Vertical front porch of video timing
	WORD											wVSyncWidth;		///<Vertical sync width of video timing
	WORD											wVBackPorch;		///<Vertical back porch of video timing
} MWCAP_VIDEO_TIMING;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_TIMING_SETTINGS
 * @details Records video timing settings\n
 * 			which is in both #MWCAP_VIDEO_CUSTOM_TIMING and #MWCAP_COMPONENT_SPECIFIC_STATUS.\n
 * 			Related functions are [MWGetCustomVideoTimings](@ref MWGetCustomVideoTimings )\n
 * 				    [MWSetCustomVideoTimings](@ref MWSetCustomVideoTimings )\n
 * 				    [MWSetCustomVideoTiming](@ref MWSetCustomVideoTiming )
*/ 
typedef struct _MWCAP_VIDEO_TIMING_SETTINGS {
	WORD											wAspectX;			///<Width of aspect ratio
	WORD											wAspectY;			///<Height of aspect ratio
	WORD											x;					///<Start position in horizontal direction
	WORD											y;					///<Start position in vertical direction
	WORD											cx;					///<Width
	WORD											cy;					///<Height
	WORD											cxTotal;			///<Total width in horizontal direction
	BYTE											byClampPos;			///<Clamp position
} MWCAP_VIDEO_TIMING_SETTINGS;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_KSPROPERTY_DWORD_S
 * @details Specifies the KSPROPERTY including a DWORD \n
*/ 
typedef struct _MWCAP_KSPROPERTY_DWORD_S {
	KSPROPERTY										Property;			///<KSPROPERTY, used to call the interface
	DWORD											dwParameter;		///<Additional DWORD
} MWCAP_KSPROPERTY_DWORD_S;							  

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_KSPROPERTY_PTR64_S
 * @details Specifies KSPROPERTY including a 64-bit pointer\n
*/ 
typedef struct _MWCAP_KSPROPERTY_PTR64_S {
	KSPROPERTY										Property;			///<KSPROPERTY, used to call the interface
	MWCAP_PTR64										pvParameter;		///<Additional 64-bit pointer
} MWCAP_KSPROPERTY_PTR64_S;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_DWORD_PARAMETER_RANGE
 * @details Defines parameters value range, step and the default value.\n
*/ 
typedef struct _MWCAP_DWORD_PARAMETER_RANGE {		
	DWORD											dwMin;				///<Minimum
	DWORD											dwMax;				///<Maximum
	DWORD											dwStep;				///<Step
	DWORD											dwDefault;			///<The default value
} MWCAP_DWORD_PARAMETER_RANGE;						
													
#define MWCAP_DWORD_PARAMETER_FLAG_AUTO				0x01

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_DWORD_PARAMETER_VALUE
 * @details Defines a DWORD\n
*/ 													
typedef struct _MWCAP_DWORD_PARAMETER_VALUE {		
	DWORD											dwFlags;			///<Flag
	DWORD											dwValue;			///<Value
} MWCAP_DWORD_PARAMETER_VALUE;                  

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_CHANNEL_INFO
 * @details Records detailed channel information\n
 * 			Related functions are: [MWGetChannelInfoByIndex](@ref MWGetChannelInfoByIndex )\n
 * 					[MWGetChannelInfo](@ref MWGetChannelInfo)
*/ 	
typedef struct _MWCAP_CHANNEL_INFO {		
	WORD											wFamilyID;								///<Product type, refers to #MW_FAMILY_ID
	WORD											wProductID;								///<device ID, refers to  #MWCAP_PRODUCT_ID
	CHAR											chHardwareVersion;						///<Hardware version ID
	BYTE											byFirmwareID;							///<Firmware ID
	DWORD											dwFirmwareVersion;						///<Firmware version
	DWORD											dwDriverVersion;						///<Driver version
	CHAR											szFamilyName[MW_FAMILY_NAME_LEN];		///<Product name
	CHAR											szProductName[MW_PRODUCT_NAME_LEN];		///<Product type
	CHAR											szFirmwareName[MW_FIRMWARE_NAME_LEN];	///<Firmware name
	CHAR											szBoardSerialNo[MW_SERIAL_NO_LEN];		///<Hardware serial number
	BYTE											byBoardIndex;							///<Rotary ID located on the capture card, 0~F.
	BYTE											byChannelIndex;							///<Channel index of the capture card, which starts from 0.
} MWCAP_CHANNEL_INFO;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_PCIE_CAPTURE_INFO
 * @details Records PCIE connctions.\n
 * 			Related functions are [MWGetFamilyInfoByIndex](@ref MWGetFamilyInfoByIndex)\n
 * 					[MWGetFamilyInfo](@ref MWGetFamilyInfo)
*/ 	
typedef struct _MWCAP_PCIE_CAPTURE_INFO {
	BYTE											byPCIBusID;								///<PCIE bus id
	BYTE											byPCIDevID;								///<PCIE device id
	BYTE											byLinkType;								///<PCIE connection type, refers to #_MWCAP_PCIE_LINK_TYPE
	BYTE											byLinkWidth;							///<PCIE bandwidth
	BYTE											byBoardIndex;							///<Capture board index, which is the same as the rotary number.
	WORD											wMaxPayloadSize;						///<PCIE max payload
	WORD											wMaxReadRequestSize;					///<PCIE Max Read Request
	DWORD											cbTotalMemorySize;						///<PCIE total memory size
	DWORD											cbFreeMemorySize;						///<PCIE free memory size
} MWCAP_PCIE_CAPTURE_INFO;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_ECO_CAPTURE_INFO
 * @details Records PCIE connection info of your device.\n
 * 			Related functions are [MWGetFamilyInfoByIndex](@ref MWGetFamilyInfoByIndex)\n
 * 					[MWGetFamilyInfo](@ref MWGetFamilyInfo).
*/ 	
typedef MWCAP_PCIE_CAPTURE_INFO MWCAP_ECO_CAPTURE_INFO;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_PRO_CAPTURE_INFO
 * @details Records PCIE connection info of your device\n
 * 			Related functions are [MWGetFamilyInfoByIndex](@ref MWGetFamilyInfoByIndex)\n
 * 					[MWGetFamilyInfo](@ref MWGetFamilyInfo).
*/ 	
typedef MWCAP_PCIE_CAPTURE_INFO MWCAP_PRO_CAPTURE_INFO;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_CAPS
 * @details Records video capture capability.\n
 * Usage:
 * @code
 * MWCAP_VIDEO_CAPS caps;
 * mr = MWGetVideoCaps(hChannel, &caps);
 * if(caps& MWCAP_USB_VIDEO_CAP_HDMI_LOOP_THROUGH){
 * //HDMI_LOOP_THROUGH
 * }
 * if(caps& MWCAP_USB_VIDEO_CAP_SDI_LOOP_THROUGH){
 * //SDI_LOOP_THROUGH
 * }
 * if(caps&MWCAP_USB_VIDEO_CAP_PLANAR_FORMAT){
 * //PLANAR_FORMAT
 * }
 * @endcode
 * Where dwCaps correspond to
 * @code
 * #define MWCAP_USB_VIDEO_CAP_HDMI_LOOP_THROUGH        0x00000001\n
 * #define MWCAP_USB_VIDEO_CAP_SDI_LOOP_THROUGH         0x00000002\n
 * #define MWCAP_USB_VIDEO_CAP_PLANAR_FORMAT            0x00000004\n
 * @endcode
 * It is used for USB capture card to determin whether a loopthrough interface or color format like I420 is supported by your USB capture card. For other capture card, it is always 0.
*/ 
typedef struct _MWCAP_VIDEO_CAPS {
	DWORD											dwCaps;									///<Capture capability
	WORD											wMaxInputWidth;							///<Max input width
	WORD											wMaxInputHeight;						///<Max input height
	WORD											wMaxOutputWidth;						///<Max output width
	WORD											wMaxOutputHeight;						///<Max output height
} MWCAP_VIDEO_CAPS;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_AUDIO_CAPS
 * @details Records audio capture capability of your card.\n
 * Usage:
 * @code
 * MWCAP_AUDIO_CAPS audioCaps;
 * MWGetAudioCaps(m_hcChannel,&audioCaps);
 * if(audioCaps.dwCaps&MWCAP_USB_AUDIO_CAP_EMBEDDED_CAPTURE){
 * 		//MWCAP_AUDIO_CAPTURE_NODE_EMBEDDED_CAPTURE;
 * }
 * if(audioCaps.dwCaps&MWCAP_USB_AUDIO_CAP_MICROPHONE){
 * 		//MWCAP_AUDIO_CAPTURE_NODE_MICROPHONE;
 * }
 * if(audioCaps.dwCaps&MWCAP_USB_AUDIO_CAP_LINE_IN){
 * 		//MWCAP_AUDIO_CAPTURE_NODE_LINE_IN;
 * }
 * if(audioCaps.dwCaps&MWCAP_USB_AUDIO_CAP_USB_CAPTURE){
 * 		//MWCAP_AUDIO_CAPTURE_NODE_USB_CAPTURE;
 * }
 * if (audioCaps.dwCaps&MWCAP_USB_AUDIO_CAP_HEADPHONE){
 * 		//headphone
 * }
 * @endcode
 * Where dwCaps correspond to \n
 * @code
 * #define MWCAP_USB_AUDIO_CAP_MICROPHONE				(1 << MWCAP_USB_AUDIO_MICROPHONE)\n
 * #define MWCAP_USB_AUDIO_CAP_HEADPHONE				(1 << MWCAP_USB_AUDIO_HEADPHONE)\n
 * #define MWCAP_USB_AUDIO_CAP_LINE_IN					(1 << MWCAP_USB_AUDIO_LINE_IN)\n
 * #define MWCAP_USB_AUDIO_CAP_LINE_OUT					(1 << MWCAP_USB_AUDIO_LINE_OUT)\n
 * #define MWCAP_USB_AUDIO_CAP_EMBEDDED_CAPTURE			(1 << MWCAP_USB_AUDIO_EMBEDDED_CAPTURE)\n
 * #define MWCAP_USB_AUDIO_CAP_EMBEDDED_PLAYBACK		(1 << MWCAP_USB_AUDIO_EMBEDDED_PLAYBACK)\n
 * #define MWCAP_USB_AUDIO_CAP_USB_CAPTURE				(1 << MWCAP_USB_AUDIO_USB_CAPTURE)\n
 * #define MWCAP_USB_AUDIO_CAP_USB_PLAYBACK				(1 << MWCAP_USB_AUDIO_USB_PLAYBACK)\n
 * @endcode
 * It is used for USB capture card to determine whether your USB capture card support for audio function
*/
typedef struct _MWCAP_AUDIO_CAPS {
	DWORD											dwCaps;									///<Audio capture capability
} MWCAP_AUDIO_CAPS;                       

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_FIRMWARE_STORAGE_CARD
 * @details Records firmware storage information of your capture card\n
 * Related data type: #MWCAP_FIRMWARE_STORAGE
*/
typedef struct _MWCAP_FIRMWARE_STORAGE_CARD{
	DWORD											cbStorage;								///<Length of firmware storage area
	DWORD											cbEraseBlock;							///<Length of erased area
	DWORD											cbProgramBlock;							///<Length of program block storage area
	DWORD											cbHeaderOffset;							///<Offset of firmware header 
}MWCAP_FIRMWARE_STORAGE_CARD;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_FIRMWARE_ERASE
 * @details Specifies the firmware storage area to erase.\n
 */
typedef struct _MWCAP_FIRMWARE_ERASE {
	DWORD											cbOffset;								///<Offset of firmware storage area to erase
	DWORD											cbErase;								///<Length of erased area
} MWCAP_FIRMWARE_ERASE;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_LED_MODE
 * @details Defines LED mode.\n
 */
typedef enum _MWCAP_LED_MODE {
	MWCAP_LED_AUTO									= 0x00000000,							///<Auto mode
	MWCAP_LED_OFF									= 0x80000000,							///<LED stays off
	MWCAP_LED_ON									= 0x80000001,							///<LED stays on
	MWCAP_LED_BLINK									= 0x80000002,							///<LED stays flashing
	MWCAP_LED_DBL_BLINK								= 0x80000003,							///<LED flashes two times, pauses, then blinks again
	MWCAP_LED_BREATH								= 0x80000004							///<Breathing/plusing slowly
}MWCAP_LED_MODE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_SDI_SPECIFIC_STATUS
 * @details Records SDI signal status\n
 * Related data type is:\n
 * 		    #MWCAP_INPUT_SPECIFIC_STATUS\n
 * Related function is:\n
 * 			[MWGetInputSpecificStatus](@ref MWGetInputSpecificStatus)\n
 */
typedef struct _MWCAP_SDI_SPECIFIC_STATUS {
	SDI_TYPE										sdiType;								///<SDI signal type
	SDI_SCANNING_FORMAT								sdiScanningFormat;						///<SDI scan format
	SDI_BIT_DEPTH									sdiBitDepth;							///<SDI bit depth
	SDI_SAMPLING_STRUCT								sdiSamplingStruct;						///<SDI sampling struct 
	BOOLEAN											bST352DataValid;						///<Whether ST352 is valid
	DWORD											dwST352Data;							///<ST352
} MWCAP_SDI_SPECIFIC_STATUS;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_HDMI_VIDEO_TIMING
 * @details Records HDMI timing.\n
 * Related data type are\n
 * 		    #MWCAP_HDMI_SPECIFIC_STATUS\n
 * 		    #MWCAP_INPUT_SPECIFIC_STATUS\n
 * Related functions is:\n
 * 			[MWGetInputSpecificStatus](@ref MWGetInputSpecificStatus)\n
 */
typedef struct _MWCAP_HDMI_VIDEO_TIMING {
	BOOLEAN											bInterlaced;							///<Whether the signal is interlaced
	DWORD											dwFrameDuration;						///<Frame time interval
	WORD											wHSyncWidth;							///<Horizontal sync width 
	WORD											wHFrontPorch;							///<Horizontal front porch 
	WORD											wHBackPorch;							///<Horizontal back porch
	WORD											wHActive;								///<Horizontal active width
	WORD											wHTotalWidth;							///<Horizontal total width
	WORD											wField0VSyncWidth;						///<Vertical sync width of top subframe
	WORD											wField0VFrontPorch;						///<Vertical front porch of top subframe
	WORD											wField0VBackPorch;						///<Vertical back porch of top subframe
	WORD											wField0VActive;							///<Vertical active width of top subframe
	WORD											wField0VTotalHeight;					///<Vertical total width of top subframe
	WORD											wField1VSyncWidth;						///<Vertical sync width of bottom subframe
	WORD											wField1VFrontPorch;						///<Vertical front porch of bottom subframe
	WORD											wField1VBackPorch;						///<Vertical back porch of bottom subframe
	WORD											wField1VActive;							///<Vertical active width of bottom subframe
	WORD											wField1VTotalHeight;					///<Vertical total width of bottom subframe
} MWCAP_HDMI_VIDEO_TIMING;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_HDMI_SPECIFIC_STATUS
 * @details Records the HDMI signal status.\n
 * Related data type are\n
 * 		    #MWCAP_INPUT_SPECIFIC_STATUS\n
 * Related functions are:\n
 * 			[MWGetInputSpecificStatus](@ref MWGetInputSpecificStatus)\n
 */
typedef struct _MWCAP_HDMI_SPECIFIC_STATUS {
	BOOLEAN											bHDMIMode;								///<Whether the signal is HDMI signal.
	BOOLEAN											bHDCP;									///<Whether the signal is HDCP-encrypted HDMI
	BYTE											byBitDepth;								///<Bit depth
	HDMI_PXIEL_ENCODING								pixelEncoding;							///<Pixel data encoding 
	BYTE											byVIC;									///<video identification code from EDID, which is used to specify standard revolution and timing
	BOOLEAN											bITContent;								///<IT Content
	BOOLEAN											b3DFormat;								///<Whether the signal is 3D
	BYTE											by3DStructure;							///<3D structure
	BYTE											bySideBySideHalfSubSampling;			///<Half Side-by-Side, sub-sampled at half resolution
	MWCAP_HDMI_VIDEO_TIMING							videoTiming;							///<Video timing
} MWCAP_HDMI_SPECIFIC_STATUS;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_COMPONENT_SPECIFIC_STATUS
 * @details Records the Component signal status.\n
 * Related data type are\n
 * 		    #MWCAP_INPUT_SPECIFIC_STATUS\n
 * Related functions are:\n
 * 			[MWGetInputSpecificStatus](@ref MWGetInputSpecificStatus)\n
 */
typedef struct _MWCAP_COMPONENT_SPECIFIC_STATUS {
	MWCAP_VIDEO_SYNC_INFO							syncInfo;								///<Video sync information
	BOOLEAN											bTriLevelSync;							///<Whether the signal is tri-level sync
	MWCAP_VIDEO_TIMING								videoTiming;							///<Video timing 
	MWCAP_VIDEO_TIMING_SETTINGS						videoTimingSettings;					///<Video timing settings
} MWCAP_COMPONENT_SPECIFIC_STATUS;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_SD_VIDEO_STANDARD
 * @details Defines the TV broadcast format used.\n
 */
typedef enum _MWCAP_SD_VIDEO_STANDARD {
	MWCAP_SD_VIDEO_NONE,																	///<None
	MWCAP_SD_VIDEO_NTSC_M,																	///<NTSC_M
	MWCAP_SD_VIDEO_NTSC_433,																///<NTSC_433
	MWCAP_SD_VIDEO_PAL_M,																	///<PAL_M
	MWCAP_SD_VIDEO_PAL_60,																	///<PAL_60
	MWCAP_SD_VIDEO_PAL_COMBN,																///<PAL_COMBN
	MWCAP_SD_VIDEO_PAL_BGHID,																///<PAL_BGHID
	MWCAP_SD_VIDEO_SECAM,																	///<SECAM
	MWCAP_SD_VIDEO_SECAM_60																	///<SECAM_60
} MWCAP_SD_VIDEO_STANDARD;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_CVBS_YC_SPECIFIC_STATUS
 * @details Records CVBS_YC signal status.\n
 * Related data type is\n
 * 		    #MWCAP_INPUT_SPECIFIC_STATUS\n
 * Related functions is\n
 * 			[MWGetInputSpecificStatus](@ref MWGetInputSpecificStatus)\n
 */
typedef struct _MWCAP_CVBS_YC_SPECIFIC_STATUS {
	MWCAP_SD_VIDEO_STANDARD							standard;								///<Defines video standard used
	BOOLEAN											b50Hz;									///<whether scanned frequency is 50Hz
} MWCAP_CVBS_YC_SPECIFIC_STATUS;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_INPUT_SPECIFIC_STATUS
 * @details Records input signal status.\n
 * Related functions are:\n
 * 			[MWGetInputSpecificStatus](@ref MWGetInputSpecificStatus)\n
 */
typedef struct _MWCAP_INPUT_SPECIFIC_STATUS {
	BOOLEAN											bValid;									///<Whether input signal is valid
	DWORD											dwVideoInputType;						///<Input video signal type. For details, refers to #MWCAP_VIDEO_INPUT_TYPE
	union {
		MWCAP_SDI_SPECIFIC_STATUS					sdiStatus;								///<SDI signal status
		MWCAP_HDMI_SPECIFIC_STATUS					hdmiStatus;								///<HDMI signal status
		MWCAP_COMPONENT_SPECIFIC_STATUS				vgaComponentStatus;						///<VGA component signal status
		MWCAP_CVBS_YC_SPECIFIC_STATUS				cvbsYcStatus;							///<CVBS-YC signal status
	};
} MWCAP_INPUT_SPECIFIC_STATUS;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_SIGNAL_STATE
 * @details Defines the input signal state\n
 */
typedef enum _MWCAP_VIDEO_SIGNAL_STATE {
	MWCAP_VIDEO_SIGNAL_NONE,																///<No signal
	MWCAP_VIDEO_SIGNAL_UNSUPPORTED,															///<Invalid signal. The capture card detects a signal but can not lock it.
	MWCAP_VIDEO_SIGNAL_LOCKING,																///<Locking signal. The signal is valid, but unlocked.
	MWCAP_VIDEO_SIGNAL_LOCKED																///<Locked signal. The capture card is ready to capture the input signal.
} MWCAP_VIDEO_SIGNAL_STATE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_SIGNAL_STATUS
 * @details Defines video signal status.\n
 * Related functions are:\n
 * 			[MWGetVideoSignalStatus](@ref MWGetVideoSignalStatus)\n
 */
typedef struct _MWCAP_VIDEO_SIGNAL_STATUS {
	MWCAP_VIDEO_SIGNAL_STATE						state;									///<Defines the accessibility of this video signal
	int												x;										///<Horizontal start position
	int												y;										///<Vertical start position
	int												cx;										///<Image width
	int												cy;										///<Image height
	int												cxTotal;								///<Total width
	int												cyTotal;								///<Total height
	BOOLEAN											bInterlaced;							///<Whether the signal is interlaced 
	DWORD											dwFrameDuration;						///<Frame interval of video frame
	int												nAspectX;								///<Width of video ratio
	int												nAspectY;								///<Height of video ratio
	BOOLEAN											bSegmentedFrame;						///<Whether the signal is segmented frame
	MWCAP_VIDEO_FRAME_TYPE							frameType;								///<video frame type
	MWCAP_VIDEO_COLOR_FORMAT						colorFormat;							///<video color format
	MWCAP_VIDEO_QUANTIZATION_RANGE					quantRange;								///<Quantization range
	MWCAP_VIDEO_SATURATION_RANGE					satRange;								///<saturation range
} MWCAP_VIDEO_SIGNAL_STATUS;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_AUDIO_SIGNAL_STATUS
 * @details Records audio signal status.\n
 * Related functions is\n
 * 			[MWGetAudioSignalStatus](@ref MWGetAudioSignalStatus)\n
 */
typedef struct _MWCAP_AUDIO_SIGNAL_STATUS {
	WORD											wChannelValid;							///<Valid audio channel mask.The lowest bit indicates whether the 1st and 2nd channels are valid, the second bit indicates whether the 3rd and 4th channels are valid, the third bit indicates whether the 5th and 6th channels are valid, and the fourth bit indicates whether the 7th and 8th channels are valid.
	BOOLEAN											bLPCM;									///<Whether the signal is LPCM
	BYTE											cBitsPerSample;							///<Bit depth of each audio sampling
	DWORD											dwSampleRate;							///<Sample rate
	BOOLEAN											bChannelStatusValid;					///<Whether channel status is valid
	IEC60958_CHANNEL_STATUS							channelStatus;							///<The audio channel status
} MWCAP_AUDIO_SIGNAL_STATUS;


typedef struct _MWCAP_TIMER_EXPIRE_TIME {
	MWCAP_PTR64										pvTimer;
	LONGLONG										llExpireTime;
} MWCAP_TIMER_EXPIRE_TIME;

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_INPUT_SOURCE_START_SCAN
 * @details Event notification. Start scanning the input source.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_INPUT_SOURCE_START_SCAN		0x0001ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_INPUT_SOURCE_STOP_SCAN
 * @details Event notification. Stop scanning the input source.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_INPUT_SOURCE_STOP_SCAN			0x0002ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_INPUT_SOURCE_SCAN_CHANGE
 * @details Event notification. Scan video input source status changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_INPUT_SOURCE_SCAN_CHANGE		0x0003ULL


/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE
 * @details Event notification. Video input changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE		0x0004ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE
 * @details Event notification. Audio input changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE		0x0008ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_INPUT_SPECIFIC_CHANGE
 * @details Event notification. Detailed format of input video changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_INPUT_SPECIFIC_CHANGE			0x0010ULL


/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE
 * @details Event notification. Format of input video changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE			0x0020ULL


/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE
 * @details Event notification. Audio signal changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE			0x0040ULL


/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_FIELD_BUFFERING
 * @details Event notification. Video field is buffering.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_FIELD_BUFFERING			0x0080ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING
 * @details Event notification. Video frame is buffering.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING			0x0100ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_FIELD_BUFFERED
 * @details Event notification. Video field has fully buffered.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_FIELD_BUFFERED			0x0200ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_FRAME_BUFFERED
 * @details Event notification. Video frame has fully buffered.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_FRAME_BUFFERED			0x0400ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_SMPTE_TIME_CODE
 * @details Event notification. SMPTE time code\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_SMPTE_TIME_CODE			0x0800ULL


/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED
 * @details Event notification. Audio frames have fully bufferred.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED			0x1000ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_AUDIO_INPUT_RESET
 * @details Event notification. Audio input reset.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_AUDIO_INPUT_RESET				0x2000ULL


/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_VIDEO_SAMPLING_PHASE_CHANGE
 * @details Event notification. Video sample phase changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_VIDEO_SAMPLING_PHASE_CHANGE	0x4000ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_LOOP_THROUGH_CHANGED
 * @details Event notification. The state of loopthrough interface changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_LOOP_THROUGH_CHANGED			0x8000ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_LOOP_THROUGH_EDID_CHANGED
 * @details Event notification. EDID of loopthrough changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_LOOP_THROUGH_EDID_CHANGED		0x10000ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_NEW_SDI_ANC_PACKET
 * @details Event notification. New ANC packets.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_NEW_SDI_ANC_PACKET				0x20000ULL

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_AVI
 * @details Event notification. AVI infoframe is changing.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_AVI				(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_AVI))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_AUDIO
 * @details Event notification. Audio infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_AUDIO			(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_AUDIO))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_SPD
 * @details Event notification. SPD infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_SPD				(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_SPD))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_MS
 * @details Event notification. MS infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_MS				(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_MS))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_VS
 * @details Event notification. VS infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_VS				(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_VS))

/**
 * @ingroup group_variables_macro
 * @brief  MWCAP_NOTIFY_HDMI_INFOFRAME_ACP
 * @details Event notification. ACP infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_ACP				(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_ACP))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_ISRC1
 * @details Event notification. ISRC1 infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_ISRC1			(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_ISRC1))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_ISRC2
 * @details Event notification. ISRC2 infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_ISRC2			(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_ISRC2))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_GAMUT
 * @details Event notification. GAMUT infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_GAMUT			(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_GAMUT))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_VBI
 * @details Event notification. VBI infoframe changed. \n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_VBI				(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_VBI))

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_NOTIFY_HDMI_INFOFRAME_HDR
 * @details Event notification. HDR infoframe changed.\n
 * Related functions are:\n
 * 			[MWRegisterNotify](@ref MWRegisterNotify)\n
 * 			[MWUnregisterNotify](@ref MWUnregisterNotify)\n
*/
#define MWCAP_NOTIFY_HDMI_INFOFRAME_HDR				(1ULL << (32 + MWCAP_HDMI_INFOFRAME_ID_HDR))

typedef struct _MWCAP_KSPROPERTY_NOTIFY_REGISTRATION_S {
	KSPROPERTY										Property;
	MWCAP_PTR64										hEvent;
	ULONGLONG										ullEnableBits;
} MWCAP_KSPROPERTY_NOTIFY_REGISTRATION_S;

// Video frame information
#define MWCAP_MAX_VIDEO_FRAME_COUNT					8

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_SMPTE_TIMECODE
 * @details Records SMPTE timecode.\n
 * Related type: #MWCAP_VIDEO_FRAME_INFO \n
 * @code
 * MWCAP_VIDEO_FRAME_INFO videoFrameInfo;
 * nRet= MWGetVideoFrameInfo(g_hChannel[i], videoBufferInfo.iNewestBufferedFullFrame, &videoFrameInfo);
 * printf("[Timecode] Top: %2x:%2x:%2x:%2x , Bottom: %2x:%2x:%2x:%2x\n",
 * 						videoFrameInfo.aSMPTETimeCodes[0].byHours,
 * 						videoFrameInfo.aSMPTETimeCodes[0].byMinutes,
 * 						videoFrameInfo.aSMPTETimeCodes[0].bySeconds,
 * 						videoFrameInfo.aSMPTETimeCodes[0].byFrames,
 * 						videoFrameInfo.aSMPTETimeCodes[1].byHours,
 * 						videoFrameInfo.aSMPTETimeCodes[1].byMinutes,
 * 						videoFrameInfo.aSMPTETimeCodes[1].bySeconds,
 * 						videoFrameInfo.aSMPTETimeCodes[1].byFrames
 * );
 * @endcode
 */
typedef struct _MWCAP_SMPTE_TIMECODE {
	BYTE 											byFrames;									///<Frames number
	BYTE											bySeconds;									///<Seconds
	BYTE											byMinutes;									///<Minutes
	BYTE											byHours;									///<Hours
} MWCAP_SMPTE_TIMECODE;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_FRAME_STATE
 * @details Defines video frame state.\n
 * Related type: #MWCAP_VIDEO_FRAME_INFO \n
 */
typedef enum _MWCAP_VIDEO_FRAME_STATE {
	MWCAP_VIDEO_FRAME_STATE_INITIAL,															///<Initial
	MWCAP_VIDEO_FRAME_STATE_F0_BUFFERING,														///<Buffering top subframe
	MWCAP_VIDEO_FRAME_STATE_F1_BUFFERING,														///<Buffering bottom subframe
	MWCAP_VIDEO_FRAME_STATE_BUFFERED															///<Fully bufferred video frame 
} MWCAP_VIDEO_FRAME_STATE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_BUFFER_INFO
 * @details Records video buffer information.\n
 * Related functions are:\n
 * 			[MWGetVideoBufferInfo](@ref MWGetVideoBufferInfo)\n
 */
typedef struct _MWCAP_VIDEO_BUFFER_INFO {
	DWORD											cMaxFrames;									///<Maximum number of frames in on-board cache

	BYTE											iNewestBuffering;							///<The number of the slices being bufferred. A frame of video data may contain multiple slices.
	BYTE											iBufferingFieldIndex;						///<The sequence number of fields being bufferred.

	BYTE											iNewestBuffered;							///<The sequence number of slices the latest bufferred piece.
	BYTE											iBufferedFieldIndex;						///<The sequence number of the latest bufferred field

	BYTE											iNewestBufferedFullFrame;					///<The sequence number of the latest bufferred frame
	DWORD											cBufferedFullFrames;						///<Number of fully bufferred full frames
} MWCAP_VIDEO_BUFFER_INFO;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_FRAME_INFO
 * @details Records video frame information\n
 * Related functions are:\n
 * 			[MWGetVideoFrameInfo](@ref MWGetVideoFrameInfo)\n
 */
typedef struct _MWCAP_VIDEO_FRAME_INFO {
	MWCAP_VIDEO_FRAME_STATE							state;										///<The state of the video framess

	BOOLEAN											bInterlaced;								///<Whether an interlaced signal
	BOOLEAN											bSegmentedFrame;							///<Whether a segmented frame
	BOOLEAN											bTopFieldFirst;								///<Whether the top subframe is in front
	BOOLEAN											bTopFieldInverted;							///<Whether to reverse the top subframe

	int												cx;											///<Width of video frames
	int												cy;											///<Height of video frames
	int												nAspectX;									///<Width of the ratio 
	int												nAspectY;									///<Height of the ratio

	LONGLONG										allFieldStartTimes[2];						///<Start time of capturing top and bottom subframe respectively
	LONGLONG										allFieldBufferedTimes[2];					///<Fully bufferred time of top and bottom frame respectively
	MWCAP_SMPTE_TIMECODE							aSMPTETimeCodes[2];							///<Time code of top and bottom frame respectively
} MWCAP_VIDEO_FRAME_INFO;

// Video capture
typedef struct _MWCAP_VIDEO_CAPTURE_OPEN {
	MWCAP_PTR64										hEvent;
} MWCAP_VIDEO_CAPTURE_OPEN;

#define MWCAP_VIDEO_MAX_NUM_OSD_RECTS				4

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERED
 * @details The latest buffered frame ID\n
 * Related functions are:\n
 * 		    [MWCaptureVideoFrameToVirtualAddress](@ref MWCaptureVideoFrameToVirtualAddress)\n
 * 			[MWCaptureVideoFrameToPhysicalAddress](@ref MWCaptureVideoFrameToPhysicalAddress)\n
 *          [MWCaptureVideoFrameWithOSDToVirtualAddress](@ref MWCaptureVideoFrameWithOSDToVirtualAddress)\n
 * 		    [MWCaptureVideoFrameWithOSDToPhysicalAddress](@ref MWCaptureVideoFrameWithOSDToPhysicalAddress)\n
 * 		    [MWCaptureVideoFrameToVirtualAddressEx](@ref MWCaptureVideoFrameToVirtualAddressEx)\n
 * 		    [MWCaptureVideoFrameToPhysicalAddressEx](@ref MWCaptureVideoFrameToPhysicalAddressEx)\n
*/
#define MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERED		(-1)

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERING
 * @details The latest bufferring frame ID\n
 * Related functions are:\n
 * 		    [MWCaptureVideoFrameToVirtualAddress](@ref MWCaptureVideoFrameToVirtualAddress)\n
 * 			[MWCaptureVideoFrameToPhysicalAddress](@ref MWCaptureVideoFrameToPhysicalAddress)\n
 *          [MWCaptureVideoFrameWithOSDToVirtualAddress](@ref MWCaptureVideoFrameWithOSDToVirtualAddress)\n
 * 		    [MWCaptureVideoFrameWithOSDToPhysicalAddress](@ref MWCaptureVideoFrameWithOSDToPhysicalAddress)\n
 * 		    [MWCaptureVideoFrameToVirtualAddressEx](@ref MWCaptureVideoFrameToVirtualAddressEx)\n
 * 		    [MWCaptureVideoFrameToPhysicalAddressEx](@ref MWCaptureVideoFrameToPhysicalAddressEx)\n
*/
#define MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERING		(-2)

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_VIDEO_FRAME_ID_NEXT_BUFFERED
 * @details The next bufferred frame ID\n
 * Related functions are:\n
 * 		    [MWCaptureVideoFrameToVirtualAddress](@ref MWCaptureVideoFrameToVirtualAddress)\n
 * 			[MWCaptureVideoFrameToPhysicalAddress](@ref MWCaptureVideoFrameToPhysicalAddress)\n
 *          [MWCaptureVideoFrameWithOSDToVirtualAddress](@ref MWCaptureVideoFrameWithOSDToVirtualAddress)\n
 * 		    [MWCaptureVideoFrameWithOSDToPhysicalAddress](@ref MWCaptureVideoFrameWithOSDToPhysicalAddress)\n
 * 		    [MWCaptureVideoFrameToVirtualAddressEx](@ref MWCaptureVideoFrameToVirtualAddressEx)\n
 * 		    [MWCaptureVideoFrameToPhysicalAddressEx](@ref MWCaptureVideoFrameToPhysicalAddressEx)\n
*/
#define MWCAP_VIDEO_FRAME_ID_NEXT_BUFFERED			(-3)

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_VIDEO_FRAME_ID_NEXT_BUFFERING
 * @details The next bufferring frame ID\n
 * Related functions are:\n
 * 		    [MWCaptureVideoFrameToVirtualAddress](@ref MWCaptureVideoFrameToVirtualAddress)\n
 * 			[MWCaptureVideoFrameToPhysicalAddress](@ref MWCaptureVideoFrameToPhysicalAddress)\n
 *          [MWCaptureVideoFrameWithOSDToVirtualAddress](@ref MWCaptureVideoFrameWithOSDToVirtualAddress)\n
 * 		    [MWCaptureVideoFrameWithOSDToPhysicalAddress](@ref MWCaptureVideoFrameWithOSDToPhysicalAddress)\n
 * 		    [MWCaptureVideoFrameToVirtualAddressEx](@ref MWCaptureVideoFrameToVirtualAddressEx)\n
 * 		    [MWCaptureVideoFrameToPhysicalAddressEx](@ref MWCaptureVideoFrameToPhysicalAddressEx)\n
*/
#define MWCAP_VIDEO_FRAME_ID_NEXT_BUFFERING			(-4)

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_VIDEO_FRAME_ID_EMPTY
 * @details Frame ID is empty\n
 * Related type: #MWCAP_VIDEO_CAPTURE_STATUS\n
 */
#define MWCAP_VIDEO_FRAME_ID_EMPTY					(-100)

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_VIDEO_PROCESS_FLIP
 * @details Flip video images Up-side-down
 * Related type: #MWCAP_VIDEO_PROCESS_SETTINGS\n
 */
#define MWCAP_VIDEO_PROCESS_FLIP					0x00000001

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_VIDEO_PROCESS_MIRROR
 * @details Flip video images left to right\n
 * Related type: #MWCAP_VIDEO_PROCESS_SETTINGS\n
 */
#define MWCAP_VIDEO_PROCESS_MIRROR					0x00000002

typedef enum _MWCAP_VIDEO_PLANE_ADDRESS_TYPE {
	MWCAP_VIDEO_PLANE_VIRTUAL_ADDRESS,
	MWCAP_VIDEO_PLANE_PHYSICAL_ADDRESS
} MWCAP_VIDEO_PLANE_ADDRESS_TYPE;

typedef struct _MWCAP_VIDEO_PLANE_BUFFER
{
	DWORD											cbBuffer;
	DWORD											cbStride;

	union {
		MWCAP_PTR64									pvVirtualAddress;
		LARGE_INTEGER								liPhysicalAddress;
	};
} MWCAP_VIDEO_PLANE_BUFFER;

typedef struct _MWCAP_VIDEO_CAPTURE_FRAME {
	// Processing parameters
	DWORD											dwFOURCC;
	WORD											cx;
	WORD											cy;
	int												nAspectX;
	int												nAspectY;
	MWCAP_VIDEO_COLOR_FORMAT						colorFormat;
	MWCAP_VIDEO_QUANTIZATION_RANGE					quantRange;
	MWCAP_VIDEO_SATURATION_RANGE					satRange;

	SHORT											sContrast;			// [50, 200]
	SHORT											sBrightness;		// [-100, 100]
	SHORT											sSaturation;		// [0, 200]
	SHORT											sHue;				// [-90, 90]

	RECT											rectSource;
	RECT											rectTarget;

	MWCAP_VIDEO_DEINTERLACE_MODE					deinterlaceMode;
	MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE			aspectRatioConvertMode;
	
	// Source
	char											iSrcFrame;

	// OSD (within rectTarget and [0,0-cx,cy))
	MWCAP_PTR64										pOSDImage;
	RECT											aOSDRects[MWCAP_VIDEO_MAX_NUM_OSD_RECTS];
	int												cOSDRects;

	// Buffer parameters
	/*
	MWCAP_VIDEO_PLANE_ADDRESS_TYPE					addressType;
	int												cPlaneBuffers;
	MWCAP_VIDEO_PLANE_BUFFER						aPlaneBuffers[3];
	*/

	BOOLEAN											bPhysicalAddress;
	union {
		MWCAP_PTR64									pvFrame;
		LARGE_INTEGER								liPhysicalAddress;
	};

	DWORD											cbFrame;
	DWORD											cbStride;

	BOOLEAN											bBottomUp;

	WORD											cyPartialNotify;		// 0: Not use partial notify

	DWORD											dwProcessSwitchs;		// MWCAP_VIDEO_PROCESS_xx

	// Context
	MWCAP_PTR64										pvContext;
} MWCAP_VIDEO_CAPTURE_FRAME;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_CAPTURE_STATUS
 * @details Records state of video capture.\n
 * Related functions are:\n
 * 		   [MWGetVideoCaptureStatus](@ref MWGetVideoCaptureStatus)\n
*/
typedef struct _MWCAP_VIDEO_CAPTURE_STATUS {
	MWCAP_PTR64										pvContext;																	///<The context of video capture

	BOOLEAN											bPhysicalAddress;															///<Whether to use the physical address to store the capture data 
	union {
		MWCAP_PTR64									pvFrame;																	///<The memory address to store the capture data 
		LARGE_INTEGER								liPhysicalAddress;															///<The physical address to store the capture data
	};

	int												iFrame;																		///<The index of capturing frame
	BOOLEAN											bFrameCompleted;															///<Whether a frame is fully captured
	WORD											cyCompleted;																///<Number of frames captured
	WORD											cyCompletedPrev;															///<Number of frames captured previously
} MWCAP_VIDEO_CAPTURE_STATUS;

#define MWCAP_AUDIO_FRAME_SYNC_CODE					0xFECA0357

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_AUDIO_SAMPLES_PER_FRAME
 * @details Sample numbers of every audio frame\n
 * Related type: \n
 * 		    #MWCAP_AUDIO_CAPTURE_FRAME\n
*/
#define MWCAP_AUDIO_SAMPLES_PER_FRAME				192

/**
 * @ingroup group_variables_macro
 * @brief MWCAP_AUDIO_MAX_NUM_CHANNELS
 * @details Max audio channels\n
 * Related type: \n
 * 		    #MWCAP_AUDIO_CAPTURE_FRAME\n
*/
#define MWCAP_AUDIO_MAX_NUM_CHANNELS				8


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_AUDIO_CAPTURE_FRAME
 * @details Records audio frame info.\n
 * Related functions are:\n
 * 		   [MWCaptureAudioFrame](@ref MWCaptureAudioFrame)\n
*/
typedef struct _MWCAP_AUDIO_CAPTURE_FRAME {
	DWORD											cFrameCount;																///<Number of bufferred frames
	DWORD											iFrame;																		///<Current frame index
	DWORD											dwSyncCode;																	///<Sync code of audio frame data
	DWORD											dwFlags;																	///<Reserved
	LONGLONG										llTimestamp;																///<The timestamp of audio frame
	DWORD											adwSamples[MWCAP_AUDIO_SAMPLES_PER_FRAME * MWCAP_AUDIO_MAX_NUM_CHANNELS];	///<Audio sample data. Each sample is 32-bit width, and high bit effective. The priority of the path is: Left0, Left1, Left2, Left3, right0, right1, right2, right3
} MWCAP_AUDIO_CAPTURE_FRAME;


/**
 * @ingroup group_variables_enum
 * @brief MWCAP_HDMI_INFOFRAME_ID
 * @details Defines HDMI infoframe ID\n
 * Related type: #MWCAP_HDMI_INFOFRAME_MASK\n
 * Related functions are:\n
 * 		    [MWGetHDMIInfoFramePacket](@ref MWGetHDMIInfoFramePacket)\n
 */
typedef enum _MWCAP_HDMI_INFOFRAME_ID {
	MWCAP_HDMI_INFOFRAME_ID_AVI,																							///<AVI infoframe
	MWCAP_HDMI_INFOFRAME_ID_AUDIO,																							///<Audio infoframe
	MWCAP_HDMI_INFOFRAME_ID_SPD,																							///<SPD infoframe
	MWCAP_HDMI_INFOFRAME_ID_MS,																								///<MS infoframe
	MWCAP_HDMI_INFOFRAME_ID_VS,																								///<VS infoframe
	MWCAP_HDMI_INFOFRAME_ID_ACP,																							///<ACP infoframe
	MWCAP_HDMI_INFOFRAME_ID_ISRC1,																							///<ISRC1 infoframe
	MWCAP_HDMI_INFOFRAME_ID_ISRC2,																							///<ISRC2 infoframe
	MWCAP_HDMI_INFOFRAME_ID_GAMUT,																							///<GAMUT infoframe
	MWCAP_HDMI_INFOFRAME_ID_VBI,																							///<VBI infoframe
	MWCAP_HDMI_INFOFRAME_ID_HDR,																							///<HDR infoframe
	MWCAP_HDMI_INFOFRAME_COUNT,																								///<Number of infoframe types
} MWCAP_HDMI_INFOFRAME_ID;

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_HDMI_INFOFRAME_MASK
 * @details Defines mask of HDMI infoframe\n
 * Related type: #MWCAP_HDMI_INFOFRAME_ID \n
*/
typedef enum _MWCAP_HDMI_INFOFRAME_MASK {
	MWCAP_HDMI_INFOFRAME_MASK_AVI					= (1 << MWCAP_HDMI_INFOFRAME_ID_AVI),									///<AVI infoframe
	MWCAP_HDMI_INFOFRAME_MASK_AUDIO					= (1 << MWCAP_HDMI_INFOFRAME_ID_AUDIO),									///<Audio infoframe
	MWCAP_HDMI_INFOFRAME_MASK_SPD					= (1 << MWCAP_HDMI_INFOFRAME_ID_SPD),									///<SPD infoframe
	MWCAP_HDMI_INFOFRAME_MASK_MS					= (1 << MWCAP_HDMI_INFOFRAME_ID_MS),									///<MS infoframe
	MWCAP_HDMI_INFOFRAME_MASK_VS					= (1 << MWCAP_HDMI_INFOFRAME_ID_VS),									///<VS infoframe
	MWCAP_HDMI_INFOFRAME_MASK_ACP					= (1 << MWCAP_HDMI_INFOFRAME_ID_ACP),									///<ACP infoframe
	MWCAP_HDMI_INFOFRAME_MASK_ISRC1					= (1 << MWCAP_HDMI_INFOFRAME_ID_ISRC1),									///<ISRC1 infoframe
	MWCAP_HDMI_INFOFRAME_MASK_ISRC2					= (1 << MWCAP_HDMI_INFOFRAME_ID_ISRC2),									///<ISRC2 infoframe
	MWCAP_HDMI_INFOFRAME_MASK_GAMUT					= (1 << MWCAP_HDMI_INFOFRAME_ID_GAMUT),									///<GAMUT infoframe
	MWCAP_HDMI_INFOFRAME_MASK_VBI					= (1 << MWCAP_HDMI_INFOFRAME_ID_VBI),									///<VBI infoframe
	MWCAP_HDMI_INFOFRAME_MASK_HDR					= (1 << MWCAP_HDMI_INFOFRAME_ID_HDR)									///<HDR infoframe
} MWCAP_HDMI_INFOFRAME_MASK;

typedef struct _MWCAP_VIDEO_ASPECT_RATIO {
	int												nAspectX;
	int												nAspectY;
} MWCAP_VIDEO_ASPECT_RATIO;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_CONNECTION_FORMAT
 * @details Defines video connection format\n
 * Related functions are:\n
 * 		   [MWGetVideoCaptureConnectionFormat](@ref MWGetVideoCaptureConnectionFormat)\n
 * 		   [MWGetVideoPreviewConnectionFormat](@ref MWGetVideoPreviewConnectionFormat)\n
*/
typedef struct _MWCAP_VIDEO_CONNECTION_FORMAT {
	// Valid flag
	BOOLEAN											bConnected;																///<Whether it is connected
	
	// Basic information
	LONG											cx;																		///<Width of video image
	LONG											cy;																		///<Height of video image
	DWORD											dwFrameDuration;														///<Interval of video image
	DWORD											dwFOURCC;																///<Color format refers to MWFOURCC.h

	// Preferred parameters
	int												nAspectX;																///<Width of video ratio
	int												nAspectY;																///<Height of video ratio
	MWCAP_VIDEO_COLOR_FORMAT						colorFormat;															///<Color format
	MWCAP_VIDEO_QUANTIZATION_RANGE					quantRange;																///<Quantization
	MWCAP_VIDEO_SATURATION_RANGE					satRange;																///<Saturation rage
} MWCAP_VIDEO_CONNECTION_FORMAT;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_PROCESS_SETTINGS
 * @details Defines settings of video process \n
 * Related functions are:\n
 * 		   [MWGetVideoCaptureProcessSettings](@ref MWGetVideoCaptureProcessSettings)\n
 * 		   [MWSetVideoCaptureProcessSettings](@ref MWSetVideoCaptureProcessSettings)\n
*/
typedef struct _MWCAP_VIDEO_PROCESS_SETTINGS {
	DWORD											dwProcessSwitchs;														///<Mask of video processing refers to #MWCAP_VIDEO_PROCESS_FLIP, #MWCAP_VIDEO_PROCESS_MIRROR
	RECT											rectSource;																///<The source area to be processed
	int												nAspectX;																///<Width of video ratio
	int												nAspectY;																///<Height of video ratio
	BOOLEAN											bLowLatency;															///<Whether to enable lowtancy 
	MWCAP_VIDEO_COLOR_FORMAT						colorFormat;															///<Standard of video color format  
	MWCAP_VIDEO_QUANTIZATION_RANGE					quantRange;																///<Quantization
	MWCAP_VIDEO_SATURATION_RANGE					satRange;																///<Saturation rage
	MWCAP_VIDEO_DEINTERLACE_MODE					deinterlaceMode;														///<Interlaced mode 
	MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE			aspectRatioConvertMode;													///<Aspect ratio conversion 
} MWCAP_VIDEO_PROCESS_SETTINGS;

#define MWCAP_VIDEO_MAX_NUM_PREFERRED_TIMINGS		8

typedef struct _MWCAP_KSPROPERTY_VIDEO_CREATE_IMAGE_S {
	KSPROPERTY										Property;
	WORD											cx;
	WORD											cy;
} MWCAP_KSPROPERTY_VIDEO_CREATE_IMAGE_S;

typedef struct _MWCAP_VIDEO_UPLOAD_IMAGE {
	// Destination parameters
	MWCAP_PTR64										pvDestImage;
	MWCAP_VIDEO_COLOR_FORMAT						cfDest;
	WORD											xDest;
	WORD											yDest;
	WORD											cxDest;
	WORD											cyDest;

	MWCAP_VIDEO_QUANTIZATION_RANGE					quantRangeDest;
	MWCAP_VIDEO_SATURATION_RANGE					satRangeDest;

	// Source parameters
	BOOLEAN											bSrcPhysicalAddress;
	union {
		MWCAP_PTR64									pvSrcFrame;
		LARGE_INTEGER								liSrcPhysicalAddress;
	};

	DWORD											cbSrcFrame;
	DWORD											cbSrcStride;

	WORD											cxSrc;
	WORD											cySrc;
	BOOLEAN											bSrcBottomUp;
	BOOLEAN											bSrcPixelAlpha;
	BOOLEAN											bSrcPixelXBGR;
} MWCAP_VIDEO_UPLOAD_IMAGE;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_OSD_SETTINGS
 * @details Defines OSD\n
 * Related functions are:\n
 * 		   [MWGetVideoCaptureOSDSettings](@ref MWGetVideoCaptureOSDSettings)\n
 * 		   [MWSetVideoCaptureOSDSettings](@ref MWSetVideoCaptureOSDSettings)\n
*/
typedef struct _MWCAP_VIDEO_OSD_SETTINGS {
	BOOLEAN											bEnable;																///<Whether to use OSD
	WCHAR											wszPNGFilePath[_MAX_PATH];												///<File Path to load
} MWCAP_VIDEO_OSD_SETTINGS;

typedef struct _MWCAP_VIDEO_OSD_IMAGE {
	MWCAP_PTR64										pvOSDImage;
	RECT											aOSDRects[MWCAP_VIDEO_MAX_NUM_OSD_RECTS];
	int												cOSDRects;
} MWCAP_VIDEO_OSD_IMAGE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_CUSTOM_TIMING
 * @details Defines custom video timing.\n
 * Related functions are:\n
 * 		   [MWSetCustomVideoTiming](@ref MWSetCustomVideoTiming)\n
*/
typedef struct _MWCAP_VIDEO_CUSTOM_TIMING {
	MWCAP_VIDEO_SYNC_INFO							syncInfo;															///<Video sync information
	MWCAP_VIDEO_TIMING_SETTINGS						videoTimingSettings;												///<Video timing settings
} MWCAP_VIDEO_CUSTOM_TIMING;

typedef struct _MWCAP_VIDEO_PIN_BUFFER {
	MWCAP_PTR64										pvBuffer;
	DWORD											cbBuffer;
} MWCAP_VIDEO_PIN_BUFFER;
/**
 * @ingroup group_variables_struct
 * @brief MWCAP_SDI_ANC_TYPE
 * @details Configurations of SDI ANC\n
 * Related functions are:\n
 * 		    [MWCaptureSetSDIANCType](@ref MWCaptureSetSDIANCType)\n
*/
typedef struct _MWCAP_SDI_ANC_TYPE {
	BYTE											byId;																///<4 anc, the id is from 0 to 3.
	BOOLEAN											bHANC;																///<Whether it is hanc
	BOOLEAN											bVANC;																///<Whether it is vanc
	BYTE											byDID;																///<Id of anc
	BYTE											bySDID;																///<Second id of anc
} MWCAP_SDI_ANC_TYPE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_SDI_ANC_PACKET
 * @details SDI ANC\n
 * Related functions are:\n
 * 		    [MWCaptureGetSDIANCPacket](@ref MWCaptureGetSDIANCPacket)\n
*/
typedef struct _MWCAP_SDI_ANC_PACKET {
	BYTE											byDID;																///<Id of anc
	BYTE											bySDID;																///<Second id of anc
	BYTE											byDC;																///<Valid length of anc
	BYTE											abyUDW[255];														///<anc data
	BYTE											abyReserved[2];														///<Reserverd anc 
} MWCAP_SDI_ANC_PACKET;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_ECO_CAPTURE_OPEN
 * @details ECO capture configurations\n
 * Related functions are:\n
 * 		    [MWStartVideoEcoCapture](@ref MWStartVideoEcoCapture)\n
*/
typedef struct _MWCAP_VIDEO_ECO_CAPTURE_OPEN {
	MWCAP_PTR64										hEvent;																///<Handle of capture event

	DWORD											dwFOURCC;															///<Capture format
	WORD											cx;																	///<Width
	WORD											cy;																	///<Height
	LONGLONG										llFrameDuration;													///<Interval, -1 indicates follow format of input source
} MWCAP_VIDEO_ECO_CAPTURE_OPEN;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_ECO_CAPTURE_SETTINGS
 * @details ECO capture configurations \n
 * Related functions are:\n
 * 		    [MWSetVideoEcoCaptureSettings](@ref MWSetVideoEcoCaptureSettings)\n
 * 		    [MWGetVideoEcoCaptureSettings](@ref MWGetVideoEcoCaptureSettings)\n
*/
typedef struct _MWCAP_VIDEO_ECO_CAPTURE_SETTINGS {
	MWCAP_VIDEO_COLOR_FORMAT						colorFormat;														///<Used color format
	MWCAP_VIDEO_QUANTIZATION_RANGE					quantRange;															///<Used quantization range
	MWCAP_VIDEO_SATURATION_RANGE					satRange;															///<Used saturation range
	SHORT											sContrast;															///<Contrast, ranges from 50 to 200
	SHORT											sBrightness;														///<Brightness, ranges from -100 to 100
	SHORT											sSaturation;														///<Saturation, ranges from 0 to 200
	SHORT											sHue;																///<Hue, ranges from -90 to 90
} MWCAP_VIDEO_ECO_CAPTURE_SETTINGS;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_ECO_CAPTURE_FRAME
 * @details ECO capture frames\n
 * Related functions are:\n
 * 		    [MWCaptureSetVideoEcoFrame](@ref MWCaptureSetVideoEcoFrame)\n
*/
typedef struct _MWCAP_VIDEO_ECO_CAPTURE_FRAME {
	MWCAP_PTR64										pvFrame;															///<The storage address for video capturing
	DWORD											cbFrame;															///<The size of storage for video capturing
	DWORD											cbStride;															///<Width of capture video frame

	BOOLEAN											bBottomUp;															///<Whether to flip
	MWCAP_VIDEO_DEINTERLACE_MODE					deinterlaceMode;													///<DeinterlaceMode

	MWCAP_PTR64										pvContext;															///<Context of ECO 
} MWCAP_VIDEO_ECO_CAPTURE_FRAME;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_ECO_CAPTURE_STATUS
 * @details ECO capture status, obatains the captured video.\n
 * Related functions are:\n
 * 		    [MWGetVideoEcoCaptureStatus](@ref MWGetVideoEcoCaptureStatus)\n
*/
typedef struct _MWCAP_VIDEO_ECO_CAPTURE_STATUS {
	MWCAP_PTR64										pvContext;															///<frame label for DWORD
	MWCAP_PTR64										pvFrame;															///<Frame data address
	LONGLONG										llTimestamp;														///<Timestamp
} MWCAP_VIDEO_ECO_CAPTURE_STATUS;

typedef enum _MWCAP_POST_RECONFIG_MODE {
	MWCAP_POST_RECONFIG_MODE_REBOOT,
	MWCAP_POST_RECONFIG_MODE_HPD
}MWCAP_POST_RECONFIG_MODE;

typedef struct _MWCAP_POST_RECONFIG {
	uint16_t timeout;
	uint8_t reserved;
	uint8_t mode;
} MWCAP_POST_RECONFIG;

// MWCAP_HID_STATUS_IMAGE
typedef struct _MWCAP_STATUS_IMAGE_ADDR {
	uint32_t                                        cbOffset;
	uint32_t                                        cbLength;
} MWCAP_STATUS_IMAGE_ADDR;

typedef struct _MWCAP_STATUS_IMAGE_DATA {
	uint8_t                                         abyData[512];
} MWCAP_STATUS_IMAGE_DATA;

#pragma pack(pop)
