/**
 * @typedef Elm_Object_Item
 * An Elementary Object item handle.
 * @ingroup General
 */
typedef Eo Elm_Object_Item;

/**
 * @typedef Elm_Object_Item_Signal_Cb
 *
 * Elm_Object_Item Signal Callback functions' prototype definition. @c data
 * will have the auxiliary data pointer at the time the callback registration.
 * @c it will be a pointer the Elm_Object_Item that have the edje object where
 * the signal comes from. @c emission will identify the exact signal's emission
 * string and @c source the exact signal's source one.
 *
 * @see elm_object_item_signal_callback_add()
 * @since 1.8
 *
 * @ingroup General
 */
typedef void                  (*Elm_Object_Item_Signal_Cb)(void *data, Elm_Object_Item *it, const char *emission, const char *source);

#include "elm_widget_item.eo.h"
#include "elm_widget_item.eo.legacy.h"

/**
 * @def  elm_object_item_content_set
 * @brief Convenient macro for  elm_object_item_part_content_set
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_part_content_set
 * @ingroup General
 */
#define elm_object_item_content_set(it, content) elm_object_item_part_content_set((it), NULL, (content))

/**
 * @def  elm_object_item_content_get
 * @brief Convenient macro for  elm_object_item_part_content_get
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_part_content_get
 * @ingroup General
 */
#define elm_object_item_content_get(it) elm_object_item_part_content_get((it), NULL)

/**
 * @def  elm_object_item_content_unset
 * @brief Convenient macro for  elm_object_item_part_content_unset
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_part_content_unset
 * @ingroup General
 */
#define elm_object_item_content_unset(it) elm_object_item_part_content_unset((it), NULL)

/**
 * @brief Sets the macro label of an object item.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remarks Elementary object items may have many labels.
 *
 * @param[in] it The Elementary object item
 * @param[in] label The new text of the label
 *
 * @ingroup General
 */
#define elm_object_item_text_set(it, label) elm_object_item_part_text_set((it), NULL, (label))

/**
 * @def  elm_object_item_text_get
 * @brief Convenient macro for  elm_object_item_part_text_get
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_part_text_get
 * @ingroup General
 */
#define elm_object_item_text_get(it) elm_object_item_part_text_get((it), NULL)

/**
 * @def  elm_object_item_domain_translatable_text_set
 * @brief Convenient macro for  elm_object_item_domain_translatable_part_text_set
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_domain_translatable_part_text_set
 * @ingroup General
 */
#define elm_object_item_domain_translatable_text_set(it, domain, text) elm_object_item_domain_translatable_part_text_set((it), NULL, (domain), (text))

/**
 * @def  elm_object_item_translatable_text_set
 * @brief Convenient macro for  elm_object_item_domain_translatable_part_text_set
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_domain_translatable_part_text_set
 * @ingroup General
 */
#define elm_object_item_translatable_text_set(it, text) elm_object_item_domain_translatable_part_text_set((it), NULL, NULL, (text))

/**
 * @def  elm_object_item_translatable_part_text_set
 * @brief Convenient macro for  elm_object_item_domain_translatable_part_text_set
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_domain_translatable_part_text_set
 * @ingroup General
 */
#define elm_object_item_translatable_part_text_set(it, part, text) elm_object_item_domain_translatable_part_text_set((it), (part), NULL, (text))

/**
 * @def  elm_object_item_translatable_text_get
 * @brief Convenient macro for  elm_object_item_translatable_part_text_get
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_translatable_part_text_get
 * @ingroup General
 */
#define elm_object_item_translatable_text_get(it) elm_object_item_translatable_part_text_get((it), NULL)

/**
 * @def  elm_object_item_part_text_translatable_set
 * @brief Convenient macro for  elm_object_item_domain_part_text_translatable_set
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_domain_part_text_translatable_set
 * @ingroup General
 */
#define elm_object_item_part_text_translatable_set(it, part, translatable) elm_object_item_domain_part_text_translatable_set((it), (part), NULL, (translatable))

/**
 * @def  elm_object_item_domain_text_translatable_set
 * @brief Convenient macro for  elm_object_item_domain_part_text_translatable_set
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @see elm_object_item_domain_part_text_translatable_set
 * @ingroup General
 */
#define elm_object_item_domain_text_translatable_set(it, domain, translatable) elm_object_item_domain_part_text_translatable_set((it), NULL, (domain), (translatable))

#define elm_object_item_text_translatable_set(it, translatable) elm_object_item_domain_part_text_translatable_set((it), NULL, NULL, (translatable))

/**
 * @brief Get the data associated with an object item
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] it The Elementary object item
 * @return The data associated with @p it
 *
 * @remark Every elm_object_item supports this API
 * @ingroup General
 */
EAPI void *elm_object_item_data_get(const Elm_Object_Item *it);

/**
 * @brief Set the data associated with an object item
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] it The Elementary object item
 * @param[in] data The data to be associated with @p it
 *
 * @remark Every elm_object_item supports this API
 * @ingroup General
 */
EAPI void elm_object_item_data_set(Elm_Object_Item *it, void *data);
