/**
 * @MOBILE_ONLY
 *
 * @brief Add a new panes widget to the given parent Elementary
 * (container) object.
 *
 * @if MOBILE @since_tizen 2.3
 * @endif
 *
 * @param[in] parent The parent object.
 * @return a new panes widget handle or @c NULL, on errors.
 *
 * @remark This function inserts a new panes widget on the canvas.
 *
 * @ingroup Panes
 */
EAPI Evas_Object                 *elm_panes_add(Evas_Object *parent);

#include "elm_panes.eo.legacy.h"
