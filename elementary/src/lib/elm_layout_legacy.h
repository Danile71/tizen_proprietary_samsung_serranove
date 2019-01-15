/**
 * @brief Add a new layout to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @see elm_layout_file_set()
 * @see elm_layout_theme_set()
 *
 * @ingroup Layout
 */
EAPI Evas_Object                 *elm_layout_add(Evas_Object *parent);

/**
 * @internal
 *
 * Get the list of swallow parts of a given container widget
 *
 * @param obj The layout object
 *
 * @return list of swallow parts which should be freed by the user program with elm_list_free()
 *
 * @since 1.9
 *
 * @ingroup Layout
 */
EAPI Eina_List                   *elm_layout_content_swallow_list_get(const Evas_Object *obj);

/**
 * @brief Set the layout content.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The layout object
 * @param[in] swallow The swallow part name in the edje file
 * @param[in] content The child that will be added in this layout object
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 *
 * @remark Once the content object is set, a previously set one will be deleted.
 * If you want to keep that old content object, use the
 * elm_layout_content_unset() function.
 *
 * @remark In an Edje theme, the part used as a content container is called
 * @c SWALLOW. This is why the parameter name is called @p swallow, but it is
 * expected to be a part name just like the second parameter of
 * elm_layout_box_append().
 *
 * @see elm_layout_box_append()
 * @see elm_layout_content_get()
 * @see elm_layout_content_unset()
 *
 * @see @ref secBox
 *
 * @ingroup Layout
 */
EAPI Eina_Bool                    elm_layout_content_set(Evas_Object *obj, const char *swallow, Evas_Object *content);

/**
 * @brief Get the child object in the given content part.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The layout object
 * @param[in] swallow The SWALLOW part to get its content
 *
 * @return The swallowed object or NULL if none or an error occurred
 *
 * @ingroup Layout
 */
EAPI Evas_Object                 *elm_layout_content_get(const Evas_Object *obj, const char *swallow);

/**
 * @brief Unset the layout content.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The layout object
 * @param[in] swallow The swallow part name in the edje file
 * @return The content that was being used
 *
 * @remark Unparent and return the content object which was set for this part.
 *
 * @ingroup Layout
 */
EAPI Evas_Object                 *elm_layout_content_unset(Evas_Object *obj, const char *swallow);

/**
 * @brief Set the file that will be used as layout
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return (1 = success, 0 = error)
 *
 * @ingroup Layout
 *
 * @param[in] file The path to file (edj) that will be used as layout
 * @param[in] group The group that the layout belongs in edje file
 */
EAPI Eina_Bool elm_layout_file_set(Eo *obj, const char *file, const char *group);

#include "elm_layout.eo.legacy.h"
