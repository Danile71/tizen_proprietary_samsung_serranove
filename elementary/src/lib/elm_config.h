/**
 * @defgroup Config Elementary Config
 * @ingroup elm_infra_group
 *
 * Elementary configuration is formed by a set options bounded to a
 * given @ref Profile profile, like @ref Theme theme, @ref Fingers
 * "finger size", etc. These are functions with which one synchronizes
 * changes made to those values to the configuration storing files, de
 * facto. You most probably don't want to use the functions in this
 * group unless you're writing an elementary configuration manager.
 *
 * @{
 */

/**
 * @internal
 *
 * @brief Save back Elementary's configuration, so that it will persist on
 * future sessions.
 *
 * @return @c EINA_TRUE, when successful. @c EINA_FALSE, otherwise.
 * @ingroup Config
 *
 * @remark This function will take effect -- thus, do I/O -- immediately. Use
 * it when you want to save all configuration changes at once. The
 * current configuration set will get saved onto the current profile
 * configuration file.
 *
 */
EAPI Eina_Bool elm_config_save(void);

/**
 * @brief Reload Elementary's configuration, bounded to current selected
 * profile.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return @c EINA_TRUE, when successful. @c EINA_FALSE, otherwise.
 * @ingroup Config
 *
 * @remark Useful when you want to force reloading of configuration values for
 * a profile. If one removes user custom configuration directories,
 * for example, it will force a reload with system values instead.
 *
 */
EAPI void      elm_config_reload(void);

/**
 * @brief Flush all config settings then apply those settings to all applications
 * using elementary on the current display.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Config
 */
EAPI void      elm_config_all_flush(void);

/**
 * @}
 */

/**
 * @defgroup Profile Elementary Profile
 * @ingroup elm_infra_group
 *
 * Profiles are pre-set options that affect the whole look-and-feel of
 * Elementary-based applications. There are, for example, profiles
 * aimed at desktop computer applications and others aimed at mobile,
 * touchscreen-based ones. You most probably don't want to use the
 * functions in this group unless you're writing an elementary
 * configuration manager.
 *
 * @{
 */

/**
 * @brief Get Elementary's profile in use.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the global profile that is applied to all Elementary
 * applications.
 *
 * @return The profile's name
 * @ingroup Profile
 */
EAPI const char *elm_config_profile_get(void);

/**
 * @brief Get an Elementary's profile directory path in the filesystem. One
 * may want to fetch a system profile's dir or a user one (fetched
 * inside $HOME).
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] profile The profile's name
 * @param[in] is_user Whether to lookup for a user profile (@c EINA_TRUE)
 *                or a system one (@c EINA_FALSE)
 * @return The profile's directory path.
 * @ingroup Profile
 *
 * @remark You must free it with elm_config_profile_dir_free().
 */
EAPI const char *elm_config_profile_dir_get(const char *profile, Eina_Bool is_user);

/**
 * @brief Free an Elementary's profile directory path, as returned by
 * elm_config_profile_dir_get().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] p_dir The profile's path
 * @ingroup Profile
 *
 */
EAPI void        elm_config_profile_dir_free(const char *p_dir);

/**
 * @brief Get Elementary's list of available profiles.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The profiles list. List node data are the profile name
 *         strings.
 * @ingroup Profile
 *
 * @remark One must free this list, after usage, with the function
 *       elm_config_profile_list_free().
 */
EAPI Eina_List  *elm_config_profile_list_get(void);

/**
 * @brief Free Elementary's list of available profiles.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] l The profiles list, as returned by elm_config_profile_list_get().
 * @ingroup Profile
 *
 */
EAPI void        elm_config_profile_list_free(Eina_List *l);

/**
 * @brief Set Elementary's profile.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the global profile that is applied to Elementary
 * applications. Just the process the call comes from will be
 * affected.
 *
 * @param[in] profile The profile's name
 * @ingroup Profile
 *
 */
EAPI void        elm_config_profile_set(const char *profile);

/**
 * @internal
 * @remarks Tizen only feature
 *
 * @brief Checks whether the given Elementary's profile exists.
 *
 * @param[in] profile The profile name
 * @return @c EINA_TRUE if the profile exists, otherwise @c EINA_FALSE
 */
EAPI Eina_Bool   elm_config_profile_exists(const char *profile);

/**
 * @}
 */

/**
 * @defgroup Scrolling Elementary Scrolling
 * @ingroup elm_infra_group
 *
 * These are functions setting how scrollable views in Elementary
 * widgets should behave on user interaction.
 *
 * @{
 */

/**
 * @brief Get whether scrollers should bounce when they reach their
 * viewport's edge during a scroll.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll bouncing state
 *
 * @remark This is the default behavior for touch screens, in general.
 * @ingroup Scrolling
 */
EAPI Eina_Bool    elm_config_scroll_bounce_enabled_get(void);

/**
 * @brief Set whether scrollers should bounce when they reach their
 * viewport's edge during a scroll.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] enabled the thumb scroll bouncing state
 *
 * @see elm_config_scroll_bounce_enabled_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_bounce_enabled_set(Eina_Bool enabled);

/**
 * @brief Get the amount of inertia a scroller will impose at bounce
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll bounce friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_bounce_friction_get(void);

/**
 * @brief Set the amount of inertia a scroller will impose at bounce
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the thumb scroll bounce friction
 *
 * @see elm_config_scroll_bounce_friction_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_bounce_friction_set(double friction);

/**
 * @brief Get the amount of inertia a <b>paged</b> scroller will impose at
 * page fitting animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the page scroll friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_page_scroll_friction_get(void);

/**
 * @brief Set the amount of inertia a <b>paged</b> scroller will impose at
 * page fitting animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the page scroll friction
 *
 * @see elm_config_scroll_page_scroll_friction_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_page_scroll_friction_set(double friction);

/**
 * @brief Get the amount of inertia a scroller will impose at region bring
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the bring in scroll friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_bring_in_scroll_friction_get(void);

/**
 * @brief Set the amount of inertia a scroller will impose at region bring
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the bring in scroll friction
 *
 * @see elm_config_scroll_bring_in_scroll_friction_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_bring_in_scroll_friction_set(double friction);

/**
 * @brief Get the amount of inertia scrollers will impose at animations
 * triggered by Elementary widgets' zooming API.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the zoom friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_zoom_friction_get(void);

/**
 * @brief Set the amount of inertia scrollers will impose at animations
 * triggered by Elementary widgets' zooming API.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the zoom friction
 *
 * @see elm_config_scroll_zoom_friction_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_zoom_friction_set(double friction);

/**
 * @brief Get whether scrollers should be draggable from any point in their
 * views.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll state
 *
 * @remark This is the default behavior for touch screens, in general.
 * @remark All other functions namespaced with "thumbscroll" will only
 *       have effect if this mode is enabled.
 *
 * @ingroup Scrolling
 */
EAPI Eina_Bool    elm_config_scroll_thumbscroll_enabled_get(void);

/**
 * @brief Set whether scrollers should be draggable from any point in their
 * views.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] enabled the thumb scroll state
 *
 * @see elm_config_scroll_thumbscroll_enabled_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_enabled_set(Eina_Bool enabled);

/**
 * @brief Get the number of pixels one should travel while dragging a
 * scroller's view to actually trigger scrolling.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll threshold
 *
 * @remark One would use higher values for touch screens, in general, because
 * of their inherent imprecision.
 * @ingroup Scrolling
 */
EAPI unsigned int elm_config_scroll_thumbscroll_threshold_get(void);

/**
 * @brief Set the number of pixels one should travel while dragging a
 * scroller's view to actually trigger scrolling.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] threshold the thumb scroll threshold
 *
 * @see elm_config_thumbscroll_threshold_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_threshold_set(unsigned int threshold);

/**
 * @brief Get the number of pixels the range which can be scrolled,
 * while the scroller is holded.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll hold threshold
 *
 * @ingroup Scrolling
 */
EAPI unsigned int elm_config_scroll_thumbscroll_hold_threshold_get(void);

/**
 * @brief Set the number of pixels the range which can be scrolled,
 * while the scroller is holded.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] threshold the thumb scroll hold threshold
 *
 * @see elm_config_thumbscroll_hold_threshold_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_hold_threshold_set(unsigned int threshold);

/**
 * @brief Get the minimum speed of mouse cursor movement which will trigger
 * list self scrolling animation after a mouse up event
 * (pixels/second).
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll momentum threshold
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_momentum_threshold_get(void);

/**
 * @brief Set the minimum speed of mouse cursor movement which will trigger
 * list self scrolling animation after a mouse up event
 * (pixels/second).
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] threshold the thumb scroll momentum threshold
 *
 * @see elm_config_thumbscroll_momentum_threshold_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_momentum_threshold_set(double threshold);

/**
 * @brief Get the number of pixels the maximum distance which can be flicked.
 * If it is flicked more than this,
 * the flick distance is same with maximum distance.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll maximum flick distance
 *
 * @ingroup Scrolling
 */
EAPI unsigned int elm_config_scroll_thumbscroll_flick_distance_tolerance_get(void);

/**
 * @brief Set the number of pixels the maximum distance which can be flicked.
 * If it is flicked more than this,
 * the flick distance is same with maximum distance.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] distance the thumb scroll maximum flick distance
 *
 * @see elm_config_thumbscroll_flick_distance_tolerance_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_flick_distance_tolerance_set(unsigned int distance);

/**
 * @brief Get the amount of inertia a scroller will impose at self scrolling
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_friction_get(void);

/**
 * @brief Set the amount of inertia a scroller will impose at self scrolling
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the thumb scroll friction
 *
 * @see elm_config_thumbscroll_friction_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_friction_set(double friction);

/**
 * @brief Get the min amount of inertia a scroller will impose at self scrolling
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll min friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_min_friction_get(void);

/**
 * @brief Set the min amount of inertia a scroller will impose at self scrolling
 * animations.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the thumb scroll min friction
 *
 * @see elm_config_thumbscroll_min_friction_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_min_friction_set(double friction);

/**
 * @brief Get the standard velocity of the scroller. The scroll animation time is
 * same with thumbscroll friction, if the velocity is same with standard
 * velocity.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_friction_standard_get(void);

/**
 * @brief Set the standard velocity of the scroller. The scroll animation time is
 * same with thumbscroll friction, if the velocity is same with standard
 * velocity.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] standard the thumb scroll friction standard
 *
 * @see elm_config_thumbscroll_friction_standard_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_friction_standard_set(double standard);

/**
 * @brief Get the amount of lag between your actual mouse cursor dragging
 * movement and a scroller's view movement itself, while pushing it
 * into bounce state manually.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll border friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_border_friction_get(void);

/**
 * @brief Set the amount of lag between your actual mouse cursor dragging
 * movement and a scroller's view movement itself, while pushing it
 * into bounce state manually.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the thumb scroll border friction. @c 0.0 for
 *        perfect synchrony between two movements, @c 1.0 for maximum
 *        lag.
 *
 * @see elm_config_thumbscroll_border_friction_get()
 * @remark parameter value will get bound to 0.0 - 1.0 interval, always
 *
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_border_friction_set(double friction);

/**
 * @brief Get the sensitivity amount which is be multiplied by the length of
 * mouse dragging.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll sensitivity friction
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_sensitivity_friction_get(void);

/**
 * @brief Set the sensitivity amount which is be multiplied by the length of
 * mouse dragging.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] friction the thumb scroll sensitivity friction. @c 0.1 for
 *        minimum sensitivity, @c 1.0 for maximum sensitivity. 0.25
 *        is proper.
 *
 * @see elm_config_thumbscroll_sensitivity_friction_get()
 * @remark parameter value will get bound to 0.1 - 1.0 interval, always
 *
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_sensitivity_friction_set(double friction);

/**
 * @brief Get the minimum speed of mouse cursor movement which will accelerate
 * scrolling velocity after a mouse up event
 * (pixels/second).
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll acceleration threshold
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_acceleration_threshold_get(void);

/**
 * @brief Set the minimum speed of mouse cursor movement which will accelerate
 * scrolling velocity after a mouse up event
 * (pixels/second).
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] threshold the thumb scroll acceleration threshold
 *
 * @see elm_config_thumbscroll_acceleration_threshold_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_acceleration_threshold_set(double threshold);

/**
 * @brief Get the time limit for accelerating velocity.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll acceleration time limit
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_acceleration_time_limit_get(void);

/**
 * @brief Set the time limit for accelerating velocity.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] time_limit the thumb scroll acceleration time limit
 *
 * @see elm_config_thumbscroll_acceleration_time_limit_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_acceleration_time_limit_set(double time_limit);

/**
 * @brief Get the weight for the acceleration.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return the thumb scroll acceleration weight
 *
 * @ingroup Scrolling
 */
EAPI double       elm_config_scroll_thumbscroll_acceleration_weight_get(void);

/**
 * @brief Set the weight for the acceleration.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] weight the thumb scroll acceleration weight
 *
 * @see elm_config_thumbscroll_acceleration_weight_get()
 * @ingroup Scrolling
 */
EAPI void         elm_config_scroll_thumbscroll_acceleration_weight_set(double weight);

// TIZEN_ONLY(20150705): Genlist item align feature
/**
 * @brief Sets the scroll item align enable.
 *
 * @since_tizen 2.3.1
 *
 * @param[in] enable The scroll item align state
 *
 * @see elm_config_scroll_item_align_enabled_get()
 */
EAPI void elm_config_scroll_item_align_enabled_set(Eina_Bool enable);

/**
 * @brief Sets the scroll item align enable.
 *
 * @since_tizen 2.3.1
 *
 * @return The scroll item align state.
 *
 * @see elm_config_scroll_item_align_enabled_set()
 */
EAPI Eina_Bool elm_config_scroll_item_align_enabled_get(void);

/**
 * @brief Sets the scroll item vertical align.
 *
 * @since_tizen 2.3.1
 *
 * @param[in] scroll_item_valign The position name.
 *
 * @see elm_config_scroll_item_valign_get()
 */
EAPI void elm_config_scroll_item_valign_set(const char *scroll_item_valign);

/**
 * @brief Sets the scroll item vertical align.
 *
 * @since_tizen 2.3.1
 *
 * @return The scroll item vertical align name.
 *
 * @see elm_config_scroll_item_valign_set()
 */
EAPI const char *elm_config_scroll_item_valign_get(void);
//

/**
 * @internal
 *
 * Focus Autoscroll Mode
 *
 * @since 1.10
 * @ingroup Focus
 */
typedef enum
{
   ELM_FOCUS_AUTOSCROLL_MODE_SHOW, /**< @internal directly show the focused region or item automatically */
   ELM_FOCUS_AUTOSCROLL_MODE_NONE, /**< @internal do not show the focused region or item automatically */
   ELM_FOCUS_AUTOSCROLL_MODE_BRING_IN /**< @internal bring_in the focused region or item automatically which might invole the scrolling */
} Elm_Focus_Autoscroll_Mode;

/**
 * @internal
 *
 * Get focus auto scroll mode.
 *
 * When a region or an item is focused and it resides inside any scroller,
 * elementary will automatically scroll the focused area to the visible
 * viewport.
 *
 * @see elm_config_focus_autoscroll_mode_set()
 * @ingroup Focus
 * @since 1.10
 */
EAPI Elm_Focus_Autoscroll_Mode elm_config_focus_autoscroll_mode_get(void);

/**
 * @internal
 *
 * Set focus auto scroll mode.
 *
 * @param mode focus auto scroll mode. This can be one of the
 * Elm_Focus_Autoscroll_Mode enum values.
 *
 * When a region or an item is focused and it resides inside any scroller,
 * elementary will automatically scroll the focused area to the visible
 * viewport.
 * Focus auto scroll mode is set to #ELM_FOCUS_AUTOSCROLL_MODE_SHOW by
 * default historically.
 *
 * @see elm_config_focus_autoscroll_mode_get()
 * @ingroup Focus
 * @since 1.10
 */
EAPI void         elm_config_focus_autoscroll_mode_set(Elm_Focus_Autoscroll_Mode mode);

/**
 * Slider's indicator visiblity mode.
 *
 * @since 1.13
 * @ingroup Slider
 */

typedef enum
{
   ELM_SLIDER_INDICATOR_VISIBLE_MODE_DEFAULT,   /**< show indicator on mouse down or change in slider value */
   ELM_SLIDER_INDICATOR_VISIBLE_MODE_ALWAYS,    /**< Always show the indicator. */
   ELM_SLIDER_INDICATOR_VISIBLE_MODE_ON_FOCUS,  /**< Show the indicator on focus */
   ELM_SLIDER_INDICATOR_VISIBLE_MODE_NONE       /**< Never show the indicator  */
} Elm_Slider_Indicator_Visible_Mode;

/**
 * @internal
 *
 * Sets the slider's indicator visible mode.
 *
 * @param obj The slider object.
 * @param mode Elm_Slider_Indicator_Visible_Mode.
 * viewport.
 *
 * @ingroup Slider
 * @since 1.13
 */
EAPI void elm_config_slider_indicator_visible_mode_set(Elm_Slider_Indicator_Visible_Mode mode);

/**
 * @internal
 *
 * Get the slider's indicator visible mode.
 *
 * @param obj The slider object.
 * @return @c ELM_SLIDER_INDICATOR_VISIBLE_MODE_DEFAULT if not set anything by the user.
 * @c ELM_SLIDER_INDICATOR_VISIBLE_MODE_ALWAYS, ELM_SLIDER_INDICATOR_VISIBLE_MODE_ON_FOCUS,
 *    ELM_SLIDER_INDICATOR_VISIBLE_MODE_NONE if any of the above is set by user.
 *
 * @ingroup Slider
 * @since 1.13
 */
EAPI Elm_Slider_Indicator_Visible_Mode elm_config_slider_indicator_visible_mode_get(void);

/**
 * @}
 */

/**
 * @defgroup longpress_group Longpress
 * @ingroup elm_infra_group
 *
 * @brief Configuration for longpress events.
 *
 * @{
 */

/**
 * @brief Get the duration for occurring long press event.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return Timeout for long press event
 * @ingroup Longpress
 */
EAPI double       elm_config_longpress_timeout_get(void);

/**
 * @brief Set the duration for occurring long press event.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] lonpress_timeout Timeout for long press event
 * @ingroup Longpress
 */
EAPI void         elm_config_longpress_timeout_set(double longpress_timeout);

/**
 * @}
 */

/**
 * @defgroup softcursor_group SotfCursor
 * @ingroup elm_infra_group
 *
 * @brief Configuration for softcursor.
 *
 * @{
 */

typedef enum _Elm_Softcursor_Mode
{
   ELM_SOFTCURSOR_MODE_AUTO, /**< Auto-detect if a software cursor should be used (default) */
   ELM_SOFTCURSOR_MODE_ON, /**< Always use a softcursor */
   ELM_SOFTCURSOR_MODE_OFF /**< Never use a softcursor */
} Elm_Softcursor_Mode; /**< @since 1.7 */

/**
 * @brief Set the mode used for software provided mouse cursors inline in the window
 * canvas.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark A software rendered cursor can be provided for rendering inline inside the
 * canvas windows in the event the native display system does not provide one
 * or the native one is not wanted.
 *
 * @param[in] lonpress_timeout Timeout for long press event
 * @ingroup Softcursor
 *
 * @see elm_config_softcursor_mode_get()
 * @since 1.7
 */
EAPI void         elm_config_softcursor_mode_set(Elm_Softcursor_Mode mode);

/**
 * @brief Get the software cursor mode
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The mode used for software cursors
 * @ingroup Softcursor
 *
 * @see elm_config_softcursor_mode_set()
 * @since 1.7
 */
EAPI Elm_Softcursor_Mode elm_config_softcursor_mode_get(void);

/**
 * @}
 */

/**
 * @ingroup Tooltips
 * @{
 */

/**
 * @brief Get the duration after which tooltip will be shown.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return Duration after which tooltip will be shown.
 */
EAPI double      elm_config_tooltip_delay_get(void);

/**
 * @brief Set the duration after which tooltip will be shown.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] delay Duration after which tooltip will be shown.
 */
EAPI void        elm_config_tooltip_delay_set(double delay);

/**
 * @}
 */

/**
 * @ingroup Cursors
 * @{
 */

/**
 * @brief Get the configured cursor engine only usage
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured exclusive usage of engine cursors.
 *
 * @return 1 if only engine cursors should be used
 * @ingroup Cursors
 */
EAPI Eina_Bool   elm_config_cursor_engine_only_get(void);

/**
 * @brief Set the configured cursor engine only usage
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured exclusive usage of engine cursors.
 * It won't affect cursors set before changing this value.
 *
 * @param[in] engine_only If 1 only engine cursors will be enabled, if 0 will
 * look for them on theme before.
 * @ingroup Cursors
 */
EAPI void        elm_config_cursor_engine_only_set(Eina_Bool engine_only);

/**
 * @}
 */

/**
 * @ingroup Scaling
 * @{
 */

/**
 * @brief Get the global scaling factor
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured scaling factor that is applied to all
 * objects.
 *
 * @return The scaling factor
 * @ingroup Scaling
 */
EAPI double elm_config_scale_get(void);

/**
 * @brief Set the global scaling factor
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured scaling factor that is applied to all
 * objects.
 *
 * @param[in] scale The scaling factor to set
 * @ingroup Scaling
 */
EAPI void   elm_config_scale_set(double scale);

/*
 * Add backwards compatability implementation for elm_scale_get and elm_scale_set
 * to allow running unmodified Tizen applications on a new drop of elementary
 */
EAPI double elm_scale_get(void);
EAPI void   elm_scale_set(double scale);

/**
 * @}
 */

/**
 * @defgroup Password_last_show Password show last
 * @ingroup elm_infra_group
 *
 * Show last feature of password mode enables user to view
 * the last input entered for few seconds before masking it.
 * These functions allow to set this feature in password mode
 * of entry widget and also allow to manipulate the duration
 * for which the input has to be visible.
 *
 * @{
 */

/**
 * @brief Get the "show last" setting of password mode.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the "show last" setting of password mode which might be
 * enabled or disabled.
 *
 * @return @c EINA_TRUE, if the "show last" setting is enabled,
 * @c EINA_FALSE  if it's disabled.
 *
 * @ingroup Password_last_show
 */
EAPI Eina_Bool elm_config_password_show_last_get(void);

/**
 * @brief Set show last setting in password mode.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This enables or disables show last setting of password mode.
 *
 * @param[in] password_show_last If @c EINA_TRUE, enables "show last" in password mode.
 * @see elm_config_password_show_last_timeout_set()
 * @ingroup Password_last_show
 */
EAPI void      elm_config_password_show_last_set(Eina_Bool password_show_last);

/**
 * @brief Get the timeout value in "show last" password mode.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the time out value for which the last input entered in password
 * mode will be visible.
 *
 * @return The timeout value of "show last" password mode.
 * @ingroup Password_last_show
 */
EAPI double    elm_config_password_show_last_timeout_get(void);

/**
 * @brief Set's the timeout value in "show last" password mode.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the time out value for which the last input entered in password
 * mode will be visible.
 *
 * @param[in] password_show_last_timeout The timeout value.
 * @see elm_config_password_show_last_set()
 * @ingroup Password_last_show
 */
EAPI void      elm_config_password_show_last_timeout_set(double password_show_last_timeout);

/**
 * @}
 */

/**
 * @defgroup Engine Elementary Engine
 * @ingroup elm_infra_group
 *
 * These are functions setting and querying which rendering engine
 * Elementary will use for drawing its windows' pixels.
 *
 * The following are the available engines:
 * @li "fb" (Framebuffer)
 * @li "buffer" (Pixel Memory Buffer)
 * @li "ews" (Ecore + Evas Single Process Windowing System)
 * @li NULL - no engine config
 *
 * @deprecated Please use elm_config_accel_preference_override_set() instead
 *
 * @{
 */

/**
 * @brief Get Elementary's rendering engine in use.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The rendering engine's name
 * @remark there's no need to free the returned string, here.
 *
 * @remark This gets the global rendering engine that is applied to all Elementary
 * applications.
 *
 * @see elm_config_engine_set()
 * @deprecated use elm_config_accel_preference_get() + elm_config_accel_preference_set()
 *             Deprecated since Tizen 2.4
 */
EAPI const char *elm_config_engine_get(void);

/**
 * @brief Set Elementary's rendering engine for use.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] engine The rendering engine's name
 *
 * @remark Note that it will take effect only to Elementary windows created after
 * this is called.
 *
 * @see elm_win_add()
 * @deprecated use elm_config_accel_preference_get() + elm_config_accel_preference_set()
 *             Deprecated since Tizen 2.4
 */
EAPI void        elm_config_engine_set(const char *engine);

/**
 * @brief Get Elementary's preferred engine to use.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The rendering engine's name
 * @remark there's no need to free the returned string, here.
 *
 * @remark This gets the global rendering engine that is applied to all Elementary
 * applications and is PREFERRED by the application. This can (and will)
 * override the engine configured for all applications which.
 *
 * @see elm_config_preferred_engine_set()
 */
EAPI const char *elm_config_preferred_engine_get(void);

/**
 * @brief Set Elementary's preferred rendering engine for use.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] engine The rendering engine's name
 *
 * @remark Note that it will take effect only to Elementary windows created after
 * this is called. This overrides the engine set by configuration at
 * application startup. Note that it is a hint and may not be honored.
 *
 * @see elm_win_add()
 * @see elm_config_accel_preference_set()
 * @see elm_config_engine_set()
 */
EAPI void        elm_config_preferred_engine_set(const char *engine);

/**
 * @brief Get Elementary's preferred engine to use.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The acceleration preference hint string
 * @remark there's no need to free the returned string, here.
 *
 * @remark See elm_config_accel_preference_set() for more information, but this simply
 * returns what was set by this call, nothing more.
 *
 * @see elm_config_accel_preference_set()
 * @since 1.10
 */
EAPI const char *elm_config_accel_preference_get(void);

/**
 * @brief Set Elementary's acceleration preferences for new windows.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] pref The preference desired as a normal C string
 *
 * @remark Note that it will take effect only to Elementary windows created after
 * this is called. The @p pref string is a freeform C string that indicates
 * what kind of acceleration is preferred. Here "acceleration" majorly
 * means to rendering and which hardware unit application renders guis with.
 * This may or may not be honored, but a best attempt will
 * be made. Known strings are as follows:
 *
 * @li "gl", "opengl" - try use opengl.
 * @li "3d" - try and use a 3d acceleration unit.
 * @li "hw", "hardware", "accel" - try any acceleration unit (best possible)
 * @li "none" - use no acceleration. try use software
 *
 * @details Accel preference's string has the egl surface configuration as a hw accel, depth, stencil and msaa.
 * The string format is "{HW Accel}:depth{value}:stencil{value}:msaa{msaa string}".
 * Especially, msaa string is related Evas GL MSAA enum value(low, mid, high).
 * so msaa string has four types as msaa, msaa_low, msaa_mid, msaa_high.
 * For instance, "opengl:depth24:stencil8:msaa_high".
 * It means that using hw accelation, window surface depth buffer's size is 24, stencil buffer's size 8 and msaa bits is the highest.
 * The other use-case is  "opengl:depth24".
 * It measn that using hw accelation, depth buffer size is 24. stencil and msaa are not used.
 * Default case is  "opengl:depth:stencil:msaa".
 * It means that depth, stencil and msaa are setted by default value(depth:24, stencil:8, msaa:low).
 *
 * @remark This takes precedence over engine preferences set with
 * elm_config_preferred_engine_set().
 *
 * @see elm_win_add()
 * @see elm_config_accel_preference_override_set()
 * @see elm_glview_mode_set()
 *
 * @since 1.10
 */
EAPI void        elm_config_accel_preference_set(const char *pref);


typedef struct _Elm_Text_Class
{
   const char *name;
   const char *desc;
} Elm_Text_Class;

/**
 * @brief Structure of Elm Font Overlay
 * @ingroup Fonts
 */
typedef struct _Elm_Font_Overlay
{
   const char    *text_class;
   const char    *font;
   Evas_Font_Size size;
} Elm_Font_Overlay;

/**
 * @brief Get Elementary's list of supported text classes.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The text classes list, with @c Elm_Text_Class blobs as data.
 * @ingroup Fonts
 *
 * @remark Release the list with elm_text_classes_list_free().
 */
EAPI Eina_List *elm_config_text_classes_list_get(void);

/**
 * @brief Free Elementary's list of supported text classes.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] list The list of supported text classes.
 * @ingroup Fonts
 *
 * @see elm_config_text_classes_list_get().
 */
EAPI void elm_config_text_classes_list_free(Eina_List *list);

/**
 * @brief Get Elementary's list of font overlays, set with
 * elm_config_font_overlay_set().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The font overlays list, with @c Elm_Font_Overlay blobs as
 * data.
 *
 * @ingroup Fonts
 *
 * @remark For each text class, one can set a <b>font overlay</b> for it,
 * overriding the default font properties for that class coming from
 * the theme in use. There is no need to free this list.
 *
 * @see elm_config_font_overlay_set() and elm_config_font_overlay_unset().
 */
EAPI const Eina_List *elm_config_font_overlay_list_get(void);

/**
 * @brief Set a font overlay for a given Elementary text class.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] text_class Text class name
 * @param[in] font Font name and style string
 * @param[in] size Font size.
 *
 * @remark If the @p size is lower than zero, the value will be the amount of the size percentage. ex) -50: half of the current size, -100: current size, -10: 1/10 size.
 *
 * @ingroup Fonts
 *
 * @remark @p font has to be in the format returned by elm_font_fontconfig_name_get().
 * @see elm_config_font_overlay_list_get()
 * @see elm_config_font_overlay_unset()
 * @see edje_object_text_class_set()
 */
EAPI void             elm_config_font_overlay_set(const char *text_class, const char *font, Evas_Font_Size size);

/**
 * @internal
 *
 * Get access mode
 *
 * @return the access mode bouncing state
 *
 * @since 1.7
 *
 * @ingroup Access
 *
 * @see elm_config_access_set()
 */
EAPI Eina_Bool        elm_config_access_get(void);

/**
 * @internal
 *
 * Set access mode
 *
 * @param is_access If @c EINA_TRUE, enables access mode
 *
 * @note Elementary objects may have information (e.g. label on the elm_button)
 * to be read. This information is read by access module when an object
 * receives EVAS_CALLBACK_MOUSE_IN event
 *
 * @since 1.7
 *
 * @ingroup Access
 *
 * @see elm_config_access_get()
 */
EAPI void             elm_config_access_set(Eina_Bool is_access);

/**
 * @}
 */

/**
 * @defgroup Selection Elementary Selection
 * @ingroup elm_infra_group
 *
 * @brief Elementary Selection configuration
 *
 * @{
 */

/**
 * @brief Get whether selection should be cleared when entry widget is unfocused.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return if the selection would be cleared on unfocus.
 *
 * @since 1.7
 *
 * @see elm_config_selection_unfocused_clear_set()
 */
EAPI Eina_Bool        elm_config_selection_unfocused_clear_get(void);

/**
 * @brief Set whether selection should be cleared when entry widget is unfocused.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] enabled If @c EINA_TRUE, clear selection when unfocus,
 * otherwise does not clear selection when unfocus.
 *
 * @since 1.7
 *
 * @see elm_config_selection_unfocused_clear_get()
 */
EAPI void             elm_config_selection_unfocused_clear_set(Eina_Bool enabled);

/**
 * @}
 */

/**
 * @brief Unset a font overlay for a given Elementary text class.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] text_class Text class name
 *
 * @ingroup Fonts
 *
 * @remark This will bring back text elements belonging to text class
 * @p text_class back to their default font settings.
 */
EAPI void             elm_config_font_overlay_unset(const char *text_class);

/**
 * @brief Apply the changes made with elm_config_font_overlay_set() and
 * elm_config_font_overlay_unset() on the current Elementary window.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Fonts
 *
 * @remark This applies all font overlays set to all objects in the UI.
 */
EAPI void             elm_config_font_overlay_apply(void);

/**
 * @internal
 *
 * Apply the specified font hinting type.
 * EVAS_FONT_HINTING_NONE < No font hinting
 * EVAS_FONT_HINTING_AUTO < Automatic font hinting
 * EVAS_FONT_HINTING_BYTECODE < Bytecode font hinting
 *
 * @ingroup Fonts
 *
 * This applies font hint changes to all windows of the current application.
 *
 * @since 1.13
 */
EAPI void elm_config_font_hint_type_set(int type);

/**
 * @brief Get the configured "finger size"
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The finger size
 *
 * @remark This gets the globally configured finger size, <b>in pixels</b>
 *
 * @ingroup Fingers
 */
EAPI Evas_Coord elm_config_finger_size_get(void);

/**
 * @brief Set the configured finger size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured finger size in pixels
 *
 * @param[in] size The finger size
 * @ingroup Fingers
 */
EAPI void       elm_config_finger_size_set(Evas_Coord size);

/**
 * @brief Get the configured cache flush interval time
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured cache flush interval time, in
 * ticks
 *
 * @return The cache flush interval time
 * @ingroup Caches
 *
 * @see elm_cache_all_flush()
 */
EAPI int       elm_config_cache_flush_interval_get(void);

/**
 * @brief Set the configured cache flush interval time
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured cache flush interval time, in ticks
 *
 * @param[in] size The cache flush interval time
 *
 * @remark The @p size must be greater than 0. if not, the cache flush will be
 *       ignored.
 *
 * @ingroup Caches
 *
 * @see elm_cache_all_flush()
 */
EAPI void      elm_config_cache_flush_interval_set(int size);

/**
 * @brief Get the configured cache flush enabled state
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured cache flush state - if it is enabled
 * or not. When cache flushing is enabled, elementary will regularly
 * (see elm_config_cache_flush_interval_get() ) flush caches and dump data out of
 * memory and allow usage to re-seed caches and data in memory where it
 * can do so. An idle application will thus minimize its memory usage as
 * data will be freed from memory and not be re-loaded as it is idle and
 * not rendering or doing anything graphically right now.
 *
 * @return The cache flush state
 * @ingroup Caches
 *
 * @see elm_cache_all_flush()
 */
EAPI Eina_Bool elm_config_cache_flush_enabled_get(void);

/**
 * @brief Set the configured cache flush enabled state
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured cache flush enabled state.
 *
 * @param[in] enabled The cache flush enabled state
 * @ingroup Caches
 *
 * @see elm_cache_all_flush()
 */
EAPI void      elm_config_cache_flush_enabled_set(Eina_Bool enabled);

/**
 * @brief Get the configured font cache size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured font cache size, in kilo bytes.
 *
 * @return The font cache size
 * @ingroup Caches
 */
EAPI int       elm_config_cache_font_cache_size_get(void);

/**
 * @brief Set the configured font cache size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured font cache size, in kilo bytes
 *
 * @param[in] size The font cache size
 * @ingroup Caches
 */
EAPI void      elm_config_cache_font_cache_size_set(int size);

/**
 * @brief Get the configured image cache size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured image cache size, in kilo bytes
 *
 * @return The image cache size
 * @ingroup Caches
 */
EAPI int       elm_config_cache_image_cache_size_get(void);

/**
 * @brief Set the configured image cache size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured image cache size, in kilo bytes
 *
 * @param[in] size The image cache size
 * @ingroup Caches
 */
EAPI void       elm_config_cache_image_cache_size_set(int size);

/**
 * @brief Get the configured edje file cache size.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured edje file cache size, in number
 * of files.
 *
 * @return The edje file cache size
 * @ingroup Caches
 */
EAPI int       elm_config_cache_edje_file_cache_size_get(void);

/**
 * @brief Set the configured edje file cache size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured edje file cache size, in number
 * of files.
 *
 * @param[in] size The edje file cache size
 * @ingroup Caches
 */
EAPI void       elm_config_cache_edje_file_cache_size_set(int size);

/**
 * @brief Get the configured edje collections (groups) cache size.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the globally configured edje collections cache size, in
 * number of collections.
 *
 * @return The edje collections cache size
 * @ingroup Caches
 */
EAPI int       elm_config_cache_edje_collection_cache_size_get(void);

/**
 * @brief Set the configured edje collections (groups) cache size
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the globally configured edje collections cache size, in
 * number of collections.
 *
 * @param[in] size The edje collections cache size
 * @ingroup Caches
 */
EAPI void       elm_config_cache_edje_collection_cache_size_set(int size);

/**
 * @internal
 *
 * Get the configured vsync flag
 *
 * This gets the globally configured vsync flag that asks some backend
 * engines to use vsync display if possible.
 *
 * @return If vsync is enabled
 * 
 * @since 1.11
 */
EAPI Eina_Bool  elm_config_vsync_get(void);

/**
 * @internal
 *
 * Set the configured vsync flag
 *
 * This sets the globally configured vsync flag that asks some backend
 * engines to use vsync display if possible.
 *
 * @param enabled This should be @c EINA_TRUE if enabled, or @c EINA_FALSE if
 * not.
 *
 * @since 1.11
 */
EAPI void       elm_config_vsync_set(Eina_Bool enabled);

/**
 * @brief Get the acceleration override preference flag
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets the acceleration override preference. This is a flag that
 * has the global system acceleration preference configuration forcibly
 * override whatever acceleration preference the application may want.
 *
 * @return If acceleration override is enabled
 *
 * @since 1.11
 *
 * @ingroup Engine
 */
EAPI Eina_Bool  elm_config_accel_preference_override_get(void);

/**
 * @brief Set the acceleration override preference flag
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This sets the acceleration override preference. This is a flag that
 * has the global system acceleration preference configuration forcibly
 * override whatever acceleration preference the application may want.
 *
 * @param[in] enabled This should be @c EINA_TRUE if enabled, or @c EINA_FALSE if
 * not.
 *
 * @see elm_config_accel_preference_set()
 *
 * @since 1.11
 *
 * @ingroup Engine
 */
EAPI void       elm_config_accel_preference_override_set(Eina_Bool enabled);

/**
 * @brief Get the enable status of the focus highlight
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark This gets whether the highlight on focused objects is enabled or not
 *
 * @return enable @c EINA_TRUE if the focus highlight is enabled, @c EINA_FALSE
 * otherwise.
 *
 * @see elm_config_focus_highlight_enabled_set()
 * @ingroup Focus
 */
EAPI Eina_Bool            elm_config_focus_highlight_enabled_get(void);

/**
 * @brief Set the enable status of the focus highlight
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] enable Enable highlight if @c EINA_TRUE, disable otherwise
 *
 * @remark Set whether to show or not the highlight on focused objects
 *
 * @remark Note that it will take effect only to Elementary windows created after
 * this is called.
 *
 * @see elm_config_focus_highlight_enabled_get()
 * @ingroup Focus
 */
EAPI void                 elm_config_focus_highlight_enabled_set(Eina_Bool enable);

/**
 * @brief Get the enable status of the focus highlight animation
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return animate @c EINA_TRUE if the focus highlight animation is enabled, @c
 * EINA_FALSE otherwise.
 *
 * @remark Get whether the focus highlight, if enabled, will animate its switch from
 * one object to the next
 *
 * @see elm_config_focus_highlight_animate_set()
 * @ingroup Focus
 */
EAPI Eina_Bool            elm_config_focus_highlight_animate_get(void);

/**
 * @brief Set the enable status of the highlight animation
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] animate Enable animation if @c EINA_TRUE, disable otherwise
 *
 * @remark Set whether the focus highlight, if enabled, will animate its switch from
 * one object to the next
 *
 * @remark Note that it will take effect only to Elementary windows created after
 * this is called.
 *
 * @see elm_config_focus_highlight_animate_get()
 * @ingroup Focus
 */
EAPI void                 elm_config_focus_highlight_animate_set(Eina_Bool animate);

/**
 * @internal
 *
 * Get the disable status of the focus highlight clip feature.
 *
 * @return The focus highlight clip disable status
 *
 * Get whether the focus highlight clip feature is disabled. If disabled return
 * @c EINA_TRUE, else return @c EINA_FALSE. If the return is @c EINA_TRUE, focus
 * highlight clip feature is not disabled so the focus highlight can be clipped.
 *
 * @see elm_config_focus_highlight_clip_disabled_set()
 * @since 1.10
 * @ingroup Focus
 */
EAPI Eina_Bool elm_config_focus_highlight_clip_disabled_get(void);

/**
 * @internal
 *
 * Set the disable status of the focus highlight clip feature.
 *
 * @param disable Disable focus highlight clip feature if @c EINA_TRUE, enable
 * it otherwise.
 *
 * @see elm_config_focus_highlight_clip_disabled_get()
 * @since 1.10
 * @ingroup Focus
 */
EAPI void elm_config_focus_highlight_clip_disabled_set(Eina_Bool disable);

/**
 * @internal
 *
 * Focus Movement Policy
 *
 * @since 1.10
 * @ingroup Focus
 */
typedef enum
{
   ELM_FOCUS_MOVE_POLICY_CLICK, /**< move focus by mouse click or touch. Elementary focus is set on mouse click and this is checked at mouse up time. (default) */
   ELM_FOCUS_MOVE_POLICY_IN, /**< move focus by mouse in. Elementary focus is set on mouse move when the mouse pointer is moved into an object. */
   ELM_FOCUS_MOVE_POLICY_KEY_ONLY /**< move focus by key. Elementary focus is set on key input like Left, Right, Up, Down, Tab, or Shift+Tab.*/
} Elm_Focus_Move_Policy;

/**
 * @internal
 *
 * Get the focus movement policy
 *
 * @return The focus movement policy
 *
 * Get how the focus is moved to another object. It can be
 * #ELM_FOCUS_MOVE_POLICY_CLICK or #ELM_FOCUS_MOVE_POLICY_IN. The first means
 * elementary focus is moved on elementary object click. The second means
 * elementary focus is moved on elementary object mouse in.
 *
 * @see elm_config_focus_move_policy_set()
 * @since 1.10
 * @ingroup Focus
 */
EAPI Elm_Focus_Move_Policy elm_config_focus_move_policy_get(void);

/**
 * @internal
 *
 * Set elementary focus movement policy
 *
 * @param policy A policy to apply for the focus movement
 *
 * @see elm_config_focus_move_policy_get()
 * @since 1.10
 * @ingroup Focus
 */
EAPI void elm_config_focus_move_policy_set(Elm_Focus_Move_Policy policy);

/**
 * @internal
 *
 * Get disable status of item select on focus feature.
 *
 * @return The item select on focus disable status
 *
 * @see elm_config_item_select_on_focus_disabled_set
 * @since 1.10
 * @ingroup Focus
 */
EAPI Eina_Bool elm_config_item_select_on_focus_disabled_get(void);

/**
 * @internal
 *
 * Set the disable status of the item select on focus feature.
 *
 * @param disabled Disable item select on focus if @c EINA_TRUE, enable otherwise
 *
 * @see elm_config_item_select_on_focus_disabled_get
 * @since 1.10
 * @ingroup Focus
 */
EAPI void elm_config_item_select_on_focus_disabled_set(Eina_Bool disabled);

/**
 * @internal
 *
 * Get status of first item focus on first focusin feature.
 *
 * @return The first item focus on first focusin status
 *
 * @see elm_config_first_item_focus_on_first_focusin_set
 * @since 1.11
 * @ingroup Focus
 */
EAPI Eina_Bool elm_config_first_item_focus_on_first_focusin_get(void);

/**
 * @internal
 *
 * Set the first item focus on first focusin feature.
 *
 * @param enabled first_item_focus_on_first_focusin if @c EINA_TRUE, enable otherwise
 *
 * @see elm_config_first_item_focus_on_first_focusin_get
 * @since 1.11
 * @ingroup Focus
 */
EAPI void elm_config_first_item_focus_on_first_focusin_set(Eina_Bool enabled);

/**
 * @brief Get the system mirrored mode. This determines the default mirrored mode
 * of widgets.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Mirroring
 *
 * @return @c EINA_TRUE if mirrored is set, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool elm_config_mirrored_get(void);

/**
 * @brief Set the system mirrored mode. This determines the default mirrored mode
 * of widgets.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Mirroring
 *
 * @param[in] mirrored @c EINA_TRUE to set mirrored mode, @c EINA_FALSE to unset it.
 */
EAPI void      elm_config_mirrored_set(Eina_Bool mirrored);

/**
 * @internal
 *
 * Get the clouseau state. @c EINA_TRUE if clouseau was tried to be run.
 *
 * @since 1.8
 * @return @c EINA_TRUE if clouseau was tried to run, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool elm_config_clouseau_enabled_get(void);

/**
 * @internal
 *
 * Get the clouseau state. @c EINA_TRUE if clouseau should be attempted to be run.
 *
 * @since 1.8
 * @param enabled @c EINA_TRUE to try and run clouseau, @c EINA_FALSE otherwise.
 */
EAPI void      elm_config_clouseau_enabled_set(Eina_Bool enabled);

/**
 * @brief Get the indicator service name according to the rotation degree.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Conformant
 *
 * @param[in] rotation The rotation which related with the indicator service name,
 * in degrees (0-360),
 *
 * @return The indicator service name according to the rotation degree. The
 * indicator service name can be either @c "elm_indicator_portrait" or
 * @c "elm_indicator_landscape".
 *
 * @remark Do not free the return string.
 */
EAPI const char *elm_config_indicator_service_get(int rotation);

/**
 * @brief Get the duration for occurring long tap event of gesture layer.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return Timeout for long tap event of gesture layer.
 * @ingroup Elm_Gesture_Layer
 * @since 1.8
 */
EAPI double   elm_config_glayer_long_tap_start_timeout_get(void);

/**
 * @brief Set the duration for occurring long tap event of gesture layer.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] long_tap_timeout Timeout for long tap event of gesture layer.
 * @ingroup Elm_Gesture_Layer
 * @since 1.8
 */
EAPI void   elm_config_glayer_long_tap_start_timeout_set(double long_tap_timeout);

/**
 * @brief Get the duration for occurring double tap event of gesture layer.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return Timeout for double tap event of gesture layer.
 * @ingroup Elm_Gesture_Layer
 * @since 1.8
 */
EAPI double   elm_config_glayer_double_tap_timeout_get(void);

/**
 * @brief Set the duration for occurring double tap event of gesture layer.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] double_tap_timeout Timeout for double tap event of gesture layer.
 * @ingroup Elm_Gesture_Layer
 * @since 1.8
 */
EAPI void   elm_config_glayer_double_tap_timeout_set(double double_tap_timeout);

/**
 * @}
 */

/**
 * @defgroup colors_group Elementary Colors
 * @ingroup elm_infra_group
 * @brief Configuration for Elementary colors.
 *
 * @{
 */

typedef struct _Elm_Color_Class
{
   const char *name;
   const char *desc;
} Elm_Color_Class;

typedef struct _Elm_Color_Overlay
{
   const char *color_class;
   struct {
      unsigned char r, g, b, a;
   } color, outline, shadow;
} Elm_Color_Overlay;

/**
 * @brief Get Elementary's list of supported color classes.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The color classes list, with @c Elm_Color_Class blobs as data.
 * @ingroup Colors
 * @since 1.10
 *
 * @remark Release the list with elm_color_classes_list_free().
 */
EAPI Eina_List *elm_config_color_classes_list_get(void);

/**
 * @brief Free Elementary's list of supported color classes.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Colors
 * @since 1.10
 *
 * @see elm_config_color_classes_list_get().
 */
EAPI void      elm_config_color_classes_list_free(Eina_List *list);

/**
 * @brief Get Elementary's list of color overlays, set with
 * elm_config_color_overlay_set().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @return The color overlays list, with @c Elm_Color_Overlay blobs as
 * data.
 *
 * @ingroup Colors
 * @since 1.10
 *
 * @remark For each color class, one can set a <b>color overlay</b> for it,
 * overriding the default color properties for that class coming from
 * the theme in use. There is no need to free this list.
 *
 * @see elm_config_color_overlay_set()
 * @see elm_config_color_overlay_unset().
 */
EAPI const Eina_List *elm_config_color_overlay_list_get(void);

/**
 * @brief Set a color overlay for a given Elementary color class.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] color_class Color class name
 * @param[in] r Object Red value
 * @param[in] g Object Green value
 * @param[in] b Object Blue value
 * @param[in] a Object Alpha value
 * @param[in] r2 Text outline Red value
 * @param[in] g2 Text outline Green value
 * @param[in] b2 Text outline Blue value
 * @param[in] a2 Text outline Alpha value
 * @param[in] r3 Text shadow Red value
 * @param[in] g3 Text shadow Green value
 * @param[in] b3 Text shadow Blue value
 * @param[in] a3 Text shadow Alpha value
 *
 * @ingroup Colors
 * @since 1.10

 * The first color is for the object itself, the second color is for the text
 * outline, and the third color is for the text shadow.
 *
 * @remark The second two color are only for texts.

 * Setting color emits a signal "color_class,set" with source being
 * the given color class in all edje objects.
 *
 * @see elm_config_color_overlay_list_get()
 * @see elm_config_color_overlay_unset()
 * @see edje_color_class_set()

 * @remark unlike Evas, Edje colors are @b not pre-multiplied. That is,
 *       half-transparent white is 255 255 255 128.
 */
EAPI void      elm_config_color_overlay_set(const char *color_class,
                                            int r, int g, int b, int a,
                                            int r2, int g2, int b2, int a2,
                                            int r3, int g3, int b3, int a3);

/**
 * @brief Unset a color overlay for a given Elementary color class.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] color_class Color class name
 *
 * @ingroup Colors
 * @since 1.10
 *
 * @remark This will bring back color elements belonging to color class
 * @p color_class back to their default color settings.
 */
EAPI void      elm_config_color_overlay_unset(const char *color_class);

/**
 * @brief Apply the changes made with elm_config_color_overlay_set() and
 * elm_config_color_overlay_unset() on the current Elementary window.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @ingroup Colors
 * @since 1.10
 *
 * @remark This applies all color overlays set to all objects in the UI.
 */
EAPI void      elm_config_color_overlay_apply(void);

/**
 * @}
 */

/**
 * @ingroup Magnifier
 *
 * @{
 */

/**
 * Get the magnifier enabled state for entries
 *
 * @return The enabled state for magnifier
 * @since 1.9
 */
EAPI Eina_Bool elm_config_magnifier_enable_get(void);

/**
 * Set the magnifier enabled state for entires
 *
 * @param enable The magnifier config state
 * @since 1.9
 */
EAPI void      elm_config_magnifier_enable_set(Eina_Bool enable);

/**
 * Get the amount of scaling the magnifer does
 *
 * @return The scaling amount (1.0 is none, 2.0 is twice as big etc.)
 * @since 1.9
 */
EAPI double    elm_config_magnifier_scale_get(void);

/**
 * Set the amount of scaling the magnifier does
 *
 * @param scale The scaling amount for magnifiers
 * @since 1.9
 */
EAPI void      elm_config_magnifier_scale_set(double scale);

/**
 * @}
 */

/**
 * @ingroup Audio
 *
 * @{
 */

/**
 * Get the mute state of an audio channel for effects
 *
 * @param channel The channel to get the mute state of
 * @return The mute state
 * @since 1.9
 */
EAPI Eina_Bool elm_config_audio_mute_get(Edje_Channel channel);

/**
 * Set the mute state of the specified channel
 *
 * @param channel The channel to set the mute state of
 * @param mute The mute state to set
 * @since 1.9
 */
EAPI void      elm_config_audio_mute_set(Edje_Channel channel, Eina_Bool mute);

/**
 * @}
 */

/**
 * @internal
 * @defgroup ATSPI AT-SPI2 Accessibility
 * @ingroup Elementary
 *
 * Elementary widgets support Linux Accessibility standard. For more
 * information please visit:
 * http://www.linuxfoundation.org/collaborate/workgroups/accessibility/atk/at-spi/at-spi_on_d-bus
 *
 * @{
 */

/**
 * Gets ATSPI mode
 *
 * @return the ATSPI mode
 *
 * @since 1.10
 *
 * @ingroup ATSPI
 *
 * @see elm_config_atspi_mode_set()
 */
EAPI Eina_Bool        elm_config_atspi_mode_get(void);

/**
 * Sets ATSPI mode
 *
 * @param is_atspi If @c EINA_TRUE, enables ATSPI2 mode
 *
 * @note Enables Linux Accessibility support for Elementary widgets.
 *
 * @since 1.10
 *
 * @ingroup ATSPI
 *
 * @see elm_config_atspi_mode_get()
 */
EAPI void             elm_config_atspi_mode_set(Eina_Bool is_atspi);

/**
 * @}
 */

/**
 * @internal
 * @brief Set the transition duration factor
 *
 * This function sets the edje transition duration factor
 * It will affect the duration of edje transitions
 *
 * @param factor The duration factor for transition in edje
 *
 * @note This value affect duration of transitions in edje
 *
 * @since 1.15
 *
 * @see edje_transition_duration_set() for more details
 */
EAPI void elm_config_transition_duration_factor_set(double factor);

/**
 * @internal
 * @brief Get the duration factor of transitions
 *
 * @return The duration factor of transition in edje
 *
 * @since 1.15
 */
EAPI double elm_config_transition_duration_factor_get(void);

/**
 * @}
 */

EAPI Evas_Coord elm_finger_size_get(void);
EAPI void       elm_finger_size_set(Evas_Coord size);
