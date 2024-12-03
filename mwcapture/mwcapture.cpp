/*
 *      Copyright (C) 2024 Matt Khan
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

#ifndef NO_QUILL
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"
#include <string_view>
#endif // !NO_QUILL

#include <windows.h>
#include <process.h>
#include <DXVA.h>
#include <streams.h>
#include <filesystem>
// linking side data GUIDs fails without this
#include <initguid.h>

#include "lavfilters_side_data.h"
#include "mwcapture.h"

#define MAX_BIT_DEPTH_IN_BYTE (sizeof(DWORD))
#define BACKOFF Sleep(20)

constexpr auto chromaticity_scale_factor = 0.00002;
constexpr auto high_luminance_scale_factor = 1.0;
constexpr auto low_luminance_scale_factor = 0.0001;

constexpr AMOVIESETUP_MEDIATYPE sVideoPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

constexpr AMOVIESETUP_MEDIATYPE sAudioPinTypes =
{
    &MEDIATYPE_Audio,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

constexpr AMOVIESETUP_PIN sVideoPin = {
        const_cast<LPWSTR>(L"Video"),     
        FALSE,          // Is it rendered
        TRUE,           // Is it an output
        FALSE,          // Are we allowed none
        FALSE,          // And allowed many
        &CLSID_NULL,    // Connects to filter
        nullptr,        // Connects to pin
        1,              // Number of types
        &sVideoPinTypes // Pin information
};

constexpr AMOVIESETUP_PIN sAudioPin = {
        const_cast<LPWSTR>(L"Audio"), 
        FALSE,          // Is it rendered
        TRUE,           // Is it an output
        FALSE,          // Are we allowed none
        FALSE,          // And allowed many
        &CLSID_NULL,    // Connects to filter
        nullptr,        // Connects to pin
        1,              // Number of types
        &sAudioPinTypes // Pin information
};

const AMOVIESETUP_PIN sMIPPins[] = { sVideoPin, sAudioPin };

constexpr AMOVIESETUP_FILTER sMIPSetup =
{
    &CLSID_MWCAPTURE_FILTER,  // Filter CLSID
    L"MagewellCapture",       // String name
    MERIT_DO_NOT_USE,         // Filter merit
    2,                        // Number of pins
    sMIPPins                  // Pin information
};


// List of class IDs and creator functions for the class factory.
CFactoryTemplate g_Templates[] = {
    { L"MagewellCapture",
      &CLSID_MWCAPTURE_FILTER,
      MagewellCaptureFilter::CreateInstance,
      nullptr,
      &sMIPSetup
    }
};

int g_cTemplates = 1;


CUnknown* MagewellCaptureFilter::CreateInstance(LPUNKNOWN punk, HRESULT* phr)
{
    MagewellCaptureFilter* pNewObject = new MagewellCaptureFilter(punk, phr);

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
            quill::PatternFormatterOptions{ "%(time) [%(thread_id)] %(short_source_location:<28) "
            "LOG_%(log_level:<9) %(logger:<12) %(message)",
            "%H:%M:%S.%Qns",
            quill::Timezone::GmtTime
        });

	// printing absolutely everything we may ever log
    mLogger->set_log_level(quill::LogLevel::TraceL3);
#endif // !NO_QUILL

	// Initialise the device and validate that it presents some form of data
	mInited = MWCaptureInitInstance();
#ifndef NO_QUILL
    if (!mInited)
	{
        LOG_ERROR(mLogger, "Unable to init");
	}
#endif

	int nIndex = MWGetChannelCount();
	int nChannel = -1;
	for (int i = 0; i < nIndex; i++)
	{
		MWCAP_CHANNEL_INFO mci;
		MWGetChannelInfoByIndex(i, &mci);
		if (0 == strcmp(mci.szFamilyName, "Pro Capture"))
		{
			mValidChannel[mValidChannelCount] = i;
			mValidChannelCount++;
			if (nChannel == -1)
			{
				nChannel = i;
			}
		}
	}

#ifndef NO_QUILL
    if (nIndex <= 0 || mValidChannelCount <= 0)
	{
        LOG_ERROR(mLogger, "No valid channels to open on board:channel {}:{}", mBoardId, mChannelId);
        // TODO throw
	}
#endif

	// critical lock
	CAutoLock cAutoLock(&m_cStateLock);

	// open channel
	MWCAP_CHANNEL_INFO channelInfo = {0};
	WCHAR path[128] = {0};
	MWGetDevicePath(mValidChannel[nChannel], path);
	mChannel = MWOpenChannelByPath(path);
#ifndef NO_QUILL
    if (mChannel == nullptr)
	{
        LOG_ERROR(mLogger, "Unable to open board:channel {}:{}", mBoardId, mChannelId);
		// TODO throw
	}
#endif

	auto hr = MWGetChannelInfo(mChannel, &channelInfo);
#ifndef NO_QUILL
    if (MW_SUCCEEDED != hr)
	{
        LOG_ERROR(mLogger, "Can't get channel info from board:channel {}:{}", mBoardId, mChannelId);
		// TODO throw
	}
    else
    {
        LOG_INFO(mLogger, "Loaded channel info from board:channel {}:{}", mBoardId, mChannelId);
    }
#endif

	mClock = new MWReferenceClock(phr, mChannel);

	new MagewellCapturePin(phr, this, PIN_VIDEO_CAPTURE);
	new MagewellCapturePin(phr, this, PIN_VIDEO_PREVIEW);
	new MagewellCapturePin(phr, this, PIN_AUDIO_CAPTURE);
}

MagewellCaptureFilter::~MagewellCaptureFilter()
{
	if (mInited)
	{
        MWCaptureExitInstance();
    }
}

HRESULT MagewellCaptureFilter::GetTime(REFERENCE_TIME* pTime)
{
    return mClock->GetTime(pTime);
}

HRESULT MagewellCaptureFilter::AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HEVENT hEvent, DWORD_PTR* pdwAdviseCookie)
{
    return mClock->AdviseTime(baseTime, streamTime, hEvent, pdwAdviseCookie);
}

HRESULT MagewellCaptureFilter::AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HSEMAPHORE hSemaphore, DWORD_PTR* pdwAdviseCookie)
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
    return mChannel;
}

STDMETHODIMP MagewellCaptureFilter::GetState(DWORD dw, FILTER_STATE* pState)
{
    CBaseFilter::GetState(dw, pState);
    if (*pState == State_Paused)
    {
        return VFW_S_CANT_CUE;
    }
    else
    {
        return S_OK;
    }
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
    LONGLONG now;
    MWGetDeviceTime(mChannel, &now);

	#ifndef NO_QUILL
    LOG_INFO(mLogger, "Filter has started running at {}", now);
	#endif

    for (auto i = 0; i < m_iPins; i++)
    {
        auto stream = dynamic_cast<MagewellCapturePin*>(m_paStreams[i]);
        stream->SetStartTime(now);
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

MagewellCapturePin::VideoFrameGrabber::VideoFrameGrabber(MagewellCapturePin* pin, HCHANNEL channel, IMediaSample* pms):
	channel(channel), pin(pin), pms(pms)
{
	this->pms->GetPointer(&pmsData);

	#ifndef NO_QUILL
	LOG_TRACE_L3(pin->mLogger, "[{}] Pinning {} bytes", this->pin->mLogPrefix, this->pms->GetSize());
	#endif
	
	MWPinVideoBuffer(this->channel, pmsData, this->pms->GetSize());
}

MagewellCapturePin::VideoFrameGrabber::~VideoFrameGrabber()
{
	#ifndef NO_QUILL
    LOG_TRACE_L3(pin->mLogger, "[{}] Unpinning {} bytes, captured {} bytes", pin->mLogPrefix, pms->GetSize(), pms->GetActualDataLength());
	#endif
    MWUnpinVideoBuffer(channel, pmsData);
}

HRESULT MagewellCapturePin::VideoFrameGrabber::grab()
{
    auto hasFrame = false;
    auto retVal = S_OK;
    while (!hasFrame)
    {
        if (pin->CheckStreamState(nullptr) == STREAM_DISCARDING)
        {
#ifndef NO_QUILL
            LOG_TRACE_L1(pin->mLogger, "[{}] Stream is discarding", pin->mLogPrefix);
#endif
            break;
        }

        auto hr = pin->LoadVideoSignal(&channel);
        if (FAILED(hr))
        {
#ifndef NO_QUILL
            LOG_WARNING(pin->mLogger, "[{}] Can't load signal, sleeping", pin->mLogPrefix);
#endif
        	BACKOFF;
            continue;
        }
        if (pin->mVideoSignal.signalStatus.state != MWCAP_VIDEO_SIGNAL_LOCKED)
        {
#ifndef NO_QUILL
            LOG_TRACE_L3(pin->mLogger, "[{}] No signal {}, sleeping", pin->mLogPrefix, static_cast<int>(pin->mVideoSignal.signalStatus.state));
#endif
        	BACKOFF;
            continue;
        }

        if (pin->ShouldChangeVideoMediaType())
        {
#ifndef NO_QUILL
            LOG_WARNING(pin->mLogger, "[{}] VideoFormat changed! Attempting to reconnect", pin->mLogPrefix);
#endif
            VIDEO_FORMAT newVideoFormat = {};
            LoadVideoFormat(&newVideoFormat, &pin->mVideoSignal);

            CMediaType proposedMediaType(pin->m_mt);
            pin->VideoFormatToMediaType(&proposedMediaType, &newVideoFormat);

            hr = pin->DoChangeMediaType(&proposedMediaType, &newVideoFormat, nullptr);
            if (FAILED(hr))
            {
#ifndef NO_QUILL
                LOG_WARNING(pin->mLogger, "[{}] VideoFormat changed but not able to reconnect! Sleeping [Result: {}]", pin->mLogPrefix, hr);
#endif
                // TODO show OSD to say we need to change
                BACKOFF;
                continue;
            }
        }

        // grab next frame and render it
        HANDLE aEventNotify[2] = { pin->mCaptureEvent, pin->mNotifyEvent };
        DWORD dwRet = WaitForMultipleObjects(2, aEventNotify, FALSE, 1000);

        // must be stopping
        if (dwRet == WAIT_OBJECT_0)
        {
            retVal = S_FALSE;
            break;
        }

        // unknown, try again
        if (dwRet == WAIT_FAILED)
        {
            continue;
        }

        // new frame, spin til it's consumed
        if (dwRet == WAIT_OBJECT_0 + 1)
        {
            pin->mLastMwResult = MWGetNotifyStatus(channel, pin->mNotify, &pin->mStatusBits);
            if (pin->mLastMwResult != MW_SUCCEEDED) continue;

            if (pin->mStatusBits & MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE)
            {
#ifndef NO_QUILL
                LOG_TRACE_L1(pin->mLogger, "[{}] Video signal change, sleeping", pin->mLogPrefix);
#endif
                BACKOFF;
                continue;
            }
            if (pin->mStatusBits & MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE)
            {
#ifndef NO_QUILL
                LOG_TRACE_L1(pin->mLogger, "[{}] Video input source change, sleeping", pin->mLogPrefix);
#endif
                BACKOFF;
                continue;
            }

            if (pin->mStatusBits & MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING)
            {
                pin->mLastMwResult = MWGetVideoBufferInfo(channel, &pin->mVideoSignal.bufferInfo);
                if (pin->mLastMwResult != MW_SUCCEEDED) continue;

                pin->mLastMwResult = MWGetVideoFrameInfo(channel, pin->mVideoSignal.bufferInfo.iNewestBuffered, &pin->mVideoSignal.frameInfo);
                if (pin->mLastMwResult != MW_SUCCEEDED) continue;

                // capture a frame to the framebuffer
                pin->mLastMwResult = MWCaptureVideoFrameToVirtualAddressEx(
                    channel,
                    pin->mVideoSignal.bufferInfo.iNewestBuffering,
                    pmsData,
                    pin->mVideoFormat.imageSize,
                    pin->mVideoFormat.lineLength,
                    FALSE,
                    nullptr,
                    pin->mVideoFormat.pixelStructure,
                    pin->mVideoSignal.signalStatus.cx,
                    pin->mVideoSignal.signalStatus.cy,
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
                do {
                    dwRet = WaitForSingleObject(pin->mCaptureEvent, 1000);
                    if (dwRet != WAIT_OBJECT_0)
                    {
                        if (pin->CheckStreamState(nullptr) == STREAM_DISCARDING)
                        {
                            break;
                        }
                        continue;
                    }
                    pin->mLastMwResult = MWGetVideoCaptureStatus(channel, &pin->mVideoSignal.captureStatus);

                    if (pin->mVideoSignal.captureStatus.bFrameCompleted)
                    {
                        MWGetDeviceTime(channel, &pin->mFrameEndTime);
                        auto endTime = pin->mFrameEndTime - pin->mStreamStartTime;
                        auto startTime = endTime - pin->mVideoFormat.frameInterval;
                        pms->SetTime(&startTime, &endTime);
                        pms->SetSyncPoint(TRUE);
                        pin->mFrameCounter++;
#ifndef NO_QUILL
                        LOG_TRACE_L1(pin->mLogger, "[{}] Captured video frame {} at {}", pin->mLogPrefix, pin->mFrameCounter, endTime);
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
                        if (pin->mVideoFormat.hdrMeta.exists && (pin->mFrameCounter % pin->mVideoFormat.fps) == 0)
                        {
#ifndef NO_QUILL
                            LOG_TRACE_L1(pin->mLogger, "[{}] Updating HDR meta in frame {}", pin->mLogPrefix, pin->mFrameCounter);
#endif
                            // This can fail if you have a filter behind this which does not understand side data
                            IMediaSideData* pMediaSideData = nullptr;
                            if (SUCCEEDED(pms->QueryInterface(&pMediaSideData)))
                            {
                                MediaSideDataHDR hdr;
                                ZeroMemory(&hdr, sizeof(hdr));

                                hdr.display_primaries_x[0] = pin->mVideoFormat.hdrMeta.g_primary_x * chromaticity_scale_factor;
                                hdr.display_primaries_x[1] = pin->mVideoFormat.hdrMeta.b_primary_x * chromaticity_scale_factor;
                                hdr.display_primaries_x[2] = pin->mVideoFormat.hdrMeta.r_primary_x * chromaticity_scale_factor;
                                hdr.display_primaries_y[0] = pin->mVideoFormat.hdrMeta.g_primary_y * chromaticity_scale_factor;
                                hdr.display_primaries_y[1] = pin->mVideoFormat.hdrMeta.b_primary_y * chromaticity_scale_factor;
                                hdr.display_primaries_y[2] = pin->mVideoFormat.hdrMeta.r_primary_y * chromaticity_scale_factor;

                                hdr.white_point_x = pin->mVideoFormat.hdrMeta.whitepoint_x * chromaticity_scale_factor;
                                hdr.white_point_y = pin->mVideoFormat.hdrMeta.whitepoint_y * chromaticity_scale_factor;

                                hdr.max_display_mastering_luminance = pin->mVideoFormat.hdrMeta.maxDML * high_luminance_scale_factor;
                                hdr.min_display_mastering_luminance = pin->mVideoFormat.hdrMeta.minDML * low_luminance_scale_factor;

                                pMediaSideData->SetSideData(IID_MediaSideDataHDR, reinterpret_cast<const BYTE*>(&hdr), sizeof(hdr));

                                MediaSideDataHDRContentLightLevel hdrLightLevel;
                                ZeroMemory(&hdrLightLevel, sizeof(hdrLightLevel));

                                hdrLightLevel.MaxCLL = pin->mVideoFormat.hdrMeta.maxCLL;
                                hdrLightLevel.MaxFALL = pin->mVideoFormat.hdrMeta.maxFALL;

                                pMediaSideData->SetSideData(IID_MediaSideDataHDRContentLightLevel, reinterpret_cast<const BYTE*>(&hdrLightLevel), sizeof(hdrLightLevel));
                                pMediaSideData->Release();
                            }
                        }
                    }
                } while (pin->mLastMwResult == MW_SUCCEEDED && !pin->mVideoSignal.captureStatus.bFrameCompleted);

                hasFrame = true;
            }
        }
    }
    return retVal;
}

//////////////////////////////////////////////////////////////////////////
//  MagewellCapturePin
//////////////////////////////////////////////////////////////////////////
void MagewellCapturePin::LoadVideoFormat(VIDEO_FORMAT* videoFormat, VIDEO_SIGNAL* videoSignal)
{
    if (videoSignal->signalStatus.state == MWCAP_VIDEO_SIGNAL_LOCKED)
    {
        videoFormat->cx = videoSignal->signalStatus.cx;
        videoFormat->cy = videoSignal->signalStatus.cy;
        videoFormat->aspectX = videoSignal->signalStatus.nAspectX;
        videoFormat->aspectY = videoSignal->signalStatus.nAspectY;
        videoFormat->quantization = videoSignal->signalStatus.quantRange;
        videoFormat->saturation = videoSignal->signalStatus.satRange;
        videoFormat->fps = 10000000 / videoSignal->signalStatus.dwFrameDuration;
        videoFormat->frameInterval = videoSignal->signalStatus.dwFrameDuration;
        videoFormat->bitDepth = videoSignal->inputStatus.hdmiStatus.byBitDepth;
        videoFormat->colourFormat = videoSignal->signalStatus.colorFormat;
        videoFormat->pixelEncoding = videoSignal->inputStatus.hdmiStatus.pixelEncoding;
    }
    LoadHdrMeta(&videoFormat->hdrMeta, &videoSignal->hdrInfo);

    if (videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV709)
    {
        videoFormat->colourFormatName = "YUV709";
    }
    else if (videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV2020)
    {
        videoFormat->colourFormatName = "YUV2020";
    }
    else 
    {
        videoFormat->colourFormatName = "UNK";
    }
    if (videoFormat->pixelEncoding == HDMI_ENCODING_RGB_444)
    {
        videoFormat->pixelStructure = MWFOURCC_RGB24;
        videoFormat->pixelStructureName = "RGB24";
        videoFormat->bitCount = 24;
    }
    else if (videoFormat->pixelEncoding == HDMI_ENCODING_YUV_420)
    {
        videoFormat->pixelStructure = videoFormat->bitDepth == 10 ? MWFOURCC_P010 : MWFOURCC_NV12;
        videoFormat->pixelStructureName = videoFormat->bitDepth == 10 ? "P010" : "NV12";
        videoFormat->bitCount = videoFormat->bitDepth == 10 ? 24 : 12;
    }
    else if (videoFormat->pixelEncoding == HDMI_ENCODING_YUV_422)
    {
        videoFormat->pixelStructure = videoFormat->bitDepth == 10 ? MWFOURCC_P210 : MWFOURCC_NV16;
        videoFormat->pixelStructureName = videoFormat->bitDepth == 10 ? "P210" : "NV16";
        videoFormat->bitCount = videoFormat->bitDepth == 10 ? 32 : 16;
    }
    else if (videoFormat->pixelEncoding == HDMI_ENCODING_YUV_444)
    {
        videoFormat->pixelStructure = videoFormat->bitDepth == 10 ? MWFOURCC_Y410 : MWFOURCC_V308;
        videoFormat->pixelStructureName = videoFormat->bitDepth == 10 ? "Y410" : "V308";
        videoFormat->bitCount = videoFormat->bitDepth == 10 ? 32 : 24;
    }
    videoFormat->lineLength = FOURCC_CalcMinStride(videoFormat->pixelStructure, videoFormat->cx, 2);
    videoFormat->imageSize = FOURCC_CalcImageSize(videoFormat->pixelStructure, videoFormat->cx, videoFormat->cy, videoFormat->lineLength);
}

void MagewellCapturePin::LoadHdrMeta(HDR_META* meta, HDMI_HDR_INFOFRAME_PAYLOAD* frame)
{
    auto hdrIn = *frame;
    auto hdrOut = meta;

    // https://shop.cta.tech/products/hdr-static-metadata-extensions
    // hdrInfo.byEOTF : 0 = SDR gamma, 1 = HDR gamma, 2 = ST2084
    int primaries_x[] = {
        hdrIn.display_primaries_lsb_x0 + (hdrIn.display_primaries_msb_x0 << 8),
        hdrIn.display_primaries_lsb_x1 + (hdrIn.display_primaries_msb_x1 << 8),
        hdrIn.display_primaries_lsb_x2 + (hdrIn.display_primaries_msb_x2 << 8)
    };
    int primaries_y[] = {
        hdrIn.display_primaries_lsb_y0 + (hdrIn.display_primaries_msb_y0 << 8),
        hdrIn.display_primaries_lsb_y1 + (hdrIn.display_primaries_msb_y1 << 8),
        hdrIn.display_primaries_lsb_y2 + (hdrIn.display_primaries_msb_y2 << 8)
    };
    // red = largest x, green = largest y, blue = remaining 
    auto r_idx = -1;
    auto maxVal = primaries_x[0];
    for (int i = 1; i < 3; ++i)
    {
        if (primaries_x[i] > maxVal)
        {
            maxVal = primaries_x[i];
            r_idx = i;
        }
    }

    auto g_idx = -1;
    maxVal = primaries_y[0];
    for (int i = 1; i < 3; ++i)
    {
        if (primaries_y[i] > maxVal)
        {
            maxVal = primaries_y[i];
            g_idx = i;
        }
    }

    if (g_idx > -1 && r_idx > -1 && g_idx != r_idx)
    {
        auto b_idx = 3 - g_idx - r_idx;
        if (b_idx != g_idx && b_idx != r_idx)
        {
            hdrOut->r_primary_x = primaries_x[r_idx];
            hdrOut->r_primary_y = primaries_y[r_idx];
            hdrOut->g_primary_x = primaries_x[g_idx];
            hdrOut->g_primary_y = primaries_y[g_idx];
            hdrOut->b_primary_x = primaries_x[b_idx];
            hdrOut->b_primary_y = primaries_y[b_idx];
        }
    }

    hdrOut->whitepoint_x = hdrIn.white_point_lsb_x + (hdrIn.white_point_msb_x << 8);
    hdrOut->whitepoint_y = hdrIn.white_point_lsb_y + (hdrIn.white_point_msb_y << 8);

    hdrOut->maxDML = hdrIn.max_display_mastering_lsb_luminance + (hdrIn.max_display_mastering_msb_luminance << 8);
    hdrOut->minDML = hdrIn.min_display_mastering_lsb_luminance + (hdrIn.min_display_mastering_msb_luminance << 8);

    hdrOut->maxCLL = hdrIn.maximum_content_light_level_lsb + (hdrIn.maximum_content_light_level_msb << 8);
    hdrOut->maxFALL = hdrIn.maximum_frame_average_light_level_lsb + (hdrIn.maximum_frame_average_light_level_msb << 8);

    hdrOut->transferFunction = hdrIn.byEOTF == 0x2 ? 15 : 4;

    hdrOut->exists = hdrOut->r_primary_x && hdrOut->r_primary_y
        && hdrOut->g_primary_x && hdrOut->g_primary_y
        && hdrOut->b_primary_x && hdrOut->b_primary_y
        && hdrOut->whitepoint_x && hdrOut->whitepoint_y
        && hdrOut->minDML && hdrOut->maxDML
        && hdrOut->maxCLL && hdrOut->maxFALL;
}

void MagewellCapturePin::LoadAudioFormat(AUDIO_FORMAT* audioFormat, AUDIO_SIGNAL* audioSignal)
{
    auto audioIn = *audioSignal;
    
	audioFormat->fs = audioIn.signalStatus.dwSampleRate;
    audioFormat->bitDepth = audioIn.signalStatus.cBitsPerSample;
    audioFormat->bitDepthInBytes = audioFormat->bitDepth / 8;
    audioFormat->pcm = audioIn.signalStatus.bLPCM;
    audioFormat->sampleInterval = 10000000.0 / audioFormat->fs;
    if (audioIn.signalStatus.wChannelValid & (0x01 << 0))
    {
        if (audioIn.signalStatus.wChannelValid & (0x01 << 1))
        {
            if (audioIn.signalStatus.wChannelValid & (0x01 << 2))
            {
                if (audioIn.signalStatus.wChannelValid & (0x01 << 3))
                {
                    audioFormat->channelCount = 8;
                    audioFormat->channelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
                }
                else
                {
                    audioFormat->channelCount = 6;
                    audioFormat->channelMask = KSAUDIO_SPEAKER_5POINT1;
                }
            }
            else
            {
                audioFormat->channelCount = 4;
                audioFormat->channelMask = KSAUDIO_SPEAKER_QUAD;
            }
        }
        else
        {
            audioFormat->channelCount = 2;
            audioFormat->channelMask = KSAUDIO_SPEAKER_STEREO;
        }
    }
	else
	{
        audioFormat->channelCount = 0;
	}
}

void MagewellCapturePin::VideoFormatToMediaType(CMediaType* pmt, VIDEO_FORMAT* videoFormat) const
{
    VIDEOINFOHEADER2* pvi = reinterpret_cast<VIDEOINFOHEADER2*>(pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2)));
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

    DXVA_ExtendedFormat* colorimetry = reinterpret_cast<DXVA_ExtendedFormat*>(&(pvi->dwControlFlags));
    // 1 = REC.709, 4 = BT.2020
    colorimetry->VideoTransferMatrix = videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV709 ? DXVA_VideoTransferMatrix_BT709 : static_cast<DXVA_VideoTransferMatrix>(4);
    // 1 = REC.709, 9 = BT.2020
    colorimetry->VideoPrimaries = videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV709 ? DXVA_VideoPrimaries_BT709 : static_cast<DXVA_VideoPrimaries>(9);
    // 4 = REC.709, 15 = SMPTE ST 2084 (PQ)
    colorimetry->VideoTransferFunction = static_cast<DXVA_VideoTransferFunction>(videoFormat->hdrMeta.transferFunction);
    // 0 = unknown, 1 = 0-255, 2 = 16-235
    colorimetry->NominalRange = static_cast<DXVA_NominalRange>(videoFormat->quantization);

	#ifndef NO_QUILL
	LOG_TRACE_L3(mLogger, "[{}] DXVA_ExtendedFormat {} {} {} {}", mLogPrefix, static_cast<int>(colorimetry->VideoTransferMatrix), 
        static_cast<int>(colorimetry->VideoPrimaries), static_cast<int>(colorimetry->VideoTransferFunction), static_cast<int>(colorimetry->NominalRange));
	#endif

	pvi->dwControlFlags += AMCONTROL_USED;
    pvi->dwControlFlags += AMCONTROL_COLORINFO_PRESENT;

    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = videoFormat->cx;
    pvi->bmiHeader.biHeight = videoFormat->cy;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biBitCount = videoFormat->bitCount;
    pvi->bmiHeader.biCompression = videoFormat->pixelStructure;
    pvi->bmiHeader.biSizeImage = videoFormat->imageSize;
    pvi->bmiHeader.biXPelsPerMeter = 0;
    pvi->bmiHeader.biYPelsPerMeter = 0;
    pvi->bmiHeader.biClrUsed = 0;
    pvi->bmiHeader.biClrImportant = 0;

    // Work out the GUID for the subtype from the header info.
    const auto subTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&subTypeGUID);
}

bool MagewellCapturePin::ShouldChangeVideoMediaType()
{
    // detect format changes
    auto reconnect = false;
    if (mVideoSignal.signalStatus.cx != mVideoFormat.cx || mVideoSignal.signalStatus.cy != mVideoFormat.cy)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video dimension change {}x{} to {}x{}", mLogPrefix, mVideoFormat.cx, mVideoFormat.cy, mVideoSignal.signalStatus.cx, mVideoSignal.signalStatus.cy);
#endif
    	reconnect = true;
    }
    if (mVideoSignal.signalStatus.nAspectX != mVideoFormat.aspectX || mVideoSignal.signalStatus.nAspectY != mVideoFormat.aspectY)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video AR change {}x{} to {}x{}", mLogPrefix, mVideoFormat.aspectX, mVideoFormat.aspectY, mVideoSignal.signalStatus.nAspectX, mVideoSignal.signalStatus.nAspectY);
#endif
        reconnect = true;
    }
    if (abs(mVideoSignal.signalStatus.dwFrameDuration - mVideoFormat.frameInterval) >= 100)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video FPS change {} to {}", mLogPrefix, mVideoFormat.fps, 10000000 / mVideoSignal.signalStatus.dwFrameDuration);
#endif
        reconnect = true;
    }
    if (mVideoFormat.bitDepth != mVideoSignal.inputStatus.hdmiStatus.byBitDepth)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video bit depth change {} to {}", mLogPrefix, mVideoFormat.bitDepth, mVideoSignal.inputStatus.hdmiStatus.byBitDepth);
#endif
        reconnect = true;
    }
    if (mVideoFormat.pixelEncoding != mVideoSignal.inputStatus.hdmiStatus.pixelEncoding)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video pixel encoding change {} to {}", mLogPrefix, static_cast<int>(mVideoFormat.pixelEncoding), static_cast<int>(mVideoSignal.inputStatus.hdmiStatus.pixelEncoding));
#endif
        reconnect = true;
    }
    if (mVideoFormat.colourFormat != mVideoSignal.signalStatus.colorFormat)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video colour format change {} to {}", mLogPrefix, static_cast<int>(mVideoFormat.colourFormat), static_cast<int>(mVideoSignal.signalStatus.colorFormat));
#endif
        reconnect = true;
    }
    if (mVideoFormat.quantization != mVideoSignal.signalStatus.quantRange || mVideoFormat.saturation != mVideoSignal.signalStatus.satRange)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video colorimetry change quant {} to {} sat {} to {}", mLogPrefix,
            static_cast<int>(mVideoFormat.quantization), static_cast<int>(mVideoSignal.signalStatus.quantRange),
            static_cast<int>(mVideoFormat.saturation), static_cast<int>(mVideoSignal.signalStatus.satRange)
        );
#endif
        reconnect = true;
    }
    auto incomingTransferFunction = mVideoSignal.hdrInfo.byEOTF == 0x2 ? 15 : 4;
    if (mVideoFormat.hdrMeta.transferFunction != incomingTransferFunction)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Video transfer function change {} to {}", mLogPrefix, mVideoFormat.hdrMeta.transferFunction, incomingTransferFunction);
#endif
        reconnect = true;
    }

    return reconnect;
}

void MagewellCapturePin::AudioFormatToMediaType(CMediaType* pmt, AUDIO_FORMAT* audioFormat)
{
    // based on https://github.com/Nevcairiel/LAVFilters/blob/81c5676cb99d0acfb1457b8165a0becf5601cae3/decoder/LAVAudio/LAVAudio.cpp#L1186
    pmt->majortype = MEDIATYPE_Audio;
    // support for non PCM format?
    pmt->subtype = MEDIASUBTYPE_PCM;
    pmt->formattype = FORMAT_WaveFormatEx;

    WAVEFORMATEXTENSIBLE wfex;
    memset(&wfex, 0, sizeof(wfex));

    WAVEFORMATEX* wfe = &wfex.Format;
    wfe->wFormatTag = static_cast<WORD>(pmt->subtype.Data1);
    wfe->nChannels = audioFormat->channelCount;
    wfe->nSamplesPerSec = audioFormat->fs;
    wfe->wBitsPerSample = audioFormat->bitDepth;
    wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample / 8;
    wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;

    if (audioFormat->channelCount > 2 || wfe->wBitsPerSample > 16 || wfe->nSamplesPerSec > 48000)
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

bool MagewellCapturePin::ShouldChangeAudioMediaType(AUDIO_FORMAT* newAudioFormat)
{
    auto reconnect = false;
    if (mAudioFormat.channelCount != newAudioFormat->channelCount)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Channel count change {} to {}", mLogPrefix, mAudioFormat.channelCount, newAudioFormat->channelCount);
#endif
        reconnect = true;
    }
    if (mAudioFormat.bitDepthInBytes != newAudioFormat->bitDepthInBytes)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Bit depth change {} to {}", mLogPrefix, mAudioFormat.bitDepthInBytes, newAudioFormat->bitDepthInBytes);
#endif
        reconnect = true;
    }
    if (mAudioFormat.fs != newAudioFormat->fs)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] Fs change {} to {}", mLogPrefix, mAudioFormat.fs, newAudioFormat->fs);
#endif
        reconnect = true;
    }
    if (mAudioFormat.pcm != newAudioFormat->pcm)
    {
#ifndef NO_QUILL
        LOG_INFO(mLogger, "[{}] PCM change {} to {}", mLogPrefix, mAudioFormat.pcm, newAudioFormat->pcm);
#endif
        reconnect = true;
    }
    return reconnect;
}

HRESULT MagewellCapturePin::LoadVideoSignal(HCHANNEL* pChannel)
{
    mLastMwResult = MWGetVideoSignalStatus(*pChannel, &mVideoSignal.signalStatus);
    if (mLastMwResult != MW_SUCCEEDED)
    {
#ifndef NO_QUILL
        LOG_WARNING(mLogger, "MagewellCapturePin::LoadVideoSignal MWGetVideoSignalStatus failed");
#endif
        return S_FALSE;
    }
    mLastMwResult = MWGetInputSpecificStatus(*pChannel, &mVideoSignal.inputStatus);
    if (mLastMwResult != MW_SUCCEEDED)
    {
#ifndef NO_QUILL
        LOG_WARNING(mLogger, "MagewellCapturePin::LoadVideoSignal MWGetInputSpecificStatus failed");
#endif
        return S_FALSE;
    }
    DWORD tPdwValidFlag = 0;
    MWGetHDMIInfoFrameValidFlag(*pChannel, &tPdwValidFlag);
    HDMI_INFOFRAME_PACKET pkt;
    if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_HDR) {
        MWGetHDMIInfoFramePacket(*pChannel, MWCAP_HDMI_INFOFRAME_ID_HDR, &pkt);
#ifndef NO_QUILL
        if (!mHasHdrInfoFrame)
        {
            LOG_TRACE_L1(mLogger, "[{}] HDR Infoframe is present tf: {} to {}", mLogPrefix, mVideoSignal.hdrInfo.byEOTF, pkt.hdrInfoFramePayload.byEOTF);
        }
#endif
        mVideoSignal.hdrInfo = pkt.hdrInfoFramePayload;
        mHasHdrInfoFrame = true;
    }
	else
    {
#ifndef NO_QUILL
        if (mHasHdrInfoFrame)
        {
            LOG_TRACE_L1(mLogger, "[{}] HDR Infoframe no longer present", mLogPrefix);
        }
#endif
        mHasHdrInfoFrame = false;
        mVideoSignal.hdrInfo = {};
    }
    if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_AVI) {
        MWGetHDMIInfoFramePacket(*pChannel, MWCAP_HDMI_INFOFRAME_ID_AVI, &pkt);
        mVideoSignal.aviInfo = pkt.aviInfoFramePayload;
    }
	else
    {
        mVideoSignal.aviInfo = {};
    }
    return S_OK;
}

MagewellCapturePin::MagewellCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, mw_pin_type pPinType) :
    CSourceStream(
        pPinType == PIN_VIDEO_CAPTURE ? L"VideoCapture" : pPinType == PIN_VIDEO_PREVIEW ? L"VideoPreview" : L"AudioCapture", 
        phr,
		pParent,
        pPinType == PIN_VIDEO_CAPTURE ? L"Capture" : pPinType == PIN_VIDEO_PREVIEW ? L"Preview" : L"Audio"
    ), mFilter(pParent)
{
    mPinType = pPinType;
#ifndef NO_QUILL
    mLogPrefix = pPinType == PIN_VIDEO_CAPTURE ? "Capture" : pPinType == PIN_VIDEO_PREVIEW ? "Preview" : "Audio";
    mLogger = CustomFrontend::get_logger("filter");
#endif
	auto h_channel = mFilter->GetChannelHandle();
    
    if (mPinType != PIN_AUDIO_CAPTURE)
    {
        auto hr = LoadVideoSignal(&h_channel);
        if (SUCCEEDED(hr))
        {
            LoadVideoFormat(&mVideoFormat, &mVideoSignal);
#ifndef NO_QUILL
            LOG_WARNING(mLogger, "[{}] Initialised video format {} x {} ({}:{}) @ {} Hz in {} bits ({} {} tf: {}) size {} bytes", mLogPrefix,
                mVideoFormat.cx, mVideoFormat.cy, mVideoFormat.aspectX, mVideoFormat.aspectY, mVideoFormat.fps, mVideoFormat.bitDepth, 
                mVideoFormat.pixelStructureName, mVideoFormat.colourFormatName, mVideoFormat.hdrMeta.transferFunction, mVideoFormat.imageSize);
#endif
        }
    	else
        {
            mVideoFormat.lineLength = FOURCC_CalcMinStride(mVideoFormat.pixelStructure, mVideoFormat.cx, 2);
            mVideoFormat.imageSize = FOURCC_CalcImageSize(mVideoFormat.pixelStructure, mVideoFormat.cx, mVideoFormat.cy, mVideoFormat.lineLength);
#ifndef NO_QUILL
            LOG_WARNING(mLogger, "[{}] Initialised video format using defaults {} x {} ({}:{}) @ {} Hz in {} bits ({} {} tf: {}) size {} bytes", mLogPrefix,
                mVideoFormat.cx, mVideoFormat.cy, mVideoFormat.aspectX, mVideoFormat.aspectY, mVideoFormat.fps, mVideoFormat.bitDepth,
                mVideoFormat.pixelStructureName, mVideoFormat.colourFormatName, mVideoFormat.hdrMeta.transferFunction, mVideoFormat.imageSize);
#endif
        }
    }
	else
    {
        DWORD dwInputCount = 0;
        mLastMwResult = MWGetAudioInputSourceArray(h_channel, nullptr, &dwInputCount);
        if (mLastMwResult != MW_SUCCEEDED)
        {
#ifndef NO_QUILL
            LOG_ERROR(mLogger, "[{}] ERROR! MWGetAudioInputSourceArray", mLogPrefix);
#endif
        }
        if (dwInputCount == 0)
        {
#ifndef NO_QUILL
            LOG_ERROR(mLogger, "[{}] ERROR! No audio signal detected", mLogPrefix);
#endif
        }
        else
        {
            MWGetAudioSignalStatus(h_channel, &mAudioSignal.signalStatus);
            if (mAudioSignal.signalStatus.wChannelValid == 0)
            {
#ifndef NO_QUILL
                LOG_ERROR(mLogger, "[{}] ERROR! No valid audio channels detected {}", mLogPrefix, mAudioSignal.signalStatus.wChannelValid);
#endif
            }
            else
            {
                LoadAudioFormat(&mAudioFormat, &mAudioSignal);
            }
        }
#ifndef NO_QUILL
        LOG_WARNING(mLogger, "[{}] Audio Status Fs: {} Bits: {} Channels: {} PCM: {}", mLogPrefix, mAudioFormat.fs, mAudioFormat.bitDepth, mAudioFormat.channelCount, mAudioFormat.pcm);
#endif
    }
}

HRESULT MagewellCapturePin::OnThreadStartPlay()
{
    if (mStreamStartTime == 0)
    {
        LONGLONG now;
        MWGetDeviceTime(mFilter->GetChannelHandle(), &now);
#ifndef NO_QUILL
        LOG_WARNING(mLogger, "[{}] Pin worker thread starting at {}, using for stream start time", mLogPrefix, now);
#endif
        mStreamStartTime = now;
    }
    return S_OK;
}

HRESULT MagewellCapturePin::DoChangeMediaType(const CMediaType* pmt, const VIDEO_FORMAT* newVideoFormat = nullptr, const AUDIO_FORMAT* newAudioFormat = nullptr)
{
    auto timeout = 100;
    auto retVal = S_FALSE;
    auto* oldVideoFormat = &mVideoFormat;
    auto* oldAudioFormat = &mAudioFormat;

#ifndef NO_QUILL
    if (newVideoFormat != nullptr)
    {
        LOG_WARNING(mLogger, "[{}] Proposing new video format {} x {} ({}:{}) @ {} Hz in {} bits ({} {} tf: {}) size {} bytes", mLogPrefix, 
            newVideoFormat->cx, newVideoFormat->cy, newVideoFormat->aspectX, newVideoFormat->aspectY, newVideoFormat->fps, newVideoFormat->bitDepth, 
            newVideoFormat->pixelStructureName, newVideoFormat->colourFormatName, newVideoFormat->hdrMeta.transferFunction, newVideoFormat->imageSize);
    }
    if (newAudioFormat != nullptr)
    {
        LOG_WARNING(mLogger, "[{}] Proposing new audio format Fs: {} Bits: {} Channels: {} PCM: {}", mLogPrefix,
            newAudioFormat->fs, newAudioFormat->bitDepth, newAudioFormat->channelCount, newAudioFormat->pcm);
    }
#endif

	HRESULT hrQA = m_Connected->QueryAccept(pmt);

	receiveconnection:

    HRESULT hr = m_Connected->ReceiveConnection(this, pmt);
    if (SUCCEEDED(hr))
    {
#ifndef NO_QUILL
        LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType ReceiveConnection accepted", mLogPrefix);
#endif
        if (newVideoFormat != nullptr)
        {
            mVideoFormat = *newVideoFormat;
        }
        if (newAudioFormat != nullptr)
        {
            mAudioFormat = *newAudioFormat;
        }
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
            LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType Buffers outstanding, retrying in 10ms..", mLogPrefix);
#endif
            BACKOFF;
            timeout -= 10;
        }
        else
        {
#ifndef NO_QUILL
            LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType Buffers outstanding, timeout reached, flushing..", mLogPrefix);
#endif
            DeliverBeginFlush();
            DeliverEndFlush();
            timeout = -1;
        }
        goto receiveconnection;
    }
    else if (SUCCEEDED(hrQA))
    {
#ifndef NO_QUILL
        LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType QueryAccept accepted", mLogPrefix);
#endif
        if (newVideoFormat != nullptr)
        {
            mVideoFormat = *newVideoFormat;
        }
        if (newAudioFormat != nullptr)
        {
            mAudioFormat = *newAudioFormat;
        }
        hr = SetMediaType(pmt);
        if (SUCCEEDED(hr))
        {
            long newSize = 0;
            if (newVideoFormat != nullptr)
            {
                if (oldVideoFormat->imageSize == newVideoFormat->imageSize)
                {
#ifndef NO_QUILL
                    LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType - No video buffer change", mLogPrefix);
#endif
                    retVal = S_OK;
                }
            	else
            	{
                    newSize = newVideoFormat->imageSize;
            	}
            }
            if (newAudioFormat != nullptr)
            {
	            if ((oldAudioFormat->bitDepthInBytes * oldAudioFormat->channelCount) == (newAudioFormat->bitDepthInBytes * newAudioFormat->channelCount))
	            {
#ifndef NO_QUILL
                    LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType - No audio buffer change", mLogPrefix);
#endif
                    retVal = S_OK;
                }
                else
                {
                    newSize = newAudioFormat->bitDepthInBytes * newAudioFormat->channelCount;
                }
            }
            
            if (nullptr != m_pInputPin && newSize > 0) {
                ALLOCATOR_PROPERTIES props, actual;
                m_pAllocator->GetProperties(&props);
                m_pAllocator->Decommit();
                props.cbBuffer = newSize;
                m_pAllocator->SetProperties(&props, &actual);
                hr = m_pAllocator->Commit();
                if (SUCCEEDED(hr))
                {
#ifndef NO_QUILL
                    LOG_TRACE_L1(mLogger, "[{}] Updated allocator {} bytes {} buffers", props.cbBuffer, props.cBuffers);
#endif
                    retVal = S_OK;
                }
            }
        }
    }
#ifndef NO_QUILL
    else
    {
        LOG_WARNING(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType Receive Connection failed (hr: {}); QueryAccept: {}", mLogPrefix, hr, hrQA);
    }
#endif
    if (retVal == S_OK)
    {
#ifndef NO_QUILL
        LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType succeeded", mLogPrefix);
#endif
        mSendMediaType = TRUE;

        if (newVideoFormat != nullptr)
        {
            mFilter->NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(newVideoFormat->cx, newVideoFormat->cy), 0);
        }
    }
    else
    {
        // reinstate the old formats otherwise we're stuck thinking we have the new format
#ifndef NO_QUILL
        LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DoChangeMediaType failed {}", mLogPrefix, retVal);
#endif
        if (newVideoFormat != nullptr)
        {
            mVideoFormat = *oldVideoFormat;
        }
        if (newAudioFormat != nullptr)
        {
            mAudioFormat = *oldAudioFormat;
        }
    }

    return retVal;
}


HRESULT MagewellCapturePin::FillBuffer(IMediaSample* pms)
{
    auto h_channel = mFilter->GetChannelHandle();
    auto retVal = S_OK;

	if (mPinType != PIN_AUDIO_CAPTURE)
    {
        VideoFrameGrabber vfg(this, h_channel, pms);
        retVal = vfg.grab();
    }
	else 
    {
        BYTE* pmsData;
        pms->GetPointer(&pmsData);
        auto hasFrame = false;
        // keep going til we have a frame
        while (!hasFrame)
        {
            if (CheckStreamState(nullptr) == STREAM_DISCARDING)
            {
#ifndef NO_QUILL
                LOG_TRACE_L1(mLogger, "[{}] Stream is discarding", mLogPrefix);
#endif
                break;
            }

            mLastMwResult = MWGetAudioSignalStatus(h_channel, &mAudioSignal.signalStatus);
            if (mLastMwResult != MW_SUCCEEDED)
            {
#ifndef NO_QUILL
                LOG_WARNING(mLogger, "[{}] MWGetAudioSignalStatus failed, sleeping", mLogPrefix);
#endif
                BACKOFF;
                continue;
            }

            AUDIO_FORMAT newAudioFormat;
            LoadAudioFormat(&newAudioFormat, &mAudioSignal);

            if (newAudioFormat.channelCount == 0)
            {
#ifndef NO_QUILL
                LOG_TRACE_L3(mLogger, "[{}] No signal, sleeping", mLogPrefix);
#endif
                BACKOFF;
                continue;
            }

            // detect format changes
            if (ShouldChangeAudioMediaType(&newAudioFormat))
            {
#ifndef NO_QUILL
                LOG_WARNING(mLogger, "[{}] AudioFormat changed! Attempting to reconnect", mLogPrefix);
#endif
                CMediaType proposedMediaType(m_mt);
                AudioFormatToMediaType(&proposedMediaType, &newAudioFormat);
                auto hr = DoChangeMediaType(&proposedMediaType, nullptr, &newAudioFormat);
                if (FAILED(hr))
                {
#ifndef NO_QUILL
                    LOG_WARNING(mLogger, "[{}] AudioFormat changed but not able to reconnect ({}) sleeping", mLogPrefix, hr);
#endif
                    // TODO show OSD to say we need to change
                    BACKOFF;
                    continue;
                }
            }

            mLastMwResult = MWGetNotifyStatus(h_channel, mNotify, &mStatusBits);
            if (mStatusBits & MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE)
            {
#ifndef NO_QUILL
                LOG_TRACE_L1(mLogger, "[{}] Audio signal change, sleeping", mLogPrefix);
#endif
                BACKOFF;
                continue;
            }
            if (mStatusBits & MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE)
            {
#ifndef NO_QUILL
                LOG_TRACE_L1(mLogger, "[{}] Audio input source change, sleeping", mLogPrefix);
#endif
                BACKOFF;
                continue;
            }

            if (mStatusBits & MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED) {
                mLastMwResult = MWCaptureAudioFrame(h_channel, &mAudioSignal.frameInfo);
                if (MW_SUCCEEDED != mLastMwResult)
                {
#ifndef NO_QUILL
                    LOG_WARNING(mLogger, "[{}] Audio frame buffered but capture failed, sleeping", mLogPrefix);
#endif
                    BACKOFF;
                	continue;
                }

                auto pbAudioFrame = reinterpret_cast<BYTE*>(mAudioSignal.frameInfo.adwSamples);
                auto sampleSize = pms->GetSize();
                long bytesCaptured = 0;
                int samplesCaptured = 0;
                for (int j = 0; j < mAudioFormat.channelCount / 2; ++j)
                {
                    // channel order on input is L0-L3,R0-R3 which has to be remapped to L0,R0,L1,R1,L2,R2,L3,R3
                    // each 4 byte sample is left zero padded if the incoming stream is a lower bit depth (which is typically the case for HDMI audio)
                    for (int i = 0; i < MWCAP_AUDIO_SAMPLES_PER_FRAME; i++)
                    {
                        int outByteStartIdx = (i * mAudioFormat.channelCount + j * 2) * mAudioFormat.bitDepthInBytes;
                        int inLeftByteStartIdx = ((i * MWCAP_AUDIO_MAX_NUM_CHANNELS + j) * MAX_BIT_DEPTH_IN_BYTE) + MAX_BIT_DEPTH_IN_BYTE - mAudioFormat.bitDepthInBytes;
                        int inRightByteStartIdx = ((i * MWCAP_AUDIO_MAX_NUM_CHANNELS + j + MWCAP_AUDIO_MAX_NUM_CHANNELS / 2) * MAX_BIT_DEPTH_IN_BYTE) + MAX_BIT_DEPTH_IN_BYTE - mAudioFormat.bitDepthInBytes;
                        for (int k = 0; k < mAudioFormat.bitDepthInBytes; ++k)
                        {
                            auto leftOutIdx = outByteStartIdx + k;
                            auto rightOutIdx = leftOutIdx + mAudioFormat.bitDepthInBytes;
                            auto inLeftIdx = inLeftByteStartIdx + k;
                            auto inRightIdx = inRightByteStartIdx + k;
                            bytesCaptured += 2;
						    if (leftOutIdx < sampleSize && rightOutIdx < sampleSize)
                            {
                                pmsData[leftOutIdx] = pbAudioFrame[inLeftIdx];
                                pmsData[rightOutIdx] = pbAudioFrame[inRightIdx];

                            }
                        }
                        samplesCaptured++;
                    }
                }
                mFrameCounter++;
                if (mSinceLastLog++ >= 1000)
                {
                    mSinceLastLog = 0;
                }
                MWGetDeviceTime(h_channel, &mFrameEndTime);
                auto endTime = mFrameEndTime - mStreamStartTime;
                auto startTime = endTime - static_cast<long>(mAudioFormat.sampleInterval * MWCAP_AUDIO_SAMPLES_PER_FRAME);
#ifndef NO_QUILL
                if (bytesCaptured != sampleSize)
                {
                    LOG_WARNING(mLogger, "[{}] Audio frame {} : samples {} time {} size {} bytes buf {} bytes", mLogPrefix, mFrameCounter, samplesCaptured, endTime, bytesCaptured, sampleSize);
                }
            	else if (mSinceLastLog == 0)
                {
                    LOG_TRACE_L1(mLogger, "[{}] Audio frame {} : samples {} time {} size {} bytes buf {} bytes", mLogPrefix, mFrameCounter, samplesCaptured, endTime, bytesCaptured, sampleSize);
                }
            	else 
                {
                    LOG_TRACE_L3(mLogger, "[{}] Audio frame {} : samples {} time {} size {} bytes buf {} bytes", mLogPrefix, mFrameCounter, samplesCaptured, endTime, bytesCaptured, sampleSize);
                }
#endif
                pms->SetTime(&startTime, &endTime);
                pms->SetSyncPoint(TRUE);
                if (mSendMediaType)
                {
                    CMediaType cmt(m_mt);
                    AM_MEDIA_TYPE* sendMediaType = CreateMediaType(&cmt);
                    pms->SetMediaType(sendMediaType);
                    DeleteMediaType(sendMediaType);
                    mSendMediaType = FALSE;
                }
                hasFrame = true;
            }
        }
    }
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
    }
    else
    {
#ifndef NO_QUILL
        LOG_TRACE_L1(mLogger, "[{}] Entering stream discard", mLogPrefix);
#endif
        mLastSampleDiscarded = TRUE;
        retVal = S_FALSE;
    }

	return retVal;
}

/**
 * called after the pins are connected to allocate buffers to stream data.
 */
HRESULT MagewellCapturePin::DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    CheckPointer(pIMemAlloc, E_POINTER)
    CheckPointer(pProperties, E_POINTER)
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;
    if (pProperties->cBuffers < 1)
    {
        // 1 works for mpc-vr, 16 works for madVR so go with that as a default if the input pin doesn't suggest a number.
        pProperties->cBuffers = 16;
    }
    if (mPinType != PIN_AUDIO_CAPTURE)
    {
        pProperties->cbBuffer = mVideoFormat.imageSize;
    }
    else
    {
        int frameSize = MWCAP_AUDIO_SAMPLES_PER_FRAME * mAudioFormat.bitDepthInBytes * mAudioFormat.channelCount;
        pProperties->cbBuffer = frameSize;
    }
#ifndef NO_QUILL
    LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::DecideBufferSize size: {} count: {}", mLogPrefix, pProperties->cbBuffer, pProperties->cBuffers);
#endif
    ALLOCATOR_PROPERTIES actual;
    hr = pIMemAlloc->SetProperties(pProperties, &actual);

    if (FAILED(hr))
    {
#ifndef NO_QUILL
        LOG_WARNING(mLogger, "[{}] MagewellCapturePin::DecideBufferSize failed to SetProperties result {}", mLogPrefix, hr);
#endif
        return hr;
    } 
    if (actual.cbBuffer < pProperties->cbBuffer)
    {
#ifndef NO_QUILL
        LOG_WARNING(mLogger, "[{}] MagewellCapturePin::DecideBufferSize actual buffer is {} not {}", mLogPrefix, actual.cbBuffer, pProperties->cbBuffer);
#endif
    	return E_FAIL;
    }

	return S_OK;
}

HRESULT MagewellCapturePin::GetMediaType(CMediaType* pmt)
{
    if (mPinType != PIN_AUDIO_CAPTURE)
    {
        VideoFormatToMediaType(pmt, &mVideoFormat);
    }
    else
    {
        AudioFormatToMediaType(pmt, &mAudioFormat);
    }
#ifndef NO_QUILL
    LOG_TRACE_L3(mLogger, "[{}] GetMediaType", mLogPrefix);
#endif
    return NOERROR;
}

HRESULT MagewellCapturePin::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr = CSourceStream::SetMediaType(pmt);
#ifndef NO_QUILL
    LOG_TRACE_L3(mLogger, "[{}] SetMediaType ({})", mLogPrefix, hr);
#endif
    return hr;
}

HRESULT MagewellCapturePin::OnThreadCreate()
{
#ifndef NO_QUILL
    CustomFrontend::preallocate();

	LOG_INFO(mLogger, "[{}] MagewellCapturePin::OnThreadCreate", mLogPrefix);
#endif
    // Wait Events
    mNotifyEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    const auto h_channel = mFilter->GetChannelHandle();

    if (mPinType != PIN_AUDIO_CAPTURE)
    {
        mLastMwResult = MWGetVideoSignalStatus(h_channel, &mVideoSignal.signalStatus);
#ifndef NO_QUILL
        if (mLastMwResult != MW_SUCCEEDED)
    	{
            LOG_WARNING(mLogger, "[{}] Unable to MWGetVideoSignalStatus", mLogPrefix);
	    }
#endif
    	// start capture
        mCaptureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        mLastMwResult = MWStartVideoCapture(h_channel, mCaptureEvent);
#ifndef NO_QUILL
        if (mLastMwResult != MW_SUCCEEDED)
        {
            LOG_ERROR(mLogger, "[{}] Unable to MWStartVideoCapture", mLogPrefix);
            // TODO throw?
        }
#endif
    	// register for signal change events & video buffering
        mNotify = MWRegisterNotify(h_channel, mNotifyEvent, MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE | MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING | MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE);
#ifndef NO_QUILL
        if (!mNotify)
        {
            LOG_ERROR(mLogger, "[{}] Unable to MWRegistryNotify", mLogPrefix);
            // TODO throw
        }
#endif
    }
    else
    {
        mLastMwResult = MWGetAudioSignalStatus(h_channel, &mAudioSignal.signalStatus);
#ifndef NO_QUILL
        if (mLastMwResult != MW_SUCCEEDED)
        {
            LOG_WARNING(mLogger, "[{}] Unable to MWGetAudioSignalStatus", mLogPrefix);
        }
#endif

        // start capture
        mLastMwResult = MWStartAudioCapture(h_channel);
#ifndef NO_QUILL
        if (mLastMwResult != MW_SUCCEEDED)
        {
            LOG_ERROR(mLogger, "[{}] Unable to MWStartAudioCapture", mLogPrefix);
            // TODO throw
        }
#endif
        // register for signal change events & audio buffered
        mNotify = MWRegisterNotify(h_channel, mNotifyEvent, MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE | MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE | MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED);
#ifndef NO_QUILL
        if (!mNotify)
        {
            LOG_ERROR(mLogger, "[{}] Unable to MWRegistryNotify", mLogPrefix);
            // TODO throw
        }
#endif
    }
	return NOERROR;
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
    if (mNotifyEvent)
    {
        CloseHandle(mNotifyEvent);
    }
    if (mCaptureEvent)
    {
        if (mPinType != PIN_AUDIO_CAPTURE)
        {
            MWStopVideoCapture(mFilter->GetChannelHandle());
        }
    	else
    	{
            MWStopAudioCapture(mFilter->GetChannelHandle());
    	}
        CloseHandle(mCaptureEvent);
    }
#ifndef NO_QUILL
    LOG_INFO(mLogger, "[{}] <<< MagewellCapturePin::OnThreadDestroy", mLogPrefix);
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
    else if (riid == _uuidof(IKsPropertySet)) 
    {
        return GetInterface(static_cast<IKsPropertySet*>(this), ppv);
    }
    else if (riid == _uuidof(IAMStreamControl))
    {
        return GetInterface(static_cast<IAMStreamControl*>(this), ppv);
    }
    else if (riid == _uuidof(IAMPushSource))
    {
        return GetInterface(static_cast<IAMPushSource*>(this), ppv);
    }
    else
    {
        return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
    }
}

void MagewellCapturePin::SetStartTime(LONGLONG streamStartTime)
{
    if (mStreamStartTime == 0)
    {
        mStreamStartTime = streamStartTime;
#ifndef NO_QUILL
        LOG_WARNING(mLogger, "[{}] MagewellCapturePin::SetStartTime at {}", mLogPrefix, streamStartTime);
    }
    else
    {
        LOG_WARNING(mLogger, "[{}] MagewellCapturePin::SetStartTime at {} but already set to {}", mLogPrefix, streamStartTime, mStreamStartTime);
#endif
    }
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

STDMETHODIMP MagewellCapturePin::GetNumberOfCapabilities(int* piCount, int* piSize)
{
    *piCount = 1;
    if (mPinType != PIN_AUDIO_CAPTURE)
    {
        *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    }
    else
    {
        *piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);
    }
    return S_OK;
}

STDMETHODIMP MagewellCapturePin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC)
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

	if (mPinType != PIN_AUDIO_CAPTURE)
    {
    	VIDEOINFOHEADER2* pvi = reinterpret_cast<VIDEOINFOHEADER2*>((*pmt)->pbFormat);

        VIDEO_STREAM_CONFIG_CAPS* pvscc = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(pSCC);

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
    }
    else
    {
        WAVEFORMATEXTENSIBLE* wfe = reinterpret_cast<WAVEFORMATEXTENSIBLE*>((*pmt)->pbFormat);
        AUDIO_STREAM_CONFIG_CAPS* pascc = reinterpret_cast<AUDIO_STREAM_CONFIG_CAPS*>(pSCC);

        pascc->guid = FORMAT_WaveFormatEx;
        pascc->MinimumChannels = mAudioFormat.channelCount;
        pascc->MaximumChannels = mAudioFormat.channelCount;
        pascc->ChannelsGranularity = 1;
        pascc->MinimumBitsPerSample = mAudioFormat.bitDepth;
        pascc->MaximumBitsPerSample = mAudioFormat.bitDepth;
        pascc->BitsPerSampleGranularity = 1;
        pascc->MinimumSampleFrequency = mAudioFormat.fs;
        pascc->MaximumSampleFrequency = mAudioFormat.fs;
        pascc->SampleFrequencyGranularity = 1;
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////

HRESULT MagewellCapturePin::Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData,
    DWORD cbInstanceData, void* pPropData, DWORD cbPropData)
{
	// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT MagewellCapturePin::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void* pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void* pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD* pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin)                     return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)                return E_PROP_ID_UNSUPPORTED;
    if (pPropData == nullptr && pcbReturned == nullptr)     return E_POINTER;

    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == nullptr)       return S_OK;            // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;    // The buffer is too small.

    // declares the pin to a live source capture or preview pin
    *static_cast<GUID*>(pPropData) = mPinType == PIN_VIDEO_PREVIEW ? PIN_CATEGORY_PREVIEW : PIN_CATEGORY_CAPTURE;  
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
