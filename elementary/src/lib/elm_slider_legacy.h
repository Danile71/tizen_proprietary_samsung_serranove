/**
 * @brief Add a new slider widget to the given parent Elementary
 * (container) object.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object.
 * @return a new slider widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new slider widget on the canvas.
 *
 * @ingroup Slider
 */
EAPI Evas_Object                 *elm_slider_add(Evas_Object *parent);

#include "elm_slider.eo.legacy.h"