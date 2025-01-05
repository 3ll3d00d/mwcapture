# mwcapture

A Direct Show filter that can capture audio and video from a [Magewell Pro Capture HDMI 4k Plus](https://www.magewell.com/products/pro-capture-hdmi-4k-plus) card using the [MWCapture SDK](https://www.magewell.com/sdk) with support for multichannel audio and HDR.

Depends on LAVAudio being downstream for audio handling purposes.

Supports both linear and non linear PCM and automatic identification of codec for the latter case.

## WARNING!

Use at own risk, tested on one machine only

## Installation

* Download and install the latest [Magewell SDK](https://www.magewell.com/downloads#capture-sdk-dark-anchor)
* Download the latest release from https://github.com/3ll3d00d/mwcapture/releases & unzip to some directory
* Run `regsvr32 mwcapture.ax` as an admin in that directory
* Configure your video player as usual specifying `Magewell Pro Capture` as the capture device (for both audio and video)

## Logging

Outputs some basic info to `%TEMP%\magewell_capture_YYYYMMDD_HHMMSS.log`

Use the -trace or -warn distribution in order to get access to logs (quite verbose logs in the trace case) that detail what the filter sees (NB: disk space use can grow rapidly in the trace case, recommended use for investigation purposes only).

## Limitations

* Live updates to audio and video formats dependent on downstream filters (renderers, audio output) as the source format changes
* Magewell Pro cards have issues handling 192kHz audio (TBD if environment or hardware specific, logged with magewell support) which prevents use with EAC3, TrueHD or DTSHD MA (effectively means AC3 only)

## Tested On

* Windows 10
* mpc-hc
* [madvr b113 or below only](http://madshi.net/madVRhdrMeasure113.zip) or mpc-vr

Compatible with [J River JRVR](https://wiki.jriver.com/index.php/JRVR_-_JRiver_Video_Renderer) in [MC33](https://yabb.jriver.com/interact/index.php/board,84.0.html) but depends on [enhancement](https://yabb.jriver.com/interact/index.php/topic,140304.0.html) to put LAV in the filter graph 
