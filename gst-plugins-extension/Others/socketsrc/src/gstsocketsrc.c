/*
 * socketsrc
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
#include "gstsocketsrc.h"



#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define LOG_TRACE(message)  GST_INFO(message)
#define MAX_URI_SIZE 		255
#define DEFAULT_PROP_SPN    GST_SOCKET_PCM_DATA
#define INTERFACE "svnet0"
#define MAX_INBUF_SIZE 1284

#define GST_TYPE_SOCKET_SPN_RESOURCE_METHOD (gst_socket_spn_resource_method_get_type())

static int gst_socketsrc_open_sock(int res);
static gboolean gst_socketsrc_event_handler (GstBaseSrc * src, GstEvent * event);

    static GType
gst_socket_spn_resource_method_get_type (void)
{
    static GType socket_spn_resource_method_type = 0;
    static const GEnumValue socket_spn_resource_methods[] = {
        {GST_SOCKET_FMT_IPC, "FMT_IPC", "FMT_IPC"},
        {GST_SOCKET_RFS_IPC, "RFS_IPC", "RFS_IPC"},
        {GST_SOCKET_VT, "VT","VT"},
        {GST_SOCKET_BT_DUN, "BT_DUN", "BT_DUN"},
        {GST_SOCKET_PCM_DATA, "PCM Interface for Voice call recording", "pcm"},
        {0, NULL, NULL},
    };

    if (!socket_spn_resource_method_type) {
        socket_spn_resource_method_type = g_enum_register_static ("GstSocketSpnResource",
                socket_spn_resource_methods);
    }
    return socket_spn_resource_method_type;
}

// All the properties that this plugin registers
enum
{
    ARG_0,
    ARG_SPN_RESOURCE
};

/**
 *Setting static pad templetes
 */

static GstStaticPadTemplate stSrcTemplate = GST_STATIC_PAD_TEMPLATE ("src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS_ANY);



GST_DEBUG_CATEGORY_STATIC (gst_socketsrc_debug);
#define GST_CAT_DEFAULT gst_socketsrc_debug
#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_socketsrc_debug, "socketsrc",0, "to read  from a socket"); \

/**
 * Filling-in SocketSrc plugin details
 */
static const GstElementDetails stSocketSrcDetails = GST_ELEMENT_DETAILS ("socketsrc",
        "Source",
        "Plugin to read data from socket",
        "Harish Jenny K N <harish.jenny@samsung.com>");
/**
 * Defining the boilerplate
 */

GST_BOILERPLATE_FULL (GstSocketSrc, gst_socketsrc, GstBaseSrc, GST_TYPE_BASE_SRC, _do_init);


/**
 * Functions for getting/setting property (like spn resource)
 */

static void gst_socketsrc_set_property (GObject* i_pObject, guint i_uiPropertyId, const GValue* i_pValue, GParamSpec* i_pParamSpec);
static void gst_socketsrc_get_property (GObject* i_pObject, guint i_uiPropertyId, GValue* o_pValue, GParamSpec* i_pParamSpec);


/**
 * Functions for starting/stopping media data flow by openig and closing sockets
 */

static gboolean gst_socketsrc_start (GstBaseSrc * i_pBaseSrc);
static gboolean gst_socketsrc_stop (GstBaseSrc * i_pBaseSrc);

static gboolean gst_socketsrc_unlock (GstBaseSrc * bsrc);
static gboolean gst_socketsrc_unlock_stop (GstBaseSrc * bsrc);
/**
 * Functions for reading media data from socket
 */

static GstFlowReturn gst_socketsrc_create (GstBaseSrc * i_pBaseSrc, guint64 i_uiOffset, guint i_uiLength, GstBuffer ** o_pBuffer);
static GstFlowReturn  gst_socketsrc_create_read (GstSocketSrc* i_pSocketSrc, guint64 i_uiOffset, guint i_uiLength, GstBuffer ** o_pBbuffer);


/**
 * This function does the following:
 *  1. Register sink-pad template
 *  2. Register element details
 *
 * @param   i_pClass        [in]    Class details.
 *
 * @return  void
 */

static void gst_socketsrc_base_init (gpointer i_pClass)
{

    GstElementClass *pGstElementClass = NULL;
    GstPadTemplate* pSrcTemplate = NULL;

    LOG_TRACE("Enter");

    pGstElementClass = GST_ELEMENT_CLASS (i_pClass);
    pSrcTemplate = gst_static_pad_template_get(&stSrcTemplate);
    if(pSrcTemplate == NULL)
    {
        GST_ERROR("Exit on error");
        return;
    }

    // 1. Register sink-pad template
    gst_element_class_add_pad_template(pGstElementClass, pSrcTemplate);

    // 2. Register element details
    gst_element_class_set_details(pGstElementClass, &stSocketSrcDetails);

    LOG_TRACE("Exit");

}

/**
 * This function does the following:
 *  1. Override get/set property functions by assigning alternate function pointers
 *  2. Override start/stop/render functions by assigning alternate function pointers
 *  3. Install properties for SocketSrc
 *
 * @param   i_pKlass        [in]    Pointer to SocketSrc class.
 *
 * @return  void
 */

static void gst_socketsrc_class_init (GstSocketSrcClass *i_pKlass)
{
    GObjectClass *pObjectClass;
    GstBaseSrcClass *pBaseSrcClass;

    LOG_TRACE("Enter");

    //  1. Override get/set property functions by assigning alternate function pointers

    pObjectClass = G_OBJECT_CLASS (i_pKlass);
    pBaseSrcClass = GST_BASE_SRC_CLASS (i_pKlass);

    pObjectClass->set_property = gst_socketsrc_set_property;
    pObjectClass->get_property = gst_socketsrc_get_property;

    //  2. Override start/stop/create functions by assigning alternate function pointers

    pBaseSrcClass->start = GST_DEBUG_FUNCPTR (gst_socketsrc_start);
    pBaseSrcClass->stop = GST_DEBUG_FUNCPTR (gst_socketsrc_stop);
    pBaseSrcClass->create = GST_DEBUG_FUNCPTR (gst_socketsrc_create);
    pBaseSrcClass->event = GST_DEBUG_FUNCPTR (gst_socketsrc_event_handler);
    pBaseSrcClass->unlock  = GST_DEBUG_FUNCPTR (gst_socketsrc_unlock);
    pBaseSrcClass->unlock_stop  = GST_DEBUG_FUNCPTR (gst_socketsrc_unlock_stop);

    //  3. Install properties for SocketSrc
    g_object_class_install_property (pObjectClass, ARG_SPN_RESOURCE,
            g_param_spec_enum ("spnresource", "spnresource", "location of bind to receive data",
                GST_TYPE_SOCKET_SPN_RESOURCE_METHOD, DEFAULT_PROP_SPN,
                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    LOG_TRACE("Exit");

}

/**
 * This function does the following:
 *  1. Initialize member variables
 *
 * @param   i_pSocketSrc        [in]    Pointer to SocketSrc instance object.
 * @param   i_pKlass              [in]    Pointer to SocketSrc class.
 *
 * @return  void
 */

static void gst_socketsrc_init (GstSocketSrc * i_pSocketSrc, GstSocketSrcClass * i_pKlass)
{
    LOG_TRACE("Enter");

    //  1. Initialize member variables
    i_pSocketSrc->spn_resource =  GST_SOCKET_PCM_DATA;
    i_pSocketSrc->raw=0;
    i_pSocketSrc->flushing=0;

    gst_base_src_set_format (GST_BASE_SRC (i_pSocketSrc), GST_FORMAT_TIME);
    gst_base_src_set_live(GST_BASE_SRC(i_pSocketSrc),TRUE);

    LOG_TRACE("Exit");

}

/**
 * This function does the following:
 *  1. Lookup for the requested property
 *  2. If SPN_RESOURCE property store the socket path
 *  3. If not an installed property, flag error
 *
 * @param   i_pObject        [in]    Pointer to SocketSrc instance object.
 * @param   i_uiPropertyId   [in]    Registered property id.
 * @param   i_pValue         [in]    Value for the specified property.
 * @param   i_pParamSpec     [in]    Type of the property-value.
 *
 * @return  void
 */

static void gst_socketsrc_set_property (GObject* i_pObject, guint i_uiPropertyId, const GValue* i_pValue, GParamSpec* i_pParamSpec)
{

    GstSocketSrc *pSocketSrc;
    GstSocketSpnResource local_spn_resource=0;
    LOG_TRACE("Enter");

    g_return_if_fail (GST_IS_SOCKET_SRC(i_pObject));

    pSocketSrc = GST_SOCKET_SRC(i_pObject);

    // 1. Lookup for the requested property
    switch (i_uiPropertyId)
    {

        case ARG_SPN_RESOURCE:
            //  2. If SPN_RESOURCE property store the socket path and open the socket if it is changed
            local_spn_resource = g_value_get_enum (i_pValue);
            if(local_spn_resource != pSocketSrc->spn_resource)
            {
                pSocketSrc->spn_resource = local_spn_resource;
                if(pSocketSrc->spn_resource != 0)
                {
                    GST_DEBUG(" Opening Socket %d",pSocketSrc->spn_resource);
                    if(pSocketSrc->raw != 0)
                    {
                        close(pSocketSrc->raw);
                        pSocketSrc->raw=0;
                    }
                    pSocketSrc->raw = gst_socketsrc_open_sock (pSocketSrc->spn_resource);
                    if (pSocketSrc->raw == -1) {
                        LOG_TRACE("Cannot open PCM_DATA");
                        return FALSE;
                    }
                    pSocketSrc->fds[0].fd = pSocketSrc->raw;
                    pSocketSrc->fds[0].events = POLLIN;
                    pSocketSrc->fds[0].revents = 0;
                }
                else
                {
                    LOG_TRACE("spn_resource is NULL");
                }
            }
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
 *  2. If Location property, return the spn resource id through o_pValue
 *  3.If not an installed property, flag error
 *
 * @param   i_pObject        [in]    Pointer to SocketSrc instance object.
 * @param   i_uiPropertyId   [in]    Registered property id.
 * @param   o_pValue         [out]   Value of the specified property.
 * @param   i_pSpec          [in]    Type of the property-value.
 *
 * @return  void
 */
static void gst_socketsrc_get_property(GObject* i_pObject, guint i_uiPropertyId,  GValue* o_pValue, GParamSpec* i_pSpec)
{

    GstSocketSrc *pSocketSrc;

    g_return_if_fail (GST_IS_SOCKET_SRC(i_pObject));
    LOG_TRACE("Enter");

    pSocketSrc = GST_SOCKET_SRC(i_pObject);

    //  1. Lookup for the requested property
    switch (i_uiPropertyId)
    {

        case ARG_SPN_RESOURCE:
            //  2. If SPN Resource property, return the value set
            g_value_set_enum (o_pValue, pSocketSrc->spn_resource);
            break;
        default:
            //  3. If not an installed property, flag error
            G_OBJECT_WARN_INVALID_PROPERTY_ID (i_pObject, i_uiPropertyId, i_pSpec);
            break;
    }

    LOG_TRACE("Exit");
}


    static gboolean
gst_socketsrc_event_handler (GstBaseSrc * basesrc, GstEvent * event)
{
    GstEventType type;

    type = GST_EVENT_TYPE (event);
    switch (type) {
        case GST_EVENT_EOS:
            {
                GST_LOG("\n EOS event in socketsrc plugin \n");
            }
        default:
            break;
    }
    return GST_BASE_SRC_CLASS (parent_class)->event (basesrc, event);
}

/**
 * This function does the following:
 *  1. Reads content from socket
 *
 * @param   i_pBaseSrc       [in]    Pointer to SocketSrc instance object.
 * @param   i_uiOffset       [in]    Offset from where to start reading data
 * @param   i_uiLength       [in]    Length of data to be read
 * @param   o_pBuffer        [in]    Buffer to be written to the OutBuffer
 *
 * @return  GstFlowReturn   Returns GST_FLOW_OK if the buffer writing to OutBuf is successful. GST_FLOW_ERROR otherwise.
 */

static GstFlowReturn gst_socketsrc_create (GstBaseSrc * i_pBaseSrc, guint64 i_uiOffset, guint i_uiLength, GstBuffer ** o_pBuffer)
{
    GstSocketSrc *pSocketSrc;
    GstFlowReturn ret;
    //  LOG_TRACE("Enter");

    pSocketSrc = GST_SOCKET_SRC (i_pBaseSrc);

    //  1. reads data from Socket interface .
    ret = gst_socketsrc_create_read (pSocketSrc, i_uiOffset, i_uiLength, o_pBuffer);

    //  LOG_TRACE("Exit");
    return ret;
}

static gboolean
gst_socketsrc_unlock (GstBaseSrc * bsrc)
{
    GstSocketSrc *src;
    src = GST_SOCKET_SRC (bsrc);
    GST_LOG_OBJECT (src, "Flushing");
    src->flushing=1;
    return TRUE;
}

static gboolean
gst_socketsrc_unlock_stop (GstBaseSrc * bsrc)
{
    GstSocketSrc *src;
    src = GST_SOCKET_SRC (bsrc);
    GST_LOG_OBJECT (src, "No longer flushing");
    src->flushing=0;
    return TRUE;
}

/**
 * This function does the following:
 *  1. Poll for some events
 *  2. If event is POLLIN then receive the data
 *  3. Set Gst Buffer size , timestamp and duration for outbuf
 *
 * @param   i_pSocketSrc    [in]   SocketSrc Instance
 * @param   i_uiOffset       [in]    Offset from where to start reading data
 * @param   i_uiLength       [in]    Length of data to be read
 * @param   o_pBuffer        [out]    Buffer to be written to the OutBuffer
 *
 * @return  GstFlowReturn   Returns GST_FLOW_OK if the buffer writing to OutBuf is successful. GST_FLOW_ERROR otherwise.
 */

static GstFlowReturn  gst_socketsrc_create_read (GstSocketSrc* i_pSocketSrc, guint64 i_uiOffset, guint length, GstBuffer ** o_pBbuffer)
{
    static int first_time=0;
    ssize_t readsize=0;
    struct sockaddr_pn addr;
    socklen_t addrlen = sizeof(addr);
    uint16_t obj;
    uint8_t res;
    GstBuffer *buffer = gst_buffer_new_and_alloc (MAX_INBUF_SIZE);

    i_pSocketSrc->duration = gst_util_uint64_scale_int (GST_SECOND, 1, 50);
    GST_LOG("Duration of frame %"GST_TIME_FORMAT"\n", GST_TIME_ARGS(i_pSocketSrc->duration));

    /* Logic - Need to verify */
    if(first_time==0)
    {
        while(readsize==0)
        {
            readsize = poll(i_pSocketSrc->fds, 1, 1000);
            if(i_pSocketSrc->flushing)
                goto STOPPED;
        }

        first_time=1;
    }
    else
    {
        readsize = poll(i_pSocketSrc->fds, 1, 1000);
        if (readsize == -1) {
            perror("poll");
            LOG_TRACE("poll  error Exit");
            return GST_FLOW_ERROR;
        }
    }
    if (readsize > 0) {
        if (i_pSocketSrc->fds[0].revents & POLLIN) {
            readsize= recvfrom(i_pSocketSrc->fds[0].fd, GST_BUFFER_DATA(buffer), MAX_INBUF_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
            if (readsize == -1) {
                perror("recvfrom");
                GST_ERROR("Error in  recvfrom\n");
                return GST_FLOW_ERROR;
            }

            obj = pn_sockaddr_get_object(&addr);
            res = addr.spn_resource;
            if (res == GST_SOCKET_PCM_DATA) {
                GST_DEBUG("Value of readsize is %d",readsize);
                if( readsize > 0 )
                {
                    GST_BUFFER_SIZE(buffer) = readsize;
                    GST_BUFFER_DURATION(buffer) = i_pSocketSrc->duration;
                    GST_BUFFER_TIMESTAMP(buffer)=i_pSocketSrc->m_TimeStamp;
                    i_pSocketSrc->m_TimeStamp += i_pSocketSrc->duration;

                    GST_LOG("gst_socketsrc_create_read: Buffer size: %d , BytesRead: %d \n", length, readsize);

                    *o_pBbuffer = buffer;
                }

            }
        }

    }
    else
    {
        // Whether unexpected???
        return GST_FLOW_UNEXPECTED;
    }
    LOG_TRACE("gst_socketsrc_create_read Exit");
    return GST_FLOW_OK;
STOPPED:
    GST_DEBUG ("stop called");
    return GST_FLOW_WRONG_STATE;

}

/**
 * This function does the following:
 *  1. Open the socket
 *  2. setsockopt
 *  3. Bind
 *
 * @param   i_res    [in]   socket ID
 *
 * @return  GstFlowReturn   Returns fd if opening socket is successful. -1 otherwise.
 */

static int gst_socketsrc_open_sock(int i_res)
{
    int fd;
    int r;
    char ifnm[IF_NAMESIZE];
    struct sockaddr_pn addr = {
        .spn_family = AF_PHONET,
    };

    fd = socket(PF_PHONET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    strcpy(ifnm, INTERFACE);
#if 0
    r = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifnm, IF_NAMESIZE);
    if (r == -1) {
        perror("setsockopt");
        close(fd);
        return -1;
    }
#endif
    addr.spn_resource = i_res;
    r = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (r == -1) {
        perror("bind");
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * This function does the following:
 *  1. Open socket for reading
 *  2. Check for queue
 *
 * @param   i_pBaseSrc        [in]    Pointer to SocketSrc instance object.
 *
 * @return  gboolean    Return TRUE if the transition from READY to PLAYING can be made, FALSE otherwise.
 */
static gboolean gst_socketsrc_start(GstBaseSrc * i_pBaseSrc)
{
    GstSocketSrc *pSocketSrc = GST_SOCKET_SRC (i_pBaseSrc);

    LOG_TRACE("Enter");


    if(pSocketSrc->spn_resource != 0)
    {
        GST_DEBUG(" Opening Socket %d",pSocketSrc->spn_resource);
        pSocketSrc->raw = gst_socketsrc_open_sock (pSocketSrc->spn_resource);
        if (pSocketSrc->raw == -1) {
            LOG_TRACE("Cannot open PCM_DATA");
            return FALSE;
        }
        pSocketSrc->fds[0].fd = pSocketSrc->raw;
        pSocketSrc->fds[0].events = POLLIN;
        pSocketSrc->fds[0].revents = 0;
    }
    else
    {
        LOG_TRACE("spn_resource is NULL");
    }
    pSocketSrc->m_TimeStamp=0;

    LOG_TRACE("Exit");
    return TRUE;

}

/**
 * This function does the following:
 *  1. Close the Opened socket
 *
 * @param   i_pBaseSrc        [in]    Pointer to SocketSrc instance object.
 *
 * @return  gboolean    Return TRUE if the transition from PLAYING to READY can be made, FALSE otherwise.
 */
static gboolean gst_socketsrc_stop (GstBaseSrc * i_pBaseSrc)
{
    GstSocketSrc *pSocketSrc = GST_SOCKET_SRC (i_pBaseSrc);


    LOG_TRACE("Enter");

    if(pSocketSrc->spn_resource != 0)
    {
        close(pSocketSrc->raw);
        pSocketSrc->spn_resource=0;
    }

    LOG_TRACE("Exit");
    return TRUE;
}


/**
 * This function does the following:
 *  1. Registers SocketSrc plugin with GStreamer plugin database
 *
 * @param   i_pPlugin        [in]    Pointer to plugin.
 *
 * @return  gboolean   Returns TRUE if the plugin registration is successful. FALSE otherwise.
 */
static gboolean plugin_init(GstPlugin* i_pPlugin)
{
    gboolean bStatus = FALSE;

    LOG_TRACE("Enter");

    GST_DEBUG_CATEGORY_INIT (gst_socketsrc_debug, "socketsrc", 0, "Plugin to read voice data from Socket");

    bStatus = gst_element_register(i_pPlugin, "socketsrc", GST_RANK_NONE, GST_TYPE_SOCKET_SRC);
    if(bStatus == FALSE)
    {
        GST_ERROR("Exit on Errror");
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
        "socketsrc",
        "Plugin to read voice data from Socket",
        plugin_init,
        VERSION,
        "Proprietary",
        "Samsung Electronics Co",
        "http://www.samsung.com")
