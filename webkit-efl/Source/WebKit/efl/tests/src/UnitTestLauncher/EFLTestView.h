/*
    Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#ifndef EFLTestView_h
#define EFLTestView_h

#include <Ecore_Evas.h>
#include <Evas.h>

namespace EFLUnitTests {

class EFLTestEcoreEvas {
public:
    EFLTestEcoreEvas();
    EFLTestEcoreEvas(const char *engine_name, int viewport_x, int viewport_y, int viewport_w, int viewport_h, const char *extra_options);
    ~EFLTestEcoreEvas();

    Evas* getEvas();
    void show();

private:
    Ecore_Evas *m_ecore_evas;
};

class EFLTestView {
public:
    enum EwkViewType {
        SingleView = 0,
        TiledView,
    };

    explicit EFLTestView(Evas*);
    EFLTestView(Evas*, const char* url);
    EFLTestView(Evas*, EwkViewType, const char* url);
    EFLTestView(Evas*, EwkViewType, const char* url, int width, int height);

    virtual ~EFLTestView();

    Evas_Object* getWebView() { return m_webView; }
    Evas_Object* getMainFrame();
    Evas* getEvas();
    void show();

    Eina_Bool init();
    void bindEvents(void (*callback)(void*, Evas_Object*, void*), const char* eventName, void* ptr);
protected:
    char* createTestUrl(const char* url);
private:
    EFLTestView(const EFLTestView&);
    EFLTestView operator=(const EFLTestView&);

    Evas* m_evas;
    Evas_Object* m_webView;

    char* m_url;
    int m_width, m_height;
    EwkViewType m_defaultViewType;
};

}

#endif
