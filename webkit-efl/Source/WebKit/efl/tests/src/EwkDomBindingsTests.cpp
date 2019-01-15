#include <gtest/gtest.h>
#include <EWebKit.h>
#include <EFLTestLauncher.h>
#include <EFLTestUtility.h>

#include <gtest/internal/gtest-filepath.h>

#include <WebKitDOM_Object.h>
#include <WebKitDOM_Node.h>
#include <WebKitDOM_NodeList.h>
#include <WebKitDOM_Document.h>
#include <WebKitDOM_Element.h>
#include <WebKitDOM_HTMLElement.h>
#include <WebKitDOM_HTMLHeadElement.h>
#include <WebKitDOM_HTMLCollection.h>
#include <WebKitDOM_HTMLAnchorElement.h>
#include <WebKitDOM_String.h>

#include <string.h>

using namespace EFLUnitTests;

static void onEwkDomBindingsDocumentAcquireTest(void* data, Evas_Object* webView, void* event_info)
{
    // First we need to define document object as instance of WebKitDOM_Document type.
    WebKitDOM_Document document;
    // Before using document we need to initialize it.
    WEBKITDOM_DOCUMENT_INIT(&document);

    // Now we are ready to acquire document object.
    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE && !WEBKITDOM_ISNULL(&document));

    // After all actions we need to deinit document object.
    WEBKITDOM_DEINIT(&document);

    END_TEST();
}

TEST(EwkDomBindingsTests, EwkDomBindingsDocumentAcquireTest)
{
    RUN_TEST("http://www.webkit.org", onEwkDomBindingsDocumentAcquireTest, "load,finished", NULL);
}

static void onEwkDomBindingsGetTitleTest(void* data, Evas_Object* webView, void* event_info)
{
    // As above we need to define, initialize and acquire document object.
    WebKitDOM_Document document;
    WEBKITDOM_DOCUMENT_INIT(&document);

    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE);
    if (ret == EINA_FALSE || WEBKITDOM_ISNULL(&document))
        END_TEST();

    // After successful getting document object, we can check document title.
    WebKitDOM_String titleString;
    WEBKITDOM_STRING_INIT(&titleString);

    ewk_webkitdom_document_get_title(&document, &titleString);

    // This function creates duplicate of title string so it needs to be released after usage.
    char* title = ewk_webkitdom_string_get_cstring(&titleString);
    EXPECT_EQ(0, strcmp(title, "Test Document Title"));

    // Free title string and deinit document object.
    free(title);

    WEBKITDOM_DEINIT(&titleString);
    WEBKITDOM_DEINIT(&document);

    END_TEST();
}


TEST(EwkDomBindingsTests, EwkDomBindingsGetTitleTest)
{
    char* filepath_str = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DomBindingsTestDocument);
    ASSERT_EQ(true, filepath_str != NULL);
    RUN_TEST(filepath_str, onEwkDomBindingsGetTitleTest, "load,finished", NULL);
    free(filepath_str);
}

static void onEwkDomBindingsSetTitleTest(void* data, Evas_Object* webView, void* event_info)
{
    WebKitDOM_Document document;
    WEBKITDOM_DOCUMENT_INIT(&document);

    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE);
    if (ret == EINA_FALSE || WEBKITDOM_ISNULL(&document))
        END_TEST();

    // Try to set new title for document and check if everything went okay.
    WebKitDOM_String titleString;
    WEBKITDOM_STRING_INIT(&titleString);

    ewk_webkitdom_string_set_data(&titleString, "Test Document Title Set By Test");
    ewk_webkitdom_document_set_title(&document, &titleString);

    WebKitDOM_String tmpTitleString;
    WEBKITDOM_STRING_INIT(&tmpTitleString);

    ewk_webkitdom_document_get_title(&document, &tmpTitleString);
    char* title =  ewk_webkitdom_string_get_cstring(&tmpTitleString);
    EXPECT_EQ(0, strcmp(title, "Test Document Title Set By Test"));

    // Free and deinit.
    free(title);
    WEBKITDOM_DEINIT(&tmpTitleString);
    WEBKITDOM_DEINIT(&titleString);
    WEBKITDOM_DEINIT(&document);

    END_TEST();
}

TEST(EwkDomBindingsTests, EwkDomBindingsSetTitleTest)
{
    char* filepath_str = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DomBindingsTestDocument);
    ASSERT_EQ(true, filepath_str != NULL);
    RUN_TEST(filepath_str, onEwkDomBindingsSetTitleTest, "load,finished", NULL);
    free(filepath_str);
}

static void onEwkDomBindingsGetElementsByTagNameTest(void* data, Evas_Object* webView, void* event_info)
{
    WebKitDOM_Document document;
    WEBKITDOM_DOCUMENT_INIT(&document);

    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE);
    if (ret == EINA_FALSE || WEBKITDOM_ISNULL(&document))
        END_TEST();

    // Define and initialize list as WebKitDOM_NodeList.
    WebKitDOM_NodeList list;
    WEBKITDOM_NODELIST_INIT(&list);

    // Get all elements with tagname "LI".
    WebKitDOM_String liString;
    WEBKITDOM_STRING_INIT(&liString);

    ewk_webkitdom_string_set_data(&liString, "LI");
    ewk_webkitdom_document_get_elements_by_tag_name(&document, &liString, &list);
    WEBKITDOM_DEINIT(&liString);
    EXPECT_EQ(false, WEBKITDOM_ISNULL(&list));
    // Since list is needed to continue test, if we fail to acquire it, we are ending this test.
    if (WEBKITDOM_ISNULL(&list))
        END_TEST();

    // Check list length. Base on TestDocument.html we expect it to have 5 elements.
    unsigned length = ewk_webkitdom_nodelist_get_length(&list);
    EXPECT_EQ(5, length);

    // Now we will iterate through list to check it's elements.
    for (int i = 0; i < length; ++i) {
        // Define and initialize temporary item.
        WebKitDOM_Node item;
        WEBKITDOM_NODE_INIT(&item);

        // Get list item with index i.
        ewk_webkitdom_nodelist_item(&list, i, &item);
        EXPECT_EQ(false, WEBKITDOM_ISNULL(&item));
        if (WEBKITDOM_ISNULL(&item))
            continue;

        // Now we will cast item (WebKitDOM_Node) to WebKitDOM_Element type.
        // This is not safe, so user should be aware which type could be casted on which other type.
        WebKitDOM_Element* pElement = WEBKITDOM_CAST(WebKitDOM_Element, &item);

        // Check tag name.
        WebKitDOM_String tmpTagName;
        WEBKITDOM_STRING_INIT(&tmpTagName);

        ewk_webkitdom_element_get_tag_name(pElement, &tmpTagName);
        char* tagName = ewk_webkitdom_string_get_cstring(&tmpTagName);
        EXPECT_EQ(0, strcmp(tagName, "LI"));

        WebKitDOM_HTMLElement* pHtmlElement = WEBKITDOM_CAST(WebKitDOM_HTMLElement, &item);

        char expected_str[250];
        sprintf(expected_str, "Element %d", i + 1);

        // Check inner text.
        WebKitDOM_String tmpInnerText;
        WEBKITDOM_STRING_INIT(&tmpInnerText);

        ewk_webkitdom_htmlelement_get_inner_text(pHtmlElement, &tmpInnerText);
        char* innerText = ewk_webkitdom_string_get_cstring(&tmpInnerText);
        EXPECT_EQ(0, strcmp(innerText, expected_str));

        // Free and deinitialize
        free(tagName);
        WEBKITDOM_DEINIT(&tmpTagName);
        free(innerText);
        WEBKITDOM_DEINIT(&tmpInnerText);
        WEBKITDOM_DEINIT(&item);
    }

    // Deinitialize
    WEBKITDOM_DEINIT(&list);
    WEBKITDOM_DEINIT(&document);

    END_TEST();
}

TEST(EwkDomBindingsTests, EwkDomBindingsGetElementsByTagNameTest)
{
    char* filepath_str = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DomBindingsTestDocument);
    ASSERT_EQ(true, filepath_str != NULL);
    RUN_TEST(filepath_str, onEwkDomBindingsGetElementsByTagNameTest, "load,finished", NULL);
    free(filepath_str);
}

static void onEwkDomBindingsGetElementsByClassNameTest(void* data, Evas_Object* webView, void* event_info)
{
    WebKitDOM_Document document;
    WEBKITDOM_DOCUMENT_INIT(&document);

    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE);
    if (ret == EINA_FALSE || WEBKITDOM_ISNULL(&document))
        END_TEST();

    WebKitDOM_NodeList list;
    WEBKITDOM_NODELIST_INIT(&list);

    // Get all elements with class name equal to "test".
    WebKitDOM_String testString;
    WEBKITDOM_STRING_INIT(&testString);

    ewk_webkitdom_string_set_data(&testString, "test");
    ewk_webkitdom_document_get_elements_by_class_name(&document, &testString, &list);
    WEBKITDOM_DEINIT(&testString);
    EXPECT_EQ(false, WEBKITDOM_ISNULL(&list));
    if (WEBKITDOM_ISNULL(&list))
        END_TEST();

    // We expect to find two of those elements.
    unsigned length = ewk_webkitdom_nodelist_get_length(&list);
    EXPECT_EQ(2, length);

    // Now we will iterate through list to check it's elements.
    for (int i = 0; i < length; ++i) {
        WebKitDOM_Node item;
        WEBKITDOM_NODE_INIT(&item);

        // Get list item with index i.
        ewk_webkitdom_nodelist_item(&list, i, &item);
        EXPECT_EQ(false, WEBKITDOM_ISNULL(&item));
        if (WEBKITDOM_ISNULL(&item))
            continue;

        WebKitDOM_Element* pElement = WEBKITDOM_CAST(WebKitDOM_Element, &item);

        WebKitDOM_String tmpTagName;
        WEBKITDOM_STRING_INIT(&tmpTagName);

        ewk_webkitdom_element_get_tag_name(pElement, &tmpTagName);
        char* tagName = ewk_webkitdom_string_get_cstring(&tmpTagName);
        EXPECT_EQ(0, strcmp(tagName, "DIV"));

        // Free and deinitialize
        free(tagName);
        WEBKITDOM_DEINIT(&item);
        WEBKITDOM_DEINIT(&tmpTagName);
    }

    // Deinitialize
    WEBKITDOM_DEINIT(&list);
    WEBKITDOM_DEINIT(&document);

    END_TEST();
}

TEST(EwkDomBindingsTests, EwkDomBindingsGetElementsByClassNameTest)
{
    char* filepath_str = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DomBindingsTestDocument);
    ASSERT_EQ(true, filepath_str != NULL);
    RUN_TEST(filepath_str, onEwkDomBindingsGetElementsByClassNameTest, "load,finished", NULL);
    free(filepath_str);
}

static void onEwkDomBindingsGetElementsByIdTest(void* data, Evas_Object* webView, void* event_info)
{
    WebKitDOM_Document document;
    WEBKITDOM_DOCUMENT_INIT(&document);

    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE);
    if (ret == EINA_FALSE || WEBKITDOM_ISNULL(&document))
        END_TEST();

    WebKitDOM_Element element;
    WEBKITDOM_DOCUMENT_INIT(&element);

    // Get element with id name equal to "id1".
    WebKitDOM_String id1String;
    WEBKITDOM_STRING_INIT(&id1String);

    ewk_webkitdom_string_set_data(&id1String, "id1");
    ewk_webkitdom_document_get_element_by_id(&document, &id1String, &element);
    WEBKITDOM_DEINIT(&id1String);
    EXPECT_EQ(false,  WEBKITDOM_ISNULL(&element));

    // Try to acquire element with id name equal to "id_no_present" which ofcourse is not present in
    // TestDocument.html so we don't expect to find any.
    // What is important, variable element could be used in next function call for acquiring its return value without
    // deinitialization/initialization - this is handled inside of function.
    WebKitDOM_String id_no_presentString;
    WEBKITDOM_STRING_INIT(&id_no_presentString);

    ewk_webkitdom_string_set_data(&id_no_presentString, "id_no_present");
    ewk_webkitdom_document_get_element_by_id(&document, &id_no_presentString, &element);
    WEBKITDOM_DEINIT(&id_no_presentString);
    EXPECT_EQ(true, WEBKITDOM_ISNULL(&element));

    // Get element with id name equal to "id2".
    WebKitDOM_String id2String;
    WEBKITDOM_STRING_INIT(&id2String);

    ewk_webkitdom_string_set_data(&id2String, "id2");
    ewk_webkitdom_document_get_element_by_id(&document, &id2String, &element);
    WEBKITDOM_DEINIT(&id2String);
    EXPECT_EQ(false, WEBKITDOM_ISNULL(&element));

    WebKitDOM_HTMLElement* pHtmlElement = WEBKITDOM_CAST(WebKitDOM_HTMLElement, &element);

    WebKitDOM_String tmpInnerText;
    WEBKITDOM_STRING_INIT(&tmpInnerText);

    ewk_webkitdom_htmlelement_get_inner_text(pHtmlElement, &tmpInnerText);
    char* innerText = ewk_webkitdom_string_get_cstring(&tmpInnerText);
    EXPECT_EQ(0, strcmp(innerText,"first occurrence"));

    // Free and deinitialize
    free(innerText);
    WEBKITDOM_DEINIT(&tmpInnerText);
    WEBKITDOM_DEINIT(&element);
    WEBKITDOM_DEINIT(&document);

    END_TEST();
}

TEST(EwkDomBindingsTests, EwkDomBindingsGetElementsByIdTest)
{
    char* filepath_str = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DomBindingsTestDocument);
    ASSERT_EQ(true, filepath_str != NULL);
    RUN_TEST(filepath_str, onEwkDomBindingsGetElementsByIdTest, "load,finished", NULL);
    free(filepath_str);
}

static const char* names[] = { "blank", "google", "webkit" };
static const char* uris[] = { "about:blank", "http://www.google.com/", "http://www.webkit.org/" };

static void onEwkDomBindingsGetLinksTest(void* data, Evas_Object* webView, void* event_info)
{
    WebKitDOM_Document document;
    WEBKITDOM_DOCUMENT_INIT(&document);

    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE);
    if (ret == EINA_FALSE || WEBKITDOM_ISNULL(&document))
        END_TEST();

    WebKitDOM_HTMLCollection links;
    WEBKITDOM_HTMLCOLLECTION_INIT(&links);

    // Get links from document.
    ewk_webkitdom_document_get_links(&document, &links);
    EXPECT_EQ(false, WEBKITDOM_ISNULL(&links));
    if (WEBKITDOM_ISNULL(&links))
        END_TEST();

    // We expect to find 3 links in TestDocument.html
    unsigned length = ewk_webkitdom_htmlcollection_get_length(&links);
    EXPECT_EQ(3, length);

    // Iterate through collection to check it's elements.
    for (int i = 0; i < length; ++i) {
        WebKitDOM_Node item;
        WEBKITDOM_NODE_INIT(&item);

        // Get collection item with index i.
        ewk_webkitdom_htmlcollection_item(&links, i, &item);
        EXPECT_EQ(false, WEBKITDOM_ISNULL(&item));
        if (WEBKITDOM_ISNULL(&item))
            continue;

        WebKitDOM_Element* pElement = WEBKITDOM_CAST(WebKitDOM_Element, &item);

        // Check tag name.
        WebKitDOM_String tmpTagName;
        WEBKITDOM_STRING_INIT(&tmpTagName);

        ewk_webkitdom_element_get_tag_name(pElement, &tmpTagName);
        char* tagName = ewk_webkitdom_string_get_cstring(&tmpTagName);
        EXPECT_EQ(0, strcmp(tagName, "A"));

        WebKitDOM_HTMLElement* pHtmlElement = WEBKITDOM_CAST(WebKitDOM_HTMLElement, &item);

        // Check inner text.
        WebKitDOM_String tmpInnerText;
        WEBKITDOM_STRING_INIT(&tmpInnerText);

        ewk_webkitdom_htmlelement_get_inner_text(pHtmlElement, &tmpInnerText);
        char* innerText = ewk_webkitdom_string_get_cstring(&tmpInnerText);
        EXPECT_EQ(0, strcmp(innerText, names[i]));

        // Check href value.
        WebKitDOM_HTMLAnchorElement* pAnchor = WEBKITDOM_CAST(WebKitDOM_HTMLAnchorElement, &item);

        WebKitDOM_String tmpHref;
        WEBKITDOM_STRING_INIT(&tmpHref);

        ewk_webkitdom_htmlanchorelement_get_href(pAnchor, &tmpHref);
        char* href  = ewk_webkitdom_string_get_cstring(&tmpHref);
        EXPECT_EQ(0, strcmp(href, uris[i]));

        // Free and deinitialize
        free(tagName);
        WEBKITDOM_DEINIT(&tmpTagName);
        free(innerText);
        WEBKITDOM_DEINIT(&tmpInnerText);
        free(href);
        WEBKITDOM_DEINIT(&tmpHref);
        WEBKITDOM_DEINIT(&item);
    }

    WEBKITDOM_DEINIT(&document);

    END_TEST();
}

TEST(EwkDomBindingsTests, EwkDomBindingsGetLinksTest)
{
    char* filepath_str = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DomBindingsTestDocument);
    ASSERT_EQ(true, filepath_str != NULL);
    RUN_TEST(filepath_str, onEwkDomBindingsGetLinksTest, "load,finished", NULL);
    free(filepath_str);
}

static void onEwkDomNodeTest(void* data, Evas_Object* webView, void* event_info)
{
    WebKitDOM_Document document;
    WEBKITDOM_DOCUMENT_INIT(&document);

    Eina_Bool ret = ewk_view_dom_document_get(webView, &document);
    EXPECT_EQ(true, ret == EINA_TRUE);
    if (ret == EINA_FALSE || WEBKITDOM_ISNULL(&document))
        END_TEST();

    WebKitDOM_HTMLHeadElement head;
    WEBKITDOM_HTMLHEADELEMENT_INIT(&head);

    // Get get head element from document.
    ewk_webkitdom_document_get_head(&document, &head);
    EXPECT_EQ(false, WEBKITDOM_ISNULL(&head));

    // Check if head element has child nodes.
    WebKitDOM_Node* pNode = WEBKITDOM_CAST(WebKitDOM_Node, &head);
    EXPECT_EQ(true, ewk_webkitdom_node_has_child_nodes(pNode));

    // We expect it to have one child node, since head node in TestDocument.html have title child node
    WebKitDOM_NodeList list;
    WEBKITDOM_NODELIST_INIT(&list);
    ewk_webkitdom_node_get_child_nodes(pNode, &list);
    EXPECT_EQ(1, ewk_webkitdom_nodelist_get_length(&list));

    // We are getting title node
    WebKitDOM_Node item;
    WEBKITDOM_NODE_INIT(&item);
    ewk_webkitdom_nodelist_item(&list, 0, &item);
    EXPECT_EQ(false, WEBKITDOM_ISNULL(&item));

    // Check if title have sibling - we don't expect any.
    WebKitDOM_Node sibling;
    WEBKITDOM_NODE_INIT(&sibling);
    ewk_webkitdom_node_get_next_sibling(&item, &sibling);
    EXPECT_EQ(true, WEBKITDOM_ISNULL(&sibling));

    // Check if head have previous sibling - we don't expect any.
    ewk_webkitdom_node_get_previous_sibling(pNode, &sibling);
    EXPECT_EQ(true, WEBKITDOM_ISNULL(&sibling));

    // but we are expecting it to have next sibling - body.
    ewk_webkitdom_node_get_next_sibling(pNode, &sibling);
    EXPECT_EQ(false, WEBKITDOM_ISNULL(&sibling));

    // In TestDocument.html body should have 14 child nodes.
    ewk_webkitdom_node_get_child_nodes(&sibling, &list);
    unsigned length = ewk_webkitdom_nodelist_get_length(&list);
    EXPECT_EQ(14, length);

    WebKitDOM_Node tmpItem;
    WEBKITDOM_NODE_INIT(&tmpItem);
    ewk_webkitdom_nodelist_item(&list, 0, &tmpItem);

    ewk_webkitdom_node_get_next_sibling(&tmpItem, &item);

    // Iterate through body child nodes.
    int i = 0;
    do {
        ewk_webkitdom_nodelist_item(&list, i, &item);
        ++i;
    } while (!WEBKITDOM_ISNULL(&item));

    EXPECT_EQ(15, i);

    ewk_webkitdom_nodelist_item(&list, 14, &tmpItem);

    // Iterate through body child nodes using previous sibling.
    while (!WEBKITDOM_ISNULL(&tmpItem)) {
        ewk_webkitdom_node_get_previous_sibling(&tmpItem, &sibling);
        _copy_webkit_node(&sibling, &tmpItem);

        EXPECT_EQ(false, WEBKITDOM_ISNULL(&tmpItem));
    }

    // Deinitialize
    WEBKITDOM_DEINIT(&sibling);
    WEBKITDOM_DEINIT(&item);
    WEBKITDOM_DEINIT(&list);
    WEBKITDOM_DEINIT(&head);
    WEBKITDOM_DEINIT(&document);

    END_TEST();
}

TEST(EwkDomBindingsTests, EwkDomNodeTest)
{
    char* filepath_str = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DomBindingsTestDocument);
    ASSERT_EQ(true, filepath_str != NULL);
    RUN_TEST(filepath_str, onEwkDomNodeTest, "load,finished", NULL);
    free(filepath_str);
}
