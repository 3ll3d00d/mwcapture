////////////////////////////////////////////////////////////////////////////////

// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2020 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "MWSDKCommon.h"

#include <stdint.h>
#include "MWCaptureExtension.h"
#include "MWUSBCaptureExtension.h"
#pragma pack(push)
#pragma pack(1)
/**
 * @ingroup group_variables_enum
 * @brief MW_RESULT
 * @details the interface returns values
*/
typedef enum MW_RESULT {
	MW_SUCCEEDED = 0x00,										///<Operation succeeded.
	MW_FAILED,													///<Operation failed.
	MW_INVALID_PARAMS,											///<Invalid parameters.
};

#ifdef _MSC_VER
DECLARE_HANDLE(HCHANNEL);

DECLARE_MWHANDLE64(HTIMER);

DECLARE_MWHANDLE64(HNOTIFY);

DECLARE_MWHANDLE64(HOSD);
#else
typedef void * HCHANNEL;
 
typedef uint64_t HTIMER;

typedef uint64_t HNOTIFY;

typedef uint64_t HOSD;
#endif

#define LEN_DEV_AUDIO_NAME							128
#define LEN_DEV_AUDIO_ID							256

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_FIRMWARE_STORAGE
 * @details Records firmware storage information.\n
 * Related data type:\n
 *  #MWCAP_FIRMWARE_STORAGE_USB\n
 *  #MWCAP_FIRMWARE_STORAGE_CARD\n
 * Related functions are:
 * [MWGetFirmwareStorageInfo](@ref MWGetFirmwareStorageInfo)\n
*/
typedef union _MWCAP_FIRMWARE_STORAGE {
		MWCAP_FIRMWARE_STORAGE_CARD					firmwareStorageCard;
		MWCAP_FIRMWARE_STORAGE_USB					firmwareStorageUSB;
} MWCAP_FIRMWARE_STORAGE;

typedef enum _MW_VIDEO_CAPTURE_MODE {
	MW_VIDEO_CAPTURE_NORMAL = 0x00,
	MW_VIDEO_CAPTURE_LOW_LATENCY,
} MW_VIDEO_CAPTURE_MODE;

typedef void(*LPFN_VIDEO_CAPTURE_CALLBACK)(LPBYTE pbFrame,
                                           DWORD cbFrame,
										   DWORD cbStride, 
                                           MWCAP_VIDEO_FRAME_INFO* pFrameInfo, 
                                           void* pvContent);
typedef struct _MWCAP_VIDEO_CAPTURE {
	_MWCAP_VIDEO_CAPTURE():
	dwFOURCC(0), 
	iWidth(0), 
	iHeight(0), 
	captureMode(MW_VIDEO_CAPTURE_NORMAL),
	lpfnVideoCaptureCallback(NULL), 
	pvContext(NULL), 
	dwFrameSize(0), 
	cbStride(0) {}
	
	DWORD						dwFOURCC;
	int							iWidth;
	int							iHeight;
	MW_VIDEO_CAPTURE_MODE		captureMode;
	LPFN_VIDEO_CAPTURE_CALLBACK lpfnVideoCaptureCallback;
	MWCAP_PTR64					pvContext;

	DWORD						dwFrameSize;
	DWORD						cbStride;
} MWCAP_VIDEO_CAPTURE;

typedef void(*LPFN_AUDIO_CAPTURE_CALLBACK)(MWCAP_AUDIO_CAPTURE_FRAME* pAudioCaptureFrame, void* pvContent);
typedef struct _MWCAP_AUDIO_CAPTURE {
	_MWCAP_AUDIO_CAPTURE() :
	lpfnAudioCaptureCallback(NULL),
	pvContext(NULL) {}
	
	LPFN_AUDIO_CAPTURE_CALLBACK lpfnAudioCaptureCallback;
	MWCAP_PTR64					pvContext;
} MWCAP_AUDIO_CAPTURE;


/**
 * @ingroup group_variables_enum
 * @brief MWCAP_AUDIO_CAPTURE_NODE
 * @details Type of audio capture card\n
 * Related functions are:\n
 * 		   [MWGetAudioCaptureSupportFormat](@ref MWGetAudioCaptureSupportFormat)\n
 * 		   [MWCreateAudioCapture](@ref MWCreateAudioCapture)\n
 * 		   [MWCreateAudioCaptureWithStdCallBack](@ref MWCreateAudioCaptureWithStdCallBack)\n
*/
typedef enum _MWCAP_AUDIO_CAPTURE_NODE {
	MWCAP_AUDIO_CAPTURE_NODE_DEFAULT,																					///<Default audio capture device
	MWCAP_AUDIO_CAPTURE_NODE_EMBEDDED_CAPTURE,																			///<Default audio capture device
	MWCAP_AUDIO_CAPTURE_NODE_MICROPHONE,																				///<Microphone
	MWCAP_AUDIO_CAPTURE_NODE_USB_CAPTURE,																				///<USB audio capture
	MWCAP_AUDIO_CAPTURE_NODE_LINE_IN,																					///<Line In audio capture
} MWCAP_AUDIO_CAPTURE_NODE;						

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_AUDIO_NODE
 * @details Audio device type\n
 * Related functions are:\n
 * 		   [MWGetAudioVolume](@ref MWGetAudioVolume)\n
 * 		   [MWSetAudioVolume](@ref MWSetAudioVolume)\n
 * 		   [MWGetAudioDeviceInfo](@ref MWGetAudioDeviceInfo)\n
 * 		   [MWGetAudioDeviceInfoByIndex](@ref MWGetAudioDeviceInfoByIndex)\n
*/
typedef enum _MWCAP_AUDIO_NODE {
	MWCAP_AUDIO_MICROPHONE,																								///<Microphone
	MWCAP_AUDIO_HEADPHONE,																								///<Headset
	MWCAP_AUDIO_LINE_IN,																								///<Line In
	MWCAP_AUDIO_LINE_OUT,																								///<Line Out
	MWCAP_AUDIO_EMBEDDED_CAPTURE,																						///<Default audio capture
	MWCAP_AUDIO_EMBEDDED_PLAYBACK,																						///<Default audio play
	MWCAP_AUDIO_USB_CAPTURE,																							///<USB audio capture
	MWCAP_AUDIO_USB_PLAYBACK																							///<USB audio play
} MWCAP_AUDIO_NODE;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_AUDIO_DEVICE_INFO
 * @details Audio device information\n
 * Related functions are:\n
 * 		   [MWGetAudioDeviceInfo](@ref MWGetAudioDeviceInfo)\n
 * 		   [MWGetAudioDeviceInfoByIndex](@ref MWGetAudioDeviceInfoByIndex)\n
*/
typedef struct _MWCAP_AUDIO_DEVICE_INFO{
	TCHAR	szName[LEN_DEV_AUDIO_NAME];																					///<Audio devive name
	TCHAR	szID[LEN_DEV_AUDIO_ID];																						///<Audio device path
}MWCAP_AUDIO_DEVICE_INFO;

typedef void(*LPFN_NOTIFY_CALLBACK)(MWCAP_PTR64 pNotify, DWORD dwEnableBits, void* pvContent);
typedef struct _MWCAP_NOTIFY {
	_MWCAP_NOTIFY() : 
	pvNotifyEvent(NULL),
	hEvent(NULL),
	ullEnableBits(0LL),
	lpfnNotifyCallback(NULL),
	pvContent(NULL) {}
	
	MWCAP_PTR64				pvNotifyEvent;
	HANDLE					hEvent;
	ULONGLONG				ullEnableBits;
	LPFN_NOTIFY_CALLBACK	lpfnNotifyCallback;
	void*	pvContent;
} MWCAP_NOTIFY;


typedef void(*LPFN_TIMER_CALLBACK)(MWCAP_PTR64 pTimer, void* pvContent);
typedef struct _MWCAP_TIMER {
	_MWCAP_TIMER() :
	lpfnTimerCallback(NULL),
	llTimeout(0LL),
	llRepeat(0LL),
	llnextTime(0LL),
	pvContent(NULL) {}

	bool operator<(const _MWCAP_TIMER another) {
		return llnextTime < another.llnextTime;
	}

	LPFN_TIMER_CALLBACK		lpfnTimerCallback;
	LONGLONG				llTimeout;
	LONGLONG				llRepeat;
	LONGLONG				llnextTime;
	void*					pvContent;
} MWCAP_TIMER;


/**
 * @ingroup group_variables_struct
 * @brief VIDEO_FORMAT_INFO
 * @details Video format information\n
 * Related function:\n
 * 		   [MWGetVideoCaptureSupportFormat](@ref MWGetVideoCaptureSupportFormat)\n
*/
typedef struct _VIDEO_FORMAT_INFO
{
	UINT			cx;																									///<Width of video	
	UINT			cy;																									///<Height of video
	char			colorSpace[16];																						///<Color format
	int				nFrameDuration;																						///<Frame interval
} VIDEO_FORMAT_INFO;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_FRAMERATE
 * @details Video frame rate\n
 * Related function:\n
 * 		   [MWUSBGetVideoCaptureSupportFrameRate](@ref MWUSBGetVideoCaptureSupportFrameRate)\n
*/
typedef struct _VIDEO_FRAMERATE{
	int				nFrameDuration;																						///<Video frame interval
	float			fFPS;																								///<Video frame rate
}MWCAP_VIDEO_FRAMERATE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_FRAMERATE
 * @details Video frame rate\n
 * Related function:\n
 * 		   [MWUSBGetVideoCaptureSupportFrameRateEx](@ref MWUSBGetVideoCaptureSupportFrameRateEx)\n
*/
typedef struct _VIDEO_FRAMERATE_EX{
	int				nFrameDurationDen;																					///<Denominator of video frame rate
	int			    nFrameDurationNum;																					///<numerator of video frame rate
}MWCAP_VIDEO_FRAMERATE_EX;


/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_RESOLUTION_MODE
 * @details Return value of video resolution\n
 * Related function:\n
 * 		   [MWGetVideoCaptureSupportResolutionMode](@ref MWGetVideoCaptureSupportResolutionMode)\n
*/
typedef enum _VIDEO_RESOLUTION_MODE{
	MWCAP_VIDEO_RESOLUTION_MODE_RANGE,																					///<Return valur of supported video resolution
	MWCAP_VIDEO_RESOLUTION_MODE_LIST																					///<Return discrete list of supported video resolution
} MWCAP_VIDEO_RESOLUTION_MODE;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_RESOLUTION
 * @details Video resolution \n
 * Related types: #MWCAP_VIDEO_RESOLUTION_RANGE \n
 * 		    #MWCAP_VIDEO_RESOLUTION_LIST \n
*/
typedef struct _VIDEO_RESOLUTION{
	UINT			cx;																									///<Width of video
	UINT			cy;																									///<Height of video
}MWCAP_VIDEO_RESOLUTION;


/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_RESOLUTION_RANGE
 * @details Range of video resolution\n
 * Related function:\n
 * 		    [MWGetVideoCaptureSupportRangeResolution](@ref MWGetVideoCaptureSupportRangeResolution)\n
*/
typedef struct _VIDEO_RESOLUTION_RANGE{
	int                     nStepCx;																					///<Steps of width of supported resolution
	int                     nStepCy;																					///<Steps of height of supported resolution
	MWCAP_VIDEO_RESOLUTION	minResolution;																				///<Supported minimal resolution
	MWCAP_VIDEO_RESOLUTION	maxResolution;																				///<Supported max resolution
}MWCAP_VIDEO_RESOLUTION_RANGE;

/**
 * @ingroup group_variables_struct
 * @brief MWCAP_VIDEO_RESOLUTION_LIST
 * @details Video resolution list\n
 * Related functions is:\n
 * 		    [MWGetVideoCaptureSupportListResolution](@ref MWGetVideoCaptureSupportListResolution)\n
*/
typedef struct _VIDEO_RESOLUTION_LIST{
	int						nListSize;																					///<List size of supported resolution
	MWCAP_VIDEO_RESOLUTION*	plistResolution;																			///<Supported resolution
}MWCAP_VIDEO_RESOLUTION_LIST;


/**
 * @ingroup group_variables_struct
 * @brief AUDIO_FORMAT_INFO
 * @details Audio capture format\n
 * Related functions is:\n
 * 		    [MWGetAudioCaptureSupportFormat](@ref MWGetAudioCaptureSupportFormat)\n
*/
typedef struct _AUDIO_FORMAT_INFO
{
	UINT			nChannels;																							///<Nmuber of audio channels
	UINT			nBitCount;																							///<Bit stepth of audio sample
	UINT			nSamplerate;																						///<Audio sample rate
} AUDIO_FORMAT_INFO;

/**
 * @ingroup group_functions_callback
 * @brief Callback function of video capture
 * @details Gets video data\n
 * Related functions is:\n
 * 		   [MWCreateVideoCapture](@ref MWCreateVideoCapture)\n
*/
typedef void (*VIDEO_CAPTURE_CALLBACK)(BYTE *pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam);

/**
 * @ingroup group_functions_callback
 * @brief Callback function of audio capture
 * @details Gets audio data\n
 * Related functions are:\n
 * 		   [MWCreateAudioCapture](@ref MWCreateAudioCapture)\n
*/
typedef void (*AUDIO_CAPTURE_CALLBACK)(const BYTE * pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam);

/**
 * @ingroup group_functions_callback
 * @brief Callback function of video capture(Calling convention stdcall)
 * @details Gets video data\n
 * Related functions are:\n
 * 		   [MWCreateVideoCaptureWithStdCallBack](@ref MWCreateVideoCaptureWithStdCallBack)\n
*/
typedef void (__stdcall *VIDEO_CAPTURE_STDCALL_CALLBACK)(BYTE *pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam);

/**
 * @ingroup group_functions_callback
 * @brief Callback function of audio capture(Calling convention stdcall)
 * @details Gets audio data\n
 * Related functions are:\n
 * 		   [MWCreateAudioCaptureWithStdCallBack](@ref MWCreateAudioCaptureWithStdCallBack)\n
*/
typedef void (__stdcall *AUDIO_CAPTURE_STDCALL_CALLBACK)(const BYTE * pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam);

/**
 * @ingroup group_variables_enum
 * @brief MWCAP_VIDEO_PROC_PARAM_TYPE
 * @details Defines parameters type of video process.\n
 * Related functions are: \n
 * 		   [MWGetVideoProcParamRange](@ref MWGetVideoProcParamRange)\n
 * 		   [MWGetVideoProcParam](@ref MWGetVideoProcParam)\n
 * 		   [MWSetVideoProcParam](@ref MWSetVideoProcParam)\n
*/
typedef enum _VIDEO_PROC_PARAM_TYPE
{
	MWCAP_VIDEO_PROC_BRIGHTNESS,																						///<Brightness
	MWCAP_VIDEO_PROC_CONTRAST,																							///<Contrast
	MWCAP_VIDEO_PROC_HUE,																								///<Hue
	MWCAP_VIDEO_PROC_SATURATION																							///<Saturation
} MWCAP_VIDEO_PROC_PARAM_TYPE;

// USB

typedef void* HUSBCHANNEL;

typedef enum _USBHOT_PLUG_EVENT {
    MWCAP_USBHOT_PLUG_EVENT_DEVICE_ARRIVED = 1,
    MWCAP_USBHOT_PLUG_EVENT_DEVICE_LEFT,
}MWCAP_USBHOT_PLUG_EVENT;

typedef void(*LPFN_HOT_PLUG_CALLBACK)(MWCAP_USBHOT_PLUG_EVENT event, const char *pszDevicePath, void* pParam);
typedef void(*LPFN_HOT_PLUG_STDCALL_CALLBACK)(MWCAP_USBHOT_PLUG_EVENT event, const char *pszDevicePath, void* pParam);

#define MW_SERIAL_NO_LEN				16
#define MW_FAMILY_NAME_LEN				64
#define MW_PRODUCT_NAME_LEN				64
#define MW_FIRMWARE_NAME_LEN			64

#define MWCAP_MAX_NUM_VIDEO_OUTPUT_FOURCC			3
#define MWCAP_MAX_NUM_VIDEO_OUTPUT_FRAME_SIZE		24
#define MWCAP_MAX_NUM_VIDEO_OUTPUT_FRAME_INTERVAL	8

typedef struct _MWCAP_USB_VIDEO_PROCESS_SETTINGS {
	DWORD											dwProcessSwitchs;		// MWCAP_VIDEO_PROCESS_xx
	MWRECT											rectSource;
	int												nAspectX;
	int												nAspectY;
	BOOLEAN											bLowLatency;
	MWCAP_VIDEO_COLOR_FORMAT						colorFormat;
	MWCAP_VIDEO_QUANTIZATION_RANGE					quantRange;
	MWCAP_VIDEO_SATURATION_RANGE					satRange;
	MWCAP_VIDEO_DEINTERLACE_MODE					deinterlaceMode;
	MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE			aspectRatioConvertMode;
} MWCAP_USB_VIDEO_PROCESS_SETTINGS;

typedef enum _MWCAP_USB_AUDIO_NODE MWCAP_USB_AUDIO_NODE;
typedef enum _MWCAP_STATUS_IMAGE_MODE MWCAP_STATUS_IMAGE_MODE;
typedef enum _MWCAP_DEVICE_NAME_MODE MWCAP_DEVICE_NAME_MODE;
#pragma pack(pop)