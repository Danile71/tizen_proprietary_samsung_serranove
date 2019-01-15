/**
 * @brief Add a new Ctxpopup object to the parent.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent Parent object
 * @return New object or @c NULL, if it cannot be created
 *
 * @ingroup Ctxpopup
 */
EAPI Evas_Object                 *elm_ctxpopup_add(Evas_Object *parent);

/**
 * @deprecated Deprecated since 2.4
 * @brief Get the possibility that the direction would be available
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Ctxpopup
 *
 * @param[in] obj The ctxpopup object
 * @param[in] direction A direction user wants to check
 *
 * Use this function to check whether input direction is proper for ctxpopup.
 * If ctxpopup cannot be at the direction since there is no sufficient area it can be,
 *
 * @return @c EINA_FALSE if you cannot put it in the direction.
 *         @c EINA_TRUE if it's possible.
 */
EAPI Eina_Bool
elm_ctxpopup_direction_available_get(Evas_Object *obj, Elm_Ctxpopup_Direction direction);

#include "elm_ctxpopup_item.eo.legacy.h"
#include "elm_ctxpopup.eo.legacy.h"
