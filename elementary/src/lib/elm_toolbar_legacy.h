/**
 * @MOBILE_ONLY
 *
 * @brief Add a new toolbar widget to the given parent Elementary
 * (container) object.
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] parent The parent object.
 * @return a new toolbar widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new toolbar widget on the canvas.
 *
 * @ingroup Toolbar
 */
EAPI Evas_Object                 *elm_toolbar_add(Evas_Object *parent);

/**
 * @MOBILE_ONLY
 *
 * @brief Set reorder mode
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] obj The toolbar object
 * @param[in] reorder_mode The reorder mode
 * (@c EINA_TRUE = on, @c EINA_FALSE = off)
 *
 * @ingroup Toolbar
 */
EAPI void                          elm_toolbar_reorder_mode_set(Evas_Object *obj, Eina_Bool reorder_mode);

/**
 * @MOBILE_ONLY
 *
 * @brief Get the reorder mode
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] obj The toolbar object
 * @return The reorder mode
 * (@c EINA_TRUE = on, @c EINA_FALSE = off)
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool                     elm_toolbar_reorder_mode_get(const Evas_Object *obj);

/**
 * @MOBILE_ONLY
 *
 * @brief Set the item's transverse expansion of a given toolbar widget @p obj.
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] obj The toolbar object.
 * @param[in] transverse_expanded The transverse expansion of the item.
 * (@c EINA_TRUE = on, @c EINA_FALSE = off, default = @c EINA_FALSE)
 *
 * @remark This will expand the transverse length of the item according the transverse length of the toolbar.
 * The default is what the transverse length of the item is set according its min value.
 *
 * @ingroup Toolbar
 */
EAPI void                         elm_toolbar_transverse_expanded_set(Evas_Object *obj, Eina_Bool transverse_expanded);

/**
 * @MOBILE_ONLY
 *
 * @brief Get the transverse expansion of toolbar @p obj.
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] obj The toolbar object.
 * @return The transverse expansion of the item.
 * (@c EINA_TRUE = on, @c EINA_FALSE = off, default = @c EINA_FALSE)
 *
 * @see elm_toolbar_transverse_expand_set() for details.
 *
 * @ingroup Toolbar
 */
EAPI Eina_Bool                    elm_toolbar_transverse_expanded_get(const Evas_Object *obj);

#include "elm_toolbar_item.eo.legacy.h"
#include "elm_toolbar.eo.legacy.h"
