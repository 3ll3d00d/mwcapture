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

#include <functional>
#include <atlcomcli.h>

#include "capture.h"
#include "DeckLinkAPI_h.h"

EXTERN_C const GUID CLSID_BMCAPTURE_FILTER;
EXTERN_C const GUID MEDIASUBTYPE_PCM_IN24;
EXTERN_C const GUID MEDIASUBTYPE_PCM_IN32;
EXTERN_C const GUID MEDIASUBTYPE_PCM_SOWT;
EXTERN_C const AMOVIESETUP_PIN sMIPPins[];

const std::function DeleteString = SysFreeString;

const std::function BSTRToStdString = [](BSTR dl_str) -> std::string
{
	int wlen = ::SysStringLen(dl_str);
	int mblen = WideCharToMultiByte(CP_ACP, 0, dl_str, wlen, nullptr, 0, nullptr, nullptr);

	std::string ret_str(mblen, '\0');
	mblen = WideCharToMultiByte(CP_ACP, 0, dl_str, wlen, &ret_str[0], mblen, nullptr, nullptr);

	return ret_str;
};

const std::function StdStringToBSTR = [](std::string std_str) -> BSTR
{
	int wlen = MultiByteToWideChar(CP_ACP, 0, std_str.data(), static_cast<int>(std_str.length()), nullptr, 0);

	BSTR ret_str = ::SysAllocStringLen(nullptr, wlen);
	MultiByteToWideChar(CP_ACP, 0, std_str.data(), static_cast<int>(std_str.length()), ret_str, wlen);

	return ret_str;
};

inline bool isInCieRange(double value)
{
	return value >= 0 && value <= 1.1;
}

constexpr int64_t invalidFrameTime = std::numeric_limits<int64_t>::lowest();

struct DEVICE_INFO
{
	std::string name{};
	int apiVersion[3]{0, 0, 0};
	int64_t audioChannelCount{0};
	bool inputFormatDetection{false};
	bool hdrMetadata{false};
	bool colourspaceMetadata{false};
	bool dynamicRangeMetadata{false};
};

struct VIDEO_SIGNAL
{
	BMDPixelFormat pixelFormat{bmdFormat10BitYUV};
	BMDDisplayMode displayMode{bmdMode4K2160p2398};
	std::string displayModeName{"4K2160p23.98"};
	uint32_t frameDuration{24000};
	uint16_t frameDurationScale{1001};
	uint16_t cx{3840};
	uint16_t cy{2160};
};

struct AUDIO_SIGNAL
{
};

class VideoFrame
{
public:
	VideoFrame(VIDEO_FORMAT format, int64_t time, uint64_t index, long rowSize, const CComQIPtr<IDeckLinkVideoBuffer>& buffer) :
		mFormat(std::move(format)),
		mFrameTime(time),
		mFrameIndex(index),
		mBuffer(buffer)
	{
		mBuffer->StartAccess(bmdBufferAccessRead);
		mBuffer->GetBytes(&mFrameData);
		mLength = rowSize * mFormat.cy;
	}

	VideoFrame(const VideoFrame& vf):
		mFormat(vf.mFormat),
		mFrameTime(vf.mFrameTime),
		mFrameIndex(vf.mFrameIndex),
		mBuffer(vf.mBuffer),
		mLength(vf.mLength)
	{
		mBuffer->StartAccess(bmdBufferAccessRead);
		mBuffer->GetBytes(&mFrameData);
	}

	~VideoFrame()
	{
		mBuffer->EndAccess(bmdBufferAccessRead);
	}

	void* GetData() const { return mFrameData; }

	uint64_t GetFrameIndex() const { return mFrameIndex; }

	int64_t GetFrameTime() const { return mFrameTime; }

	VIDEO_FORMAT GetVideoFormat() const { return mFormat; }

	long GetLength() const { return mLength; }

private:
	VIDEO_FORMAT mFormat{};
	int64_t mFrameTime{0};
	uint64_t mFrameIndex{0};
	long mLength{ 0 };
	CComQIPtr<IDeckLinkVideoBuffer> mBuffer;
	void* mFrameData = nullptr;
};

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
			t = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		}
		return t;
	}
};

/**
 * DirectShow filter which can uses the Blackmagic SDK to receive video and audio from a connected HDMI capture card.
 * Can inject HDR/WCG data if found on the incoming HDMI stream.
 */
class BlackmagicCaptureFilter final :
	public HdmiCaptureFilter<DEVICE_INFO, VIDEO_SIGNAL, AUDIO_SIGNAL>,
	public IDeckLinkInputCallback,
	public IDeckLinkNotificationCallback
{
public:
	// Provide the way for COM to create a Filter object
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

	// Callbacks to update the prop page data
	void OnVideoSignalLoaded(VIDEO_SIGNAL* vs) override;
	void OnAudioSignalLoaded(AUDIO_SIGNAL* as) override;
	void OnDeviceSelected() override;

	HRESULT Reload() override;

	// filter <-> pin communication
	HRESULT PinThreadCreated();
	HRESULT PinThreadDestroyed();

	//////////////////////////////////////////////////////////////////////////
	//  IDeckLinkInputCallback
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
	                                                  IDeckLinkDisplayMode* newDisplayMode,
	                                                  BMDDetectedVideoInputFormatFlags detectedSignalFlags) override;

	HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame,
	                                                 IDeckLinkAudioInputPacket* audioPacket) override;

	//////////////////////////////////////////////////////////////////////////
	//  IDeckLinkNotificationCallback
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Notify(BMDNotifications topic, ULONGLONG param1, ULONGLONG param2);

	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	HRESULT QueryInterface(const IID& riid, void** ppvObject) override;

	ULONG AddRef() override
	{
		return CaptureFilter::AddRef();
	}

	ULONG Release() override
	{
		return CaptureFilter::Release();
	}

	HANDLE GetVideoFrameHandle() const;
	std::shared_ptr<VideoFrame> GetVideoFrame()
	{
		return mCurrentFrame;
	}

private:
	// Constructor
	BlackmagicCaptureFilter(LPUNKNOWN punk, HRESULT* phr);
	~BlackmagicCaptureFilter() override;

	CComPtr<IDeckLink> mDeckLink;
	CComQIPtr<IDeckLinkInput> mDeckLinkInput;
	CComQIPtr<IDeckLinkNotification> mDeckLinkNotification;
	CComQIPtr<IDeckLinkStatus> mDeckLinkStatus;
	CComQIPtr<IDeckLinkHDMIInputEDID> mDeckLinkHDMIInputEDID;

	uint8_t mRunningPins{0};
	VIDEO_SIGNAL mVideoSignal{};
	VIDEO_FORMAT mVideoFormat{};
	int64_t mPreviousVideoFrameTime{invalidFrameTime};
	uint64_t mCapturedVideoFrameCount{0};
	std::shared_ptr<VideoFrame> mCurrentFrame;
	HANDLE mVideoFrameEvent;
};


/**
 * A video stream flowing from the capture device to an output pin.
 */
class BlackmagicVideoCapturePin final :
	public HdmiVideoCapturePin<BlackmagicCaptureFilter>
{
public:
	BlackmagicVideoCapturePin(HRESULT* phr, BlackmagicCaptureFilter* pParent, bool pPreview);
	~BlackmagicVideoCapturePin() override;

	//////////////////////////////////////////////////////////////////////////
	//  CBaseOutputPin
	//////////////////////////////////////////////////////////////////////////
	HRESULT GetDeliveryBuffer(__deref_out IMediaSample** ppSample, __in_opt REFERENCE_TIME* pStartTime,
	                          __in_opt REFERENCE_TIME* pEndTime, DWORD dwFlags) override;

	//////////////////////////////////////////////////////////////////////////
	//  CSourceStream
	//////////////////////////////////////////////////////////////////////////
	HRESULT FillBuffer(IMediaSample* pms) override;
	HRESULT OnThreadCreate(void) override;

protected:
	void DoThreadDestroy() override;

	void LogHdrMetaIfPresent(VIDEO_FORMAT* newVideoFormat);
	HRESULT DoChangeMediaType(const CMediaType* pmt, const VIDEO_FORMAT* newVideoFormat);

	VIDEO_FORMAT mVideoFormat{};
	std::shared_ptr<VideoFrame> mCurrentFrame;
};

/**
 * An audio stream flowing from the capture device to an output pin.
 */
class BlackmagicAudioCapturePin final :
	public HdmiAudioCapturePin<BlackmagicCaptureFilter>
{
public:
	BlackmagicAudioCapturePin(HRESULT* phr, BlackmagicCaptureFilter* pParent, bool pPreview);
	~BlackmagicAudioCapturePin() override;

	//////////////////////////////////////////////////////////////////////////
	//  CBaseOutputPin
	//////////////////////////////////////////////////////////////////////////
	HRESULT GetDeliveryBuffer(__deref_out IMediaSample** ppSample, __in_opt REFERENCE_TIME* pStartTime,
	                          __in_opt REFERENCE_TIME* pEndTime, DWORD dwFlags) override;

	//////////////////////////////////////////////////////////////////////////
	//  CSourceStream
	//////////////////////////////////////////////////////////////////////////
	HRESULT OnThreadCreate(void) override;
	HRESULT FillBuffer(IMediaSample* pms) override;

protected:
	AUDIO_SIGNAL mAudioSignal{};

	void LoadFormat(AUDIO_FORMAT* audioFormat, const AUDIO_SIGNAL* audioSignal) const;
	HRESULT DoChangeMediaType(const CMediaType* pmt, const AUDIO_FORMAT* newAudioFormat);
	bool ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties) override;
	void DoThreadDestroy() override;
};
