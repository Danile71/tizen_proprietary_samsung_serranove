/**
 * @brief Add a new grid to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Grid
 */
EAPI Evas_Object *elm_grid_add(Evas_Object *parent);

/**
 * @brief Set packing of an existing child at to position and size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] subobj The child to set packing of
 * @param[in] x The virtual x coord at which to pack it
 * @param[in] y The virtual y coord at which to pack it
 * @param[in] w The virtual width at which to pack it
 * @param[in] h The virtual height at which to pack it
 *
 * @ingroup Grid
 */
EAPI void         elm_grid_pack_set(Evas_Object *subobj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);

/**
 * @brief Get packing of a child
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] subobj The child to query
 * @param[out] x Pointer to integer to store the virtual x coord
 * @param[out] y Pointer to integer to store the virtual y coord
 * @param[out] w Pointer to integer to store the virtual width
 * @param[out] h Pointer to integer to store the virtual height
 *
 * @ingroup Grid
 */
EAPI void         elm_grid_pack_get(Evas_Object *subobj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);

#include "elm_grid.eo.legacy.h"