/**
 * @brief Adds a new Popup to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Popup
 */
EAPI Evas_Object *elm_popup_add(Evas_Object *parent) EINA_ARG_NONNULL(1);

#include "elm_popup.eo.legacy.h"