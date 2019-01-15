/**
 * @MOBILE_ONLY
 *
 * @brief Add a new map widget to the given parent Elementary (container) object.
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] parent The parent object.
 * @return a new map widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new map widget on the canvas.
 *
 * @ingroup Map
 */
EAPI Evas_Object          *elm_map_add(Evas_Object *parent);

#include "elm_map.eo.legacy.h"
