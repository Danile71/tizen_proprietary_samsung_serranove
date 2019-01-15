/**
 * @MOBILE_ONLY
 *
 * @brief Add a new calendar widget to the given parent Elementary
 * (container) object.
 *
 * @param parent The parent object.
 * @return a new calendar widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new calendar widget on the canvas.
 *
 * @if MOBILE @since_tizen 2.4
 * @endif
 *
 * @ingroup Calendar
 */
EAPI Evas_Object         *elm_calendar_add(Evas_Object *parent);

#include "elm_calendar.eo.legacy.h"
