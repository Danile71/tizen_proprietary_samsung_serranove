/**
 * @brief Add a new mapbuf widget to the given parent Elementary
 * (container) object.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object.
 * @return A new mapbuf widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new mapbuf widget on the canvas.
 *
 * @ingroup Mapbuf
 */
EAPI Evas_Object                 *elm_mapbuf_add(Evas_Object *parent);

#include "elm_mapbuf.eo.legacy.h"