/************************************************************************************************/
// MWProCapture.h : header file

// MAGEWELL PROPRIETARY INFORMATION

// The following license only applies to head files and library within Magewell's SDK 
// and not to Magewell's SDK as a whole. 

// Copyrights © Nanjing Magewell Electronics Co., Ltd. ("Magewell") All rights reserved.

// Magewell grands to any person who obtains the copy of Magewell's head files and library 
// the rights,including without limitation, to use on the condition that the following terms are met:
// - The above copyright notice shall be retained in any circumstances.
// -The following disclaimer shall be included in the software and documentation and/or 
// other materials provided for the purpose of publish, distribution or sublicense.

// THE SOFTWARE IS PROVIDED BY MAGEWELL "AS IS" AND ANY EXPRESS, INCLUDING BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL MAGEWELL BE LIABLE 

// FOR ANY CLAIM, DIRECT OR INDIRECT DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT,
// TORT OR OTHERWISE, ARISING IN ANY WAY OF USING THE SOFTWARE.

// CONTACT INFORMATION:
// SDK@magewell.net
// http://www.magewell.com/
//
/************************************************************************************************/
#pragma once

#ifdef LIBMWCAPTURE_EXPORTS
#define LIBMWCAPTURE_API __declspec(dllexport)
#elif LIBMWCAPTURE_DLL
#define LIBMWCAPTURE_API __declspec(dllimport)
#else
#define LIBMWCAPTURE_API 
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @ingroup group_functions_proecocapture
 * @brief Gets device instance ID
 * @param[in]  hChannel			Opened channel handle
 * @param[out] pwszInstanceID	Instance ID
 * @param[in]	cbInstanceID	Passed string size
 * @return #MW_RESULT values are as follows:
 * <table>
 * <tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * WCHAR t_wc_device_id[256];
 * ULONG t_ul_cb=256;
 * MWGetDeviceInstanceID(hChannel,t_wc_device_id,t_ul_cb);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetDeviceInstanceID(
	HCHANNEL hChannel,
	WCHAR * pwszInstanceID,
	ULONG cbInstanceID
);

/**
 * @ingroup group_functions_proecocapture
 * @brief Opens capture channel by the rotary number and channel number.
 * @param[in] nBoardValue      Rotary number on the capture device, 0 to f
 * @param[in] nChannelIndex    Channel index, starts from 0
 * @return If the call succeeds, which returns the video channel handle; the call fails, which returns the empty value.
 * @note If there are 2 devices with the same rotary number, the one with a smaller PCI-e number
 * will be opened.\n
 * The same channel can be opened multiple times at the same time, and the handles that open the same channel are independent of each other and do not affect each other.\n
 * @details Usage:\n
 * It is used by all examples in @ref page_examples_cpp_cmd.\n
 * @code
 * MWCAP_CHANNEL_INFO info;
 * MWGetChannelInfoByIndex(0,&info);
 * HCHANNEL t_channel=MWOpenChannel(info.byBoardIndex,info.byChannelIndex);
 * ...
 * MWCloseChannel(t_channel); 
 * @endcode
*/
HCHANNEL
LIBMWCAPTURE_API
MWOpenChannel(
	int								nBoardValue,
	int								nChannelIndex
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Gets time value of capture device
 * @param[in] hChannel      Opened video channel handle
 * @param[out] pllTime      Returns time value of capture device in 100ns
 * @return Returns #MW_RESULT values as follows:
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * 	</tr>
 * </table>
 * @note Use the same clock for different channels on the same capture device, 
 * which plays an important role in audio and video data synchronization and multichannel data synchronization.\n
 * @details Usage:\n
 * Gets device time, uses the same clock for different channels on the same capture device.\n
 * It is used by most examples in @ref page_examples.\n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * LONGLONG t_time = 0;
 * MWGetDeviceTime(t_channel, &t_time);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetDeviceTime(
	HCHANNEL						hChannel,
	LONGLONG*						pllTime
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Sets time of capture device
 * @param[in] hChannel     	Opened video channel handle
 * @param[in] llTime      	Sets the time value of the capture device clock, which is in 100 nanoseconds
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @note Synchronizes clocks of different devices.
 * @details Usage:\n
 * Sets the clock of capture device.\n
 * The usage refers to @ref page_examples_cpp_gui_xicapturequad in @ref page_examples_cpp_gui.\n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * LONGLONG t_time = 0;
 * t_time=...;
 * ...
 * MWSetDeviceTime(t_channel, t_time);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWSetDeviceTime(
	HCHANNEL						hChannel,
	LONGLONG						llTime
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Adjusts time of capture device
 * @param[in] hChannel      	Opened video channel handle
 * @param[in] llTime      		Adjusts time in 100ns.
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @note Synchronizes clocks of different devices.\n
 * The difference between this API and MWSetDeviceTime is that 
 * the clock value of the capture device will calibrate to the set value
 * in a gradual manner instead of a sudden change for this API. 
 * The clock value of the capture device becomes the set time value 
 * immediately after setting MWSetDeviceTime if you are using MWSetDeviceTime. \n
 * @details Usage:\n
 * Adjusts device time.\n
 * The usage refers to @ref page_examples_cpp_gui_xicapturequad in @ref page_examples_cpp_gui.\n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * LONGLONG llStandardTime = 0LL;
 * xr = MWRegulateDeviceTime(t_channel, llStandardTime);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWRegulateDeviceTime(
	HCHANNEL						hChannel,
	LONGLONG						llTime
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Registers a time event object
 * @param[in] hChannel      	Opened video channel handle
 * @param[in] hEvent      	  	Event handle, creates with Win32 CreateEvent()
 * @return If the function succeeds, the return value is a handle to the time event object, otherwise it is NULL.
 * @details  The API is used for pro capture cards to specify the capture interval between adjacent video frames\n
 * Detailed usage refers to @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd. \n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * HANDLE hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * HANDLE hCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * MW_RESULT xr;
 * xr = MWStartVideoCapture(t_channel, hCaptureEvent);
 * HTIMER hTimerEvent = MWRegisterTimer(t_channel, hNotifyEvent);
 * LONGLONG llBegin = 0LL;
 * xr = MWGetDeviceTime(t_channel, &llBegin);
 * ...
 * xr = MWScheduleTimer(t_channel, hTimerEvent, llBegin + i * TIMER_DURATION);
 * WaitForSingleObject(hNotifyEvent, INFINITE);
 * MWCAP_VIDEO_BUFFER_INFO videoBufferInfo;
 * xr = MWGetVideoBufferInfo(t_channel, &videoBufferInfo);
 * MWCAP_VIDEO_FRAME_INFO videoFrameInfo;
 * xr = MWGetVideoFrameInfo(t_channel, videoBufferInfo.iNewestBufferedFullFrame, &videoFrameInfo);
 * xr = MWCaptureVideoFrameToVirtualAddress(t_channel, -1, (LPBYTE)bitmapData.Scan0, bitmapData.Stride * cy, bitmapData.Stride, bBottomUp, NULL, MWFOURCC_BGRA, cx, cy);
 * WaitForSingleObject(hCaptureEvent, INFINITE);
 * MWCAP_VIDEO_CAPTURE_STATUS captureStatus;
 * xr = MWGetVideoCaptureStatus(t_channel, &captureStatus);
 * ...
 * xr = MWUnregisterTimer(t_channel, hTimerEvent);
 * xr = MWStopVideoCapture(t_channel);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
*/
HTIMER
LIBMWCAPTURE_API
MWRegisterTimer(
	HCHANNEL						hChannel,
	HANDLE							hEvent
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Unregisters a time event object.
 * @param[in] hChannel      	Opened video channel handle
 * @param[in] hTimer      		Handle of time event object that has been registered
 * @return Returns #MW_RESULT, the values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 *  The API is used for pro capture cards that specify the capture interval between adjacent video frames.\n
 * Detailed usage refers to  @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd.\n
 * Related function(s): MWRegisterTimer(HCHANNEL hChannel, HANDLE hEvent) \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWUnregisterTimer(
	HCHANNEL 						hChannel,
	HTIMER							hTimer
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Schedules time event objects
 * @param[in] hChannel      	Opened video channel handle
 * @param[in] hTimer      		Scheduled time event objects
 * @param[in] llExpireTime    	Sets the time value of the scheduled time event, which is the absolute value of the capture device's clock.
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @note When the scheduled time is reached, the event associated with the time event object will be set to be the signaled state.\n
 * @details Usage:\n
 *  The API is used for pro capture cards to set the time when the time event object is triggered.\n
 * Detailed usage refers to @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd.\n
 * Related function(s): [MWRegisterTimer](@ref MWRegisterTimer) \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWScheduleTimer(
	HCHANNEL 						hChannel,
	HTIMER							hTimer,
	LONGLONG						llExpireTime
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Registers an event notification
 * @param[in] hChannel      	Opened video channel handle
 * @param[in] hEvent      	    An event handle, created by the function CreateEvent() using Win32
 * @param[in] dwEnableBits     Type mask for notifications
 * @return Returns the handle to the notification event object if succeeded, otherwise returns NULL.
 * @note dwEnableBits can be set to one or more of the following notification types.\n
 * MWCAP_NOTIFY_xx\n
 * When the notification type you set occurs, the event associated with the notification event object will be set to the signaled state.\n
 * @details Usage:\n
 * The API is used to trigger Pro capture cards to capture video clips by notification type.\n
 * Detailed usage refers to : @ref page_examples_cpp_cmd_capturebyinput in @ref page_examples_cpp_cmd   \n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * HANDLE hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * HANDLE hCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * MW_RESULT xr;
 * xr = MWStartVideoCapture(t_channel, hCaptureEvent);
 * ...
 * HNOTIFY hNotify = MWRegisterNotify(t_channel, hNotifyEvent, MWCAP_NOTIFY_VIDEO_FRAME_BUFFERED);
 * ...
 * WaitForSingleObject(hNotifyEvent, INFINITE);
 * xr = MWGetNotifyStatus(t_channel, hNotify, &ullStatusBits);
 * MWCAP_VIDEO_BUFFER_INFO videoBufferInfo;
 * xr = MWGetVideoBufferInfo(t_channel, &videoBufferInfo);
 * MWCAP_VIDEO_FRAME_INFO videoFrameInfo;
 * xr = MWGetVideoFrameInfo(t_channel, videoBufferInfo.iNewestBufferedFullFrame, &videoFrameInfo);
 * xr = MWCaptureVideoFrameToVirtualAddress(t_channel, -1, (LPBYTE)bitmapData.Scan0, bitmapData.Stride * cy, bitmapData.Stride, bBottomUp, NULL, MWFOURCC_BGRA, cx, cy);
 * WaitForSingleObject(hCaptureEvent, INFINITE);
 * MWCAP_VIDEO_CAPTURE_STATUS captureStatus;
 * xr = MWGetVideoCaptureStatus(t_channel, &captureStatus);
 * ...
 * xr = MWUnregisterNotify(t_channel, hNotify);
 * xr = MWStopVideoCapture(t_channel);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
*/
HNOTIFY
LIBMWCAPTURE_API
MWRegisterNotify(
	HCHANNEL 						hChannel,
	HANDLE							hEvent,
	DWORD64							dwEnableBits
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Unregisters an event notification object
 * @param[in] hChannel      		Channel handle
 * @param[in] hNotify        		Event notification
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * The API is used for Pro capture cards to capture by the notification type.\n
 * Detailed usage refers to function(s): @ref page_examples_cpp_cmd_capturebyinput in @ref page_examples_cpp_cmd.\n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 */
MW_RESULT
	LIBMWCAPTURE_API
	MWUnregisterNotify(
	HCHANNEL 						hChannel,
	HNOTIFY							hNotify
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Gets current notification type
 * @param[in] hChannel      	Opened video channel handle
 * @param[in] hNotify      		Handle of registerred notify event object
 * @param[out] pullStatus     	Returns notification type. The value refers to the note of [MWRegisterNotify](@ref MWRegisterNotify) 
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * The function is used for Pro capture cards to capture video clips by notification type.\n
 * The detailed usage refers to the @ref page_examples_cpp_cmd_capturebyinput in @ref page_examples_cpp_cmd. \n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetNotifyStatus(
	HCHANNEL 						hChannel,
	HNOTIFY							hNotify,
	ULONGLONG*						pullStatus
	);

/**
 * @ingroup group_functions_procapture
 * @brief Starts video capture 
 * @param[in] hChannel      	Opened video channel handle
 * @param[in] hEvent      		An event handle, which is created by CreateEvent() using Win32.
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * It is uses for Pro capture cards.\n
 * Detailed usage refers to the related function(s): @ref page_examples_cpp_cmd of
 *  @ref page_examples_cpp_cmd_capturebyinput and 
 *  @ref page_examples_cpp_cmd_capturebytimer  in @ref page_examples_cpp_cmd.\n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 *  [MWRegisterTimer](@ref MWRegisterTimer)\n
 */
MW_RESULT
LIBMWCAPTURE_API
MWStartVideoCapture(
	HCHANNEL 						hChannel,
	HANDLE							hEvent
	);	

/**
 * @ingroup group_functions_procapture
 * @brief Stops the video capture
 * @param[in] hChannel      	Opened handle of video capture channel 
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n 
 * Detailed usage refers to the related function(s): 
 *  @ref page_examples_cpp_cmd_capturebyinput and 
 *  @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd.\n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 *  [MWRegisterTimer](@ref MWRegisterTimer) \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWStopVideoCapture(
	HCHANNEL 						hChannel
	);

/**
 * @ingroup group_functions_procapture
 * @brief Locks a segment of virtual memory to reduce CPU load when capturing video data to that memory.
 * @param[in] hChannel      	Opened channel handle
 * @param[in] pbFrame      		Virtual memory address
 * @param[in] cbFrame     		Virtual memory size
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Locks a segment of virtual memory to reduce the CPU usage when storing the captured video clips to that memory.\n
 * Detailed usage refers to the related function(s): @ref page_examples_cpp_gui of @ref page_examples_cpp_gui_lowlatency \n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * HANDLE hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * HANDLE hCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * MW_RESULT xr;
 * xr = MWPinVideoBuffer(t_channel, byBuffer, dwFrameSize)
 * ...
 * xr = MWStartVideoCapture(t_channel, hCaptureEvent);
 * ...
 * ...
 * xr = MWStopVideoCapture(t_channel);
 * xr = MWUnpinVideoBuffer(t_channel, byBuffer);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWPinVideoBuffer(
	HCHANNEL 						hChannel,
	LPBYTE							pbFrame,
	DWORD							cbFrame
	);

/**
 * @ingroup group_functions_procapture
 * @brief Unlocks a segment of virtual memory.
 * @param[in] hChannel      	Opened channel handle
 * @param[in] pbFrame      		Virtual memory address
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Unlocks a segment of virtual memory, and works with [MWPinVideoBuffer](@ref MWPinVideoBuffer).\n
 * Detailed usage refers to the related function(s): @ref page_examples_cpp_gui_lowlatency in @ref page_examples_cpp_gui \n
 * Related function(s):  [MWPinVideoBuffer](@ref MWPinVideoBuffer) \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWUnpinVideoBuffer(
	HCHANNEL 						hChannel,
	LPBYTE							pbFrame
	);

/**
 * @ingroup group_functions_procapture
 * @brief Captures and saves video frames to system memory.
 * @param[in] hChannel      	Opened video capture channel handle
 * @param[in] iFrame      	    Video frame number to be captured
 * @param[out] pbFrame     		Stores virtual memory pointers for captured data
 * @param[in] cbFrame      		Byte length of memory that stores the captured data  
 * @param[in] cbStride      	Step of memory that stores the captured data  
 * @param[in] bBottomUp     	Whether to store the captured video frames from bottom to top
 * @param[in] pvContext      	Custom context pointers
 * @param[in] dwFOURCC      	Color format of captured video frames, see MWFOURCC.h. 
 * @param[in] cx     			Width of captured video frames
 * @param[in] cy     			Height of captured video frames
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Detailed usage refers to the related function(s): 
 *  @ref page_examples_cpp_cmd_capturebyinput and 
 *  @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd. \n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 *  [MWRegisterTimer](@ref MWRegisterTimer)\n
 */
MW_RESULT
LIBMWCAPTURE_API
MWCaptureVideoFrameToVirtualAddress(
	HCHANNEL 						hChannel,
	int								iFrame,
	LPBYTE							pbFrame,
	DWORD							cbFrame,
	DWORD							cbStride,
	BOOLEAN							bBottomUp,
	MWCAP_PTR64						pvContext,
	DWORD							dwFOURCC,
	int								cx,
	int								cy
	);

/**
 * @ingroup group_functions_procapture
 * @brief Stores captured video frames to physical addresses
 * @param[in] hChannel			Video channel handle that has started video capturing
 * @param[in] iFrame			Video frame number to be captured
 * @param[in] llFrameAddress	Physical addresses that store the captured video frames
 * @param[in] cbFrame			Byte length of memory that stores the captured data
 * @param[in] cbStride			Step of memory that stores the captured data
 * @param[in] bBottomUp			Whether to store the captured video frames from bottom to top
 * @param[in] pvContext			Custom context pointers
 * @param[in] dwFOURCC			Color format of captured video frames, see MWFOURCC.h.
 * @param[in] cx				Width of captured video frames
 * @param[in] cy				Height of captured video frames
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @note \n
 * Be cautious when using it.\n
*/
MW_RESULT
LIBMWCAPTURE_API
MWCaptureVideoFrameToPhysicalAddress(
	HCHANNEL 						hChannel,
	int								iFrame,
	LONGLONG						llFrameAddress,
	DWORD							cbFrame,
	DWORD							cbStride,
	BOOLEAN							bBottomUp,
	MWCAP_PTR64						pvContext,
	DWORD							dwFOURCC,
	int								cx,
	int								cy
	);

/**
 * @ingroup group_functions_procapture
 * @brief Captures video frames to virtual memory, and adds OSD.
 * @param[in] hChannel		Video channel handle that has started video capturing
 * @param[in] iFrame		Video frame number to be captured
 * @param[out] pbFrame		Physical addresses that store the captured video frames
 * @param[in] cbFrame		Byte length of memory that stores the captured data
 * @param[in] cbStride		Step of memory that stores the captured data
 * @param[in] bBottomUp		Whether to store the captured video frames from bottom to top
 * @param[in] pvContext		Custom context pointers
 * @param[in] dwFOURCC		Color format of captured video frames, see MWFOURCC.h.
 * @param[in] cx			Width of captured video frames
 * @param[in] cy			Height of captured video frames
 * @param[in] hOSDImage     Handle of OSD image which is geted using [MWCreateImage](@ref MWCreateImage)
 * @param[in] pOSDRects     Target area of OSD image
 * @param[in] cOSDRects     Number of OSD image target areas
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Saves the captured video frame to virtual memory, and adds OSD. \n
 * Detailed usage refers to the related function(s): @ref page_examples_cpp_gui_osdpreview in @ref page_examples_cpp_gui. \n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * HANDLE hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * HANDLE hCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * MW_RESULT xr;
 * HOSD hOSD = MWCreateImage(t_channel, CAPTURE_WIDTH, CAPTURE_HEIGHT);
 * ...
 * xr = MWStartVideoCapture(t_channel, hCaptureEvent);
 * ...
 * HNOTIFY hNotify = MWRegisterNotify(t_channel, hNotifyEvent, MWCAP_NOTIFY_VIDEO_FRAME_BUFFERED);
 * ...
 * WaitForSingleObject(hNotifyEvent, INFINITE);
 * xr = MWGetNotifyStatus(t_channel, hNotify, &ullStatusBits);
 * MWCAP_VIDEO_BUFFER_INFO videoBufferInfo;
 * xr = MWGetVideoBufferInfo(t_channel, &videoBufferInfo);
 * MWCAP_VIDEO_FRAME_INFO videoFrameInfo;
 * xr = MWGetVideoFrameInfo(t_channel, videoBufferInfo.iNewestBufferedFullFrame, &videoFrameInfo);
 * xr=MWCaptureVideoFrameWithOSDToVirtualAddress(t_channel, MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERED, byBuffer, dwFrameSize, cbStride, FALSE, NULL, CAPTURE_COLOR, CAPTURE_WIDTH, CAPTURE_HEIGHT, hOSD, aOSDRects, 1); 
 * WaitForSingleObject(hCaptureEvent, INFINITE);
 * MWCAP_VIDEO_CAPTURE_STATUS captureStatus;
 * xr = MWGetVideoCaptureStatus(t_channel, &captureStatus);
 * ...
 * xr = MWUnregisterNotify(t_channel, hNotify);
 * xr = MWStopVideoCapture(t_channel);
 * ...
 * MWCloseImage(t_channel, hOSD, &lRet);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
*/
MW_RESULT
LIBMWCAPTURE_API
MWCaptureVideoFrameWithOSDToVirtualAddress(
	HCHANNEL 						hChannel,
	int								iFrame,
	LPBYTE							pbFrame,
	DWORD							cbFrame,
	DWORD							cbStride,
	BOOLEAN							bBottomUp,
	MWCAP_PTR64						pvContext,
	DWORD							dwFOURCC,
	int								cx,
	int								cy,
	HOSD							hOSDImage,
	const RECT *					pOSDRects,
	int								cOSDRects
	);

/**
 * @ingroup group_functions_procapture
 * @brief  Saves the captured video frame to physical memory and add OSD
 * @param[in] hChannel			Video channel handle that has started video capturing
 * @param[in] iFrame			Video frame number to be captured
 * @param[in] llFrameAddress	Physical addresses that store the captured video frames
 * @param[in] cbFrame			Byte length of memory that stores the captured data
 * @param[in] cbStride			Step of memory that stores the captured data
 * @param[in] bBottomUp			Whether to store the captured video frames from bottom to top
 * @param[in] pvContext			Custom context pointers
 * @param[in] dwFOURCC			Color format of captured video frames, see MWFOURCC.h.
 * @param[in] cx				width of captured video frames
 * @param[in] cy				Height of captured video frames
 * @param[in] hOSDImage			handle of OSD image which is geted using [MWCreateImage](@ref MWCreateImage).
 * @param[in] pOSDRects			Target area of OSD image 
 * @param[in] cOSDRects			Number of OSD image target areas
 * @return Function return values are as follows
 * <table>
 * <tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Be cautious when using it.\n
*/
MW_RESULT
LIBMWCAPTURE_API
MWCaptureVideoFrameWithOSDToPhysicalAddress(
	HCHANNEL 						hChannel,
	int								iFrame,
	LONGLONG						llFrameAddress,
	DWORD							cbFrame,
	DWORD							cbStride,
	BOOLEAN							bBottomUp,
	MWCAP_PTR64						pvContext,
	DWORD							dwFOURCC,
	int								cx,
	int								cy,
	HOSD							hOSDImage,
	const RECT *					pOSDRects,
	int								cOSDRects
	);

/**
 * @ingroup group_functions_procapture
 * @brief Save captured video frames to system memory. It is the extended MWCaptureVideoFrameToVirtualAddress.
 * @param[in] hChannel					Video channel handle that has started video capturing
 * @param[in] iFrame					Video frame number to be captured
 * @param[out] pbFrame					Physical addresses that store the captured video frames
 * @param[in] cbFrame					Byte length of memory that stores the captured data
 * @param[in] cbStride					Step of memory that stores the captured data
 * @param[in] bBottomUp					Whether to store the captured video frames from bottom to top
 * @param[in] pvContext					Custom context pointers
 * @param[in] dwFOURCC					Color format of captured video frames, see MWFOURCC.h.
 * @param[in] cx						width of captured video frames
 * @param[in] cy						Height of captured video frames
 * @param[in] dwProcessSwitchs			Image processing mask. Refers to MWCAP_VIDEO_PROCESS_xx
 * @param[in] cyParitalNotify			Number of rows captured by lines
 * @param[in] hOSDImage					OSD image handle which is geted using [MWCreateImage](@ref MWCreateImage).
 * @param[in] pOSDRects					Target area of OSD image 
 * @param[in] cOSDRects					Number of OSD image target areas
 * @param[in] sContrast					Contrast	[50,200]
 * @param[in] sBrightness				Brightness	[-100,100]
 * @param[in] sSaturation				Saturation	[0,200]
 * @param[in] sHue						Hue	[-90,90]
 * @param[in] deinterlaceMode			Deinterlace Mode
 * @param[in] aspectRatioConvertMode	Aspect Ratio Convert Mode
 * @param[in] pRectSrc					The source area of the image to capture
 * @param[in] pRectDest					The destination area of the image to capture
 * @param[in] nAspectX					Width of the aspect ratio
 * @param[in] nAspectY					Height of the aspect ratio
 * @param[in] colorFormat				Color Format
 * @param[in] quantRange				Quantization Range
 * @param[in] satRange					Saturation Range
 * @return Function return values are as follows
 * <table>
 * <tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Captures video frames and saves the data to system memory. The extended version of [MWCaptureVideoFrameToVirtualAddress](@ref MWCaptureVideoFrameToVirtualAddress) which has more parameters for controlling.\n
 * Related function(s):
 *  @ref page_examples_cpp_gui_avcapture and 
 *  @ref page_examples_cpp_gui_avcapturequad in @ref page_examples_cpp_gui.\n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * HANDLE hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * HANDLE hCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * MW_RESULT xr;
 * HOSD hOSD = MWCreateImage(t_channel, CAPTURE_WIDTH, CAPTURE_HEIGHT);
 * ...
 * xr = MWStartVideoCapture(t_channel, hCaptureEvent);
 * ...
 * HNOTIFY hNotify = MWRegisterNotify(t_channel, hNotifyEvent, MWCAP_NOTIFY_VIDEO_FRAME_BUFFERED);
 * ...
 * WaitForSingleObject(hNotifyEvent, INFINITE);
 * xr = MWGetNotifyStatus(t_channel, hNotify, &ullStatusBits);
 * MWCAP_VIDEO_BUFFER_INFO videoBufferInfo;
 * xr = MWGetVideoBufferInfo(t_channel, &videoBufferInfo);
 * MWCAP_VIDEO_FRAME_INFO videoFrameInfo;
 * xr = MWGetVideoFrameInfo(t_channel, videoBufferInfo.iNewestBufferedFullFrame, &videoFrameInfo);
 * xr =MWCaptureVideoFrameToVirtualAddressEx(t_channel,
 * 	MWCAP_VIDEO_FRAME_ID_NEWEST_BUFFERED,
 * 	m_p_buffer,m_cb_buffer,t_n_stride,FALSE,NULL,m_dw_fourcc,t_n_width,t_n_height,
 * 	0,0,NULL,NULL,0,100,0,100,0,MWCAP_VIDEO_DEINTERLACE_BLEND,
 * 	MWCAP_VIDEO_ASPECT_RATIO_IGNORE,NULL,NULL,
 * 	0,0,MWCAP_VIDEO_COLOR_FORMAT_UNKNOWN,
 * 	MWCAP_VIDEO_QUANTIZATION_UNKNOWN,
 * 	MWCAP_VIDEO_SATURATION_UNKNOWN);
 * MWCAP_VIDEO_CAPTURE_STATUS captureStatus;
 * xr = MWGetVideoCaptureStatus(t_channel, &captureStatus);
 * ...
 * xr = MWUnregisterNotify(t_channel, hNotify);
 * xr = MWStopVideoCapture(t_channel);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
*/
MW_RESULT
LIBMWCAPTURE_API
MWCaptureVideoFrameToVirtualAddressEx(
	HCHANNEL 						hChannel,
	int								iFrame,
	LPBYTE							pbFrame,
	DWORD							cbFrame,
	DWORD							cbStride,
	BOOLEAN							bBottomUp,
	MWCAP_PTR64						pvContext,
	DWORD							dwFOURCC,
	int								cx,
	int								cy,
	DWORD							dwProcessSwitchs,
	int								cyParitalNotify,
	HOSD							hOSDImage,
	const RECT *					pOSDRects,
	int								cOSDRects,
	SHORT							sContrast,
	SHORT							sBrightness,
	SHORT							sSaturation,
	SHORT							sHue,
	MWCAP_VIDEO_DEINTERLACE_MODE			deinterlaceMode,
	MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE	aspectRatioConvertMode,
	const RECT *							pRectSrc,
	const RECT *							pRectDest,
	int										nAspectX,
	int										nAspectY,
	MWCAP_VIDEO_COLOR_FORMAT				colorFormat,
	MWCAP_VIDEO_QUANTIZATION_RANGE			quantRange,
	MWCAP_VIDEO_SATURATION_RANGE			satRange
	);

/**
 * @ingroup group_functions_procapture
 * @brief Captures video frames and saves to physical memory. And is the extended version of [MWCaptureVideoFrameToPhysicalAddress](@ref MWCaptureVideoFrameToPhysicalAddress).
 * @param[in] hChannel					Video channel handle that has started video capturing
 * @param[in] iFrame					Video frame number to be captured
 * @param[in] llFrameAddress			Physical addresses that store the captured video frames
 * @param[in] cbFrame					Byte length of memory that stores the captured data
 * @param[in] cbStride					Step of memory that stores the captured data
 * @param[in] bBottomUp					Whether to store the captured video frames from bottom to top
 * @param[in] pvContext					Custom context pointers
 * @param[in] dwFOURCC					Color format of captured video frames, see MWFOURCC.h.
 * @param[in] cx						width of captured video frames
 * @param[in] cy						Height of captured video frames
 * @param[in] dwProcessSwitchs			Mask of video processing refers to MWCAP_VIDEO_PROCESS_xx
 * @param[in] cyParitalNotify			The number of lines when capturing by lines
 * @param[in] hOSDImage					OSD image handle which is geted using MWCreateImage(HCHANNEL hChannel,int cx,int cy)
 * @param[in] pOSDRects					Target area of OSD image 
 * @param[in] cOSDRects					Number of OSD image target areas
 * @param[in] sContrast					Contrast		
 * @param[in] sBrightness				Brightness			
 * @param[in] sSaturation				Saturation		
 * @param[in] sHue						Hue		
 * @param[in] deinterlaceMode			Deinterlace Mode	
 * @param[in] aspectRatioConvertMode	Aspect Ratio Convert Mode
 * @param[in] pRectSrc					The source area of the image to capture
 * @param[in] pRectDest					The destination area of the image to capture
 * @param[in] nAspectX					Width of the aspect ratio
 * @param[in] nAspectY					Height of the aspect ratio
 * @param[in] colorFormat				Color Format
 * @param[in] quantRange				Quantization Range
 * @param[in] satRange					Saturation Range
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Be cautious when using it.\n
*/
MW_RESULT
LIBMWCAPTURE_API
MWCaptureVideoFrameToPhysicalAddressEx(
	HCHANNEL 						hChannel,
	int								iFrame,
	LONGLONG						llFrameAddress,
	DWORD							cbFrame,
	DWORD							cbStride,
	BOOLEAN							bBottomUp,
	MWCAP_PTR64						pvContext,
	DWORD							dwFOURCC,
	int								cx,
	int								cy,
	DWORD							dwProcessSwitchs,
	int								cyParitalNotify,
	HOSD							hOSDImage,
	const RECT *					pOSDRects,
	int								cOSDRects,
	SHORT							sContrast,
	SHORT							sBrightness,
	SHORT							sSaturation,
	SHORT							sHue,
	MWCAP_VIDEO_DEINTERLACE_MODE			deinterlaceMode,
	MWCAP_VIDEO_ASPECT_RATIO_CONVERT_MODE	aspectRatioConvertMode,
	const RECT *							pRectSrc,
	const RECT *							pRectDest,
	int										nAspectX,
	int										nAspectY,
	MWCAP_VIDEO_COLOR_FORMAT				colorFormat,
	MWCAP_VIDEO_QUANTIZATION_RANGE			quantRange,
	MWCAP_VIDEO_SATURATION_RANGE			satRange
	);

/**
 * @ingroup group_functions_procapture
 * @brief Gets video bufferring information
 * @param[in] hChannel      			Video channel handle that has started video capturing
 * @param[out] pVideoBufferInfo      	Video bufferring information
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Gets video buffer on-board storage information that gives you the current number of video buffer frames and the frame id being cached.\n
 * Detailed usage refers to the related function(s):
 *  @ref page_examples_cpp_cmd_capturebyinput and 
 *  @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd.\n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 *  [MWRegisterTimer](@ref MWRegisterTimer) \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetVideoBufferInfo(
	HCHANNEL 						hChannel,
	MWCAP_VIDEO_BUFFER_INFO *		pVideoBufferInfo
	);

/**
 * @ingroup group_functions_procapture
 * @brief Gets video frame information
 * @param[in] hChannel      		Video channel handle that has started video capturing
 * @param[in] i      		 		Video frame index
 * @param[out] pVideoFrameInfo     	Returns video frame information
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Gets video frame information, including bufferring start and end time with time code.\n
 * Detailed usage refers to the related function(s): @ref page_examples_cpp_cmd of
 *  @ref page_examples_cpp_cmd_capturebyinput and 
 *  @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd.\n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 *  [MWRegisterTimer](@ref MWRegisterTimer)\n
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetVideoFrameInfo(
	HCHANNEL 						hChannel,
	BYTE							i,
	MWCAP_VIDEO_FRAME_INFO*			pVideoFrameInfo
	);

/**
 * @ingroup group_functions_procapture
 * @brief Gets video capture state, and frees up system storage.
 * @param[in] hChannel      Video channel handle that has started video capturing
 * @param[out] pStatus      Returns video capture state
 * @return Function return values are as follows:
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Gets video capture state. After the capture function call is completed, call the function, which gets the capture state while cleaning up the occupied system storage.\n
 * Detailed usage refers to the related function(s): 
 *  @ref page_examples_cpp_cmd_capturebyinput and
 *  @ref page_examples_cpp_cmd_capturebytimer in @ref page_examples_cpp_cmd.\n
 * Related function(s): [MWRegisterNotify](@ref MWRegisterNotify) \n
 *  [MWRegisterTimer](@ref MWRegisterTimer) \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetVideoCaptureStatus(
	HCHANNEL 						hChannel,
	MWCAP_VIDEO_CAPTURE_STATUS *	pStatus
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Starts audio capturing of specified channel
 * @param[in] hChannel      Opened channel handle
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Starts audio capturing of specified channel.\n
 * The usage of the API refers to @ref page_examples_cpp_cmd_audiocapture in @ref page_examples_cpp_cmd.  \n
 * @code
 * WCHAR path[128];
 * MWGetDevicePath(k,path);
 * HCHANNEL t_channel=MWOpenChannelByPath(path);
 * ...
 * HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 * HNOTIFY hNotify=MWRegisterNotify(t_channel, hEvent, MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE | MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED);
 * ...
 * xr = MWStartAudioCapture(t_channel);
 * ...
 * WaitForSingleObject(hEvent, INFINITE);
 * ULONGLONG ullStatusBits = 0LL;
 * MWGetNotifyStatus(t_channel, hNotify, &ullStatusBits);
 * if (ullStatusBits & MWCAP_NOTIFY_AUDIO_FRAME_BUFFERED) {
 * 	MWCAP_AUDIO_CAPTURE_FRAME audioFrame;
 * 	xr = MWCaptureAudioFrame(t_channel, &audioFrame);
 * 	...
 * }
 * ...
 * xr = MWUnregisterNotify(t_channel, hNotify);
 * xr = MWStopVideoCapture(t_channel);
 * ...
 * MWCloseChannel(t_channel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWStartAudioCapture(
	HCHANNEL 						hChannel
	);	

/**
 * @ingroup group_functions_proecocapture
 * @brief Stops audio capturing of specified channel
 * @param[in] hChannel      channel handle of opened audio capture
 * @return Function return values are as follows
 * <table>
<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Stops audio capturing of specified channel.\n
 * Usage:\nRelated function(s): @ref page_examples_cpp_cmd_audiocapture in @ref page_examples_cpp_cmd. \n
 * The API is used together with [MWStartAudioCapture](@ref MWStartAudioCapture).
 */
MW_RESULT
LIBMWCAPTURE_API
MWStopAudioCapture(
	HCHANNEL 						hChannel
	);
	
/**
 * @ingroup group_functions_proecocapture
 * @brief Captures an audio frame
 * @param[in] hChannel      			Channel handle of opened audio capture
 * @param[out] pAudioCaptureFrame      	Returns an audio frame
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Captures audio frames. \n
 * The usage of the API refers to @ref page_examples_cpp_cmd_audiocapturein @ref page_examples_cpp_cmd and
 * @ref page_examples_cpp_gui_multiaudiocapture in @ref page_examples_cpp_gui. \n
 * Related function(s): [MWStartAudioCapture](@ref MWStartAudioCapture) \n
 * For LPCM to capture up to 8 channel data, the channel correspondence are 0L, 1L, 2L, 3L, 0R, 1R, 2R, 3R.
 */
MW_RESULT
LIBMWCAPTURE_API
MWCaptureAudioFrame(
	HCHANNEL 						hChannel,
	MWCAP_AUDIO_CAPTURE_FRAME*	pAudioCaptureFrame
	);

/**
 * @ingroup group_functions_procapture
 * @brief Creates OSD image
 * @param[in] hChannel      Opened channel handle
 * @param[in] cx      		Width of image
 * @param[in] cy     		Height of image
 * @return Returns image handle if succeeded, otherwise returns NULL.\n
 * @details Usage：\n
 * Creates OSD image.\n
 * Detailed usage refers to the related function(s): @ref page_examples_cpp_gui_osdpreview in @ref page_examples_cpp_gui.
 * Related function(s): [MWCaptureVideoFrameWithOSDToVirtualAddress](@ref MWCaptureVideoFrameWithOSDToVirtualAddress) \n
*/
//only rgba image
HOSD
LIBMWCAPTURE_API
MWCreateImage(
	HCHANNEL 						hChannel,
	int								cx,
	int								cy
	);

/**
 * @ingroup group_functions_procapture
 * @brief Opens OSD images.
 * @param[in] hChannel      Opened channel handle
 * @param[in] hImage      	Image handle
 * @param[out] plRet        Returns counts of the image being called  
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Opens OSD image, and loads it to specified channel.\n
 * @code
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * HOSD hOSD = NULL;
 * hOSD =MWCreateImage(hChannel, 1920, 1080);
 * ...
 * HCHANNEL hChannel1 = MWOpenChannelByPath(pDevicePath);
 * ...
 * LONG lRet = 0;
 * mr = MWOpenImage(hChannel1, hOSD, &lRet);
 * ...
 * mr = MWCloseImage(hChannel1, hOSD, &lRet);
 * mr = MWCloseImage(hChannel, hOSD, &lRet);
 * ...
 * MWCloseChannel(hChannel1);
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWOpenImage(
	HCHANNEL 						hChannel,
	HOSD							hImage,
	LONG*							plRet
	);

/**
 * @ingroup group_functions_procapture
 * @brief Turns off the OSD image.
 * @param[in] hChannel      Opened channel handle
 * @param[in] hImage        Image handle
 * @param[out] plRet     	Returns counts of the image being called  
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Turns off the OSD image and frees the OSD occupied sources.\n
 * The usage refers to the related function(s): @ref page_examples_cpp_gui_osdpreview in @ref page_examples_cpp_gui. \n
 * Related function(s): [MWOpenImage](@ref MWOpenImage) \n
 *  Using [MWCreateImage](@ref MWCreateImage) needs to call [MWCloseImage](@ref MWCloseImage). \n
 *  Using [MWOpenImage](@ref MWOpenImage) needs to call [MWCloseImage](@ref MWCloseImage). \n
 */
MW_RESULT
LIBMWCAPTURE_API
MWCloseImage(
	HCHANNEL 						hChannel,
	HOSD							hImage,
	LONG*							plRet
	);

/**
 * @ingroup group_functions_procapture
 * @brief Uploads images from system memory to capture device.
 * @param[in] hChannel      			Opened channel handle
 * @param[in] hImage      	 			Image handle
 * @param[in] cfDest     	 			Color format of the target image
 * @param[in] quantRangeDest      		Quantization range of the target image
 * @param[in] satRangeDest      		Saturation range of the target image
 * @param[in] xDest     				Horizontal target position
 * @param[in] yDest      				Vertical target position
 * @param[in] cxDest      				Target width of image
 * @param[in] cyDest     				Target height of image
 * @param[in] pvSrcFrame      			Source image 
 * @param[in] cbSrcFrame      			Data length of source image
 * @param[in] cbSrcStride     			Data step of source image
 * @param[in] cxSrc      				Width of source image
 * @param[in] cySrc      				Height of source image
 * @param[in] bSrcBottomUp     			Whether the source image is up-side-down
 * @param[in] bSrcPixelAlpha      		Whether there is an alpha component in the source image
 * @param[in] bSrcPixelXBGR      		Whether the color format of the source image is XBGR
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.</td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s). </td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Uploads images from system memory to capture device.\n
 * The usage refers to @ref page_examples_cpp_gui_osdpreview in @ref page_examples_cpp_gui.\n
 */
MW_RESULT
LIBMWCAPTURE_API
MWUploadImageFromVirtualAddress(
	HCHANNEL 						hChannel,
	HOSD							hImage,
	MWCAP_VIDEO_COLOR_FORMAT		cfDest,
	MWCAP_VIDEO_QUANTIZATION_RANGE	quantRangeDest,
	MWCAP_VIDEO_SATURATION_RANGE	satRangeDest,
	WORD							xDest,
	WORD							yDest,
	WORD							cxDest,
	WORD							cyDest,
	MWCAP_PTR64						pvSrcFrame,
	DWORD							cbSrcFrame,
	DWORD							cbSrcStride,
	WORD							cxSrc,
	WORD							cySrc,
	BOOLEAN							bSrcBottomUp,
	BOOLEAN							bSrcPixelAlpha,
	BOOLEAN							bSrcPixelXBGR
	);

/**
 * @ingroup group_functions_procapture
 * @brief Uploads an image from physical memory to capture device.
 * @param[in] hChannel      			Opened channel handle
 * @param[in] hImage      	 			Image handle
 * @param[in] cfDest     	 			Color format of the target image
 * @param[in] quantRangeDest      		Quantization range of the target image
 * @param[in] satRangeDest      		Saturation range of the target image
 * @param[in] xDest     				Horizontal target position
 * @param[in] yDest      				Vertical target position
 * @param[in] cxDest      				Target width of image
 * @param[in] cyDest     				Target height of image
 * @param[in] llSrcFrameAddress      	Physical address that stores the source image 
 * @param[in] cbSrcFrame      			Data length of source image
 * @param[in] cbSrcStride     			Data step of source image
 * @param[in] cxSrc      				Width of source image
 * @param[in] cySrc      			    Height of source image
 * @param[in] bSrcBottomUp     			Whether the source image is up-side-down
 * @param[in] bSrcPixelAlpha      		Whether there is an alpha component in the source image
 * @param[in] bSrcPixelXBGR      		Whether the color format of the source image is XBGR
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Uploads an image from physical memory to capture device.\n
 * Be cautious when using it.\n
*/
MW_RESULT
LIBMWCAPTURE_API
MWUploadImageFromPhysicalAddress(
	HCHANNEL 						hChannel,
	HOSD							hImage,
	MWCAP_VIDEO_COLOR_FORMAT		cfDest,
	MWCAP_VIDEO_QUANTIZATION_RANGE	quantRangeDest,
	MWCAP_VIDEO_SATURATION_RANGE	satRangeDest,
	WORD							xDest,
	WORD							yDest,
	WORD							cxDest,
	WORD							cyDest,
	LONGLONG						llSrcFrameAddress,
	DWORD							cbSrcFrame,
	DWORD							cbSrcStride,
	WORD							cxSrc,
	WORD							cySrc,
	BOOLEAN							bSrcBottomUp,
	BOOLEAN							bSrcPixelAlpha,
	BOOLEAN							bSrcPixelXBGR
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Gets the temperature of capture card.
 * @param[in] hChannel      		Opened channel handle
 * @param[out] pnTemp     	  		Capture card temperature
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * @code
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * Unsigned int t_temp;
 * MWGetTemperature(hChannel ,&t_temp);
 * ...
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetTemperature(
	HCHANNEL                        hChannel,
	unsigned int*                   pnTemp
	);

/**
 * @ingroup group_functions_procapture
 * @brief Gets number of custom video timings.  
 * @param[in] hChannel      		Opened channel handle
 * @param[out] dwCount				Number of custom video timing  
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.</td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s). </td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * @code
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * DWORD t_dw_count;
 * MWGetCustomVideoTimingsCount(hChannel, &t_dw_count);
 * ...
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
	LIBMWCAPTURE_API
	MWGetCustomVideoTimingsCount(
	HCHANNEL						hChannel,
	DWORD * 						dwCount
	);

/**
 * @ingroup group_functions_procapture
 * @brief Gets number of custom video resolutions.
 * @param[in] hChannel      		Opened channel handle
 * @param[out] dwCount				Number of custom video resolution 
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * @code
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * DWORD t_dw_count;
 * MWGetCustomVideoResolutionsCount(hChannel, &t_dw_count);
 * ...
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
	LIBMWCAPTURE_API
	MWGetCustomVideoResolutionsCount(
	HCHANNEL							hChannel,
	DWORD *								dwCount
	);

/**
 * @ingroup group_functions_proecocapture
 * @brief Gets extended information of channels.
 * @param[in] hChannel      		Opened channel handle
 * @param[in] guid					Extended information flag
 * @param[out] ppObject			 	Extended information structure
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * @code
 * DEFINE_GUID( GUID_MWDATA, 0x2df41b52, 0x4d59, 0x4596, 0x9e, 0x56, 0xce, 0x84, 0x8a, 0xb2, 0x3c, 0x43);
 * ...
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * IExtendObject* pExtend = NULL;
 * MWCreateExtendObject(hChannel, GUID_MWDATA, (LPVOID*)&pExtend);
 * BYTE byBuffer[32];
 * pExtend->GetUserData(byBuffer, 32);
 * pExtend->Release();
 * printf("UserData: %s\n\n", (char*)byBuffer);
 * ...
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWCreateExtendObject(
	HCHANNEL						hChannel,
	GUID							guid,
	LPVOID*							ppObject
	);

/**
 * @ingroup group_functions_procapture
 * @brief Gets OSD settings. 
 * @param[in] hChannel      		Opened channel handle
 * @param[out] paOSDSettings		OSD settings  
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * Only png and rgba images are supported.\n
 * @code
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * MWCAP_VIDEO_OSD_SETTINGS t_osd_setting;
 * 	MWGetVideoCaptureOSDSettings(hChannel, &t_osd_setting);
 * ...
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWGetVideoCaptureOSDSettings(
	HCHANNEL						hChannel,
	MWCAP_VIDEO_OSD_SETTINGS*		paOSDSettings
	);

/**
 * @ingroup group_functions_procapture
 * @brief Sets OSD configurations.
 * @param[in] hChannel      		Opened channel handle
 * @param[out] paOSDSettings		OSD configurations
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * @code
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * MWCAP_VIDEO_OSD_SETTINGS t_osd_setting;
 * t_osd_setting.xx=...;
 * ...
 * MWSetVideoCaptureOSDSettings(hChannel, &t_osd_setting);
 * ...
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
LIBMWCAPTURE_API
MWSetVideoCaptureOSDSettings(
	HCHANNEL						hChannel,
	MWCAP_VIDEO_OSD_SETTINGS*		paOSDSettings
	);

/**
 * @ingroup group_functions_procapture
 * @brief Gets connection format of the preview pin.
 * @param[in] hChannel      		Opened channel handle
 * @param[out] paConnectionFormat	Connection format of the preview pin
 * @return Function return values are as follows
 * <table>
 * 	<tr>
 * 		<td> #MW_SUCCEEDED </td>
 * 		<td> Function call succeeded.  </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_FAILED </td>
 * 		<td> Function call failed. </td>
 * 	</tr>
 * 	<tr>
 * 		<td> #MW_INVALID_PARAMS </td>
 * 		<td> Input invalid value(s).</td>
 * 	</tr>
 * </table>
 * @details Usage:\n
 * @code
 * WCHAR pDevicePath[128] = {0};
 * mr = MWGetDevicePath(i, pDevicePath);
 * ...
 * HCHANNEL hChannel = MWOpenChannelByPath(pDevicePath);
 * ...
 * MWCAP_VIDEO_CONNECTION_FORMAT t_fmt;
 * MWGetVideoPreviewConnectionFormat(hChannel, &t_fmt);
 * ...
 * MWCloseChannel(hChannel);
 * @endcode
 */
MW_RESULT
	LIBMWCAPTURE_API
	MWGetVideoPreviewConnectionFormat(
	HCHANNEL							hChannel,
	MWCAP_VIDEO_CONNECTION_FORMAT*		paConnectionFormat
	);

#ifdef __cplusplus
}
#endif
