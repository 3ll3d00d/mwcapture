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

constexpr auto not_present = 1024;

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

#include <string>
#include <windows.h>
#include <source.h>
#include <dvdmedia.h>
#include <array>
#include <wmcodecdsp.h>

#include "LibMWCapture/MWCapture.h"

// HDMI Audio Bitstream Codec Identification metadata

// IEC 61937-1 Chapter 6.1.7 Field Pa
constexpr auto IEC61937_SYNCWORD_1 = 0xF872;
// IEC 61937-1 Chapter 6.1.7 Field Pb
constexpr auto IEC61937_SYNCWORD_2 = 0x4E1F;
// IEC 61937-2 Table 2
enum IEC61937DataType : uint8_t
{
    IEC61937_NULL               = 0x0,           ///< NULL
    IEC61937_AC3                = 0x01,          ///< AC-3 data
    IEC61937_PAUSE              = 0x03,          ///< Pause
    IEC61937_MPEG1_LAYER1       = 0x04,          ///< MPEG-1 layer 1
    IEC61937_MPEG1_LAYER23      = 0x05,          ///< MPEG-1 layer 2 or 3 data or MPEG-2 without extension
    IEC61937_MPEG2_EXT          = 0x06,          ///< MPEG-2 data with extension
    IEC61937_MPEG2_AAC          = 0x07,          ///< MPEG-2 AAC ADTS
    IEC61937_MPEG2_LAYER1_LSF   = 0x08,          ///< MPEG-2, layer-1 low sampling frequency
    IEC61937_MPEG2_LAYER2_LSF   = 0x09,          ///< MPEG-2, layer-2 low sampling frequency
    IEC61937_MPEG2_LAYER3_LSF   = 0x0A,          ///< MPEG-2, layer-3 low sampling frequency
    IEC61937_DTS1               = 0x0B,          ///< DTS type I   (512 samples)
    IEC61937_DTS2               = 0x0C,          ///< DTS type II  (1024 samples)
    IEC61937_DTS3               = 0x0D,          ///< DTS type III (2048 samples)
    IEC61937_ATRAC              = 0x0E,          ///< ATRAC data
    IEC61937_ATRAC3             = 0x0F,          ///< ATRAC3 data
    IEC61937_ATRACX             = 0x10,          ///< ATRAC3+ data
    IEC61937_DTSHD              = 0x11,          ///< DTS HD data
    IEC61937_WMAPRO             = 0x12,          ///< WMA 9 Professional data
    IEC61937_MPEG2_AAC_LSF_2048 = 0x13,          ///< MPEG-2 AAC ADTS half-rate low sampling frequency
    IEC61937_MPEG2_AAC_LSF_4096 = 0x13 | 0x20,   ///< MPEG-2 AAC ADTS quarter-rate low sampling frequency
    IEC61937_EAC3               = 0x15,          ///< E-AC-3 data
    IEC61937_TRUEHD             = 0x16,          ///< TrueHD/MAT data
};
enum Codec
{
    PCM,
    AC3,
    DTS,
    DTSHD,
    EAC3,
    TRUEHD,
    BITSTREAM,
    PAUSE_OR_NULL
};
static const std::string codecNames[8] = {
    "PCM",
    "AC3",
    "DTS",
    "DTSHD",
    "EAC3",
    "TrueHD",
    "Unidentified",
	"PAUSE_OR_NULL"
};
constexpr int maxBitDepthInBytes = sizeof(DWORD);
constexpr int maxFrameLengthInBytes = MWCAP_AUDIO_SAMPLES_PER_FRAME * MWCAP_AUDIO_MAX_NUM_CHANNELS * maxBitDepthInBytes;

EXTERN_C const GUID CLSID_MWCAPTURE_FILTER;
EXTERN_C const GUID MEDIASUBTYPE_PCM_IN24;
EXTERN_C const GUID MEDIASUBTYPE_PCM_IN32;
EXTERN_C const GUID MEDIASUBTYPE_PCM_SOWT;
EXTERN_C const AMOVIESETUP_PIN sMIPPins[];

struct HDR_META 
{
    bool exists{ false };
    int r_primary_x{ 0 };
    int r_primary_y{ 0 };
    int g_primary_x{ 0 };
    int g_primary_y{ 0 };
    int b_primary_x{ 0 };
    int b_primary_y{ 0 };
    int whitepoint_x{ 0 };
    int whitepoint_y{ 0 };
    int minDML{ 0 };
    int maxDML{ 0 };
    int maxCLL{ 0 };
    int maxFALL{ 0 };
    int transferFunction{ 4 };
};

struct USB_CAPTURE_FORMATS
{
    bool usb{ false };
    MWCAP_VIDEO_OUTPUT_FOURCC fourccs;
    MWCAP_VIDEO_OUTPUT_FRAME_INTERVAL frameIntervals;
    MWCAP_VIDEO_OUTPUT_FRAME_SIZE frameSizes;
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
    MWCAP_VIDEO_COLOR_FORMAT colourFormat{ MWCAP_VIDEO_COLOR_FORMAT_YUV709 };
    HDMI_PXIEL_ENCODING pixelEncoding{ HDMI_ENCODING_YUV_420 };
    byte bitDepth{ 10 };
    int cx{ 3840 };
    int cy{ 2160 };
    DWORD fps{ 50 };
    LONGLONG frameInterval{ 200000 };
    int aspectX{ 16 };
    int aspectY{ 9 };
    MWCAP_VIDEO_QUANTIZATION_RANGE quantization{ MWCAP_VIDEO_QUANTIZATION_LIMITED };
    MWCAP_VIDEO_SATURATION_RANGE saturation{ MWCAP_VIDEO_SATURATION_LIMITED };
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
    HDMI_AUDIO_INFOFRAME_PAYLOAD audioInfo;
};

struct AUDIO_FORMAT
{
    boolean pcm{ true };
    DWORD fs{ 48000 };
    double sampleInterval{ 10000000.0 / 48000 };
    BYTE bitDepth{ 16 };
    BYTE bitDepthInBytes{ 2 };
    BYTE channelAllocation{ 0x00 };
    WORD channelValidityMask{ 0 };
    WORD inputChannelCount{ 2 };
    WORD outputChannelCount{ 2 };
    std::array<int, 8> channelOffsets{ 0, 0, not_present, not_present, not_present, not_present, not_present, not_present };
    WORD channelMask{ KSAUDIO_SPEAKER_STEREO };
    int lfeChannelIndex{ not_present };
    double lfeLevelAdjustment{ 1.0 };
    Codec codec{ PCM };
    // encoded content only
    uint16_t dataBurstSize{ 0 };
};

enum DeviceType : uint8_t
{
    USB,
    PRO
};

inline const char* devicetype_to_name(DeviceType e)
{
    switch (e)
    {
    case USB: return "USB";
    case PRO: return "PRO";
    default: return "unknown";
    }
}

struct DEVICE_INFO
{
    DeviceType deviceType;
    std::string serialNo;
    WCHAR devicePath[128];
    HCHANNEL hChannel;
};

struct CAPTURED_FRAME
{
    BYTE* data;
    int length;
    UINT64 ts;
};

class MWReferenceClock final :
    public CBaseReferenceClock
{
    HCHANNEL mChannel;
    bool mIsPro;

public:
    MWReferenceClock(HRESULT* phr, HCHANNEL hChannel, bool isProDevice)
        : CBaseReferenceClock(L"MWReferenceClock", nullptr, phr, nullptr),
    mChannel(hChannel),
    mIsPro(isProDevice)
    {
    }

    REFERENCE_TIME GetPrivateTime() override
    {
        REFERENCE_TIME t;
        if (mIsPro)
        {
            MWGetDeviceTime(mChannel, &t);
        }
        else
        {
            t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        }
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
public:

    DECLARE_IUNKNOWN;

    // Provide the way for COM to create a Filter object
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;

    HCHANNEL GetChannelHandle() const;

    DeviceType GetDeviceType() const;

    void GetReferenceTime(REFERENCE_TIME* rt) const;

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
    DEVICE_INFO mDeviceInfo;
    BOOL mInited;
    MWReferenceClock* mClock;

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
    WORD mSinceLast{0};
    LONGLONG mStreamStartTime;

    // Common - temp 
    HNOTIFY mNotify;
    ULONGLONG mStatusBits = 0;
    HANDLE mNotifyEvent;
    MW_RESULT mLastMwResult;
    boolean mLastSampleDiscarded;
    boolean mSendMediaType;
    // per frame
    LONGLONG mFrameEndTime;
    // pro only
    HANDLE mCaptureEvent;
};


/**
 * A video stream flowing from the capture device to an output pin.
 */
class MagewellVideoCapturePin final :
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
    VideoCapture* mVideoCapture{nullptr};
    CAPTURED_FRAME mCapturedFrame{};

    static void LoadFormat(VIDEO_FORMAT* videoFormat, VIDEO_SIGNAL* videoSignal, USB_CAPTURE_FORMATS* captureFormats);
    static void LoadHdrMeta(HDR_META* meta, HDMI_HDR_INFOFRAME_PAYLOAD* frame);
    // USB only
    static void CaptureFrame(BYTE* pbFrame, int cbFrame, UINT64 u64TimeStamp, void* pParam);

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