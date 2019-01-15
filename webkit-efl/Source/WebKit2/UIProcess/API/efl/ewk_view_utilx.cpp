/*
    Copyright (C) 2013 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_view_utilx.h"

#if PLATFORM(TIZEN)
#include "EwkViewImpl.h"

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
#define Bool int

#include "EflScreenUtilities.h"

#include <Elementary.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>
#include <pixman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#define RETURN_VAL_IF_FAIL(c, v) { if (!(c)) { return v; } }
#define GOTO_IF_FAIL(c, d) { if (!(c)) { goto d; } }

typedef struct {
    XShmSegmentInfo xShmSegmentInfo;
    XImage* xImage;
    char* rotateImageBuffer;
    Display* displayForX;
} CaptureData;

static bool convertImage (uint32_t* srcbuf,
                          uint32_t* dstbuf,
                          pixman_format_code_t srcFormat,
                          pixman_format_code_t dstFormat,
                          int srcWidth, int srcHeight,
                          int dstWidth, int dstHeight,
                          int angle)
{
    pixman_image_t* srcImg = 0;
    pixman_image_t* dstImg = 0;
    pixman_transform_t transform;
    int srcStride, dstStride;
    int srcBpp, dstBpp;
    pixman_op_t op;
    int rotateStep;
    int ret = false;

    RETURN_VAL_IF_FAIL(srcbuf != 0, false);
    RETURN_VAL_IF_FAIL(dstbuf != 0, false);
    RETURN_VAL_IF_FAIL(angle <= 360 && angle >= -360, false);

    op = PIXMAN_OP_SRC;

    srcBpp = PIXMAN_FORMAT_BPP(srcFormat) / 8;
    RETURN_VAL_IF_FAIL(srcBpp > 0, false);

    dstBpp = PIXMAN_FORMAT_BPP(dstFormat) / 8;
    RETURN_VAL_IF_FAIL(dstBpp > 0, false);

    rotateStep = (angle + 360) / 90 % 4;

    srcStride = srcWidth * srcBpp;
    dstStride = dstWidth * dstBpp;

    srcImg = pixman_image_create_bits(srcFormat, srcWidth, srcHeight, srcbuf, srcStride);
    dstImg = pixman_image_create_bits(dstFormat, dstWidth, dstHeight, dstbuf, dstStride);

    GOTO_IF_FAIL(srcImg != 0, failToConvert);
    GOTO_IF_FAIL(dstImg != 0, failToConvert);

    pixman_transform_init_identity(&transform);

    if (rotateStep > 0) {
        int c, s, tx = 0, ty = 0;
        switch (rotateStep) {
        case 1:
            /* 270 degrees */
            c = 0;
            s = -pixman_fixed_1;
            ty = pixman_int_to_fixed(dstWidth);
            break;
        case 2:
            /* 180 degrees */
            c = -pixman_fixed_1;
            s = 0;
            tx = pixman_int_to_fixed(dstWidth);
            ty = pixman_int_to_fixed(dstHeight);
            break;
        case 3:
            /* 90 degrees */
            c = 0;
            s = pixman_fixed_1;
            tx = pixman_int_to_fixed(dstHeight);
            break;
        }
        pixman_transform_rotate(&transform, 0, c, s);
        pixman_transform_translate(&transform, 0, tx, ty);
    }

    pixman_image_set_transform(srcImg, &transform);
    pixman_image_composite(op, srcImg, 0, dstImg,
                           0, 0, 0, 0, 0, 0, dstWidth, dstHeight);

    ret = true;

failToConvert:
    if (srcImg)
        pixman_image_unref(srcImg);
    if (dstImg)
        pixman_image_unref(dstImg);

    return ret;
}

static void* captureScreenShotForRotation(Evas_Object* ewkView, CaptureData* captureData, int angle)
{
    int x, y;
    unsigned int width, height, border, depth;
    void* ret;
    XID xId, root;
#if ENABLE(TIZEN_FIX_PRERENDERING_FOR_ROTATION_SNAPSHOT_BUG)
    Ecore_X_Window xwinl = 0;
    Ecore_Evas *eel = NULL;
#endif
    captureData->displayForX = XOpenDisplay(0);
    GOTO_IF_FAIL(captureData->displayForX != 0, failToCapture);

#if ENABLE(TIZEN_FIX_PRERENDERING_FOR_ROTATION_SNAPSHOT_BUG)
    xId = elm_win_xwindow_get(ewkView);
    if (!xId) {
        eel = ecore_evas_ecore_evas_get(evas_object_evas_get(ewkView));
        if (eel)
            xwinl = (Ecore_X_Window)ecore_evas_window_get(eel);
        xId = xwinl;
    }
#else
    xId = elm_win_xwindow_get(ewkView);
#endif
    GOTO_IF_FAIL(xId > 0, failToCapture);

    XGetGeometry(captureData->displayForX, xId, &root,
                 &x, &y, &width, &height, &border, &depth);

    GOTO_IF_FAIL(width > 0, failToCapture);
    GOTO_IF_FAIL(height > 0, failToCapture);

    captureData->xShmSegmentInfo.shmid = -1;
    captureData->xImage = XShmCreateImage(captureData->displayForX,
                                          DefaultVisualOfScreen(DefaultScreenOfDisplay(captureData->displayForX)),
                                          24,
                                          ZPixmap,
                                          0, &captureData->xShmSegmentInfo, width, height);
    GOTO_IF_FAIL(captureData->xImage != 0, failToCapture);

    captureData->xShmSegmentInfo.shmid = shmget(IPC_PRIVATE,
                                                captureData->xImage->bytes_per_line * captureData->xImage->height,
                                                IPC_CREAT | 0777);
    captureData->xShmSegmentInfo.shmaddr = captureData->xImage->data = reinterpret_cast<char*>(shmat(captureData->xShmSegmentInfo.shmid, 0, 0));
    captureData->xShmSegmentInfo.readOnly = false;

    XShmAttach(captureData->displayForX, &captureData->xShmSegmentInfo);
    if (!XShmGetImage(captureData->displayForX, xId, captureData->xImage, 0, 0, AllPlanes))
        goto failToCapture;

    if (angle == 90 || angle == 270) {
        captureData->rotateImageBuffer = reinterpret_cast<char*>(calloc(captureData->xImage->bytes_per_line * captureData->xImage->height, 1));
        convertImage((uint32_t*)captureData->xImage->data, (uint32_t*)captureData->rotateImageBuffer,
                     PIXMAN_a8b8g8r8, PIXMAN_a8b8g8r8,
                     width, height, height, width, angle);
        ret = captureData->rotateImageBuffer;
    } else if (angle == 180) {
        captureData->rotateImageBuffer = reinterpret_cast<char*>(calloc(captureData->xImage->bytes_per_line * captureData->xImage->height, 1));
        convertImage((uint32_t*)captureData->xImage->data, (uint32_t*)captureData->rotateImageBuffer,
                     PIXMAN_a8b8g8r8, PIXMAN_a8b8g8r8,
                     width, height, width, height, angle);
        ret = captureData->rotateImageBuffer;
    } else
        ret = captureData->xImage->data;

    GOTO_IF_FAIL(ret != 0, failToCapture);
    return ret;

failToCapture:
    if (captureData->xShmSegmentInfo.shmid != -1) {
        XShmDetach(captureData->displayForX, &captureData->xShmSegmentInfo);
        shmdt(captureData->xShmSegmentInfo.shmaddr);
        shmctl(captureData->xShmSegmentInfo.shmid, IPC_RMID, 0);
    }

    if (captureData->xImage)
        XDestroyImage(captureData->xImage);

    if (captureData->rotateImageBuffer)
        free(captureData->rotateImageBuffer);

    if (captureData->displayForX)
        XCloseDisplay(captureData->displayForX);

    free(captureData);

    return 0;
}

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
void ewkViewScreenShotForRotationDisplay(Evas_Object* ewkView, int angle)
{
    EwkViewImpl* impl = EwkViewImpl::fromEvasObject(ewkView);
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    Ecore_X_Window xWindow = impl->context->xWindow();
    if (WebCore::isMiniWindowMode(xWindow) || WebCore::isMultiWindowMode(xWindow))
        return;
#endif

    int deviceWidth = WebCore::getDefaultScreenResolution().width();
    int deviceHeight = WebCore::getDefaultScreenResolution().height();
    if (angle == 90 || angle == 270) {
        int tempWidth = deviceWidth;
        deviceWidth = deviceHeight;
        deviceHeight = tempWidth;
    }

    ewkViewScreenShotForRotationDelete(ewkView);

    CaptureData* captureData = reinterpret_cast<CaptureData*>(calloc(1, sizeof(CaptureData)));
    void* data = captureScreenShotForRotation(ewkView, captureData, angle);
    if (!data) {
        TIZEN_LOGE("Preparint rotation failed\n");
        return;
    }

    impl->setScreenShotForRotation(evas_object_image_filled_add(evas_object_evas_get(ewkView)));
    evas_object_data_set(impl->screenShotForRotation(), "ScreenShotForRotation", captureData);
    evas_object_image_size_set(impl->screenShotForRotation(), deviceWidth, deviceHeight);
    evas_object_image_data_set(impl->screenShotForRotation(), data);
    evas_object_image_smooth_scale_set(impl->screenShotForRotation(), EINA_FALSE);

    evas_object_move(impl->screenShotForRotation(), 0, 0);
    evas_object_resize(impl->screenShotForRotation(), deviceWidth, deviceHeight);
    evas_object_show(impl->screenShotForRotation());
}

void ewkViewScreenShotForRotationDelete(Evas_Object* ewkView)
{
    EwkViewImpl* impl = EwkViewImpl::fromEvasObject(ewkView);

    if (impl->screenShotForRotation()) {
        CaptureData* captureData = static_cast<CaptureData*>(evas_object_data_get(impl->screenShotForRotation(), "ScreenShotForRotation"));
        evas_object_hide(impl->screenShotForRotation());
        evas_object_del(impl->screenShotForRotation());
        impl->setScreenShotForRotation(0);

        if (captureData) {
            if (captureData->xShmSegmentInfo.shmid != -1) {
                XShmDetach(captureData->displayForX, &captureData->xShmSegmentInfo);
                shmdt(captureData->xShmSegmentInfo.shmaddr);
                shmctl(captureData->xShmSegmentInfo.shmid, IPC_RMID, 0);
            }

            if (captureData->xImage)
                XDestroyImage(captureData->xImage);

            if (captureData->rotateImageBuffer)
                free(captureData->rotateImageBuffer);

            if (captureData->displayForX)
                XCloseDisplay (captureData->displayForX);

            free(captureData);
        }
    }
}
#endif

#endif // #if PLATFORM(TIZEN)
