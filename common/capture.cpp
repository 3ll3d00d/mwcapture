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

#ifndef NO_QUILL
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"
#include <string_view>
#include <utility>
#include "quill/std/WideString.h"
#endif // !NO_QUILL

#include "capture.h"
#include <DXVA.h>

#ifdef _DEBUG
#define MIN_LOG_LEVEL quill::LogLevel::TraceL3
#else
#define MIN_LOG_LEVEL quill::LogLevel::TraceL2
#endif

CaptureFilter::CaptureFilter(LPCTSTR pName, LPUNKNOWN punk, HRESULT* phr, CLSID clsid, std::string pLogPrefix) :
	CSource(pName, punk, clsid)
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
	mLogData.logger = 
		CustomFrontend::create_or_get_logger("filter",
			std::move(fileSink),
			quill::PatternFormatterOptions{
				"%(time) [%(thread_id)] %(short_source_location:<28) "
				"LOG_%(log_level:<9) %(logger:<12) %(message)",
				"%H:%M:%S.%Qns",
				quill::Timezone::GmtTime
			});

	// printing absolutely everything we may ever log
	mLogData.logger->set_log_level(MIN_LOG_LEVEL);
	#endif // !NO_QUILL

	mLogData.prefix = std::move(pLogPrefix);
}

STDMETHODIMP CaptureFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
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
		return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
	}
	if (riid == IID_ISignalInfo)
	{
		return GetInterface(static_cast<ISignalInfo*>(this), ppv);
	}
	return CSource::NonDelegatingQueryInterface(riid, ppv);
}

void CaptureFilter::GetReferenceTime(REFERENCE_TIME* rt) const
{
	m_pClock->GetTime(rt);
}

HRESULT CaptureFilter::GetTime(REFERENCE_TIME* pTime)
{
	return mClock->GetTime(pTime);
}

HRESULT CaptureFilter::AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HEVENT hEvent,
	DWORD_PTR* pdwAdviseCookie)
{
	return mClock->AdviseTime(baseTime, streamTime, hEvent, pdwAdviseCookie);
}

HRESULT CaptureFilter::AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime,
	HSEMAPHORE hSemaphore, DWORD_PTR* pdwAdviseCookie)
{
	return mClock->AdvisePeriodic(startTime, periodTime, hSemaphore, pdwAdviseCookie);
}

HRESULT CaptureFilter::Unadvise(DWORD_PTR dwAdviseCookie)
{
	return mClock->Unadvise(dwAdviseCookie);
}

ULONG CaptureFilter::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
};

STDMETHODIMP CaptureFilter::GetState(DWORD dw, FILTER_STATE* pState)
{
	CBaseFilter::GetState(dw, pState);
	return *pState == State_Paused ? VFW_S_CANT_CUE : S_OK;
}

STDMETHODIMP CaptureFilter::SetSyncSource(IReferenceClock* pClock)
{
	CBaseFilter::SetSyncSource(pClock);
	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<IMediaFilter*>(m_paStreams[i]);
		stream->SetSyncSource(pClock);
	}
	return NOERROR;
}

HRESULT CaptureFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	auto hr = CSource::JoinFilterGraph(pGraph, pName);
	if (SUCCEEDED(hr))
	{
		for (auto i = 0; i < m_iPins; i++)
		{
			auto stream = dynamic_cast<CBaseStreamControl*>(m_paStreams[i]);
			stream->SetFilterGraph(m_pSink);
		}
	}
	return hr;
}

STDMETHODIMP CaptureFilter::Run(REFERENCE_TIME tStart)
{
	REFERENCE_TIME rt;
	GetReferenceTime(&rt);

	#ifndef NO_QUILL
	LOG_INFO(mLogData.logger, "[{}] Filter has started running at {}", mLogData.prefix, rt);
	#endif

	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<IAMTimeAware*>(m_paStreams[i]);
		stream->SetStartTime(rt);
		auto s1 = dynamic_cast<CBaseStreamControl*>(m_paStreams[i]);
		s1->NotifyFilterState(State_Running, tStart);
	}
	return CBaseFilter::Run(tStart);
}

STDMETHODIMP CaptureFilter::Pause()
{
	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<CBaseStreamControl*>(m_paStreams[i]);
		stream->NotifyFilterState(State_Paused);
	}
	return CBaseFilter::Pause();
}

STDMETHODIMP CaptureFilter::Stop()
{
	for (auto i = 0; i < m_iPins; i++)
	{
		auto stream = dynamic_cast<CBaseStreamControl*>(m_paStreams[i]);
		stream->NotifyFilterState(State_Stopped);
	}
	return CBaseFilter::Stop();
}

HRESULT CaptureFilter::SetCallback(ISignalInfoCB* cb)
{
	mInfoCallback = cb;
	return S_OK;
}

STDMETHODIMP CaptureFilter::GetPages(CAUUID* pPages)
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

STDMETHODIMP CaptureFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
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
	delete* ppPage;
	ppPage = nullptr;
	return E_FAIL;
}

void CaptureFilter::OnVideoFormatLoaded(VIDEO_FORMAT* vf)
{
	mVideoOutputStatus.outX = vf->cx;
	mVideoOutputStatus.outY = vf->cy;
	mVideoOutputStatus.outAspectX = vf->aspectX;
	mVideoOutputStatus.outAspectY = vf->aspectY;
	mVideoOutputStatus.outFps = vf->fps;

	switch (vf->colourFormat)
	{
	case COLOUR_FORMAT_UNKNOWN:
		mVideoOutputStatus.outColourFormat = "?";
		break;
	case RGB:
		mVideoOutputStatus.outColourFormat = "RGB";
		break;
	case YUV601:
		mVideoOutputStatus.outColourFormat = "YUV601";
		break;
	case YUV709:
		mVideoOutputStatus.outColourFormat = "YUV709";
		break;
	case YUV2020:
		mVideoOutputStatus.outColourFormat = "YUV2020";
		break;
	case YUV2020C:
		mVideoOutputStatus.outColourFormat = "YUV2020C";
		break;
	}

	switch (vf->quantisation)
	{
	case QUANTISATION_UNKNOWN:
		mVideoOutputStatus.outQuantisation = "?";
		break;
	case QUANTISATION_LIMITED:
		mVideoOutputStatus.outQuantisation = "Limited";
		break;
	case QUANTISATION_FULL:
		mVideoOutputStatus.outQuantisation = "Full";
		break;
	}

	switch (vf->saturation)
	{
	case SATURATION_UNKNOWN:
		mVideoOutputStatus.outSaturation = "?";
		break;
	case SATURATION_LIMITED:
		mVideoOutputStatus.outSaturation = "Limited";
		break;
	case SATURATION_FULL:
		mVideoOutputStatus.outSaturation = "Full";
		break;
	case EXTENDED_GAMUT:
		mVideoOutputStatus.outSaturation = "Extended";
		break;
	}

	mVideoOutputStatus.outBitDepth = vf->bitDepth;

	switch (vf->pixelEncoding)
	{
	case YUV_420:
		mVideoOutputStatus.outPixelLayout = "YUV 4:2:0";
		break;
	case YUV_422:
		mVideoOutputStatus.outPixelLayout = "YUV 4:2:2";
		break;
	case YUV_444:
		mVideoOutputStatus.outPixelLayout = "YUV 4:4:4";
		break;
	case RGB_444:
		mVideoOutputStatus.outPixelLayout = "RGB 4:4:4";
		break;
	}

	mVideoOutputStatus.outPixelStructure = vf->pixelStructureName;
	switch (vf->hdrMeta.transferFunction)
	{
	case 4:
		mVideoOutputStatus.outTransferFunction = "REC.709";
		break;
	case 15:
		mVideoOutputStatus.outTransferFunction = "SMPTE ST 2084 (PQ)";
		break;
	default:
		mVideoOutputStatus.outTransferFunction = "?";
		break;
	}

	if (mInfoCallback != nullptr)
	{
		mInfoCallback->Reload(&mVideoOutputStatus);
	}
}

void CaptureFilter::OnHdrUpdated(MediaSideDataHDR* hdr, MediaSideDataHDRContentLightLevel* light)
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

void CaptureFilter::OnAudioFormatLoaded(AUDIO_FORMAT* af)
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

void IAMTimeAware::SetStartTime(LONGLONG streamStartTime)
{
	mStreamStartTime = streamStartTime;

	#ifndef NO_QUILL
	LOG_WARNING(mLogData.logger, "[{}] CapturePin::SetStartTime at {}", mLogData.prefix, streamStartTime);
	#endif
}


//////////////////////////////////////////////////////////////////////////
// CapturePin
//////////////////////////////////////////////////////////////////////////
CapturePin::CapturePin(HRESULT* phr, CSource* pParent, LPCSTR pObjectName, LPCWSTR pPinName, std::string pLogPrefix) :
	CSourceStream(pObjectName, phr, pParent, pPinName),
	IAMTimeAware(pLogPrefix, "filter"),
	mFrameCounter(0),
	mPreview(false),
	mLastSampleDiscarded(0),
	mSendMediaType(0),
	mFrameEndTime(0),
	mHasSignal(false),
	mLastSentHdrMetaAt(0LL)
{
	#ifndef NO_QUILL
	mLogData.prefix = std::move(pLogPrefix);
	mLogData.logger = CustomFrontend::get_logger("filter");
	#endif
}

HRESULT CapturePin::HandleStreamStateChange(IMediaSample* pms)
{
	// TODO override this if MediaType changed?
	int iStreamState = CheckStreamState(pms);
	if (iStreamState == STREAM_FLOWING)
	{
		if (mLastSampleDiscarded)
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(mLogData.logger, "[{}] Recovery after sample discard, setting discontinuity", mLogData.prefix);
			#endif

			pms->SetDiscontinuity(TRUE);
			mLastSampleDiscarded = FALSE;
		}
		return S_OK;
	}
	else
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogData.logger, "[{}] Entering stream discard", mLogData.prefix);
		#endif

		mLastSampleDiscarded = TRUE;
		return S_FALSE;
	}
}

HRESULT CapturePin::OnThreadStartPlay()
{
	#ifndef NO_QUILL
	REFERENCE_TIME rt;
	GetReferenceTime(&rt);

	if (mStreamStartTime == 0)
	{
		LOG_WARNING(mLogData.logger, "[{}] Pin worker thread starting at {} but stream not started yet", mLogData.prefix, rt);
	}
	else
	{
		LOG_WARNING(mLogData.logger, "[{}] Pin worker thread starting at {}, stream started at {}", mLogData.prefix, rt,
			mStreamStartTime);
	}
	#endif
	return S_OK;
}

STDMETHODIMP CapturePin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
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
HRESULT CapturePin::DoBufferProcessingLoop(void) {
	#ifndef NO_QUILL
	LOG_INFO(mLogData.logger, "[{}] Entering DoBufferProcessingLoop", mLogData.prefix);
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
				LOG_WARNING(mLogData.logger, "[{}] Failed to GetDeliveryBuffer ({:#08x}), retrying", mLogData.prefix, hrBuf);
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
					LOG_WARNING(mLogData.logger, "[{}] Failed to deliver sample downstream ({:#08x}), process loop will exit", mLogData.prefix, hr);
					#endif

					return S_OK;
				}

			}
			else if (hr == S_FALSE)
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogData.logger, "[{}] Buffer not filled, retrying", mLogData.prefix);
				#endif
				pSample->Release();
			}
			else
			{
				#ifndef NO_QUILL
				LOG_WARNING(mLogData.logger, "[{}] FillBuffer failed ({:#08x}), sending EOS and EC_ERRORABORT", mLogData.prefix, hr);
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
			LOG_INFO(mLogData.logger, "[{}] DoBufferProcessingLoop Replying to CMD {}", mLogData.prefix, static_cast<int>(com));
			#endif
			Reply(NOERROR);
		}
		else if (com != CMD_STOP)
		{
			#ifndef NO_QUILL
			LOG_ERROR(mLogData.logger, "[{}] DoBufferProcessingLoop Replying to UNEXPECTED CMD {}", mLogData.prefix, static_cast<int>(com));
			#endif
			Reply(static_cast<DWORD>(E_UNEXPECTED));
		}
		else
		{
			#ifndef NO_QUILL
			LOG_INFO(mLogData.logger, "[{}] DoBufferProcessingLoop CMD_STOP will exit", mLogData.prefix);
			#endif
		}
	} while (com != CMD_STOP);

	#ifndef NO_QUILL
	LOG_INFO(mLogData.logger, "[{}] Exiting DoBufferProcessingLoop", mLogData.prefix);
	#endif
	return S_FALSE;
}

HRESULT CapturePin::OnThreadDestroy()
{
	#ifndef NO_QUILL
	LOG_INFO(mLogData.logger, "[{}] >>> CapturePin::OnThreadDestroy", mLogData.prefix);
	#endif

	DoThreadDestroy();

	#ifndef NO_QUILL
	LOG_INFO(mLogData.logger, "[{}] <<< CapturePin::OnThreadDestroy", mLogData.prefix);
	#endif

	return S_OK;
}

HRESULT CapturePin::BeginFlush()
{
	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::BeginFlush", mLogData.prefix);
	#endif

	this->Flushing(TRUE);
	return CSourceStream::BeginFlush();
}

HRESULT CapturePin::EndFlush()
{
	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::EndFlush", mLogData.prefix);
	#endif

	this->Flushing(FALSE);
	return CSourceStream::EndFlush();
}

HRESULT CapturePin::Notify(IBaseFilter* pSelf, Quality q)
{
	// just to avoid use of DbgBreak in default implementation as we can't do anything about it given we are a slave to the device
	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::Notify {}", mLogData.prefix, q.Type == 0 ? "Famine" : "Flood");
	#endif

	return S_OK;
}

// see https://learn.microsoft.com/en-us/windows/win32/api/strmif/nf-strmif-iamstreamconfig-setformat
STDMETHODIMP CapturePin::SetFormat(AM_MEDIA_TYPE* pmt)
{
	#ifndef NO_QUILL
	LOG_WARNING(mLogData.logger, "[{}] CapturePin::SetFormat is not supported", mLogData.prefix);
	#endif
	// TODO try to support this?
	return VFW_E_INVALIDMEDIATYPE;
}

STDMETHODIMP CapturePin::GetFormat(AM_MEDIA_TYPE** ppmt)
{
	CMediaType cmt;
	GetMediaType(&cmt);
	*ppmt = CreateMediaType(&cmt);
	return S_OK;
}

HRESULT CapturePin::SetMediaType(const CMediaType* pmt)
{
	HRESULT hr = CSourceStream::SetMediaType(pmt);

	#ifndef NO_QUILL
	LOG_TRACE_L3(mLogData.logger, "[{}] SetMediaType (res: {:#08x})", mLogData.prefix, hr);
	#endif

	return hr;
}

HRESULT CapturePin::DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	CheckPointer(pIMemAlloc, E_POINTER)
		CheckPointer(pProperties, E_POINTER)
		CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;
	auto acceptedUpstreamBufferCount = ProposeBuffers(pProperties);

	#ifndef NO_QUILL
	LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::DecideBufferSize size: {} count: {} (from upstream? {})",
		mLogData.prefix, pProperties->cbBuffer, pProperties->cBuffers, acceptedUpstreamBufferCount);
	#endif

	ALLOCATOR_PROPERTIES actual;
	hr = pIMemAlloc->SetProperties(pProperties, &actual);

	if (FAILED(hr))
	{
		#ifndef NO_QUILL
		LOG_WARNING(mLogData.logger, "[{}] CapturePin::DecideBufferSize failed to SetProperties result {:#08x}", mLogData.prefix,
			hr);
		#endif

		return hr;
	}
	if (actual.cbBuffer < pProperties->cbBuffer)
	{
		#ifndef NO_QUILL
		LOG_WARNING(mLogData.logger, "[{}] CapturePin::DecideBufferSize actual buffer is {} not {}", mLogData.prefix,
			actual.cbBuffer, pProperties->cbBuffer);
		#endif

		return E_FAIL;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// CapturePin -> IKsPropertySet
//////////////////////////////////////////////////////////////////////////
HRESULT CapturePin::Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData,
	DWORD cbInstanceData, void* pPropData, DWORD cbPropData)
{
	// Set: Cannot set any properties.
	return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CapturePin::Get(
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
HRESULT CapturePin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
{
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	// We support getting this property, but not setting it.
	if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	return S_OK;
}

HRESULT CapturePin::RenegotiateMediaType(const CMediaType* pmt, long newSize, boolean renegotiateOnQueryAccept)
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
		LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::RenegotiateMediaType ReceiveConnection accepted", mLogData.prefix);
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
			LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::NegotiateMediaType Buffers outstanding, retrying in 10ms..",
				mLogData.prefix);
			#endif

			BACKOFF;
			timeout -= 10;
		}
		else
		{
			#ifndef NO_QUILL
			LOG_TRACE_L1(
				mLogData.logger, "[{}] CapturePin::NegotiateMediaType Buffers outstanding, timeout reached, flushing..",
				mLogData.prefix);
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
		LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::NegotiateMediaType QueryAccept accepted", mLogData.prefix);
		#endif

		hr = SetMediaType(pmt);
		if (SUCCEEDED(hr))
		{
			if (!renegotiateOnQueryAccept)
			{
				#ifndef NO_QUILL
				LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::NegotiateMediaType - No buffer change", mLogData.prefix);
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
							LOG_TRACE_L1(mLogData.logger, "[{}] Updated allocator to {} bytes {} buffers", mLogData.prefix,
								props.cbBuffer, props.cBuffers);
							#endif
							retVal = S_OK;
						}
						else
						{
							#ifndef NO_QUILL
							LOG_WARNING(
								mLogData.logger,
								"[{}] Allocator accepted update to {} bytes {} buffers but is {} bytes {} buffers",
								mLogData.prefix, props.cbBuffer, props.cBuffers, checkProps.cbBuffer, checkProps.cBuffers);
							#endif
						}
					}
					else
					{
						#ifndef NO_QUILL
						LOG_WARNING(mLogData.logger, "[{}] Allocator did not accept update to {} bytes {} buffers [{:#08x}]",
							mLogData.prefix, props.cbBuffer, props.cBuffers, hr);
						#endif
					}
				}
				else
				{
					#ifndef NO_QUILL
					LOG_WARNING(mLogData.logger, "[{}] Allocator did not commit update to {} bytes {} buffers [{:#08x}]",
						mLogData.prefix, props.cbBuffer, props.cBuffers, hr);
					#endif
				}
			}
		}
	}
	else
	{
		#ifndef NO_QUILL
		LOG_WARNING(
			mLogData.logger, "[{}] CapturePin::NegotiateMediaType Receive Connection failed (hr: {:#08x}); QueryAccept: {:#08x}",
			mLogData.prefix, hr, hrQA);
		#endif
	}
	if (retVal == S_OK)
	{
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::NegotiateMediaType succeeded", mLogData.prefix);
		#endif

		mSendMediaType = TRUE;
	}
	else
	{
		// reinstate the old formats otherwise we're stuck thinking we have the new format
		#ifndef NO_QUILL
		LOG_TRACE_L1(mLogData.logger, "[{}] CapturePin::NegotiateMediaType failed {:#08x}", mLogData.prefix, retVal);
		#endif

		SetMediaType(&oldMediaType);
	}

	return retVal;
}

//////////////////////////////////////////////////////////////////////////
// VideoCapturePin
//////////////////////////////////////////////////////////////////////////
void VideoCapturePin::VideoFormatToMediaType(CMediaType* pmt, VIDEO_FORMAT* videoFormat) const
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
	colorimetry->VideoTransferMatrix = videoFormat->colourFormat == YUV2020
		? static_cast<DXVA_VideoTransferMatrix>(4)
		: DXVA_VideoTransferMatrix_BT709;
	// 1 = REC.709, 9 = BT.2020
	colorimetry->VideoPrimaries = videoFormat->colourFormat == YUV2020
		? static_cast<DXVA_VideoPrimaries>(9)
		: DXVA_VideoPrimaries_BT709;
	// 4 = REC.709, 15 = SMPTE ST 2084 (PQ)
	colorimetry->VideoTransferFunction = static_cast<DXVA_VideoTransferFunction>(videoFormat->hdrMeta.transferFunction);
	// 0 = unknown, 1 = 0-255, 2 = 16-235
	colorimetry->NominalRange = static_cast<DXVA_NominalRange>(videoFormat->quantisation);

	#ifndef NO_QUILL
	LOG_TRACE_L3(mLogData.logger, "[{}] DXVA_ExtendedFormat {} {} {} {}", mLogData.prefix,
		static_cast<int>(colorimetry->VideoTransferMatrix),
		static_cast<int>(colorimetry->VideoPrimaries), static_cast<int>(colorimetry->VideoTransferFunction),
		static_cast<int>(colorimetry->NominalRange));
	#endif

	pvi->dwControlFlags += AMCONTROL_USED;
	pvi->dwControlFlags += AMCONTROL_COLORINFO_PRESENT;

	auto isRgb = videoFormat->pixelEncoding == RGB_444;
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth = videoFormat->cx;
	pvi->bmiHeader.biHeight = isRgb ? -(videoFormat->cy) : videoFormat->cy; // RGB on windows is upside down
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biBitCount = videoFormat->bitCount;
	pvi->bmiHeader.biCompression = isRgb ? BI_RGB : videoFormat->pixelStructure;
	pvi->bmiHeader.biSizeImage = videoFormat->imageSize;
	pvi->bmiHeader.biXPelsPerMeter = 0;
	pvi->bmiHeader.biYPelsPerMeter = 0;
	pvi->bmiHeader.biClrUsed = 0;
	pvi->bmiHeader.biClrImportant = 0;

	// Work out the GUID for the subtype from the header info.
	const auto subTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
	pmt->SetSubtype(&subTypeGUID);
}

bool VideoCapturePin::ShouldChangeMediaType(VIDEO_FORMAT* newVideoFormat)
{
	auto reconnect = false;
	if (newVideoFormat->cx != mVideoFormat.cx || newVideoFormat->cy != mVideoFormat.cy)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogData.logger, "[{}] Video dimension change {}x{} to {}x{}", 
			mLogData.prefix, mVideoFormat.cx, mVideoFormat.cy,
			newVideoFormat->cx, newVideoFormat->cy);
		#endif
	}
	if (newVideoFormat->aspectX != mVideoFormat.aspectX || newVideoFormat->aspectY != mVideoFormat.aspectY)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogData.logger, "[{}] Video AR change {}x{} to {}x{}", 
			mLogData.prefix, mVideoFormat.aspectX, mVideoFormat.aspectY,
			newVideoFormat->aspectX, newVideoFormat->aspectY);
		#endif
	}
	if (abs(newVideoFormat->frameInterval - mVideoFormat.frameInterval) >= 100)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogData.logger, "[{}] Video FPS change {:.3f} to {:.3f}", 
			mLogData.prefix, mVideoFormat.fps, newVideoFormat->fps);
		#endif
	}
	if (mVideoFormat.bitDepth != newVideoFormat->bitDepth)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogData.logger, "[{}] Video bit depth change {} to {}", 
			mLogData.prefix, mVideoFormat.bitDepth, newVideoFormat->bitDepth);
		#endif
	}
	if (mVideoFormat.pixelEncoding != newVideoFormat->pixelEncoding)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogData.logger, "[{}] Video pixel encoding change {} to {}", 
			mLogData.prefix,
			static_cast<int>(mVideoFormat.pixelEncoding),
			static_cast<int>(newVideoFormat->pixelEncoding));
		#endif
	}
	if (mVideoFormat.colourFormat != newVideoFormat->colourFormat)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogData.logger, "[{}] Video colour format change {} to {}", 
			mLogData.prefix, mVideoFormat.colourFormatName, newVideoFormat->colourFormatName);
		#endif
	}
	if (mVideoFormat.quantisation != newVideoFormat->quantisation || mVideoFormat.saturation != newVideoFormat->saturation)
	{
		reconnect = true;

		#ifndef NO_QUILL
		LOG_INFO(mLogData.logger, "[{}] Video colorimetry change quant {} to {} sat {} to {}", 
			mLogData.prefix,
			static_cast<int>(mVideoFormat.quantisation), static_cast<int>(newVideoFormat->quantisation),
			static_cast<int>(mVideoFormat.saturation), static_cast<int>(newVideoFormat->saturation)
		);
		#endif
	}
	auto incomingTransferFunction = newVideoFormat->hdrMeta.transferFunction;
	if (mVideoFormat.hdrMeta.transferFunction != newVideoFormat->hdrMeta.transferFunction)
	{
		reconnect = true;

		#ifndef NO_QUILL
		auto formatFrom = mVideoFormat.hdrMeta.transferFunction == 0 ? "?" : mVideoFormat.hdrMeta.transferFunction == 4 ? "REC.709" : "SMPTE ST 2084 (PQ)";
		auto formatTo = incomingTransferFunction == 0 ? "?" : incomingTransferFunction == 4 ? "REC.709" : "SMPTE ST 2084 (PQ)";
		LOG_INFO(mLogData.logger, "[{}] Video transfer function change {} ({}) to {} ({})", 
			mLogData.prefix, formatFrom, mVideoFormat.hdrMeta.transferFunction, formatTo, incomingTransferFunction);
		#endif
	}

	return reconnect;
}

HRESULT VideoCapturePin::GetMediaType(CMediaType* pmt)
{
	VideoFormatToMediaType(pmt, &mVideoFormat);
	return NOERROR;
}

STDMETHODIMP VideoCapturePin::GetNumberOfCapabilities(int* piCount, int* piSize)
{
	*piCount = 1;
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

STDMETHODIMP VideoCapturePin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC)
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

bool VideoCapturePin::ProposeBuffers(ALLOCATOR_PROPERTIES* pProperties)
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
