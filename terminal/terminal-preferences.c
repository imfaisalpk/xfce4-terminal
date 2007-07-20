/* $Id$ */
/*-
 * Copyright (c) 2004-2007 os-cillation e.K.
 *
 * Written by Benedikt Meurer <benny@xfce.org>.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-monitor.h>
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-private.h>



enum
{
  PROP_0,
  PROP_ACCEL_NEW_TAB,
  PROP_ACCEL_NEW_WINDOW,
  PROP_ACCEL_DETACH_TAB,
  PROP_ACCEL_CLOSE_TAB,
  PROP_ACCEL_CLOSE_WINDOW,
  PROP_ACCEL_COPY,
  PROP_ACCEL_PASTE,
  PROP_ACCEL_PASTE_SELECTION,
  PROP_ACCEL_PREFERENCES,
  PROP_ACCEL_SHOW_MENUBAR,
  PROP_ACCEL_SHOW_TOOLBARS,
  PROP_ACCEL_SHOW_BORDERS,
  PROP_ACCEL_FULLSCREEN,
  PROP_ACCEL_SET_TITLE,
  PROP_ACCEL_RESET,
  PROP_ACCEL_RESET_AND_CLEAR,
  PROP_ACCEL_PREV_TAB,
  PROP_ACCEL_NEXT_TAB,
  PROP_ACCEL_SWITCH_TO_TAB1,
  PROP_ACCEL_SWITCH_TO_TAB2,
  PROP_ACCEL_SWITCH_TO_TAB3,
  PROP_ACCEL_SWITCH_TO_TAB4,
  PROP_ACCEL_SWITCH_TO_TAB5,
  PROP_ACCEL_SWITCH_TO_TAB6,
  PROP_ACCEL_SWITCH_TO_TAB7,
  PROP_ACCEL_SWITCH_TO_TAB8,
  PROP_ACCEL_SWITCH_TO_TAB9,
  PROP_ACCEL_CONTENTS,
  PROP_BACKGROUND_MODE,
  PROP_BACKGROUND_IMAGE_FILE,
  PROP_BACKGROUND_IMAGE_STYLE,
  PROP_BACKGROUND_DARKNESS,
  PROP_BINDING_BACKSPACE,
  PROP_BINDING_DELETE,
  PROP_COLOR_FOREGROUND,
  PROP_COLOR_BACKGROUND,
  PROP_COLOR_CURSOR,
  PROP_COLOR_SELECTION,
  PROP_COLOR_SELECTION_USE_DEFAULT,
  PROP_COLOR_PALETTE1,
  PROP_COLOR_PALETTE2,
  PROP_COLOR_PALETTE3,
  PROP_COLOR_PALETTE4,
  PROP_COLOR_PALETTE5,
  PROP_COLOR_PALETTE6,
  PROP_COLOR_PALETTE7,
  PROP_COLOR_PALETTE8,
  PROP_COLOR_PALETTE9,
  PROP_COLOR_PALETTE10,
  PROP_COLOR_PALETTE11,
  PROP_COLOR_PALETTE12,
  PROP_COLOR_PALETTE13,
  PROP_COLOR_PALETTE14,
  PROP_COLOR_PALETTE15,
  PROP_COLOR_PALETTE16,
  PROP_COMMAND_UPDATE_RECORDS,
  PROP_COMMAND_LOGIN_SHELL,
  PROP_FONT_ALLOW_BOLD,
  PROP_FONT_ANTI_ALIAS,
  PROP_FONT_NAME,
  PROP_MISC_ALWAYS_SHOW_TABS,
  PROP_MISC_BELL,
  PROP_MISC_BORDERS_DEFAULT,
  PROP_MISC_CURSOR_BLINKS,
  PROP_MISC_DEFAULT_GEOMETRY,
  PROP_MISC_INHERIT_GEOMETRY,
  PROP_MISC_MENUBAR_DEFAULT,
  PROP_MISC_MOUSE_AUTOHIDE,
  PROP_MISC_TOOLBARS_DEFAULT,
  PROP_MISC_CONFIRM_CLOSE,
  PROP_MISC_CYCLE_TABS,
  PROP_MISC_TAB_CLOSE_BUTTONS,
  PROP_MISC_TAB_POSITION,
  PROP_MISC_HIGHLIGHT_URLS,
  PROP_SCROLLING_BAR,
  PROP_SCROLLING_LINES,
  PROP_SCROLLING_ON_OUTPUT,
  PROP_SCROLLING_ON_KEYSTROKE,
  PROP_SHORTCUTS_NO_MENUKEY,
  PROP_SHORTCUTS_NO_MNEMONICS,
  PROP_TITLE_INITIAL,
  PROP_TITLE_MODE,
  PROP_TERM,
  PROP_VTE_WORKAROUND_TITLE_BUG,
  PROP_WORD_CHARS,
  N_PROPERTIES,
};

struct _TerminalPreferences
{
  GObject __parent__;

  /*< private >*/
  TerminalMonitor *monitor;
  GClosure        *notify;
  gchar          **files;

  GValue          *values;

  guint            load_idle_id;
  guint            store_idle_id;
  guint            loading_in_progress : 1;
};



static void     terminal_preferences_dispose            (GObject             *object);
static void     terminal_preferences_finalize           (GObject             *object);
static void     terminal_preferences_get_property       (GObject             *object,
                                                         guint                prop_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void     terminal_preferences_set_property       (GObject             *object,
                                                         guint                prop_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void     terminal_preferences_schedule_load      (TerminalMonitor     *monitor,
                                                         const gchar         *path,
                                                         TerminalPreferences *preferences);
static gboolean terminal_preferences_load_idle          (gpointer             user_data);
static void     terminal_preferences_load_idle_destroy  (gpointer             user_data);
static void     terminal_preferences_schedule_store     (TerminalPreferences *preferences);
static gboolean terminal_preferences_store_idle         (gpointer             user_data);
static void     terminal_preferences_store_idle_destroy (gpointer             user_data);
static void     terminal_preferences_suspend_monitor    (TerminalPreferences *preferences);
static void     terminal_preferences_resume_monitor     (TerminalPreferences *preferences);



static void
transform_color_to_string (const GValue *src,
                           GValue       *dst)
{
  GdkColor *color;
  gchar     buffer[32];

  color = g_value_get_boxed (src);
  g_snprintf (buffer, 32, "#%04x%04x%04x",
              (guint) color->red,
              (guint) color->green,
              (guint) color->blue);
  g_value_set_string (dst, buffer);
}



static void
transform_string_to_boolean (const GValue *src,
                             GValue       *dst)
{
  g_value_set_boolean (dst, !exo_str_is_equal (g_value_get_string (src), "FALSE"));
}



static void
transform_string_to_color (const GValue *src,
                           GValue       *dst)
{
  GdkColor color;

  gdk_color_parse (g_value_get_string (src), &color);
  g_value_set_boxed (dst, &color);
}



static void
transform_string_to_double (const GValue *src,
                            GValue       *dst)
{
  const gchar *sval;
  gdouble      dval;
  gchar       *endptr;

  sval = g_value_get_string (src);
  dval = strtod (sval, &endptr);

  if (*endptr != '\0')
    dval = g_ascii_strtod (sval, NULL);

  g_value_set_double (dst, dval);
}



static void
transform_string_to_uint (const GValue *src,
                          GValue       *dst)
{
  g_value_set_uint (dst, strtoul (g_value_get_string (src), NULL, 10));
}



static void
transform_string_to_enum (const GValue *src,
                          GValue       *dst)
{
  GEnumClass *genum_class;
  GEnumValue *genum_value;

  genum_class = g_type_class_peek (G_VALUE_TYPE (dst));
  genum_value = g_enum_get_value_by_name (genum_class, g_value_get_string (src));
  if (G_UNLIKELY (genum_value == NULL))
    genum_value = genum_class->values;
  g_value_set_enum (dst, genum_value->value);
}



G_DEFINE_TYPE (TerminalPreferences, terminal_preferences, G_TYPE_OBJECT);



static void
terminal_preferences_class_init (TerminalPreferencesClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_preferences_dispose;
  gobject_class->finalize = terminal_preferences_finalize;
  gobject_class->get_property = terminal_preferences_get_property;
  gobject_class->set_property = terminal_preferences_set_property;

  /* register transform functions */
  if (!g_value_type_transformable (GDK_TYPE_COLOR, G_TYPE_STRING))
    g_value_register_transform_func (GDK_TYPE_COLOR, G_TYPE_STRING, transform_color_to_string);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_BOOLEAN))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, transform_string_to_boolean);
  if (!g_value_type_transformable (G_TYPE_STRING, GDK_TYPE_COLOR))
    g_value_register_transform_func (G_TYPE_STRING, GDK_TYPE_COLOR, transform_string_to_color);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_DOUBLE))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE, transform_string_to_double);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_UINT))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, transform_string_to_uint);
  if (!g_value_type_transformable (G_TYPE_STRING, GTK_TYPE_POSITION_TYPE))
    g_value_register_transform_func (G_TYPE_STRING, GTK_TYPE_POSITION_TYPE, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND_STYLE))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND_STYLE, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_SCROLLBAR))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_SCROLLBAR, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_TITLE))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_TITLE, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_ERASE_BINDING))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_ERASE_BINDING, transform_string_to_enum);

  /**
   * TerminalPreferences:accel-new-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEW_TAB,
                                   g_param_spec_string ("accel-new-tab",
                                                        _("Open Tab"),
                                                        _("Open Tab"),
                                                        "<control><shift>t",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-new-window:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEW_WINDOW,
                                   g_param_spec_string ("accel-new-window",
                                                        _("Open Terminal"),
                                                        _("Open Terminal"),
                                                        "<control><shift>n",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-detach-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_DETACH_TAB,
                                   g_param_spec_string ("accel-detach-tab",
                                                        _("Detach Tab"),
                                                        _("Detach Tab"),
                                                        "<control><shift>d",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-close-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CLOSE_TAB,
                                   g_param_spec_string ("accel-close-tab",
                                                        _("Close Tab"),
                                                        _("Close Tab"),
                                                        "<control><shift>w",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-close-window:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CLOSE_WINDOW,
                                   g_param_spec_string ("accel-close-window",
                                                        _("Close Window"),
                                                        _("Close Window"),
                                                        "<control><shift>q",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-copy:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_COPY,
                                   g_param_spec_string ("accel-copy",
                                                        _("Copy"),
                                                        _("Copy"),
                                                        "<control><shift>c",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-paste:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PASTE,
                                   g_param_spec_string ("accel-paste",
                                                        _("Paste"),
                                                        _("Paste"),
                                                        "<control><shift>v",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-paste-selection:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PASTE_SELECTION,
                                   g_param_spec_string ("accel-paste-selection",
                                                        _("Paste Selection"),
                                                        _("Paste Selection"),
                                                        NULL,
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-preferences:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PREFERENCES,
                                   g_param_spec_string ("accel-preferences",
                                                        _("Preferences"),
                                                        _("Preferences"),
                                                        _("Disabled"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-menubar:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_MENUBAR,
                                   g_param_spec_string ("accel-show-menubar",
                                                        _("Show menubar"),
                                                        _("Show menubar"),
                                                        _("Disabled"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-toolbars:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_TOOLBARS,
                                   g_param_spec_string ("accel-show-toolbars",
                                                        _("Show toolbars"),
                                                        _("Show toolbars"),
                                                        _("Disabled"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-borders:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_BORDERS,
                                   g_param_spec_string ("accel-show-borders",
                                                        _("Show borders"),
                                                        _("Show borders"),
                                                        _("Disabled"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-fullscreen:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_FULLSCREEN,
                                   g_param_spec_string ("accel-fullscreen",
                                                        _("Fullscreen"),
                                                        _("Fullscreen"),
                                                        "F11",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-set-title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SET_TITLE,
                                   g_param_spec_string ("accel-set-title",
                                                        _("Set Title"),
                                                        _("Set Title"),
                                                        _("Disabled"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-reset:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_RESET,
                                   g_param_spec_string ("accel-reset",
                                                        _("Reset"),
                                                        _("Reset"),
                                                        _("Disabled"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-reset-and-clear:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_RESET_AND_CLEAR,
                                   g_param_spec_string ("accel-reset-and-clear",
                                                        _("Reset and Clear"),
                                                        _("Reset and Clear"),
                                                        _("Disabled"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-prev-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PREV_TAB,
                                   g_param_spec_string ("accel-prev-tab",
                                                        _("Previous Tab"),
                                                        _("Previous Tab"),
                                                        "<control>Page_Up",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-next-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEXT_TAB,
                                   g_param_spec_string ("accel-next-tab",
                                                        _("Next Tab"),
                                                        _("Next Tab"),
                                                        "<control>Page_Down",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab1:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB1,
                                   g_param_spec_string ("accel-switch-to-tab1",
                                                        _("Switch to Tab 1"),
                                                        _("Switch to Tab 1"),
                                                        "<Alt>1",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab2:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB2,
                                   g_param_spec_string ("accel-switch-to-tab2",
                                                        _("Switch to Tab 2"),
                                                        _("Switch to Tab 2"),
                                                        "<Alt>2",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab3:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB3,
                                   g_param_spec_string ("accel-switch-to-tab3",
                                                        _("Switch to Tab 3"),
                                                        _("Switch to Tab 3"),
                                                        "<Alt>3",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab4:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB4,
                                   g_param_spec_string ("accel-switch-to-tab4",
                                                        _("Switch to Tab 4"),
                                                        _("Switch to Tab 4"),
                                                        "<Alt>4",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab5:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB5,
                                   g_param_spec_string ("accel-switch-to-tab5",
                                                        _("Switch to Tab 5"),
                                                        _("Switch to Tab 5"),
                                                        "<Alt>5",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab6:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB6,
                                   g_param_spec_string ("accel-switch-to-tab6",
                                                        _("Switch to Tab 6"),
                                                        _("Switch to Tab 6"),
                                                        "<Alt>6",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab7:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB7,
                                   g_param_spec_string ("accel-switch-to-tab7",
                                                        _("Switch to Tab 7"),
                                                        _("Switch to Tab 7"),
                                                        "<Alt>7",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab8:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB8,
                                   g_param_spec_string ("accel-switch-to-tab8",
                                                        _("Switch to Tab 8"),
                                                        _("Switch to Tab 8"),
                                                        "<Alt>8",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab9:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB9,
                                   g_param_spec_string ("accel-switch-to-tab9",
                                                        _("Switch to Tab 9"),
                                                        _("Switch to Tab 9"),
                                                        "<Alt>9",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-contents:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CONTENTS,
                                   g_param_spec_string ("accel-contents",
                                                        _("Contents"),
                                                        _("Contents"),
                                                        "F1",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-mode:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_MODE,
                                   g_param_spec_enum ("background-mode",
                                                      "background-mode",
                                                      "background-mode",
                                                      TERMINAL_TYPE_BACKGROUND,
                                                      TERMINAL_BACKGROUND_SOLID,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-image-file:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_IMAGE_FILE,
                                   g_param_spec_string ("background-image-file",
                                                        "background-image-file",
                                                        "background-image-file",
                                                        "",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-image-style:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_IMAGE_STYLE,
                                   g_param_spec_enum ("background-image-style",
                                                      "background-image-style",
                                                      "background-image-style",
                                                      TERMINAL_TYPE_BACKGROUND_STYLE,
                                                      TERMINAL_BACKGROUND_STYLE_TILED,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-darkness:
   *
   * If a background image has been set (either an image file or a transparent background), the
   * terminal will adjust the brightness of the image before drawing the image. To do so, the
   * terminal will create a copy of the background image (or snapshot of the root window) and
   * modify its pixel values.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_DARKNESS,
                                   g_param_spec_double ("background-darkness",
                                                        "background-darkness",
                                                        "background-darkness",
                                                        0.0, 1.0, 0.5,
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:binding-backspace:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BINDING_BACKSPACE,
                                   g_param_spec_enum ("binding-backspace",
                                                      "binding-backspace",
                                                      "binding-backspace",
                                                      TERMINAL_TYPE_ERASE_BINDING,
                                                      TERMINAL_ERASE_BINDING_AUTO,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:binding-delete:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BINDING_DELETE,
                                   g_param_spec_enum ("binding-delete",
                                                      "binding-delete",
                                                      "binding-delete",
                                                      TERMINAL_TYPE_ERASE_BINDING,
                                                      TERMINAL_ERASE_BINDING_AUTO,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-foreground:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_FOREGROUND,
                                   g_param_spec_string ("color-foreground",
                                                        "color-foreground",
                                                        "color-foreground",
                                                        "White",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-background:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_BACKGROUND,
                                   g_param_spec_string ("color-background",
                                                        "color-background",
                                                        "color-background",
                                                        "Black",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-cursor:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_CURSOR,
                                   g_param_spec_string ("color-cursor",
                                                        "color-cursor",
                                                        "color-cursor",
                                                        "Green",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-selection:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_SELECTION,
                                   g_param_spec_string ("color-selection",
                                                        "color-selection",
                                                        "color-selection",
                                                        "White",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-selection-use-default:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_SELECTION_USE_DEFAULT,
                                   g_param_spec_boolean ("color-selection-use-default",
                                                         "color-selection-use-default",
                                                         "color-selection-use-default",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette1:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE1,
                                   g_param_spec_string ("color-palette1",
                                                        "color-palette1",
                                                        "color-palette1",
                                                        "#000000000000",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette2:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE2,
                                   g_param_spec_string ("color-palette2",
                                                        "color-palette2",
                                                        "color-palette2",
                                                        "#aaaa00000000",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette3:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE3,
                                   g_param_spec_string ("color-palette3",
                                                        "color-palette3",
                                                        "color-palette3",
                                                        "#0000aaaa0000",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette4:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE4,
                                   g_param_spec_string ("color-palette4",
                                                        "color-palette4",
                                                        "color-palette4",
                                                        "#aaaa55550000",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette5:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE5,
                                   g_param_spec_string ("color-palette5",
                                                        "color-palette5",
                                                        "color-palette5",
                                                        "#00000000aaaa",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette6:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE6,
                                   g_param_spec_string ("color-palette6",
                                                        "color-palette6",
                                                        "color-palette6",
                                                        "#aaaa0000aaaa",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette7:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE7,
                                   g_param_spec_string ("color-palette7",
                                                        "color-palette7",
                                                        "color-palette7",
                                                        "#0000aaaaaaaa",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette8:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE8,
                                   g_param_spec_string ("color-palette8",
                                                        "color-palette8",
                                                        "color-palette8",
                                                        "#aaaaaaaaaaaa",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette9:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE9,
                                   g_param_spec_string ("color-palette9",
                                                        "color-palette9",
                                                        "color-palette9",
                                                        "#555555555555",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette10:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE10,
                                   g_param_spec_string ("color-palette10",
                                                        "color-palette10",
                                                        "color-palette10",
                                                        "#ffff55555555",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette11:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE11,
                                   g_param_spec_string ("color-palette11",
                                                        "color-palette11",
                                                        "color-palette11",
                                                        "#5555ffff5555",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette12:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE12,
                                   g_param_spec_string ("color-palette12",
                                                        "color-palette12",
                                                        "color-palette12",
                                                        "#ffffffff5555",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette13:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE13,
                                   g_param_spec_string ("color-palette13",
                                                        "color-palette13",
                                                        "color-palette13",
                                                        "#55555555ffff",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette14:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE14,
                                   g_param_spec_string ("color-palette14",
                                                        "color-palette14",
                                                        "color-palette14",
                                                        "#ffff5555ffff",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette15:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE15,
                                   g_param_spec_string ("color-palette15",
                                                        "color-palette15",
                                                        "color-palette15",
                                                        "#5555ffffffff",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette16:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE16,
                                   g_param_spec_string ("color-palette16",
                                                        "color-palette16",
                                                        "color-palette16",
                                                        "#ffffffffffff",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:command-update-records:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COMMAND_UPDATE_RECORDS,
                                   g_param_spec_boolean ("command-update-records",
                                                         "command-update-records",
                                                         "command-update-records",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:command-login-shell:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COMMAND_LOGIN_SHELL,
                                   g_param_spec_boolean ("command-login-shell",
                                                         "command-login-shell",
                                                         "command-login-shell",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:font-allow-bold:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_ALLOW_BOLD,
                                   g_param_spec_boolean ("font-allow-bold",
                                                         "font-allow-bold",
                                                         "font-allow-bold",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:font-anti-alias:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_ANTI_ALIAS,
                                   g_param_spec_boolean ("font-anti-alias",
                                                         "font-anti-alias",
                                                         "font-anti-alias",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:font-name:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        "font-name",
                                                        "font-name",
                                                        "Monospace 12",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-always-show-tabs:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_ALWAYS_SHOW_TABS,
                                   g_param_spec_boolean ("misc-always-show-tabs",
                                                         "misc-always-show-tabs",
                                                         "misc-always-show-tabs",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-bell:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_BELL,
                                   g_param_spec_boolean ("misc-bell",
                                                         "misc-bell",
                                                         "misc-bell",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-borders-default:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_BORDERS_DEFAULT,
                                   g_param_spec_boolean ("misc-borders-default",
                                                         "misc-borders-default",
                                                         "misc-borders-default",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-cursor-blinks:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_CURSOR_BLINKS,
                                   g_param_spec_boolean ("misc-cursor-blinks",
                                                         "misc-cursor-blinks",
                                                         "misc-cursor-blinks",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-default-geometry:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_DEFAULT_GEOMETRY,
                                   g_param_spec_string ("misc-default-geometry",
                                                        "misc-default-geometry",
                                                        "misc-default-geometry",
                                                        "80x24",
                                                        EXO_PARAM_READWRITE));
  /**
   * TerminalPreferences:misc-inherit-geometry:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_INHERIT_GEOMETRY,
                                   g_param_spec_boolean ("misc-inherit-geometry",
                                                         "misc-inherit-geometry",
                                                         "misc-inherit-geometry",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-menubar-default:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_MENUBAR_DEFAULT,
                                   g_param_spec_boolean ("misc-menubar-default",
                                                         "misc-menubar-default",
                                                         "misc-menubar-default",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-mouse-autohide:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_MOUSE_AUTOHIDE,
                                   g_param_spec_boolean ("misc-mouse-autohide",
                                                         "misc-mouse-autohide",
                                                         "misc-mouse-autohide",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-toolbars-default:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_TOOLBARS_DEFAULT,
                                   g_param_spec_boolean ("misc-toolbars-default",
                                                         "misc-toolbars-default",
                                                         "misc-toolbars-default",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-confirm-close:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_CONFIRM_CLOSE,
                                   g_param_spec_boolean ("misc-confirm-close",
                                                         "misc-confirm-close",
                                                         "misc-confirm-close",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-cycle-tabs:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_CYCLE_TABS,
                                   g_param_spec_boolean ("misc-cycle-tabs",
                                                         "misc-cycle-tabs",
                                                         "misc-cycle-tabs",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-tab-close-buttons:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_TAB_CLOSE_BUTTONS,
                                   g_param_spec_boolean ("misc-tab-close-buttons",
                                                         "misc-tab-close-buttons",
                                                         "misc-tab-close-buttons",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-tab-position:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_TAB_POSITION,
                                   g_param_spec_enum ("misc-tab-position",
                                                      "misc-tab-position",
                                                      "misc-tab-position",
                                                      GTK_TYPE_POSITION_TYPE,
                                                      GTK_POS_TOP,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-highlight-urls:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_HIGHLIGHT_URLS,
                                   g_param_spec_boolean ("misc-highlight-urls",
                                                         "misc-highlight-urls",
                                                         "misc-highlight-urls",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-bar:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_BAR,
                                   g_param_spec_enum ("scrolling-bar",
                                                      "scrolling-bar",
                                                      "scrolling-bar",
                                                      TERMINAL_TYPE_SCROLLBAR,
                                                      TERMINAL_SCROLLBAR_RIGHT,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-lines:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_LINES,
                                   g_param_spec_uint ("scrolling-lines",
                                                      "scrolling-lines",
                                                      "scrolling-lines",
                                                      0u, 1024u * 1024u, 1000u,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-on-output:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_ON_OUTPUT,
                                   g_param_spec_boolean ("scrolling-on-output",
                                                         "scrolling-on-output",
                                                         "scrolling-on-output",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-on-keystroke:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_ON_KEYSTROKE,
                                   g_param_spec_boolean ("scrolling-on-keystroke",
                                                         "scrolling-on-keystroke",
                                                         "scrolling-on-keystroke",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:shortcuts-no-menukey:
   *
   * Disable menu shortcut key (F10 by default).
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SHORTCUTS_NO_MENUKEY,
                                   g_param_spec_boolean ("shortcuts-no-menukey",
                                                         "shortcuts-no-menukey",
                                                         "shortcuts-no-menukey",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:shortcuts-no-mnemonics:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SHORTCUTS_NO_MNEMONICS,
                                   g_param_spec_boolean ("shortcuts-no-mnemonics",
                                                         "shortcuts-no-mnemonics",
                                                         "shortcuts-no-mnemonics",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:title-initial:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE_INITIAL,
                                   g_param_spec_string ("title-initial",
                                                        "title-initial",
                                                        "title-initial",
                                                        _("Terminal"),
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:title-mode:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE_MODE,
                                   g_param_spec_enum ("title-mode",
                                                      "title-mode",
                                                      "title-mode",
                                                      TERMINAL_TYPE_TITLE,
                                                      TERMINAL_TITLE_APPEND,
                                                      EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:term:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TERM,
                                   g_param_spec_string ("term",
                                                        "term",
                                                        "term",
                                                        "xterm",
                                                        EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:vte-workaround-title-bug:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_VTE_WORKAROUND_TITLE_BUG,
                                   g_param_spec_boolean ("vte-workaround-title-bug",
                                                         "vte-workaround-title-bug",
                                                         "vte-workaround-title-bug",
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  /**
   * TerminalPreferences:word-chars:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_WORD_CHARS,
                                   g_param_spec_string ("word-chars",
                                                        "word-chars",
                                                        "word-chars",
                                                        "-A-Za-z0-9,./?%&#:_~",
                                                        EXO_PARAM_READWRITE));
}



static void
terminal_preferences_init (TerminalPreferences *preferences)
{
  gchar **directories;
  guint   n;

  /* setup file monitor interface */
  preferences->monitor = terminal_monitor_get ();
  preferences->notify  = g_cclosure_new_object (G_CALLBACK (terminal_preferences_schedule_load),
                                                G_OBJECT (preferences));
  g_closure_set_marshal (preferences->notify, g_cclosure_marshal_VOID__STRING);

  /* find all possible config files */
  directories = xfce_resource_dirs (XFCE_RESOURCE_CONFIG);
  for (n = 0; directories[n] != NULL; ++n) ;
  preferences->files = g_new (gchar *, n + 1);
  for (n = 0; directories[n] != NULL; ++n)
    preferences->files[n] = g_build_filename (directories[n], "Terminal", "terminalrc", NULL);
  preferences->files[n] = NULL;
  g_strfreev (directories);

  /* load the settings */
  preferences->values = g_new0 (GValue, N_PROPERTIES);
  terminal_preferences_load_idle (preferences);

  /* launch the file monitor */
  terminal_preferences_resume_monitor (preferences);
}



static void
terminal_preferences_dispose (GObject *object)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);

  /* flush preferences */
  if (G_UNLIKELY (preferences->store_idle_id != 0))
    {
      terminal_preferences_store_idle (preferences);
      g_source_remove (preferences->store_idle_id);
    }

  if (G_UNLIKELY (preferences->load_idle_id != 0))
    g_source_remove (preferences->load_idle_id);

  terminal_preferences_suspend_monitor (preferences);
  g_object_unref (G_OBJECT (preferences->monitor));

  (*G_OBJECT_CLASS (terminal_preferences_parent_class)->dispose) (object);
}



static void
terminal_preferences_finalize (GObject *object)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  guint                n;

  for (n = 1; n < N_PROPERTIES; ++n)
    if (G_IS_VALUE (preferences->values + n))
      g_value_unset (preferences->values + n);
  g_free (preferences->values);

  g_closure_unref (preferences->notify);
  g_strfreev (preferences->files);

  (*G_OBJECT_CLASS (terminal_preferences_parent_class)->finalize) (object);
}



static void
terminal_preferences_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  GValue              *src;

  _terminal_return_if_fail (prop_id < N_PROPERTIES);

  src = preferences->values + prop_id;
  if (G_VALUE_HOLDS (src, pspec->value_type))
    g_value_copy (src, value);
  else
    g_param_value_set_default (pspec, value);
}



static void
terminal_preferences_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  GValue              *dst;

  _terminal_return_if_fail (prop_id < N_PROPERTIES);

  dst = preferences->values + prop_id;
  if (!G_IS_VALUE (dst))
    {
      g_value_init (dst, pspec->value_type);
      g_param_value_set_default (pspec, dst);
    }

  if (g_param_values_cmp (pspec, value, dst) != 0)
    {
      g_value_copy (value, dst);
      g_object_notify (object, pspec->name);
      terminal_preferences_schedule_store (preferences);
    }
}



static gchar*
property_name_to_option_name (const gchar *property_name)
{
  const gchar *s;
  gboolean     upper = TRUE;
  gchar       *option;
  gchar       *t;

  option = g_new (gchar, strlen (property_name) + 1);
  for (s = property_name, t = option; *s != '\0'; ++s)
    {
      if (*s == '-')
        {
          upper = TRUE;
        }
      else if (upper)
        {
          *t++ = g_ascii_toupper (*s);
          upper = FALSE;
        }
      else
        {
          *t++ = *s;
        }
    }
  *t = '\0';

  return option;
}



static void
terminal_preferences_schedule_load (TerminalMonitor     *monitor,
                                    const gchar         *path,
                                    TerminalPreferences *preferences)
{
  if (preferences->load_idle_id == 0 && preferences->store_idle_id == 0)
    {
      preferences->load_idle_id = g_idle_add_full (G_PRIORITY_LOW, terminal_preferences_load_idle,
                                                   preferences, terminal_preferences_load_idle_destroy);
    }
}



static gboolean
terminal_preferences_load_idle (gpointer user_data)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (user_data);
  const gchar         *string;
  GParamSpec         **specs;
  GParamSpec          *spec;
  XfceRc              *rc;
  GValue               dst = { 0, };
  GValue               src = { 0, };
  gchar               *option;
  guint                nspecs;
  guint                n;

  rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG, "Terminal/terminalrc", TRUE);
  if (G_UNLIKELY (rc == NULL))
    {
      g_warning ("Unable to load terminal preferences.");
      return FALSE;
    }

  g_object_freeze_notify (G_OBJECT (preferences));

  xfce_rc_set_group (rc, "Configuration");

  preferences->loading_in_progress = TRUE;

  specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (preferences), &nspecs);
  for (n = 0; n < nspecs; ++n)
    {
      spec = specs[n];

      option = property_name_to_option_name (spec->name);
      string = xfce_rc_read_entry (rc, option, NULL);
      g_free (option);

      if (G_UNLIKELY (string == NULL))
        continue;

      g_value_init (&src, G_TYPE_STRING);
      g_value_set_static_string (&src, string);

      if (spec->value_type == G_TYPE_STRING)
        {
          g_object_set_property (G_OBJECT (preferences), spec->name, &src);
        }
      else if (g_value_type_transformable (G_TYPE_STRING, spec->value_type))
        {
          g_value_init (&dst, spec->value_type);
          if (g_value_transform (&src, &dst))
            g_object_set_property (G_OBJECT (preferences), spec->name, &dst);
          g_value_unset (&dst);
        }
      else
        {
          g_warning ("Unable to load property \"%s\"", spec->name);
        }

      g_value_unset (&src);
    }
  g_free (specs);

  preferences->loading_in_progress = FALSE;

  xfce_rc_close (rc);

  g_object_thaw_notify (G_OBJECT (preferences));

  return FALSE;
}



static void
terminal_preferences_load_idle_destroy (gpointer user_data)
{
  TERMINAL_PREFERENCES (user_data)->load_idle_id = 0;
}



static void
terminal_preferences_schedule_store (TerminalPreferences *preferences)
{
  if (preferences->store_idle_id == 0 && !preferences->loading_in_progress)
    {
      preferences->store_idle_id = g_idle_add_full (G_PRIORITY_LOW, terminal_preferences_store_idle,
                                                    preferences, terminal_preferences_store_idle_destroy);
    }
}



static gboolean
terminal_preferences_store_idle (gpointer user_data)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (user_data);
  const gchar         *string;
  GParamSpec         **specs;
  GParamSpec          *spec;
  XfceRc              *rc;
  GValue               dst = { 0, };
  GValue               src = { 0, };
  gchar               *option;
  guint                nspecs;
  guint                n;

  rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG, "Terminal/terminalrc", FALSE);
  if (G_UNLIKELY (rc == NULL))
    {
      g_warning ("Unable to store terminal preferences.");
      return FALSE;
    }

  terminal_preferences_suspend_monitor (preferences);

  xfce_rc_set_group (rc, "Configuration");

  specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (preferences), &nspecs);
  for (n = 0; n < nspecs; ++n)
    {
      spec = specs[n];

      g_value_init (&dst, G_TYPE_STRING);

      if (spec->value_type == G_TYPE_STRING)
        {
          g_object_get_property (G_OBJECT (preferences), spec->name, &dst);
        }
      else
        {
          g_value_init (&src, spec->value_type);
          g_object_get_property (G_OBJECT (preferences), spec->name, &src);
          g_value_transform (&src, &dst);
          g_value_unset (&src);
        }

      option = property_name_to_option_name (spec->name);

      string = g_value_get_string (&dst);

      if (G_LIKELY (string != NULL))
        xfce_rc_write_entry (rc, option, string);
      
      g_free (option);

      g_value_unset (&dst);
    }
  g_free (specs);

  xfce_rc_close (rc);

  terminal_preferences_resume_monitor (preferences);

  return FALSE;
}



static void
terminal_preferences_store_idle_destroy (gpointer user_data)
{
  TERMINAL_PREFERENCES (user_data)->store_idle_id = 0;
}



static void
terminal_preferences_suspend_monitor (TerminalPreferences *preferences)
{
  terminal_monitor_remove_by_closure (preferences->monitor, preferences->notify);
}



static void
terminal_preferences_resume_monitor (TerminalPreferences *preferences)
{
  guint n;

  for (n = 0; preferences->files[n] != NULL; ++n)
    {
      terminal_monitor_add (preferences->monitor,
                            preferences->files[n],
                            preferences->notify);
    }
}



/**
 * terminal_preferences_get:
 *
 * Return value :
 **/
TerminalPreferences*
terminal_preferences_get (void)
{
  static TerminalPreferences *preferences = NULL;

  if (G_UNLIKELY (preferences == NULL))
    {
      preferences = g_object_new (TERMINAL_TYPE_PREFERENCES, NULL);
      g_object_add_weak_pointer (G_OBJECT (preferences),
                                 (gpointer) &preferences);
    }
  else
    {
      g_object_ref (G_OBJECT (preferences));
    }

  return preferences;
}



