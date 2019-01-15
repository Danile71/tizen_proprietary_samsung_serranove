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
#include "EFLTestLauncher.h"
#include "EWebKit.h"
#include <gtest/gtest.h>

using namespace EFLUnitTests;

void editableSetTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    EXPECT_EQ(true, ewk_view_editable_set(o, EINA_FALSE));
    EXPECT_EQ(EINA_FALSE, ewk_view_editable_get(o));
    END_TEST();
}

TEST(EwkEditableTests, EditableSetTest)
{
    RUN_TEST(editableSetTestCallback, "load,finished", 0);
}
