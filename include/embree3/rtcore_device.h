// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "rtcore_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* Defines an opaque device type */
typedef struct RTCDeviceTy* RTCDevice;

/* Creates a new Embree device. */
RTCORE_API RTCDevice rtcNewDevice(const char* cfg);

/* Retains the Embree device (increments reference count) */
RTCORE_API void rtcRetainDevice(RTCDevice device);
  
/* Releases an Embree device (decrements reference count) */
RTCORE_API void rtcReleaseDevice(RTCDevice device);

/* Device properties */
enum RTCDeviceProperty
{
  RTC_DEVICE_PROPERTY_SOFTWARE_CACHE_SIZE = 0,
  
  RTC_DEVICE_PROPERTY_NATIVE_RAY4_SUPPORTED = 2,
  RTC_DEVICE_PROPERTY_NATIVE_RAY8_SUPPORTED = 3,
  RTC_DEVICE_PROPERTY_NATIVE_RAY16_SUPPORTED = 4,
  RTC_DEVICE_PROPERTY_INTERSECT_STREAM_SUPPORTED = 5,

  RTC_DEVICE_PROPERTY_RAY_MASK_SUPPORTED = 6,
  RTC_DEVICE_PROPERTY_BACKFACE_CULLING_ENABLED = 7,
  RTC_DEVICE_PROPERTY_FILTER_FUNCTION_SUPPORTED = 8,
  RTC_DEVICE_PROPERTY_IGNORE_INVALID_RAYS_ENABLED = 11,
  RTC_DEVICE_PROPERTY_TASKING_SYSTEM = 12,

  RTC_DEVICE_PROPERTY_VERSION_MAJOR = 13,
  RTC_DEVICE_PROPERTY_VERSION_MINOR = 14,
  RTC_DEVICE_PROPERTY_VERSION_PATCH = 15,
  RTC_DEVICE_PROPERTY_VERSION = 16,

  RTC_DEVICE_PROPERTY_TRIANGLE_GEOMETRY_SUPPORTED = 17,
  RTC_DEVICE_PROPERTY_QUAD_GEOMETRY_SUPPORTED = 18,
  RTC_DEVICE_PROPERTY_CURVE_GEOMETRY_SUPPORTED = 19,
  RTC_DEVICE_PROPERTY_SUBDIVISION_GEOMETRY_SUPPORTED = 20,
  RTC_DEVICE_PROPERTY_USER_GEOMETRY_SUPPORTED = 21,

  RTC_DEVICE_PROPERTY_JOIN_COMMIT_SUPPORTED = 30,
};

/* Sets a device property. */
RTCORE_API void rtcSetDeviceProperty(RTCDevice device, const enum RTCDeviceProperty prop, ssize_t val);

/* Gets a device property. */
RTCORE_API ssize_t rtcGetDeviceProperty(RTCDevice device, const enum RTCDeviceProperty prop);

/* Error codes. */
enum RTCError
{
  RTC_ERROR_NONE = 0,
  RTC_ERROR_UNKNOWN = 1,
  RTC_ERROR_INVALID_ARGUMENT = 2,
  RTC_ERROR_INVALID_OPERATION = 3,
  RTC_ERROR_OUT_OF_MEMORY = 4,
  RTC_ERROR_UNSUPPORTED_CPU = 5,
  RTC_ERROR_CANCELLED = 6,
};

/* Returns the error code. */
RTCORE_API enum RTCError rtcGetDeviceError(RTCDevice device);

/* Type of error callback function. */
typedef void (*RTCErrorFunction)(void* userPtr, const enum RTCError code, const char* str);

/* Sets an error callback function */
RTCORE_API void rtcSetDeviceErrorFunction(RTCDevice device, RTCErrorFunction func, void* userPtr);

/* Type of memory monitor callback function. */
typedef bool (*RTCMemoryMonitorFunction)(void* ptr, const ssize_t bytes, const bool post);

/* Sets the memory monitor callback function. */
RTCORE_API void rtcSetDeviceMemoryMonitorFunction(RTCDevice device, RTCMemoryMonitorFunction func, void* userPtr);

#if defined(__cplusplus)
}
#endif