/*
 *      Copyright (C) 2025 Matt Khan
 *      https://github.com/3ll3d00d/mwcapture
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#define NOMINMAX

#ifndef NO_QUILL
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"
#include <string_view>
#include "quill/std/WideString.h"
#endif // !NO_QUILL

#include <windows.h>
#include <process.h>
#include <DXVA.h>
#include <filesystem>
#include <utility>
 // linking side data GUIDs fails without this
#include "mwcapture.h"

#include <initguid.h>

#include <cmath>
// std::reverse
#include <algorithm>

#ifdef _DEBUG
#define MIN_LOG_LEVEL quill::LogLevel::TraceL3
#else
#define MIN_LOG_LEVEL quill::LogLevel::TraceL2
#endif

#define BACKOFF Sleep(20)
#define SHORT_BACKOFF Sleep(1)
#define S_PARTIAL_DATABURST    ((HRESULT)2L)
#define S_POSSIBLE_BITSTREAM    ((HRESULT)3L)
#define S_NO_CHANNELS    ((HRESULT)2L)

constexpr auto bitstreamDetectionWindowSecs = 0.075;
constexpr auto bitstreamDetectionRetryAfter = 1.0 / bitstreamDetectionWindowSecs;
constexpr auto bitstreamBufferSize = 6144;
constexpr auto unity = 1.0;
constexpr auto chromaticity_scale_factor = 0.00002;
constexpr auto high_luminance_scale_factor = 1.0;
constexpr auto low_luminance_scale_factor = 0.0001;

// bit depth -> pixel encoding -> fourcc
constexpr DWORD fourcc[3][4] = {
	// RGB444, YUV422, YUV444, YUY420
	{MWFOURCC_BGR24, MWFOURCC_NV16, MWFOURCC_AYUV, MWFOURCC_NV12}, // 8  bit
	{MWFOURCC_BGR10, MWFOURCC_P210, MWFOURCC_AYUV, MWFOURCC_P010}, // 10 bit
	{MWFOURCC_BGR10, MWFOURCC_P210, MWFOURCC_AYUV, MWFOURCC_P010}, // 12 bit
};
constexpr std::string_view fourccName[3][4] = {
	{"BGR24", "NV16", "AYUV", "NV12"},
	{"BGR10", "P210", "AYUV", "P010"},
	{"BGR10", "P210", "AYUV", "P010"},
};

//////////////////////////////////////////////////////////////////////////
// MagewellCaptureFilter
//////////////////////////////////////////////////////////////////////////
CUnknown* MagewellCaptureFilter::CreateInstance(LPUNKNOWN punk, HRESULT* phr)
{
	auto pNewObject = new MagewellCaptureFilter(punk, phr);

	if (pNewObject == nullptr)
	{
		if (phr)
			*phr = E_OUTOFMEMORY;
	}

	return pNewObject;
}

STDMETHODIMP MagewellCaptureFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER)

	if (riid == _uuidof(IReferenceClock))
	{
		return GetInterface(static_cast<IReferenceClock*>(this), ppv);
	}
	if (riid == _uuidof(IAMFilterMiscFlags))
	{
		return GetInterface(static_cast<IAMFilterMiscFlags*>(this), ppv);
	}
	if (riid == _uuidof(ISpecifyPropertyPages2))
	{
		return GetInterface(static_cast<ISpecifyPropertyPages2*>(this), ppv);
	}
	if (riid == IID_ISpecifyPropertyPages)
	{
		return GetInterface(static_cast<ISpecifyPropertyPages*>(this),ppv);
	}
	if (riid == IID_ISignalInfo)
	{
		return GetInterface(static_cast<ISignalInfo*>(this), ppv);
	}
	return CSource::NonDelegatingQueryInterface(riid, ppv);
}

MagewellCaptureFilter::MagewellCaptureFilter(LPUNKNOWN punk, HRESULT* phr) :
	CSource(L"MagewellCaptureFilter", punk, CLSID_MWCAPTURE_FILTER)
{
	#ifndef NO_QUILL
	// TODO make logging configurable
	// start logging with reduced cpu load
	quill::BackendOptions bopt;
	bopt.enable_yield_when_idle = true;
	bopt.sleep_duration = std::chrono::nanoseconds(0);
	quill::Backend::start(bopt);

	// to a file suffixed with the startdate/time
	auto fileSink = CustomFrontend::create_or_get_sink<quill::FileSink>(
		(std::filesystem::temp_directory_path() / "magewell_capture.log").string(),
		[]()
		{
			quill::FileSinkConfig cfg;
			cfg.set_open_mode('w');
			cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
			return cfg;
		}(),
			quill::FileEventNotifier{});
	mLogger =
		CustomFrontend::create_or_get_logger("filter",
			std::move(fileSink),
			quill::PatternFormatterOptions{
				"%(time) [%(thread_id)] %(short_source_location:<28) "
				"LOG_%(log_level:<9) %(logger:<12) %(message)",
				"%H:%M:%S.%Qns",
				quill::Timezone::GmtTime
			});

	// printing absolutely everything we may ever log
	mLogger->set_log_level(MIN_LOG_LEVEL);
	#endif // !NO_QUILL

	// Initialise the device and validate that it presents some form of data
	mInited = MWCaptureInitInstance();
	#ifndef NO_QUILL
	if (!mInited)
	{
		LOG_ERROR(mLogger, "Unable to init");
	}
	#endif

	CAutoLock lck(&m_cStateLock);
	DEVICE_INFO* diToUse(nullptr);
	int channelCount = MWGetChannelCount();
	// TODO read HKEY_LOCAL_MACHINE L"Software\\mwcapture\\devicepath"
	for (int i = 0; i < channelCount; i++)
	{
		DEVICE_INFO di{};
		MWCAP_CHANNEL_INFO mci;
		MWGetChannelInfoByIndex(i, &mci);
		if (0 == strcmp(mci.szFamilyName, "Pro Capture"))
		{
			di.deviceType = PRO;
			di.serialNo = std::string{ mci.szBoardSerialNo };
		}
		else if (0 == strcmp(mci.szFamilyName, "USB Capture"))
		{
			di.deviceType = USB;
			di.serialNo = std::string{ mci.szBoardSerialNo };
			// TODO use MWCAP_DEVICE_NAME_MODE and MWUSBGetDeviceNameMode mode?
		}

		MWGetDevicePath(i, di.devicePath);
		di.hChannel = MWOpenChannelByPath(di.devicePath);
		if (di.hChannel == nullptr)
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Unable to open channel on {} device {} at path {}, ignoring", mLogPrefix,
				devicetype_to_name(di.deviceType), di.serialNo, std::wstring{ di.devicePath });
			#endif
			continue;
		}
		DWORD videoInputTypeCount = 0;
		if (MW_SUCCEEDED != MWGetVideoInputSourceArray(di.hChannel, nullptr, &videoInputTypeCount))
		{
			MWCloseChannel(di.hChannel);
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Unable to detect video inputs on {} device {} at path {}, ignoring", mLogPrefix,
				devicetype_to_name(di.deviceType), di.serialNo, std::wstring{ di.devicePath });
			#endif
			continue;
		}
		DWORD videoInputTypes[16] = { 0 };
		if (MW_SUCCEEDED != MWGetVideoInputSourceArray(di.hChannel, videoInputTypes, &videoInputTypeCount)) {
			MWCloseChannel(di.hChannel);
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Unable to load supported video input types on {} device {} at path {}, ignoring", mLogPrefix,
				devicetype_to_name(di.deviceType), di.serialNo, std::wstring{ di.devicePath });
			#endif
			continue;
		}
		boolean hdmiFound = false;
		for (int j = 0; j < videoInputTypeCount && !hdmiFound; j++) {
			if (INPUT_TYPE(videoInputTypes[j]) == MWCAP_VIDEO_INPUT_TYPE_HDMI) {
				#ifndef NO_QUILL
				LOG_WARNING(mLogger, "[{}] Found HDMI input at position {} on {} device {} at path {}, ignoring", mLogPrefix,
					j, devicetype_to_name(di.deviceType), di.serialNo, std::wstring{ di.devicePath });
				#endif
				hdmiFound = true;
			}
		}
		if (!hdmiFound)
		{
			MWCloseChannel(di.hChannel);
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Found device but no HDMI input available on {} device {} at path {}, ignoring", mLogPrefix,
				devicetype_to_name(di.deviceType), di.serialNo, std::wstring{ di.devicePath });
			#endif
			continue;
		}

		// TODO match against targetPath
		if (diToUse == nullptr) // && di.devicePath == targetPath || targetPath == nullptr
		{
			#ifndef NO_QUILL
			LOG_INFO(mLogger, "[{}] Filter will use {} device {} at path {}", mLogPrefix, devicetype_to_name(di.deviceType),
				di.serialNo, std::wstring{ di.devicePath });
			#endif

			diToUse = &di;
			MWGetDevicePath(i, mDeviceInfo.devicePath);
			mDeviceInfo.serialNo += diToUse->serialNo;
			mDeviceInfo.deviceType = diToUse->deviceType;
			mDeviceInfo.hChannel = diToUse->hChannel;
		}
		else
		{
			#ifndef NO_QUILL
			LOG_INFO(mLogger, "[{}] Ignoring usable {} device {} at path {}", mLogPrefix, devicetype_to_name(di.deviceType),
				di.serialNo, std::wstring{ di.devicePath });
			#endif

			MWCloseChannel(di.hChannel);
			di.hChannel = nullptr;
		}
	}


	if (diToUse == nullptr)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "No valid channels found");
		#endif

		// TODO throw
	}
	else
	{
		OnDeviceSelected();
	}

	mClock = new MWReferenceClock(phr, mDeviceInfo.hChannel, mDeviceInfo.deviceType == PRO);

	new MagewellVideoCapturePin(phr, this, false);
	new MagewellVideoCapturePin(phr, this, true);
	new MagewellAudioCapturePin(phr, this, false);
	new MagewellAudioCapturePin(phr, this, true);
}

MagewellCaptureFilter::~MagewellCaptureFilter()
{
	if (mInited)
	{
		MWCaptureExitInstance();
	}
}

void MagewellCaptureFilter::GetReferenceTime(REFERENCE_TIME* rt) const
{
	m_pClock->GetTime(rt);
}

void MagewellCaptureFilter::OnVideoSignalLoaded(VIDEO_SIGNAL* vs)
{
	mVideoInputStatus.inX = vs->signalStatus.cx;
	mVideoInputStatus.inY = vs->signalStatus.cy;
	mVideoInputStatus.inAspectX = vs->signalStatus.nAspectX;
	mVideoInputStatus.inAspectY = vs->signalStatus.nAspectY;
	mVideoInputStatus.inFps = vs->signalStatus.dwFrameDuration > 0 ? 10000000.0 / vs->signalStatus.dwFrameDuration : 0.0;

	switch (vs->signalStatus.state)
	{
	case MWCAP_VIDEO_SIGNAL_NONE:
		mVideoInputStatus.signalStatus = "No Signal";
		break;
	case MWCAP_VIDEO_SIGNAL_UNSUPPORTED:
		mVideoInputStatus.signalStatus = "Unsupported Signal";
		break;
	case MWCAP_VIDEO_SIGNAL_LOCKING:
		mVideoInputStatus.signalStatus = "Locking";
		break;
	case MWCAP_VIDEO_SIGNAL_LOCKED:
		mVideoInputStatus.signalStatus = "Locked";
		break;
	}

	switch (vs->signalStatus.colorFormat)
	{
	case MWCAP_VIDEO_COLOR_FORMAT_UNKNOWN:
		mVideoInputStatus.inColourFormat = "?";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_RGB:
		mVideoInputStatus.inColourFormat = "RGB";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV601:
		mVideoInputStatus.inColourFormat = "YUV601";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV709:
		mVideoInputStatus.inColourFormat = "YUV709";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV2020:
		mVideoInputStatus.inColourFormat = "YUV2020";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV2020C:
		mVideoInputStatus.inColourFormat = "YUV2020C";
		break;
	}

	switch (vs->signalStatus.quantRange)
	{
	case MWCAP_VIDEO_QUANTIZATION_UNKNOWN:
		mVideoInputStatus.inQuantisation = "?";
		break;
	case MWCAP_VIDEO_QUANTIZATION_LIMITED:
		mVideoInputStatus.inQuantisation = "Limited";
		break;
	case MWCAP_VIDEO_QUANTIZATION_FULL:
		mVideoInputStatus.inQuantisation = "Full";
		break;
	}

	switch (vs->signalStatus.satRange)
	{
	case MWCAP_VIDEO_SATURATION_UNKNOWN:
		mVideoInputStatus.inSaturation = "?";
		break;
	case MWCAP_VIDEO_SATURATION_LIMITED:
		mVideoInputStatus.inSaturation = "Limited";
		break;
	case MWCAP_VIDEO_SATURATION_FULL:
		mVideoInputStatus.inSaturation = "Full";
		break;
	case MWCAP_VIDEO_SATURATION_EXTENDED_GAMUT:
		mVideoInputStatus.inSaturation = "Extended";
		break;
	}

	mVideoInputStatus.validSignal = vs->inputStatus.bValid;
	mVideoInputStatus.inBitDepth = vs->inputStatus.hdmiStatus.byBitDepth;

	switch (vs->inputStatus.hdmiStatus.pixelEncoding)
	{
	case HDMI_ENCODING_YUV_420:
		mVideoInputStatus.inPixelLayout = "YUV 4:2:0";
		break;
	case HDMI_ENCODING_YUV_422:
		mVideoInputStatus.inPixelLayout = "YUV 4:2:2";
		break;
	case HDMI_ENCODING_YUV_444:
		mVideoInputStatus.inPixelLayout = "YUV 4:4:4";
		break;
	case HDMI_ENCODING_RGB_444:
		mVideoInputStatus.inPixelLayout = "RGB 4:4:4";
		break;
	}

	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mVideoInputStatus);
	}
}

void MagewellCaptureFilter::OnVideoFormatLoaded(VIDEO_FORMAT* vf)
{
	mVideoOutputStatus.outX = vf->cx;
	mVideoOutputStatus.outY = vf->cy;
	mVideoOutputStatus.outAspectX = vf->aspectX;
	mVideoOutputStatus.outAspectY = vf->aspectY;
	mVideoOutputStatus.outFps = vf->fps;

	switch (vf->colourFormat)
	{
	case MWCAP_VIDEO_COLOR_FORMAT_UNKNOWN:
		mVideoOutputStatus.outColourFormat = "?";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_RGB:
		mVideoOutputStatus.outColourFormat = "RGB";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV601:
		mVideoOutputStatus.outColourFormat = "YUV601";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV709:
		mVideoOutputStatus.outColourFormat = "YUV709";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV2020:
		mVideoOutputStatus.outColourFormat = "YUV2020";
		break;
	case MWCAP_VIDEO_COLOR_FORMAT_YUV2020C:
		mVideoOutputStatus.outColourFormat = "YUV2020C";
		break;
	}

	switch (vf->quantization)
	{
	case MWCAP_VIDEO_QUANTIZATION_UNKNOWN:
		mVideoOutputStatus.outQuantisation = "?";
		break;
	case MWCAP_VIDEO_QUANTIZATION_LIMITED:
		mVideoOutputStatus.outQuantisation = "Limited";
		break;
	case MWCAP_VIDEO_QUANTIZATION_FULL:
		mVideoOutputStatus.outQuantisation = "Full";
		break;
	}

	switch (vf->saturation)
	{
	case MWCAP_VIDEO_SATURATION_UNKNOWN:
		mVideoOutputStatus.outSaturation = "?";
		break;
	case MWCAP_VIDEO_SATURATION_LIMITED:
		mVideoOutputStatus.outSaturation = "Limited";
		break;
	case MWCAP_VIDEO_SATURATION_FULL:
		mVideoOutputStatus.outSaturation = "Full";
		break;
	case MWCAP_VIDEO_SATURATION_EXTENDED_GAMUT:
		mVideoOutputStatus.outSaturation = "Extended";
		break;
	}

	mVideoOutputStatus.outBitDepth = vf->bitDepth;

	switch (vf->pixelEncoding)
	{
	case HDMI_ENCODING_YUV_420:
		mVideoOutputStatus.outPixelLayout = "YUV 4:2:0";
		break;
	case HDMI_ENCODING_YUV_422:
		mVideoOutputStatus.outPixelLayout = "YUV 4:2:2";
		break;
	case HDMI_ENCODING_YUV_444:
		mVideoOutputStatus.outPixelLayout = "YUV 4:4:4";
		break;
	case HDMI_ENCODING_RGB_444:
		mVideoOutputStatus.outPixelLayout = "RGB 4:4:4";
		break;
	}

	mVideoOutputStatus.outPixelStructure = vf->pixelStructureName;
	mVideoOutputStatus.outTransferFunction = vf->hdrMeta.transferFunction == 4 ? "REC.709" : "SMPTE ST 2084 (PQ)";

	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mVideoOutputStatus);
	}
}

void MagewellCaptureFilter::OnHdrUpdated(MediaSideDataHDR* hdr, MediaSideDataHDRContentLightLevel* light)
{
	if (hdr == nullptr)
	{
		mHdrStatus.hdrOn = false;
	}
	else
	{
		mHdrStatus.hdrOn = true;
		mHdrStatus.hdrPrimaryRX = hdr->display_primaries_x[2];
		mHdrStatus.hdrPrimaryRY = hdr->display_primaries_y[2];
		mHdrStatus.hdrPrimaryGX = hdr->display_primaries_x[0];
		mHdrStatus.hdrPrimaryGY = hdr->display_primaries_y[0];
		mHdrStatus.hdrPrimaryBX = hdr->display_primaries_x[1];
		mHdrStatus.hdrPrimaryBY = hdr->display_primaries_y[1];
		mHdrStatus.hdrWpX = hdr->white_point_x;
		mHdrStatus.hdrWpY = hdr->white_point_y;
		mHdrStatus.hdrMinDML = hdr->min_display_mastering_luminance;
		mHdrStatus.hdrMaxDML = hdr->max_display_mastering_luminance;
		mHdrStatus.hdrMaxCLL = light->MaxCLL;
		mHdrStatus.hdrMaxFALL = light->MaxFALL;
	}

	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mHdrStatus);
	}
}

void MagewellCaptureFilter::OnAudioSignalLoaded(AUDIO_SIGNAL* as)
{
	// TODO always false, is it a bug in SDK?
	// mStatusInfo.audioInStatus = as->signalStatus.bChannelStatusValid;
	mAudioInputStatus.audioInStatus = as->signalStatus.cBitsPerSample > 0;
	mAudioInputStatus.audioInIsPcm = as->signalStatus.bLPCM;
	mAudioInputStatus.audioInBitDepth = as->signalStatus.cBitsPerSample;
	mAudioInputStatus.audioInFs = as->signalStatus.dwSampleRate;
	mAudioInputStatus.audioInChannelPairs = as->signalStatus.wChannelValid;
	mAudioInputStatus.audioInChannelMap = as->audioInfo.byChannelAllocation;
	mAudioInputStatus.audioInLfeLevel = as->audioInfo.byLFEPlaybackLevel;

	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mAudioInputStatus);
	}
}

void MagewellCaptureFilter::OnAudioFormatLoaded(AUDIO_FORMAT* af)
{
	mAudioOutputStatus.audioOutChannelLayout = af->channelLayout;
	mAudioOutputStatus.audioOutBitDepth = af->bitDepth;
	mAudioOutputStatus.audioOutCodec = codecNames[af->codec];
	mAudioOutputStatus.audioOutFs = af->fs;
	constexpr double epsilon = 1e-6;
	mAudioOutputStatus.audioOutLfeOffset = std::abs(af->lfeLevelAdjustment - unity) <= epsilon * std::abs(af->lfeLevelAdjustment) ? 0 : -10;
	if (af->lfeChannelIndex == not_present)
	{
		mAudioOutputStatus.audioOutLfeChannelIndex = -1;
	}
	else
	{
		mAudioOutputStatus.audioOutLfeChannelIndex = af->lfeChannelIndex + af->channelOffsets[af->lfeChannelIndex];
	}
	mAudioOutputStatus.audioOutChannelCount = af->outputChannelCount;
	mAudioOutputStatus.audioOutDataBurstSize = af->dataBurstSize;

	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mAudioOutputStatus);
	}
}

void MagewellCaptureFilter::OnDeviceSelected()
{
	mDeviceStatus.deviceDesc = devicetype_to_name(mDeviceInfo.deviceType);
	mDeviceStatus.deviceDesc += " [";
	mDeviceStatus.deviceDesc += mDeviceInfo.serialNo;
	mDeviceStatus.deviceDesc += "]";

	#ifndef NO_QUILL
	LOG_INFO(mLogger, "[{}] Recorded device description: {}", mLogPrefix, mDeviceStatus.deviceDesc);
	#endif

	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mDeviceStatus);
	}
}

HRESULT MagewellCaptureFilter::GetTime(REFERENCE_TIME* pTime)
{
	return mClock->GetTime(pTime);
}

HRESULT MagewellCaptureFilter::AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HEVENT hEvent,
	DWORD_PTR* pdwAdviseCookie)
{
	return mClock->AdviseTime(baseTime, streamTime, hEvent, pdwAdviseCookie);
}

HRESULT MagewellCaptureFilter::AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime,
	HSEMAPHORE hSemaphore, DWORD_PTR* pdwAdviseCookie)
{
	return mClock->AdvisePeriodic(startTime, periodTime, hSemaphore, pdwAdviseCookie);
}

HRESULT MagewellCaptureFilter::Unadvise(DWORD_PTR dwAdviseCookie)
{
	return mClock->Unadvise(dwAdviseCookie);
}

ULONG MagewellCaptureFilter::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
};

HCHANNEL MagewellCaptureFilter::GetChannelHandle() const
{
	return mDeviceInfo.hChannel;
}

DeviceType MagewellCaptureFilter::GetDeviceType() const
{
	return mDeviceInfo.deviceType;
}

STDMETHODIMP MagewellCaptureFilter::GetState(DWORD dw, FILTER_STATE* pState)
{
	CBaseFilter::GetState(dw, pState);
	return *pState == State_Paused ? VFW_S_CANT_CUE : S_OK;
}

STDMETHODIMP MagewellCaptureFilter::SetSyncSource(IReferenceClock* pClock)
{
	CBaseFilter::SetSyncSource(pClock);
	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<MagewellCapturePin*>(m_paStreams[i]);
		stream->SetSyncSource(pClock);
	}
	return NOERROR;
}

HRESULT MagewellCaptureFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	auto hr = CSource::JoinFilterGraph(pGraph, pName);
	if (SUCCEEDED(hr))
	{
		for (auto i = 0; i < m_iPins; i++)
		{
			auto stream = dynamic_cast<MagewellCapturePin*>(m_paStreams[i]);
			stream->SetFilterGraph(m_pSink);
		}
	}
	return hr;
}

STDMETHODIMP MagewellCaptureFilter::Run(REFERENCE_TIME tStart)
{
	REFERENCE_TIME rt;
	GetReferenceTime(&rt);

	#ifndef NO_QUILL
	LOG_INFO(mLogger, "Filter has started running at {}", rt);
	#endif

	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<MagewellCapturePin*>(m_paStreams[i]);
		stream->SetStartTime(rt);
		stream->NotifyFilterState(State_Running, tStart);
	}
	return CBaseFilter::Run(tStart);
}

STDMETHODIMP MagewellCaptureFilter::Pause()
{
	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<MagewellCapturePin*>(m_paStreams[i]);
		stream->NotifyFilterState(State_Paused);
	}
	return CBaseFilter::Pause();
}

STDMETHODIMP MagewellCaptureFilter::Stop()
{
	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<MagewellCapturePin*>(m_paStreams[i]);
		stream->NotifyFilterState(State_Stopped);
	}
	return CBaseFilter::Stop();
}

HRESULT MagewellCaptureFilter::Reload()
{
	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mAudioInputStatus);
		mInfoCallback->Reload(&mAudioOutputStatus);
		mInfoCallback->Reload(&mVideoInputStatus);
		mInfoCallback->Reload(&mVideoOutputStatus);
		mInfoCallback->Reload(&mHdrStatus);
		mInfoCallback->Reload(&mDeviceStatus);
		return S_OK;
	}
	return E_FAIL;
}

HRESULT MagewellCaptureFilter::SetCallback(ISignalInfoCB* cb)
{
	mInfoCallback = cb;
	return S_OK;
}

STDMETHODIMP MagewellCaptureFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER)
	pPages->cElems = 1;
	pPages->pElems = static_cast<GUID*>(CoTaskMemAlloc(sizeof(GUID) * pPages->cElems));
	if (pPages->pElems == nullptr)
	{
		return E_OUTOFMEMORY;
	}
	pPages->pElems[0] = CLSID_SignalInfoProps;
	return S_OK;
}

STDMETHODIMP MagewellCaptureFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER)
	HRESULT hr = S_OK;

	if (*ppPage != nullptr)
	{
		return E_INVALIDARG;
	}

	if (guid == CLSID_SignalInfoProps)
	{
		*ppPage = new CSignalInfoProp(nullptr, &hr);
	}

	if (SUCCEEDED(hr) && *ppPage)
	{
		(*ppPage)->AddRef();
		return S_OK;
	}
	delete *ppPage;
	ppPage = nullptr;  
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////
// MagewellCapturePin
//////////////////////////////////////////////////////////////////////////
MagewellCapturePin::MagewellCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, LPCSTR pObjectName,
	LPCWSTR pPinName, std::string pLogPrefix) :
	CSourceStream(pObjectName, phr, pParent, pPinName),
	mFrameCounter(0),
	mPreview(false),
	mFilter(pParent),
	mStreamStartTime(0),
	mNotify(nullptr),
	mCaptureEvent(nullptr),
	mNotifyEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr)),
	mLastMwResult(),
	mLastSampleDiscarded(0),
	mSendMediaType(0),
	mFrameEndTime(0)
{
	#ifndef NO_QUILL
	mLogPrefix = std::move(pLogPrefix);
	mLogger = CustomFrontend::get_logger("filter");
	#endif
}

MagewellCapturePin::~MagewellCapturePin()
{
	CloseHandle(mNotifyEvent);
}

HRESULT MagewellCapturePin::HandleStreamStateChange(IMediaSample* pms)
{
	// TODO override this if MediaType changed?
	int iStreamState = CheckStreamState(pms);
	if (iStreamState == STREAM_FLOWING)
	{
		if (mLastSampleDiscarded)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Recovery after sample discard, setting discontinuity", mLogPrefix);
			#endif

			pms->SetDiscontinuity(TRUE);
			mLastSampleDiscarded = FALSE;
		}
		return S_OK;
	}
	else
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] Entering stream discard", mLogPrefix);
		#endif

		mLastSampleDiscarded = TRUE;
		return S_FALSE;
	}
}

HRESULT MagewellCapturePin::OnThreadStartPlay()
{
	#ifndef NO_QUILL
	REFERENCE_TIME rt;
	mFilter->GetReferenceTime(&rt);

	if (mStreamStartTime == 0)
	{
		LOG_WARNING(mLogger, "[{}] Pin worker thread starting at {} but stream not started yet", mLogPrefix, rt);
	}
	else
	{
		LOG_WARNING(mLogger, "[{}] Pin worker thread starting at {}, stream started at {}", mLogPrefix, rt,
			mStreamStartTime);
	}
	#endif
	return S_OK;
}

STDMETHODIMP MagewellCapturePin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER)

	if (riid == _uuidof(IAMStreamConfig))
	{
		return GetInterface(static_cast<IAMStreamConfig*>(this), ppv);
	}
	if (riid == _uuidof(IKsPropertySet))
	{
		return GetInterface(static_cast<IKsPropertySet*>(this), ppv);
	}
	if (riid == _uuidof(IAMStreamControl))
	{
		return GetInterface(static_cast<IAMStreamControl*>(this), ppv);
	}
	if (riid == _uuidof(IAMPushSource))
	{
		return GetInterface(static_cast<IAMPushSource*>(this), ppv);
	}
	return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

// largely a copy of CSourceStream but with logging replaced (as DbgLog to file seems to never ever work)
// and with better error handling to avoid visible freezes with no option but to restart
HRESULT MagewellCapturePin::DoBufferProcessingLoop(void) {
	#ifndef NO_QUILL
	LOG_INFO(mLogger, "[{}] Entering DoBufferProcessingLoop", mLogPrefix);
	#endif

	Command com;

	OnThreadStartPlay();

	do {
		while (!CheckRequest(&com)) {

			IMediaSample* pSample;

			HRESULT hrBuf = GetDeliveryBuffer(&pSample, nullptr, nullptr, 0);
			if (FAILED(hrBuf) || hrBuf == S_FALSE)
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogger, "[{}] Failed to GetDeliveryBuffer ({:#08x}), retrying", mLogPrefix, hrBuf);
				#endif
				SHORT_BACKOFF;
				continue;
			}

			HRESULT hr = FillBuffer(pSample);

			if (hr == S_OK)
			{
				hr = Deliver(pSample);
				pSample->Release();

				if (hr != S_OK)
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Failed to deliver sample downstream ({:#08x}), process loop will exit", mLogPrefix, hr);
					#endif

					return S_OK;
				}

			}
			else if (hr == S_FALSE)
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogger, "[{}] Buffer not filled, retrying", mLogPrefix);
				#endif
				pSample->Release();
			}
			else
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogger, "[{}] FillBuffer failed ({:#08x}), sending EOS and EC_ERRORABORT", mLogPrefix, hr);
				#endif

				pSample->Release();
				DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}

			// all paths release the sample
		}

		// For all commands sent to us there must be a Reply call!

		if (com == CMD_RUN || com == CMD_PAUSE)
		{
			#ifndef NO_QUILL
			LOG_INFO(mLogger, "[{}] DoBufferProcessingLoop Replying to CMD {}", mLogPrefix, static_cast<int>(com));
			#endif
			Reply(NOERROR);
		}
		else if (com != CMD_STOP)
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogger, "[{}] DoBufferProcessingLoop Replying to UNEXPECTED CMD {}", mLogPrefix, static_cast<int>(com));
			#endif
			Reply(static_cast<DWORD>(E_UNEXPECTED));
		}
		else
		{
			#ifndef NO_QUILL
			LOG_INFO(mLogger, "[{}] DoBufferProcessingLoop CMD_STOP will exit", mLogPrefix);
			#endif
		}
	} while (com != CMD_STOP);

	#ifndef NO_QUILL
	LOG_INFO(mLogger, "[{}] Exiting DoBufferProcessingLoop", mLogPrefix);
	#endif
	return S_FALSE;
}

HRESULT MagewellCapturePin::OnThreadDestroy()
{
	#ifndef NO_QUILL
	LOG_INFO(mLogger, "[{}] >>> MagewellCapturePin::OnThreadDestroy", mLogPrefix);
	#endif

	if (mNotify)
	{
		MWUnregisterNotify(mFilter->GetChannelHandle(), mNotify);
	}
	StopCapture();
	if (mCaptureEvent)
	{
		CloseHandle(mCaptureEvent);
	}

	#ifndef NO_QUILL
	LOG_INFO(mLogger, "[{}] <<< MagewellCapturePin::OnThreadDestroy", mLogPrefix);
	#endif

	return S_OK;
}

void MagewellCapturePin::SetStartTime(LONGLONG streamStartTime)
{
	mStreamStartTime = streamStartTime;

	#ifndef NO_QUILL
	LOG_WARNING(mLogger, "[{}] MagewellCapturePin::SetStartTime at {}", mLogPrefix, streamStartTime);
	#endif
}

HRESULT MagewellCapturePin::BeginFlush()
{
	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::BeginFlush", mLogPrefix);
	#endif

	this->Flushing(TRUE);
	return CSourceStream::BeginFlush();
}

HRESULT MagewellCapturePin::EndFlush()
{
	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::EndFlush", mLogPrefix);
	#endif

	this->Flushing(FALSE);
	return CSourceStream::EndFlush();
}

HRESULT MagewellCapturePin::Notify(IBaseFilter* pSelf, Quality q)
{
	// just to avoid use of DbgBreak in default implementation as we can't do anything about it given we are a slave to the device
	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::Notify {}", mLogPrefix, q.Type == 0 ? "Famine" : "Flood");
	#endif

	return S_OK;
}

// see https://learn.microsoft.com/en-us/windows/win32/api/strmif/nf-strmif-iamstreamconfig-setformat
STDMETHODIMP MagewellCapturePin::SetFormat(AM_MEDIA_TYPE* pmt)
{
	#ifndef NO_QUILL
	LOG_WARNING(mLogger, "[{}] MagewellCapturePin::SetFormat is not supported", mLogPrefix);
	#endif
	// TODO try to support this?
	return VFW_E_INVALIDMEDIATYPE;
}

STDMETHODIMP MagewellCapturePin::GetFormat(AM_MEDIA_TYPE** ppmt)
{
	CMediaType cmt;
	GetMediaType(&cmt);
	*ppmt = CreateMediaType(&cmt);
	return S_OK;
}

HRESULT MagewellCapturePin::SetMediaType(const CMediaType* pmt)
{
	HRESULT hr = CSourceStream::SetMediaType(pmt);

	#ifndef NO_QUILL
	LOG_TRACE_L3(mLogger, "[{}] SetMediaType (res: {:#08x})", mLogPrefix, hr);
	#endif

	return hr;
}

HRESULT MagewellCapturePin::DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	CheckPointer(pIMemAlloc, E_POINTER)
		CheckPointer(pProperties, E_POINTER)
		CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;
	auto acceptedUpstreamBufferCount = ProposeBuffers(pProperties);

	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DecideBufferSize size: {} count: {} (from upstream? {})",
		mLogPrefix, pProperties->cbBuffer, pProperties->cBuffers, acceptedUpstreamBufferCount);
	#endif

	ALLOCATOR_PROPERTIES actual;
	hr = pIMemAlloc->SetProperties(pProperties, &actual);

	if (FAILED(hr))
	{
		#ifndef NO_QUILL
		LOG_WARNING(mLogger, "[{}] MagewellCapturePin::DecideBufferSize failed to SetProperties result {:#08x}", mLogPrefix,
			hr);
		#endif

		return hr;
	}
	if (actual.cbBuffer < pProperties->cbBuffer)
	{
		#ifndef NO_QUILL
		LOG_WARNING(mLogger, "[{}] MagewellCapturePin::DecideBufferSize actual buffer is {} not {}", mLogPrefix,
			actual.cbBuffer, pProperties->cbBuffer);
		#endif

		return E_FAIL;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// MagewellCapturePin -> IKsPropertySet
//////////////////////////////////////////////////////////////////////////
HRESULT MagewellCapturePin::Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData,
	DWORD cbInstanceData, void* pPropData, DWORD cbPropData)
{
	// Set: Cannot set any properties.
	return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT MagewellCapturePin::Get(
	REFGUID guidPropSet, // Which property set.
	DWORD dwPropID, // Which property in that set.
	void* pInstanceData, // Instance data (ignore).
	DWORD cbInstanceData, // Size of the instance data (ignore).
	void* pPropData, // Buffer to receive the property data.
	DWORD cbPropData, // Size of the buffer.
	DWORD* pcbReturned // Return the size of the property.
)
{
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	if (pPropData == nullptr && pcbReturned == nullptr) return E_POINTER;

	if (pcbReturned) *pcbReturned = sizeof(GUID);
	if (pPropData == nullptr) return S_OK; // Caller just wants to know the size. 
	if (cbPropData < sizeof(GUID)) return E_UNEXPECTED; // The buffer is too small.

	// declares the pin to a live source capture or preview pin
	*static_cast<GUID*>(pPropData) = mPreview ? PIN_CATEGORY_PREVIEW : PIN_CATEGORY_CAPTURE;
	return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT MagewellCapturePin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
{
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	// We support getting this property, but not setting it.
	if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	return S_OK;
}

HRESULT MagewellCapturePin::RenegotiateMediaType(const CMediaType* pmt, long newSize, boolean renegotiateOnQueryAccept)
{
	auto timeout = 100;
	auto retVal = VFW_E_CHANGING_FORMAT;
	auto oldMediaType = m_mt;
	HRESULT hrQA = m_Connected->QueryAccept(pmt);

receiveconnection:

	HRESULT hr = m_Connected->ReceiveConnection(this, pmt);
	if (SUCCEEDED(hr))
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::RenegotiateMediaType ReceiveConnection accepted", mLogPrefix);
		#endif

		hr = SetMediaType(pmt);
		if (SUCCEEDED(hr))
		{
			retVal = S_OK;
		}
	}
	else if (hr == VFW_E_BUFFERS_OUTSTANDING && timeout != -1)
	{
		if (timeout > 0)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::NegotiateMediaType Buffers outstanding, retrying in 10ms..",
				mLogPrefix);
			#endif

			BACKOFF;
			timeout -= 10;
		}
		else
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(
				mLogger, "[{}] MagewellCapturePin::NegotiateMediaType Buffers outstanding, timeout reached, flushing..",
				mLogPrefix);
			#endif

			DeliverBeginFlush();
			DeliverEndFlush();
			timeout = -1;
		}
		goto receiveconnection;
	}
	else if (hrQA == S_OK) // docs say check S_OK explicitly rather than use the SUCCEEDED macro
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::NegotiateMediaType QueryAccept accepted", mLogPrefix);
		#endif

		hr = SetMediaType(pmt);
		if (SUCCEEDED(hr))
		{
			if (!renegotiateOnQueryAccept)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::NegotiateMediaType - No buffer change", mLogPrefix);
				#endif

				retVal = S_OK;
			}
			else if (nullptr != m_pInputPin)
			{
				ALLOCATOR_PROPERTIES props, actual, checkProps;
				m_pAllocator->GetProperties(&props);
				m_pAllocator->Decommit();
				props.cbBuffer = newSize;
				hr = m_pAllocator->SetProperties(&props, &actual);
				if (SUCCEEDED(hr))
				{
					hr = m_pAllocator->Commit();
					m_pAllocator->GetProperties(&checkProps);
					if (SUCCEEDED(hr))
					{
						if (checkProps.cbBuffer == props.cbBuffer && checkProps.cBuffers == props.cBuffers)
						{
							#ifndef NO_QUILL
							LOG_TRACE_L1(mLogger, "[{}] Updated allocator to {} bytes {} buffers", mLogPrefix,
								props.cbBuffer, props.cBuffers);
							#endif
							retVal = S_OK;
						}
						else
						{
							#ifndef NO_QUILL
							LOG_WARNING(
								mLogger,
								"[{}] Allocator accepted update to {} bytes {} buffers but is {} bytes {} buffers",
								mLogPrefix, props.cbBuffer, props.cBuffers, checkProps.cbBuffer, checkProps.cBuffers);
							#endif
						}
					}
					else
					{
						#ifndef NO_QUILL
						LOG_WARNING(mLogger, "[{}] Allocator did not accept update to {} bytes {} buffers [{:#08x}]",
							mLogPrefix, props.cbBuffer, props.cBuffers, hr);
						#endif
					}
				}
				else
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Allocator did not commit update to {} bytes {} buffers [{:#08x}]",
						mLogPrefix, props.cbBuffer, props.cBuffers, hr);
					#endif
				}
			}
		}
	}
	else
	{
		#ifndef NO_QUILL
		LOG_WARNING(
			mLogger, "[{}] MagewellCapturePin::NegotiateMediaType Receive Connection failed (hr: {:#08x}); QueryAccept: {:#08x}",
			mLogPrefix, hr, hrQA);
		#endif
	}
	if (retVal == S_OK)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::NegotiateMediaType succeeded", mLogPrefix);
		#endif

		mSendMediaType = TRUE;
	}
	else
	{
		// reinstate the old formats otherwise we're stuck thinking we have the new format
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::NegotiateMediaType failed {:#08x}", mLogPrefix, retVal);
		#endif

		SetMediaType(&oldMediaType);
	}

	return retVal;
}

//////////////////////////////////////////////////////////////////////////
//  MagewellVideoCapturePin::VideoCapture
//////////////////////////////////////////////////////////////////////////
MagewellVideoCapturePin::VideoCapture::VideoCapture(MagewellVideoCapturePin* pin, HCHANNEL hChannel) :
	pin(pin)
{
	mEvent = MWCreateVideoCapture(hChannel, pin->mVideoFormat.cx, pin->mVideoFormat.cy, pin->mVideoFormat.pixelStructure, 
		pin->mVideoFormat.frameInterval, CaptureFrame, pin);
	#ifndef NO_QUILL
	if (mEvent == nullptr)
	{
		LOG_ERROR(pin->mLogger, "[{}] MWCreateVideoCapture failed {}x{} {} {}", pin->mLogPrefix, 
			pin->mVideoFormat.cx, pin->mVideoFormat.cy, pin->mVideoFormat.pixelStructureName, pin->mVideoFormat.frameInterval);
	}
	else
	{
		LOG_INFO(pin->mLogger, "[{}] MWCreateVideoCapture succeeded {}x{} {} {}", pin->mLogPrefix,
			pin->mVideoFormat.cx, pin->mVideoFormat.cy, pin->mVideoFormat.pixelStructureName, pin->mVideoFormat.frameInterval);
	}
	#endif
}

MagewellVideoCapturePin::VideoCapture::~VideoCapture()
{
	if (mEvent != nullptr)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L3(pin->mLogger, "[{}] ~VideoCapture", pin->mLogPrefix);
		#endif

		#ifndef NO_QUILL
		LOG_TRACE_L3(pin->mLogger, "[{}] Ready to MWDestoryVideoCapture", pin->mLogPrefix);
		#endif

		const auto hr = MWDestoryVideoCapture(mEvent);
		mEvent = nullptr;

		#ifndef NO_QUILL
		if (MW_SUCCEEDED == hr)
		{
			LOG_INFO(pin->mLogger, "[{}] MWDestoryVideoCapture complete", pin->mLogPrefix);
		}
		else
		{
			LOG_WARNING(pin->mLogger, "[{}] MWDestoryVideoCapture failed", pin->mLogPrefix);
		}
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////
//  MagewellVideoCapturePin::VideoFrameGrabber
//////////////////////////////////////////////////////////////////////////
MagewellVideoCapturePin::VideoFrameGrabber::VideoFrameGrabber(MagewellVideoCapturePin* pin,
	HCHANNEL hChannel, DeviceType deviceType, IMediaSample* pms) :
	hChannel(hChannel),
	deviceType(deviceType),
	pin(pin),
	pms(pms)
{
	this->pms->GetPointer(&pmsData);

	if (deviceType == PRO)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L2(pin->mLogger, "[{}] Pinning {} bytes", this->pin->mLogPrefix, this->pms->GetSize());
		#endif
		MWPinVideoBuffer(hChannel, pmsData, this->pms->GetSize());
	}
}

MagewellVideoCapturePin::VideoFrameGrabber::~VideoFrameGrabber()
{
	if (deviceType == PRO)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L2(pin->mLogger, "[{}] Unpinning {} bytes, captured {} bytes", pin->mLogPrefix, pms->GetSize(),
			pms->GetActualDataLength());
		#endif

		MWUnpinVideoBuffer(hChannel, pmsData);
	}
}

HRESULT MagewellVideoCapturePin::VideoFrameGrabber::grab() const
{
	auto retVal = S_OK;
	auto hasFrame = false;
	auto proDevice = deviceType == PRO;
	auto mustExit = false;
	while (!hasFrame && !mustExit)
	{
		if (proDevice)
		{
			pin->mLastMwResult = MWGetVideoBufferInfo(hChannel, &pin->mVideoSignal.bufferInfo);
			if (pin->mLastMwResult != MW_SUCCEEDED)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(pin->mLogger, "[{}] Can't get VideoBufferInfo ({})", pin->mLogPrefix, static_cast<int>(pin->mLastMwResult));
				#endif

				continue;
			}

			pin->mLastMwResult = MWGetVideoFrameInfo(hChannel, pin->mVideoSignal.bufferInfo.iNewestBuffered,
				&pin->mVideoSignal.frameInfo);
			if (pin->mLastMwResult != MW_SUCCEEDED)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(pin->mLogger, "[{}] Can't get VideoFrameInfo ({})", pin->mLogPrefix, static_cast<int>(pin->mLastMwResult));
				#endif

				continue;
			}

			pin->mLastMwResult = MWCaptureVideoFrameToVirtualAddressEx(
				hChannel,
				pin->mHasSignal ? pin->mVideoSignal.bufferInfo.iNewestBuffering : MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERING,
				pmsData,
				pin->mVideoFormat.imageSize,
				pin->mVideoFormat.lineLength,
				FALSE,
				nullptr,
				pin->mVideoFormat.pixelStructure,
				pin->mVideoFormat.cx,
				pin->mVideoFormat.cy,
				0,
				64,
				nullptr,
				nullptr,
				0,
				100,
				0,
				100,
				0,
				MWCAP_VIDEO_DEINTERLACE_BLEND,
				MWCAP_VIDEO_ASPECT_RATIO_IGNORE,
				nullptr,
				nullptr,
				pin->mVideoFormat.aspectX,
				pin->mVideoFormat.aspectY,
				pin->mVideoFormat.colourFormat,
				pin->mVideoFormat.quantization,
				pin->mVideoFormat.saturation
			);
			if (pin->mLastMwResult != MW_SUCCEEDED)
			{
				#ifndef NO_QUILL
				LOG_WARNING(pin->mLogger, "[{}] Unexpected failed call to MWCaptureVideoFrameToVirtualAddressEx ({})", pin->mLogPrefix, 
					static_cast<int>(pin->mLastMwResult));
				#endif
				break;
			}
			do
			{
				DWORD dwRet = WaitForSingleObject(pin->mCaptureEvent, 1000);
				auto skip = dwRet != WAIT_OBJECT_0;
				if (skip)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(pin->mLogger, "[{}] Unexpected capture event ({:#08x})", pin->mLogPrefix, dwRet);
					#endif

					if (dwRet == STATUS_TIMEOUT)
					{
						#ifndef NO_QUILL
						LOG_TRACE_L1(pin->mLogger, "[{}] Wait for frame has timed out", pin->mLogPrefix);
						#endif
						mustExit = true;
						break;
					}

					if (pin->CheckStreamState(nullptr) == STREAM_DISCARDING)
					{
						mustExit = true;
						break;
					}
				}

				if (skip) continue;

				pin->mLastMwResult = MWGetVideoCaptureStatus(hChannel, &pin->mVideoSignal.captureStatus);

				#ifndef NO_QUILL
				if (pin->mLastMwResult != MW_SUCCEEDED)
				{
					LOG_TRACE_L1(pin->mLogger, "[{}] MWGetVideoCaptureStatus failed ({})", pin->mLogPrefix, static_cast<int>(pin->mLastMwResult));
				}
				#endif

				hasFrame = pin->mVideoSignal.captureStatus.bFrameCompleted;

			} while (pin->mLastMwResult == MW_SUCCEEDED && !hasFrame);
		}
		else
		{
			CAutoLock lck(&pin->mCaptureCritSec);
			memcpy(pmsData, pin->mCapturedFrame.data, pin->mCapturedFrame.length);
			hasFrame = true;
		}
	}
	if (hasFrame)
	{
		pin->GetReferenceTime(&pin->mFrameEndTime);
		auto endTime = pin->mFrameEndTime - pin->mStreamStartTime;
		auto startTime = endTime - pin->mVideoFormat.frameInterval;
		pms->SetTime(&startTime, &endTime);
		pms->SetSyncPoint(TRUE);
		pin->mFrameCounter++;

		if (pin->mVideoFormat.pixelStructure == MWFOURCC_AYUV)
		{
			// TODO endianness is wrong so flip the bytes
			BYTE* istart = pmsData, * iend = istart + pin->mVideoFormat.imageSize;
			std::reverse(istart, iend);
		}

		#ifndef NO_QUILL
		LOG_TRACE_L1(pin->mLogger, "[{}] Captured video frame {} at {}", pin->mLogPrefix,
			pin->mFrameCounter, endTime);
		#endif

		if (pin->mSendMediaType)
		{
			CMediaType cmt(pin->m_mt);
			AM_MEDIA_TYPE* sendMediaType = CreateMediaType(&cmt);
			pms->SetMediaType(sendMediaType);
			DeleteMediaType(sendMediaType);
			pin->mSendMediaType = FALSE;
		}
		// Update once per second
		if (endTime > pin->mLastSentHdrMetaAt + oneSecondIn100ns)
		{
			pin->mLastSentHdrMetaAt = endTime;
			if (pin->mVideoFormat.hdrMeta.exists)
			{
				// This can fail if you have a filter behind this which does not understand side data
				IMediaSideData* pMediaSideData = nullptr;
				if (SUCCEEDED(pms->QueryInterface(&pMediaSideData)))
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(pin->mLogger, "[{}] Updating HDR meta in frame {}, last update at {}", pin->mLogPrefix,
						pin->mFrameCounter, pin->mLastSentHdrMetaAt);
					#endif

					MediaSideDataHDR hdr;
					ZeroMemory(&hdr, sizeof(hdr));

					hdr.display_primaries_x[0] = pin->mVideoFormat.hdrMeta.g_primary_x *
						chromaticity_scale_factor;
					hdr.display_primaries_x[1] = pin->mVideoFormat.hdrMeta.b_primary_x *
						chromaticity_scale_factor;
					hdr.display_primaries_x[2] = pin->mVideoFormat.hdrMeta.r_primary_x *
						chromaticity_scale_factor;
					hdr.display_primaries_y[0] = pin->mVideoFormat.hdrMeta.g_primary_y *
						chromaticity_scale_factor;
					hdr.display_primaries_y[1] = pin->mVideoFormat.hdrMeta.b_primary_y *
						chromaticity_scale_factor;
					hdr.display_primaries_y[2] = pin->mVideoFormat.hdrMeta.r_primary_y *
						chromaticity_scale_factor;

					hdr.white_point_x = pin->mVideoFormat.hdrMeta.whitepoint_x * chromaticity_scale_factor;
					hdr.white_point_y = pin->mVideoFormat.hdrMeta.whitepoint_y * chromaticity_scale_factor;

					hdr.max_display_mastering_luminance = pin->mVideoFormat.hdrMeta.maxDML *
						high_luminance_scale_factor;
					hdr.min_display_mastering_luminance = pin->mVideoFormat.hdrMeta.minDML *
						low_luminance_scale_factor;

					pMediaSideData->SetSideData(IID_MediaSideDataHDR, reinterpret_cast<const BYTE*>(&hdr),
						sizeof(hdr));

					MediaSideDataHDRContentLightLevel hdrLightLevel;
					ZeroMemory(&hdrLightLevel, sizeof(hdrLightLevel));

					hdrLightLevel.MaxCLL = pin->mVideoFormat.hdrMeta.maxCLL;
					hdrLightLevel.MaxFALL = pin->mVideoFormat.hdrMeta.maxFALL;

					pMediaSideData->SetSideData(IID_MediaSideDataHDRContentLightLevel,
						reinterpret_cast<const BYTE*>(&hdrLightLevel),
						sizeof(hdrLightLevel));
					pMediaSideData->Release();

					#ifndef NO_QUILL
					LOG_TRACE_L1(pin->mLogger, "[{}] HDR meta: R {:.4f} {:.4f}", pin->mLogPrefix,
						hdr.display_primaries_x[2], hdr.display_primaries_y[2]);
					LOG_TRACE_L1(pin->mLogger, "[{}] HDR meta: G {:.4f} {:.4f}", pin->mLogPrefix,
						hdr.display_primaries_x[0], hdr.display_primaries_y[0]);
					LOG_TRACE_L1(pin->mLogger, "[{}] HDR meta: B {:.4f} {:.4f}", pin->mLogPrefix,
						hdr.display_primaries_x[1], hdr.display_primaries_y[1]);
					LOG_TRACE_L1(pin->mLogger, "[{}] HDR meta: W {:.4f} {:.4f}", pin->mLogPrefix,
						hdr.white_point_x, hdr.white_point_y);
					LOG_TRACE_L1(pin->mLogger, "[{}] HDR meta: DML {} {}", pin->mLogPrefix,
						hdr.min_display_mastering_luminance, hdr.max_display_mastering_luminance);
					LOG_TRACE_L1(pin->mLogger, "[{}] HDR meta: MaxCLL/MaxFALL {} {}", pin->mLogPrefix,
						hdrLightLevel.MaxCLL, hdrLightLevel.MaxFALL);
					#endif

					pin->mFilter->OnHdrUpdated(&hdr, &hdrLightLevel);
				}
				else
				{
					#ifndef NO_QUILL
					LOG_WARNING(pin->mLogger, "[{}] HDR meta to send via MediaSideDataHDR but not supported by MediaSample", pin->mLogPrefix);
					#endif
				}
			}
			else
			{
				pin->mFilter->OnHdrUpdated(nullptr, nullptr);
			}
		}
	}
	else
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(pin->mLogger, "[{}] No frame loaded", pin->mLogPrefix, static_cast<int>(pin->mLastMwResult));
		#endif
	}
	return retVal;
}

//////////////////////////////////////////////////////////////////////////
// MagewellVideoCapturePin
//////////////////////////////////////////////////////////////////////////
MagewellVideoCapturePin::MagewellVideoCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, bool pPreview) :
	MagewellCapturePin(
		phr,
		pParent,
		pPreview ? "VideoPreview" : "VideoCapture",
		pPreview ? L"Preview" : L"Capture",
		pPreview ? "Preview" : "Capture"
	)
{
	auto hChannel = mFilter->GetChannelHandle();

	if (mFilter->GetDeviceType() == USB)
	{
		if (MW_SUCCEEDED == MWUSBGetVideoOutputFOURCC(hChannel, &mUsbCaptureFormats.fourccs))
		{
			if (MW_SUCCEEDED == MWUSBGetVideoOutputFrameInterval(hChannel, &mUsbCaptureFormats.frameIntervals))
			{
				if (MW_SUCCEEDED == MWUSBGetVideoOutputFrameSize(hChannel, &mUsbCaptureFormats.frameSizes))
				{
					mUsbCaptureFormats.usb = true;
				}
				else
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Could not load USB video frame sizes", mLogPrefix);
					#endif
				}
			}
			else
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogger, "[{}] Could not load USB video frame intervals", mLogPrefix);
				#endif
			}
		}
		else
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Could not load USB video FourCCs", mLogPrefix);
			#endif
		}
	}

	auto hr = LoadSignal(&hChannel);
	mFilter->OnVideoSignalLoaded(&mVideoSignal);

	if (SUCCEEDED(hr))
	{
		LoadFormat(&mVideoFormat, &mVideoSignal, &mUsbCaptureFormats);

		#ifndef NO_QUILL
		LOG_WARNING(
			mLogger, "[{}] Initialised video format {} x {} ({}:{}) @ {:.3f} Hz in {} bits ({} {} tf: {}) size {} bytes",
			mLogPrefix,
			mVideoFormat.cx, mVideoFormat.cy, mVideoFormat.aspectX, mVideoFormat.aspectY, mVideoFormat.fps,
			mVideoFormat.bitDepth,
			mVideoFormat.pixelStructureName, mVideoFormat.colourFormatName, mVideoFormat.hdrMeta.transferFunction,
			mVideoFormat.imageSize);
		#endif
	}
	else
	{
		mVideoFormat.lineLength = FOURCC_CalcMinStride(mVideoFormat.pixelStructure, mVideoFormat.cx, 2);
		mVideoFormat.imageSize = FOURCC_CalcImageSize(mVideoFormat.pixelStructure, mVideoFormat.cx, mVideoFormat.cy,
			mVideoFormat.lineLength);

		#ifndef NO_QUILL
		LOG_WARNING(
			mLogger,
			"[{}] Initialised video format using defaults {} x {} ({}:{}) @ {.3f} Hz in {} bits ({} {} tf: {}) size {} bytes",
			mLogPrefix,
			mVideoFormat.cx, mVideoFormat.cy, mVideoFormat.aspectX, mVideoFormat.aspectY, mVideoFormat.fps,
			mVideoFormat.bitDepth,
			mVideoFormat.pixelStructureName, mVideoFormat.colourFormatName, mVideoFormat.hdrMeta.transferFunction,
			mVideoFormat.imageSize);
		#endif
	}
	mFilter->OnVideoFormatLoaded(&mVideoFormat);

	if (mFilter->GetDeviceType() == USB)
	{
		mCapturedFrame.data = new BYTE[mVideoFormat.imageSize];
	}
}

void MagewellVideoCapturePin::GetReferenceTime(REFERENCE_TIME* rt) const
{
	mFilter->GetReferenceTime(rt);
}

void MagewellVideoCapturePin::LoadFormat(VIDEO_FORMAT* videoFormat, VIDEO_SIGNAL* videoSignal, USB_CAPTURE_FORMATS* captureFormats)
{
	if (videoSignal->signalStatus.state == MWCAP_VIDEO_SIGNAL_LOCKED)
	{
		videoFormat->cx = videoSignal->signalStatus.cx;
		videoFormat->cy = videoSignal->signalStatus.cy;
		videoFormat->aspectX = videoSignal->signalStatus.nAspectX;
		videoFormat->aspectY = videoSignal->signalStatus.nAspectY;
		videoFormat->quantization = videoSignal->signalStatus.quantRange;
		videoFormat->saturation = videoSignal->signalStatus.satRange;
		videoFormat->fps = 10000000.0 / videoSignal->signalStatus.dwFrameDuration;
		videoFormat->frameInterval = videoSignal->signalStatus.dwFrameDuration;
		videoFormat->bitDepth = videoSignal->inputStatus.hdmiStatus.byBitDepth;
		videoFormat->colourFormat = videoSignal->signalStatus.colorFormat;
		videoFormat->pixelEncoding = videoSignal->inputStatus.hdmiStatus.pixelEncoding;

		LoadHdrMeta(&videoFormat->hdrMeta, &videoSignal->hdrInfo);
	}
	else
	{
		// invalid/no signal is 720x480 RGB 4:4:4 image 
		videoFormat->cx = 720;
		videoFormat->cy = 480;
		videoFormat->bitDepth = 8;
		videoFormat->quantization = MWCAP_VIDEO_QUANTIZATION_FULL;
		videoFormat->saturation = MWCAP_VIDEO_SATURATION_FULL;
		videoFormat->colourFormat = MWCAP_VIDEO_COLOR_FORMAT_RGB;
		videoFormat->pixelEncoding = HDMI_ENCODING_RGB_444;
	}

	auto idx = videoFormat->bitDepth == 8 ? 0 : videoFormat->bitDepth == 10 ? 1 : 2;
	videoFormat->pixelStructure = fourcc[idx][videoFormat->pixelEncoding];
	videoFormat->pixelStructureName = fourccName[idx][videoFormat->pixelEncoding];

	if (videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV709)
	{
		videoFormat->colourFormatName = "YUV709";
	}
	else if (videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV2020)
	{
		videoFormat->colourFormatName = "YUV2020";
	}
	else if (videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_RGB)
	{
		videoFormat->colourFormatName = "RGB";
	}
	else
	{
		videoFormat->colourFormatName = "UNK";
	}

	if (captureFormats->usb)
	{
		bool found = false;
		for (int i = 0; i < captureFormats->fourccs.byCount && !found; i++) {
			if (captureFormats->fourccs.adwFOURCCs[i] == videoFormat->pixelStructure)
			{
				found = true;
			}
		}
		if (!found)
		{
			videoFormat->pixelStructure = captureFormats->fourccs.adwFOURCCs[0];
			std::string captureFormatName{ static_cast<char>(videoFormat->pixelStructure & 0xFF) };
			captureFormatName += static_cast<char>(videoFormat->pixelStructure >> 8 & 0xFF);
			captureFormatName += static_cast<char>(videoFormat->pixelStructure >> 16 & 0xFF);
			captureFormatName += static_cast<char>(videoFormat->pixelStructure >> 24 & 0xFF);
			videoFormat->pixelStructureName = captureFormatName;
		}

		found = false;
		for (int i = 0; i < captureFormats->frameIntervals.byCount && !found; i++) {
			if (abs(captureFormats->frameIntervals.adwIntervals[i] - videoFormat->frameInterval) < 100)
			{
				found = true;
			}
		}
		if (!found)
		{
			videoFormat->frameInterval = captureFormats->frameIntervals.adwIntervals[captureFormats->frameIntervals.byDefault];
		}

		found = false;
		for (int i = 0; i < captureFormats->frameSizes.byCount && !found; i++) {
			if (captureFormats->frameSizes.aSizes[i].cx == videoFormat->cx && captureFormats->frameSizes.aSizes[i].cy == videoFormat->cy)
			{
				found = true;
			}
		}
		if (!found)
		{
			videoFormat->cx = captureFormats->frameSizes.aSizes[captureFormats->frameSizes.byDefault].cx;
			videoFormat->cy = captureFormats->frameSizes.aSizes[captureFormats->frameSizes.byDefault].cy;
		}
	}


	videoFormat->bitCount = FOURCC_GetBpp(videoFormat->pixelStructure);
	videoFormat->lineLength = FOURCC_CalcMinStride(videoFormat->pixelStructure, videoFormat->cx, 2);
	videoFormat->imageSize = FOURCC_CalcImageSize(videoFormat->pixelStructure, videoFormat->cx, videoFormat->cy, videoFormat->lineLength);
}

void MagewellVideoCapturePin::LogHdrMetaIfPresent(VIDEO_FORMAT* newVideoFormat)
{
	auto hdrIf = mVideoSignal.hdrInfo;
	if (hdrIf.byEOTF || hdrIf.byMetadataDescriptorID 
		|| hdrIf.display_primaries_lsb_x0 || hdrIf.display_primaries_lsb_x1 || hdrIf.display_primaries_lsb_x2 
		|| hdrIf.display_primaries_msb_x0 || hdrIf.display_primaries_msb_x1 || hdrIf.display_primaries_msb_x2
		|| hdrIf.display_primaries_lsb_y0 || hdrIf.display_primaries_lsb_y1 || hdrIf.display_primaries_lsb_y2 
		|| hdrIf.display_primaries_msb_y0 || hdrIf.display_primaries_msb_y1 || hdrIf.display_primaries_msb_y2
		|| hdrIf.white_point_msb_x || hdrIf.white_point_msb_y || hdrIf.white_point_lsb_x || hdrIf.white_point_lsb_y
		|| hdrIf.max_display_mastering_lsb_luminance || hdrIf.max_display_mastering_msb_luminance
		|| hdrIf.min_display_mastering_lsb_luminance || hdrIf.min_display_mastering_msb_luminance
		|| hdrIf.maximum_content_light_level_lsb|| hdrIf.maximum_content_light_level_msb
		|| hdrIf.maximum_frame_average_light_level_lsb || hdrIf.maximum_frame_average_light_level_msb)
	{
		auto newMeta = newVideoFormat->hdrMeta;
		auto oldMeta = mVideoFormat.hdrMeta;
		if (newMeta.exists)
		{
			bool logPrimaries;
			bool logWp;
			bool logMax;
			if (oldMeta.exists)
			{
				logPrimaries = newMeta.r_primary_x != oldMeta.r_primary_x || newMeta.r_primary_y != oldMeta.r_primary_y
					|| newMeta.g_primary_x != oldMeta.g_primary_x || newMeta.g_primary_y != oldMeta.g_primary_y
					|| newMeta.b_primary_x != oldMeta.b_primary_x || newMeta.b_primary_y != oldMeta.b_primary_y;
				logWp = newMeta.whitepoint_x != oldMeta.whitepoint_x || newMeta.whitepoint_y != oldMeta.whitepoint_y;
				logMax = newMeta.maxCLL != oldMeta.maxCLL || newMeta.minDML != oldMeta.minDML || newMeta.maxDML != oldMeta.maxDML || newMeta.maxFALL != oldMeta.maxFALL;
				if (logPrimaries || logWp || logMax)
				{
					#ifndef NO_QUILL
					LOG_INFO(mLogger, "[{}] HDR metadata has changed", mLogPrefix);
					#endif
				}
			}
			else
			{
				logPrimaries = true;
				logWp = true;
				logMax = true;
				#ifndef NO_QUILL
				LOG_INFO(mLogger, "[{}] HDR metadata is now present", mLogPrefix);
				#endif
			}

			#ifndef NO_QUILL
			if (logPrimaries)
			{
				LOG_INFO(mLogger, "[{}] Primaries RGB {} x {} {} x {} {} x {}", mLogPrefix,
					newMeta.r_primary_x, newMeta.r_primary_y, newMeta.g_primary_x, newMeta.g_primary_y, newMeta.b_primary_x, newMeta.b_primary_y);
			}
			if (logWp)
			{
				LOG_INFO(mLogger, "[{}] Whitepoint {} x {}", mLogPrefix,
					newMeta.whitepoint_x, newMeta.whitepoint_y);
			}
			if (logMax)
			{
				LOG_INFO(mLogger, "[{}] DML/MaxCLL/MaxFALL {} / {} {} {}", mLogPrefix,
					newMeta.minDML, newMeta.maxDML, newMeta.maxCLL, newMeta.maxFALL);
			}
			#endif
		}
		else
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] HDR InfoFrame parsing failure, values are present but no metadata exists", mLogPrefix);
			#endif
		}
	}
	if (!newVideoFormat->hdrMeta.exists && mVideoFormat.hdrMeta.exists)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] HDR metadata has been removed", mLogPrefix);
		#endif
	}
}

void MagewellVideoCapturePin::VideoFormatToMediaType(CMediaType* pmt, VIDEO_FORMAT* videoFormat) const
{
	auto pvi = reinterpret_cast<VIDEOINFOHEADER2*>(pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2)));
	ZeroMemory(pvi, sizeof(VIDEOINFOHEADER2));

	pmt->SetType(&MEDIATYPE_Video);
	pmt->SetFormatType(&FORMAT_VIDEOINFO2);
	pmt->SetTemporalCompression(FALSE);
	pmt->SetSampleSize(videoFormat->imageSize);

	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
	SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle
	pvi->dwBitRate = static_cast<DWORD>(videoFormat->bitDepth * videoFormat->imageSize * 8 * videoFormat->fps);
	pvi->dwBitErrorRate = 0;
	pvi->AvgTimePerFrame = static_cast<DWORD>(static_cast<double>(10000000LL) / videoFormat->fps);
	pvi->dwInterlaceFlags = 0;
	pvi->dwPictAspectRatioX = videoFormat->aspectX;
	pvi->dwPictAspectRatioY = videoFormat->aspectY;

	// dwControlFlags is a 32bit int. With AMCONTROL_COLORINFO_PRESENT the upper 24 bits are used by DXVA_ExtendedFormat.
	// That struct is 32 bits so it's lower member (SampleFormat) is actually overbooked with the value of dwConotrolFlags
	// so can't be used. LAV has defined some out-of-spec but compatible with madVR values for the more modern formats,
	// which we use as well see
	// https://github.com/Nevcairiel/LAVFilters/blob/ddef56ae155d436f4301346408f4fdba755197d6/decoder/LAVVideo/Media.cpp

	auto colorimetry = reinterpret_cast<DXVA_ExtendedFormat*>(&(pvi->dwControlFlags));
	// 1 = REC.709, 4 = BT.2020
	colorimetry->VideoTransferMatrix = videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV2020
		? static_cast<DXVA_VideoTransferMatrix>(4)
		: DXVA_VideoTransferMatrix_BT709;
	// 1 = REC.709, 9 = BT.2020
	colorimetry->VideoPrimaries = videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV2020
		? static_cast<DXVA_VideoPrimaries>(9)
		: DXVA_VideoPrimaries_BT709;
	// 4 = REC.709, 15 = SMPTE ST 2084 (PQ)
	colorimetry->VideoTransferFunction = static_cast<DXVA_VideoTransferFunction>(videoFormat->hdrMeta.transferFunction);
	// 0 = unknown, 1 = 0-255, 2 = 16-235
	colorimetry->NominalRange = static_cast<DXVA_NominalRange>(videoFormat->quantization);

	#ifndef NO_QUILL
	LOG_TRACE_L3(mLogger, "[{}] DXVA_ExtendedFormat {} {} {} {}", mLogPrefix,
		static_cast<int>(colorimetry->VideoTransferMatrix),
		static_cast<int>(colorimetry->VideoPrimaries), static_cast<int>(colorimetry->VideoTransferFunction),
		static_cast<int>(colorimetry->NominalRange));
	#endif

	pvi->dwControlFlags += AMCONTROL_USED;
	pvi->dwControlFlags += AMCONTROL_COLORINFO_PRESENT;

	auto isRgb = FOURCC_IsRGB(videoFormat->pixelStructure);
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth = videoFormat->cx;
	pvi->bmiHeader.biHeight = isRgb ? -(videoFormat->cy) : videoFormat->cy; // RGB on windows is upside down
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biBitCount = videoFormat->bitCount;
	pvi->bmiHeader.biCompression = FOURCC_IsRGB(videoFormat->pixelStructure) ? BI_RGB : videoFormat->pixelStructure;
	pvi->bmiHeader.biSizeImage = videoFormat->imageSize;
	pvi->bmiHeader.biXPelsPerMeter = 0;
	pvi->bmiHeader.biYPelsPerMeter = 0;
	pvi->bmiHeader.biClrUsed = 0;
	pvi->bmiHeader.biClrImportant = 0;

	// Work out the GUID for the subtype from the header info.
	const auto subTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
	pmt->SetSubtype(&subTypeGUID);
}

bool MagewellVideoCapturePin::ShouldChangeMediaType(VIDEO_FORMAT* newVideoFormat)
{
	auto reconnect = false;
	if (newVideoFormat->cx != mVideoFormat.cx || newVideoFormat->cy != mVideoFormat.cy)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video dimension change {}x{} to {}x{}", mLogPrefix, mVideoFormat.cx, mVideoFormat.cy,
			newVideoFormat->cx, newVideoFormat->cy);
		#endif
	}
	if (newVideoFormat->aspectX != mVideoFormat.aspectX || newVideoFormat->aspectY != mVideoFormat.aspectY)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video AR change {}x{} to {}x{}", mLogPrefix, mVideoFormat.aspectX, mVideoFormat.aspectY,
			newVideoFormat->aspectX, newVideoFormat->aspectY);
		#endif
	}
	if (abs(newVideoFormat->frameInterval - mVideoFormat.frameInterval) >= 100)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video FPS change {:.3f} to {:.3f}", mLogPrefix, mVideoFormat.fps, newVideoFormat->fps);
		#endif
	}
	if (mVideoFormat.bitDepth != newVideoFormat->bitDepth)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video bit depth change {} to {}", mLogPrefix, mVideoFormat.bitDepth, newVideoFormat->bitDepth);
		#endif
	}
	if (mVideoFormat.pixelEncoding != newVideoFormat->pixelEncoding)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video pixel encoding change {} to {}", mLogPrefix,
			static_cast<int>(mVideoFormat.pixelEncoding),
			static_cast<int>(newVideoFormat->pixelEncoding));
		#endif
	}
	if (mVideoFormat.colourFormat != newVideoFormat->colourFormat)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video colour format change {} to {}", mLogPrefix, mVideoFormat.colourFormatName, newVideoFormat->colourFormatName);
		#endif
	}
	if (mVideoFormat.quantization != newVideoFormat->quantization || mVideoFormat.saturation != newVideoFormat->saturation)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video colorimetry change quant {} to {} sat {} to {}", mLogPrefix,
			static_cast<int>(mVideoFormat.quantization), static_cast<int>(newVideoFormat->quantization),
			static_cast<int>(mVideoFormat.saturation), static_cast<int>(newVideoFormat->saturation)
		);
		#endif
	}
	auto incomingTransferFunction = mHasSignal && mVideoSignal.hdrInfo.byEOTF == 0x2 ? 15 : 4;
	if (mVideoFormat.hdrMeta.transferFunction != incomingTransferFunction)
	{
		reconnect = true;

		#ifndef NO_QUILL
		auto formatFrom = mVideoFormat.hdrMeta.transferFunction == 4 ? "REC.709" : "SMPTE ST 2084 (PQ)";
		auto formatTo = incomingTransferFunction == 4 ? "REC.709" : "SMPTE ST 2084 (PQ)";
		LOG_INFO(mLogger, "[{}] Video transfer function change {} ({}) to {} ({})", mLogPrefix,
			formatFrom, mVideoFormat.hdrMeta.transferFunction, formatTo, incomingTransferFunction);
		#endif
	}

	return reconnect;
}

HRESULT MagewellVideoCapturePin::LoadSignal(HCHANNEL* pChannel)
{
	mLastMwResult = MWGetVideoSignalStatus(*pChannel, &mVideoSignal.signalStatus);
	auto retVal = S_OK;
	if (mLastMwResult != MW_SUCCEEDED)
	{
		#ifndef NO_QUILL
		LOG_WARNING(mLogger, "[{}] LoadSignal MWGetVideoSignalStatus failed", mLogPrefix);
		#endif

		mVideoSignal.signalStatus.state = MWCAP_VIDEO_SIGNAL_NONE;

		retVal = S_FALSE;
	}
	mLastMwResult = MWGetInputSpecificStatus(*pChannel, &mVideoSignal.inputStatus);
	if (mLastMwResult != MW_SUCCEEDED)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] LoadSignal MWGetInputSpecificStatus failed", mLogPrefix);
		#endif

		mVideoSignal.inputStatus.bValid = false;

		retVal = S_FALSE;
	}
	else if (!mVideoSignal.inputStatus.bValid)
	{
		retVal = S_FALSE;
	}

	if (retVal != S_OK)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] LoadSignal MWGetInputSpecificStatus is invalid, will display no/unsupported signal image", mLogPrefix);
		#endif

		mVideoSignal.inputStatus.hdmiStatus.byBitDepth = 8;
		mVideoSignal.inputStatus.hdmiStatus.pixelEncoding = HDMI_ENCODING_RGB_444;
		mHasHdrInfoFrame = true;
		mVideoSignal.hdrInfo = {};
		mVideoSignal.aviInfo = {};
	}
	else
	{
		DWORD tPdwValidFlag = 0;
		MWGetHDMIInfoFrameValidFlag(*pChannel, &tPdwValidFlag);
		HDMI_INFOFRAME_PACKET pkt;
		auto readPacket = false;
		if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_HDR)
		{
			if (MW_SUCCEEDED == MWGetHDMIInfoFramePacket(*pChannel, MWCAP_HDMI_INFOFRAME_ID_HDR, &pkt))
			{
				if (!mHasHdrInfoFrame)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(mLogger, "[{}] HDR Infoframe is present tf: {} to {}", mLogPrefix, mVideoSignal.hdrInfo.byEOTF,
						pkt.hdrInfoFramePayload.byEOTF);
					#endif
					mHasHdrInfoFrame = true;
				}
				mVideoSignal.hdrInfo = pkt.hdrInfoFramePayload;
				readPacket = true;
			}
		}
		if (!readPacket)
		{
			
			if (mHasHdrInfoFrame)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(mLogger, "[{}] HDR Infoframe no longer present", mLogPrefix);
				#endif
				mHasHdrInfoFrame = false;
			}
			mVideoSignal.hdrInfo = {};
		}

		readPacket = false;
		if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_AVI)
		{
			if (MW_SUCCEEDED == MWGetHDMIInfoFramePacket(*pChannel, MWCAP_HDMI_INFOFRAME_ID_AVI, &pkt))
			{
				mVideoSignal.aviInfo = pkt.aviInfoFramePayload;
				readPacket = true;
			}
		}
		if (!readPacket)
		{
			mVideoSignal.aviInfo = {};
		}
	}
	return S_OK;
}

HRESULT MagewellVideoCapturePin::DoChangeMediaType(const CMediaType* pmt, const VIDEO_FORMAT* newVideoFormat)
{
	#ifndef NO_QUILL
	LOG_WARNING(
		mLogger, "[{}] Proposing new video format {} x {} ({}:{}) @ {:.3f} Hz in {} bits ({} {} tf: {}) size {} bytes",
		mLogPrefix,
		newVideoFormat->cx, newVideoFormat->cy, newVideoFormat->aspectX, newVideoFormat->aspectY, newVideoFormat->fps,
		newVideoFormat->bitDepth,
		newVideoFormat->pixelStructureName, newVideoFormat->colourFormatName, newVideoFormat->hdrMeta.transferFunction,
		newVideoFormat->imageSize);
	#endif

	auto retVal = RenegotiateMediaType(pmt, newVideoFormat->imageSize, newVideoFormat->imageSize != mVideoFormat.imageSize);
	if (retVal == S_OK)
	{
		mFilter->NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(newVideoFormat->cx, newVideoFormat->cy), 0);
		if (mFilter->GetDeviceType() == USB)
		{
			delete mVideoCapture;
			mVideoCapture = new VideoCapture(this, mFilter->GetChannelHandle());
			if (newVideoFormat->imageSize > mVideoFormat.imageSize)
			{
				CAutoLock lck(&mCaptureCritSec);
				delete mCapturedFrame.data;
				mCapturedFrame.data = new BYTE[newVideoFormat->imageSize];
			}
		}
		mVideoFormat = *newVideoFormat;
	}

	return retVal;
}

void MagewellVideoCapturePin::CaptureFrame(BYTE* pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam)
{
	MagewellVideoCapturePin* pin = static_cast<MagewellVideoCapturePin*>(pParam);
	CAutoLock lck(&pin->mCaptureCritSec);
	memcpy(pin->mCapturedFrame.data, pbFrame, cbFrame);
	pin->mCapturedFrame.length = cbFrame;
	pin->mCapturedFrame.ts = u64TimeStamp;
	if (!SetEvent(pin->mNotifyEvent))
	{
		auto err  = GetLastError();
		#ifndef NO_QUILL
		LOG_ERROR(pin->mLogger, "[{}] Failed to notify on frame {:#08x}", pin->mLogPrefix, err);
		#endif
	}
}

// loops til we have a frame to process, dealing with any mediatype changes as we go and then grabs a buffer once it's time to go
HRESULT MagewellVideoCapturePin::GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime,
	REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	auto hasFrame = false;
	auto retVal = S_FALSE;
	auto proDevice = mFilter->GetDeviceType() == PRO;
	auto hChannel = mFilter->GetChannelHandle();

	while (!hasFrame)
	{
		if (CheckStreamState(nullptr) == STREAM_DISCARDING)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Stream is discarding", mLogPrefix);
			#endif

			break;
		}
		if (mStreamStartTime == 0)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Stream has not started, retry after backoff", mLogPrefix);
			#endif

			BACKOFF;
			continue;
		}
		auto channel = mFilter->GetChannelHandle();
		auto hr = LoadSignal(&channel);
		auto hadSignal = mHasSignal == true;

		mHasSignal = true;

		if (FAILED(hr))
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Can't load signal", mLogPrefix);
			#endif

			mHasSignal = false;
		}
		if (mVideoSignal.signalStatus.state != MWCAP_VIDEO_SIGNAL_LOCKED)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L2(mLogger, "[{}] Signal is not locked ({})", mLogPrefix,
				static_cast<int>(mVideoSignal.signalStatus.state));
			#endif

			mHasSignal = false;
		}
		if (mVideoSignal.inputStatus.hdmiStatus.byBitDepth == 0)
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Reported bit depth is 0", mLogPrefix);
			#endif

			mHasSignal = false;
		}

		VIDEO_FORMAT newVideoFormat;
		LoadFormat(&newVideoFormat, &mVideoSignal, &mUsbCaptureFormats);

		#ifndef NO_QUILL
		LogHdrMetaIfPresent(&newVideoFormat);
		#endif

		// TODO compare to old format
		if (ShouldChangeMediaType(&newVideoFormat))
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] VideoFormat changed! Attempting to reconnect", mLogPrefix);
			#endif

			CMediaType proposedMediaType(m_mt);
			VideoFormatToMediaType(&proposedMediaType, &newVideoFormat);

			hr = DoChangeMediaType(&proposedMediaType, &newVideoFormat);

			mFilter->OnVideoSignalLoaded(&mVideoSignal);

			if (FAILED(hr))
			{
				#ifndef NO_QUILL
				LOG_ERROR(mLogger, "[{}] VideoFormat changed but not able to reconnect! retry after backoff [Result: {:#08x}]",
					mLogPrefix, hr);
				#endif

				// TODO show OSD to say we need to change
				BACKOFF;
				continue;
			}

			mFilter->OnVideoFormatLoaded(&mVideoFormat);
		}

		if (hadSignal && !mHasSignal)
		{
			mFilter->OnVideoSignalLoaded(&mVideoSignal);
		}

		// grab next frame 
		DWORD dwRet = WaitForSingleObject(mNotifyEvent, 1000);

		// unknown, try again
		if (dwRet == WAIT_FAILED)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Wait for frame failed, retrying", mLogPrefix);
			#endif
			continue;
		}

		if (dwRet == WAIT_OBJECT_0)
		{
			if (proDevice)
			{
				// wait til we see a BUFFERING notification
				mLastMwResult = MWGetNotifyStatus(hChannel, mNotify, &mStatusBits);
				if (mLastMwResult != MW_SUCCEEDED) {
					#ifndef NO_QUILL
					LOG_TRACE_L1(mLogger, "[{}] MWGetNotifyStatus failed {}", mLogPrefix, static_cast<int>(mLastMwResult));
					#endif

					BACKOFF;
					continue;
				}

				if (mStatusBits & MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(mLogger, "[{}] Video signal change, retry after backoff", mLogPrefix);
					#endif

					BACKOFF;
					continue;
				}
				if (mStatusBits & MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(mLogger, "[{}] Video input source change, retry after backoff", mLogPrefix);
					#endif

					BACKOFF;
					continue;
				}

				if (mStatusBits & MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING)
				{
					hasFrame = true;
				}

				if (!mHasSignal)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(mLogger, "[{}] No signal will be displayed ", mLogPrefix);
					#endif

					hasFrame = true;
				}
			}
			else
			{
				// new frame is the only type of notification
				hasFrame = true;
			}

			if (hasFrame)
			{
				retVal = MagewellCapturePin::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
				if (!SUCCEEDED(retVal))
				{
					hasFrame = false;
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Video frame buffered but unable to get delivery buffer, retry after backoff", mLogPrefix);
					#endif
				}
			}

			if (!hasFrame) SHORT_BACKOFF;
		}
		else
		{
			if (!mHasSignal && dwRet == STATUS_TIMEOUT)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(mLogger, "[{}] Timeout and no signal, get delivery buffer for no signal image", mLogPrefix);
				#endif
				retVal = MagewellCapturePin::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
				if (!SUCCEEDED(retVal))
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Unable to get delivery buffer, retry after backoff", mLogPrefix);
					#endif
					SHORT_BACKOFF;
				}
				else
				{
					hasFrame = true;
				}
			}
			else
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(mLogger, "[{}] Wait for frame unexpected response ({:#08x})", mLogPrefix, dwRet);
				#endif
			}
		}
	}
	return retVal;
}

HRESULT MagewellVideoCapturePin::FillBuffer(IMediaSample* pms)
{
	VideoFrameGrabber vfg(this, mFilter->GetChannelHandle(), mFilter->GetDeviceType(), pms);
	auto retVal = vfg.grab();
	if (S_FALSE == HandleStreamStateChange(pms))
	{
		retVal = S_FALSE;
	}
	return retVal;
}

HRESULT MagewellVideoCapturePin::GetMediaType(CMediaType* pmt)
{
	VideoFormatToMediaType(pmt, &mVideoFormat);
	return NOERROR;
}

HRESULT MagewellVideoCapturePin::OnThreadCreate()
{
	#ifndef NO_QUILL
	CustomFrontend::preallocate();

	LOG_INFO(mLogger, "[{}] MagewellVideoCapturePin::OnThreadCreate", mLogPrefix);
	#endif

	auto hChannel = mFilter->GetChannelHandle();
	LoadSignal(&hChannel);

	mFilter->OnVideoSignalLoaded(&mVideoSignal);

	auto deviceType = mFilter->GetDeviceType();
	if (deviceType == PRO)
	{
		// start capture
		mCaptureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		mLastMwResult = MWStartVideoCapture(hChannel, mCaptureEvent);
		#ifndef NO_QUILL
		if (mLastMwResult != MW_SUCCEEDED)
		{
			LOG_ERROR(mLogger, "[{}] Unable to MWStartVideoCapture", mLogPrefix);
		} else
		{
			LOG_INFO(mLogger, "[{}] MWStartVideoCapture started", mLogPrefix);
		}
		#endif

		// register for signal change events & video buffering 
		mNotify = MWRegisterNotify(hChannel, mNotifyEvent,
			MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE | 
			MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING |
			MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE);
		if (!mNotify)
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogger, "[{}] Unable to MWRegistryNotify", mLogPrefix);
			#endif

			// TODO throw
		}
	}
	else if (deviceType == USB)
	{
		delete mVideoCapture;
		mVideoCapture = new VideoCapture(this, mFilter->GetChannelHandle());
	}
	return NOERROR;
}

void MagewellVideoCapturePin::StopCapture()
{
	auto deviceType = mFilter->GetDeviceType();
	if (deviceType == PRO)
	{
		MWStopVideoCapture(mFilter->GetChannelHandle());
	}
	else if (deviceType == USB)
	{
		delete mVideoCapture;
	}
}

bool MagewellVideoCapturePin::ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cbBuffer = mVideoFormat.imageSize;
	if (pProperties->cBuffers < 1)
	{
		// 1 works for mpc-vr, 16 works for madVR so go with that as a default if the input pin doesn't suggest a number.
		pProperties->cBuffers = 16;
		return false;
	}
	return true;
}

STDMETHODIMP MagewellVideoCapturePin::GetNumberOfCapabilities(int* piCount, int* piSize)
{
	*piCount = 1;
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

STDMETHODIMP MagewellVideoCapturePin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC)
{
	if (iIndex > 0)
	{
		return S_FALSE;
	}
	if (iIndex < 0)
	{
		return E_INVALIDARG;
	}
	CMediaType cmt;
	GetMediaType(&cmt);
	*pmt = CreateMediaType(&cmt);

	auto pvi = reinterpret_cast<VIDEOINFOHEADER2*>((*pmt)->pbFormat);

	auto pvscc = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(pSCC);

	pvscc->guid = FORMAT_VideoInfo2;
	pvscc->VideoStandard = AnalogVideo_PAL_D;
	pvscc->InputSize.cx = pvi->bmiHeader.biWidth;
	pvscc->InputSize.cy = pvi->bmiHeader.biHeight;
	pvscc->MinCroppingSize.cx = 80;
	pvscc->MinCroppingSize.cy = 60;
	pvscc->MaxCroppingSize.cx = pvi->bmiHeader.biWidth;
	pvscc->MaxCroppingSize.cy = pvi->bmiHeader.biHeight;
	pvscc->CropGranularityX = 80;
	pvscc->CropGranularityY = 60;
	pvscc->CropAlignX = 0;
	pvscc->CropAlignY = 0;

	pvscc->MinOutputSize.cx = 80;
	pvscc->MinOutputSize.cy = 60;
	pvscc->MaxOutputSize.cx = pvi->bmiHeader.biWidth;
	pvscc->MaxOutputSize.cy = pvi->bmiHeader.biHeight;
	pvscc->OutputGranularityX = 0;
	pvscc->OutputGranularityY = 0;
	pvscc->StretchTapsX = 0;
	pvscc->StretchTapsY = 0;
	pvscc->ShrinkTapsX = 0;
	pvscc->ShrinkTapsY = 0;
	pvscc->MinFrameInterval = pvi->AvgTimePerFrame;
	pvscc->MaxFrameInterval = pvi->AvgTimePerFrame;
	pvscc->MinBitsPerSecond = pvi->dwBitRate;
	pvscc->MaxBitsPerSecond = pvi->dwBitRate;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//  MagewellAudioCapturePin::AudioCapture
//////////////////////////////////////////////////////////////////////////
MagewellAudioCapturePin::AudioCapture::AudioCapture(MagewellAudioCapturePin* pin, HCHANNEL hChannel) :
	pin(pin)
{
	mEvent = MWCreateAudioCapture(hChannel, MWCAP_AUDIO_CAPTURE_NODE_EMBEDDED_CAPTURE, pin->mAudioFormat.fs,
		pin->mAudioFormat.bitDepth, pin->mAudioFormat.inputChannelCount, CaptureFrame, pin);
	if (mEvent == nullptr)
	{
		#ifndef NO_QUILL
		LOG_ERROR(pin->mLogger, "[{}] MWCreateAudioCapture failed {} Hz {} bits {} channels", pin->mLogPrefix,
			pin->mAudioFormat.fs, pin->mAudioFormat.bitDepth, pin->mAudioFormat.inputChannelCount);
		#endif
	}
}

MagewellAudioCapturePin::AudioCapture::~AudioCapture()
{
	if (mEvent != nullptr)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L3(pin->mLogger, "[{}] AudioCapture", pin->mLogPrefix);
		#endif

		#ifndef NO_QUILL
		LOG_TRACE_L3(pin->mLogger, "[{}] Ready to MWDestoryAudioCapture", pin->mLogPrefix);
		#endif

		const auto hr = MWDestoryAudioCapture(mEvent);
		mEvent = nullptr;

		#ifndef NO_QUILL
		if (MW_SUCCEEDED == hr)
		{
			LOG_TRACE_L3(pin->mLogger, "[{}] MWDestoryAudioCapture complete", pin->mLogPrefix);
		}
		else
		{
			LOG_WARNING(pin->mLogger, "[{}] MWDestoryAudioCapture failed", pin->mLogPrefix);
		}
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////
// MagewellAudioCapturePin
//////////////////////////////////////////////////////////////////////////
MagewellAudioCapturePin::MagewellAudioCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, bool pPreview) :
	MagewellCapturePin(
		phr,
		pParent,
		pPreview ? "AudioPreview" : "AudioCapture",
		pPreview ? L"AudioPreview" : L"AudioCapture",
		pPreview ? "AudioPreview" : "AudioCapture"
	),
	mDataBurstBuffer(bitstreamBufferSize) // initialise to a reasonable default size that is not wastefully large but also is unlikely to need to be expanded very often
{
	mCapturedFrame.data = new BYTE[maxFrameLengthInBytes];
	mCapturedFrame.length = maxFrameLengthInBytes;

	mDataBurstBuffer.assign(bitstreamBufferSize, 0);
	DWORD dwInputCount = 0;
	auto hChannel = pParent->GetChannelHandle();
	mLastMwResult = MWGetAudioInputSourceArray(hChannel, nullptr, &dwInputCount);
	if (mLastMwResult != MW_SUCCEEDED)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] MWGetAudioInputSourceArray", mLogPrefix);
		#endif
	}

	if (dwInputCount == 0)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] No audio signal detected", mLogPrefix);
		#endif
	}
	else
	{
		auto hr = LoadSignal(&hChannel);
		mFilter->OnAudioSignalLoaded(&mAudioSignal);
		if (hr == S_OK)
		{
			LoadFormat(&mAudioFormat, &mAudioSignal);
			mFilter->OnAudioFormatLoaded(&mAudioFormat);
		}
		else
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogger, "[{}] Unable to load audio signal", mLogPrefix);
			#endif
		}
	}

	#ifndef NO_QUILL
	LOG_WARNING(mLogger, "[{}] Audio Status Fs: {} Bits: {} Channels: {} Codec: {}", mLogPrefix, mAudioFormat.fs,
		mAudioFormat.bitDepth, mAudioFormat.outputChannelCount, codecNames[mAudioFormat.codec]);
	#endif

	#if defined RECORD_ENCODED || defined RECORD_RAW
	time_t timeNow = time(nullptr);
	struct tm* tmLocal;
	tmLocal = localtime(&timeNow);

	#ifdef RECORD_ENCODED
	strcpy_s(mEncodedInFileName, std::filesystem::temp_directory_path().string().c_str());
	CHAR encodedInFileName[128];
	sprintf_s(encodedInFileName, "\\%s-%d-%02d-%02d-%02d-%02d-%02d.encin",
		pPreview ? "audio_prev" : "audio_cap", tmLocal->tm_year + 1900, tmLocal->tm_mon + 1, tmLocal->tm_mday, tmLocal->tm_hour, tmLocal->tm_min, tmLocal->tm_sec);
	strcat_s(mEncodedInFileName, encodedInFileName);

	if (fopen_s(&mEncodedInFile, mEncodedInFileName, "wb") != 0)
	{
		LOG_WARNING(mLogger, "[{}] Failed to open {}", mLogPrefix, mEncodedInFileName);
	}

	strcpy_s(mEncodedOutFileName, std::filesystem::temp_directory_path().string().c_str());
	CHAR encodedOutFileName[128];
	sprintf_s(encodedOutFileName, "\\%s-%d-%02d-%02d-%02d-%02d-%02d.encout",
		pPreview ? "audio_prev" : "audio_cap", tmLocal->tm_year + 1900, tmLocal->tm_mon + 1, tmLocal->tm_mday, tmLocal->tm_hour, tmLocal->tm_min, tmLocal->tm_sec);
	strcat_s(mEncodedOutFileName, encodedOutFileName);

	if (fopen_s(&mEncodedOutFile, mEncodedOutFileName, "wb") != 0)
	{
		LOG_WARNING(mLogger, "[{}] Failed to open {}", mLogPrefix, mEncodedOutFileName);
	}
	#endif

	#ifdef RECORD_RAW
	strcpy_s(mRawFileName, std::filesystem::temp_directory_path().string().c_str());
	CHAR rawFileName[128];
	sprintf_s(rawFileName, "\\%s-%d-%02d-%02d-%02d-%02d-%02d.raw",
		pPreview ? "audio_prev" : "audio_cap", tmLocal->tm_year + 1900, tmLocal->tm_mon + 1, tmLocal->tm_mday, tmLocal->tm_hour, tmLocal->tm_min, tmLocal->tm_sec);
	strcat_s(mRawFileName, rawFileName);

	if (fopen_s(&mRawFile, mRawFileName, "wb") != 0)
	{
		LOG_WARNING(mLogger, "[{}] Failed to open {}", mLogPrefix, mRawFileName);
	}
	#endif

	#endif
}

MagewellAudioCapturePin::~MagewellAudioCapturePin()
{
	#ifdef RECORD_ENCODED
	if (0 != fclose(mEncodedInFile))
	{
		LOG_WARNING(mLogger, "[{}] Failed to close {}", mLogPrefix, mEncodedInFileName);
	}
	if (0 != fclose(mEncodedOutFile))
	{
		LOG_WARNING(mLogger, "[{}] Failed to close {}", mLogPrefix, mEncodedOutFileName);
	}
	#endif

	#ifdef RECORD_RAW
	if (0 != fclose(mRawFile))
	{
		LOG_WARNING(mLogger, "[{}] Failed to close {}", mLogPrefix, mRawFileName);
	}
	#endif
	delete mAudioCapture;
}

HRESULT MagewellAudioCapturePin::DecideAllocator(IMemInputPin* pPin, IMemAllocator** ppAlloc)
{
	// copied from CBaseOutputPin but preferring to use our own allocator first

	HRESULT hr = NOERROR;
	*ppAlloc = nullptr;

	ALLOCATOR_PROPERTIES prop;
	ZeroMemory(&prop, sizeof(prop));

	pPin->GetAllocatorRequirements(&prop);
	if (prop.cbAlign == 0)
	{
		prop.cbAlign = 1;
	}

	/* Try the allocator provided by the output pin. */
	hr = InitAllocator(ppAlloc);
	if (SUCCEEDED(hr))
	{
		hr = DecideBufferSize(*ppAlloc, &prop);
		if (SUCCEEDED(hr))
		{
			hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
			if (SUCCEEDED(hr))
			{
				return NOERROR;
			}
		}
	}

	if (*ppAlloc)
	{
		(*ppAlloc)->Release();
		*ppAlloc = nullptr;
	}

	/* Try the allocator provided by the input pin */
	hr = pPin->GetAllocator(ppAlloc);
	if (SUCCEEDED(hr))
	{
		hr = DecideBufferSize(*ppAlloc, &prop);
		if (SUCCEEDED(hr))
		{
			hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
			if (SUCCEEDED(hr))
			{
				return NOERROR;
			}
		}
	}

	if (*ppAlloc)
	{
		(*ppAlloc)->Release();
		*ppAlloc = nullptr;
	}

	return hr;
}

void MagewellAudioCapturePin::LoadFormat(AUDIO_FORMAT* audioFormat, const AUDIO_SIGNAL* audioSignal) const
{
	auto audioIn = *audioSignal;
	auto currentChannelAlloc = audioFormat->channelAllocation;
	auto currentChannelMask = audioFormat->channelValidityMask;
	if (mFilter->GetDeviceType() == USB)
	{
		audioFormat->fs = 48000;
	}
	else
	{
		audioFormat->fs = audioIn.signalStatus.dwSampleRate;
	}
	audioFormat->bitDepth = audioIn.signalStatus.cBitsPerSample;
	audioFormat->bitDepthInBytes = audioFormat->bitDepth / 8;
	audioFormat->codec = audioIn.signalStatus.bLPCM ? PCM : BITSTREAM;
	audioFormat->sampleInterval = 10000000.0 / audioFormat->fs;
	audioFormat->channelAllocation = audioIn.audioInfo.byChannelAllocation;
	audioFormat->channelValidityMask = audioIn.signalStatus.wChannelValid;

	if (audioFormat->channelAllocation == currentChannelAlloc && audioFormat->channelValidityMask == currentChannelMask)
	{
		// no change, leave untouched [inputChannelCount, outputChannelCount, channelMask, channelOffsets]
	}
	else
	{
		// https://ia903006.us.archive.org/11/items/CEA-861-E/CEA-861-E.pdf 
		if (audioIn.signalStatus.wChannelValid & (0x01 << 0))
		{
			if (audioIn.signalStatus.wChannelValid & (0x01 << 1))
			{
				if (audioIn.signalStatus.wChannelValid & (0x01 << 2))
				{
					if (audioIn.signalStatus.wChannelValid & (0x01 << 3))
					{
						audioFormat->inputChannelCount = 8;
						audioFormat->outputChannelCount = 8;
						audioFormat->channelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
						audioFormat->channelOffsets.fill(0);
						// swap LFE and FC
						audioFormat->channelOffsets[2] = 1;
						audioFormat->channelOffsets[3] = -1;
						audioFormat->lfeChannelIndex = 2;
						audioFormat->channelLayout = "FL FR FC LFE BL BR SL SR";
					}
					else
					{
						audioFormat->inputChannelCount = 6;
						audioFormat->outputChannelCount = 6;
						audioFormat->channelMask = KSAUDIO_SPEAKER_5POINT1;
						audioFormat->channelOffsets.fill(0);
						audioFormat->channelOffsets[2] = 1;
						audioFormat->channelOffsets[3] = -1;
						audioFormat->channelOffsets[6] = not_present;
						audioFormat->channelOffsets[7] = not_present;
						audioFormat->lfeChannelIndex = 2;
						audioFormat->channelLayout = "FL FR FC LFE BL BR";
					}
				}
				else
				{
					audioFormat->inputChannelCount = 4;
					audioFormat->outputChannelCount = 4;
					audioFormat->channelMask = KSAUDIO_SPEAKER_3POINT1;
					audioFormat->channelOffsets.fill(not_present);
					audioFormat->channelOffsets[0] = 0;
					audioFormat->channelOffsets[1] = 0;
					audioFormat->channelOffsets[2] = 1;
					audioFormat->channelOffsets[3] = -1;
					audioFormat->lfeChannelIndex = 2;
					audioFormat->channelLayout = "FL FR FC LFE";
				}
			}
			else
			{
				audioFormat->inputChannelCount = 2;
				audioFormat->outputChannelCount = 2;
				audioFormat->channelMask = KSAUDIO_SPEAKER_STEREO;
				audioFormat->channelOffsets.fill(not_present);
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->lfeChannelIndex = not_present;
				audioFormat->channelLayout = "FL FR";
			}

			// CEA-861-E Table 28
			switch (audioFormat->channelAllocation)
			{
			case 0x00:
				// FL FR
				audioFormat->channelLayout = "FL FR";
				break;
			case 0x01:
				// FL FR LFE --
				audioFormat->channelLayout = "FL FR LFE";
				audioFormat->channelMask = KSAUDIO_SPEAKER_2POINT1;
				audioFormat->inputChannelCount = 4;
				audioFormat->outputChannelCount = 3;
				audioFormat->channelOffsets.fill(not_present);
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x02:
				// FL FR -- FC
				audioFormat->channelLayout = "FL FR FC";
				audioFormat->channelMask = KSAUDIO_SPEAKER_3POINT0;
				audioFormat->inputChannelCount = 4;
				audioFormat->outputChannelCount = 3;
				audioFormat->channelOffsets.fill(not_present);
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x03:
				// FL FR LFE FC
				audioFormat->channelLayout = "FL FR FC LFE";
				audioFormat->channelMask = KSAUDIO_SPEAKER_3POINT1;
				audioFormat->inputChannelCount = 4;
				audioFormat->outputChannelCount = 4;
				audioFormat->channelOffsets.fill(not_present);
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1; // LFE -> FC
				audioFormat->channelOffsets[3] = -1; // FC -> LFE
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x04:
				// FL FR -- -- RC --
				audioFormat->channelLayout = "FL FR RC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 3;
				audioFormat->channelOffsets.fill(not_present);
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x05:
				// FL FR LFE -- RC --
				audioFormat->channelLayout = "FL FR LFE RC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_BACK_CENTER;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 4;
				audioFormat->channelOffsets.fill(not_present);
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x06:
				// FL FR -- FC RC --
				audioFormat->channelLayout = "FL FR FC RC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_CENTER;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 4;
				audioFormat->channelOffsets.fill(not_present);
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x07:
				// FL FR LFE FC RC --
				audioFormat->channelLayout = "FL FR LFE FC RC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x08:
				// FL FR -- -- RL RR
				audioFormat->channelLayout = "FL FR RL RR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT |
					SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 4;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				break;
			case 0x09:
				// FL FR LFE -- RL RR
				audioFormat->channelLayout = "FL FR LFE RL RR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x0A:
				// FL FR -- FC RL RR
				audioFormat->channelLayout = "FL FR FC RL RR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x0B:
				// FL FR LFE FC BL BR
				audioFormat->channelLayout = "FL FR FC LFE BL BR";
				audioFormat->channelMask = KSAUDIO_SPEAKER_5POINT1;
				audioFormat->inputChannelCount = 6;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x0C:
				// FL FR -- -- RL RR RC --
				audioFormat->channelLayout = "FL FR BL BR BC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT |
					SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x0D:
				// FL FR LFE -- RL RR RC --
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER;
				audioFormat->channelLayout = "FL FR LFE BL BR BC";
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x0E:
				// FL FR -- FC RL RR RC --
				audioFormat->channelLayout = "FL FR FC BL BR BC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x0F:
				// FL FR LFE FC RL RR RC --
				audioFormat->channelLayout = "FL FR FC LFE BL BR BC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x10:
				// FL FR -- -- RL RR RLC RRC
				audioFormat->channelLayout = "FL FR BL BR SL SR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_SIDE_LEFT |
					SPEAKER_SIDE_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = 2;
				audioFormat->channelOffsets[6] = -2;
				audioFormat->channelOffsets[7] = -2;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x11:
				// FL FR LFE -- RL RR RLC RRC
				audioFormat->channelLayout = "FL FR LFE BL BR SL SR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = 2;
				audioFormat->channelOffsets[6] = -2;
				audioFormat->channelOffsets[7] = -2;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x12:
				// FL FR -- FC RL RR RLC RRC
				audioFormat->channelLayout = "FL FR FC BL BR SL SR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = 2;
				audioFormat->channelOffsets[6] = -2;
				audioFormat->channelOffsets[7] = -2;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x13:
				// FL FR LFE FC RL RR RLC RRC (RL = side, RLC = back)
				audioFormat->channelLayout = "FL FR FC LFE BL BR SL SR";
				audioFormat->channelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 8;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = 2;
				audioFormat->channelOffsets[6] = -2;
				audioFormat->channelOffsets[7] = -2;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x14:
				// FL FR -- -- -- -- FLC FRC
				audioFormat->channelLayout = "FL FR FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER |
					SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 4;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = not_present;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x15:
				// FL FR LFE -- -- -- FLC FRC
				audioFormat->channelLayout = "FL FR LFE FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = not_present;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x16:
				// FL FR -- FC -- -- FLC FRC
				audioFormat->channelLayout = "FL FR FC FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 5;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = not_present;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x17:
				// FL FR LFE FC -- -- FLC FRC
				audioFormat->channelLayout = "FL FR FC LFE FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = not_present;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x18:
				// FL FR -- -- RC -- FLC FRC
				audioFormat->channelLayout = "FL FR RC FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER |
					SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = -1;
				audioFormat->channelOffsets[7] = -1;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x19:
				// FL FR LFE -- RC -- FLC FRC
				audioFormat->channelLayout = "FL FR LFE RC FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_BACK_CENTER | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = -1;
				audioFormat->channelOffsets[7] = -1;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x1A:
				// FL FR -- FC RC -- FLC FRC
				audioFormat->channelLayout = "FL FR FC RC FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_CENTER | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = -1;
				audioFormat->channelOffsets[7] = -1;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x1B:
				// FL FR LFE FC RC -- FLC FRC
				audioFormat->channelLayout = "FL FR FC LFE RC FLC FRC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER | SPEAKER_FRONT_LEFT_OF_CENTER |
					SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 2;
				audioFormat->channelOffsets[5] = not_present;
				audioFormat->channelOffsets[6] = -1;
				audioFormat->channelOffsets[7] = -1;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x1C:
				// FL FR -- -- RL RR FLC FRC
				audioFormat->channelLayout = "FL FR BL BR FLC FLR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT |
					SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x1D:
				// FL FR LFE -- RL RR FLC FRC
				audioFormat->channelLayout = "FL FR LFE BL BR FLC FLR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER |
					SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x1E:
				// FL FR -- FC RL RR FLC FRC
				audioFormat->channelLayout = "FL FR FC BL BR FLC FLR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER |
					SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x1F:
				// FL FR LFE FC RL RR FLC FRC
				audioFormat->channelLayout = "FL FR LFE FC BL BR FLC FLR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER |
					SPEAKER_FRONT_RIGHT_OF_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 8;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x20:
				// FL FR -- FC RL RR FCH --
				audioFormat->channelLayout = "FL FR FC BL BR TFC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_FRONT_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x21:
				// FL FR LFE FC RL RR FCH --
				audioFormat->channelLayout = "FL FR FC LFE BL BR TFC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_FRONT_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x22:
				// FL FR -- FC RL RR -- TC
				audioFormat->channelLayout = "FL FR FC BL BR TC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x23:
				// FL FR LFE FC RL RR -- TC
				audioFormat->channelLayout = "FL FR FC LFE BL BR TC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x24:
				// FL FR -- -- RL RR FLH FRH
				audioFormat->channelLayout = "FL FR BL BR TFL TFR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT |
					SPEAKER_BACK_RIGHT | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x25:
				// FL FR LFE -- RL RR FLH FRH
				audioFormat->channelLayout = "FL FR LFE BL BR TFL TFR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_BACK_LEFT | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x26:
				// FL FR -- -- RL RR FLW FRW (WIDE not supported by Windows, discarded)
				audioFormat->channelLayout = "FL FR BL BR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT |
					SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 4;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x27:
				// FL FR LFE -- RL RR FLW FRW (WIDE not supported by Windows, discarded)
				audioFormat->channelLayout = "FL FR LFE BL BR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_BACK_LEFT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x28:
				// FL FR -- FC RL RR RC TC
				audioFormat->channelLayout = "FL FR FC BL BR BC TC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER | SPEAKER_TOP_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x29:
				// FL FR LFE FC RL RR RC TC
				audioFormat->channelLayout = "FL FR FC LFE BL BR BC TC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER |
					SPEAKER_TOP_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 8;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x2A:
				// FL FR -- FC RL RR RC FCH
				audioFormat->channelLayout = "FL FR FC BL BR BC TFC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER | SPEAKER_TOP_FRONT_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x2B:
				// FL FR LFE FC RL RR RC FCH
				audioFormat->channelLayout = "FL FR FC LFE BL BR BC TFC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_BACK_CENTER |
					SPEAKER_TOP_FRONT_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 8;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x2C:
				// FL FR -- FC RL RR FCH TC
				audioFormat->channelLayout = "FL FR FC BL BR TFC TC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_FRONT_CENTER | SPEAKER_TOP_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 0;
				audioFormat->channelOffsets[3] = not_present;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 1;
				audioFormat->channelOffsets[7] = -1;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x2D:
				// FL FR LFE FC RL RR FCH TC
				audioFormat->channelLayout = "FL FR FC LFE BL BR TFC TC";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_FRONT_CENTER |
					SPEAKER_TOP_CENTER;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 8;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 1;
				audioFormat->channelOffsets[7] = -1;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x2E:
				// FL FR -- FC RL RR FLH FRH
				audioFormat->channelLayout = "FL FR FC BL BR TFL TFR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 7;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x2F:
				// FL FR LFE FC RL RR FLH FRH
				audioFormat->channelLayout = "FL FR FC LFE BL BR TFL TFR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_TOP_FRONT_LEFT |
					SPEAKER_TOP_FRONT_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 8;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = 0;
				audioFormat->channelOffsets[7] = 0;
				audioFormat->lfeChannelIndex = 2;
				break;
			case 0x30:
				// FL FR -- FC RL RR FLW FRW (WIDE not supported by Windows, discarded)
				audioFormat->channelLayout = "FL FR FC BL BR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 5;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = not_present;
				audioFormat->channelOffsets[3] = 0;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = not_present;
				break;
			case 0x31:
				// FL FR LFE FC RL RR FLW FRW (WIDE not supported by Windows, discarded)
				audioFormat->channelLayout = "FL FR FC LFE BL BR";
				audioFormat->channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY |
					SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				audioFormat->inputChannelCount = 8;
				audioFormat->outputChannelCount = 6;
				audioFormat->channelOffsets[0] = 0;
				audioFormat->channelOffsets[1] = 0;
				audioFormat->channelOffsets[2] = 1;
				audioFormat->channelOffsets[3] = -1;
				audioFormat->channelOffsets[4] = 0;
				audioFormat->channelOffsets[5] = 0;
				audioFormat->channelOffsets[6] = not_present;
				audioFormat->channelOffsets[7] = not_present;
				audioFormat->lfeChannelIndex = 2;
				break;
			default:
				// ignore
				break;
			}

			// CEA-861-E Table 31
			audioFormat->lfeLevelAdjustment = audioIn.audioInfo.byLFEPlaybackLevel == 0x2 ? minus_10db : unity;
		}
		else
		{
			audioFormat->inputChannelCount = 0;
			audioFormat->outputChannelCount = 0;
			audioFormat->channelOffsets.fill(not_present);
			audioFormat->lfeChannelIndex = not_present;
		}
	}
}

void MagewellAudioCapturePin::AudioFormatToMediaType(CMediaType* pmt, AUDIO_FORMAT* audioFormat)
{
	// based on https://github.com/Nevcairiel/LAVFilters/blob/81c5676cb99d0acfb1457b8165a0becf5601cae3/decoder/LAVAudio/LAVAudio.cpp#L1186
	pmt->majortype = MEDIATYPE_Audio;
	pmt->formattype = FORMAT_WaveFormatEx;

	if (audioFormat->codec == PCM)
	{
		// LAVAudio compatible big endian PCM
		if (audioFormat->bitDepthInBytes == 3)
		{
			pmt->subtype = MEDIASUBTYPE_PCM_IN24;
		}
		else if (audioFormat->bitDepthInBytes == 4)
		{
			pmt->subtype = MEDIASUBTYPE_PCM_IN32;
		}
		else
		{
			pmt->subtype = MEDIASUBTYPE_PCM_SOWT;
		}

		WAVEFORMATEXTENSIBLE wfex;
		memset(&wfex, 0, sizeof(wfex));

		WAVEFORMATEX* wfe = &wfex.Format;
		wfe->wFormatTag = static_cast<WORD>(pmt->subtype.Data1);
		wfe->nChannels = audioFormat->outputChannelCount;
		wfe->nSamplesPerSec = audioFormat->fs;
		wfe->wBitsPerSample = audioFormat->bitDepth;
		wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample / 8;
		wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;

		if (audioFormat->outputChannelCount > 2 || wfe->wBitsPerSample > 16 || wfe->nSamplesPerSec > 48000)
		{
			wfex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			wfex.Format.cbSize = sizeof(wfex) - sizeof(wfex.Format);
			wfex.dwChannelMask = audioFormat->channelMask;
			wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;
			wfex.SubFormat = pmt->subtype;
		}
		pmt->SetSampleSize(wfe->wBitsPerSample * wfe->nChannels / 8);
		pmt->SetFormat(reinterpret_cast<BYTE*>(&wfex), sizeof(wfex.Format) + wfex.Format.cbSize);
	}
	else
	{
		// working assumption is that LAVAudio is downstream so use a format it supports
		// https://learn.microsoft.com/en-us/windows/win32/coreaudio/representing-formats-for-iec-61937-transmissions
		WAVEFORMATEXTENSIBLE_IEC61937 wf_iec61937;
		memset(&wf_iec61937, 0, sizeof(wf_iec61937));
		WAVEFORMATEXTENSIBLE* wf = &wf_iec61937.FormatExt;
		wf->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;

		switch (audioFormat->codec)
		{
		case AC3:
			pmt->subtype = MEDIASUBTYPE_DOLBY_AC3;
			wf->Format.nChannels = 2;						// 1 IEC 60958 Line.
			wf->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
			wf_iec61937.dwEncodedChannelCount = 6;
			wf->Format.nSamplesPerSec = 48000;
			break;
		case EAC3:
			pmt->subtype = MEDIASUBTYPE_DOLBY_DDPLUS;
			wf->Format.nChannels = 2;						// 1 IEC 60958 Line.
			wf->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
			wf_iec61937.dwEncodedChannelCount = 6;
			wf->Format.nSamplesPerSec = 192000;
			break;
		case DTS:
			pmt->subtype = MEDIASUBTYPE_DTS;
			wf->Format.nChannels = 2;						// 1 IEC 60958 Lines.
			wf->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
			wf_iec61937.dwEncodedChannelCount = 6;
			wf->Format.nSamplesPerSec = 48000;
			break;
		case DTSHD:
			pmt->subtype = MEDIASUBTYPE_DTS_HD;
			wf->Format.nChannels = 8;						// 4 IEC 60958 Lines.
			wf->dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
			wf_iec61937.dwEncodedChannelCount = 8;
			wf->Format.nSamplesPerSec = 192000;
			break;
		case TRUEHD:
			pmt->subtype = MEDIASUBTYPE_DOLBY_TRUEHD;
			wf->Format.nChannels = 8;						// 4 IEC 60958 Lines.
			wf->dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
			wf_iec61937.dwEncodedChannelCount = 8;
			wf->Format.nSamplesPerSec = 192000;
			break;
		case BITSTREAM:
		case PCM:
		case PAUSE_OR_NULL:
			// should never get here
			break;
		}
		wf_iec61937.dwEncodedSamplesPerSec = 48000;
		wf_iec61937.dwAverageBytesPerSec = 0;
		wf->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wf->Format.wBitsPerSample = 16;
		wf->Samples.wValidBitsPerSample = 16;
		wf->Format.nBlockAlign = wf->Format.wBitsPerSample / 8 * wf->Format.nChannels;
		wf->Format.nAvgBytesPerSec = wf->Format.nSamplesPerSec * wf->Format.nBlockAlign;
		wf->Format.cbSize = sizeof(wf_iec61937) - sizeof(wf->Format);

		pmt->SetSampleSize(wf->Format.nBlockAlign);
		pmt->SetFormat(reinterpret_cast<BYTE*>(&wf_iec61937), sizeof(wf_iec61937) + wf->Format.cbSize);
	}
}

HRESULT MagewellAudioCapturePin::LoadSignal(HCHANNEL* hChannel)
{
	mLastMwResult = MWGetAudioSignalStatus(*hChannel, &mAudioSignal.signalStatus);
	if (MW_SUCCEEDED != mLastMwResult)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] LoadSignal MWGetAudioSignalStatus", mLogPrefix);
		#endif
		return S_FALSE;
	}

	MWCAP_INPUT_SPECIFIC_STATUS status;
	mLastMwResult = MWGetInputSpecificStatus(*hChannel, &status);
	if (mLastMwResult == MW_SUCCEEDED)
	{
		DWORD tPdwValidFlag = 0;
		if (!status.bValid)
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogger, "[{}] MWGetInputSpecificStatus is invalid", mLogPrefix);
			#endif
		}
		else if (status.dwVideoInputType != MWCAP_VIDEO_INPUT_TYPE_HDMI) 
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogger, "[{}] Video input type is not HDMI {}", mLogPrefix, status.dwVideoInputType);
			#endif
		}
		else if (MW_SUCCEEDED != MWGetHDMIInfoFrameValidFlag(*hChannel, &tPdwValidFlag))
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Unable to detect HDMI InfoFrame", mLogPrefix);
			#endif
		}
		if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_AUDIO)
		{
			HDMI_INFOFRAME_PACKET pkt;
			MWGetHDMIInfoFramePacket(*hChannel, MWCAP_HDMI_INFOFRAME_ID_AUDIO, &pkt);
			mAudioSignal.audioInfo = pkt.audioInfoFramePayload;
		}
		else
		{
			mAudioSignal.audioInfo = {};
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] No HDMI Audio infoframe detected", mLogPrefix);
			#endif
			return S_FALSE;
		}
	}
	else
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] LoadSignal MWGetInputSpecificStatus", mLogPrefix);
		#endif
		return S_FALSE;
	}

	if (mAudioSignal.signalStatus.wChannelValid == 0)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] No valid audio channels detected {}", mLogPrefix,
			mAudioSignal.signalStatus.wChannelValid);
		#endif
		return S_NO_CHANNELS;
	}
	return S_OK;
}

bool MagewellAudioCapturePin::ShouldChangeMediaType(AUDIO_FORMAT* newAudioFormat)
{
	auto reconnect = false;
	if (mAudioFormat.inputChannelCount != newAudioFormat->inputChannelCount)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Input channel count change {} to {}", mLogPrefix, mAudioFormat.inputChannelCount,
			newAudioFormat->inputChannelCount);
		#endif
	}
	if (mAudioFormat.outputChannelCount != newAudioFormat->outputChannelCount)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Output channel count change {} to {}", mLogPrefix, mAudioFormat.outputChannelCount,
			newAudioFormat->outputChannelCount);
		#endif
	}
	if (mAudioFormat.bitDepthInBytes != newAudioFormat->bitDepthInBytes)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Bit depth change {} to {}", mLogPrefix, mAudioFormat.bitDepthInBytes,
			newAudioFormat->bitDepthInBytes);
		#endif
	}
	if (mAudioFormat.fs != newAudioFormat->fs)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Fs change {} to {}", mLogPrefix, mAudioFormat.fs, newAudioFormat->fs);
		#endif
	}
	if (mAudioFormat.codec != newAudioFormat->codec)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Codec change {} to {}", mLogPrefix, codecNames[mAudioFormat.codec], codecNames[newAudioFormat->codec]);
		#endif
	}
	if (mAudioFormat.channelAllocation != newAudioFormat->channelAllocation)
	{
		reconnect = true;
		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Channel allocation change {} to {}", mLogPrefix, mAudioFormat.channelAllocation,
			newAudioFormat->channelAllocation);
		#endif
	}
	if (mAudioFormat.codec != PCM && newAudioFormat->codec != PCM && mAudioFormat.dataBurstSize != newAudioFormat->dataBurstSize)
	{
		reconnect = true;
		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Bitstream databurst change {} to {}", mLogPrefix, mAudioFormat.dataBurstSize,
			newAudioFormat->dataBurstSize);
		#endif
	}
	return reconnect;
}

void MagewellAudioCapturePin::CaptureFrame(const BYTE* pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam)
{
	MagewellAudioCapturePin* pin = static_cast<MagewellAudioCapturePin*>(pParam);
	CAutoLock lck(&pin->mCaptureCritSec);
	memcpy(pin->mCapturedFrame.data, pbFrame, cbFrame);
	pin->mCapturedFrame.length = cbFrame;
	pin->mCapturedFrame.ts = u64TimeStamp;
	if (!SetEvent(pin->mNotifyEvent))
	{
		auto err = GetLastError();
		#ifndef NO_QUILL
		LOG_ERROR(pin->mLogger, "[{}] Failed to notify on frame {:#08x}", pin->mLogPrefix, err);
		#endif
	}
}

HRESULT MagewellAudioCapturePin::FillBuffer(IMediaSample* pms)
{
	auto retVal = S_OK;

	if (CheckStreamState(nullptr) == STREAM_DISCARDING)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogger, "[{}] Stream is discarding", mLogPrefix);
		#endif

		return S_FALSE;
	}

	BYTE* pmsData;
	pms->GetPointer(&pmsData);
	auto sampleSize = pms->GetSize();
	auto bytesCaptured = 0L;
	auto samplesCaptured = 0;

	if (mAudioFormat.codec != PCM)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L3(mLogger, "[{}] Sending {} {} bytes", mLogPrefix, mDataBurstPayloadSize, codecNames[mAudioFormat.codec]);
		#endif

		for (auto i = 0; i < mDataBurstPayloadSize; i++)
		{
			pmsData[i] = mDataBurstBuffer[i];
		}
		pms->SetActualDataLength(mDataBurstPayloadSize);
		samplesCaptured++;
		bytesCaptured = mDataBurstPayloadSize;
		mDataBurstPayloadSize = 0;
	}
	else
	{
		// channel order on input is L0-L3,R0-R3 which has to be remapped to L0,R0,L1,R1,L2,R2,L3,R3
		// each 4 byte sample is left zero padded if the incoming stream is a lower bit depth (which is typically the case for HDMI audio)
		// must also apply the channel offsets to ensure each input channel is offset as necessary to be written to the correct output channel index
		auto outputChannelIdxL = -1;
		auto outputChannelIdxR = -1;
		auto outputChannels = -1;
		auto mustRescaleLfe = mAudioFormat.lfeLevelAdjustment != unity; // NOLINT(clang-diagnostic-float-equal)

		#ifndef NO_QUILL
		if (mustRescaleLfe)
		{
			LOG_ERROR(mLogger, "[{}] ERROR! Rescale LFE not implemented!", mLogPrefix);
		}
		#endif

		for (auto pairIdx = 0; pairIdx < mAudioFormat.inputChannelCount / 2; ++pairIdx)
		{
			auto channelIdxL = pairIdx * 2;
			auto outputOffsetL = mAudioFormat.channelOffsets[channelIdxL];
			if (outputOffsetL != not_present) outputChannelIdxL = ++outputChannels;

			auto channelIdxR = channelIdxL + 1;
			auto outputOffsetR = mAudioFormat.channelOffsets[channelIdxR];
			if (outputOffsetR != not_present) outputChannelIdxR = ++outputChannels;

			if (outputOffsetL == not_present && outputOffsetR == not_present) continue;

			for (auto sampleIdx = 0; sampleIdx < MWCAP_AUDIO_SAMPLES_PER_FRAME; sampleIdx++)
			{
				auto inByteStartIdxL = sampleIdx * MWCAP_AUDIO_MAX_NUM_CHANNELS;    // scroll to the sample block
				inByteStartIdxL += pairIdx;										        // scroll to the left channel slot
				inByteStartIdxL *= maxBitDepthInBytes;									// convert from slot index to byte index

				auto inByteStartIdxR = sampleIdx * MWCAP_AUDIO_MAX_NUM_CHANNELS;    // scroll to the sample block
				inByteStartIdxR += pairIdx + MWCAP_AUDIO_MAX_NUM_CHANNELS / 2;	        // scroll to the left channel slot then jump to the matching right channel
				inByteStartIdxR *= maxBitDepthInBytes;									// convert from slot index to byte index

				auto outByteStartIdxL = sampleIdx * mAudioFormat.outputChannelCount;// scroll to the sample block
				outByteStartIdxL += (outputChannelIdxL + outputOffsetL);                // jump to the output channel slot
				outByteStartIdxL *= mAudioFormat.bitDepthInBytes;                       // convert from slot index to byte index

				auto outByteStartIdxR = sampleIdx * mAudioFormat.outputChannelCount;// scroll to the sample block
				outByteStartIdxR += (outputChannelIdxR + outputOffsetR);                // jump to the output channel slot
				outByteStartIdxR *= mAudioFormat.bitDepthInBytes;                       // convert from slot index to byte index

				if (mAudioFormat.lfeChannelIndex == channelIdxL && mustRescaleLfe)
				{
					// PCM in network (big endian) byte order hence have to shift rather than use memcpy
					//   convert to an int
					int sampleValueL = mFrameBuffer[inByteStartIdxL] << 24 | mFrameBuffer[inByteStartIdxL + 1] << 16 |
						mFrameBuffer[inByteStartIdxL + 2] << 8 | mFrameBuffer[inByteStartIdxL + 3];

					int sampleValueR = mFrameBuffer[inByteStartIdxR] << 24 | mFrameBuffer[inByteStartIdxR + 1] << 16 |
						mFrameBuffer[inByteStartIdxR + 2] << 8 | mFrameBuffer[inByteStartIdxR + 3];

					//   adjust gain to a double
					double scaledValueL = mAudioFormat.lfeLevelAdjustment * sampleValueL;
					double scaledValueR = mAudioFormat.lfeLevelAdjustment * sampleValueR;

					// TODO
					//   triangular dither back to 16 or 24 bit PCM

					//   convert back to bytes and write to the sample
				}
				else
				{
					// skip past any zero padding (if any)
					inByteStartIdxL += maxBitDepthInBytes - mAudioFormat.bitDepthInBytes;
					inByteStartIdxR += maxBitDepthInBytes - mAudioFormat.bitDepthInBytes;
					for (int k = 0; k < mAudioFormat.bitDepthInBytes; ++k)
					{
						if (outputOffsetL != not_present)
						{
							auto outIdx = outByteStartIdxL + k;
							bytesCaptured++;
							if (outIdx < sampleSize)
							{
								pmsData[outIdx] = mFrameBuffer[inByteStartIdxL + k];
							}
							else
							{
								#ifndef NO_QUILL
								LOG_ERROR(mLogger, "[{}] Skipping L byte {} when sample should only be {} bytes long", mLogPrefix, outIdx, sampleSize);
								#endif
							}
						}

						if (outputOffsetR != not_present)
						{
							auto outIdx = outByteStartIdxR + k;
							bytesCaptured++;
							if (outIdx < sampleSize)
							{
								pmsData[outIdx] = mFrameBuffer[inByteStartIdxR + k];
							}
							else
							{
								#ifndef NO_QUILL
								LOG_ERROR(mLogger, "[{}] Skipping R byte {} when sample should only be {} bytes long", mLogPrefix, outIdx, sampleSize);
								#endif

							}
						}
					}
				}
				if (pairIdx == 0) samplesCaptured++;
			}
		}

		#ifdef RECORD_ENCODED
		LOG_TRACE_L3(mLogger, "[{}] pcm_out,{},{}", mLogPrefix, mFrameCounter, bytesCaptured);
		fwrite(pmsData, bytesCaptured, 1, mEncodedOutFile); 
		#endif
	}

	auto lastEndTime = mFrameEndTime - mStreamStartTime;
	mFilter->GetReferenceTime(&mFrameEndTime);
	auto endTime = mFrameEndTime - mStreamStartTime;
	auto startTime = endTime - static_cast<long>(mAudioFormat.sampleInterval * MWCAP_AUDIO_SAMPLES_PER_FRAME);
	auto sincePrev = endTime - lastEndTime;

	#ifndef NO_QUILL
	if (bytesCaptured != sampleSize)
	{
		LOG_WARNING(mLogger, "[{}] Audio frame {} : samples {} time {} delta {} size {} bytes buf {} bytes (since {}? {})", mLogPrefix,
			mFrameCounter, samplesCaptured, endTime, sincePrev, bytesCaptured, sampleSize, codecNames[mAudioFormat.codec], mSinceCodecChange);
	}
	else
	{
		LOG_TRACE_L2(mLogger, "[{}] Audio frame {} : samples {} time {} delta {} size {} bytes buf {} bytes (since {}? {})", mLogPrefix,
			mFrameCounter, samplesCaptured, endTime, sincePrev, bytesCaptured, sampleSize, codecNames[mAudioFormat.codec], mSinceCodecChange);
	}
	#endif

	pms->SetTime(&startTime, &endTime);
	pms->SetSyncPoint(mAudioFormat.codec == PCM);
	pms->SetDiscontinuity(mSinceCodecChange < 2 && mAudioFormat.codec != PCM);
	if (mSendMediaType)
	{
		CMediaType cmt(m_mt);
		AM_MEDIA_TYPE* sendMediaType = CreateMediaType(&cmt);
		pms->SetMediaType(sendMediaType);
		DeleteMediaType(sendMediaType);
		mSendMediaType = FALSE;
	}
	if (S_FALSE == HandleStreamStateChange(pms))
	{
		retVal = S_FALSE;
	}
	return retVal;
}

HRESULT MagewellAudioCapturePin::GetMediaType(CMediaType* pmt)
{
	AudioFormatToMediaType(pmt, &mAudioFormat);
	return NOERROR;
}

HRESULT MagewellAudioCapturePin::OnThreadCreate()
{
	#ifndef NO_QUILL
	CustomFrontend::preallocate();

	LOG_INFO(mLogger, "[{}] MagewellAudioCapturePin::OnThreadCreate", mLogPrefix);
	#endif

	memset(mCompressedBuffer, 0, sizeof(mCompressedBuffer));

	auto hChannel = mFilter->GetChannelHandle();
	LoadSignal(&hChannel);
	mFilter->OnAudioSignalLoaded(&mAudioSignal);

	auto deviceType = mFilter->GetDeviceType();
	if (deviceType == PRO)
	{
		// start capture
		mLastMwResult = MWStartAudioCapture(hChannel);
		if (mLastMwResult != MW_SUCCEEDED)
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogger, "[{}] MagewellAudioCapturePin::OnThreadCreate Unable to MWStartAudioCapture", mLogPrefix);
			#endif
			// TODO throw
		}

		// register for signal change events & audio buffered
		mNotify = MWRegisterNotify(hChannel, mNotifyEvent,
			MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE | MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE |
			MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED);
		if (!mNotify)
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogger, "[{}] MagewellAudioCapturePin::OnThreadCreate Unable to MWRegistryNotify", mLogPrefix);
			#endif
			// TODO throw
		}
	}
	else if (deviceType == USB)
	{
		delete mAudioCapture;
		mAudioCapture = new AudioCapture(this, mFilter->GetChannelHandle());
	}
	return NOERROR;
}

STDMETHODIMP MagewellAudioCapturePin::GetNumberOfCapabilities(int* piCount, int* piSize)
{
	*piCount = 1;
	*piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);
	return S_OK;
}

STDMETHODIMP MagewellAudioCapturePin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC)
{
	if (iIndex > 0)
	{
		return S_FALSE;
	}
	if (iIndex < 0)
	{
		return E_INVALIDARG;
	}
	CMediaType cmt;
	GetMediaType(&cmt);
	*pmt = CreateMediaType(&cmt);

	auto pascc = reinterpret_cast<AUDIO_STREAM_CONFIG_CAPS*>(pSCC);
	pascc->guid = FORMAT_WaveFormatEx;
	pascc->MinimumChannels = mAudioFormat.outputChannelCount;
	pascc->MaximumChannels = mAudioFormat.outputChannelCount;
	pascc->ChannelsGranularity = 1;
	pascc->MinimumBitsPerSample = mAudioFormat.bitDepth;
	pascc->MaximumBitsPerSample = mAudioFormat.bitDepth;
	pascc->BitsPerSampleGranularity = 1;
	pascc->MinimumSampleFrequency = mAudioFormat.fs;
	pascc->MaximumSampleFrequency = mAudioFormat.fs;
	pascc->SampleFrequencyGranularity = 1;

	return S_OK;
}

HRESULT MagewellAudioCapturePin::DoChangeMediaType(const CMediaType* pmt, const AUDIO_FORMAT* newAudioFormat)
{
	#ifndef NO_QUILL
	LOG_WARNING(mLogger, "[{}] Proposing new audio format Fs: {} Bits: {} Channels: {} Codec: {}", mLogPrefix,
		newAudioFormat->fs, newAudioFormat->bitDepth, newAudioFormat->outputChannelCount, codecNames[newAudioFormat->codec]);
	#endif
	long newSize = MWCAP_AUDIO_SAMPLES_PER_FRAME * newAudioFormat->bitDepthInBytes * newAudioFormat->outputChannelCount;
	if (newAudioFormat->codec != PCM)
	{
		newSize = newAudioFormat->dataBurstSize;
	}
	long oldSize = MWCAP_AUDIO_SAMPLES_PER_FRAME * mAudioFormat.bitDepthInBytes * mAudioFormat.outputChannelCount;
	if (mAudioFormat.codec != PCM)
	{
		oldSize = mAudioFormat.dataBurstSize;
	}
	auto shouldRenegotiateOnQueryAccept = newSize != oldSize || mAudioFormat.codec != newAudioFormat->codec;
	auto retVal = RenegotiateMediaType(pmt, newSize, shouldRenegotiateOnQueryAccept);
	if (retVal == S_OK)
	{
		mAudioFormat = *newAudioFormat;
		if (mFilter->GetDeviceType() == USB)
		{
			delete mAudioCapture;
			mAudioCapture = new AudioCapture(this, mFilter->GetChannelHandle());
		}
	}
	return retVal;
}

void MagewellAudioCapturePin::StopCapture()
{
	auto deviceType = mFilter->GetDeviceType();
	if (deviceType == PRO)
	{
		MWStopAudioCapture(mFilter->GetChannelHandle());
	}
	else if (deviceType == USB)
	{
		delete mAudioCapture;
	}
}

bool MagewellAudioCapturePin::ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties)
{
	if (mAudioFormat.codec == PCM)
	{
		pProperties->cbBuffer = MWCAP_AUDIO_SAMPLES_PER_FRAME * mAudioFormat.bitDepthInBytes * mAudioFormat.outputChannelCount;
	}
	else
	{
		pProperties->cbBuffer = mDataBurstBuffer.size();
	}
	if (pProperties->cBuffers < 1)
	{
		pProperties->cBuffers = 16;
		return false;
	}
	return true;
}

// loops til we have a frame to process, dealing with any mediatype changes as we go and then grabs a buffer once it's time to go
HRESULT MagewellAudioCapturePin::GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime,
	REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	auto hChannel = mFilter->GetChannelHandle();
	auto proDevice = mFilter->GetDeviceType() == PRO;
	auto hasFrame = false;
	auto retVal = S_FALSE;
	// keep going til we have a frame to process
	while (!hasFrame)
	{
		auto frameCopied = false;
		if (CheckStreamState(nullptr) == STREAM_DISCARDING)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Stream is discarding", mLogPrefix);
			#endif

			mSinceCodecChange = 0;
			break;
		}

		if (mStreamStartTime == 0)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Stream has not started, retry after backoff", mLogPrefix);
			#endif

			mSinceCodecChange = 0;
			BACKOFF;
			continue;
		}

		auto sigLoaded = LoadSignal(&hChannel);

		if (S_OK != sigLoaded)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Unable to load signal, retry after backoff", mLogPrefix);
			#endif

			if (mSinceCodecChange > 0)
				mFilter->OnAudioSignalLoaded(&mAudioSignal);

			mSinceCodecChange = 0;
			BACKOFF;
			continue;
		}
		if (mAudioSignal.signalStatus.cBitsPerSample == 0)
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Reported bit depth is 0, retry after backoff", mLogPrefix);
			#endif

			if (mSinceCodecChange > 0)
				mFilter->OnAudioSignalLoaded(&mAudioSignal);

			mSinceCodecChange = 0;
			BACKOFF;
			continue;
		}
		if (mAudioSignal.audioInfo.byChannelAllocation > 0x31)
		{
			#ifndef NO_QUILL
			LOG_WARNING(mLogger, "[{}] Reported channel allocation is {}, retry after backoff", mLogPrefix, 
				mAudioSignal.audioInfo.byChannelAllocation);
			#endif

			if (mSinceCodecChange > 0)
				mFilter->OnAudioSignalLoaded(&mAudioSignal);

			mSinceCodecChange = 0;
			BACKOFF;
			continue;
		}

		AUDIO_FORMAT newAudioFormat(mAudioFormat);
		LoadFormat(&newAudioFormat, &mAudioSignal);

		if (newAudioFormat.outputChannelCount == 0)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] No output channels in signal, retry after backoff", mLogPrefix);
			#endif

			if (mSinceCodecChange > 0)
				mFilter->OnAudioSignalLoaded(&mAudioSignal);

			mSinceLast = 0;
			mSinceCodecChange = 0;

			BACKOFF;
			continue;
		}

		// grab next frame 
		DWORD dwRet = WaitForSingleObject(mNotifyEvent, 1000);

		// unknown, try again
		if (dwRet == WAIT_FAILED)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] Wait for frame failed, retrying", mLogPrefix);
			#endif
			continue;
		}

		if (dwRet == WAIT_OBJECT_0)
		{
			// TODO magewell SDK bug means audio is always reported as PCM, until fixed allow 6 frames of audio to pass through before declaring it definitely PCM
			// 12 frames is 7680 bytes of 2 channel audio & 30720 of 8 channel which should be more than enough to be sure
			mBitstreamDetectionWindowLength = std::lround(bitstreamDetectionWindowSecs / (static_cast<double>(MWCAP_AUDIO_SAMPLES_PER_FRAME) / newAudioFormat.fs));
			if (mDetectedCodec != PCM)
			{
				newAudioFormat.codec = mDetectedCodec;
			}

			if (proDevice)
			{
				mStatusBits = 0;
				mLastMwResult = MWGetNotifyStatus(hChannel, mNotify, &mStatusBits);
				if (mStatusBits & MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(mLogger, "[{}] Audio signal change, retry after backoff", mLogPrefix);
					#endif

					if (mSinceCodecChange > 0)
						mFilter->OnAudioSignalLoaded(&mAudioSignal);

					mSinceLast = 0;
					mSinceCodecChange = 0;

					BACKOFF;
					continue;
				}

				if (mStatusBits & MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L1(mLogger, "[{}] Audio input source change, retry after backoff", mLogPrefix);
					#endif

					if (mSinceCodecChange > 0)
						mFilter->OnAudioSignalLoaded(&mAudioSignal);

					mSinceLast = 0;
					mSinceCodecChange = 0;
					BACKOFF;
					continue;
				}

				if (mStatusBits & MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED)
				{
					mLastMwResult = MWCaptureAudioFrame(hChannel, &mAudioSignal.frameInfo);
					if (MW_SUCCEEDED == mLastMwResult)
					{
						#ifndef NO_QUILL
						LOG_TRACE_L3(mLogger, "[{}] Audio frame buffered and captured", mLogPrefix);
						#endif

						memcpy(mFrameBuffer, mAudioSignal.frameInfo.adwSamples, maxFrameLengthInBytes);
						frameCopied = true;
					}
					else
					{
						// NB: evidence suggests this is harmless but logging for clarity
						if (mDataBurstSize > 0)
						{
							#ifndef NO_QUILL
							LOG_WARNING(mLogger, "[{}] Audio frame buffered but capture failed ({}), possible packet corruption after {} bytes", mLogPrefix,
								static_cast<int>(mLastMwResult), mDataBurstRead);
							#endif
						}
						else
						{
							#ifndef NO_QUILL
							LOG_WARNING(mLogger, "[{}] Audio frame buffered but capture failed ({}), retrying", mLogPrefix, static_cast<int>(mLastMwResult));
							#endif
						}
						continue;
					}
				}
			}
			else
			{
				#ifndef NO_QUILL
				LOG_TRACE_L3(mLogger, "[{}] Audio frame buffered and captured", mLogPrefix);
				#endif

				CAutoLock lck(&mCaptureCritSec);
				memcpy(mFrameBuffer, mCapturedFrame.data, mCapturedFrame.length);
				frameCopied = true;
			}
		}

		if (frameCopied)
		{
			mFrameCounter++;
			#ifndef NO_QUILL
			LOG_TRACE_L2(mLogger, "[{}] Reading frame {}", mLogPrefix, mFrameCounter);
			#endif

			#ifdef RECORD_RAW
			#ifndef NO_QUILL
			LOG_TRACE_L3(mLogger, "[{}] raw,{},{}", mLogPrefix, mFrameCounter, maxFrameLengthInBytes);
			#endif
			fwrite(mFrameBuffer, maxFrameLengthInBytes, 1, mRawFile);
			#endif

			Codec* detectedCodec = &newAudioFormat.codec;
			const auto mightBeBitstream = newAudioFormat.fs >= 48000 && mSinceLast < mBitstreamDetectionWindowLength;
			const auto examineBitstream = newAudioFormat.codec != PCM || mightBeBitstream || mDataBurstSize > 0;
			if (examineBitstream)
			{
				#ifndef NO_QUILL
				if (!mProbeOnTimer && newAudioFormat.codec == PCM)
				{
					LOG_TRACE_L2(mLogger, "[{}] Bitstream probe in frame {} - {} {} Hz (since: {} len: {} burst: {})", mLogPrefix, mFrameCounter,
						codecNames[newAudioFormat.codec], newAudioFormat.fs, mSinceLast, mBitstreamDetectionWindowLength, mDataBurstSize);
				}
				#endif

				CopyToBitstreamBuffer(mFrameBuffer);

				uint16_t bufferSize = mAudioFormat.bitDepthInBytes * MWCAP_AUDIO_SAMPLES_PER_FRAME * mAudioFormat.inputChannelCount;
				auto res = ParseBitstreamBuffer(bufferSize, &detectedCodec);
				if (S_OK == res || S_PARTIAL_DATABURST == res)
				{
					#ifndef NO_QUILL
					LOG_TRACE_L2(mLogger, "[{}] Detected bitstream in frame {} {} (res: {:#08x})", mLogPrefix, mFrameCounter, codecNames[mDetectedCodec], res);
					#endif
					mProbeOnTimer = false;
					if (mDetectedCodec == *detectedCodec)
					{
						if (mDataBurstPayloadSize > 0) mSinceCodecChange++;
					}
					else
					{
						mSinceCodecChange = 0;
						mDetectedCodec = *detectedCodec;
					}
					mSinceLast = 0;
					if (mDataBurstPayloadSize > 0)
					{
						#ifndef NO_QUILL
						LOG_TRACE_L3(mLogger, "[{}] Bitstream databurst complete, collected {} bytes from {} frames", mLogPrefix, mDataBurstPayloadSize, ++mDataBurstFrameCount);
						#endif
						newAudioFormat.dataBurstSize = mDataBurstPayloadSize;
						mDataBurstFrameCount = 0;
					}
					else
					{
						if (S_PARTIAL_DATABURST == res) mDataBurstFrameCount++;
						continue;
					}
				}
				else
				{
					if (++mSinceLast < mBitstreamDetectionWindowLength)
					{
						// skip to the next frame if we're in the initial probe otherwise allow publication downstream to continue
						if (!mProbeOnTimer)
						{
							continue;
						}
					}
					else
					{
						#ifndef NO_QUILL
						if (mSinceLast == mBitstreamDetectionWindowLength)
						{
							LOG_TRACE_L1(mLogger, "[{}] Probe complete after {} frames, not bitstream (timer? {})", mLogPrefix, mSinceLast, mProbeOnTimer);
						}
						#endif
						mProbeOnTimer = false;
						mDetectedCodec = PCM;
						mBytesSincePaPb = 0;
					}
				}
			}
			else
			{
				mSinceLast++;
			}
			int probeTrigger = std::lround(mBitstreamDetectionWindowLength * bitstreamDetectionRetryAfter);
			if (mSinceLast >= probeTrigger)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(mLogger, "[{}] Triggering bitstream probe after {} frames", mLogPrefix, mSinceLast);
				#endif
				mProbeOnTimer = true;
				mSinceLast = 0;
				mBytesSincePaPb = 0;
			}

			// don't try to publish PAUSE_OR_NULL downstream
			if (mDetectedCodec == PAUSE_OR_NULL)
			{
				mSinceCodecChange = 0;
				continue;
			}

			newAudioFormat.codec = mDetectedCodec;

			// detect format changes
			if (ShouldChangeMediaType(&newAudioFormat))
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogger, "[{}] AudioFormat changed! Attempting to reconnect", mLogPrefix);
				#endif

				CMediaType proposedMediaType(m_mt);
				AudioFormatToMediaType(&proposedMediaType, &newAudioFormat);
				auto hr = DoChangeMediaType(&proposedMediaType, &newAudioFormat);
				if (FAILED(hr))
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] AudioFormat changed but not able to reconnect ({:#08x}) retry after backoff", mLogPrefix, hr);
					#endif

					// TODO communicate that we need to change somehow
					BACKOFF;
					continue;
				}

				mFilter->OnAudioSignalLoaded(&mAudioSignal);
				mFilter->OnAudioFormatLoaded(&mAudioFormat);
			}

			if (newAudioFormat.codec == PCM || mDataBurstPayloadSize > 0)
			{
				retVal = MagewellCapturePin::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
				if (SUCCEEDED(retVal))
				{
					hasFrame = true;
				}
				else
				{
					mSinceCodecChange = 0;
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Audio frame buffered but unable to get delivery buffer, retry after backoff", mLogPrefix);
					#endif
				}
			}
		}

		if (!hasFrame) SHORT_BACKOFF;
	}
	return retVal;
}

// copies the inbound byte stream into a format that can be probed
void MagewellAudioCapturePin::CopyToBitstreamBuffer(BYTE* buf)
{
	// copies from input to output skipping zero bytes and with a byte swap per sample
	auto bytesCopied = 0;
	for (auto pairIdx = 0; pairIdx < mAudioFormat.inputChannelCount / 2; ++pairIdx)
	{
		for (auto sampleIdx = 0; sampleIdx < MWCAP_AUDIO_SAMPLES_PER_FRAME; sampleIdx++)
		{
			int inStartL = (sampleIdx * MWCAP_AUDIO_MAX_NUM_CHANNELS + pairIdx) * maxBitDepthInBytes;
			int inStartR = (sampleIdx * MWCAP_AUDIO_MAX_NUM_CHANNELS + pairIdx + MWCAP_AUDIO_MAX_NUM_CHANNELS / 2) * maxBitDepthInBytes;
			int outStart = (sampleIdx * mAudioFormat.inputChannelCount + pairIdx * mAudioFormat.inputChannelCount) * mAudioFormat.bitDepthInBytes;
			for (int byteIdx = 0; byteIdx < mAudioFormat.bitDepthInBytes; ++byteIdx) {
				auto outL = outStart + byteIdx;
				auto outR = outStart + mAudioFormat.bitDepthInBytes + byteIdx;
				auto inL = inStartL + maxBitDepthInBytes - byteIdx - 1;
				auto inR = inStartR + maxBitDepthInBytes - byteIdx - 1;
				// byte swap because compressed audio is big endian 
				mCompressedBuffer[outL] = buf[inL];
				mCompressedBuffer[outR] = buf[inR];
				bytesCopied += 2;
			}
		}
	}
	#ifdef RECORD_ENCODED
	LOG_TRACE_L3(mLogger, "[{}] encoder_in,{},{}", mLogPrefix, mFrameCounter, bytesCopied);
	fwrite(mCompressedBuffer, bytesCopied, 1, mEncodedInFile);
	#endif
}

// probes a non PCM buffer for the codec based on format of the IEC 61937 dataframes and/or copies the content to the data burst burffer
HRESULT MagewellAudioCapturePin::ParseBitstreamBuffer(uint16_t bufSize, enum Codec** codec)
{
	uint16_t bytesRead = 0;
	bool copiedBytes = false;
	bool partialDataBurst = false;
	bool maybeBitstream = false;

	#ifndef NO_QUILL
	bool foundPause = **codec == PAUSE_OR_NULL;
	#endif

	while (bytesRead < bufSize)
	{
		uint16_t remainingInBurst = std::max(mDataBurstSize - mDataBurstRead, 0);
		if (remainingInBurst > 0)
		{
			uint16_t remainingInBuffer = bufSize - bytesRead;
			auto toCopy = std::min(remainingInBurst, remainingInBuffer);
			#ifndef NO_QUILL
			LOG_TRACE_L3(mLogger, "[{}] Copying {} bytes of databurst from {}-{} to {}-{}", mLogPrefix, toCopy,
				bytesRead, bytesRead + toCopy - 1, mDataBurstRead, mDataBurstRead + toCopy - 1);
			#endif
			for (auto i = 0; i < toCopy; i++)
			{
				mDataBurstBuffer[mDataBurstRead + i] = mCompressedBuffer[bytesRead + i];
			}
			bytesRead += toCopy;
			mDataBurstRead += toCopy;
			remainingInBurst -= toCopy;
			mBytesSincePaPb += toCopy;
			copiedBytes = true;

			if (remainingInBurst == 0)
			{
				mDataBurstPayloadSize = mDataBurstSize;
				#ifdef RECORD_ENCODED
				LOG_TRACE_L3(mLogger, "[{}] encoder_out,{},{}", mLogPrefix, mFrameCounter, mDataBurstSize);
				fwrite(mDataBurstBuffer.data(), mDataBurstSize, 1, mEncodedOutFile);
				#endif
			}
		}
		// more to read = will need another frame
		if (remainingInBurst > 0)
		{
			partialDataBurst = true;
			continue;
		}

		// no more to read so reset the databurst state ready for the next frame
		mDataBurstSize = mDataBurstRead = 0;

		// burst complete so search the frame for the PaPb preamble F8 72 4E 1F (248 114 78 31)
		for (; (bytesRead < bufSize) && mPaPbBytesRead != 4; ++bytesRead, ++mBytesSincePaPb)
		{
			if (mCompressedBuffer[bytesRead] == 0xf8 && mPaPbBytesRead == 0
				|| mCompressedBuffer[bytesRead] == 0x72 && mPaPbBytesRead == 1
				|| mCompressedBuffer[bytesRead] == 0x4e && mPaPbBytesRead == 2
				|| mCompressedBuffer[bytesRead] == 0x1f && mPaPbBytesRead == 3)
			{
				if (++mPaPbBytesRead == 4)
				{
					mDataBurstSize = mDataBurstRead = 0;
					bytesRead++;
					#ifndef NO_QUILL
					if (!foundPause)
						LOG_TRACE_L2(mLogger, "[{}] Found PaPb at position {}-{} ({} since last)", mLogPrefix, bytesRead - 4, bytesRead, mBytesSincePaPb);
					#endif
					mBytesSincePaPb = 4;
					maybeBitstream = false;
					break;
				}
			}
			else
			{
				mPaPbBytesRead = 0;
			}
		}

		if (mPaPbBytesRead == 1 || mPaPbBytesRead == 2 || mPaPbBytesRead == 3)
		{
			#ifndef NO_QUILL
			if (!foundPause)
			{
				LOG_TRACE_L3(mLogger, "[{}] PaPb {} bytes found", mLogPrefix, mPaPbBytesRead);
			}
			#endif
			maybeBitstream = true;
			continue;
		}

		// grab PcPd preamble words
		uint8_t bytesToCopy = std::min(bufSize - bytesRead, 4 - mPcPdBytesRead);
		if (bytesToCopy > 0)
		{
			memcpy(mPcPdBuffer, mCompressedBuffer + bytesRead, bytesToCopy);
			mPcPdBytesRead += bytesToCopy;
			bytesRead += bytesToCopy;
			mBytesSincePaPb += bytesToCopy;
			copiedBytes = true;
		}

		if (mPcPdBytesRead != 4)
		{

			#ifndef NO_QUILL
			if (!foundPause && mPcPdBytesRead != 0)
				LOG_TRACE_L3(mLogger, "[{}] Found PcPd at position {} but only {} bytes available", mLogPrefix, bytesRead - bytesToCopy, bytesToCopy);
			#endif
			continue;
		}

		mDataBurstSize = ((static_cast<uint16_t>(mPcPdBuffer[2]) << 8) + static_cast<uint16_t>(mPcPdBuffer[3]));
		auto dt = static_cast<uint8_t>(mPcPdBuffer[1] & 0x7f);
		GetCodecFromIEC61937Preamble(IEC61937DataType{ dt }, &mDataBurstSize, *codec);

		// ignore PAUSE_OR_NULL, start search again
		if (**codec == PAUSE_OR_NULL)
		{
			#ifndef NO_QUILL
			if (!foundPause)
			{
				foundPause = true;
				LOG_TRACE_L2(mLogger, "[{}] Found PAUSE_OR_NULL ({}) with burst size {}, start skipping", mLogPrefix, dt, mDataBurstSize);
			}
			#endif
			mPaPbBytesRead = mPcPdBytesRead = 0;
			mDataBurstSize = mDataBurstPayloadSize = mDataBurstRead = 0;
			continue;
		}

		#ifndef NO_QUILL
		if (foundPause)
		{
			LOG_TRACE_L2(mLogger, "[{}] Exiting PAUSE_OR_NULL skip mode", mLogPrefix);
			foundPause = false;
		}
		#endif

		if (mDataBurstBuffer.size() > mDataBurstSize)
		{
			mDataBurstBuffer.clear();
		}
		if (mDataBurstBuffer.size() < mDataBurstSize)
		{
			mDataBurstBuffer.resize(mDataBurstSize);
		}

		mPaPbBytesRead = mPcPdBytesRead = 0;
		#ifndef NO_QUILL
		LOG_TRACE_L2(mLogger, "[{}] Found codec {} with burst size {}", mLogPrefix, codecNames[static_cast<int>(**codec)], mDataBurstSize);
		#endif
	}
	return partialDataBurst ? S_PARTIAL_DATABURST : maybeBitstream ? S_POSSIBLE_BITSTREAM : copiedBytes ? S_OK : S_FALSE;
}

// identifies codecs that are known/expected to be carried via HDMI in an AV setup
// from IEC 61937-2 Table 2
HRESULT MagewellAudioCapturePin::GetCodecFromIEC61937Preamble(const IEC61937DataType dataType, uint16_t* burstSize, Codec* codec)
{
	switch (dataType & 0xff)
	{
	case IEC61937_AC3:
		*burstSize /= 8; // bits
		*codec = AC3;
		break;
	case IEC61937_DTS1:
	case IEC61937_DTS2:
	case IEC61937_DTS3:
		*burstSize /= 8; // bits
		*codec = DTS;
		break;
	case IEC61937_DTSHD:
		*codec = DTSHD;
		break;
	case IEC61937_EAC3:
		*codec = EAC3;
		break;
	case IEC61937_TRUEHD:
		*codec = TRUEHD;
		break;
	case IEC61937_NULL:
	case IEC61937_PAUSE:
		*codec = PAUSE_OR_NULL;
		break;
	default:
		*codec = PAUSE_OR_NULL;
		#ifndef NO_QUILL
		LOG_WARNING(mLogger, "[{}] Unknown IEC61937 datatype {} will be treated as PAUSE", mLogPrefix, dataType & 0xff);
		#endif
		break;
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// MemAllocator 
//////////////////////////////////////////////////////////////////////////
MemAllocator::MemAllocator(LPUNKNOWN pUnk, HRESULT* pHr) : CMemAllocator("MemAllocator", pUnk, pHr)
{
	// exists purely to allow for easy debugging of what is going on inside CMemAllocator
}

HRESULT MagewellAudioCapturePin::InitAllocator(IMemAllocator** ppAllocator)
{
	HRESULT hr = S_OK;
	auto pAlloc = new MemAllocator(nullptr, &hr);
	if (!pAlloc)
	{
		return E_OUTOFMEMORY;
	}

	if (FAILED(hr))
	{
		delete pAlloc;
		return hr;
	}

	return pAlloc->QueryInterface(IID_IMemAllocator, reinterpret_cast<void**>(ppAllocator));
}
