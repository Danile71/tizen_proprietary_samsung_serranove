/**
 * @defgroup Bg Background
 * @ingroup elm_widget_group
 *
 * @image html bg_inheritance_tree.png
 * @image latex bg_inheritance_tree.eps
 *
 *
 * @brief Background object, used for setting a solid color, image or
 * Edje group as a background to a window or any container object.
 *
 * The bg (background) widget is used for setting (solid) background
 * decorations to a window (unless it has transparency enabled) or to
 * any container object. It works just like an image, but has some
 * properties useful to a background, like setting it to tiled,
 * centered, scaled or stretched.
 *
 * This widget inherits from the @ref Layout one, so that all the
 * functions acting on it also work for background objects.
 *
 * Default content parts of the bg widget that you can use for are:
 * @li @c "overlay" - overlay of the bg
 *
 */

#include "elm_bg_common.h"
#ifdef EFL_EO_API_SUPPORT
#include "elm_bg_eo.h"
#endif
#ifndef EFL_NOLEGACY_API_SUPPORT
#include "elm_bg_legacy.h"
#endif
/**
 * @}
 */
