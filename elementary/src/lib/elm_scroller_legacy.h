/**
 * @brief Add a new scroller to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Scroller
 */
EAPI Evas_Object                  *elm_scroller_add(Evas_Object *parent);

/**
 * @brief Make the scroller minimum size limited to the minimum size of the content
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] w Enable limiting minimum size horizontally
 * @param[in] h Enable limiting minimum size vertically
 *
 * @remark By default the scroller will be as small as its design allows,
 * irrespective of its content. This will make the scroller minimum size the
 * right size horizontally and/or vertically to perfectly fit its content in
 * that direction.
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_content_min_limit(Evas_Object *obj, Eina_Bool w, Eina_Bool h);

/**
 * @brief Show a specific virtual region within the scroller content object
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] x X coordinate of the region
 * @param[in] y Y coordinate of the region
 * @param[in] w Width of the region
 * @param[in] h Height of the region
 *
 * @remark This will ensure all (or part if it does not fit) of the designated
 * region in the virtual content object (0, 0 starting at the top-left of the
 * virtual content object) is shown within the scroller.
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_region_show(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);

/**
 * @brief Set the scrollbar visibility policy
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] policy_h Horizontal scrollbar policy
 * @param[in] policy_v Vertical scrollbar policy
 *
 * @remark This sets the scrollbar visibility policy for the given scroller.
 * #ELM_SCROLLER_POLICY_AUTO means the scrollbar is made visible if it is
 * needed, and otherwise kept hidden. #ELM_SCROLLER_POLICY_ON turns it on all
 * the time, and #ELM_SCROLLER_POLICY_OFF always keeps it off. This applies
 * respectively for the horizontal and vertical scrollbars.
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_policy_set(Evas_Object *obj, Elm_Scroller_Policy policy_h, Elm_Scroller_Policy policy_v);

/**
 * @brief Get scrollbar visibility policy
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] policy_h Horizontal scrollbar policy
 * @param[out] policy_v Vertical scrollbar policy
 *
 * @see elm_scroller_policy_set()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_policy_get(const Evas_Object *obj, Elm_Scroller_Policy *policy_h, Elm_Scroller_Policy *policy_v);

/**
 * @brief Set the type of single direction scroll
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] single_dir The type of single direction
 *
 * @see elm_scroller_single_direction_get()
 *
 * @since 1.8
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_single_direction_set(Evas_Object *obj, Elm_Scroller_Single_Direction single_dir);

/**
 * @brief Get the type of single direction scroll
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @return the type of single direction
 *
 * @see elm_scroller_single_direction_get()
 *
 * @since 1.8
 *
 * @ingroup Scroller
 */
EAPI Elm_Scroller_Single_Direction elm_scroller_single_direction_get(const Evas_Object *obj);

/**
 * @brief Get the currently visible content region
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] x X coordinate of the region
 * @param[out] y Y coordinate of the region
 * @param[out] w Width of the region
 * @param[out] h Height of the region
 *
 * @remark This gets the current region in the content object that is visible through
 * the scroller. The region co-ordinates are returned in the @p x, @p y, @p
 * w, @p h values pointed to.
 *
 * @remark All coordinates are relative to the content.
 *
 * @see elm_scroller_region_show()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_region_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);

/**
 * @brief Get the size of the content object
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] w Width of the content object.
 * @param[out] h Height of the content object.
 *
 * @remark This gets the size of the content object of the scroller.
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_child_size_get(const Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);

/**
 * @internal
 *
 * @brief Set page snapping behavior of a scroller
 *
 * @param obj The scroller object
 * @param page_h_snap Allow snap horizontally
 * @param page_v_snap Allow snap vertically
 *
 * When scrolling, if a scroller is paged (see
 * elm_scroller_page_size_set() and elm_scroller_page_relative_set()),
 * the scroller may snap to pages when being scrolled, i.e., even if
 * it had momentum to scroll further, it will stop at the next page
 * boundaries. This is @b disabled, by default, for both axis. This
 * function will set if it that is enabled or not, for each axis.
 *
 * @note If @a obj is not set to have pages, nothing will happen after
 * this call.
 *
 * @since 1.8
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_snap_set(Evas_Object *obj, Eina_Bool page_h_snap, Eina_Bool page_v_snap);

/**
 * @internal
 *
 * @brief Get the page snap behaviour of a scroller
 *
 * @param obj The Scroller object
 * @param page_h_snap Will the scroller snap horizontally or not
 * @param page_v_snap Will the scroller snap vertically or not
 *
 * @see elm_scroller_page_snap_set() for more details
 *
 * @since 1.8
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_snap_get(const Evas_Object *obj, Eina_Bool *page_h_snap, Eina_Bool *page_v_snap);

/**
 * @brief Set bouncing behavior
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] h_bounce Allow bounce horizontally
 * @param[in] v_bounce Allow bounce vertically
 *
 * @remark When scrolling, the scroller may "bounce" when reaching an edge of the
 * content object. This is a visual way to indicate the end has been reached.
 * This is enabled by default for both axis. This API will set if it is enabled
 * for the given axis with the boolean parameters for each axis.
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce);

/**
 * @brief Get the bounce behaviour
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The Scroller object
 * @param[out] h_bounce Will the scroller bounce horizontally or not
 * @param[out] v_bounce Will the scroller bounce vertically or not
 *
 * @see elm_scroller_bounce_set()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce);

/**
 * @brief Set scroll page size relative to viewport size.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] h_pagerel The horizontal page relative size
 * @param[in] v_pagerel The vertical page relative size
 *
 * @remark The scroller is capable of limiting scrolling by the user to "pages". That
 * is to jump by and only show a "whole page" at a time as if the continuous
 * area of the scroller content is split into page sized pieces. This sets
 * the size of a page relative to the viewport of the scroller. 1.0 is "1
 * viewport" is size (horizontally or vertically). 0.0 turns it off in that
 * axis. This is mutually exclusive with page size
 * (see elm_scroller_page_size_set()  for more information). Likewise 0.5
 * is "half a viewport". Sane usable values are normally between 0.0 and 1.0
 * including 1.0. If you only want 1 axis to be page "limited", use 0.0 for
 * the other axis.
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_relative_set(Evas_Object *obj, double h_pagerel, double v_pagerel);

/**
 * @brief Get a given scroller widget's scrolling page size, relative to
 * its viewport size.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] h_pagerel Pointer to a variable where to store the
 * horizontal page (relative) size
 * @param[out] v_pagerel Pointer to a variable where to store the
 * vertical page (relative) size
 *
 * @see elm_scroller_page_relative_set() for more details
 *
 * @since 1.7
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_relative_get(const Evas_Object *obj, double *h_pagerel, double *v_pagerel);

/**
 * @brief Set scroll page size.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] h_pagesize The horizontal page size
 * @param[in] v_pagesize The vertical page size
 *
 * @remark This sets the page size to an absolute fixed value, with 0 turning it off
 * for that axis.
 *
 * @see elm_scroller_page_relative_set()
 * @see elm_scroller_page_size_get()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_size_set(Evas_Object *obj, Evas_Coord h_pagesize, Evas_Coord v_pagesize);

/**
 * @brief Get a scroller widget's current page size.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] h_pagesize Where to store its horizontal page size
 * @param[out] v_pagesize Where to store its vertical page size
 *
 * @see elm_scroller_page_size_set() for more details
 * @see elm_scroller_page_relative_set()
 *
 * @since 1.7
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_size_get(const Evas_Object *obj, Evas_Coord *h_pagesize, Evas_Coord *v_pagesize);

/**
 * @brief Get scroll current page number.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] h_pagenumber The horizontal page number
 * @param[out] v_pagenumber The vertical page number
 *
 * @remark The page number starts from 0. 0 is the first page.
 * Current page means the page which meets the top-left of the viewport.
 * If there are two or more pages in the viewport, it returns the number of the page
 * which meets the top-left of the viewport.
 *
 * @see elm_scroller_last_page_get()
 * @see elm_scroller_page_show()
 * @see elm_scroller_page_bring_in()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_current_page_get(const Evas_Object *obj, int *h_pagenumber, int *v_pagenumber);

/**
 * @brief Get scroll last page number.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] h_pagenumber The horizontal page number
 * @param[out] v_pagenumber The vertical page number
 *
 * @remark The page number starts from 0. 0 is the first page.
 * This returns the last page number among the pages.
 *
 * @see elm_scroller_current_page_get()
 * @see elm_scroller_page_show()
 * @see elm_scroller_page_bring_in()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_last_page_get(const Evas_Object *obj, int *h_pagenumber, int *v_pagenumber);

/**
 * @brief Show a specific virtual region within the scroller content object by page number.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] h_pagenumber The horizontal page number
 * @param[in] v_pagenumber The vertical page number
 *
 * @remark 0, 0 of the indicated page is located at the top-left of the viewport.
 * This will jump to the page directly without animation.
 *
 * @remark Example of usage:
 *
 * @code
 * sc = elm_scroller_add(win);
 * elm_object_content_set(sc, content);
 * elm_scroller_page_relative_set(sc, 1, 0);
 * elm_scroller_current_page_get(sc, &h_page, &v_page);
 * elm_scroller_page_show(sc, h_page + 1, v_page);
 * @endcode
 *
 * @see elm_scroller_page_bring_in()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_show(Evas_Object *obj, int h_pagenumber, int v_pagenumber);

/**
 * @brief Show a specific virtual region within the scroller content object by page number.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] h_pagenumber The horizontal page number
 * @param[in] v_pagenumber The vertical page number
 *
 * @remark 0, 0 of the indicated page is located at the top-left of the viewport.
 * This will slide to the page with animation.
 *
 * @remark Example of usage:
 *
 * @code
 * sc = elm_scroller_add(win);
 * elm_object_content_set(sc, content);
 * elm_scroller_page_relative_set(sc, 1, 0);
 * elm_scroller_last_page_get(sc, &h_page, &v_page);
 * elm_scroller_page_bring_in(sc, h_page, v_page);
 * @endcode
 *
 * @see elm_scroller_page_show()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_page_bring_in(Evas_Object *obj, int h_pagenumber, int v_pagenumber);

/**
 * @brief Show a specific virtual region within the scroller content object.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] x X coordinate of the region
 * @param[in] y Y coordinate of the region
 * @param[in] w Width of the region
 * @param[in] h Height of the region
 *
 * @remark This will ensure all (or part if it does not fit) of the designated
 * region in the virtual content object (0, 0 starting at the top-left of the
 * virtual content object) is shown within the scroller. Unlike
 * elm_scroller_region_show(), this allow the scroller to "smoothly slide"
 * to this location (if configuration in general calls for transitions). It
 * may not jump immediately to the new location and make take a while and
 * show other content along the way.
 *
 * @see elm_scroller_region_show()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_region_bring_in(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);

/**
 * @brief Set scrolling gravity on a scroller
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] x The scrolling horizontal gravity
 * @param[in] y The scrolling vertical gravity
 *
 * @remark The gravity, defines how the scroller will adjust its view
 * when the size of the scroller contents increase.
 *
 * @remark The scroller will adjust the view to glue itself as follows.
 *
 * @remark  x=0.0, for staying where it is relative to the left edge of the content
 *  x=1.0, for staying where it is relative to the rigth edge of the content
 *  y=0.0, for staying where it is relative to the top edge of the content
 *  y=1.0, for staying where it is relative to the bottom edge of the content
 *
 * @remark Default values for x and y are 0.0
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_gravity_set(Evas_Object *obj, double x, double y);

/**
 * @brief Get scrolling gravity values for a scroller
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[out] x The scrolling horizontal gravity
 * @param[out] y The scrolling vertical gravity
 *
 * @remark This gets gravity values for a scroller.
 *
 * @see elm_scroller_gravity_set()
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_gravity_get(const Evas_Object *obj, double *x, double *y);

/**
 * @brief Set blocking of scrolling (per axis) on a given scroller
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] block The axis to block
 *
 * @remark This function will block scrolling movement (by input of a user) in
 * a given direction. One can disable movements in the X axis, the Y
 * axis or both. The default value is #ELM_SCROLLER_MOVEMENT_NO_BLOCK,
 * where movements are allowed in both directions.
 *
 * @remark What makes this function different from
 * elm_object_scroll_freeze_push(), elm_object_scroll_hold_push() and
 * elm_object_scroll_lock_x_set() (or elm_object_scroll_lock_y_set())
 * is that it @b doesn't propagate its effects to any parent or child
 * widget of @a obj. Only the target scrollable widget will be locked
 * with regard to scrolling.
 *
 * @since 1.8
 *
 * @ingroup Scroller
 */
EAPI void                         elm_scroller_movement_block_set(Evas_Object *obj, Elm_Scroller_Movement_Block block);

/**
 * @brief Get a scroller's scroll blocking state
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The scroller object
 * @return The blocking state
 *
 * @since 1.8
 *
 * @see elm_scroller_movement_block_set() for more details
 *
 * @ingroup Scroller
 */
EAPI Elm_Scroller_Movement_Block  elm_scroller_movement_block_get(const Evas_Object *obj);

/**
 * @internal
 *
 * @brief Set scroll step size.
 *
 * @param obj The scroller object
 * @param x The horizontal step size
 * @param y The vertical step size
 *
 * This sets the step size to move scroller by key event.
 *
 * @see elm_scroller_step_size_get() for more details
 *
 * @since 1.13
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_step_size_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);

/**
 * @internal
 *
 * @brief Get the current step size of the scroller object.
 *
 * @param obj The scroller object
 * @param x Where to store its x axis step size
 * @param y Where to store its y axis step size
 *
 * @see elm_scroller_step_size_set() for more details
 *
 * @since 1.13
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_step_size_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);

/**
 * @brief Set an infinite loop_ for a scroller
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The scroller object
 * @param[in] loop_h The scrolling horizontal loop
 * @param[in] loop_v The scrolling vertical loop
 *
 * @remark This function sets the infinite loop horizontally or vertically.
 * If the content is set, it will be shown repeatedly.
 *
 * @since 1.14
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_loop_set(Evas_Object *obj, Eina_Bool loop_h, Eina_Bool loop_v);

/**
 * @internal
 *
 * @brief Get an infinite loop_ for a scroller
 *
 * @param obj The scroller object
 * @param loop_h The scrolling horizontal loop
 * @param loop_v The scrolling vertical loop
 *
 * @since 1.14
 *
 * @see elm_scroller_loop_set() for more details
 *
 * @ingroup Scroller
 */
EAPI void                          elm_scroller_loop_get(const Evas_Object *obj, Eina_Bool *loop_h, Eina_Bool *loop_v);

/**
 * @internal
 * @remarks Tizen only feature
 */
EAPI void                          elm_scroller_origin_reverse_set(Evas_Object *obj, Eina_Bool origin_x, Eina_Bool origin_y);

/**
 * @internal
 * @remarks Tizen only feature
 */
EAPI void                          elm_scroller_origin_reverse_get(Evas_Object *obj, Eina_Bool *origin_x, Eina_Bool *origin_y);

/**
 * @internal
 * @remarks Tizen only feature
 *
 * @brief Sets the mouse wheel event on a scroller.
 *
 * @details This enables or disables the mouse wheel event for a scroller.
 *          By default mouse wheel event is enabled.
 *
 * @param obj The scroller object
 * @param disabled The boolean value that indicates whether the wheel event is disabled
 */
EAPI void                          elm_scroller_wheel_disabled_set(Evas_Object *obj, Eina_Bool disabled);

/**
 * @internal
 * @remarks Tizen only feature
 *
 * @brief Gets the mouse wheel event for a scroller.
 *
 * @remarks This gets whether the mouse wheel event is disabled.
 *
 * @param obj The scroller object
 *
 * @see elm_scroller_wheel_disabled_set()
 */
EAPI Eina_Bool                      elm_scroller_wheel_disabled_get(const Evas_Object *obj);

#include "elm_scroller.eo.legacy.h"
