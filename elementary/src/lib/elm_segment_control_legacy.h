/**
 * @MOBILE_ONLY
 *
 * @brief Add a new segment control widget to the given parent Elementary
 * (container) object.
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] parent The parent object.
 * @return a new segment control widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new segment control widget on the canvas.
 *
 * @ingroup SegmentControl
 */
EAPI Evas_Object      *elm_segment_control_add(Evas_Object *parent);

#include "elm_segment_control_item.eo.legacy.h"
#include "elm_segment_control.eo.legacy.h"
