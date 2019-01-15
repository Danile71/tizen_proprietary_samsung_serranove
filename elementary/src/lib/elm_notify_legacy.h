/**
 * @brief Add a new notify to the parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Notify
 */
EAPI Evas_Object                 *elm_notify_add(Evas_Object *parent);

/**
 * @brief Set the notify parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The notify object
 * @param[in] parent The new parent
 *
 * @remark Once the parent object is set, a previously set one will be disconnected
 * and replaced.
 *
 * @ingroup Notify
 */
EAPI void                         elm_notify_parent_set(Evas_Object *obj, Evas_Object *parent);

/**
 * @brief Get the notify parent
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] obj The notify object
 * @return The parent
 *
 * @see elm_notify_parent_set()
 *
 * @ingroup Notify
 */
EAPI Evas_Object                 *elm_notify_parent_get(const Evas_Object *obj);

#include "elm_notify.eo.legacy.h"