/**
 * @addtogroup Map
 *
 * @{
 */

/**
 * Set map's zoom behavior. It can be set to manual or automatic.
 *
 * Default value is #ELM_MAP_ZOOM_MODE_MANUAL.
 *
 * Values <b> don't </b> work as bitmask, only one can be chosen.
 *
 * @note Valid sizes are 2^zoom, consequently the map may be smaller
 * than the scroller view.
 *
 * @see elm_map_zoom_mode_set()
 * @see elm_map_zoom_mode_get()
 */
typedef enum
{
   ELM_MAP_ZOOM_MODE_MANUAL,      /**< Zoom controlled manually by elm_map_zoom_set(). It's set by default. */
   ELM_MAP_ZOOM_MODE_AUTO_FIT,    /**< Zoom until map fits inside the scroll frame with no pixels outside this area. */
   ELM_MAP_ZOOM_MODE_AUTO_FILL,   /**< Zoom until map fills scroll, ensuring no pixels are left unfilled. */
   ELM_MAP_ZOOM_MODE_LAST
} Elm_Map_Zoom_Mode;

/**
 * Set type of a external source (provider).
 *
 * @see elm_map_sources_get()
 * @see elm_map_source_get()
 * @see elm_map_source_set()
 */
typedef enum
{
   ELM_MAP_SOURCE_TYPE_TILE,   /**< Map tile provider. */
   ELM_MAP_SOURCE_TYPE_ROUTE,  /**< Route service provider. */
   ELM_MAP_SOURCE_TYPE_NAME,   /**< Name service provider. */
   ELM_MAP_SOURCE_TYPE_LAST
} Elm_Map_Source_Type;

/**
 * Set type of transport used on route.
 *
 * @see elm_map_route_add()
 */
typedef enum
{
   ELM_MAP_ROUTE_TYPE_MOTOCAR,   /**< Route should consider an automobile will be used. */
   ELM_MAP_ROUTE_TYPE_BICYCLE,   /**< Route should consider a bicycle will be used by the user. */
   ELM_MAP_ROUTE_TYPE_FOOT,      /**< Route should consider user will be walking. */
   ELM_MAP_ROUTE_TYPE_LAST
} Elm_Map_Route_Type;

/**
 * Set the routing method, what should be prioritized, time or distance.
 *
 * @see elm_map_route_add()
 */
typedef enum
{
   ELM_MAP_ROUTE_METHOD_FASTEST,  /**< Route should prioritize time. */
   ELM_MAP_ROUTE_METHOD_SHORTEST, /**< Route should prioritize distance. */
   ELM_MAP_ROUTE_METHOD_LAST
} Elm_Map_Route_Method;

/**
 * Set the name search method.
 *
 * This is for name module interface.
 */
typedef enum
{
   ELM_MAP_NAME_METHOD_SEARCH,
   ELM_MAP_NAME_METHOD_REVERSE,
   ELM_MAP_NAME_METHOD_LAST
} Elm_Map_Name_Method;

/**
 * Set overlay type to be used. This type is resolved
 * when the overlay is created.
 * You can get this value by elm_map_overlay_type_get().
 *
 * @see elm_map_overlay_type_get()
 * @see elm_map_overlay_add()
 * @see elm_map_overlay_class_add()
 * @see elm_map_overlay_bubble_add()
 */
typedef enum
{
   ELM_MAP_OVERLAY_TYPE_NONE = 0,
   ELM_MAP_OVERLAY_TYPE_DEFAULT,
   ELM_MAP_OVERLAY_TYPE_CLASS,
   ELM_MAP_OVERLAY_TYPE_GROUP,
   ELM_MAP_OVERLAY_TYPE_BUBBLE,
   ELM_MAP_OVERLAY_TYPE_ROUTE,
   ELM_MAP_OVERLAY_TYPE_LINE,
   ELM_MAP_OVERLAY_TYPE_POLYGON,
   ELM_MAP_OVERLAY_TYPE_CIRCLE,
   ELM_MAP_OVERLAY_TYPE_SCALE,
   ELM_MAP_OVERLAY_TYPE_POLYLINE
} Elm_Map_Overlay_Type;

typedef struct _Elm_Map_Marker       Elm_Map_Marker;       /**< A marker to be shown in a specific point of the map. Can be created with elm_map_marker_add() and deleted with elm_map_marker_remove(). */
typedef struct _Elm_Map_Marker_Class Elm_Map_Marker_Class; /**< Each marker must be associated to a class. It's required to add a mark. The class defines the style of the marker when a marker is displayed alone (not grouped). A new class can be created with elm_map_marker_class_new(). */
typedef struct _Elm_Map_Group_Class  Elm_Map_Group_Class;  /**< Each marker must be associated to a group class. It's required to add a mark. The group class defines the style of the marker when a marker is grouped to other markers. Markers with the same group are grouped if they are close. A new group class can be created with elm_map_marker_group_class_new(). */
typedef struct _Elm_Map_Route        Elm_Map_Route;        /**< A route to be shown in the map. Can be created with elm_map_route_add() and deleted with elm_map_route_del(). */
typedef struct _Elm_Map_Name         Elm_Map_Name;         /**< A handle for specific coordinates. */
typedef struct _Elm_Map_Overlay      Elm_Map_Overlay;      /**< A overlay to be shown in a specific point of the map. This can be created by elm_map_overlay_add() and similar functions and deleted by elm_map_overlay_del(). */
typedef struct _Elm_Map_Region       Elm_Map_Region;       /**< A geographical point specified by latitude, longitude and altitude values.*/

typedef Evas_Object               *(*Elm_Map_Marker_Get_Func)(Evas_Object *obj, Elm_Map_Marker *marker, void *data); /**< Bubble content fetching class function for marker classes. When the user click on a marker, a bubble is displayed with a content. */
typedef void                       (*Elm_Map_Marker_Del_Func)(Evas_Object *obj, Elm_Map_Marker *marker, void *data, Evas_Object *o); /**< Function to delete bubble content for marker classes. */
typedef Evas_Object               *(*Elm_Map_Marker_Icon_Get_Func)(Evas_Object *obj, Elm_Map_Marker *marker, void *data); /**< Icon fetching class function for marker classes. */
typedef Evas_Object               *(*Elm_Map_Group_Icon_Get_Func)(Evas_Object *obj, void *data); /**< Icon fetching class function for markers group classes. */

typedef void                       (*Elm_Map_Overlay_Get_Cb)(void *data, Evas_Object *map, Elm_Map_Overlay *overlay);   /**< Get callback function for the overlay. */
typedef void                       (*Elm_Map_Overlay_Del_Cb)(void *data, Evas_Object *map, Elm_Map_Overlay *overlay);   /**< Det callback function for the overlay. @since 1.7 */
typedef void                       (*Elm_Map_Name_Cb)(void *data, Evas_Object *map, Elm_Map_Name *name);                /**< Async-callback function for the name request. */
typedef void                       (*Elm_Map_Name_List_Cb)(void *data, Evas_Object *map, Eina_List *name_list);                /**< Async-callback function for the name list request. */
typedef void                       (*Elm_Map_Route_Cb)(void *data, Evas_Object *map, Elm_Map_Route *route);             /**< Async-callback function for the route request. */
typedef void                       (*Elm_Map_Capture_Result_Cb)(Evas_Object*, void*);/** Async ofscreen map image capture function */

/**
 * @brief Delete a overlay from the map. This function can delete all types
 * of overlays.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be deleted.
 *
 * @see elm_map_overlay_add()
 * @see elm_map_overlay_class_add()
 * @see elm_map_overlay_bubble_add()
 */
EAPI void                  elm_map_overlay_del(Elm_Map_Overlay *overlay);

/**
 * @brief Get the overlay type.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return type.
 * @return Return the overlay type.
 *
 * @remark This type is resolved when the overlay is created.
 *
 * @see elm_map_overlay_add()
 * @see elm_map_overlay_class_add()
 * @see elm_map_overlay_bubble_add()
 */
EAPI Elm_Map_Overlay_Type  elm_map_overlay_type_get(const Elm_Map_Overlay *overlay);

 /**
 * Set a pointer of user data for a overlay.
 *
 * @param overlay The overlay to own the user data.
 * @param data A pointer of user data
 *
 * @see elm_map_overlay_data_get()
 */
EAPI void                  elm_map_overlay_data_set(Elm_Map_Overlay *overlay, void *data);

/**
 * @brief Get the user data stored on a overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return the user data.
 * @return A pointer to data stored using elm_map_overlay_data_set(),
 *         or @c NULL, if none has been set.
 *
 * @see elm_map_overlay_data_set()
 */
EAPI void *                elm_map_overlay_data_get(const Elm_Map_Overlay *overlay);

/**
 * @brief Set if the overlay is hidden or not.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be hidden.
 * @param[in] hide Use @c EINA_TRUE to hide the overlay or @c EINA_FALSE to show.
 *
 * @see elm_map_overlay_hide_get()
 */
EAPI void                  elm_map_overlay_hide_set(Elm_Map_Overlay *overlay, Eina_Bool hide);

/**
 * @brief Get a value whether the overlay is hidden or not.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return the hidden state.
 * @return @c EINA_TRUE means the overlay is hidden. @c EINA_FALSE indicates
 * it is not.
 *
 * @remark This gets the current hidden state for the overlay.
 *
 * @see elm_map_overlay_hide_set()
 */
EAPI Eina_Bool             elm_map_overlay_hide_get(const Elm_Map_Overlay *overlay);

/**
 * @brief Set the minimum zoom from where the overlay is displayed.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be set the minimum zoom.
 * @param[in] zoom The minimum zoom.
 *
 * @remark The overlay only will be displayed when the map is displayed at @p zoom
 * or bigger.
 *
 * @see elm_map_overlay_displayed_zoom_min_get()
 */
EAPI void                  elm_map_overlay_displayed_zoom_min_set(Elm_Map_Overlay *overlay, int zoom);

/**
 * @brief Get the minimum zoom from where the overlay is displayed.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return the minimum zoom.
 * @return zoom The minimum zoom.
 *
 * @see elm_map_overlay_displayed_zoom_min_set()
 */
EAPI int                   elm_map_overlay_displayed_zoom_min_get(const Elm_Map_Overlay *overlay);

/**
 * @brief Pause or unpause the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be paused.
 * @param[in] paused Use @c EINA_TRUE to pause the @p overlay or @c EINA_FALSE
 * to unpause it.
 *
 * @remark This sets the paused state to on (@c EINA_TRUE) or off (@c EINA_FALSE)
 * for the overlay.
 *
 * @remark The default is off.
 *
 * @remark This will stop moving the overlay coordinates instantly.
 * even if map being scrolled or zoomed.
 *
 * @see elm_map_overlay_paused_get()
 */
EAPI void                  elm_map_overlay_paused_set(Elm_Map_Overlay *overlay, Eina_Bool paused);

/**
 * @brief Get a value whether the overlay is paused or not.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return paused state.
 * @return @c EINA_TRUE means overlay is paused. @c EINA_FALSE indicates
 * it is not.
 *
 * @remark This gets the current paused state for the overlay.
 *
 * @see elm_map_overlay_paused_set()
 */
EAPI Eina_Bool             elm_map_overlay_paused_get(const Elm_Map_Overlay *overlay);

/**
 * @brief Get a value whether the overlay is visible or not.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return visible state.
 * @return @c EINA_TRUE means overlay is visible. @c EINA_FALSE indicates
 * it is not.
 *
 * @remark The visible of the overlay can not be set.
 * This value can be changed dynamically while zooming and panning
 *
 * @since 1.7
 */
EAPI Eina_Bool             elm_map_overlay_visible_get(const Elm_Map_Overlay *overlay);

/**
 * @brief Set the content object of the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be set the content.
 * @param[in] obj The evas object will be used to display the overlay.
 *
 * @remark Only default and class type overlay support this function.
 *
 * @remark The content should be resized or set size hints before set to the overlay.
 * <b> Do not modify this object</b> (move, show, hide, del, etc.),
 * after set.
 * You can only resize this.
 *
 * @remark This content is what will be inside the overlay that will be displayed.
 * If a content is set, icon and default style layout are no more used before
 * the content is deleted.
 *
 * @remark If @p obj is @c NULL, content inside the overlay is deleted.
 *
 * @see elm_map_overlay_content_get()
 */
EAPI void                  elm_map_overlay_content_set(Elm_Map_Overlay *overlay, Evas_Object *obj);

/**
 * @brief Get the content object.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return the content.
 * @return Return the evas object if it exists, else @c NULL.
 *
 * @remark Only default and class type overlay support this function.
 *
 * @remark Returned content is what being inside the overlay that being displayed.
 *
 * <b> Do not modify this object</b> (move, show, hide, del, etc.).
 * @remark You can only resize this.
 *
 * @remark The content can be set by elm_map_overlay_content_set().
 *
 * @see elm_map_overlay_content_set()
 */
EAPI const Evas_Object *   elm_map_overlay_content_get(const Elm_Map_Overlay *overlay);

/**
 * @brief Set a icon of the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be set the icon.
 * @param[in] icon The icon will be used to display the overlay.
 *
 * @remark Only default and class type overlay support this function.
 *
 * <b> Do not modify this object</b> (move, show, hide, resize, del, etc.),
 * @remark after set.
 *
 * @remark If icon is set, default style layout will not be used.
 *
 * @remark If @p icon is @c NULL, icon inside the overlay will be deleted.
 *
 * @see elm_map_overlay_icon_get()
 */
EAPI void                  elm_map_overlay_icon_set(Elm_Map_Overlay *overlay, Evas_Object *icon);

/**
 * @brief Get the icon object.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return the icon.
 * @return Return the icon object if it exists, else @c NULL.
 *
 * @remark Only default and class type overlay support this function.
 *
 * @remark Returned icon is what being inside the overlay that being displayed.
 *
 * <b> Do not modify this icon </b> (move, show, hide, resize, del, etc.).
 *
 * @remark The icon can be set by elm_map_overlay_icon_set().
 *
 * @see elm_map_overlay_icon_set()
 */
EAPI const Evas_Object *   elm_map_overlay_icon_get(const Elm_Map_Overlay *overlay);

/**
 * @brief Set the geographic coordinates of the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be set geographic coordinates.
 * @param[in] lon Longitude to be set.
 * @param[in] lat Latitude to be set.
 *
 * @remark Only default and bubble type overlay support this function.
 *
 * @remark This sets the center coordinates of the overlay. It can be
 * get by elm_map_overlay_region_get().
 *
 * @see elm_map_overlay_region_get()
 */
EAPI void                  elm_map_overlay_region_set(Elm_Map_Overlay *overlay, double lon, double lat);

/**
 * @brief Get the geographic coordinates of the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return geographic coordinates.
 * @param[out] lon Pointer to store longitude.
 * @param[out] lat Pointer to store latitude.
 *
 * @remark Only default and bubble type overlay support this function.
 *
 * @remark This returns the center coordinates of the overlay. It can be
 * set by elm_map_overlay_region_set().
 *
 * @see elm_map_overlay_region_set()
 */
EAPI void                  elm_map_overlay_region_get(const Elm_Map_Overlay *overlay, double *lon, double *lat);


/**
 * @brief Set the object color of the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be set color.
 * @param[in] r Red channel value, from 0 to 255.
 * @param[in] g Green channel value, from 0 to 255.
 * @param[in] b Blue channel value, from 0 to 255.
 * @param[in] a Alpha channel value, from 0 to 255.
 *
 * @remark It uses an additive color model, so each color channel represents
 * how much of each primary colors must to be used. 0 represents
 * absence of this color, so if all of the three are set to 0,
 * the color will be black.
 *
 * @remark These component values should be integers in the range 0 to 255,
 * (single 8-bit byte).
 *
 * @remark This sets the color used for the overlay. By default, it is set to
 * solid red (r = 255, g = 0, b = 0, a = 255).
 *
 * @remark For alpha channel, 0 represents completely transparent, and 255, opaque.
 *
 * @remark Function supports only ELM_MAP_OVERLAY_TYPE_CLASS, ELM_MAP_OVERLAY_TYPE_DEFAULT
 * and ELM_MAP_OVERLAY_TYPE_ROUTE Elm_Map_Overlay_Type types.
 *
 * @see elm_map_overlay_color_get()
 */
EAPI void                  elm_map_overlay_color_set(Elm_Map_Overlay *overlay, int r, int g, int b, int a);

/**
 * @brief Get the object color of the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to return color.
 * @param[out] r Pointer to store the red channel value.
 * @param[out] g Pointer to store the green channel value.
 * @param[out] b Pointer to store the blue channel value.
 * @param[out] a Pointer to store the alpha channel value.
 *
 * @see elm_map_overlay_color_set()
 */
EAPI void                  elm_map_overlay_color_get(const Elm_Map_Overlay *overlay, int *r, int *g, int *b, int *a);

/**
 * @brief Show the given overlay at the center of the map, immediately.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to be center at.
 *
 * @remark This causes map to @b redraw its viewport's contents to the
 * region containing the given @p overlay's coordinates, that will be
 * moved to the center of the map.
 *
 * @see elm_map_overlays_show() if more than one overlay need to be displayed.
 */
EAPI void                  elm_map_overlay_show(Elm_Map_Overlay *overlay);

/**
 * @brief Move and zoom the map to display a list of overlays.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlays A list of #Elm_Map_Overlay handles.
 *
 * @remark The map will be centered on the center point of the overlays in the list.
 * Then the map will be zoomed in order to fit the overlays using the maximum
 * zoom which allows display of all the overlays.
 *
 * @warning All the overlays should belong to the same map object.
 *
 * @see elm_map_overlay_show() to show a single overlay.
 */
EAPI void                  elm_map_overlays_show(Eina_List *overlays);

/**
 * @brief Set the get callback function of the overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to own the get callback function.
 * @param[in] get_cb The callback function.
 * @param[in] data The user callback data.
 *
 * @remark If the overlay is clicked, the callback wll be called.
 * The clicked overlay is returned by callback.
 *
 * @remark You can add callback to the class overlay. If one of the group overlays in this class
 * is clicked, callback will be called and return a virtual group overlays.
 *
 * @remark You can delete this callback function by setting @c NULL.
 *
 */
EAPI void                  elm_map_overlay_get_cb_set(Elm_Map_Overlay *overlay, Elm_Map_Overlay_Get_Cb get_cb, void *data);

/**
 * @brief Set the del callback function to call when the overlay is deleted.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The overlay to own the del callback function.
 * @param[in] del_cb The callback function.
 * @param[in] data The user callback data.
 *
 * @remark If the overlay is deleted, the callback wll be called.
 * The deleted overlay is returned by callback.
 *
 * @remark You can delete this callback function by setting @c NULL.
 *
 * @since 1.7
 */
EAPI void                  elm_map_overlay_del_cb_set(Elm_Map_Overlay *overlay, Elm_Map_Overlay_Del_Cb del_cb, void *data);

/**
 * @brief Add a new overlay member to the class overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] clas The class overlay to add a new overlay.
 * @param[in] overlay The overlay to be added to the class overlay.
 *
 * @see elm_map_overlay_class_remove()
 */
EAPI void                  elm_map_overlay_class_append(Elm_Map_Overlay *clas, Elm_Map_Overlay *overlay);

/**
 * @brief Remove a overlay from the class.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] clas The class overlay to delete the overlay.
 * @param[in] overlay The overlay to be deleted from the class overlay.
 *
 * @see elm_map_overlay_class_append()
 */
EAPI void                  elm_map_overlay_class_remove(Elm_Map_Overlay *clas, Elm_Map_Overlay *overlay);

/**
 * @brief Set the maximum zoom from where the overlay members in the class can be
 * grouped.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] clas The overlay class has overlay members.
 * @param[in] zoom The maximum zoom.
 *
 * @remark Overlay members in the class only will be grouped when the map
 * is displayed at less than @p zoom.
 *
 * @see elm_map_overlay_class_zoom_max_get()
 */
EAPI void                  elm_map_overlay_class_zoom_max_set(Elm_Map_Overlay *clas, int zoom);

/**
 * @brief Get the maximum zoom from where the overlay members in the class can be
 * grouped.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] clas The overlay class has overlay members.
 *
 * @return The maximum zoom.
 *
 * @see elm_map_overlay_class_zoom_max_set()
 */
EAPI int                   elm_map_overlay_class_zoom_max_get(const Elm_Map_Overlay *clas);

/**
 * Get the overlay members of the group overlay.
 *
 * @param grp The group overlay has overlay members.
 *
 * @return The list of group overlay members.
 *
 * The group overlays are virtualy overlays. Those are shown and hidden dynamically.
 * You can add callback to the class overlay. If one of the group overlays in this class
 * is clicked, callback will be called and return a virtual group overlays.
 *
 * You can change the state (hidden, paused, etc.) or set the content
 * or icon of the group overlays by chaning the state of the class overlay.
 * Do not modifty the group overlay itself.
 *
 * @see elm_map_overlay_class_add()
 */
EAPI Eina_List *           elm_map_overlay_group_members_get(const Elm_Map_Overlay *grp);

/**
 * @brief Follow a other overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] bubble The bubble overlay to follow a parent overlay.
 * @param[in] parent The parent overlay to be followed by the bubble overlay.
 *
 * @remark Bubble overlay will follow the parent overlay's movement (hide, show, move).
 *
 * @see elm_map_overlay_bubble_add()
 */
EAPI void                  elm_map_overlay_bubble_follow(Elm_Map_Overlay *bubble, const Elm_Map_Overlay *parent);

/**
 * @brief Add a content object to the bubble overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] bubble The bubble overlay to add a content.
 * @param[in] content The content to be added to the bubble overlay.
 *
 * @remark Added contents will be displayed inside the bubble overlay.
 *
 * @see elm_map_overlay_bubble_content_clear()
 */
EAPI void                  elm_map_overlay_bubble_content_append(Elm_Map_Overlay *bubble, Evas_Object *content);

/**
 * @brief Clear all contents inside the bubble overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] bubble The bubble overlay to clear the contents.
 *
 * @remark This will delete all contents inside the bubble overlay.
 *
 * @see elm_map_overlay_bubble_content_append()
 */
EAPI void                  elm_map_overlay_bubble_content_clear(Elm_Map_Overlay *bubble);

/**
 * @brief Add a  geographic coordinates  to the polygon overlay.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] overlay The polygon overlay to get a region.
 * @param[in] lon The longitude.
 * @param[in] lat The latitude.
 *
 * @remark At least 3 regions should be added to show the polygon overlay.
 *
 * @remark Overlay created with this method can be deleted with elm_map_overlay_del().
 *
 * @see elm_map_overlay_polygon_add()
 * @see elm_map_overlay_del()
 */
EAPI void                  elm_map_overlay_polygon_region_add(Elm_Map_Overlay *overlay, double lon, double lat);

/**
 * Remove a route from the map.
 *
 * @param route The route to remove.
 *
 * @see elm_map_route_add()
 */
EAPI void                  elm_map_route_del(Elm_Map_Route *route);

/**
 * Get the route distance in kilometers.
 *
 * @param route The route object.
 * @return The distance of route (unit : km).
 */
EAPI double                elm_map_route_distance_get(const Elm_Map_Route *route);

/**
 * Get the information of route nodes.
 *
 * @param route The route object.
 * @return Returns a string with the nodes of route.
 */
EAPI const char           *elm_map_route_node_get(const Elm_Map_Route *route);

/**
 * Get the information of route waypoint.
 *
 * @param route the route object.
 * @return Returns a string with information about waypoint of route.
 */
EAPI const char           *elm_map_route_waypoint_get(const Elm_Map_Route *route);

/**
 * @brief Get the address of the name.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] name The name handle.
 * @return Returns the address string of @p name.
 *
 * @remark This gets the coordinates of the @p name, created with one of the
 * conversion functions.
 *
 * @see elm_map_name_add()
 */
EAPI const char           *elm_map_name_address_get(const Elm_Map_Name *name);

/**
 * @brief Get the current coordinates of the name.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] name The name handle.
 * @param[out] lat Pointer to store the latitude.
 * @param[out] lon Pointer to store The longitude.
 *
 * @remark This gets the coordinates of the @p name, created with one of the
 * conversion functions.
 *
 * @see elm_map_name_add()
 */
EAPI void                  elm_map_name_region_get(const Elm_Map_Name *name, double *lon, double *lat);

/**
 * @brief Remove a name from the map.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] name The name to remove.
 *
 * @remark Basically the struct handled by @p name will be freed, so conversions
 * between address and coordinates will be lost.
 *
 * @see elm_map_name_add()
 */
EAPI void                  elm_map_name_del(Elm_Map_Name *name);

/**
 * @internal
 * @remarks Tizen only feature
 *
 * @brief Add a geographic coordinates to the polyline overlay.
 *
 * @remarks At least 2 regions should be added to show the polyline overlay.
 *
 * @remarks Overlay created with this method can be deleted with elm_map_overlay_del().
 *
 * @param overlay The polyline overlay to add geographic coordinates
 * @param lon The longitude.
 * @param lat The latitude.
 *
 * @see elm_map_overlay_polyline_add()
 * @see elm_map_overlay_del()
 */
EAPI void                             elm_map_overlay_polyline_region_add(Elm_Map_Overlay *overlay, double lon, double lat);

/**
 * @internal
 * @remarks Tizen only feature
 *
 * @brief Sets the width of the polyline overlay.
 *
 * @param overlay The polyline overlay to set a width.
 * @param width The line width in pixels
 *
 * @see elm_map_overlay_polyline_width_get()
 */
EAPI void                             elm_map_overlay_polyline_width_set(Elm_Map_Overlay *overlay, int width);

/**
 * @internal
 * @remarks Tizen only feature
 *
 * @brief Gets the width of the polyline overlay.
 *
 * @param overlay The polyline overlay to get a width.
 * @return The line width in pixels
 *
 * @see elm_map_overlay_polyline_width_set()
 */
EAPI int                 elm_map_overlay_polyline_width_get(Elm_Map_Overlay *overlay);
/**
 * @}
 */
