/**
 * @brief This adds an entry to @p parent object.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark By default, entries are:
 * @li not scrolled
 * @li multi-line
 * @li word wrapped
 * @li autosave is enabled
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Entry
 */
EAPI Evas_Object       *elm_entry_add(Evas_Object *parent);

/**
 * @brief This sets the text displayed within the entry to @p entry.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The entry object
 * @param[in] entry The text to be displayed
 *
 * @remark Using this function bypasses text filters
 *
 * @ingroup Entry
 */
EAPI void               elm_entry_entry_set(Evas_Object *obj, const char *entry);

/**
 * @brief This returns the text currently shown in object @p entry.
 * See also elm_entry_entry_set().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The entry object
 * @return The currently displayed text or NULL on failure
 *
 * @ingroup Entry
 */
EAPI const char        *elm_entry_entry_get(const Evas_Object *obj);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Disables the entry's magnifier feature.
 *
 * @param obj The entry object
 * @param disabled If @c true the magnifier is not displayed,
 *                 otherwise @c false
 */
EAPI void                        elm_entry_magnifier_disabled_set(Evas_Object *obj, Eina_Bool disabled);
/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Returns whether the entry's magnifier feature is disabled.
 *
 * @param obj The entry object
 * @return @c true if the feature is disabled,
 *         otherwise @c false
 */
EAPI Eina_Bool                   elm_entry_magnifier_disabled_get(const Evas_Object *obj);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Disables the entry's cursor handler.
 *
 * @param obj The entry object
 * @param disabled If @c true the cursor handler is disabled,
 *                 otherwise @c false to enable it
 */
EAPI void               elm_entry_cursor_handler_disabled_set(Evas_Object *obj, Eina_Bool disabled);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Returns whether the entry's cursor handler is disabled.
 *
 * @param obj The entry object
 * @return @c true if the cursor handler is disabled,
 *         otherwise @c false
 */
EAPI Eina_Bool          elm_entry_cursor_handler_disabled_get(const Evas_Object *obj);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Enables selection in the entry.
 *
 * @param obj The entry object
 * @param allow If @c true selection is enabled,
 *              otherwise @c false if selection is disabled
 */
EAPI void               elm_entry_select_allow_set(Evas_Object *obj, Eina_Bool allow);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Returns whether selection in the entry is allowed.
 *
 * @param obj The entry object
 * @return @c true if selection is enabled,
 *         otherwise @c false if selection is disabled
 */
EAPI Eina_Bool               elm_entry_select_allow_get(const Evas_Object *obj);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Disables the default drag action in the entry.
 *
 * @param obj The entry object
 * @param disabled If @c true, disable the default drag action in the entry,
 *                 otherwise @c false
 */
EAPI void                        elm_entry_drag_disabled_set(Evas_Object *obj, Eina_Bool disabled);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Gets whether the default drag action in the entry is disabled.
 *
 * @param obj The entry object
 * @return @c true if the default drag action in the entry is disabled,
 *         otherwise @c false
 */
EAPI Eina_Bool                   elm_entry_drag_disabled_get(const Evas_Object *obj);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Disables the default drop callback in the entry.
 *
 * @param obj The entry object
 * @param disabled If @c true the default drop callback in the entry is disabled,
 *                 otherwise @c false
 */
EAPI void                        elm_entry_drop_disabled_set(Evas_Object *obj, Eina_Bool disabled);

/**
 * @internal
 * @remarks Tizen Only Feature
 *
 * @brief Gets whether the default drop callback in the entry is disabled.
 *
 * @param obj The entry object
 * @return @c true if the default drop callback in the entry is disabled,
 *         otherwise @c false
 */
EAPI Eina_Bool                   elm_entry_drop_disabled_get(Evas_Object *obj);

/**
 * @internal
 *
 * @brief Sets the entry's scrollbar policy (i.e. enabling/disabling
 *        them).
 *
 * @remarks Setting an entry to the single-line mode with elm_entry_single_line_set()
 *          automatically disables the display of scrollbars when the entry
 *          moves inside its scroller.
 *
 * @param obj The entry object
 * @param h The horizontal scrollbar policy to apply
 * @param v The vertical scrollbar policy to apply
 *
 * @deprecated Use elm_scroller_policy_set() instead.
 */
EAPI void elm_entry_scrollbar_policy_set(Evas_Object *obj, Elm_Scroller_Policy h, Elm_Scroller_Policy v);

/**
 * @internal
 *
 * @brief Enables or disables bouncing within the entry.
 *
 * @remarks This function sets whether the entry bounces when scrolling reaches
 *          the end of the contained entry.
 *
 * @param obj The entry object
 * @param h_bounce The horizontal bounce state
 * @param v_bounce The vertical bounce state
 *
 * @deprecated Use elm_scroller_bounce_set() instead.
 */
EAPI void elm_entry_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce);

/**
 * @internal
 *
 * @brief Gets the bounce mode.
 *
 * @param obj The entry object
 * @param h_bounce The boolean value that indicates whether horizontal bounce is allowed
 * @param v_bounce The boolean value that indicates whether vertical bounce is allowed
 *
 * @deprecated Use elm_scroller_bounce_get() instead.
 */
EAPI void elm_entry_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce);

#include "elm_entry.eo.legacy.h"
