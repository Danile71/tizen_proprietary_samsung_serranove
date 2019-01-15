/*
 * Copyright (C) 2011 ProFUSION Embedded Systems. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Red istributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DumpRenderTreeView_h
#define DumpRenderTreeView_h

#if ENABLE(TIZEN_WEBKIT_EFL_DRT)
#include <Evas.h>

Evas_Object* drtViewAdd(Evas*);

#else // ENABLE(TIZEN_WEBKIT_EFL_DRT)

#include <gtk/gtk.h> //Only for testing, will be changed
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <string>
#include <EWebKit.h>
#include "utility.h"

class LayoutTestController;
class GCController;
class AccessibilityController;
class EWebKitView;
class EventSenderEfl;

class EWebKitViewItem : public ENoncopyable
{
    static EWebKitView* webKitView;
public:
    static EWebKitView* instance(EWebKitView* view = 0);
};

class EWebKitView
{
protected:
    Evas* evas;
    Ecore_Evas* ecore_evas;        
    Evas_Object* webView;
    Evas_Object* mainFrame;
public:
    EWebKitView(){}
    virtual ~EWebKitView(){}
    
    Evas_Object* getWebView(){ return webView; }
    Evas_Object* getMainFrame(){ return mainFrame; }
    Evas* getEvas(){ return evas; }
    
    virtual void webViewInitialize(){}
    virtual int webViewExec();
};

class DumpRenderTreeView : public EWebKitView
{
    static const int MAX_VIEW_WIDTH = 480;
    static const int MAX_VIEW_HEIGHT = 800;
        
    std::string layoutTestName;
    std::string expectedPixelHash;
    bool dumpTree;
    bool dumpPixels;
    bool flushOutput;
    bool loadFinished;

    Ecore_Timer* timer;
    
    LayoutTestController* gLayoutTestController;
    GCController* gcController;
    AccessibilityController* axController;
    EventSenderEfl* eventSender;
    
protected:
    static void onLoadStarted(void* data, Evas_Object* webView, void* event_info);
    static void onLoadFinished(void* data, Evas_Object* webView, void* event_info);
    static void onTitleChanged(void* data, Evas_Object* webView, void* event_info);
    static void onWindowObjectCleared(void* data, Evas_Object* webView, void* event_info);
    static void onMouseDown(void* data, Evas_Object* webView, void* event_info);
    static void onMouseUp(void* data, Evas_Object* webView, void* event_info);
    static void onMouseMoveTo(void* data, Evas_Object* webView, void* event_info);
    
    
    static int processWork(void* data);

    void initializeJSProperties();
    void layoutTestControllerProperties();
    void bindDumpRenderTreeViewEvents();
    std::string dumpFrameAsText(Evas_Object* o);
    std::string dumpBackForwardList();
    std::string dumpHistoryItem(Ewk_History_Item* hItem, int indent, bool currentItem);
    bool shouldLogFrameLoadDelegates();
    bool shouldOpenWebInspector();

public:
    DumpRenderTreeView(const std::string& expectedPixelHash, const std::string& layoutTestName, bool dumpTree, bool dumpPixel, bool flushOutput);
    ~DumpRenderTreeView();
    virtual void webViewInitialize();
    static void initializeFonts();
    LayoutTestController& getLayoutTestController(){ return *gLayoutTestController; }
    void dump();
};

class DumpRenderTree
{
    int argc;
    char** argv;

public:
    DumpRenderTree(int argc, char** argv);
    ~DumpRenderTree(){}

    void urlCorrection(const char* url, std::string& testUrl);
    int runTest();
    int runTest(const char* test, bool dumpTree, bool dumpPixel, bool flushOutput);
};
#endif
#endif // DumpRenderTreeView_h
