////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2020 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "MWCaptureExtension.h"
#include "MWFOURCC.h"

struct __declspec(uuid("3A45B930-FF5C-42AA-A5A2-43771B93EBEF")) IMWCaptureExtension;
DECLARE_INTERFACE_(IMWCaptureExtension, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	// Device information
	STDMETHOD(GetChannelInfo)(THIS_
		MWCAP_CHANNEL_INFO * pChannelInfo
		) PURE;

	STDMETHOD(GetFamilyInfo)(THIS_
		void * pvFamilyInfo,
		ULONG * pcbFamilyInfo
		) PURE;

	STDMETHOD(GetVideoCaps)(THIS_
		MWCAP_VIDEO_CAPS * pVideoCaps
		) PURE;

	STDMETHOD(GetAudioCaps)(THIS_
		MWCAP_AUDIO_CAPS * pAudioCaps
		) PURE;

	STDMETHOD(GetDeviceInstanceID)(THIS_
		WCHAR * pszInstanceID,
		ULONG * pcbInstanceID
		) PURE;

	// Event notification
	STDMETHOD(RegisterNotify)(THIS_
		HANDLE hEvent,
		ULONGLONG ullEnableBits,
		MWCAP_PTR64& pvNotifyEvent
		) PURE;

	STDMETHOD(UnregisterNotify)(THIS_
		MWCAP_PTR64 pvNotifyEvent
		) PURE;

	STDMETHOD(GetNotifyStatus)(THIS_
		MWCAP_PTR64 pvNotifyEvent,
		ULONGLONG& ullStatusBits
		) PURE;

	// Input source
	STDMETHOD(GetVideoInputSourceCount)(THIS_
		DWORD * pcInputSources
		) PURE;

	STDMETHOD(GetVideoInputSourceArray)(THIS_
		DWORD * pdwInputSources,
		DWORD * pcBufferItems
		) PURE;

	STDMETHOD(GetAudioInputSourceCount)(THIS_
		DWORD * pcInputSources
		) PURE;

	STDMETHOD(GetAudioInputSourceArray)(THIS_
		DWORD * pdwInputSources,
		DWORD * pcBufferItems
		) PURE;

	STDMETHOD(SetInputSourceScan)(THIS_
		BOOLEAN bInputSourceScan
		) PURE;

	STDMETHOD(GetInputSourceScan)(THIS_
		BOOLEAN * pbInputSourceScan
		) PURE;

	STDMETHOD(GetInputSourceScanState)(THIS_
		BOOLEAN * pbInputSourceScanning
		) PURE;

	STDMETHOD(SetAVInputSourceLink)(THIS_
		BOOLEAN bAVLink
		) PURE;

	STDMETHOD(GetAVInputSourceLink)(THIS_
		BOOLEAN * pbAVLink
		) PURE;

	STDMETHOD(SetVideoInputSource)(THIS_
		DWORD dwInputSource
		) PURE;

	STDMETHOD(GetVideoInputSource)(THIS_
		DWORD * pdwInputSource
		) PURE;

	STDMETHOD(SetAudioInputSource)(THIS_
		DWORD dwInputSource
		) PURE;

	STDMETHOD(GetAudioInputSource)(THIS_
		DWORD * pdwInputSource
		) PURE;

	// EDID
	STDMETHOD(SetEDID)(THIS_
		LPBYTE pbyEDID,
		ULONG cbEDID
		) PURE;

	STDMETHOD(GetEDID)(THIS_
		LPBYTE pbyEDID,
		ULONG * pcbEDID
		) PURE;

	// Signal status
	STDMETHOD(GetInputSpecificStatus)(THIS_
		MWCAP_INPUT_SPECIFIC_STATUS * pStatus
		) PURE;

	STDMETHOD(GetVideoSignalStatus)(THIS_
		MWCAP_VIDEO_SIGNAL_STATUS * pStatus
		) PURE;

	STDMETHOD(GetAudioSignalStatus)(THIS_
		MWCAP_AUDIO_SIGNAL_STATUS * pStatus
		) PURE;

	// HDMI InfoFrames
	STDMETHOD(GetHDMIInfoFrameValidFlags)(THIS_
		DWORD * pdwValidFlags
		) PURE;

	STDMETHOD(GetHDMIInfoFramePacket)(THIS_
		MWCAP_HDMI_INFOFRAME_ID id,
		HDMI_INFOFRAME_PACKET * pInfoFramePacket
		) PURE;

	// Video processing
	STDMETHOD(SetVideoInputAspectRatio)(THIS_
		const MWCAP_VIDEO_ASPECT_RATIO * pAspectRatio
		) PURE;

	STDMETHOD(GetVideoInputAspectRatio)(THIS_
		MWCAP_VIDEO_ASPECT_RATIO * pAspectRatio
		) PURE;

	STDMETHOD(SetVideoInputColorFormat)(THIS_
		MWCAP_VIDEO_COLOR_FORMAT colorFormat
		) PURE;

	STDMETHOD(GetVideoInputColorFormat)(THIS_
		MWCAP_VIDEO_COLOR_FORMAT * pColorFormat
		) PURE;

	STDMETHOD(SetVideoInputQuantizationRange)(THIS_
		MWCAP_VIDEO_QUANTIZATION_RANGE quantRange
		) PURE;

	STDMETHOD(GetVideoInputQuantizationRange)(THIS_
		MWCAP_VIDEO_QUANTIZATION_RANGE * pQuantRange
		) PURE;

	STDMETHOD(GetVideoCaptureConnectionFormat)(THIS_
		MWCAP_VIDEO_CONNECTION_FORMAT * pFormat
		) PURE;

	STDMETHOD(SetVideoCaptureProcessPreset)(THIS_
		const MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoCaptureProcessPreset)(THIS_
		MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(SetVideoCaptureProcessSettings)(THIS_
		const MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoCaptureProcessSettings)(THIS_
		MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(SetVideoCaptureOSDPreset)(THIS_
		const MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoCaptureOSDPreset)(THIS_
		MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	STDMETHOD(SetVideoCaptureOSDSettings)(THIS_
		const MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoCaptureOSDSettings)(THIS_
		MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoPreviewConnectionFormat)(THIS_
		MWCAP_VIDEO_CONNECTION_FORMAT * pFormat
		) PURE;

	STDMETHOD(SetVideoPreviewProcessPreset)(THIS_
		const MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoPreviewProcessPreset)(THIS_
		MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(SetVideoPreviewProcessSettings)(THIS_
		const MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoPreviewProcessSettings)(THIS_
		MWCAP_VIDEO_PROCESS_SETTINGS * pSettings
		) PURE;

	STDMETHOD(SetVideoPreviewOSDPreset)(THIS_
		const MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoPreviewOSDPreset)(THIS_
		MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	STDMETHOD(SetVideoPreviewOSDSettings)(THIS_
		const MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	STDMETHOD(GetVideoPreviewOSDSettings)(THIS_
		MWCAP_VIDEO_OSD_SETTINGS * pSettings
		) PURE;

	// Temperature
	STDMETHOD(GetCoreTemperature)(THIS_
		LONG * plTemperatureDegC_X10
		) PURE;

	// VGA/Component timings
	STDMETHOD(SetVideoAutoHAlign)(THIS_
		BOOLEAN bAutoHAlign
		) PURE;

	STDMETHOD(GetVideoAutoHAlign)(THIS_
		BOOLEAN * pbAutoHAlign
		) PURE;

	STDMETHOD(SetVideoSamplingPhase)(THIS_
		BYTE bySamplingPhase
		) PURE;

	STDMETHOD(GetVideoSamplingPhase)(THIS_
		BYTE * pbySamplingPhase
		) PURE;

	STDMETHOD(SetVideoSamplingPhaseAutoAdjust)(THIS_
		BOOLEAN bAuto
		) PURE;

	STDMETHOD(GetVideoSamplingPhaseAutoAdjust)(THIS_
		BOOLEAN * pbAuto
		) PURE;

	STDMETHOD(SetVideoTiming)(THIS_
		MWCAP_VIDEO_TIMING * pTiming
		) PURE;

	STDMETHOD(GetPreferredVideoTimings)(THIS_
		MWCAP_VIDEO_TIMING * pTiming,
		DWORD * pcTimings
		) PURE;

	STDMETHOD(SetCustomVideoTiming)(THIS_
		MWCAP_VIDEO_CUSTOM_TIMING * pTiming
		) PURE;

	STDMETHOD(GetCustomVideoTimingsCount)(THIS_
		DWORD * pcTimings
		) PURE;

	STDMETHOD(GetCustomVideoTimingsArray)(THIS_
		MWCAP_VIDEO_CUSTOM_TIMING * pTiming,
		DWORD * pcTimings
		) PURE;

	STDMETHOD(SetCustomVideoTimingsArray)(THIS_
		const MWCAP_VIDEO_CUSTOM_TIMING * pTiming,
		DWORD cTimings
		) PURE;

	STDMETHOD(GetCustomVideoResolutionsCount)(THIS_
		DWORD * pcResolutions
		) PURE;

	STDMETHOD(GetCustomVideoResolutionsArray)(THIS_
		SIZE * pResolutions,
		DWORD * pcResolutions
		) PURE;

	STDMETHOD(SetCustomVideoResolutionsArray)(THIS_
		const SIZE * pResolutions,
		DWORD cResolutions
		) PURE;

	// Extension query
	STDMETHOD(IsSupportProExtension)(THIS
		) PURE;

	STDMETHOD(IsSupportHDMIExtension)(THIS
		) PURE;

	STDMETHOD(IsSupportTimingExtension)(THIS
		) PURE;

	STDMETHOD(IsSupportOSDExtension)(THIS
		) PURE;

	STDMETHOD(IsSupportVideoExtension)(THIS
		) PURE;
};

