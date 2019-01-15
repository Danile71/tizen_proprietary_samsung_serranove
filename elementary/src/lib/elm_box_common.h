/**
 * @addtogroup Box
 *
 * @{
 */

/**
 * @typedef Elm_Box_Transition
 *
 * Opaque handler containing the parameters to perform an animated
 * transition of the layout the box uses.
 *
 * @see elm_box_transition_new()
 * @see elm_box_layout_set()
 * @see elm_box_layout_transition()
 */
typedef struct _Elm_Box_Transition Elm_Box_Transition;

/**
 * @brief Special layout function that animates the transition from one layout to another
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The box object.
 * @param[in] priv Smart object instance data, providing box object requirements.
 * @param[in] data The data that will be passed to layout function.
 *
 * @remark Normally, when switching the layout function for a box, this will be
 * reflected immediately on screen on the next render, but it's also
 * possible to do this through an animated transition.
 *
 * @remark This is done by creating an ::Elm_Box_Transition and setting the box
 * layout to this function.
 *
 * @remark For example:
 * @code
 * Elm_Box_Transition *t = elm_box_transition_new(1.0,
 *                            evas_object_box_layout_vertical, // start
 *                            NULL, // data for initial layout
 *                            NULL, // free function for initial data
 *                            evas_object_box_layout_horizontal, // end
 *                            NULL, // data for final layout
 *                            NULL, // free function for final data
 *                            anim_end, // will be called when animation ends
 *                            NULL); // data for anim_end function
 * elm_box_layout_set(box, elm_box_layout_transition, t,
 *                    elm_box_transition_free);
 * @endcode
 *
 * @remark This function can only be used with elm_box_layout_set(). Calling
 * it directly will not have the expected results.
 *
 * @see elm_box_transition_new
 * @see elm_box_transition_free
 * @see elm_box_layout_set
 */
EAPI void                elm_box_layout_transition(Evas_Object *obj, Evas_Object_Box_Data *priv, void *data);

/**
 * @brief Create a new ::Elm_Box_Transition to animate the switch of layouts
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark If you want to animate the change from one layout to another, you need
 * to set the layout function of the box to elm_box_layout_transition(),
 * passing as user data to it an instance of ::Elm_Box_Transition with the
 * necessary information to perform this animation. The free function to
 * set for the layout is elm_box_transition_free().
 *
 * @remark The parameters to create an ::Elm_Box_Transition sum up to how long
 * will it be, in seconds, a layout function to describe the initial point,
 * another for the final position of the children and one function to be
 * called when the whole animation ends. This last function is useful to
 * set the definitive layout for the box, usually the same as the end
 * layout for the animation, but could be used to start another transition.
 *
 * @param[in] duration The duration of the transition in seconds
 * @param[in] start_layout The layout function that will be used to start the animation
 * @param[in] start_layout_data The data to be passed the @p start_layout function
 * @param[in] start_layout_free_data Function to free @p start_layout_data
 * @param[in] end_layout The layout function that will be used to end the animation
 * @param[in] end_layout_data Data param passed to @p end_layout
 * @param[in] end_layout_free_data The data to be passed the @p end_layout function
 * @param[in] end_layout_free_data Function to free @p end_layout_data
 * @param[in] transition_end_cb Callback function called when animation ends
 * @param[in] transition_end_data Data to be passed to @p transition_end_cb
 * @return An instance of ::Elm_Box_Transition
 *
 * @see elm_box_transition_new
 * @see elm_box_layout_transition
 */
EAPI Elm_Box_Transition *elm_box_transition_new(const double duration, Evas_Object_Box_Layout start_layout, void *start_layout_data, Ecore_Cb start_layout_free_data, Evas_Object_Box_Layout end_layout, void *end_layout_data, Ecore_Cb end_layout_free_data, Ecore_Cb transition_end_cb, void *transition_end_data);

/**
 * @brief Free a Elm_Box_Transition instance created with elm_box_transition_new().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This function is mostly useful as the @c free_data parameter in
 * elm_box_layout_set() when elm_box_layout_transition().
 *
 * @param[in] data The Elm_Box_Transition instance to be freed.
 *
 * @see elm_box_transition_new
 * @see elm_box_layout_transition
 */
EAPI void                elm_box_transition_free(void *data);

/**
 * @}
 */
