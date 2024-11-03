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

#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <dvdmedia.h>
#include <dllsetup.h>
#include "mwcapture.h"

#define CreateComObject(clsid, iid, var) CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&var);

STDAPI AMovieSetupRegisterServer(CLSID clsServer, LPCWSTR szDescription, LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", LPCWSTR szServerType = L"InprocServer32");

STDAPI AMovieSetupUnregisterServer(CLSID clsServer);

// {4E3B0A92-2476-4016-81F0-201F19F6FBAE}
DEFINE_GUID(CLSID_MWCAPTURE_FILTER, 
    0x4e3b0a92, 0x2476, 0x4016, 0x81, 0xf0, 0x20, 0x1f, 0x19, 0xf6, 0xfb, 0xae);


STDAPI RegisterFilters(BOOL bRegister)
{
	WCHAR achFileName[MAX_PATH];
    char achTemp[MAX_PATH];
    ASSERT(g_hInst != 0);

    if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp)))
        return AmHresultFromWin32(GetLastError());

    MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1, achFileName, NUMELMS(achFileName));

    HRESULT hr = CoInitialize(0);
    if (bRegister)
    {
        hr = AMovieSetupRegisterServer(CLSID_MWCAPTURE_FILTER, L"Magewell Pro Capture", achFileName, L"Both", L"InprocServer32");
    }

    if (SUCCEEDED(hr))
    {
        IFilterMapper2* fm = 0;
        hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm)
        if (SUCCEEDED(hr))
        {
            if (bRegister)
            {
                REGFILTER2 videoFilter;
                videoFilter.dwVersion = 1;
                videoFilter.dwMerit = MERIT_DO_NOT_USE;
                videoFilter.cPins = 1;
                videoFilter.rgPins = &sMIPPins[0];
                hr = fm->RegisterFilter(CLSID_MWCAPTURE_FILTER, L"Magewell Pro Capture", nullptr, &CLSID_VideoInputDeviceCategory, nullptr, &videoFilter);
            }
            else
            {
                hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, nullptr, CLSID_MWCAPTURE_FILTER);
            }
        }

        if (fm)
            fm->Release();
    }

    if (!bRegister)
        hr += AMovieSetupUnregisterServer(CLSID_MWCAPTURE_FILTER);

    CoFreeUnusedLibraries();
    CoUninitialize();
    return hr;
}

STDAPI DllRegisterServer()
{
    return RegisterFilters(TRUE);
}

STDAPI DllUnregisterServer()
{
    return RegisterFilters(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
    return DllEntryPoint(static_cast<HINSTANCE>(hModule), dwReason, lpReserved);
}
