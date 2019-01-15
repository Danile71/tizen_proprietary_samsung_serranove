/*
 * imagereader
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <stdio.h>
#include <gst/gst.h>
#include "gstimagereader.h"

#ifdef HAVE_UNISTD_H
    # include <unistd.h>
#endif

#define O_BINARY (0)
#define LOG_TRACE(message)  GST_INFO (message)
#define MAX_URI_SIZE        255
#define FRAMEINTERVAL       142000
#define DEFAULT_FPS         7

// All the properties that this plugin registers
enum
{
  ARG_0,
  ARG_LOCATION,
  ARG_FPS
};

/**
 *Setting static pad templetes
 */
static GstStaticPadTemplate stSrcTemplate = GST_STATIC_PAD_TEMPLATE ("src",
                                                                                                                  GST_PAD_SRC,
                                                                                                                  GST_PAD_ALWAYS,
                                                                                                                  GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_image_reader_debug);
#define GST_CAT_DEFAULT gst_image_reader_debug

G_DEFINE_TYPE_WITH_CODE (GstImageReader, gst_image_reader, GST_TYPE_BASE_SRC, GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "imagereader", 0, "to read an image from file in loop"));

/**
 * Functions for getting/setting property (like Location)
 */
static void gst_image_reader_set_property (GObject* i_pObject, guint i_uiPropertyId, const GValue* i_pValue, GParamSpec* i_pParamSpec);
static void gst_image_reader_get_property (GObject* i_pObject, guint i_uiPropertyId, GValue* o_pValue, GParamSpec* i_pParamSpec);

/**
 * Functions for starting/stopping media data flow
 */
static gboolean gst_image_reader_start (GstBaseSrc * i_pBaseSrc);
static gboolean gst_image_reader_stop (GstBaseSrc * i_pBaseSrc);

/**
 * Functions for reading media data
 */
static GstFlowReturn gst_image_reader_create (GstBaseSrc * i_pBaseSrc, guint64 i_uiOffset, guint i_uiLength, GstBuffer ** o_pBuffer);
static GstFlowReturn  gst_image_reader_create_read (GstImageReader* i_pImageReader, guint64 i_uiOffset, guint i_uiLength, GstBuffer ** o_pBbuffer);

/**
 * This function does the following:
 *  1. Register sink-pad template
 *  2. Register element details
 *
 * @param   i_pClass        [in]    Class details.
 *
 * @return  void
 */
static void gst_image_reader_base_init (gpointer i_pClass)
{
  GstElementClass *pGstElementClass = NULL;
  GstPadTemplate* pSrcTemplate = NULL;

  LOG_TRACE("Enter");

  pGstElementClass = GST_ELEMENT_CLASS (i_pClass);

  LOG_TRACE("Exit");
}

/**
 * This function does the following:
 *  1. Override get/set property functions by assigning alternate function pointers
 *  2. Override start/stop/render functions by assigning alternate function pointers
 *  3. Install properties for ImageReader
 *
 * @param   i_pKlass        [in]    Pointer to ImageReader class.
 *
 * @return  void
 */
static void gst_image_reader_class_init (GstImageReaderClass *i_pKlass)
{
  GObjectClass *pObjectClass;
  GstBaseSrcClass *pBaseSrcClass;
  GstElementClass *pGstElementClass = NULL;
  GstPadTemplate* pSrcTemplate = NULL;

  LOG_TRACE("Enter");

  //  1. Override get/set property functions by assigning alternate function pointers
  pObjectClass = G_OBJECT_CLASS (i_pKlass);
  pBaseSrcClass = GST_BASE_SRC_CLASS (i_pKlass);

  pGstElementClass = GST_ELEMENT_CLASS (i_pKlass);
  pSrcTemplate = gst_static_pad_template_get (&stSrcTemplate);
  if (pSrcTemplate == NULL) {
    GST_ERROR ("Exit on error");
    return;
  }

  // 1. Register sink-pad template
  gst_element_class_add_pad_template (pGstElementClass, pSrcTemplate);

  // 2. Register element details
  gst_element_class_set_static_metadata (pGstElementClass,
                                                             "imagereader",
                                                             "Source/file",
                                                             "Plugin to read data from File and if it reaches EOS automatically restarts from begining by Seeking",
                                                             "Kishore Arepalli <kishore.a@samsung.com>");

  pObjectClass->set_property = gst_image_reader_set_property;
  pObjectClass->get_property = gst_image_reader_get_property;

  //  2. Override start/stop/create functions by assigning alternate function pointers
  pBaseSrcClass->start = GST_DEBUG_FUNCPTR (gst_image_reader_start);
  pBaseSrcClass->stop = GST_DEBUG_FUNCPTR (gst_image_reader_stop);
  pBaseSrcClass->create = GST_DEBUG_FUNCPTR (gst_image_reader_create);

 //  3. Install properties for ImageReader
  g_object_class_install_property (pObjectClass, ARG_LOCATION,g_param_spec_string ("location", "location", "location of file to read from", NULL, G_PARAM_READWRITE));

 g_object_class_install_property( G_OBJECT_CLASS(pObjectClass), ARG_FPS, g_param_spec_uint("framerate", "Target framerate", "Target framerate",
                                                                                                                                               0, G_MAXUINT, DEFAULT_FPS, G_PARAM_READWRITE));

  LOG_TRACE("Exit");
}

/**
 * This function does the following:
 *  1. Initialize member variables
 *  2. Allocate memory for file location
 *
 * @param   i_pImageReader        [in]    Pointer to ImageReader instance object.
 * @param   i_pKlass              [in]    Pointer to ImageReader class.
 *
 * @return  void
 */
static void gst_image_reader_init (GstImageReader * i_pImageReader)
{
  LOG_TRACE("Enter");

  //  1. Initialize member variables
  i_pImageReader->fp = NULL;

  //  2. Allocate memory for storing FIFO location
  i_pImageReader->m_pfile = (char *)g_malloc0 (MAX_URI_SIZE);

  memset (i_pImageReader->m_pfile, 0,  MAX_URI_SIZE);

  gst_base_src_set_format (GST_BASE_SRC (i_pImageReader), GST_FORMAT_TIME);
  gst_base_src_set_live(GST_BASE_SRC (i_pImageReader), TRUE);

  LOG_TRACE("Exit");
}

/**
 * This function does the following:
 *  1. Lookup for the requested property
 *  2. If Location property store the FIFO path
 *  3. If not an installed property, flag error
 *
 * @param   i_pObject        [in]    Pointer to ImageReader instance object.
 * @param   i_uiPropertyId   [in]    Registered property id.
 * @param   i_pValue         [in]    Value for the specified property.
 * @param   i_pParamSpec     [in]    Type of the property-value.
 *
 * @return  void
 */
static void gst_image_reader_set_property (GObject* i_pObject, guint i_uiPropertyId, const GValue* i_pValue, GParamSpec* i_pParamSpec)
{
  GstImageReader *pImageReader;

  LOG_TRACE("Enter");

  g_return_if_fail (GST_IS_IMAGE_READER (i_pObject));

  pImageReader = GST_IMAGE_READER (i_pObject);

  // 1. Lookup for the requested property
  switch (i_uiPropertyId) {
    case ARG_LOCATION:
      //  2. If Location property store the FIFO path
      strncpy (pImageReader->m_pfile, g_value_get_string (i_pValue), MAX_URI_SIZE);
      break;
    case ARG_FPS:
      pImageReader->frameRate = g_value_get_uint (i_pValue);
      break;
    default:
      //  3. If not an installed property, flag error
      G_OBJECT_WARN_INVALID_PROPERTY_ID (i_pObject, i_uiPropertyId, i_pParamSpec);
      break;
  }

  LOG_TRACE("Exit");
}

/**
 * This function does the following:
 *  1. Lookup for the requested property
 *  2. If Location property, return the file path through o_pValue
 *  3.If not an installed property, flag error
 *
 * @param   i_pObject        [in]    Pointer to ImageReader instance object.
 * @param   i_uiPropertyId   [in]    Registered property id.
 * @param   o_pValue         [out]   Value of the specified property.
 * @param   i_pSpec          [in]    Type of the property-value.
 *
 * @return  void
 */
static void gst_image_reader_get_property (GObject* i_pObject, guint i_uiPropertyId,  GValue* o_pValue, GParamSpec* i_pSpec)
{
  GstImageReader *pImageReader;

  g_return_if_fail (GST_IS_IMAGE_READER (i_pObject));
  LOG_TRACE("Enter");

  pImageReader = GST_IMAGE_READER (i_pObject);

  //  1. Lookup for the requested property
  switch (i_uiPropertyId) {
    case ARG_LOCATION:
      //  2. If Location property, return the FIFO path through o_pValue
      g_value_set_string (o_pValue,pImageReader->m_pfile);
      break;
    case ARG_FPS:
      g_value_set_uint (o_pValue, pImageReader->frameRate);
      break;
    default:
      //  3. If not an installed property, flag error
      G_OBJECT_WARN_INVALID_PROPERTY_ID (i_pObject, i_uiPropertyId, i_pSpec);
      break;
  }

  LOG_TRACE("Exit");
}

/**
 * This function does the following:
 *  1. Reads content from FIFO
 *
 * @param   i_pBaseSrc       [in]    Pointer to ImageReader instance object.
 * @param   i_uiOffset       [in]    Offset from where to start reading data
 * @param   i_uiLength       [in]    Length of data to be read
 * @param   o_pBuffer        [in]    Buffer to be written to the FIFO
 *
 * @return  GstFlowReturn   Returns GST_FLOW_OK if the buffer writing to FIFO is successful. GST_FLOW_ERROR otherwise.
 */

static GstFlowReturn gst_image_reader_create (GstBaseSrc * i_pBaseSrc, guint64 i_uiOffset, guint i_uiLength, GstBuffer ** o_pBuffer)
{
  GstImageReader *pImageReader;
  GstFlowReturn ret;
  //LOG_TRACE("Enter");

  pImageReader = GST_IMAGE_READER (i_pBaseSrc);

  //  1. reads data from File .
  ret = gst_image_reader_create_read (pImageReader, i_uiOffset, i_uiLength, o_pBuffer);

  //LOG_TRACE("Exit");
  return ret;
}

/**
 * This function does the following:
 *  1. Read Data from File
 *  2. Allocates Buffer to copy data from Queue
 *  3. Set Gst Buffer size , offset and length for Queue
 *  4. Read Data from FIFO
 *  5. Allocates Buffer to copy data from FIFO
 *  6. Set Gst Buffer size , offset and length for FIFO
 *
 * @param   i_pImageReader    [in]   ImageReader Instance
 * @param   i_uiOffset       [in]    Offset from where to start reading data
 * @param   i_uiLength       [in]    Length of data to be read
 * @param   o_pBuffer        [out]   Buffer containing data read from FIFO
 *
 * @return  GstFlowReturn   Returns GST_FLOW_OK if the buffer writing to FIFO is successful. GST_FLOW_ERROR otherwise.
 */
static GstFlowReturn  gst_image_reader_create_read (GstImageReader* i_pImageReader, guint64 i_uiOffset, guint length, GstBuffer ** o_pBbuffer)
{
  GstBuffer *buf = NULL;
  size_t iBytesRead = 0;
  int fileSize = 0;
  GstMapInfo buf_info = GST_MAP_INFO_INIT;

  if (i_pImageReader->frameRate == 0) {
    GST_ERROR ("framerate not set\n");
    return GST_FLOW_ERROR;
  }
  if (i_pImageReader->fp == NULL) {
    GST_ERROR ("file pointer is null\n");
    return GST_FLOW_ERROR;
  }

  /*duration as per framrate set by user*/
  i_pImageReader->duration = gst_util_uint64_scale_int (GST_SECOND, 1, i_pImageReader->frameRate);
  GST_LOG ("Duration of frame %"GST_TIME_FORMAT"\n", GST_TIME_ARGS (i_pImageReader->duration));


  /*suspends execution of the calling thread for frameduration*/
  usleep (i_pImageReader->duration / 1000);

  ///LOG_TRACE("Enter");
  fseek (i_pImageReader->fp, 0, SEEK_SET);
  fseek (i_pImageReader->fp, 0, SEEK_END);
  fileSize = ftell (i_pImageReader->fp);

  if (fileSize <= 0) {
    GST_ERROR ("Error in getting filesize\n");
    return GST_FLOW_ERROR;
  }

  fseek (i_pImageReader->fp, 0, SEEK_SET);

  buf = gst_buffer_new_and_alloc (fileSize);
  if (buf == NULL) {
    GST_ERROR ("Exit on error\n");
    return GST_FLOW_ERROR;
  }

  buf = gst_buffer_make_writable (buf);
  gst_buffer_map (buf, &buf_info, GST_MAP_READ | GST_MAP_WRITE);
  iBytesRead = fread (buf_info.data, 1, fileSize, i_pImageReader->fp);
  if (iBytesRead != fileSize) {
    GST_ERROR ("Error in reading complete file of size %d but read %d bytes\n", fileSize, iBytesRead);
  }

  fileSize = gst_buffer_get_size (buf);
  GST_BUFFER_DURATION (buf) = i_pImageReader->duration;
  GST_BUFFER_TIMESTAMP (buf)=i_pImageReader->m_TimeStamp;
  gst_buffer_unmap (buf, &buf_info);

  i_pImageReader->m_TimeStamp += i_pImageReader->duration;

  GST_LOG ("gst_image_reader_create_read: Buffer size: %d , BytesRead: %d \n", length, iBytesRead);

  if (ferror (i_pImageReader->fp) != 0) {
    GST_ERROR ("Exit on error");
    gst_object_unref (buf);
    return GST_FLOW_ERROR;
  }

  *o_pBbuffer = buf;

  //LOG_TRACE("Exit");
  return GST_FLOW_OK;
}

/**
 * This function does the following:
 *  1. Open FIFO for reading
 *  2. Check for queue
 *
 * @param   i_pBaseSrc        [in]    Pointer to ImageReader instance object.
 *
 * @return  gboolean    Return TRUE if the transition from READY to PLAYING can be made, FALSE otherwise.
 */
static gboolean gst_image_reader_start (GstBaseSrc * i_pBaseSrc)
{
  GstImageReader *pImageReader = GST_IMAGE_READER (i_pBaseSrc);

  LOG_TRACE("Enter");

  LOG_TRACE("Opening file");

  if ((pImageReader->m_pfile != NULL) && (pImageReader->fp == NULL)) {
    long lFileSize = 0;
    pImageReader->fp = fopen(pImageReader->m_pfile, "rb");
    if (pImageReader->fp == NULL) {
      GST_ERROR ("Exit on error");
      return FALSE;
    }

  fseek (pImageReader->fp, 0, SEEK_END);
  lFileSize = ftell (pImageReader->fp);

  i_pBaseSrc->blocksize = lFileSize+1;

  fseek (pImageReader->fp, 0, SEEK_SET);

  }
  pImageReader->m_TimeStamp=0;

  LOG_TRACE("Exit");
  return TRUE;

}

/**
 * This function does the following:
 *  1. Close the FIFO
 *  2. Release memory for file-path
 *  3. Close the queue
 *
 * @param   i_pBaseSrc        [in]    Pointer to ImageReader instance object.
 *
 * @return  gboolean    Return TRUE if the transition from PLAYING to READY can be made, FALSE otherwise.
 */
static gboolean gst_image_reader_stop (GstBaseSrc * i_pBaseSrc)
{
  GstImageReader *pImageReader = GST_IMAGE_READER (i_pBaseSrc);

  LOG_TRACE("Enter");

  if (pImageReader->fp != NULL) {
    fclose (pImageReader->fp);
    pImageReader->fp = NULL;
  }

  LOG_TRACE("Exit");
  return TRUE;
}

/**
 * This function does the following:
 *  1. Registers ImageReader plugin with GStreamer plugin database
 *
 * @param   i_pPlugin        [in]    Pointer to plugin.
 *
 * @return  gboolean   Returns TRUE if the plugin registration is successful. FALSE otherwise.
 */
static gboolean plugin_init (GstPlugin* i_pPlugin)
{
  gboolean bStatus = FALSE;

  LOG_TRACE("Enter");

  GST_DEBUG_CATEGORY_INIT (gst_image_reader_debug, "imagereader", 0, "Plugin to read data from File and if it reaches EOS automatically restarts from begining by Seeking");

  bStatus = gst_element_register (i_pPlugin, "imagereader", GST_RANK_NONE, GST_TYPE_IMAGE_READER);
  if (bStatus == FALSE) {
    GST_ERROR ("Exit on Errror");
    return bStatus;
  }

  LOG_TRACE("Exit");
  return bStatus;
}

/* This is the structure that gstreamer looks for to register plugins
 *
 */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                                 GST_VERSION_MINOR,
                                 imagereader,
                                 "Plugin to read data from File and if it reaches EOS automatically restarts from begining by Seeking",
                                 plugin_init,
                                 VERSION,
                                 "LGPL",
                                 "GStreamer",
                                 "http://gstreamer.net/")
