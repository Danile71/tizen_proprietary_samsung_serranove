/*
**Copyright (c) 2014 - 2015 Samsung Electronics Co., Ltd.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef __CAMERAHDR_H__
#define __CAMERAHDR_H__

typedef struct _HDRsrc_buffer_t {
    /* Supports for Planes & DMA-buf */
    struct {
        unsigned int length;    /**< Size of stored data */
        unsigned char *start;   /**< Start address of data */
        int fd;                 /* dmabuf-fd */
    } planes[4];                    /* planes: SCMN_IMGB_MAX_PLANE */
    int num_planes;
    int width;                                      /**< width of image */
    int height;                                     /**< height of image */
} HDRsrc_buffer_t;

typedef enum {
	HDR_FORMAT_NONE = 0,
	HDR_FORMAT_NV12,
	HDR_FORMAT_NV21,
	HDR_FORMAT_NV16,
	HDR_FORMAT_NV61,
	HDR_FORMAT_UYVY,
	HDR_FORMAT_YU16
} HDRInputPictureFormat;

typedef enum {
    HDR_PROCESS_NO_ERROR = 0,
    HDR_PROCESS_IN_DATA_ERROR,
    HDR_PROCESS_MEM_ALLOC_ERROR,
    HDR_PROCESS_OPERATION_ERROR
} hdr_err_t;

#define HDR_BRACKET_FRAME_NUM                   3
#define NUM_OF_CPU_CORES                             4
#define YUV422_BUFFER_SIZE(width,height) ( ((width)*(height)) << 1 )

int run_hdr_processing(HDRsrc_buffer_t *input_buffer,unsigned int img_width,unsigned int img_height, HDRInputPictureFormat format,
                                                    unsigned char **result_data,void* data_cb,void* user_data);

#endif /* __CAMERAHDR_H__ */
