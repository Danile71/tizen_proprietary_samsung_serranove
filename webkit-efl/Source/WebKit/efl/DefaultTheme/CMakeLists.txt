SET(DefaultTheme_RESOURCE_NAME default.edj CACHE STRING "default theme name")
SET(DefaultTheme_DIR "${WEBKIT_DIR}/efl/DefaultTheme")

SET(DefaultTheme_DEFINITION "")
IF (ENABLE_PROGRESS_ELEMENT)
    LIST(APPEND DefaultTheme_DEFINITION "-DENABLE_PROGRESS_ELEMENT")
ENDIF ()

SET(TizenDependencies_DEFINITION "")
SET(DefaultDependencies_DEFINITION "")
IF (ENABLE_TIZEN_MEDIA_PLAYER)
    LIST(APPEND DefaultTheme_DEFINITION "-DENABLE_TIZEN_MEDIA_PLAYER")
    LIST(APPEND TizenDependencies_DEFINITION
         "widget/mediacontrol/playpausebutton/playpause_button_tizen.edc"
         "widget/mediacontrol/playpausebutton/Pausebutton_focus_tizen.png"
         "widget/mediacontrol/playpausebutton/Pausebutton_press_tizen.png"
         "widget/mediacontrol/playpausebutton/Playbutton_focus_tizen.png"
         "widget/mediacontrol/playpausebutton/Playbutton_press_tizen.png"
         "widget/mediacontrol/playpausebutton/Pausebutton_focus_fullscreen_tizen.png"
         "widget/mediacontrol/playpausebutton/Pausebutton_press_fullscreen_tizen.png"
         "widget/mediacontrol/playpausebutton/Playbutton_focus_fullscreen_tizen.png"
         "widget/mediacontrol/playpausebutton/Playbutton_press_fullscreen_tizen.png"
         "widget/mediacontrol/seekbackwardbutton/seekbackward_button_tizen.edc"
         "widget/mediacontrol/seekbackwardbutton/seekbackwardbutton_default_tizen.png"
         "widget/mediacontrol/seekbackwardbutton/seekbackwardbutton_press_tizen.png"
         "widget/mediacontrol/seekforwardbutton/seekforward_button_tizen.edc"
         "widget/mediacontrol/seekforwardbutton/seekforwardbutton_default_tizen.png"
         "widget/mediacontrol/seekforwardbutton/seekforwardbutton_press_tizen.png"
         "widget/mediacontrol/rotatebutton/rotate_button_press_tizen.png"
         "widget/mediacontrol/rotatebutton/rotate_button_tizen.edc"
         "widget/mediacontrol/rotatebutton/rotate_button_tizen.png")
ELSE()
    LIST(APPEND DefaultDependencies_DEFINITION
         "widget/mediacontrol/playpausebutton/playpause_button.edc"
         "widget/mediacontrol/playpausebutton/pausebutton.png"
         "widget/mediacontrol/playpausebutton/playbutton.png"
         "widget/mediacontrol/seekbackwardbutton/seekbackward_button.edc"
         "widget/mediacontrol/seekbackwardbutton/seekbackwardbutton.png"
         "widget/mediacontrol/seekforwardbutton/seekforward_button.edc"
         "widget/mediacontrol/seekforwardbutton/seekforwardbutton.png"
         "widget/mediacontrol/rotatebutton/rotate_button.edc"
         "widget/mediacontrol/rotatebutton/rotate_button.png")
ENDIF()

ADD_CUSTOM_COMMAND(
    OUTPUT ${THEME_BINARY_DIR}/${DefaultTheme_RESOURCE_NAME}
    COMMAND ${EDJE_CC_EXECUTABLE} -v ${DefaultTheme_DEFINITION} default.edc ${THEME_BINARY_DIR}/${DefaultTheme_RESOURCE_NAME}
    DEPENDS
        default.edc
        ${TizenDependencies_DEFINITION}
        ${DefaultDependencies_DEFINITION}
        widget/button/button.edc
        widget/button/00_button_01.png
        widget/button/00_button_01_dim.png
        widget/button/00_button_01_focus.png
        widget/check/00_check_activated.png
        widget/check/00_check_activated_dim.png
        widget/check/00_check_bg.png
        widget/check/00_check_dim_bg.png
        widget/check/00_check_focus.png
        widget/check/check.edc
        widget/combo/combo.edc
        widget/combo/00_combo_focus_button.png
        widget/combo/00_combo_focus.png
        widget/combo/00_combo_normal_button.png
        widget/combo/00_combo_normal.png
        widget/combo/00_combo_press_button.png
        widget/combo/00_combo_press.png
        widget/combo/00_combo_border.png
        widget/combo/00_combo_focus_border.png
        widget/combo/00_icon.png
        widget/entry/entry.edc
        widget/entry/00_focused.png
        widget/entry/00_normal.png
        widget/mediacontrol/fullscreenbutton/fullscreen_button.edc
        widget/mediacontrol/mutebutton/mute_button.edc
        widget/mediacontrol/mutebutton/mutebutton.png
        widget/mediacontrol/mutebutton/unmutebutton.png
        widget/mediacontrol/overlayloadingspinner/loading_spinner.edc
        widget/mediacontrol/overlayloadingspinner/loading_spinner.png
        widget/mediacontrol/sliderthumb/slider_thumb.edc
        widget/mediacontrol/sliderthumb/sliderthumb.png
        widget/mediacontrol/sliderthumb/sliderthumb_press.png
        widget/mediacontrol/defaultimage/default_image.edc
        widget/mediacontrol/defaultimage/defaultimage.png
        widget/mediacontrol/controlpanel/media_control_panel_background.edc
        widget/mediacontrol/controlpanel/Control_Panel_BackGround_Vertical.png
        widget/mediacontrol/controlpanel/Control_Panel_BackGround_Horizontal.png
        widget/progressbar/bt_base.png
        widget/progressbar/progressbar.edc
        widget/progressbar/shelf_inset.png
        widget/radio/00_button_radio_activated.png
        widget/radio/00_button_radio_activated_dim.png
        widget/radio/00_button_radio_bg.png
        widget/radio/00_button_radio_dim_bg.png
        widget/radio/00_button_radio_focus.png
        widget/radio/radio.edc
        widget/scrollbar/scrollbar.edc
        widget/scrollbar/scrollbar_h.png
        widget/scrollbar/scrollbar_knob_h.png
        widget/scrollbar/scrollbar_knob_v.png
        widget/scrollbar/scrollbar_v.png
        widget/search/cancel/cancel_normal_button2.png
        widget/search/cancel/cancel_normal_button.png
        widget/search/cancel/search_cancel.edc
        widget/search/decoration/decoration_normal_button.png
        widget/search/decoration/search_decoration.edc
        widget/search/field/field_focused.png
        widget/search/field/field_normal.png
        widget/search/field/search_field.edc
        widget/slider/slider.edc
        widget/slider/slider_fill_h.png
        widget/slider/slider_fill_v.png
        widget/slider/slider_h.png
        widget/spinner/sp_down_default.png
        widget/spinner/sp_down_pressed.png
        widget/spinner/spinner.edc
        widget/spinner/sp_up_default.png
        widget/spinner/sp_up_pressed.png
    WORKING_DIRECTORY ${DefaultTheme_DIR}
    VERBATIM
)

ADD_CUSTOM_TARGET(DefaultTheme ALL
    DEPENDS ${THEME_BINARY_DIR}/${DefaultTheme_RESOURCE_NAME}
)

# FIXME: Should we install our default theme to share/edje insted of share/ewebkit-0/themes?
INSTALL(FILES ${THEME_BINARY_DIR}/${DefaultTheme_RESOURCE_NAME} DESTINATION share/edje)
