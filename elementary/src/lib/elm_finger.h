/**
 * @defgroup Fingers Fingers
 * @ingroup elm_infra_group
 *
 * Elementary is designed to be finger-friendly for touchscreens,
 * and so in addition to scaling for display resolution, it can
 * also scale based on finger "resolution" (or size). You can then
 * customize the granularity of the areas meant to receive clicks
 * on touchscreens.
 *
 * Different profiles may have pre-set values for finger sizes.
 *
 *
 * @{
 */

/**
 * @brief Adjust size of an element for finger usage.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] times_w How many fingers should fit horizontally
 * @param[out] w Pointer to the width size to adjust
 * @param[in] times_h How many fingers should fit vertically
 * @param[out] h Pointer to the height size to adjust
 *
 * @remark This takes width and height sizes (in pixels) as input and a
 * size multiple (which is how many fingers you want to place
 * within the area, being "finger" the size set by
 * elm_config_finger_size_set()), and adjusts the size to be large enough
 * to accommodate the resulting size -- if it doesn't already
 * accommodate it. On return the @p w and @p h sizes pointed to by
 * these parameters will be modified, on those conditions.
 *
 * @remark This is kind of low level Elementary call, most useful
 * on size evaluation times for widgets. An external user wouldn't
 * be calling, most of the time.
 *
 * @ingroup Fingers
 */
EAPI void       elm_coords_finger_size_adjust(int times_w, Evas_Coord *w, int times_h, Evas_Coord *h);

/**
 * @}
 */
