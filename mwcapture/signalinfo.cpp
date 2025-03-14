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

#include "version.h"
#include "signalinfo.h"
#include "commctrl.h"

#include <corecrt_wstdio.h>

#include "resource.h"

CUnknown* CSignalInfoProp::CreateInstance(LPUNKNOWN punk, HRESULT* phr)
{
	auto pNewObject = new CSignalInfoProp(punk, phr);

	if (pNewObject == nullptr)
	{
		if (phr)
		{
			*phr = E_OUTOFMEMORY;
		}
	}

	return pNewObject;
}

CSignalInfoProp::CSignalInfoProp(LPUNKNOWN pUnk, HRESULT* phr)
	: CBasePropertyPage("SignalInfoProp", pUnk, IDD_PROPPAGE_SIGNAL_INFO, IDS_TITLE)
{
}

CSignalInfoProp::~CSignalInfoProp() = default;


HRESULT CSignalInfoProp::OnActivate()
{
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_BAR_CLASSES;
	if (InitCommonControlsEx(&icc) == FALSE)
	{
		return E_FAIL;
	}
	ASSERT(mSignalInfo != nullptr);

	auto version = L"MWCapture" " " MW_VERSION_STR;
	SendDlgItemMessage(m_Dlg, IDC_SIGNAL_STATUS_FOOTER, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(version));

	return Reload(ALL);
}

HRESULT CSignalInfoProp::OnConnect(IUnknown* pUnk)
{
	if (pUnk == nullptr)
	{
		return E_POINTER;
	}
	ASSERT(mSignalInfo == nullptr);
	HRESULT hr = pUnk->QueryInterface(&mSignalInfo);
	if (SUCCEEDED(hr))
	{
		mSignalInfo->SetCallback(this);
	}
	return hr;
}

HRESULT CSignalInfoProp::OnDisconnect()
{
	if (mSignalInfo)
	{
		mSignalInfo->SetCallback(nullptr);
		mSignalInfo->Release();
		mSignalInfo = nullptr;
	}
	return S_OK;
}

HRESULT CSignalInfoProp::OnApplyChanges()
{
	return CBasePropertyPage::OnApplyChanges();
}

INT_PTR CSignalInfoProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CSignalInfoProp::Reload(ReloadType reload)
{
	HRESULT hr = LoadData();
	if (SUCCEEDED(hr))
	{
		WCHAR buffer[28];

		if (reload == ALL || reload == VIDEO_IN)
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%d x %d (%d:%d) %d bit", mSignalInfoValues.inX, mSignalInfoValues.inY, mSignalInfoValues.inAspectX, mSignalInfoValues.inAspectY, mSignalInfoValues.inBitDepth);
			SendDlgItemMessage(m_Dlg, IDC_IN_DIMENSIONS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%.3f Hz", mSignalInfoValues.inFps);
			SendDlgItemMessage(m_Dlg, IDC_IN_FPS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.inColourFormat.c_str());
			SendDlgItemMessage(m_Dlg, IDC_IN_CF, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.inQuantisation.c_str());
			SendDlgItemMessage(m_Dlg, IDC_IN_QUANTISATION, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.inSaturation.c_str());
			SendDlgItemMessage(m_Dlg, IDC_IN_SATURATION, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.inPixelLayout.c_str());
			SendDlgItemMessage(m_Dlg, IDC_IN_PIXEL_LAYOUT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.signalStatus.c_str());
			SendDlgItemMessage(m_Dlg, IDC_SIGNAL_STATUS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
		}

		if (reload == ALL || reload == VIDEO_OUT)
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%d x %d (%d:%d) %d bit", mSignalInfoValues.outX, mSignalInfoValues.outY, mSignalInfoValues.outAspectX, mSignalInfoValues.outAspectY, mSignalInfoValues.outBitDepth);
			SendDlgItemMessage(m_Dlg, IDC_OUT_DIMENSIONS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%.3f Hz", mSignalInfoValues.outFps);
			SendDlgItemMessage(m_Dlg, IDC_OUT_FPS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.outColourFormat.c_str());
			SendDlgItemMessage(m_Dlg, IDC_OUT_CF, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.outQuantisation.c_str());
			SendDlgItemMessage(m_Dlg, IDC_OUT_QUANTISATION, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.outSaturation.c_str());
			SendDlgItemMessage(m_Dlg, IDC_OUT_SATURATION, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.outPixelLayout.c_str());
			SendDlgItemMessage(m_Dlg, IDC_OUT_PIXEL_LAYOUT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.outTransferFunction.c_str());
			SendDlgItemMessage(m_Dlg, IDC_VIDEO_OUT_TF, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
		}

		if (reload == ALL || reload == HDR)
		{
			if (mSignalInfoValues.hdrOn)
			{
				_snwprintf_s(buffer, _TRUNCATE, L"%.4f x %.4f", mSignalInfoValues.hdrPrimaryRX, mSignalInfoValues.hdrPrimaryRY);
				SendDlgItemMessage(m_Dlg, IDC_HDR_RED, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				_snwprintf_s(buffer, _TRUNCATE, L"%.4f x %.4f", mSignalInfoValues.hdrPrimaryGX, mSignalInfoValues.hdrPrimaryGY);
				SendDlgItemMessage(m_Dlg, IDC_HDR_GREEN, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				_snwprintf_s(buffer, _TRUNCATE, L"%.4f x %.4f", mSignalInfoValues.hdrPrimaryBX, mSignalInfoValues.hdrPrimaryBY);
				SendDlgItemMessage(m_Dlg, IDC_HDR_BLUE, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				_snwprintf_s(buffer, _TRUNCATE, L"%.4f x %.4f", mSignalInfoValues.hdrWpX, mSignalInfoValues.hdrWpY);
				SendDlgItemMessage(m_Dlg, IDC_HDR_WHITE, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				_snwprintf_s(buffer, _TRUNCATE, L"%.4f / %.1f", mSignalInfoValues.hdrMinDML, mSignalInfoValues.hdrMaxDML);
				SendDlgItemMessage(m_Dlg, IDC_HDR_DML, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				_snwprintf_s(buffer, _TRUNCATE, L"%.1f", mSignalInfoValues.hdrMaxCLL);
				SendDlgItemMessage(m_Dlg, IDC_HDR_MAX_CLL, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				_snwprintf_s(buffer, _TRUNCATE, L"%.1f", mSignalInfoValues.hdrMaxFALL);
				SendDlgItemMessage(m_Dlg, IDC_HDR_MAX_FALL, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			}
			else
			{
				_snwprintf_s(buffer, _TRUNCATE, L"SDR");
				SendDlgItemMessage(m_Dlg, IDC_HDR_RED, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				SendDlgItemMessage(m_Dlg, IDC_HDR_GREEN, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				SendDlgItemMessage(m_Dlg, IDC_HDR_BLUE, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				SendDlgItemMessage(m_Dlg, IDC_HDR_WHITE, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				SendDlgItemMessage(m_Dlg, IDC_HDR_DML, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				SendDlgItemMessage(m_Dlg, IDC_HDR_MAX_CLL, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
				SendDlgItemMessage(m_Dlg, IDC_HDR_MAX_FALL, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			}
		}

		if (reload == ALL || reload == AUDIO_IN)
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.audioInStatus ? "LOCKED" : "NONE");
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_IN_SIGNAL_STATUS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.audioInIsPcm ? "Y" : "N");
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_IN_PCM, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%d bit", mSignalInfoValues.audioInBitDepth);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_IN_BIT_DEPTH, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%d", mSignalInfoValues.audioInChannelMask);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_IN_CH_MASK, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%#04x", mSignalInfoValues.audioInChannelMap);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_IN_CH_MAP, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%d Hz", mSignalInfoValues.audioInFs);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_IN_FS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%#04x", mSignalInfoValues.audioInLfeLevel);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_IN_LFE_LEVEL, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
		}

		if (reload == ALL || reload == AUDIO_OUT)
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.audioOutCodec.c_str());
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_CODEC, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%d bit", mSignalInfoValues.audioOutBitDepth);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_BIT_DEPTH, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%d", mSignalInfoValues.audioOutChannelCount);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_CH_COUNT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%hs", mSignalInfoValues.audioOutChannelLayout.c_str());
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_CH_LAYOUT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%d Hz", mSignalInfoValues.audioOutFs);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_FS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			_snwprintf_s(buffer, _TRUNCATE, L"%d dB", mSignalInfoValues.audioOutLfeOffset);
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_LFE_LEVEL, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			if (mSignalInfoValues.audioOutLfeChannelIndex == -1)
			{
				_snwprintf_s(buffer, _TRUNCATE, L"No LFE");
			} 
			else
			{
				_snwprintf_s(buffer, _TRUNCATE, L"%d", mSignalInfoValues.audioOutLfeChannelIndex);
			}
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_LFE_CH, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
			if (mSignalInfoValues.audioOutCodec == "PCM")
			{
				_snwprintf_s(buffer, _TRUNCATE, L"N/A");
			}
			else
			{
				_snwprintf_s(buffer, _TRUNCATE, L"%d", mSignalInfoValues.audioOutDataBurstSize);
			}
			SendDlgItemMessage(m_Dlg, IDC_AUDIO_OUT_BURST_SZ, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
		}
	}
	return hr;
}

HRESULT CSignalInfoProp::LoadData()
{
	HRESULT hr = S_OK;
	hr = mSignalInfo->GetSignalInfo(&mSignalInfoValues);
	return hr;
}
