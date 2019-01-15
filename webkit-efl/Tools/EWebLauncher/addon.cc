#include <string>
#include <algorithm>
#include <queue>
#include <sstream>
#include <cerrno>
#include <sys/stat.h>
#include <limits.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <JavaScriptCore/API/JSContextRef.h>
#include <JavaScriptCore/API/JSStringRef.h>
#include "EWebKit.h"
#include "EWebLauncher.h"

using namespace std;


namespace {
ELauncher* g_elauncher = 0;

int currentZoomLevel = DEFAULT_ZOOM_LEVEL;

// the zoom values are chosen to be like in Mozilla Firefox 3
int zoomLevels[] = {30, 50, 67, 80, 90,
                            100,
                            110, 120, 133, 150, 170, 200, 240, 300};

void print_history(Eina_List *list)
{
    Eina_List *l;
    void *d;

    if (!ewk_test_verbose)
       return;

    printf("Session history contains:\n");

    EINA_LIST_FOREACH(list, l, d) {
       Ewk_History_Item *item = (Ewk_History_Item*)d;
       cairo_surface_t *cs = ewk_history_item_icon_surface_get(item);
       char buf[PATH_MAX];
       int s = snprintf(buf, sizeof(buf), "/tmp/favicon-%s.png", ewk_history_item_uri_original_get(item));
       for (s--; s >= (int)sizeof("/tmp/favicon-"); s--) {
           if (!isalnum(buf[s]) && buf[s] != '.')
               buf[s] = '_';
       }
       cs = ewk_history_item_icon_surface_get(item);

       if (cs && cairo_surface_status(cs) == CAIRO_STATUS_SUCCESS)
           cairo_surface_write_to_png(cs, buf);
       else
           buf[0] = '\0';

       printf("* '%s' title='%s' icon='%s'\n",
              ewk_history_item_uri_original_get(item),
              ewk_history_item_title_get(item), buf);
    }
}

void zoom_level_set(Evas_Object *webview, int level)
{
    float factor = ((float) zoomLevels[level]) / 100.0;
    Evas_Coord ox, oy, mx, my, cx, cy;
    evas_pointer_canvas_xy_get(evas_object_evas_get(webview), &mx, &my);
    evas_object_geometry_get(webview, &ox, &oy, NULL, NULL);
    cx = mx - ox;
    cy = my - oy;
    printf( "EWL: Zoom set: %f\n", factor );
    ewk_view_zoom_animated_set(webview, factor, 0.5, cx, cy);
}



// UTILITY!

template <class Object, size_t size> inline
Object* ra_end( Object (&aref)[size] )
{
    Object* begin = aref;
    return begin + size;
}

struct Cmd2Key
{
    string cmd;
    string key;

} ewk_test_cmd2key [] = {
    { "quit", "Escape" },
    { "back", "F1" },
    { "forward", "F2" },
    { "next-encoding", "F3" },
    { "hit-test", "F4" },
    { "reload", "F5" },
    { "stop", "F6" },
    { "zoom-out", "F7" },
    { "zoom-in", "F8" },
    { "new", "F9" },
    { "expand", "F10" },
    { "prerender1", "F11" },
    { "suspend", "d" },
    { "resume", "e" },
    { "js-suspend", "s" },
    { "js-resume", "q" }
};


struct FCmdEql
{
    string k;
    FCmdEql( string yk ): k( yk ) {}
    template <class Kmd>
    bool operator()( const Kmd& item ) { return item.cmd == k; }
};

queue<string> cmdq;

}

int ewk_test_load_script( const char* script )
{
    struct stat state;
    if ( -1 == stat( script, &state ) )
    {
        perror( script );
        return EINA_FALSE;
    }
    size_t size = state.st_size;
    FILE* sin = fopen( script, "r" );
    if ( sin )
    {
        char* buf = (char*)malloc( size+1 );
        int realsize = fread( buf, size, 1, sin );
        if ( realsize != 1 )
        {
            printf( "Warning: script not fully read!\n" );
            free( buf );
        }
        else
        {
            buf[size] = '\0';
            ewk_test_script_commands = buf;
        }
        fclose( sin );
        return EINA_TRUE;
    }
    return EINA_FALSE;
}

Eina_Bool ewk_test_pickup_script_command( void* state );

Eina_Bool kick_ecore( void* )
{
    printf( "*TICK\n" );
    if ( cmdq.empty() )
    {
        printf( "*--- empty (60 second of delay)\n" );
        ecore_timer_add( 60, kick_ecore, 0 );
        return ECORE_CALLBACK_CANCEL;
    }

    ecore_idler_add( &ewk_test_pickup_script_command, g_elauncher );
    return ECORE_CALLBACK_CANCEL; // stop timing - idler will install you back
}

void ewk_test_reload_script( int signum )
{
    if ( !ewk_test_script_path )
        return;

    printf( "EWK: Reloading commands from %s\n", ewk_test_script_path );
    if ( ewk_test_load_script( ewk_test_script_path ) )
    {
        ewk_test_enqueue_script( ewk_test_script_commands );
    }
    return;
}

void ewk_test_execute_command( ELauncher* app, const char* cmd )
{
    Evas_Object* obj = app->browser;
    int s = 0;
    switch (*cmd)
    {
    case '.':
        printf( "**EWK: %s\n", cmd+1 );
        goto end;

    case '%':
        s = atoi( cmd+1 );
        sleep( s );
        goto end;

    case '*':
        printf( "**EWK is setting new URL: %s\n", cmd+1 );
        ewk_view_uri_set( app->browser, cmd+1 );
        goto end;

    case '#':
        return; // comment


    default:
        break;

    }

    {
        string k = cmd;
        printf( "*COMMAND: %s\n", k.c_str() );
        const char scrollprefix [] = "scroll:"; size_t scrolln = (sizeof scrollprefix)-1;
        if ( 0 == k.compare( 0, scrolln, scrollprefix ) )
        {
            k = k.substr( scrolln );
            size_t pos = k.find( ':' );
            if ( pos == string::npos )
                return;

            int x = atoi( k.substr( 0, pos-0 ).c_str() );
            int y = atoi( k.substr( pos+1 ).c_str() );

            ewk_frame_scroll_add( ewk_view_frame_main_get(obj), x, y );
        }
        else
        {
            Cmd2Key* i = find_if( ewk_test_cmd2key, ra_end( ewk_test_cmd2key ), FCmdEql( cmd ) );
            if ( i == ra_end( ewk_test_cmd2key ) )
                return; // ignore false commands - this is only a test application

            ewk_test_handle_command( app, i->key.c_str(), obj );
        }

        if ( k == "quit" )
            return; // prevent any additional action after you already quit
    }

end: ;
   // ecore_main_loop_iterate(); // flush pending events
}

void ewk_test_handle_command( ELauncher* app, const char* ev_key, Evas_Object* obj )
{
    static const char *encodings[] = {
        "ISO-8859-1",
        "UTF-8",
        NULL
    };
    static int currentEncoding = -1;

    if (0 == strcmp(ev_key, "Escape")) {
        ewk_test_close_window(app->ee);
    } else if (0 == strcmp(ev_key, "F1")) {
        info("Back (F1) was pressed\n");
        if (ewk_view_back_possible(obj)) {
            Ewk_History *history = ewk_view_history_get(obj);
            Eina_List *list = ewk_history_back_list_get(history);
            print_history(list);
            ewk_history_item_list_free(list);
            ewk_view_back(obj);
        } else
            info("Back ignored: No back history\n");
    } else if (0 == strcmp(ev_key, "F2")) {
        info("Forward (F2) was pressed\n");
        if (ewk_view_forward_possible(obj)) {
            Ewk_History *history = ewk_view_history_get(obj);
            Eina_List *list = ewk_history_forward_list_get(history);
            print_history(list);
            ewk_history_item_list_free(list);
            ewk_view_forward(obj);
        } else
            info("Forward ignored: No forward history\n");
    } else if (0 == strcmp(ev_key, "F3")) {
        currentEncoding++;
        currentEncoding %= (sizeof(encodings) / sizeof(encodings[0]));
        info("Set encoding (F3) pressed. New encoding to %s", encodings[currentEncoding]);
        ewk_view_setting_encoding_custom_set(obj, encodings[currentEncoding]);
    } else if (0 == strcmp(ev_key, "F4")) {
        Evas_Object *frame = ewk_view_frame_main_get(obj);
        Evas_Coord x, y;
        Ewk_Hit_Test *ht;

        evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &x, &y);
        ht = ewk_frame_hit_test_new(frame, x, y);
        if (!ht)
            printf("No hit test returned for point %d,%d\n", x, y);
        else {
            printf("Hit test for point %d,%d\n"
                   "  pos=%3d,%3d\n"
                   "  bounding_box=%d,%d + %dx%d\n"
                   "  title='%s'\n"
                   "  alternate_text='%s'\n"
                   "  frame=%p (%s)\n"
                   "  link {\n"
                   "    text='%s'\n"
                   "    url='%s'\n"
                   "    title='%s'\n"
                   "    target frame=%p (%s)\n"
                   "  }\n"
                   "  flags {\n"
                   "    editable=%hhu\n"
                   "    selected=%hhu\n"
                   "  }\n",
                   x, y,
                   ht->x, ht->y,
                   ht->bounding_box.x, ht->bounding_box.y, ht->bounding_box.w, ht->bounding_box.h,
                   ht->title,
                   ht->alternate_text,
                   ht->frame, evas_object_name_get(ht->frame),
                   ht->link.text,
                   ht->link.url,
                   ht->link.title,
                   ht->link.target_frame, evas_object_name_get(ht->link.target_frame),
                   ht->context & EWK_HIT_TEST_RESULT_CONTEXT_EDITABLE,
                   ht->context & EWK_HIT_TEST_RESULT_CONTEXT_SELECTION);
            ewk_frame_hit_test_free(ht);
        }

    } else if (0 == strcmp(ev_key, "F5")) {
        info("Reload (F5) was pressed, reloading.\n");
        ewk_view_reload(obj);
    } else if (0 == strcmp(ev_key, "F6")) {
        info("Stop (F6) was pressed, stop loading.\n");
        ewk_view_stop(obj);
    /* } FIXME: uncomment code below after Bug 18662 lands upstream.
    else if (0 == strcmp(ev_key, "F12")) {
        bool status = ewk_webframe_object_keyboard_navigation_get(page);
        ewk_webframe_object_keyboard_navigation_set(page, !status);
        info("Command::keyboard navigation toggle\n");*/
    } else if (0 == strcmp(ev_key, "F7")) {
        info("Zoom out (F7) was pressed.\n");
        if (currentZoomLevel > MIN_ZOOM_LEVEL)
            zoom_level_set(obj, --currentZoomLevel);
    } else if (0 == strcmp(ev_key, "F8")) {
        info("Zoom in (F8) was pressed.\n");
        if (currentZoomLevel < MAX_ZOOM_LEVEL)
            zoom_level_set(obj, ++currentZoomLevel);
    } else if (0 == strcmp(ev_key, "F9")) {
        info("Create new window (F9) was pressed.\n");
        Eina_Rectangle geometry = {0, 0, 0, 0};
        ewk_test_create_browser("http://www.google.com",
                      app->theme, app->userAgent, geometry, app->backingStore,
                      NULL, 0, ewk_view_zoom_cairo_scaling_get(obj));
    } else if (0 == strcmp(ev_key, "F10")) {
        Evas_Coord x, y, w, h;
        Evas_Object *frame = ewk_view_frame_main_get(obj);
        float zoom = zoomLevels[currentZoomLevel] / 100.0;

        ewk_frame_visible_content_geometry_get(frame, EINA_FALSE, &x, &y, &w, &h);
        x -= w;
        y -= h;
        w *= 4;
        h *= 4;
        info("Pre-render %d,%d + %dx%d\n", x, y, w, h);
        ewk_view_pre_render_region(obj, x, y, w, h, zoom);
    } else if (0 == strcmp(ev_key, "d")) {
        info("Render suspended");
        ewk_view_disable_render(obj);
    } else if (0 == strcmp(ev_key, "e")) {
        info("Render resumed");
        ewk_view_enable_render(obj);
    } else if (0 == strcmp(ev_key, "s")) {
        ewk_view_javascript_suspend( obj );
    } else if (0 == strcmp(ev_key, "q")) {
        ewk_view_javascript_resume( obj );
    }
}

Eina_Bool ewk_test_pickup_script_command( void* state )
{
    if ( cmdq.empty() )
    {
        // shouldn't happen, but let's not make difference
        ecore_timer_add( 60, kick_ecore, 0 );
        return ECORE_CALLBACK_CANCEL;
    }

    ELauncher* app = static_cast<ELauncher*>( state );
    string cmd = cmdq.front();
    cmdq.pop();
    ewk_test_execute_command( app, cmd.c_str() );
    ecore_timer_add( 0.5, kick_ecore, 0 );
    return ECORE_CALLBACK_CANCEL;
}

void ewk_test_enqueue_script( char* commands )
{
    const char* p = commands;
    for (;;) {
        const char* b = p;
        p = strchr( p, '\n' );
        if ( !p )
        {
            string cmd( b );
            if ( cmd != "" )
                cmdq.push( cmd );
            break;
        }

        string cmd( b, p );
        ++p;
        if ( cmd != "" )
            cmdq.push( cmd );
    }
    free( ewk_test_script_commands ); // eat up
}

void ewk_test_install_ewk_function( ELauncher* app )
{
    g_elauncher = app;
    ecore_timer_add( 0.5, kick_ecore, 0 );
}

