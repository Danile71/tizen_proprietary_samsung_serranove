//////////////////////////////////////////////////////////
//TIZEN_ONLY(20150205): Elm plug in msg add for indicator
/**
 * @typedef Elm_Plug_Message
 *
 * @brief The structure type holding the message
 *        which elm plug received from ecore evas.
 *
 */
struct _Elm_Plug_Message
{
   int msg_domain;
   int msg_id;
   void *data;
   int size;
};

typedef struct _Elm_Plug_Message Elm_Plug_Message;
//////////////////////////////////////////////////////////

/**
 * @brief Add a new plug image to the parent.
 *
 * @if MOBILE @since_tizen 2.3
 * @elseif WEARABLE @since_tizen 2.3.1
 * @endif
 *
 * @param[in] parent The parent object
 * @return The new plug image object or NULL if it cannot be created
 *
 * @ingroup Plug
 */
EAPI Evas_Object    *elm_plug_add(Evas_Object *parent);

#include "elm_plug.eo.legacy.h"
