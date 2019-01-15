/**
 * @brief Add a new progress bar widget to the given parent Elementary
 * (container) object
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return a new progress bar widget handle or @c NULL, on errors
 *
 * @remark This function inserts a new progress bar widget on the canvas.
 *
 * @ingroup Progressbar
 */
EAPI Evas_Object                 *elm_progressbar_add(Evas_Object *parent);

#include "elm_progressbar.eo.legacy.h"