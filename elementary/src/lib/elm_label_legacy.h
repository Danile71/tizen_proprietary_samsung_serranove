/**
 * @brief Add a new label to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Label
 */
EAPI Evas_Object                *elm_label_add(Evas_Object *parent);

/**
 * @internal
 * @typedef Elm_Label_Item_Provider_Cb
 * @brief Called to provide items.
 *
 * @since_tizen 2.3
 *
 * @remarks If it returns an object handle other than @c NULL (it should create an
 *          object to do this), then this object is used to replace the current item.
 *          If not, the next provider is called until one provides an item object, or the
 *          default provider in the label does.
 * @param[in] data The data specified as the last parameter when adding the provider
 * @param[in] label The label object
 * @param[in] text A pointer to the item href string in the text
 * @return The object to be placed in the label like an icon, or another element
 * @see elm_label_item_provider_append
 * @see elm_label_item_provider_prepend
 * @see elm_label_item_provider_remove
 */
typedef Evas_Object * (*Elm_Label_Item_Provider_Cb)(void *data, Evas_Object * label, const char *item);

/**
 * @internal
 * @remarks Tizen only feature 2014.06.28
 *
 * @typedef Elm_Label_Anchor_Access_Provider_Cb
 * @brief This callback type is used to provide TTS string of an anchor.
 * @remarks If it returns a string other than NULL,
 *          then this string is used to replace the current anchor's TTS string.
 *          If not the next provider is called until one provides a string, or the
 *          default string will be read.
 *
 * @param data The data specified as the last param when adding the provider
 * @param label The label object
 * @param name A pointer to the anchor href string in the text
 * @param text A pointer to the text inside of the anchor's range.
 * @return TTS string for the anchor.
 *
 * @see elm_label_anchor_access_provider_append
 * @see elm_label_anchor_access_provider_prepend
 * @see elm_label_anchor_access_provider_remove
 */
typedef char * (*Elm_Label_Anchor_Access_Provider_Cb)(void *data, Evas_Object * label, const char *name, const char *text);
//

/**
 * @internal
 * @remarks Tizen only feature 2013.10.28: Support item, anchor formats
 *
 * @brief Appends a custom item provider to the list for that label.
 *
 * @details This appends the given callback. The list is walked from beginning to end
 *          with each function called, given the item href string in the text. If the
 *          function returns an object handle other than @c NULL (it should create an
 *          object to do this), then this object is used to replace that item. If
 *          not, the next provider is called until one provides an item object, or the
 *          default provider in the label does.
 *
 * @param obj The label object
 * @param func The function called to provide the item object
 * @param data The data passed to @a func
 *
 * @see @ref label-items
 */
EAPI void               elm_label_item_provider_append(Evas_Object *obj, Elm_Label_Item_Provider_Cb func, void *data);

/**
 * @internal
 * @remarks Tizen only feature 2013.10.28: Support item, anchor formats
 *
 * @brief Prepends a custom item provider to the list for that label.
 *
 * @details This prepends the given callback. See elm_label_item_provider_append() for
 *          more information.
 *
 * @param obj The label object
 * @param func The function called to provide the item object
 * @param data The data passed to @a func
 */
EAPI void               elm_label_item_provider_prepend(Evas_Object *obj, Elm_Label_Item_Provider_Cb func, void *data);

/**
 * @internal
 * @remarks Tizen only feature 2013.10.28: Support item, anchor formats
 *
 * @brief Removes a custom item provider to the list for that label.
 *
 * @details This removes the given callback. See elm_label_item_provider_append() for
 *          more information.
 *
 * @param obj The label object
 * @param func The function called to provide the item object
 * @param data The data passed to @a func
 */
EAPI void               elm_label_item_provider_remove(Evas_Object *obj, Elm_Label_Item_Provider_Cb func, void *data);

/**
 * @internal
 * @remarks Tizen only feature 2014.06.28
 *
 * @brief This appends a custom anchor access provider to the list for that label
 *
 * @remarks This appends the given callback. The list is walked from beginning to end
 *          with each function called given the anchor href string in the text. If the
 *          function returns a string other than NULL, then this string is used
 *          to replace that TTS string.
 *          If not the next provider is called until one provides a string, or the
 *          default TTS string will be read.
 *
 * @param obj The label object
 * @param func The function called to provide the TTS string
 * @param data The data passed to @p func
 *
 * @see @ref label-anchors
 */
EAPI void               elm_label_anchor_access_provider_append(Evas_Object *obj, Elm_Label_Anchor_Access_Provider_Cb func, void *data);

/**
 * @internal
 * @remarks Tizen only feature 2014.06.28
 *
 * @brief This prepends a custom anchor access provider to the list for that label
 *
 * @remarks This prepends the given callback.
 *
 * @param obj The label object
 * @param func The function called to provide the TTS string
 * @param data The data passed to @p func
 *
 * @see elm_label_anchor_access_provider_append().
 */
EAPI void               elm_label_anchor_access_provider_prepend(Evas_Object *obj, Elm_Label_Anchor_Access_Provider_Cb func, void *data);

/**
 * @internal
 * @remarks Tizen only feature 2014.06.28
 *
 * @brief This removes a custom anchor access provider to the list for that label
 *
 * @remarks This removes the given callback.
 *
 * @see elm_label_anchor_access_provider_append().
 *
 * @param obj The label object
 * @param func The function called to provide the TTS string
 * @param data The data passed to @p func
 */
EAPI void               elm_label_anchor_access_provider_remove(Evas_Object *obj, Elm_Label_Anchor_Access_Provider_Cb func, void *data);

#include "elm_label.eo.legacy.h"
