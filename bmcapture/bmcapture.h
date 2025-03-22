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
#include <quill/Logger.h>
#include <quill/LogMacros.h>
#include "quill/Frontend.h"

struct CustomFrontendOptions
{
    static constexpr quill::QueueType queue_type{ quill::QueueType::BoundedDropping };
    static constexpr uint32_t initial_queue_capacity{ 64 * 1024 * 1024 }; // 64MiB
    static constexpr uint32_t blocking_queue_retry_interval_ns{ 800 };
    static constexpr bool huge_pages_enabled{ false };
};
using CustomFrontend = quill::FrontendImpl<CustomFrontendOptions>;
using CustomLogger = quill::LoggerImpl<CustomFrontendOptions>;
#else
#include <vector>
#include <chrono>
#endif // !NO_QUILL

#include "signalinfo.h"
#include "lavfilters_side_data.h"
#include "ISpecifyPropertyPages2.h"

class BMReferenceClock final :
    public CBaseReferenceClock
{

public:
    BMReferenceClock(HRESULT* phr)
        : CBaseReferenceClock(L"BMReferenceClock", nullptr, phr, nullptr)
    {
    }

    REFERENCE_TIME GetPrivateTime() override
    {
        REFERENCE_TIME t;
        if (false)
        {
            // TODO use BM API
            // MWGetDeviceTime(mChannel, &t);
        }
        else
        {
            t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        }
        return t;
    }
};

/**
 * Directshow filter which can uses the Blackmagic SDK to receive video and audio from a connected HDMI capture card.
 * Can inject HDR/WCG data if found on the incoming HDMI stream.
 */
class BlackmagicCaptureFilter final :
    public CSource,
    public IReferenceClock,
    public IAMFilterMiscFlags,
    public ISignalInfo,
    public ISpecifyPropertyPages2
{
public:

    DECLARE_IUNKNOWN;

    // Provide the way for COM to create a Filter object
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;

    // HCHANNEL GetChannelHandle() const;

    // DeviceType GetDeviceType() const;

    void GetReferenceTime(REFERENCE_TIME* rt) const;

    // Callbacks to update the prop page data
    // void OnVideoSignalLoaded(VIDEO_SIGNAL* vs);
    // void OnVideoFormatLoaded(VIDEO_FORMAT* vf);
    void OnHdrUpdated(MediaSideDataHDR* hdr, MediaSideDataHDRContentLightLevel* light);
    // void OnAudioSignalLoaded(AUDIO_SIGNAL* as);
    // void OnAudioFormatLoaded(AUDIO_FORMAT* af);
    void OnDeviceSelected();

private:

    // Constructor
    BlackmagicCaptureFilter(LPUNKNOWN punk, HRESULT* phr);
    ~BlackmagicCaptureFilter() override;

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

    //////////////////////////////////////////////////////////////////////////
    //  ISignalInfo
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Reload() override;
    STDMETHODIMP SetCallback(ISignalInfoCB* cb) override;

    //////////////////////////////////////////////////////////////////////////
    //  ISpecifyPropertyPages2
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP GetPages(CAUUID* pPages) override;
    STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage) override;

private:
    // DEVICE_INFO mDeviceInfo{};
    BOOL mInited;
    // MWReferenceClock* mClock;
    // DEVICE_STATUS mDeviceStatus{};
    // AUDIO_INPUT_STATUS mAudioInputStatus{};
    // AUDIO_OUTPUT_STATUS mAudioOutputStatus{};
    // VIDEO_INPUT_STATUS mVideoInputStatus{};
    // VIDEO_OUTPUT_STATUS mVideoOutputStatus{};
    // HDR_STATUS mHdrStatus{};
    ISignalInfoCB* mInfoCallback = nullptr;

    #ifndef NO_QUILL
    std::string mLogPrefix = "BlackmagicCaptureFilter";
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
    ~MagewellCapturePin();

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
    HRESULT DoBufferProcessingLoop(void) override;

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
    HRESULT RenegotiateMediaType(const CMediaType* pmt, long newSize, boolean renegotiateOnQueryAccept);
    HRESULT HandleStreamStateChange(IMediaSample* pms);

    #ifndef NO_QUILL
    std::string mLogPrefix;
    CustomLogger* mLogger;
    #endif

    CCritSec mCaptureCritSec;
    LONGLONG mFrameCounter;
    bool mPreview;
    MagewellCaptureFilter* mFilter;
    WORD mSinceLast{ 0 };
    LONGLONG mStreamStartTime;

    // Common - temp 
    HNOTIFY mNotify;
    ULONGLONG mStatusBits = 0;
    HANDLE mNotifyEvent;
    MW_RESULT mLastMwResult;
    boolean mLastSampleDiscarded;
    boolean mSendMediaType;
    boolean mHasSignal;
    LONGLONG mLastSentHdrMetaAt;
    // per frame
    LONGLONG mFrameEndTime;
    // pro only
    HANDLE mCaptureEvent;
};


/**
 * A video stream flowing from the capture device to an output pin.
 */
class MagewellVideoCapturePin :
    public MagewellCapturePin
{
public:
    MagewellVideoCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, bool pPreview);

    void GetReferenceTime(REFERENCE_TIME* rt) const;

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int* piCount, int* piSize) override;
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC) override;

    //////////////////////////////////////////////////////////////////////////
    //  CBaseOutputPin
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetDeliveryBuffer(__deref_out IMediaSample** ppSample, __in_opt REFERENCE_TIME* pStartTime, __in_opt REFERENCE_TIME* pEndTime, DWORD dwFlags) override;

    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    HRESULT FillBuffer(IMediaSample* pms) override;
    HRESULT GetMediaType(CMediaType* pmt) override;
    HRESULT OnThreadCreate(void) override;

protected:
    // Encapsulates pinning the IMediaSample buffer into video memory (and unpinning on destruct)
    class VideoFrameGrabber
    {
    public:
        VideoFrameGrabber(MagewellVideoCapturePin* pin, HCHANNEL hChannel, DeviceType deviceType, IMediaSample* pms);
        ~VideoFrameGrabber();

        VideoFrameGrabber(VideoFrameGrabber const&) = delete;
        VideoFrameGrabber& operator =(VideoFrameGrabber const&) = delete;
        VideoFrameGrabber(VideoFrameGrabber&&) = delete;
        VideoFrameGrabber& operator=(VideoFrameGrabber&&) = delete;

        HRESULT grab() const;

    private:
        HCHANNEL hChannel;
        DeviceType deviceType;
        MagewellVideoCapturePin* pin;
        IMediaSample* pms;
        BYTE* pmsData;
    };

    // USB only
    class VideoCapture
    {
    public:
        VideoCapture(MagewellVideoCapturePin* pin, HCHANNEL hChannel);
        ~VideoCapture();

    private:
        MagewellVideoCapturePin* pin;
        HANDLE mEvent;
    };

    VIDEO_SIGNAL mVideoSignal{};
    VIDEO_FORMAT mVideoFormat{};
    USB_CAPTURE_FORMATS mUsbCaptureFormats{};
    boolean mHasHdrInfoFrame{ false };
    // USB only
    VideoCapture* mVideoCapture{ nullptr };
    CAPTURED_FRAME mCapturedFrame{};

    static void LoadFormat(VIDEO_FORMAT* videoFormat, VIDEO_SIGNAL* videoSignal, USB_CAPTURE_FORMATS* captureFormats);
    // USB only
    static void CaptureFrame(BYTE* pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam);

    void LogHdrMetaIfPresent(VIDEO_FORMAT* newVideoFormat);
    void VideoFormatToMediaType(CMediaType* pmt, VIDEO_FORMAT* videoFormat) const;
    bool ShouldChangeMediaType(VIDEO_FORMAT* newVideoFormat);
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
    MagewellAudioCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, bool pPreview);
    ~MagewellAudioCapturePin() override;

    void CopyToBitstreamBuffer(BYTE* buf);
    HRESULT ParseBitstreamBuffer(uint16_t bufSize, enum Codec** codec);
    HRESULT GetCodecFromIEC61937Preamble(enum IEC61937DataType dataType, uint16_t* burstSize, enum Codec* codec);

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
    class AudioCapture
    {
    public:
        AudioCapture(MagewellAudioCapturePin* pin, HCHANNEL hChannel);
        ~AudioCapture();

    private:
        MagewellAudioCapturePin* pin;
        HANDLE mEvent;
    };

    double minus_10db{ pow(10.0, -10.0 / 20.0) };
    AUDIO_SIGNAL mAudioSignal{};
    AUDIO_FORMAT mAudioFormat{};
    BYTE mFrameBuffer[maxFrameLengthInBytes];
    // IEC61937 processing
    uint32_t mBitstreamDetectionWindowLength{ 0 };
    uint8_t mPaPbBytesRead{ 0 };
    BYTE mPcPdBuffer[4];
    uint8_t mPcPdBytesRead{ 0 };
    uint16_t mDataBurstFrameCount{ 0 };
    uint16_t mDataBurstRead{ 0 };
    uint16_t mDataBurstSize{ 0 };
    uint16_t mDataBurstPayloadSize{ 0 };
    uint32_t mBytesSincePaPb{ 0 };
    uint64_t mSinceCodecChange{ 0 };
    bool mPacketMayBeCorrupt{ false };
    BYTE mCompressedBuffer[maxFrameLengthInBytes];
    std::vector<BYTE> mDataBurstBuffer; // variable size
    AudioCapture* mAudioCapture{ nullptr };
    CAPTURED_FRAME mCapturedFrame{};

    #ifdef RECORD_RAW
    char mRawFileName[MAX_PATH];
    FILE* mRawFile;
    #endif
    #ifdef RECORD_ENCODED
    char mEncodedInFileName[MAX_PATH];
    FILE* mEncodedInFile;
    char mEncodedOutFileName[MAX_PATH];
    FILE* mEncodedOutFile;
    #endif
    // TODO remove after SDK bug is fixed
    Codec mDetectedCodec{ PCM };
    bool mProbeOnTimer{ false };

    static void AudioFormatToMediaType(CMediaType* pmt, AUDIO_FORMAT* audioFormat);
    static void CaptureFrame(const BYTE* pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam);

    void LoadFormat(AUDIO_FORMAT* audioFormat, const AUDIO_SIGNAL* audioSignal) const;
    HRESULT LoadSignal(HCHANNEL* hChannel);
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