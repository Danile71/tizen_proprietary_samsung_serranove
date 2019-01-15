/*
 * wfdhalconfig
 *
 * Copyright (c) 2011 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Manoj Kumar K <manojkumar.k@samsung.com>, Abhishek Bajaj <abhi.bajaj@samsung.com>,
 * Nikhilesh Mittal <nikhilesh.m@samsung.com>, Seema Singh <seema.singh@samsung.com>,
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _WFD_HAL_CONFIG_H_
#define _WFD_HAL_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define TRUE  1
#define FALSE 0

//Target specific device node information
#define TIZEN_KEYCODE_MENU 169
#define TIZEN_KEYCODE_HOME 139
#define TIZEN_KEYCODE_BACK 158
#define TIZEN_KEYCODE_VOL_UP 115
#define TIZEN_KEYCODE_VOL_DOWN 114
#define GENERIC_DEV_DIR "/dev/input"
#define GENERIC_DEV_NAME "event"
#define HID_DIR "/dev"
#define HID_NAME "hidraw"


#define TIZEN_WIDTH 1280
#define TIZEN_HEIGHT 720

#define TIZEN_HID_MAX_DESCRIPTOR_SIZE 4096

/* Debug */

#define WFD_DEBUG
#ifdef USE_DLOG
#ifdef DLOG_TAG
#undef DLOG_TAG
#endif
#define DLOG_TAG "WFD_HAL"
#define WFD_LOG_ERROR(...)            SLOG(LOG_ERROR, DLOG_TAG, __VA_ARGS__)
#define WFD_LOG_WARN(...)             SLOG(LOG_WARN, DLOG_TAG, __VA_ARGS__)
#define WFD_LOG_INFO(...)             SLOG(LOG_INFO, DLOG_TAG, __VA_ARGS__)
#define WFD_LOG_DEBUG(...)            SLOG(LOG_DEBUG, DLOG_TAG, __VA_ARGS__)
#define WFD_LOG_VERBOSE(...)          SLOG(LOG_DEBUG, DLOG_TAG, __VA_ARGS__)
#else
#define WFD_LOG_ERROR(...)            fprintf(stderr, __VA_ARGS__)
#define WFD_LOG_WARN(...)             fprintf(stderr, __VA_ARGS__)
#define WFD_LOG_INFO(...)             fprintf(stdout, __VA_ARGS__)
#define WFD_LOG_DEBUG(...)            fprintf(stdout, __VA_ARGS__)
#define WFD_LOG_VERBOSE(...)          fprintf(stdout, __VA_ARGS__)
#endif

typedef enum {
  HID_START = 0,
  HID_CONNECTED,
  HID_DISCONNECTED
}HIDInfo;

typedef enum {
  NO_SCAN = 0,
  BT_DISCONNECTED,
  BT_CONNECTED,
  USB_DISCONNECTED,
  USB_CONNECTED
}HIDScan;

typedef enum {
  WFD_HAL_HID_BUS_BLUETOOTH,
  WFD_HAL_HID_BUS_USB,
  WFD_HAL_HID_BUS_UNSUPPORTED
}WFDHAL_HID_BUSTYPE;

typedef enum {
  WFD_HAL_HID_KEYBOARD,
  WFD_HAL_HID_MOUSE,
  WFD_HAL_HID_DEV_UNSUPPORTED
}WFDHAL_HID_DEVTYPE;

typedef enum {
  WFD_HAL_FAIL = -1,
  WFD_HAL_OK = 0
}WFDHALRESULT;


typedef enum {
  WFD_HAL_HDCP_NONE = 0,
  WFD_HAL_HDCP_2_0  = (1 << 0),
  WFD_HAL_HDCP_2_1  = (1 << 1)
}HDCPPROTECTION;

typedef struct wfd_hal_hid_report_descriptor {
  uint32_t size;
  char *value;
}wfd_hal_hid_report_descriptor_t;

typedef struct hal_hid_device_list {
  WFDHAL_HID_BUSTYPE bus_type;
  WFDHAL_HID_DEVTYPE dev_type;
  char *fname;
  wfd_hal_hid_report_descriptor_t hid_report;
}wfd_hal_hid_device_t;

typedef struct hal_virtual_hid_device {
  char *name;
  int fd;
}wfd_hal_virtual_hid_device_t;

/** wfd config property**/
typedef struct wfd_hal_config {
    char* property_name;
    union {
        float fValue;
        uint32_t i32Value;
        uint64_t i64Value;
        double dValue;
        char* cValue;
        uint8_t  bValue;
    }value;
}wfd_hal_config_t;

/** HDCP Config **/
typedef struct hdcp_config {
  HDCPPROTECTION hdcp_version;
  uint8_t hdcp_content_protection;
  uint32_t hdcp_port;
  char hdcp_ip[17];
  void* hdcp_handle;
}hdcp_config_t;


/** Edid Config **/

typedef struct edid_config {
  uint32_t edid_hres;
  uint32_t edid_vres;
}edid_config_t;

/* Video paramerts */
typedef struct wfd_hal_video_param {
  uint32_t video_codec;
  uint32_t video_native_resolution;
  uint32_t video_cea_support;
  uint32_t video_vesa_support;
  uint32_t video_hh_support;
  uint32_t video_profile;
  uint32_t video_level;
  uint32_t video_latency;
  uint32_t video_vertical_resolution;
  uint32_t video_horizontal_resolution;
  uint32_t video_minimum_slicing;
  uint32_t video_slice_enc_param;
  uint32_t video_framerate_control_support;
} wfd_hal_video_param_t;

/* Audio paramerts */
typedef struct wfd_hal_audio_param {
  uint32_t audio_codec;
  uint32_t audio_latency;
  uint32_t audio_channel;
  uint32_t audio_sampling_frequency;
}wfd_hal_audio_param_t;

typedef struct wfd_hal_interface {
  /*Audio related APIs*/
  void (*get_audiosrc_config) (wfd_hal_config_t** audiosrc_config, uint32_t* size);
  char* (*get_audiosrc_type) (void);
  uint8_t (*is_audioconvert_required) (void);
  char* (*get_audioconvert_type) (void);
  char* (*get_audiosink_type) (void);
  void (*get_audiosink_config) (wfd_hal_config_t** audiosink_conf, uint32_t* size);
  void (*get_audio_param) (wfd_hal_audio_param_t** audio_param);

  /*HDCP related APIs*/
  uint8_t (*is_hdcp_supported) (void);
  char* (*get_video_hdcp_type) (void);
  char* (*get_audio_hdcp_type) (void);
  void (*get_hdcp_config) (hdcp_config_t ** hdcp_conf);
  void (*set_hdcp_port_ip) (uint32_t hdcp_port, char* hdcp_ip);
  WFDHALRESULT (*hdcp_prepare_protection) (char* wfdsink_ip, uint32_t hdcp_tcpport);
  uint8_t (*is_hdcp_version_supported) (uint32_t hdcp_version);
  uint8_t (*check_hdcp) (void);

  /*Display related APIs*/
  void (*set_edid_info) (char* edid_info ,uint8_t plug);
  uint8_t (*is_edid_supported) (void);
  void (*create_edid_payload) (uint32_t edid_block_count, char** edid_payload, char* value, uint32_t* payload_size);
  uint8_t (*check_edid_resolution) (char* edid_payload);
  uint64_t (*get_edid_supported_resolution) (void);
  char* (*get_videosink_type) (void);
  void (*get_videosink_config) (wfd_hal_config_t** videosink_conf, uint32_t* size, uint8_t hdcp_enabled);
  char* (*get_videosrc_type) (void);
  void (*get_videosrc_config) (wfd_hal_config_t** videosrc_conf, uint32_t* size, uint8_t hdcp_enabled);
  void (*get_video_param) (wfd_hal_video_param_t** video_param);

  /*Codec related APIs*/
  char* (*get_video_encoder_type) (uint8_t hdcp_enabled);
  void (*get_video_encoder_config) (wfd_hal_config_t** video_encoder_conf, uint32_t* size, uint8_t hdcp_enabled);
  char* (*get_video_decoder_type) (uint8_t hdcp_enabled);
  void (*get_video_decoder_config) (wfd_hal_config_t** video_decoder_conf, uint32_t* size, uint8_t hdcp_enabled);
  char* (*get_audio_encoder_type) (uint8_t hdcp_enabled);
  void (*get_audio_encoder_config) (wfd_hal_config_t** audio_encoder_conf, uint32_t* size, uint8_t hdcp_enabled);
  char* (*get_audio_decoder_type) (uint8_t hdcp_enabled);
  void (*get_audio_decoder_config) (wfd_hal_config_t** audio_decoder_conf, uint32_t* size, uint8_t hdcp_enabled);

  /* HID */
  int (*is_uibc_hid_supported) (void);
  int (*uibc_hid_get_number_of_hid_devices) (void);
  wfd_hal_hid_device_t* (*uibc_hid_scan_hid_devices) (void);
  wfd_hal_virtual_hid_device_t* (*uibc_hid_create_virtual_device) (char *name, WFDHAL_HID_BUSTYPE bus_type, char *report, int length);
  void (*uibc_hid_input_virtual_device) (wfd_hal_virtual_hid_device_t *device, char *report, int length);
  void (*uibc_hid_destroy_virtual_device) (wfd_hal_virtual_hid_device_t *device);
} wfd_hal_interface_t;

/*Audio related APIs*/
void wfd_hal_get_audiosrc_config (wfd_hal_config_t** audiosrc_config, uint32_t* size);
char* wfd_hal_get_audiosrc_type ();
uint8_t wfd_hal_is_audioconvert_required ();
char* wfd_hal_get_audioconvert_type ();
char* wfd_hal_get_audiosink_type (void);
void wfd_hal_get_audiosink_config (wfd_hal_config_t** audiosink_conf, uint32_t* size);
void wfd_hal_get_audio_param(wfd_hal_audio_param_t** audio_param);

/*HDCP related APIs*/
uint8_t wfd_hal_is_hdcp_supported ();
char* wfd_hal_get_video_hdcp_type ();
char* wfd_hal_get_audio_hdcp_type ();
void wfd_hal_get_hdcp_config (hdcp_config_t ** hdcp_conf);
void wfd_hal_set_hdcp_port_ip (uint32_t hdcp_port, char* hdcp_ip);
WFDHALRESULT wfd_hal_hdcp_prepare_protection (char* wfdsink_ip, uint32_t hdcp_tcpport);
uint8_t wfd_hal_is_hdcp_version_supported(uint32_t hdcp_version);
uint8_t wfd_hal_check_hdcp(void);

/*Display related APIs*/
void wfd_hal_set_edid_info (char* edid_info ,uint8_t plug);
uint8_t wfd_hal_is_edid_supported (void);
void wfd_hal_create_edid_payload(uint32_t edid_block_count, char** edid_payload, char* value, uint32_t* payload_size);
uint8_t wfd_hal_check_edid_resolution (char* edid_payload);
uint64_t wfd_hal_get_edid_supported_resolution ();
char* wfd_hal_get_videosink_type (void);
void wfd_hal_get_videosink_config (wfd_hal_config_t** videosink_conf, uint32_t* size, uint8_t hdcp_enabled);
char* wfd_hal_get_videosrc_type (void);
void wfd_hal_get_videosrc_config (wfd_hal_config_t** videosrc_conf, uint32_t* size, uint8_t hdcp_enabled);
void wfd_hal_get_video_param(wfd_hal_video_param_t** video_param);

/*Codec related APIs*/
char* wfd_hal_get_video_encoder_type(uint8_t hdcp_enabled);
void wfd_hal_get_video_encoder_config (wfd_hal_config_t** video_encoder_conf, uint32_t* size, uint8_t hdcp_enabled);
char* wfd_hal_get_video_decoder_type(uint8_t hdcp_enabled);
void wfd_hal_get_video_decoder_config (wfd_hal_config_t** video_decoder_conf, uint32_t* size, uint8_t hdcp_enabled);
char* wfd_hal_get_audio_encoder_type(uint8_t hdcp_enabled);
void wfd_hal_get_audio_encoder_config (wfd_hal_config_t** audio_encoder_conf, uint32_t* size, uint8_t hdcp_enabled);
char* wfd_hal_get_audio_decoder_type(uint8_t hdcp_enabled);
void wfd_hal_get_audio_decoder_config (wfd_hal_config_t** audio_decoder_conf, uint32_t* size, uint8_t hdcp_enabled);

/* HID support */
int wfd_hal_is_uibc_hid_supported();
/* HID capturer device list */
int wfd_hal_uibc_hid_get_number_of_hid_devices();
wfd_hal_hid_device_t* wfd_hal_uibc_hid_scan_hid_devices();
/* HID capturer state change  */
typedef void (*hid_device_state_change)(HIDScan change_noti, void *user_param);
void wfd_hal_uibc_register_hid_device_state_change(hid_device_state_change state_change_cb, void *user_param);
void wfd_hal_uibc_deregister_hid_device_state_change();
/* HID injector virtual device */
wfd_hal_virtual_hid_device_t* wfd_hal_uibc_hid_create_virtual_device(char *name, WFDHAL_HID_BUSTYPE bus_type, char *report, int length);
void wfd_hal_uibc_hid_input_virtual_device(wfd_hal_virtual_hid_device_t *device, char *report, int length);
void wfd_hal_uibc_hid_destroy_virtual_device(wfd_hal_virtual_hid_device_t *device);

#endif //_WFD_HAL_CONFIG_H_
