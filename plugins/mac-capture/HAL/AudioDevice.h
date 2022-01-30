/*
 Copyright © 2020 Apple Inc.

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//==================================================================================================
//	Includes
//==================================================================================================

//	System Includes
#include <CoreAudio/AudioServerPlugIn.h>
#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/syslog.h>

//==================================================================================================
#pragma mark -
#pragma mark Macros
//==================================================================================================

#if TARGET_RT_BIG_ENDIAN
#define FourCCToCString(the4CC)                                       \
	{                                                             \
		((char *)&the4CC)[0], ((char *)&the4CC)[1],           \
			((char *)&the4CC)[2], ((char *)&the4CC)[3], 0 \
	}
#else
#define FourCCToCString(the4CC)                                       \
	{                                                             \
		((char *)&the4CC)[3], ((char *)&the4CC)[2],           \
			((char *)&the4CC)[1], ((char *)&the4CC)[0], 0 \
	}
#endif

#if DEBUG

#define DebugMsg(inFormat, ...) printf(inFormat "\n", ##__VA_ARGS__)

#define FailIf(inCondition, inHandler, inMessage) \
	if (inCondition) {                        \
		DebugMsg(inMessage);              \
		goto inHandler;                   \
	}

#define FailWithAction(inCondition, inAction, inHandler, inMessage) \
	if (inCondition) {                                          \
		DebugMsg(inMessage);                                \
		{                                                   \
			inAction;                                   \
		}                                                   \
		goto inHandler;                                     \
	}

#else

#define DebugMsg(inFormat, ...)

#define FailIf(inCondition, inHandler, inMessage) \
	if (inCondition) {                        \
		goto inHandler;                   \
	}

#define FailWithAction(inCondition, inAction, inHandler, inMessage) \
	if (inCondition) {                                          \
		{                                                   \
			inAction;                                   \
		}                                                   \
		goto inHandler;                                     \
	}

#endif

//==================================================================================================
#pragma mark -
#pragma mark ODAC State
//==================================================================================================

/* clang-format off */
//	Declare the internal object ID numbers for all the objects this driver implements. Note that
//	this driver has a fixed set of objects that never grows or shrinks. If this were not the case,
//	the driver would need to have a means to dynamically allocate these IDs. It's important to
//	realize that a lot of the structure of this driver is vastly simpler when the IDs are all
//	known a priori. Comments in the code will try to identify some of these simplifications and
//	point out what a more complicated driver will need to do.
enum
{
	kObjectID_PlugIn					= kAudioObjectPlugInObject,
	kObjectID_Box						= 2,
	kObjectID_Device					= 3,
	kObjectID_Stream_Input				= 4,
	kObjectID_Volume_Input_Master		= 5,
	kObjectID_Mute_Input_Master			= 6,
	kObjectID_DataSource_Input_Master	= 7,
	kObjectID_Stream_Output				= 8,
	kObjectID_Volume_Output_Master		= 9,
	kObjectID_Mute_Output_Master		= 10,
	kObjectID_DataSource_Output_Master	= 11,
	kObjectID_DataDestination_PlayThru_Master	= 12
};

//	Declare the stuff that tracks the state of the plug-in, the device and its sub-objects.
//	Note that we use global variables here because this driver only ever has a single device. If
//	multiple devices were supported, this state would need to be encapsulated in one or more structs
//	so that each object's state can be tracked individually.
//	Note also that we share a single mutex across all objects to be thread safe for the same reason.
#define										kPlugIn_BundleID				"com.obsproject.hal-plugin"
static pthread_mutex_t						gPlugIn_StateMutex				= PTHREAD_MUTEX_INITIALIZER;
static UInt32								gPlugIn_RefCount				= 0;
static AudioServerPlugInHostRef				gPlugIn_Host					= NULL;
static const AudioObjectPropertySelector	kPlugIn_CustomPropertyID		= 'PCst';

#define										kBox_UID						"ODACBox_UID"
static CFStringRef							gBox_Name						= NULL;
static Boolean								gBox_Acquired					= true;

#define										kDevice_UID						"ODACDevice_UID"
#define										kDevice_ModelUID				"ODACDevice_ModelUID"
static pthread_mutex_t						gDevice_IOMutex					= PTHREAD_MUTEX_INITIALIZER;
static Float64								gDevice_SampleRate				= 44100.0;
static UInt64								gDevice_IOIsRunning				= 0;
static const UInt32							kDevice_RingBufferSize			= 16384;
static Float64								gDevice_HostTicksPerFrame		= 0.0;
static UInt64								gDevice_NumberTimeStamps		= 0;
static Float64								gDevice_AnchorSampleTime		= 0.0;
static UInt64								gDevice_AnchorHostTime			= 0;

static bool									gStream_Input_IsActive			= true;
static bool									gStream_Output_IsActive			= true;

static const Float32						kVolume_MinDB					= -96.0;
static const Float32						kVolume_MaxDB					= 6.0;
static Float32								gVolume_Input_Master_Value		= 0.0;
static Float32								gVolume_Output_Master_Value		= 0.0;

static bool									gMute_Input_Master_Value		= false;
static bool									gMute_Output_Master_Value		= false;

static const UInt32							kDataSource_NumberItems			= 4;
#define										kDataSource_ItemNamePattern		"Data Source Item %d"
static UInt32								gDataSource_Input_Master_Value	= 0;
static UInt32								gDataSource_Output_Master_Value	= 0;
static UInt32								gDataDestination_PlayThru_Master_Value	= 0;

//==================================================================================================
#pragma mark -
#pragma mark AudioServerPlugInDriverInterface Implementation
//==================================================================================================

#pragma mark Prototypes

//	Entry points for the COM methods
void*				ODAC_Create(CFAllocatorRef inAllocator, CFUUIDRef inRequestedTypeUUID);
static HRESULT		ODAC_QueryInterface(void* inDriver, REFIID inUUID, LPVOID* outInterface);
static ULONG		ODAC_AddRef(void* inDriver);
static ULONG		ODAC_Release(void* inDriver);
static OSStatus		ODAC_Initialize(AudioServerPlugInDriverRef inDriver, AudioServerPlugInHostRef inHost);
static OSStatus		ODAC_CreateDevice(AudioServerPlugInDriverRef inDriver, CFDictionaryRef inDescription, const AudioServerPlugInClientInfo* inClientInfo, AudioObjectID* outDeviceObjectID);
static OSStatus		ODAC_DestroyDevice(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID);
static OSStatus		ODAC_AddDeviceClient(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo);
static OSStatus		ODAC_RemoveDeviceClient(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo);
static OSStatus		ODAC_PerformDeviceConfigurationChange(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo);
static OSStatus		ODAC_AbortDeviceConfigurationChange(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo);
static Boolean		ODAC_HasProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus		ODAC_IsPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus		ODAC_GetPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus		ODAC_GetPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus		ODAC_SetPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData);
static OSStatus		ODAC_StartIO(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID);
static OSStatus		ODAC_StopIO(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID);
static OSStatus		ODAC_GetZeroTimeStamp(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, Float64* outSampleTime, UInt64* outHostTime, UInt64* outSeed);
static OSStatus		ODAC_WillDoIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, Boolean* outWillDo, Boolean* outWillDoInPlace);
static OSStatus		ODAC_BeginIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo);
static OSStatus		ODAC_DoIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, AudioObjectID inStreamObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo, void* ioMainBuffer, void* ioSecondaryBuffer);
static OSStatus		ODAC_EndIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo);

//	Implementation
static Boolean		ODAC_HasPlugInProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus		ODAC_IsPlugInPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus		ODAC_GetPlugInPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus		ODAC_GetPlugInPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus		ODAC_SetPlugInPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean		ODAC_HasBoxProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus		ODAC_IsBoxPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus		ODAC_GetBoxPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus		ODAC_GetBoxPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus		ODAC_SetBoxPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean		ODAC_HasDeviceProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus		ODAC_IsDevicePropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus		ODAC_GetDevicePropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus		ODAC_GetDevicePropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus		ODAC_SetDevicePropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean		ODAC_HasStreamProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus		ODAC_IsStreamPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus		ODAC_GetStreamPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus		ODAC_GetStreamPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus		ODAC_SetStreamPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);

static Boolean		ODAC_HasControlProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress);
static OSStatus		ODAC_IsControlPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);
static OSStatus		ODAC_GetControlPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);
static OSStatus		ODAC_GetControlPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);
static OSStatus		ODAC_SetControlPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, AudioObjectPropertyAddress outChangedAddresses[2]);
/* clang-format on */

#pragma mark The Interface

static AudioServerPlugInDriverInterface gAudioServerPlugInDriverInterface = {
	NULL,
	ODAC_QueryInterface,
	ODAC_AddRef,
	ODAC_Release,
	ODAC_Initialize,
	ODAC_CreateDevice,
	ODAC_DestroyDevice,
	ODAC_AddDeviceClient,
	ODAC_RemoveDeviceClient,
	ODAC_PerformDeviceConfigurationChange,
	ODAC_AbortDeviceConfigurationChange,
	ODAC_HasProperty,
	ODAC_IsPropertySettable,
	ODAC_GetPropertyDataSize,
	ODAC_GetPropertyData,
	ODAC_SetPropertyData,
	ODAC_StartIO,
	ODAC_StopIO,
	ODAC_GetZeroTimeStamp,
	ODAC_WillDoIOOperation,
	ODAC_BeginIOOperation,
	ODAC_DoIOOperation,
	ODAC_EndIOOperation};
static AudioServerPlugInDriverInterface *gAudioServerPlugInDriverInterfacePtr =
	&gAudioServerPlugInDriverInterface;
static AudioServerPlugInDriverRef gAudioServerPlugInDriverRef =
	&gAudioServerPlugInDriverInterfacePtr;
