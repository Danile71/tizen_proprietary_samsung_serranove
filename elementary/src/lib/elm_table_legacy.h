/**
 * @brief Add a new table to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Table
 */
EAPI Evas_Object *elm_table_add(Evas_Object *parent);

/**
 * @brief Set the packing location of an existing child of the table
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] subobj The subobject to be modified in the table
 * @param[in] col Column number
 * @param[in] row Row number
 * @param[in] colspan colspan
 * @param[in] rowspan rowspan
 *
 * @remark Modifies the position of an object already in the table.
 *
 * @remark All positioning inside the table is relative to rows and columns, so
 * a value of 0 for col and row, means the top left cell of the table, and a
 * value of 1 for colspan and rowspan means @p subobj only takes that 1 cell.
 *
 * @ingroup Table
 */
EAPI void      elm_table_pack_set(Evas_Object *subobj, int col, int row, int colspan, int rowspan);

/**
 * @brief Get the packing location of an existing child of the table
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] subobj The subobject to be modified in the table
 * @param[out] col Column number
 * @param[out] row Row number
 * @param[out] colspan colspan
 * @param[out] rowspan rowspan
 *
 * @see elm_table_pack_set()
 *
 * @ingroup Table
 */
EAPI void      elm_table_pack_get(Evas_Object *subobj, int *col, int *row, int *colspan, int *rowspan);

#include "elm_table.eo.legacy.h"