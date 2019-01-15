/**
 * @defgroup Mirroring Mirroring
 * @ingroup elm_infra_group
 *
 * These functions allow you to set ui-mirroring on specific
 * widgets or the whole interface. Widgets can be in one of two
 * modes, automatic and manual.  Automatic means they'll be changed
 * according to the system mirroring mode and manual means only
 * explicit changes will matter. You are not supposed to change
 * mirroring state of a widget set to automatic, will mostly work,
 * but the behavior is not really defined.
 *
 * @{
 */

/**
 * @brief Get the widget's mirrored mode.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The widget.
 * @return @c EINA_TRUE if mirrored is set, @c EINA_FALSE otherwise
 *
 * @ingroup Mirroring
 */
EAPI Eina_Bool elm_object_mirrored_get(const Evas_Object *obj);

/**
 * @brief Set the widget's mirrored mode.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The widget.
 * @param[in] mirrored @c EINA_TRUE to set mirrored mode, @c EINA_FALSE to unset it.
 *
 * @ingroup Mirroring
 */
EAPI void      elm_object_mirrored_set(Evas_Object *obj, Eina_Bool mirrored);

/**
 * @brief Returns the widget's mirrored mode setting.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The widget.
 * @return mirrored mode setting of the object.
 *
 * @ingroup Mirroring
 */
EAPI Eina_Bool elm_object_mirrored_automatic_get(const Evas_Object *obj);

/**
 * @brief Sets the widget's mirrored mode setting.
 * When widget in automatic mode, it follows the system mirrored mode set by
 * elm_mirrored_set().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The widget.
 * @param[in] automatic @c EINA_TRUE for auto mirrored mode. @c EINA_FALSE for
 * manual.
 *
 * @ingroup Mirroring
 */
EAPI void      elm_object_mirrored_automatic_set(Evas_Object *obj, Eina_Bool automatic);

/**
 * @}
 */
