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
#include <utility>
 // linking side data GUIDs fails without this
#include "mwcapture.h"

#include <initguid.h>

#include "lavfilters_side_data.h"
#include <cmath>

#define BACKOFF Sleep(20)
#define SHORT_BACKOFF Sleep(1)
// byte swap (big <=> little for a 16bit value)
#define BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))

constexpr auto unity = 1.0;
constexpr auto chromaticity_scale_factor = 0.00002;
constexpr auto high_luminance_scale_factor = 1.0;
constexpr auto low_luminance_scale_factor = 0.0001;

constexpr AMOVIESETUP_MEDIATYPE sVideoPinTypes =
{
	&MEDIATYPE_Video, // Major type
	&MEDIASUBTYPE_NULL // Minor type
};

constexpr AMOVIESETUP_MEDIATYPE sAudioPinTypes =
{
	&MEDIATYPE_Audio, // Major type
	&MEDIASUBTYPE_NULL // Minor type
};

constexpr AMOVIESETUP_PIN sVideoPin = {
	const_cast<LPWSTR>(L"Video"),
	FALSE, // Is it rendered
	TRUE, // Is it an output
	FALSE, // Are we allowed none
	FALSE, // And allowed many
	&CLSID_NULL, // Connects to filter
	nullptr, // Connects to pin
	1, // Number of types
	&sVideoPinTypes // Pin information
};

constexpr AMOVIESETUP_PIN sAudioPin = {
	const_cast<LPWSTR>(L"Audio"),
	FALSE, // Is it rendered
	TRUE, // Is it an output
	FALSE, // Are we allowed none
	FALSE, // And allowed many
	&CLSID_NULL, // Connects to filter
	nullptr, // Connects to pin
	1, // Number of types
	&sAudioPinTypes // Pin information
};

const AMOVIESETUP_PIN sMIPPins[] = { sVideoPin, sAudioPin };

constexpr AMOVIESETUP_FILTER sMIPSetup =
{
	&CLSID_MWCAPTURE_FILTER, // Filter CLSID
	L"MagewellCapture", // String name
	MERIT_DO_NOT_USE, // Filter merit
	2, // Number of pins
	sMIPPins // Pin information
};


// List of class IDs and creator functions for the class factory.
CFactoryTemplate g_Templates[] = {
	{
		L"MagewellCapture",
		&CLSID_MWCAPTURE_FILTER,
		MagewellCaptureFilter::CreateInstance,
		nullptr,
		&sMIPSetup
	}
};

int g_cTemplates = 1;


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
	MWCAP_CHANNEL_INFO channelInfo = { 0 };
	WCHAR path[128] = { 0 };
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

	new MagewellVideoCapturePin(phr, this, false);
	new MagewellVideoCapturePin(phr, this, true);
	new MagewellAudioCapturePin(phr, this);
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
	return mChannel;
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


//////////////////////////////////////////////////////////////////////////
// MagewellCapturePin
//////////////////////////////////////////////////////////////////////////
MagewellCapturePin::MagewellCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent, LPCSTR pObjectName,
	LPCWSTR pPinName, std::string pLogPrefix) :
	CSourceStream(pObjectName, phr, pParent, pPinName),
	mFrameCounter(0),
	mPreview(false),
	mFilter(pParent),
	mSinceLastLog(0),
	mStreamStartTime(0),
	mNotify(nullptr),
	mCaptureEvent(nullptr),
	mNotifyEvent(nullptr),
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
	LONGLONG now;
	MWGetDeviceTime(mFilter->GetChannelHandle(), &now);

	if (mStreamStartTime == 0)
	{
		LOG_WARNING(mLogger, "[{}] Pin worker thread starting at {} but stream not started yet", mLogPrefix, now);
	}
	else
	{
		LOG_WARNING(mLogger, "[{}] Pin worker thread starting at {}, stream started at ", mLogPrefix, now,
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
		StopCapture();
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
	LOG_TRACE_L3(mLogger, "[{}] SetMediaType ({})", mLogPrefix, hr);
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
		LOG_WARNING(mLogger, "[{}] MagewellCapturePin::DecideBufferSize failed to SetProperties result {}", mLogPrefix,
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

HRESULT MagewellCapturePin::RenegotiateMediaType(const CMediaType* pmt, int oldSize, int newSize)
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
			if (newSize == oldSize)
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
						LOG_WARNING(mLogger, "[{}] Allocator did not accept update to {} bytes {} buffers [{}]",
							props.cbBuffer, props.cBuffers, hr);
						#endif
					}
				}
				else
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Allocator did not commit update to {} bytes {} buffers [{}]",
						props.cbBuffer, props.cBuffers, hr);
					#endif
				}
			}
		}
	}
	else
	{
		#ifndef NO_QUILL
		LOG_WARNING(
			mLogger, "[{}] MagewellCapturePin::NegotiateMediaType Receive Connection failed (hr: {}); QueryAccept: {}",
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
		LOG_TRACE_L1(mLogger, "[{}] MagewellCapturePin::NegotiateMediaType failed {}", mLogPrefix, retVal);
		#endif

		SetMediaType(&oldMediaType);
	}

	return retVal;
}

//////////////////////////////////////////////////////////////////////////
//  MagewellVideoCapturePin::VideoFrameGrabber
//////////////////////////////////////////////////////////////////////////
MagewellVideoCapturePin::VideoFrameGrabber::VideoFrameGrabber(MagewellVideoCapturePin* pin, HCHANNEL channel,
	IMediaSample* pms) :
	channel(channel),
	pin(pin),
	pms(pms)
{
	this->pms->GetPointer(&pmsData);

	#ifndef NO_QUILL
	LOG_TRACE_L2(pin->mLogger, "[{}] Pinning {} bytes", this->pin->mLogPrefix, this->pms->GetSize());
	#endif

	MWPinVideoBuffer(this->channel, pmsData, this->pms->GetSize());
}

MagewellVideoCapturePin::VideoFrameGrabber::~VideoFrameGrabber()
{
	#ifndef NO_QUILL
	LOG_TRACE_L2(pin->mLogger, "[{}] Unpinning {} bytes, captured {} bytes", pin->mLogPrefix, pms->GetSize(),
		pms->GetActualDataLength());
	#endif

	MWUnpinVideoBuffer(channel, pmsData);
}

HRESULT MagewellVideoCapturePin::VideoFrameGrabber::grab()
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
		if (pin->mStreamStartTime == 0)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(pin->mLogger, "[{}] Stream has not started, sleeping", pin->mLogPrefix);
			#endif

			BACKOFF;
			continue;
		}
		auto hr = pin->LoadSignal(&channel);
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
			LOG_TRACE_L2(pin->mLogger, "[{}] No signal {}, sleeping", pin->mLogPrefix,
				static_cast<int>(pin->mVideoSignal.signalStatus.state));
			#endif

			BACKOFF;
			continue;
		}

		if (pin->ShouldChangeMediaType())
		{
			#ifndef NO_QUILL
			LOG_WARNING(pin->mLogger, "[{}] VideoFormat changed! Attempting to reconnect", pin->mLogPrefix);
			#endif

			VIDEO_FORMAT newVideoFormat = {};
			LoadFormat(&newVideoFormat, &pin->mVideoSignal);

			CMediaType proposedMediaType(pin->m_mt);
			pin->VideoFormatToMediaType(&proposedMediaType, &newVideoFormat);

			hr = pin->DoChangeMediaType(&proposedMediaType, &newVideoFormat);
			if (FAILED(hr))
			{
				#ifndef NO_QUILL
				LOG_WARNING(pin->mLogger, "[{}] VideoFormat changed but not able to reconnect! Sleeping [Result: {}]",
					pin->mLogPrefix, hr);
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

				pin->mLastMwResult = MWGetVideoFrameInfo(channel, pin->mVideoSignal.bufferInfo.iNewestBuffered,
					&pin->mVideoSignal.frameInfo);
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
				do
				{
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
						if (pin->mVideoFormat.hdrMeta.exists && (pin->mFrameCounter % pin->mVideoFormat.fps) == 0)
						{
							#ifndef NO_QUILL
							LOG_TRACE_L1(pin->mLogger, "[{}] Updating HDR meta in frame {}", pin->mLogPrefix,
								pin->mFrameCounter);
							#endif

							// This can fail if you have a filter behind this which does not understand side data
							IMediaSideData* pMediaSideData = nullptr;
							if (SUCCEEDED(pms->QueryInterface(&pMediaSideData)))
							{
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
	auto h_channel = mFilter->GetChannelHandle();

	auto hr = LoadSignal(&h_channel);
	if (SUCCEEDED(hr))
	{
		LoadFormat(&mVideoFormat, &mVideoSignal);

		#ifndef NO_QUILL
		LOG_WARNING(
			mLogger, "[{}] Initialised video format {} x {} ({}:{}) @ {} Hz in {} bits ({} {} tf: {}) size {} bytes",
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
			"[{}] Initialised video format using defaults {} x {} ({}:{}) @ {} Hz in {} bits ({} {} tf: {}) size {} bytes",
			mLogPrefix,
			mVideoFormat.cx, mVideoFormat.cy, mVideoFormat.aspectX, mVideoFormat.aspectY, mVideoFormat.fps,
			mVideoFormat.bitDepth,
			mVideoFormat.pixelStructureName, mVideoFormat.colourFormatName, mVideoFormat.hdrMeta.transferFunction,
			mVideoFormat.imageSize);
		#endif
	}
}

void MagewellVideoCapturePin::LoadFormat(VIDEO_FORMAT* videoFormat, VIDEO_SIGNAL* videoSignal)
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
	videoFormat->imageSize = FOURCC_CalcImageSize(videoFormat->pixelStructure, videoFormat->cx, videoFormat->cy,
		videoFormat->lineLength);
}

void MagewellVideoCapturePin::LoadHdrMeta(HDR_META* meta, HDMI_HDR_INFOFRAME_PAYLOAD* frame)
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
	colorimetry->VideoTransferMatrix = videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV709
		? DXVA_VideoTransferMatrix_BT709
		: static_cast<DXVA_VideoTransferMatrix>(4);
	// 1 = REC.709, 9 = BT.2020
	colorimetry->VideoPrimaries = videoFormat->colourFormat == MWCAP_VIDEO_COLOR_FORMAT_YUV709
		? DXVA_VideoPrimaries_BT709
		: static_cast<DXVA_VideoPrimaries>(9);
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

bool MagewellVideoCapturePin::ShouldChangeMediaType()
{
	auto reconnect = false;
	if (mVideoSignal.signalStatus.cx != mVideoFormat.cx || mVideoSignal.signalStatus.cy != mVideoFormat.cy)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video dimension change {}x{} to {}x{}", mLogPrefix, mVideoFormat.cx, mVideoFormat.cy,
			mVideoSignal.signalStatus.cx, mVideoSignal.signalStatus.cy);
		#endif
	}
	if (mVideoSignal.signalStatus.nAspectX != mVideoFormat.aspectX || mVideoSignal.signalStatus.nAspectY != mVideoFormat
		.aspectY)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video AR change {}x{} to {}x{}", mLogPrefix, mVideoFormat.aspectX, mVideoFormat.aspectY,
			mVideoSignal.signalStatus.nAspectX, mVideoSignal.signalStatus.nAspectY);
		#endif
	}
	if (abs(mVideoSignal.signalStatus.dwFrameDuration - mVideoFormat.frameInterval) >= 100)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video FPS change {} to {}", mLogPrefix, mVideoFormat.fps,
			10000000 / mVideoSignal.signalStatus.dwFrameDuration);
		#endif
	}
	if (mVideoFormat.bitDepth != mVideoSignal.inputStatus.hdmiStatus.byBitDepth)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video bit depth change {} to {}", mLogPrefix, mVideoFormat.bitDepth,
			mVideoSignal.inputStatus.hdmiStatus.byBitDepth);
		#endif
	}
	if (mVideoFormat.pixelEncoding != mVideoSignal.inputStatus.hdmiStatus.pixelEncoding)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video pixel encoding change {} to {}", mLogPrefix,
			static_cast<int>(mVideoFormat.pixelEncoding),
			static_cast<int>(mVideoSignal.inputStatus.hdmiStatus.pixelEncoding));
		#endif
	}
	if (mVideoFormat.colourFormat != mVideoSignal.signalStatus.colorFormat)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video colour format change {} to {}", mLogPrefix,
			static_cast<int>(mVideoFormat.colourFormat), static_cast<int>(mVideoSignal.signalStatus.colorFormat));
		#endif
	}
	if (mVideoFormat.quantization != mVideoSignal.signalStatus.quantRange || mVideoFormat.saturation != mVideoSignal.
		signalStatus.satRange)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video colorimetry change quant {} to {} sat {} to {}", mLogPrefix,
			static_cast<int>(mVideoFormat.quantization), static_cast<int>(mVideoSignal.signalStatus.quantRange),
			static_cast<int>(mVideoFormat.saturation), static_cast<int>(mVideoSignal.signalStatus.satRange)
		);
		#endif
	}
	auto incomingTransferFunction = mVideoSignal.hdrInfo.byEOTF == 0x2 ? 15 : 4;
	if (mVideoFormat.hdrMeta.transferFunction != incomingTransferFunction)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogger, "[{}] Video transfer function change {} to {}", mLogPrefix,
			mVideoFormat.hdrMeta.transferFunction, incomingTransferFunction);
		#endif
	}

	return reconnect;
}

HRESULT MagewellVideoCapturePin::LoadSignal(HCHANNEL* pChannel)
{
	mLastMwResult = MWGetVideoSignalStatus(*pChannel, &mVideoSignal.signalStatus);
	if (mLastMwResult != MW_SUCCEEDED)
	{
		#ifndef NO_QUILL
		LOG_WARNING(mLogger, "MagewellVideoCapturePin::LoadSignal MWGetVideoSignalStatus failed");
		#endif

		return S_FALSE;
	}
	mLastMwResult = MWGetInputSpecificStatus(*pChannel, &mVideoSignal.inputStatus);
	if (mLastMwResult != MW_SUCCEEDED)
	{
		#ifndef NO_QUILL
		LOG_WARNING(mLogger, "MagewellVideoCapturePin::LoadSignal MWGetInputSpecificStatus failed");
		#endif

		return S_FALSE;
	}
	DWORD tPdwValidFlag = 0;
	MWGetHDMIInfoFrameValidFlag(*pChannel, &tPdwValidFlag);
	HDMI_INFOFRAME_PACKET pkt;
	if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_HDR)
	{
		MWGetHDMIInfoFramePacket(*pChannel, MWCAP_HDMI_INFOFRAME_ID_HDR, &pkt);
		if (!mHasHdrInfoFrame)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] HDR Infoframe is present tf: {} to {}", mLogPrefix, mVideoSignal.hdrInfo.byEOTF,
				pkt.hdrInfoFramePayload.byEOTF);
			#endif
		}
		mVideoSignal.hdrInfo = pkt.hdrInfoFramePayload;
		mHasHdrInfoFrame = true;
	}
	else
	{
		if (mHasHdrInfoFrame)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogger, "[{}] HDR Infoframe no longer present", mLogPrefix);
			#endif
		}
		mHasHdrInfoFrame = false;
		mVideoSignal.hdrInfo = {};
	}
	if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_AVI)
	{
		MWGetHDMIInfoFramePacket(*pChannel, MWCAP_HDMI_INFOFRAME_ID_AVI, &pkt);
		mVideoSignal.aviInfo = pkt.aviInfoFramePayload;
	}
	else
	{
		mVideoSignal.aviInfo = {};
	}
	return S_OK;
}

HRESULT MagewellVideoCapturePin::DoChangeMediaType(const CMediaType* pmt, const VIDEO_FORMAT* newVideoFormat)
{
	#ifndef NO_QUILL
	LOG_WARNING(
		mLogger, "[{}] Proposing new video format {} x {} ({}:{}) @ {} Hz in {} bits ({} {} tf: {}) size {} bytes",
		mLogPrefix,
		newVideoFormat->cx, newVideoFormat->cy, newVideoFormat->aspectX, newVideoFormat->aspectY, newVideoFormat->fps,
		newVideoFormat->bitDepth,
		newVideoFormat->pixelStructureName, newVideoFormat->colourFormatName, newVideoFormat->hdrMeta.transferFunction,
		newVideoFormat->imageSize);
	#endif

	auto retVal = RenegotiateMediaType(pmt, mVideoFormat.imageSize, newVideoFormat->imageSize);
	if (retVal == S_OK)
	{
		mFilter->NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(newVideoFormat->cx, newVideoFormat->cy), 0);
		mVideoFormat = *newVideoFormat;
	}

	return retVal;
}

HRESULT MagewellVideoCapturePin::FillBuffer(IMediaSample* pms)
{
	VideoFrameGrabber vfg(this, mFilter->GetChannelHandle(), pms);
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

	// Wait Events
	mNotifyEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	auto h_channel = mFilter->GetChannelHandle();
	LoadSignal(&h_channel);

	// start capture
	mCaptureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	mLastMwResult = MWStartVideoCapture(h_channel, mCaptureEvent);
	if (mLastMwResult != MW_SUCCEEDED)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] Unable to MWStartVideoCapture", mLogPrefix);
		// TODO throw?
		#endif
	}

	// register for signal change events & video buffering
	mNotify = MWRegisterNotify(h_channel, mNotifyEvent,
		MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE | MWCAP_NOTIFY_VIDEO_FRAME_BUFFERING |
		MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE);
	if (!mNotify)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] Unable to MWRegistryNotify", mLogPrefix);
		#endif
		// TODO throw
	}
	return NOERROR;
}

void MagewellVideoCapturePin::StopCapture()
{
	MWStopVideoCapture(mFilter->GetChannelHandle());
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
// MagewellAudioCapturePin
//////////////////////////////////////////////////////////////////////////
MagewellAudioCapturePin::MagewellAudioCapturePin(HRESULT* phr, MagewellCaptureFilter* pParent) :
	MagewellCapturePin(phr, pParent, "AudioCapture", L"Audio", "Audio")
{
	DWORD dwInputCount = 0;
	auto h_channel = pParent->GetChannelHandle();
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
		auto hr = LoadSignal(&h_channel);
		if (hr == S_OK)
		{
			LoadFormat(&mAudioFormat, &mAudioSignal);
		}
	}

	#ifndef NO_QUILL
	LOG_WARNING(mLogger, "[{}] Audio Status Fs: {} Bits: {} Channels: {} Codec: {}", mLogPrefix, mAudioFormat.fs,
		mAudioFormat.bitDepth, mAudioFormat.outputChannelCount, codecNames[mAudioFormat.codec]);
	#endif
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
	audioFormat->fs = audioIn.signalStatus.dwSampleRate;
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
			}

			// CEA-861-E Table 28
			switch (audioFormat->channelAllocation)
			{
			case 0x00:
				// FL FR
				break;
			case 0x01:
				// FL FR LFE --
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
				// FL FR LFE FC RL RR
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
		pmt->subtype = MEDIASUBTYPE_PCM;

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
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
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

HRESULT MagewellAudioCapturePin::LoadSignal(HCHANNEL* pChannel)
{
	mLastMwResult = MWGetAudioSignalStatus(*pChannel, &mAudioSignal.signalStatus);
	if (MW_SUCCEEDED != mLastMwResult)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] ERROR! MWGetAudioSignalStatus", mLogPrefix);
		#endif
		return S_FALSE;
	}

	DWORD tPdwValidFlag = 0;
	MWGetHDMIInfoFrameValidFlag(*pChannel, &tPdwValidFlag);
	HDMI_INFOFRAME_PACKET pkt;
	if (tPdwValidFlag & MWCAP_HDMI_INFOFRAME_MASK_AUDIO)
	{
		MWGetHDMIInfoFramePacket(*pChannel, MWCAP_HDMI_INFOFRAME_ID_AUDIO, &pkt);
		mAudioSignal.audioInfo = pkt.audioInfoFramePayload;
	}
	else
	{
		mAudioSignal.audioInfo = {};
	}
	if (mAudioSignal.signalStatus.wChannelValid == 0)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] ERROR! No valid audio channels detected {}", mLogPrefix,
			mAudioSignal.signalStatus.wChannelValid);
		#endif
		return S_FALSE;
	}
	return S_OK;
}

bool MagewellAudioCapturePin::ShouldChangeMediaType(AUDIO_FORMAT* newAudioFormat)
{
	auto reconnect = false;
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
	return reconnect;
}

HRESULT MagewellAudioCapturePin::FillBuffer(IMediaSample* pms)
{
	auto h_channel = mFilter->GetChannelHandle();
	auto retVal = S_OK;

	BYTE* pmsData;
	pms->GetPointer(&pmsData);
	auto sampleSize = pms->GetSize();
	auto bytesCaptured = 0L;
	auto samplesCaptured = 0;

	if (mAudioFormat.codec != PCM)
	{
		memcpy(pmsData, mDataBurstBuffer, mDataBurstSize);
		pms->SetActualDataLength(mDataBurstSize);
		samplesCaptured++;
		bytesCaptured = mDataBurstSize;
	}
	else
	{
		auto pbAudioFrame = reinterpret_cast<BYTE*>(mAudioSignal.frameInfo.adwSamples);

		// channel order on input is L0-L3,R0-R3 which has to be remapped to L0,R0,L1,R1,L2,R2,L3,R3
		// each 4 byte sample is left zero padded if the incoming stream is a lower bit depth (which is typically the case for HDMI audio)
		// must also apply the channel offsets to ensure each input channel is offset as necessary to be written to the correct output channel index
		auto outputChannelIdxL = -1;
		auto outputChannelIdxR = -1;
		auto outputChannels = -1;
		auto mustRescaleLfe = mAudioFormat.lfeLevelAdjustment != unity; // NOLINT(clang-diagnostic-float-equal)

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
					int sampleValueL = pbAudioFrame[inByteStartIdxL] << 24 | pbAudioFrame[inByteStartIdxL + 1] << 16 |
						pbAudioFrame[inByteStartIdxL + 2] << 8 | pbAudioFrame[inByteStartIdxL + 3];

					int sampleValueR = pbAudioFrame[inByteStartIdxR] << 24 | pbAudioFrame[inByteStartIdxR + 1] << 16 |
						pbAudioFrame[inByteStartIdxR + 2] << 8 | pbAudioFrame[inByteStartIdxR + 3];

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
							if (outIdx < sampleSize) pmsData[outIdx] = pbAudioFrame[inByteStartIdxL + k];
						}

						if (outputOffsetR != not_present)
						{
							auto outIdx = outByteStartIdxR + k;
							bytesCaptured++;
							if (outIdx < sampleSize) pmsData[outIdx] = pbAudioFrame[inByteStartIdxR + k];
						}
					}
				}
				if (pairIdx == 0) samplesCaptured++;
			}
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
		LOG_WARNING(mLogger, "[{}] Audio frame {} : samples {} time {} size {} bytes buf {} bytes", mLogPrefix,
			mFrameCounter, samplesCaptured, endTime, bytesCaptured, sampleSize);
	}
	else if (mSinceLastLog == 0)
	{
		LOG_TRACE_L1(mLogger, "[{}] Audio frame {} : samples {} time {} size {} bytes buf {} bytes", mLogPrefix,
			mFrameCounter, samplesCaptured, endTime, bytesCaptured, sampleSize);
	}
	else
	{
		LOG_TRACE_L3(mLogger, "[{}] Audio frame {} : samples {} time {} size {} bytes buf {} bytes", mLogPrefix,
			mFrameCounter, samplesCaptured, endTime, bytesCaptured, sampleSize);
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
	memset(mDataBurstBuffer, 0, sizeof(mDataBurstBuffer));

	// Wait Events
	mNotifyEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	auto h_channel = mFilter->GetChannelHandle();
	LoadSignal(&h_channel);

	// start capture
	mLastMwResult = MWStartAudioCapture(h_channel);
	if (mLastMwResult != MW_SUCCEEDED)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] MagewellAudioCapturePin::OnThreadCreate Unable to MWStartAudioCapture", mLogPrefix);
		#endif
		// TODO throw
	}

	// register for signal change events & audio buffered
	mNotify = MWRegisterNotify(h_channel, mNotifyEvent,
		MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE | MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE |
		MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED);
	if (!mNotify)
	{
		#ifndef NO_QUILL
		LOG_ERROR(mLogger, "[{}] MagewellAudioCapturePin::OnThreadCreate Unable to MWRegistryNotify", mLogPrefix);
		#endif
		// TODO throw
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

	auto newSize = MWCAP_AUDIO_SAMPLES_PER_FRAME * newAudioFormat->bitDepthInBytes * newAudioFormat->outputChannelCount;
	auto oldSize = MWCAP_AUDIO_SAMPLES_PER_FRAME * mAudioFormat.bitDepthInBytes * mAudioFormat.outputChannelCount;
	auto retVal = RenegotiateMediaType(pmt, oldSize, newSize);
	if (retVal == S_OK)
	{
		mAudioFormat = *newAudioFormat;
	}
	return retVal;
}

void MagewellAudioCapturePin::StopCapture()
{
	MWStopAudioCapture(mFilter->GetChannelHandle());
}

bool MagewellAudioCapturePin::ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cbBuffer = MWCAP_AUDIO_SAMPLES_PER_FRAME * mAudioFormat.bitDepthInBytes * mAudioFormat.
		outputChannelCount;
	if (pProperties->cBuffers < 1)
	{
		pProperties->cBuffers = 4;
		return false;
	}
	return true;
}

// loops til we have a frame to process, dealing with any mediatype changes as we go and then grabs a buffer once it's time to go
HRESULT MagewellAudioCapturePin::GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime,
	REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	auto h_channel = mFilter->GetChannelHandle();
	auto hasFrame = false;
	auto retVal = S_FALSE;
	// keep going til we have a frame to process
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
			LOG_TRACE_L1(mLogger, "[{}] Stream has not started, sleeping", mLogPrefix);
			#endif

			BACKOFF;
			continue;
		}

		if (S_OK != LoadSignal(&h_channel))
		{
			BACKOFF;
			continue;
		}

		AUDIO_FORMAT newAudioFormat(mAudioFormat);
		LoadFormat(&newAudioFormat, &mAudioSignal);

		if (newAudioFormat.outputChannelCount == 0)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L2(mLogger, "[{}] No signal, sleeping", mLogPrefix);
			#endif

			BACKOFF;
			continue;
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

		if (mStatusBits & MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED)
		{
			mLastMwResult = MWCaptureAudioFrame(h_channel, &mAudioSignal.frameInfo);
			if (MW_SUCCEEDED == mLastMwResult)
			{
				// some devices report compressed audio as 192kHz LPCM
				auto mustProbe = newAudioFormat.codec == BITSTREAM || newAudioFormat.fs > 48000;
				Codec* contentCodec{};
				*contentCodec = PCM;
				if (mustProbe)
				{
					CopyToBitstreamBuffer(reinterpret_cast<BYTE*>(mAudioSignal.frameInfo.adwSamples));
					uint16_t bufferSize = mAudioFormat.bitDepthInBytes * MWCAP_AUDIO_SAMPLES_PER_FRAME * mAudioFormat.inputChannelCount;
					if (S_OK != ProbeBitstreamBuffer(bufferSize, &contentCodec))
					{
						*contentCodec = PCM;
					}
				}
				newAudioFormat.codec = *contentCodec;
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
						LOG_WARNING(mLogger, "[{}] AudioFormat changed but not able to reconnect ({}) sleeping", mLogPrefix, hr);
						#endif

						// TODO communicate that we need to change somehow
						BACKOFF;
					}
					continue;
				}

				retVal = MagewellCapturePin::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
				if (SUCCEEDED(retVal))
				{
					hasFrame = true;
				}
				else
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogger, "[{}] Audio frame buffered but unable to get delivery buffer, sleeping", mLogPrefix);
					#endif
				}
			}
			else
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogger, "[{}] Audio frame buffered but capture failed, sleeping", mLogPrefix);
				#endif
			}
			if (!hasFrame) SHORT_BACKOFF;
		}
	}
	return retVal;
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

// copies the inbound byte stream into a format that can be probed
void MagewellAudioCapturePin::CopyToBitstreamBuffer(BYTE* pBuf)
{
	// copies from input to output skipping zero bytes and with a byte swap per sample
	for (auto pairIdx = 0; pairIdx < mAudioFormat.inputChannelCount / 2; ++pairIdx)
	{
		for (auto sampleIdx = 0; sampleIdx < MWCAP_AUDIO_SAMPLES_PER_FRAME; sampleIdx++)
		{
			int inStartL = (sampleIdx * MWCAP_AUDIO_MAX_NUM_CHANNELS + pairIdx) * maxBitDepthInBytes + mAudioFormat.bitDepthInBytes - 1;
			int inStartR = (sampleIdx * MWCAP_AUDIO_MAX_NUM_CHANNELS + pairIdx + MWCAP_AUDIO_MAX_NUM_CHANNELS / 2) * maxBitDepthInBytes + mAudioFormat.bitDepthInBytes - 1;
			int outStart = (sampleIdx * 2 + pairIdx * 2) * mAudioFormat.bitDepthInBytes;
			for (int byteIdx = 0; byteIdx < mAudioFormat.bitDepthInBytes; ++byteIdx) {
				mCompressedBuffer[outStart + byteIdx] = pBuf[inStartL - byteIdx];
				mCompressedBuffer[outStart + mAudioFormat.bitDepthInBytes + byteIdx] = pBuf[inStartR - byteIdx];
			}
		}
	}
}

// probes a non PCM buffer for the codec based on format of the IEC 61937 dataframes
HRESULT MagewellAudioCapturePin::ProbeBitstreamBuffer(uint16_t bufSize, enum Codec** codec)
{
	mDataBurstSize = 0;
	auto bytesRead = 0;
	auto burstSize = 0;
	auto burstRead = 0;
	auto foundPaPb = false;
	while (bytesRead < bufSize)  // expect to get a single burst per frame so this should only loop twice (once to read the header, once to copy the data)
	{
		if (burstSize > 0)
		{
			auto toCopy = std::min(burstSize, bufSize - bytesRead);
			#ifndef NO_QUILL
			LOG_TRACE_L3(mLogger, "[{}] Copying {} of {} bytes of encoded audio", mLogPrefix, toCopy, burstSize);
			#endif
			memcpy(mCompressedBuffer + bytesRead, mDataBurstBuffer + burstRead, toCopy);
		}

		// Find Pa Pb preamble words "F8 72 4E 1F"
		for (; (bytesRead < bufSize - 3) && !foundPaPb; ++bytesRead)
		{
			if (mCompressedBuffer[bytesRead] == 0xf8 && mCompressedBuffer[bytesRead + 1] == 0x72 &&
				mCompressedBuffer[bytesRead + 2] == 0x4e && mCompressedBuffer[bytesRead + 3] == 0x1f)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L3(mLogger, "[{}] Found PaPb at position {}", mLogPrefix, bytesRead);
				#endif
				foundPaPb = true;
				bytesRead += 2 * sizeof(WORD);
				break;
			}
		}

		if (!foundPaPb)
		{
			continue;
		}

		// grab Pc Pd preamble words
		auto bytesToCopy = std::min(bufSize - bytesRead, 4 - mPcPdBufferSize);
		memcpy(mPcPdBuffer, mCompressedBuffer + bytesRead, bytesToCopy);
		mPcPdBufferSize += bytesToCopy;
		bytesRead += bytesToCopy;

		if (mPcPdBufferSize != 4)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L3(mLogger, "[{}] Found PcPd at position {} but only {} bytes available", mLogPrefix, bytesRead - bytesToCopy, bytesToCopy);
			#endif
			continue;
		}

		burstSize = ((static_cast<WORD>(mPcPdBuffer[2]) << 8) + static_cast<WORD>(mPcPdBuffer[3])) / 8;
		GetCodecFromIEC61937Preamble(IEC61937DataType{ mPcPdBuffer[1] & 0x7f }, &burstSize, *codec);
		mPcPdBufferSize = 0;
		foundPaPb = FALSE;
		#ifndef NO_QUILL
		LOG_TRACE_L3(mLogger, "[{}] Found codec {} with burst size {}", mLogPrefix, static_cast<int>(**codec), burstSize);
		#endif
	}
	#ifndef NO_QUILL
	LOG_TRACE_L3(mLogger, "[{}] Read {} bytes of {} from burst for codec {}", mLogPrefix, burstRead, burstSize, static_cast<int>(**codec));
	#endif
	if (burstSize == burstRead)
	{
		mDataBurstSize = burstSize;
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

// identifies codecs that are known/expected to be carried via HDMI in an AV setup
// from IEC 61937-2 Table 2
HRESULT MagewellAudioCapturePin::GetCodecFromIEC61937Preamble(enum IEC61937DataType dataType, int* burstSize, enum Codec* codec)
{
	switch (dataType & 0xff)
	{
	case IEC61937_AC3:
		*codec = AC3;
		break;
	case IEC61937_DTS1:
	case IEC61937_DTS2:
	case IEC61937_DTS3:
		*codec = DTS;
		break;
	case IEC61937_DTSHD:
		*burstSize *= 8;
		*codec = DTSHD;
		break;
	case IEC61937_EAC3:
		*codec = EAC3;
		*burstSize *= 8;
		break;
	case IEC61937_TRUEHD:
		*codec = TRUEHD;
		*burstSize *= 8;
		break;
	default:
		break;
	}
	return S_OK;
}
