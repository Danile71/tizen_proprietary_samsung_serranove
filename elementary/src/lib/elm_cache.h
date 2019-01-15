/**
 * @defgroup Caches Caches
 * @ingroup elm_infra_group
 *
 * These are functions which let one fine-tune some cache values for
 * Elementary applications, thus allowing for performance adjustments.
 *
 * @{
 */

/**
 * @brief Flush all caches.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @remark Frees all data that was in cache and is not currently being used to reduce
 * memory usage. This frees Edje's, Evas' and Eet's cache.
 * @remark Evas caches are flushed for every canvas associated with a window.
 *
 * @ingroup Caches
 */
EAPI void      elm_cache_all_flush(void);

/**
 * @}
 */
