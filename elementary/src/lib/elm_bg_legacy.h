/**
 * @brief Add a new background to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or @c NULL if it cannot be created
 *
 * @ingroup Bg
 */
EAPI Evas_Object                 *elm_bg_add(Evas_Object *parent);

/**
 * @brief Set the color on a given background widget
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The background object handle
 * @param[in] r The red color component's value
 * @param[in] g The green color component's value
 * @param[in] b The blue color component's value
 *
 * @remark This sets the color used for the background rectangle, in RGB
 * format. Each color component's range is from 0 to 255.
 *
 * @remark You probably only want to use this function if you haven't
 * previously called elm_bg_file_set(), so that you just want a solid
 * color background.
 *
 * @remark You can reset the color by setting @p r, @p g, @p b as -1, -1, -1.
 *
 * @see elm_bg_color_get()
 *
 * @ingroup Bg
 */
EAPI void                         elm_bg_color_set(Evas_Object *obj, int r, int g, int b);

/**
 * @brief Get the color set on a given background widget
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The background object handle
 * @param[out] r Where to store the red color component's value
 * @param[out] g Where to store the green color component's value
 * @param[out] b Where to store the blue color component's value
 *
 * @remark Use @c NULL pointers on the file components you're not
 * interested in: they'll be ignored by the function.
 *
 * @see elm_bg_color_get() for more details
 *
 * @ingroup Bg
 */
EAPI void                         elm_bg_color_get(const Evas_Object *obj, int *r, int *g, int *b);

/**
 * @brief Set the file (image or edje collection) to give life for the
 * background
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 *
 * @remark This sets the image file used in the background object. If the
 * image comes from an Edje group, it will be stretched to completely
 * fill the background object. If it comes from a traditional image file, it
 * will by default be centered in this widget's area (thus retaining
 * its aspect), what could lead to some parts being not visible. You
 * may change the mode of exhibition for a real image file with
 * elm_bg_option_set().
 *
 * @remark Once the image of @p obj is set, a previously set one will be
 * deleted, even if @p file is @c NULL.
 *
 * @remark This will only affect the contents of one of the background's
 * swallow spots, namely @c "elm.swallow.background". If you want to
 * achieve the @c Layout's file setting behavior, you'll have to call
 * that method on this object.
 *
 * @ingroup Bg
 *
 * @param[in] file The file path
 * @param[in] group Optional key (group in Edje) within the file
 */
EAPI Eina_Bool elm_bg_file_set(Eo *obj, const char *file, const char *group);

/**
 * @brief
 * Get the file (image or edje collection) set on a given background
 * widget
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark Use @c NULL pointers on the file components you're not
 * interested in: they'll be ignored by the function.
 *
 * @ingroup Bg
 *
 * @param[out] file The file path
 * @param[out] group Optional key (group in Edje) within the file
 */
EAPI void elm_bg_file_get(const Eo *obj, const char **file, const char **group);

#include "elm_bg.eo.legacy.h"
