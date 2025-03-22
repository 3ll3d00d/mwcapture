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
#include <string>

struct DEVICE_STATUS
{
    std::string deviceDesc{};
};

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

struct AUDIO_INPUT_STATUS
{
    bool audioInStatus;
    bool audioInIsPcm;
    unsigned char audioInBitDepth;
    unsigned long audioInFs;
    unsigned short audioInChannelPairs;
    unsigned char audioInChannelMap;
    unsigned char audioInLfeLevel;
};

struct AUDIO_OUTPUT_STATUS
{
    std::string audioOutChannelLayout;
    unsigned char audioOutBitDepth;
    std::string audioOutCodec;
    unsigned long audioOutFs;
    short audioOutLfeOffset;
    int audioOutLfeChannelIndex;
    unsigned short audioOutChannelCount;
    uint16_t audioOutDataBurstSize;
};

struct VIDEO_INPUT_STATUS
{
    int inX{ -1 };
    int inY{ -1 };
    int inAspectX{ -1 };
    int inAspectY{ -1 };
    std::string signalStatus;
    std::string inColourFormat;
    std::string inQuantisation;
    std::string inSaturation;
    double inFps;
    int inBitDepth{ 0 };
    std::string inPixelLayout;
    bool validSignal{ false };
};

struct VIDEO_OUTPUT_STATUS
{
    int outX{ -1 };
    int outY{ -1 };
    int outAspectX{ -1 };
    int outAspectY{ -1 };
    std::string outColourFormat;
    std::string outQuantisation;
    std::string outSaturation;
    double outFps;
    int outBitDepth{ 0 };
    std::string outPixelLayout;
    std::string outPixelStructure;
    std::string outTransferFunction;
};

struct HDR_STATUS
{
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
    double hdrMaxCLL;
    double hdrMaxFALL;
};
