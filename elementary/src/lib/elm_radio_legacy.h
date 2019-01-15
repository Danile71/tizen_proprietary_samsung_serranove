/**
 * @brief Add a new radio to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Radio
 */
EAPI Evas_Object                 *elm_radio_add(Evas_Object *parent);

#include "elm_radio.eo.legacy.h"