# mwcapture

A Direct Show filter that can capture audio and video from a [Magewell Pro Capture HDMI 4k Plus](https://www.magewell.com/products/pro-capture-hdmi-4k-plus) card using the [MWCapture SDK](https://www.magewell.com/sdk) with support for multichannel audio and HDR.

## WARNING!

Use at own risk, tested on one machine with a single source device

## Installation

* Download and install the latest [Magewell SDK](https://www.magewell.com/downloads#capture-sdk-dark-anchor)
* Download the latest release from https://github.com/3ll3d00d/mwcapture/releases & unzip to some directory
* Run `regsvr32 mwcapture.ax` as an admin in that directory
* Configure your video player as usual specifying `Magewell Pro Capture` as the capture device (for both audio and video)

## Logging

Outputs some basic info to `%TEMP%\magewell_capture_YYYYMMDD_HHMMSS.log`

Use the -debug or -logging distribution for much (much) more verbose logging (NB: performance may be impacted, disk space use will grow rapidly, use for investigation purposes only).

## Limitations

* PCM audio only
* Live updates to audio and video formats dependent on downstream filters (renderers, audio output) as the source format changes

## Tested On

* Windows 10
* [J River JRVR](https://wiki.jriver.com/index.php/JRVR_-_JRiver_Video_Renderer) in [MC33](https://yabb.jriver.com/interact/index.php/board,84.0.html)
* [madvr b113 or below only](http://madshi.net/madVRhdrMeasure113.zip) 
