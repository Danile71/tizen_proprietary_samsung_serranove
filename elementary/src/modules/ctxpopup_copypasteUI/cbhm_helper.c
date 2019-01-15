#include "cbhm_helper.h"

#ifdef HAVE_ELEMENTARY_X
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#define ATOM_CBHM_WINDOW_NAME "CBHM_XWIN"
#define ATOM_CBHM_MSG "CBHM_MSG"
#define ATOM_CBHM_COUNT_GET "CBHM_cCOUNT"
#define ATOM_CBHM_TEXT_COUNT_GET "CBHM_TEXT_cCOUNT"
#define ATOM_CBHM_IMAGE_COUNT_GET "CBHM_IMAGE_cCOUNT"
#define ATOM_CBHM_SERIAL_NUMBER "CBHM_SERIAL_NUMBER"
#define MSG_CBHM_COUNT_GET "get count"
#define ATOM_CBHM_ERROR "CBHM_ERROR"
#define ATOM_CBHM_ITEM "CBHM_ITEM"
#endif

#ifdef HAVE_ELEMENTARY_X
Ecore_X_Window
_elm_ee_xwin_get(const Ecore_Evas *ee)
{
   const char *engine_name;
   if (!ee) return 0;

   engine_name = ecore_evas_engine_name_get(ee);
   if (EINA_UNLIKELY(!engine_name)) return 0;

   if (!strcmp(engine_name, "software_x11"))
     {
        return ecore_evas_software_x11_window_get(ee);
     }
   else if (!strcmp(engine_name, "opengl_x11"))
     {
        return ecore_evas_gl_x11_window_get(ee);
     }
   return 0;
}

static Ecore_X_Window
_x11_elm_widget_xwin_get(const Evas_Object *obj)
{
   Evas_Object *top;
   Ecore_X_Window xwin = 0;

   top = elm_widget_top_get(obj);
   if (!top) top = elm_widget_top_get(elm_widget_parent_widget_get(obj));
   if (top) xwin = elm_win_xwindow_get(top);
   if (!xwin)
     {
        Ecore_Evas *ee;
        Evas *evas = evas_object_evas_get(obj);
        if (!evas) return 0;
        ee = ecore_evas_ecore_evas_get(evas);
        if (!ee) return 0;
        xwin = _elm_ee_xwin_get(ee);
     }
   return xwin;
}

Ecore_X_Window
_cbhm_window_get()
{
   Ecore_X_Atom x_atom_cbhm = ecore_x_atom_get(ATOM_CBHM_WINDOW_NAME);
   Ecore_X_Window x_cbhm_win = 0;
   unsigned char *buf = NULL;
   int num = 0;
   int ret = ecore_x_window_prop_property_get(0, x_atom_cbhm, XA_WINDOW, 0, &buf, &num);
   DMSG("ret: %d, num: %d\n", ret, num);
   if (ret && num)
     memcpy(&x_cbhm_win, buf, sizeof(Ecore_X_Window));
   if (buf)
     free(buf);
   return x_cbhm_win;
}
#endif

Eina_Bool
_cbhm_msg_send(Evas_Object *obj, char *msg)
{
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window x_cbhm_win = _cbhm_window_get();
   Ecore_X_Atom x_atom_cbhm_msg = ecore_x_atom_get(ATOM_CBHM_MSG);

   Ecore_X_Window xwin = _x11_elm_widget_xwin_get(obj);
   if (!xwin)
   {
     DMSG("ERROR: can't get x window\n");
     return EINA_FALSE;
   }

   DMSG("x_cbhm: 0x%x\n", x_cbhm_win);
   if (!x_cbhm_win || !x_atom_cbhm_msg)
     return EINA_FALSE;

   XClientMessageEvent m;
   memset(&m, 0, sizeof(m));
   m.type = ClientMessage;
   m.display = ecore_x_display_get();
   m.window = xwin;
   m.message_type = x_atom_cbhm_msg;
   m.format = 8;
   snprintf(m.data.b, 20, "%s", msg);

   XSendEvent(ecore_x_display_get(), x_cbhm_win, False, NoEventMask, (XEvent*)&m);

   ecore_x_sync();
   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif
}

#ifdef HAVE_ELEMENTARY_X
void *
_cbhm_reply_get(Ecore_X_Window xwin, Ecore_X_Atom property, Ecore_X_Atom *x_data_type, int *num)
{
   unsigned char *data = NULL;
   if (x_data_type)
     *x_data_type = 0;
   if (!property)
      return NULL;
   ecore_x_sync();
   if (num)
     *num = 0;

   long unsigned int num_ret = 0, bytes = 0;
   int ret = 0, size_ret;
   unsigned int i;
   unsigned char *prop_ret;
   Ecore_X_Atom type_ret;
   ret = XGetWindowProperty(ecore_x_display_get(), xwin, property, 0, LONG_MAX,
                            False, ecore_x_window_prop_any_type(), (Atom *)&type_ret, &size_ret,
                            &num_ret, &bytes, &prop_ret);
   if (ret != Success)
     return NULL;
   if (!num_ret)
     {
        XFree(prop_ret);
        return NULL;
     }

   if (!(data = malloc(num_ret * size_ret / 8)))
     {
        XFree(prop_ret);
        return NULL;
     }

   switch (size_ret) {
      case 8:
        for (i = 0; i < num_ret; i++)
          (data)[i] = prop_ret[i];
        break;

      case 16:
        for (i = 0; i < num_ret; i++)
          ((unsigned short *)data)[i] = ((unsigned short *)prop_ret)[i];
        break;

      case 32:
        for (i = 0; i < num_ret; i++)
          ((unsigned int *)data)[i] = ((unsigned long *)prop_ret)[i];
        break;
     }

   XFree(prop_ret);

   if (num)
     *num = num_ret;
   if (x_data_type)
     *x_data_type = type_ret;

   return data;
}
#endif

int
_cbhm_item_count_get(Evas_Object *obj EINA_UNUSED, int atom_index)
{
#ifdef HAVE_ELEMENTARY_X
   char *ret, count;
   Ecore_X_Atom x_atom_cbhm_count_get = 0;

   if (atom_index == ATOM_INDEX_CBHM_COUNT_ALL)
     x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_COUNT_GET);
   else if (atom_index == ATOM_INDEX_CBHM_COUNT_TEXT)
     x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_TEXT_COUNT_GET);
   else if (atom_index == ATOM_INDEX_CBHM_COUNT_IMAGE)
     x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_IMAGE_COUNT_GET);

   Ecore_X_Window cbhm_xwin = _cbhm_window_get();
   DMSG("x_win: 0x%x, x_atom: %d\n", cbhm_xwin, x_atom_cbhm_count_get);
   ret = _cbhm_reply_get(cbhm_xwin, x_atom_cbhm_count_get, NULL, NULL);
   if (ret)
     {
        count = atoi(ret);
        DMSG("count: %d\n", count);
        free(ret);
        return count;
     }
   DMSG("ret: 0x%x\n", ret);
#endif
   return -1;
}
/*
int
_cbhm_item_count_get(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_X
   char *ret, count;
   if(_cbhm_msg_send(obj, MSG_CBHM_COUNT_GET))
     {
        DMSG("message send success\n");
        Ecore_X_Atom x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_COUNT_GET);
        Ecore_X_Window xwin = ecore_evas_software_x11_window_get(
           ecore_evas_ecore_evas_get(evas_object_evas_get(obj)));
        DMSG("x_win: 0x%x, x_atom: %d\n", xwin, x_atom_cbhm_count_get);
        ret = _cbhm_reply_get(xwin, x_atom_cbhm_count_get, NULL, NULL);
        if (ret)
          {
             count = atoi(ret);
             DMSG("count: %d\n", count);
             free(ret);
             return count;
          }
        DMSG("ret: 0x%x\n", ret);
     }
#endif
   return -1;
}
*/

#ifdef HAVE_ELEMENTARY_X
Eina_Bool
_cbhm_item_get(Evas_Object *obj EINA_UNUSED, int idx, Ecore_X_Atom *data_type, char **buf)
#else
Eina_Bool
_cbhm_item_get(Evas_Object *obj, int idx, void *data_type, char **buf)
#endif

{
   if (buf)
     *buf = NULL;
   if (data_type)
     *(int *)data_type = 0;

#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window cbhm_xwin = _cbhm_window_get();
   char send_buf[20];
   char *ret;

   snprintf(send_buf, 20, "CBHM_ITEM%d", idx);
   Ecore_X_Atom x_atom_cbhm_item = ecore_x_atom_get(send_buf);
   Ecore_X_Atom x_atom_item_type = 0;

   DMSG("x_win: 0x%x, x_atom: %d\n", cbhm_xwin, x_atom_cbhm_item);
   ret = _cbhm_reply_get(cbhm_xwin, x_atom_cbhm_item, &x_atom_item_type, NULL);
   if (ret)
     {
        DMSG("data_type: %d, buf: %s\n", x_atom_item_type, ret);
        if (buf)
          *buf = ret;
        else
          free(ret);

        if (data_type)
          *data_type = x_atom_item_type;

        Ecore_X_Atom x_atom_cbhm_error = ecore_x_atom_get(ATOM_CBHM_ERROR);
        if (x_atom_item_type == x_atom_cbhm_error)
          return EINA_FALSE;
     }
   DMSG("ret: 0x%x\n", ret);
/*
   Ecore_X_Window xwin = ecore_evas_software_x11_window_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(obj)));
   char send_buf[20];
   char *ret;

   snprintf(send_buf, 20, "GET_ITEM%d", idx);
   if (_cbhm_msg_send(obj, send_buf))
     {
        DMSG("message send success\n");
        Ecore_X_Atom x_atom_cbhm_item = ecore_x_atom_get(ATOM_CBHM_ITEM);
        Ecore_X_Atom x_atom_item_type = 0;

        DMSG("x_win: 0x%x, x_atom: %d\n", xwin, x_atom_cbhm_item);
        ret = _cbhm_reply_get(xwin, x_atom_cbhm_item, &x_atom_item_type, NULL);
        if (ret)
          {
             DMSG("data_type: %d, buf: %s\n", x_atom_item_type, ret);
             if (buf)
               *buf = ret;
             else
               free(ret);
             if (data_type)
               *data_type = x_atom_item_type;

             Ecore_X_Atom x_atom_cbhm_error = ecore_x_atom_get(ATOM_CBHM_ERROR);
             if (x_atom_item_type == x_atom_cbhm_error)
               return EINA_FALSE;
          }
        DMSG("ret: 0x%x\n", ret);
     }
*/
#endif
   return EINA_FALSE;
}

#ifdef HAVE_ELEMENTARY_X
Eina_Bool
_cbhm_item_set(Evas_Object *obj, Ecore_X_Atom data_type, char *item_data)
{
   Ecore_X_Window x_cbhm_win = _cbhm_window_get();
   Ecore_X_Atom x_atom_cbhm_item = ecore_x_atom_get(ATOM_CBHM_ITEM);
   ecore_x_window_prop_property_set(x_cbhm_win, x_atom_cbhm_item, data_type, 8, item_data, strlen(item_data) + 1);
   ecore_x_sync();
   if (_cbhm_msg_send(obj, "SET_ITEM"))
     {
        DMSG("message send success\n");
        return EINA_TRUE;
     }
   return EINA_FALSE;
}
#endif

unsigned int
_cbhm_serial_number_get()
{
   unsigned int senum = 0;
#ifdef HAVE_ELEMENTARY_X
   unsigned char *buf = NULL;
   Ecore_X_Atom x_atom_cbhm_SN = ecore_x_atom_get(ATOM_CBHM_SERIAL_NUMBER);
   Ecore_X_Window x_cbhm_win = _cbhm_window_get();
   buf = _cbhm_reply_get(x_cbhm_win, x_atom_cbhm_SN, NULL, NULL);
   if (buf)
     {
        memcpy(&senum, buf, sizeof(senum));
        free(buf);
     }
#endif
   return senum;
}
