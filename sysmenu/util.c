/*
 * GNOME panel utils
 * (C) 1997, 1998, 1999, 2000 The Free Software Foundation
 * Copyright 2000 Helix Code, Inc.
 * Copyright 2000,2001 Eazel, Inc.
 * Copyright 2001 George Lebl
 * Copyright 2002 Sun Microsystems Inc.
 *
 * Authors: George Lebl
 *          Jacob Berkman
 *          Mark McLoughlin
 *          Vincent Untz <vuntz@gnome.org>
 */

#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "util.h"

#define PANEL_GENERAL_SCHEMA                    "org.gnome.gnome-panel.general"
#define PANEL_GENERAL_ENABLE_TOOLTIPS_KEY       "enable-tooltips"

static gboolean
panel_util_query_tooltip_cb (GtkWidget  *widget,
			     gint        x,
			     gint        y,
			     gboolean    keyboard_tip,
			     GtkTooltip *tooltip,
			     const char *text)
{
  GSettings *gsettings;
  gboolean   enable_tooltips;

  gsettings = g_settings_new (PANEL_GENERAL_SCHEMA);
  enable_tooltips = g_settings_get_boolean (gsettings,
					    PANEL_GENERAL_ENABLE_TOOLTIPS_KEY);
  g_object_unref (gsettings);

  if (!enable_tooltips)
    return FALSE;

  gtk_tooltip_set_text (tooltip, text);
  return TRUE;
}

void
panel_util_set_tooltip_text (GtkWidget  *widget,
			     const char *text)
{
  g_signal_handlers_disconnect_matched (widget,
					G_SIGNAL_MATCH_FUNC,
					0, 0, NULL,
					panel_util_query_tooltip_cb,
					NULL);

  if (PANEL_GLIB_STR_EMPTY (text)) {
    g_object_set (widget, "has-tooltip", FALSE, NULL);
    return;
  }

  g_object_set (widget, "has-tooltip", TRUE, NULL);
  g_signal_connect_data (widget, "query-tooltip",
			 G_CALLBACK (panel_util_query_tooltip_cb),
			 g_strdup (text), (GClosureNotify) g_free, 0);
}

static char *
panel_xdg_icon_remove_extension (const char *icon)
{
  char *icon_no_extension;
  char *p;

  icon_no_extension = g_strdup (icon);
  p = strrchr (icon_no_extension, '.');
  if (p &&
      (strcmp (p, ".png") == 0 ||
       strcmp (p, ".xpm") == 0 ||
       strcmp (p, ".svg") == 0)) {
    *p = 0;
  }

  return icon_no_extension;
}

char *
panel_find_icon (GtkIconTheme  *icon_theme,
		 const char    *icon_name,
		 gint           size)
{
  GtkIconInfo *info;
  char        *retval;
  char        *icon_no_extension;

  if (icon_name == NULL || strcmp (icon_name, "") == 0)
    return NULL;

  if (g_path_is_absolute (icon_name)) {
    if (g_file_test (icon_name, G_FILE_TEST_EXISTS)) {
      return g_strdup (icon_name);
    } else {
      char *basename;

      basename = g_path_get_basename (icon_name);
      retval = panel_find_icon (icon_theme, basename,
				size);
      g_free (basename);

      return retval;
    }
  }

  /* This is needed because some .desktop files have an icon name *and*
   * an extension as icon */
  icon_no_extension = panel_xdg_icon_remove_extension (icon_name);

  info = gtk_icon_theme_lookup_icon (icon_theme, icon_no_extension,
				     size, 0);

  g_free (icon_no_extension);

  if (info) {
    retval = g_strdup (gtk_icon_info_get_filename (info));
    gtk_icon_info_free (info);
  } else
    retval = NULL;

  return retval;
}
