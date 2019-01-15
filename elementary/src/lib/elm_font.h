/**
 * @defgroup Fonts Elementary Fonts
 * @ingroup elm_infra_group
 *
 * These are functions dealing with font rendering, selection and the
 * like for Elementary applications. One might fetch which system
 * fonts are there to use and set custom fonts for individual classes
 * of UI items containing text (text classes).
 *
 * @{
 */

/**
 * @brief Structure of Elm Font Property
 */
typedef struct _Elm_Font_Properties
{
   const char *name;
   Eina_List  *styles;
} Elm_Font_Properties;

/**
 * @brief Translate a font (family) name string in fontconfig's font names
 * syntax into an @c Elm_Font_Properties struct.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] font The font name and styles string
 * @return the font properties struct
 *
 * @ingroup Fonts
 *
 * @remark The reverse translation can be achieved with
 * elm_font_fontconfig_name_get(), for one style only (single font
 * instance, not family).
 */
EAPI Elm_Font_Properties *elm_font_properties_get(const char *font);

/**
 * @brief Free font properties return by elm_font_properties_get().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] efp the font properties struct
 *
 * @ingroup Fonts
 */
EAPI void             elm_font_properties_free(Elm_Font_Properties *efp);

/**
 * @brief Translate a font name, bound to a style, into fontconfig's font names
 * syntax.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] name The font (family) name
 * @param[in] style The given style (may be @c NULL)
 *
 * @return the font name and style string
 *
 * @ingroup Fonts
 *
 * @remark The reverse translation can be achieved with
 * elm_font_properties_get(), for one style only (single font
 * instance, not family).
 */
EAPI char      *elm_font_fontconfig_name_get(const char *name, const char *style);

/**
 * @brief Free the font string return by elm_font_fontconfig_name_get().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] name the font properties struct
 *
 * @ingroup Fonts
 */
EAPI void             elm_font_fontconfig_name_free(char *name);

/**
 * @brief Create a font hash table of available system fonts.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark One must call it with @p list being the return value of
 * Evas Font Available List function. The hash will be indexed by font
 * (family) names, being its values @c Elm_Font_Properties blobs.
 *
 * @param[in] list The list of available system fonts, as returned by
 * Evas Font Available List function.
 * @return the font hash.
 *
 * @ingroup Fonts
 *
 * @remark The user is supposed to get it populated at least with 3
 * default font families (Sans, Serif, Monospace), which should be
 * present on most systems.
 */
EAPI Eina_Hash       *elm_font_available_hash_add(Eina_List *list);

/**
 * @brief Free the hash returned by elm_font_available_hash_add().
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] hash the hash to be freed.
 *
 * @ingroup Fonts
 */
EAPI void             elm_font_available_hash_del(Eina_Hash *hash);

/**
 * @}
 */
