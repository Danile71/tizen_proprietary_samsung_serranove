/*
   Copyright (C) 2011 Samsung Electronics

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

#ifndef ewk_hit_test_product_h
#define ewk_hit_test_product_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Enum values with flags representing the hit test mode.
 * @since_tizen 2.3
 */
typedef enum {
    EWK_HIT_TEST_MODE_DEFAULT = 1 << 1, /**< Link data. */
    EWK_HIT_TEST_MODE_NODE_DATA = 1 << 2,   /**< @internal Extra node data(tag name, node value, attribute infomation, etc). */
    EWK_HIT_TEST_MODE_IMAGE_DATA = 1 << 3,  /**< @internal Extra image data(image data, image data length, image file name exteionsion, etc). */
    EWK_HIT_TEST_MODE_ALL = EWK_HIT_TEST_MODE_DEFAULT | EWK_HIT_TEST_MODE_NODE_DATA | EWK_HIT_TEST_MODE_IMAGE_DATA  /**< @internal All data. */
} Ewk_Hit_Test_Mode;

/**
 * @brief Enum values with flags representing the context of a #Ewk_Hit_Test.
 * @since_tizen 2.3
 */
typedef enum {
    EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT = 1 << 1,  /**< Anywhere in the document. */
    EWK_HIT_TEST_RESULT_CONTEXT_LINK = 1 << 2,  /**< A hyperlink element. */
    EWK_HIT_TEST_RESULT_CONTEXT_IMAGE = 1 << 3, /**< An image element. */
    EWK_HIT_TEST_RESULT_CONTEXT_MEDIA = 1 << 4, /**< A video or audio element. */
    EWK_HIT_TEST_RESULT_CONTEXT_SELECTION = 1 << 5, /**< The area is selected by the user. */
    EWK_HIT_TEST_RESULT_CONTEXT_EDITABLE = 1 << 6,  /**< The area is editable by the user. */
    EWK_HIT_TEST_RESULT_CONTEXT_TEXT = 1 << 7   /**< The area is text. */
} Ewk_Hit_Test_Result_Context;

/**
 * @brief Creates a type name for _Ewk_Hit_Test
 * @since_tizen 2.3
 */
typedef struct _Ewk_Hit_Test Ewk_Hit_Test;

/**
 * @brief Frees hit test instance created by ewk_view_hit_test_new().
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @see ewk_view_hit_test_new
 */
EXPORT_API void ewk_hit_test_free(Ewk_Hit_Test* hit_test);

/**
 * @brief Gets the context of  the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return a bitmask of the hit test context on success, @c #EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT otherwise
 */
EXPORT_API Ewk_Hit_Test_Result_Context ewk_hit_test_result_context_get(Ewk_Hit_Test* hit_test);

/**
 * @brief Gets the link uri string of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the URI of the link element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_link_uri_get(Ewk_Hit_Test* hit_test);

/**
 * @brief Gets the link title string of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the title of the link element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_link_title_get(Ewk_Hit_Test* hit_test);

/**
 * @brief Gets the link label string of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the label of the link element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_link_label_get(Ewk_Hit_Test* hit_test);

/**
 * @brief Gets the image uri string of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the URI of the image element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_image_uri_get(Ewk_Hit_Test* hit_test);

/**
 * @brief Gets the media uri string of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the URI of the media element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_media_uri_get(Ewk_Hit_Test* hit_test);

/**
 * @internal
 * @brief Gets the tag name string of hit element of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the tag name of the hit element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_tag_name_get(Ewk_Hit_Test* hit_test);

/**
 * @internal
 * @brief Gets the node value string of hit element of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the node value of the hit element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_node_value_get(Ewk_Hit_Test* hit_test);

/**
 * @internal
 * @brief Gets the attribute data of hit element of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the attribute data of the hit element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API Eina_Hash* ewk_hit_test_attribute_hash_get(Ewk_Hit_Test* hit_test);

/**
 * @internal
 * @brief Gets the image buffer of hit element of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the image buffer of the hit element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API void* ewk_hit_test_image_buffer_get(Ewk_Hit_Test* hit_test);

/**
 * @internal
 * @brief Gets the image buffer length of hit element of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the image buffer length of the hit element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API unsigned int ewk_hit_test_image_buffer_length_get(Ewk_Hit_Test* hit_test);

/**
 * @internal
 * @brief Gets the image fiile name extension string of hit element of the hit test
 *
 * @since_tizen 2.3
 *
 * @param[in] hit_test instance
 *
 * @return the image fiile name extension of the hit element in the coordinates of the hit test on success, @c 0 otherwise
 */
EXPORT_API const char* ewk_hit_test_image_file_name_extension_get(Ewk_Hit_Test* hit_test);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_hit_test_product_h
