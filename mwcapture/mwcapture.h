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

#define NOMINMAX


#ifndef NO_QUILL
#include <quill/Logger.h>
#include <quill/LogMacros.h>
#include "quill/Frontend.h"

struct CustomFrontendOptions
{
    static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
    static constexpr uint32_t initial_queue_capacity = 64 * 1024 * 1024; // 64MiB
    static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
    static constexpr bool huge_pages_enabled = false;
};
using CustomFrontend = quill::FrontendImpl<CustomFrontendOptions>;
using CustomLogger = quill::LoggerImpl<CustomFrontendOptions>;

#endif // !NO_QUILL

#include <string>
#include <windows.h>
#include <source.h>
#include <dvdmedia.h>

#include "LibMWCapture/MWCapture.h"

EXTERN_C const GUID CLSID_MWCAPTURE_FILTER;
EXTERN_C const AMOVIESETUP_PIN sMIPPins[];

struct HDR_META 
{
    bool exists = false;
    int r_primary_x = 0;
    int r_primary_y = 0;
    int g_primary_x = 0;
    int g_primary_y = 0;
    int b_primary_x = 0;
    int b_primary_y = 0;
    int whitepoint_x = 0;
    int whitepoint_y = 0;
    int minDML = 0;
    int maxDML = 0;
    int maxCLL = 0;
    int maxFALL = 0;
    int transferFunction = 4;
};

struct VIDEO_SIGNAL
{
    MWCAP_INPUT_SPECIFIC_STATUS inputStatus;
    MWCAP_VIDEO_SIGNAL_STATUS signalStatus;
    MWCAP_VIDEO_BUFFER_INFO bufferInfo;
    MWCAP_VIDEO_FRAME_INFO frameInfo;
    MWCAP_VIDEO_CAPTURE_STATUS captureStatus;
    HDMI_HDR_INFOFRAME_PAYLOAD hdrInfo;
    HDMI_AVI_INFOFRAME_PAYLOAD aviInfo;
};

struct VIDEO_FORMAT
{
    MWCAP_VIDEO_COLOR_FORMAT colourFormat = MWCAP_VIDEO_COLOR_FORMAT_YUV709;
    HDMI_PXIEL_ENCODING pixelEncoding = HDMI_ENCODING_YUV_420;
    byte bitDepth = 10;
    int cx = 3840;
    int cy = 2160;
    DWORD fps = 50;
    LONGLONG frameInterval = 200000;
    int aspectX = 16;
    int aspectY = 9;
    MWCAP_VIDEO_QUANTIZATION_RANGE quantization = MWCAP_VIDEO_QUANTIZATION_LIMITED;
    MWCAP_VIDEO_SATURATION_RANGE saturation = MWCAP_VIDEO_SATURATION_LIMITED;
    HDR_META hdrMeta;
    // derived from the above attributes
    byte bitCount;
    DWORD pixelStructure;
	std::string pixelStructureName;
	std::string colourFormatName;
    DWORD lineLength;
    DWORD imageSize;
};

struct AUDIO_SIGNAL
{
    MWCAP_AUDIO_SIGNAL_STATUS signalStatus;
    MWCAP_AUDIO_CAPTURE_FRAME frameInfo;
};

struct AUDIO_FORMAT
{
    boolean pcm = true;
    DWORD fs = 48000;
    double sampleInterval = 10000000.0 / 48000;
    BYTE bitDepth = 16;
    BYTE bitDepthInBytes = 2;
    WORD channelCount = 2;
    WORD channelMask = KSAUDIO_SPEAKER_STEREO;
};

class MWReferenceClock final :
    public CBaseReferenceClock
{
    HCHANNEL mChannel;

public:
    MWReferenceClock(HRESULT* phr, HCHANNEL p_hchannel)
        : CBaseReferenceClock(L"MWReferenceClock", nullptr, phr, nullptr)
    {
        mChannel = p_hchannel;
    }

    REFERENCE_TIME GetPrivateTime() override
    {
        REFERENCE_TIME t;
        MWGetDeviceTime(mChannel, &t);
        return t;
    }
};


/**
 * Directshow filter which can uses the MWCapture SDK to receive video and audio from a HDMI capture card.
 * Can inject HDR/WCG data if found on the incoming HDMI stream.
 */
class MagewellCaptureFilter final :
	public CSource, public IReferenceClock, public IAMFilterMiscFlags
{
	HCHANNEL mChannel;

public:

    DECLARE_IUNKNOWN;

    // Provide the way for COM to create a Filter object
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;

    HCHANNEL GetChannelHandle() const;


private:

    // Constructor
    MagewellCaptureFilter(LPUNKNOWN punk, HRESULT* phr);
    ~MagewellCaptureFilter() override;

public:

    //////////////////////////////////////////////////////////////////////////
    //  IReferenceClock
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetTime(REFERENCE_TIME* pTime) override;
	HRESULT AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HEVENT hEvent,
		DWORD_PTR* pdwAdviseCookie) override;
	HRESULT AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HSEMAPHORE hSemaphore,
		DWORD_PTR* pdwAdviseCookie) override;
	HRESULT Unadvise(DWORD_PTR dwAdviseCookie) override;

    //////////////////////////////////////////////////////////////////////////
    //  IAMFilterMiscFlags
    //////////////////////////////////////////////////////////////////////////
	ULONG GetMiscFlags() override;

    //////////////////////////////////////////////////////////////////////////
    //  IMediaFilter
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP GetState(DWORD dw, FILTER_STATE* pState) override;
    STDMETHODIMP SetSyncSource(IReferenceClock* pClock) override;
    STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName) override;
    STDMETHODIMP Run(REFERENCE_TIME tStart) override;
    STDMETHODIMP Pause() override;
    STDMETHODIMP Stop() override;

private:
	BOOL mInited;
    MWReferenceClock* mClock;

    int mValidChannel[32] = { -1 };
    int mValidChannelCount = 0;

    int mBoardId = -1;
    int mChannelId = -1;

#ifndef NO_QUILL
    std::string mLogPrefix = "MagewellCaptureFilter";
    CustomLogger* mLogger;
#endif
};

/**
 * A stream of audio or video flowing from the capture device to an output pin.
 */
class MagewellCapturePin :
	public CSourceStream, public IAMStreamConfig, public IKsPropertySet, public IAMPushSource, public CBaseStreamControl
{
public:
    MagewellCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, LPCSTR pObjectName, LPCWSTR pPinName, std::string pLogPrefix);

    DECLARE_IUNKNOWN;

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;

    void SetStartTime(LONGLONG streamStartTime);

    //////////////////////////////////////////////////////////////////////////
    //  IPin
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP BeginFlush(void) override;
    STDMETHODIMP EndFlush(void) override;

    //////////////////////////////////////////////////////////////////////////
    //  IQualityControl
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Notify(IBaseFilter* pSelf, Quality q) override;

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE* pmt) override;
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE** ppmt) override;

    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties) override;
    HRESULT SetMediaType(const CMediaType* pmt) override;
    HRESULT OnThreadDestroy(void) override;
    HRESULT OnThreadStartPlay(void) override;

    //////////////////////////////////////////////////////////////////////////
    //  CBaseStreamControl
    //////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData) override;
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData, DWORD* pcbReturned) override;
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport) override;

    //////////////////////////////////////////////////////////////////////////
    //  IAMPushSource
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetPushSourceFlags(ULONG* pFlags) override
    {
        *pFlags = 0;
        return S_OK;
    }

    HRESULT GetLatency(REFERENCE_TIME* prtLatency) override { return E_NOTIMPL; }
    HRESULT SetPushSourceFlags(ULONG Flags) override { return E_NOTIMPL; }
    HRESULT SetStreamOffset(REFERENCE_TIME rtOffset) override { return E_NOTIMPL; }
    HRESULT GetStreamOffset(REFERENCE_TIME* prtOffset) override { return E_NOTIMPL; }
    HRESULT GetMaxStreamOffset(REFERENCE_TIME* prtMaxOffset) override { return E_NOTIMPL; }
    HRESULT SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset) override { return E_NOTIMPL; }

protected:
    virtual void StopCapture() = 0;
    virtual bool ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties) = 0;
    HRESULT RenegotiateMediaType(const CMediaType* pmt, int oldSize, int newSize);
    HRESULT HandleStreamStateChange(IMediaSample* pms);

#ifndef NO_QUILL
    std::string mLogPrefix;
    CustomLogger* mLogger;
#endif

    LONGLONG mFrameCounter;
    bool mPreview;
    MagewellCaptureFilter* mFilter;
    WORD mSinceLastLog;
    LONGLONG mStreamStartTime;

    // Common - temp 
    HNOTIFY mNotify;
    ULONGLONG mStatusBits = 0;
    HANDLE mCaptureEvent;
    HANDLE mNotifyEvent;
    MW_RESULT mLastMwResult;
    boolean mLastSampleDiscarded;
    boolean mSendMediaType;
    // per frame
    LONGLONG mFrameEndTime;
};


/**
 * A video stream flowing from the capture device to an output pin.
 */
class MagewellVideoCapturePin final :
    public MagewellCapturePin
{
public:
    MagewellVideoCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, bool pPreview);

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int* piCount, int* piSize) override;
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC) override;

    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    HRESULT FillBuffer(IMediaSample* pms) override;
    HRESULT GetMediaType(CMediaType* pmt) override;
    HRESULT OnThreadCreate(void) override;

protected:
    VIDEO_SIGNAL mVideoSignal = {};
    VIDEO_FORMAT mVideoFormat = {};
    boolean mHasHdrInfoFrame;

    // Encapsulates pinning the IMediaSample buffer into video memory (and unpinning on destruct)
    class VideoFrameGrabber
    {
    public:
        VideoFrameGrabber(MagewellVideoCapturePin* pin, HCHANNEL channel, IMediaSample* pms);
        ~VideoFrameGrabber();

        VideoFrameGrabber(VideoFrameGrabber const&) = delete;
        VideoFrameGrabber& operator =(VideoFrameGrabber const&) = delete;
        VideoFrameGrabber(VideoFrameGrabber&&) = delete;
        VideoFrameGrabber& operator=(VideoFrameGrabber&&) = delete;

        HRESULT grab();

    private:
        HCHANNEL channel;
        MagewellVideoCapturePin* pin;
        IMediaSample* pms;
        BYTE* pmsData;
    };

    static void LoadFormat(VIDEO_FORMAT* videoFormat, VIDEO_SIGNAL* videoSignal);
    static void LoadHdrMeta(HDR_META* meta, HDMI_HDR_INFOFRAME_PAYLOAD* frame);
    void VideoFormatToMediaType(CMediaType* pmt, VIDEO_FORMAT* videoFormat) const;
    bool ShouldChangeMediaType();
    HRESULT LoadSignal(HCHANNEL* pChannel);
    HRESULT DoChangeMediaType(const CMediaType* pmt, const VIDEO_FORMAT* newVideoFormat);
    void StopCapture() override;
    bool ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties) override;
};

/**
 * An audio stream flowing from the capture device to an output pin.
 */
class MagewellAudioCapturePin final :
    public MagewellCapturePin
{
public:
    MagewellAudioCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent);

    //////////////////////////////////////////////////////////////////////////
    //  CBaseOutputPin
    //////////////////////////////////////////////////////////////////////////
    HRESULT DecideAllocator(IMemInputPin* pPin, __deref_out IMemAllocator** pAlloc) override;
    HRESULT InitAllocator(__deref_out IMemAllocator** ppAlloc) override;
    HRESULT GetDeliveryBuffer(__deref_out IMediaSample** ppSample, __in_opt REFERENCE_TIME* pStartTime, __in_opt REFERENCE_TIME* pEndTime, DWORD dwFlags) override;

	//////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int* piCount, int* piSize) override;
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC) override;

    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetMediaType(CMediaType* pmt) override;
    HRESULT OnThreadCreate(void) override;
    HRESULT FillBuffer(IMediaSample* pms) override;

protected:
    AUDIO_SIGNAL mAudioSignal = {};
    AUDIO_FORMAT mAudioFormat = {};

    static void LoadFormat(AUDIO_FORMAT* audioFormat, AUDIO_SIGNAL* audioSignal);
    static void AudioFormatToMediaType(CMediaType* pmt, AUDIO_FORMAT* audioFormat);
    bool ShouldChangeMediaType(AUDIO_FORMAT* newAudioFormat);
    HRESULT DoChangeMediaType(const CMediaType* pmt, const AUDIO_FORMAT* newAudioFormat);
    void StopCapture() override;
    bool ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties) override;
};

class MemAllocator final : public CMemAllocator
{
public:
    MemAllocator(__inout_opt LPUNKNOWN, __inout HRESULT*);
};