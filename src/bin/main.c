#include <stdlib.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Ecore_X_Atoms.h>
#include <Edje.h>
#include "enscribi_canvas.h" 
#include "enscribi_input_frame.h" 
#include "enscribi_util.h" 

static void
_cb_exit(Ecore_Evas *ee)
{
    printf("Enscribi: _cb_exit\n");
    ecore_main_loop_quit();
}

static void
_cb_move(Ecore_Evas *ee)
{
    printf("Enscribi: _cb_move\n");

    //Evas_Coord w, h, x, y;
    //ecore_evas_geometry_get(ee, &x, &y, &w, &h);
    //printf("Moved to %d %d %d %d\n", x, y, w, h);
}

static void
_cb_resize(Ecore_Evas *ee)
{
    Evas_Object *kbd;
    int w, h;

    /* get the geometry of ee. we don't need the x,y coords so we send NULL. */
    ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);

    /* find our bg object and resize it to the window size (if it exists) */
    kbd = evas_object_name_find(ecore_evas_get(ee), "kbd");
    if (kbd) evas_object_resize(kbd, w, h);
}

int _cb_client_message(void *data, int type, void *event)
{
  Ecore_X_Event_Client_Message *ev = event;
  Ecore_Evas *ee = data;

  /* if the message is for the keyboard window... */
  if (ev->win == ecore_evas_software_x11_window_get(ee))
    {
      /* if the message type is a keyboard state... */
      if (ev->message_type == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
        {
          /* handle the various keyboard types */
          if (ev->data.l[0] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_OFF)
            {
              printf("KBD Off!\n");
            }
          else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
            {
              printf("KBD On - use default mode!\n");
            }
          else if (ev->data.l[0] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_ALPHA)
            {
              printf("KBD Alpha - use alpha mode!\n");
            }
          else if (ev->data.l[0] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_NUMERIC)
            {
              printf("KBD Numeric - use numeric mode!\n");
            }
          else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_PIN)
            {
              printf("KBD Pin - use pin number entry mode!\n");
            }
          else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_PHONE_NUMBER)
            {
              printf("KBD Phone Number - use phone number entry mode!\n");
            }
          else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_HEX)
            {
              printf("KBD Hex - use hex entry mode mode!\n");
            }
          else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_TERMINAL)
            {
              printf("KBD Terminal - use terminal entry mode mode!\n");
            }
          else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_PASSWORD)
            {
              printf("KBD Password - use password entry mode!\n");
            }
          else
            {
              printf("KBD Unknown mode - use default\n");
            }
        }
    }
  /* return 1 to allow this event to be passed on to other handlers */
  return 1;
}

static const char *
_string_to_keysym(const char *str)
{
   int glyph, ok;
   /* utf8 -> glyph id (unicode - ucs4) */
   glyph = 0;
   ok = evas_string_char_next_get(str, 0, &glyph);
   if (glyph <= 0) return NULL;
   /* glyph id -> keysym */
   if (glyph > 0xff) glyph |= 0x1000000;
   return ecore_x_keysym_string_get(glyph);
}

void
_send_string_press(const char *str)
{
   const char *key = NULL;
   
   key = _string_to_keysym(str);
   printf("Enscribi: key = %s\n", key);
   if (!key) return;
   ecore_x_test_fake_key_press(key);
}

static void
_cb_input_send(void *data, Evas_Object *obj, void *event_info)
{
    const char * string;

    string = event_info;
    printf("Enscribi: _cb_input_send (%s)\n", string);
    
    _send_string_press(string);
}

static void
_cb_key_pressed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
    printf("Enscribi: _cb_key_pressed (%s : %s)\n", emission, source);
    
    ecore_x_test_fake_key_press(source);
}

static void
_cb_key_send_pressed(void *data, Evas_Object *obj, const char *emission, const char *source)
{
    printf("Enscribi: _cb_key_send_pressed (%s : %s)\n", emission, source);
    enscribi_input_frame_send_result(edje_object_part_swallow_get(obj, "input/1"));
    enscribi_input_frame_send_result(edje_object_part_swallow_get(obj, "input/2"));
    enscribi_input_frame_send_result(edje_object_part_swallow_get(obj, "input/3"));
}

int
main(int argc, char **argv)
{
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *bg, *edje, *o, *canvas;
    Evas_Coord w, h;
    Enscribi_Recognizer *recognizer;
    w = 480;
    h = 160;

    /* initialize our libraries */
    evas_init();
    ecore_init();
    ecore_evas_init();
    edje_init();

    /* create our Ecore_Evas */
    ee = ecore_evas_new(NULL, 0, 0, w, h, NULL);
    ecore_evas_title_set(ee, "EKanji");
	ecore_evas_callback_destroy_set(ee, _cb_exit);
    ecore_evas_callback_delete_request_set(ee, _cb_exit);
	ecore_evas_callback_move_set(ee, _cb_move);
    ecore_evas_callback_resize_set(ee, _cb_resize);

    /* get a pointer our new Evas canvas */
    evas = ecore_evas_get(ee);

    /* Load and set up the edje objects */
    edje = edje_object_add(evas);
    evas_object_name_set(edje, "kbd");
    edje_object_file_set(edje, enscribi_theme_find("enscribi"), "enscribi/kbd");
    evas_object_move(edje, 0, 0);
    evas_object_resize(edje, w, h);
    evas_object_show(edje);

    /* Add the input frames and handwriting recognition engine */
    recognizer = enscribi_recognizer_new();
    o = enscribi_input_frame_add(evas, edje);
    enscribi_input_frame_recognizer_set(o, recognizer);
    evas_object_smart_callback_add(o, "input,selected", _cb_input_send, NULL);
    edje_object_part_swallow(edje, "input/1", o);
    o = enscribi_input_frame_add(evas, edje);
    enscribi_input_frame_recognizer_set(o, recognizer);
    evas_object_smart_callback_add(o, "input,selected", _cb_input_send, NULL);
    edje_object_part_swallow(edje, "input/2", o);
    o = enscribi_input_frame_add(evas, edje);
    enscribi_input_frame_recognizer_set(o, recognizer);
    evas_object_smart_callback_add(o, "input,selected", _cb_input_send, NULL);
    edje_object_part_swallow(edje, "input/3", o);
    edje_object_signal_callback_add(edje, "key,pressed", "*", _cb_key_pressed, NULL);
    edje_object_signal_callback_add(edje, "result,send", "*", _cb_key_send_pressed, NULL);

    /* show the window */
    ecore_evas_show(ee);

    /* set window states and properties */
    Ecore_X_Window_State states[2];

    states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
    states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
    ecore_x_netwm_window_state_set(ecore_evas_software_x11_window_get(ee), states, 2);
    ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _cb_client_message, ee);

    ecore_evas_title_set(ee, "Virtual Keyboard");
    ecore_evas_name_class_set(ee, "Enscribi", "Virtual-Keyboard");

    /* tell e that this is a vkbd window */
    ecore_x_e_virtual_keyboard_set(ecore_evas_software_x11_window_get(ee), 1);
    
    /* start the main event loop */
    ecore_main_loop_begin();

    /* when the main event loop exits, shutdown our libraries */
    enscribi_recognizer_del(recognizer);
    edje_shutdown();
    ecore_evas_shutdown();
    ecore_shutdown();
    evas_shutdown();
}
