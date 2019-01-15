/*
    Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef WebKitDOM_Defines_h
#define WebKitDOM_Defines_h

typedef struct _WebKitDOM_CSSRule    WebKitDOM_CSSRule;
typedef struct _WebKitDOM_CSSRuleList    WebKitDOM_CSSRuleList;
typedef struct _WebKitDOM_CSSStyleDeclaration    WebKitDOM_CSSStyleDeclaration;
typedef struct _WebKitDOM_CSSStyleSheet    WebKitDOM_CSSStyleSheet;
typedef struct _WebKitDOM_CSSValue    WebKitDOM_CSSValue;
typedef struct _WebKitDOM_MediaList    WebKitDOM_MediaList;
typedef struct _WebKitDOM_MediaQueryList    WebKitDOM_MediaQueryList;
typedef struct _WebKitDOM_MediaQueryListListener    WebKitDOM_MediaQueryListListener;
typedef struct _WebKitDOM_StyleMedia    WebKitDOM_StyleMedia;
typedef struct _WebKitDOM_StyleSheet    WebKitDOM_StyleSheet;
typedef struct _WebKitDOM_StyleSheetList    WebKitDOM_StyleSheetList;
typedef struct _WebKitDOM_Attr    WebKitDOM_Attr;
typedef struct _WebKitDOM_CDATASection    WebKitDOM_CDATASection;
typedef struct _WebKitDOM_CharacterData    WebKitDOM_CharacterData;
typedef struct _WebKitDOM_Comment    WebKitDOM_Comment;
typedef struct _WebKitDOM_DocumentFragment    WebKitDOM_DocumentFragment;
typedef struct _WebKitDOM_Document    WebKitDOM_Document;
typedef struct _WebKitDOM_DocumentType    WebKitDOM_DocumentType;
typedef struct _WebKitDOM_DOMImplementation    WebKitDOM_DOMImplementation;
typedef struct _WebKitDOM_DOMStringList    WebKitDOM_DOMStringList;
typedef struct _WebKitDOM_DOMStringMap    WebKitDOM_DOMStringMap;
typedef struct _WebKitDOM_Element    WebKitDOM_Element;
typedef struct _WebKitDOM_EntityReference    WebKitDOM_EntityReference;
typedef struct _WebKitDOM_Event    WebKitDOM_Event;
typedef struct _WebKitDOM_EventListener    WebKitDOM_EventListener;
typedef struct _WebKitDOM_MessagePort    WebKitDOM_MessagePort;
typedef struct _WebKitDOM_MouseEvent    WebKitDOM_MouseEvent;
typedef struct _WebKitDOM_NamedNodeMap    WebKitDOM_NamedNodeMap;
typedef struct _WebKitDOM_NodeFilter    WebKitDOM_NodeFilter;
typedef struct _WebKitDOM_Node    WebKitDOM_Node;
typedef struct _WebKitDOM_NodeIterator    WebKitDOM_NodeIterator;
typedef struct _WebKitDOM_NodeList    WebKitDOM_NodeList;
typedef struct _WebKitDOM_ProcessingInstruction    WebKitDOM_ProcessingInstruction;
typedef struct _WebKitDOM_Range    WebKitDOM_Range;
typedef struct _WebKitDOM_Text    WebKitDOM_Text;
typedef struct _WebKitDOM_TreeWalker    WebKitDOM_TreeWalker;
typedef struct _WebKitDOM_UIEvent    WebKitDOM_UIEvent;
typedef struct _WebKitDOM_Blob    WebKitDOM_Blob;
typedef struct _WebKitDOM_File    WebKitDOM_File;
typedef struct _WebKitDOM_FileList    WebKitDOM_FileList;
typedef struct _WebKitDOM_DOMSettableTokenList    WebKitDOM_DOMSettableTokenList;
typedef struct _WebKitDOM_DOMTokenList    WebKitDOM_DOMTokenList;
typedef struct _WebKitDOM_HTMLAnchorElement    WebKitDOM_HTMLAnchorElement;
typedef struct _WebKitDOM_HTMLAppletElement    WebKitDOM_HTMLAppletElement;
typedef struct _WebKitDOM_HTMLAreaElement    WebKitDOM_HTMLAreaElement;
typedef struct _WebKitDOM_HTMLAudioElement    WebKitDOM_HTMLAudioElement;
typedef struct _WebKitDOM_HTMLBaseElement    WebKitDOM_HTMLBaseElement;
typedef struct _WebKitDOM_HTMLBaseFontElement    WebKitDOM_HTMLBaseFontElement;
typedef struct _WebKitDOM_HTMLBlockquoteElement    WebKitDOM_HTMLBlockquoteElement;
typedef struct _WebKitDOM_HTMLBodyElement    WebKitDOM_HTMLBodyElement;
typedef struct _WebKitDOM_HTMLBRElement    WebKitDOM_HTMLBRElement;
typedef struct _WebKitDOM_HTMLButtonElement    WebKitDOM_HTMLButtonElement;
typedef struct _WebKitDOM_HTMLCanvasElement    WebKitDOM_HTMLCanvasElement;
typedef struct _WebKitDOM_HTMLCollection    WebKitDOM_HTMLCollection;
typedef struct _WebKitDOM_HTMLDetailsElement    WebKitDOM_HTMLDetailsElement;
typedef struct _WebKitDOM_HTMLDirectoryElement    WebKitDOM_HTMLDirectoryElement;
typedef struct _WebKitDOM_HTMLDivElement    WebKitDOM_HTMLDivElement;
typedef struct _WebKitDOM_HTMLDListElement    WebKitDOM_HTMLDListElement;
typedef struct _WebKitDOM_HTMLDocument    WebKitDOM_HTMLDocument;
typedef struct _WebKitDOM_HTMLElement    WebKitDOM_HTMLElement;
typedef struct _WebKitDOM_HTMLEmbedElement    WebKitDOM_HTMLEmbedElement;
typedef struct _WebKitDOM_HTMLFieldSetElement    WebKitDOM_HTMLFieldSetElement;
typedef struct _WebKitDOM_HTMLFontElement    WebKitDOM_HTMLFontElement;
typedef struct _WebKitDOM_HTMLFormElement    WebKitDOM_HTMLFormElement;
typedef struct _WebKitDOM_HTMLFrameElement    WebKitDOM_HTMLFrameElement;
typedef struct _WebKitDOM_HTMLFrameSetElement    WebKitDOM_HTMLFrameSetElement;
typedef struct _WebKitDOM_HTMLHeadElement    WebKitDOM_HTMLHeadElement;
typedef struct _WebKitDOM_HTMLHeadingElement    WebKitDOM_HTMLHeadingElement;
typedef struct _WebKitDOM_HTMLHRElement    WebKitDOM_HTMLHRElement;
typedef struct _WebKitDOM_HTMLHtmlElement    WebKitDOM_HTMLHtmlElement;
typedef struct _WebKitDOM_HTMLIFrameElement    WebKitDOM_HTMLIFrameElement;
typedef struct _WebKitDOM_HTMLImageElement    WebKitDOM_HTMLImageElement;
typedef struct _WebKitDOM_HTMLInputElement    WebKitDOM_HTMLInputElement;
typedef struct _WebKitDOM_HTMLIsIndexElement    WebKitDOM_HTMLIsIndexElement;
typedef struct _WebKitDOM_HTMLKeygenElement    WebKitDOM_HTMLKeygenElement;
typedef struct _WebKitDOM_HTMLLabelElement    WebKitDOM_HTMLLabelElement;
typedef struct _WebKitDOM_HTMLLegendElement    WebKitDOM_HTMLLegendElement;
typedef struct _WebKitDOM_HTMLLIElement    WebKitDOM_HTMLLIElement;
typedef struct _WebKitDOM_HTMLLinkElement    WebKitDOM_HTMLLinkElement;
typedef struct _WebKitDOM_HTMLMapElement    WebKitDOM_HTMLMapElement;
typedef struct _WebKitDOM_HTMLMarqueeElement    WebKitDOM_HTMLMarqueeElement;
typedef struct _WebKitDOM_HTMLMediaElement    WebKitDOM_HTMLMediaElement;
typedef struct _WebKitDOM_HTMLMenuElement    WebKitDOM_HTMLMenuElement;
typedef struct _WebKitDOM_HTMLMetaElement    WebKitDOM_HTMLMetaElement;
typedef struct _WebKitDOM_HTMLModElement    WebKitDOM_HTMLModElement;
typedef struct _WebKitDOM_HTMLObjectElement    WebKitDOM_HTMLObjectElement;
typedef struct _WebKitDOM_HTMLOListElement    WebKitDOM_HTMLOListElement;
typedef struct _WebKitDOM_HTMLOptGroupElement    WebKitDOM_HTMLOptGroupElement;
typedef struct _WebKitDOM_HTMLOptionElement    WebKitDOM_HTMLOptionElement;
typedef struct _WebKitDOM_HTMLOptionsCollection    WebKitDOM_HTMLOptionsCollection;
typedef struct _WebKitDOM_HTMLParagraphElement    WebKitDOM_HTMLParagraphElement;
typedef struct _WebKitDOM_HTMLParamElement    WebKitDOM_HTMLParamElement;
typedef struct _WebKitDOM_HTMLPreElement    WebKitDOM_HTMLPreElement;
typedef struct _WebKitDOM_HTMLQuoteElement    WebKitDOM_HTMLQuoteElement;
typedef struct _WebKitDOM_HTMLScriptElement    WebKitDOM_HTMLScriptElement;
typedef struct _WebKitDOM_HTMLSelectElement    WebKitDOM_HTMLSelectElement;
typedef struct _WebKitDOM_HTMLStyleElement    WebKitDOM_HTMLStyleElement;
typedef struct _WebKitDOM_HTMLTableCaptionElement    WebKitDOM_HTMLTableCaptionElement;
typedef struct _WebKitDOM_HTMLTableCellElement    WebKitDOM_HTMLTableCellElement;
typedef struct _WebKitDOM_HTMLTableColElement    WebKitDOM_HTMLTableColElement;
typedef struct _WebKitDOM_HTMLTableElement    WebKitDOM_HTMLTableElement;
typedef struct _WebKitDOM_HTMLTableRowElement    WebKitDOM_HTMLTableRowElement;
typedef struct _WebKitDOM_HTMLTableSectionElement    WebKitDOM_HTMLTableSectionElement;
typedef struct _WebKitDOM_HTMLTextAreaElement    WebKitDOM_HTMLTextAreaElement;
typedef struct _WebKitDOM_HTMLTimeElement    WebKitDOM_HTMLTimeElement;
typedef struct _WebKitDOM_HTMLTitleElement    WebKitDOM_HTMLTitleElement;
typedef struct _WebKitDOM_HTMLUListElement    WebKitDOM_HTMLUListElement;
typedef struct _WebKitDOM_HTMLVideoElement    WebKitDOM_HTMLVideoElement;
typedef struct _WebKitDOM_MediaController    WebKitDOM_MediaController;
typedef struct _WebKitDOM_MediaError    WebKitDOM_MediaError;
typedef struct _WebKitDOM_TimeRanges    WebKitDOM_TimeRanges;
typedef struct _WebKitDOM_ValidityState    WebKitDOM_ValidityState;
typedef struct _WebKitDOM_DOMApplicationCache    WebKitDOM_DOMApplicationCache;
typedef struct _WebKitDOM_BarInfo    WebKitDOM_BarInfo;
typedef struct _WebKitDOM_Console    WebKitDOM_Console;
typedef struct _WebKitDOM_DOMSelection    WebKitDOM_DOMSelection;
typedef struct _WebKitDOM_DOMWindow    WebKitDOM_DOMWindow;
typedef struct _WebKitDOM_History    WebKitDOM_History;
typedef struct _WebKitDOM_MemoryInfo    WebKitDOM_MemoryInfo;
typedef struct _WebKitDOM_Navigator    WebKitDOM_Navigator;
typedef struct _WebKitDOM_Screen    WebKitDOM_Screen;
typedef struct _WebKitDOM_WebKitAnimation    WebKitDOM_WebKitAnimation;
typedef struct _WebKitDOM_WebKitAnimationList    WebKitDOM_WebKitAnimationList;
typedef struct _WebKitDOM_WebKitPoint    WebKitDOM_WebKitPoint;
typedef struct _WebKitDOM_DOMMimeTypeArray    WebKitDOM_DOMMimeTypeArray;
typedef struct _WebKitDOM_DOMMimeType    WebKitDOM_DOMMimeType;
typedef struct _WebKitDOM_DOMPluginArray    WebKitDOM_DOMPluginArray;
typedef struct _WebKitDOM_DOMPlugin    WebKitDOM_DOMPlugin;
typedef struct _WebKitDOM_Storage    WebKitDOM_Storage;
typedef struct _WebKitDOM_XPathExpression    WebKitDOM_XPathExpression;
typedef struct _WebKitDOM_XPathNSResolver    WebKitDOM_XPathNSResolver;
typedef struct _WebKitDOM_XPathResult    WebKitDOM_XPathResult;
typedef struct _WebKitDOM_EventTarget    WebKitDOM_EventTarget;

typedef struct _WebKitDOM_String    WebKitDOM_String;

#endif //WebKitDOM_Defines_h

