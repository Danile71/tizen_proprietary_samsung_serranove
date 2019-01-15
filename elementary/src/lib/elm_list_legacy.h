/**
 * @brief Add a new list widget to the given parent Elementary
 * (container) object.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object.
 * @return a new list widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new list widget on the canvas.
 *
 * @ingroup List
 */
EAPI Evas_Object                 *elm_list_add(Evas_Object *parent);

#include "elm_list_item.eo.legacy.h"
#include "elm_list.eo.legacy.h"
