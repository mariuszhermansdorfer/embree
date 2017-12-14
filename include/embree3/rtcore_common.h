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

#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32)
#if defined(_M_X64)
typedef long long ssize_t;
#else
typedef int ssize_t;
#endif
#endif

#ifndef RTCORE_API
#if defined(_WIN32) && !defined(EMBREE_STATIC_LIB)
#  define RTCORE_API __declspec(dllimport) 
#else
#  define RTCORE_API 
#endif
#endif

#ifdef _WIN32
#  define RTCORE_ALIGN(...) __declspec(align(__VA_ARGS__))
#else
#  define RTCORE_ALIGN(...) __attribute__((aligned(__VA_ARGS__)))
#endif

#if !defined (RTCORE_DEPRECATED)
#ifdef __GNUC__
  #define RTCORE_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
  #define RTCORE_DEPRECATED __declspec(deprecated)
#else
  #define RTCORE_DEPRECATED
#endif
#endif

#if defined(_WIN32) 
#  define RTCORE_FORCEINLINE __forceinline
#else
#  define RTCORE_FORCEINLINE inline __attribute__((always_inline))
#endif

/* invalid geometry ID */
#define RTC_INVALID_GEOMETRY_ID ((unsigned int)-1)

/* maximal number of time steps */
#define RTC_MAX_TIME_STEP_COUNT 129

/* maximal number of instancing levels */
#define RTC_MAX_INSTANCE_LEVELS 1
  
/* Specifies the format of buffers and other data structures */
enum RTCFormat
{
  RTC_FORMAT_UNDEFINED = 0,

  RTC_FORMAT_UCHAR = 0x101,
  RTC_FORMAT_UCHAR2,
  RTC_FORMAT_UCHAR3,
  RTC_FORMAT_UCHAR4,

  RTC_FORMAT_CHAR = 0x201,
  RTC_FORMAT_CHAR2,
  RTC_FORMAT_CHAR3,
  RTC_FORMAT_CHAR4,

  RTC_FORMAT_USHORT = 0x301,
  RTC_FORMAT_USHORT2,
  RTC_FORMAT_USHORT3,
  RTC_FORMAT_USHORT4,

  RTC_FORMAT_SHORT = 0x401,
  RTC_FORMAT_SHORT2,
  RTC_FORMAT_SHORT3,
  RTC_FORMAT_SHORT4,

  RTC_FORMAT_UINT = 0x501,
  RTC_FORMAT_UINT2,
  RTC_FORMAT_UINT3,
  RTC_FORMAT_UINT4,

  RTC_FORMAT_INT = 0x601,
  RTC_FORMAT_INT2,
  RTC_FORMAT_INT3,
  RTC_FORMAT_INT4,

  RTC_FORMAT_ULLONG = 0x701,
  RTC_FORMAT_ULLONG2,
  RTC_FORMAT_ULLONG3,
  RTC_FORMAT_ULLONG4,

  RTC_FORMAT_LLONG = 0x801,
  RTC_FORMAT_LLONG2,
  RTC_FORMAT_LLONG3,
  RTC_FORMAT_LLONG4,

  RTC_FORMAT_FLOAT = 0x901,
  RTC_FORMAT_FLOAT2,
  RTC_FORMAT_FLOAT3,
  RTC_FORMAT_FLOAT4,
  RTC_FORMAT_FLOAT5,
  RTC_FORMAT_FLOAT6,
  RTC_FORMAT_FLOAT7,
  RTC_FORMAT_FLOAT8,
  RTC_FORMAT_FLOAT9,
  RTC_FORMAT_FLOAT10,
  RTC_FORMAT_FLOAT11,
  RTC_FORMAT_FLOAT12,
  RTC_FORMAT_FLOAT13,
  RTC_FORMAT_FLOAT14,
  RTC_FORMAT_FLOAT15,
  RTC_FORMAT_FLOAT16,

  RTC_FORMAT_FLOAT2X2_ROW_MAJOR = 0xa22,
  RTC_FORMAT_FLOAT2X3_ROW_MAJOR = 0xa23,
  RTC_FORMAT_FLOAT2X4_ROW_MAJOR = 0xa24,
  RTC_FORMAT_FLOAT3X2_ROW_MAJOR = 0xa32,
  RTC_FORMAT_FLOAT3X3_ROW_MAJOR = 0xa33,
  RTC_FORMAT_FLOAT3X4_ROW_MAJOR = 0xa34,
  RTC_FORMAT_FLOAT4X2_ROW_MAJOR = 0xa42,
  RTC_FORMAT_FLOAT4X3_ROW_MAJOR = 0xa43,
  RTC_FORMAT_FLOAT4X4_ROW_MAJOR = 0xa44,

  RTC_FORMAT_FLOAT2X2_COLUMN_MAJOR = 0xb22,
  RTC_FORMAT_FLOAT2X3_COLUMN_MAJOR = 0xb23,
  RTC_FORMAT_FLOAT2X4_COLUMN_MAJOR = 0xb24,
  RTC_FORMAT_FLOAT3X2_COLUMN_MAJOR = 0xb32,
  RTC_FORMAT_FLOAT3X3_COLUMN_MAJOR = 0xb33,
  RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR = 0xb34,
  RTC_FORMAT_FLOAT4X2_COLUMN_MAJOR = 0xb42,
  RTC_FORMAT_FLOAT4X3_COLUMN_MAJOR = 0xb43,
  RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR = 0xb44
};

/* Axis aligned bounding box representation */
struct RTCORE_ALIGN(16) RTCBounds
{
  float lower_x, lower_y, lower_z, align0;
  float upper_x, upper_y, upper_z, align1;
};

/* Linear axis aligned bounding box representation */
struct RTCORE_ALIGN(16) RTCLinearBounds
{
  struct RTCBounds bounds0;
  struct RTCBounds bounds1;
};

#if defined(__cplusplus)
}
#endif