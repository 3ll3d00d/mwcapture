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

#include <Windows.h>
#include <Commctrl.h>
#include <initguid.h>
#include <streams.h>

#include <string>


// {8DC689DB-68FE-4C30-AAE5-0E515CF9324C}
DEFINE_GUID(CLSID_SignalInfoProps,
    0x8dc689db, 0x68fe, 0x4c30, 0xaa, 0xe5, 0xe, 0x51, 0x5c, 0xf9, 0x32, 0x4c);


// {6A505550-28B2-4668-BC2C-461E75A63BC4}
DEFINE_GUID(IID_ISignalInfo,
            0x6a505550, 0x28b2, 0x4668, 0xbc, 0x2c, 0x46, 0x1e, 0x75, 0xa6, 0x3b, 0xc4);


struct SIGNAL_INFO_VALUES
{
    int inX{ -1 };
    int inY{ -1 };
    int inAspectX{ -1 };
    int inAspectY{ -1 };
    int outX{ -1 };
    int outY{ -1 };
    int outAspectX{ -1 };
    int outAspectY{ -1 };
    std::string signalStatus;
    std::string inColourFormat;
    std::string outColourFormat;
    std::string inQuantisation;
    std::string outQuantisation;
    std::string inSaturation;
    std::string outSaturation;
    double inFps;
    double outFps;
    bool validSignal{ false };
    int inBitDepth{ 0 };
    int outBitDepth{ 0 };
    std::string inPixelLayout;
    std::string outPixelLayout;
    std::string outTransferFunction;
    bool hdrOn{ false };
    double hdrPrimaryRX;
    double hdrPrimaryRY;
    double hdrPrimaryGX;
    double hdrPrimaryGY;
    double hdrPrimaryBX;
    double hdrPrimaryBY;
    double hdrWpX;
    double hdrWpY;
    double hdrMinDML;
    double hdrMaxDML;
    bool audioInStatus;
    bool audioInIsPcm;
    unsigned char audioInBitDepth;
    unsigned long audioInFs;
    unsigned short audioInChannelMask;
    unsigned char audioInChannelMap;
    unsigned char audioInLfeLevel;
    std::string audioOutChannelLayout;
    unsigned char audioOutBitDepth;
    std::string audioOutCodec;
    unsigned long audioOutFs;
    double audioOutLfeOffset;
    int audioOutLfeChannelIndex;
    unsigned short audioOutChannelCount;
    uint16_t audioOutDataBurstSize;
};


interface __declspec(uuid("6A505550-28B2-4668-BC2C-461E75A63BC4")) ISignalInfo : public IUnknown
{
	STDMETHOD(GetSignalInfo)(SIGNAL_INFO_VALUES* value) = 0;
};

class CSignalInfoProp : public CBasePropertyPage
{
public:
	// Provide the way for COM to create a Filter object
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

	CSignalInfoProp(LPUNKNOWN pUnk, HRESULT* phr);
	~CSignalInfoProp() override;

	HRESULT OnActivate() override;
	HRESULT OnConnect(IUnknown* pUnk) override;
	HRESULT OnDisconnect() override;
	HRESULT OnApplyChanges() override;
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
	HRESULT LoadData();

	void SetDirty()
	{
		m_bDirty = TRUE;
		if (m_pPageSite)
		{
			m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
		}
	}

	ISignalInfo* mSignalInfo = nullptr;
	SIGNAL_INFO_VALUES mSignalInfoValues{};
};
