/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

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

#include "config.h"
#include "EWK2UnitTestEnvironment.h"

#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>

namespace EWK2UnitTest {

EWK2UnitTestEnvironment::EWK2UnitTestEnvironment(bool useX11Window)
    : m_defaultWidth(800)
    , m_defaultHeight(600)
    , m_useX11Window(useX11Window)
#if ENABLE(TIZEN_WEBKIT2_UNIT_TESTS)
    , m_resourcesPath(getenv("TEST_RESOURCES_DIR"))
#endif
{
#if ENABLE(TIZEN_WEBKIT2_UNIT_TESTS)
    if (!m_resourcesPath)
        fprintf(stderr, "You may need to set TEST_RESOURCES_DIR environment to proper reousrce directory");
#endif
}

const char* EWK2UnitTestEnvironment::defaultTestPageUrl() const
{
#if ENABLE(TIZEN_WEBKIT2_UNIT_TESTS)
    static CString defaultTestPageUrl = makeString("file://", m_resourcesPath ? m_resourcesPath : TEST_RESOURCES_DIR"/", "default_test_page.html").utf8();
    return defaultTestPageUrl.data();
#else
    return "file://"TEST_RESOURCES_DIR"/default_test_page.html";
#endif
}

const char* EWK2UnitTestEnvironment::defaultTheme() const
{
    return TEST_THEME_DIR"/default.edj";
}

const char* EWK2UnitTestEnvironment::injectedBundleSample() const
{
    return TEST_RESOURCES_DIR "/libewk2UnitTestInjectedBundleSample.so";
}

CString EWK2UnitTestEnvironment::urlForResource(const char* resource)
{
#if ENABLE(TIZEN_WEBKIT2_UNIT_TESTS)
    return makeString("file://", m_resourcesPath ? m_resourcesPath : TEST_RESOURCES_DIR"/", resource).utf8();
#else
    return makeString("file://"TEST_RESOURCES_DIR"/", resource).utf8();
#endif
}

CString EWK2UnitTestEnvironment::pathForResource(const char* resource)
{
    StringBuilder builder;
#if ENABLE(TIZEN_WEBKIT2_UNIT_TESTS)
    builder.append(m_resourcesPath ? m_resourcesPath : TEST_RESOURCES_DIR "/");
#else
    builder.appendLiteral(TEST_RESOURCES_DIR "/");
#endif
    builder.append(resource);
    return builder.toString().utf8();
}

} // namespace EWK2UnitTest
