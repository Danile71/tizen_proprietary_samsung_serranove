/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Jan Michael Alonzo
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "AccessibilityUIElement.h"

#include <JavaScriptCore/JSStringRef.h>
#include <wtf/Assertions.h>


AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement element)
    : m_element(element)
{
}

AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement& other)
    : m_element(other.m_element)
{
}

AccessibilityUIElement::~AccessibilityUIElement()
{
}

void AccessibilityUIElement::getLinkedUIElements(Vector<AccessibilityUIElement>& elements)
{
}

void AccessibilityUIElement::getDocumentLinks(Vector<AccessibilityUIElement>&)
{
}

void AccessibilityUIElement::getChildren(Vector<AccessibilityUIElement>& children)
{
    int count = childrenCount();
}

void AccessibilityUIElement::getChildrenWithRange(Vector<AccessibilityUIElement>& elementVector, unsigned start, unsigned end)
{
}

int AccessibilityUIElement::rowCount()
{
    return 0;
}

int AccessibilityUIElement::columnCount()
{
    return 0;
}

int AccessibilityUIElement::childrenCount()
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::elementAtPoint(int x, int y)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::getChildAtIndex(unsigned index)
{
    return 0;
}

unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement* element)
{ 
    return 0;
}

JSStringRef AccessibilityUIElement::allAttributes()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfLinkedUIElements()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfDocumentLinks()
{
    return JSStringCreateWithCharacters(0, 0);
}

AccessibilityUIElement AccessibilityUIElement::titleUIElement()
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::parentElement()
{
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfChildren()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::parameterizedAttributeNames()
{
     return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::role()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::subrole()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::roleDescription()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::title()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::description()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::stringValue()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::language()
{
    return JSStringCreateWithCharacters(0, 0);
}

double AccessibilityUIElement::x()
{
    return 0.0;
}

double AccessibilityUIElement::y()
{
    return 0.0;
}

double AccessibilityUIElement::width()
{
    return 0.0;
}

double AccessibilityUIElement::height()
{
    return 0.0;
}

double AccessibilityUIElement::clickPointX()
{
    return 0.0;
}

double AccessibilityUIElement::clickPointY()
{
    return 0.0;
}

JSStringRef AccessibilityUIElement::orientation() const
{
    return JSStringCreateWithCharacters(0, 0);
}

double AccessibilityUIElement::intValue() const
{
    return 0.0;
}

double AccessibilityUIElement::minValue()
{
    return 0.0;
}

double AccessibilityUIElement::maxValue()
{
    return 0.0;
}

JSStringRef AccessibilityUIElement::valueDescription()
{
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::isEnabled()
{
    return false;
}


int AccessibilityUIElement::insertionPointLineNumber()
{
    return 0;
}

bool AccessibilityUIElement::isActionSupported(JSStringRef action)
{
    return false;
}

bool AccessibilityUIElement::isRequired() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isSelected() const
{
    // FIXME: implement
    return false;
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    // FIXME: implement
    return 0;
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    return false;
}
 
JSStringRef AccessibilityUIElement::ariaDropEffects() const
{   
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::isExpanded() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isChecked() const
{
}

JSStringRef AccessibilityUIElement::attributesOfColumnHeaders()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRowHeaders()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfColumns()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRows()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfVisibleCells()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfHeader()
{
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::indexInTable()
{
    return 0;
}

JSStringRef AccessibilityUIElement::rowIndexRange()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::columnIndexRange()
{
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::lineForIndex(int)
{
    return 0;
}

JSStringRef AccessibilityUIElement::boundsForRange(unsigned location, unsigned length)
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::stringForRange(unsigned, unsigned) 
{
    return JSStringCreateWithCharacters(0, 0);
} 

AccessibilityUIElement AccessibilityUIElement::cellForColumnAndRow(unsigned column, unsigned row)
{
    return 0;
}

JSStringRef AccessibilityUIElement::selectedTextRange()
{
    return JSStringCreateWithCharacters(0, 0);
}

void AccessibilityUIElement::setSelectedTextRange(unsigned location, unsigned length)
{
}

JSStringRef AccessibilityUIElement::stringAttributeValue(JSStringRef attribute)
{
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::boolAttributeValue(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isAttributeSettable(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef attribute)
{
    return false;
}

void AccessibilityUIElement::increment()
{
    // FIXME: implement
}

void AccessibilityUIElement::decrement()
{
    // FIXME: implement
}

void AccessibilityUIElement::press()
{
    // FIXME: implement
}

void AccessibilityUIElement::showMenu()
{
    // FIXME: implement
}

AccessibilityUIElement AccessibilityUIElement::disclosedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedByRow()
{
    return 0;
}

JSStringRef AccessibilityUIElement::accessibilityValue() const
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::documentEncoding()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::documentURI()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::url()
{
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::addNotificationListener(JSObjectRef functionCallback)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isSelectable() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isMultiSelectable() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isVisible() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isOffScreen() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isCollapsed() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::hasPopup() const
{
    // FIXME: implement
    return false;
}

void AccessibilityUIElement::takeFocus()
{
    // FIXME: implement
}

void AccessibilityUIElement::takeSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::addSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::removeSelection()
{
    // FIXME: implement
}

JSStringRef AccessibilityUIElement::helpText() const
{
    return JSStringCreateWithCharacters(0, 0);
}

void AccessibilityUIElement::removeNotificationListener()
{
}
