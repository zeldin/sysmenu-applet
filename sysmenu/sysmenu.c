/* sysmenu.c - A Gnome2-workalike system menu
 * Copyright (C) 2012 Marcus Comstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <panel-applet.h>

#include "menu.h"
#include "sysmenu.h"

static GtkWidget *
sysmenu_create_menu (SysMenuApplet *sysmenu)
{
  GtkWidget *menu;

  menu = create_applications_menu ("gnomecc.menu", NULL, FALSE);

  g_object_set_data (G_OBJECT (menu), "menu_applet", sysmenu);

  return menu;
}

static void
panel_menu_bar_object_init (GtkWidget *menubar)
{
  GtkStyleContext *context;
  GtkCssProvider *provider;

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider,
				   "PanelMenuBarObject {\n"
				   " border-width: 0px;\n"
				   "}",
				   -1, NULL);
  context = gtk_widget_get_style_context (GTK_WIDGET (menubar));
  gtk_style_context_add_provider (context,
				  GTK_STYLE_PROVIDER (provider),
				  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
  gtk_style_context_add_class (context, "gnome-panel-menu-bar");
}

static SysMenuApplet *
create_sysmenu (PanelApplet *applet)
{
  SysMenuApplet *sysmenu_applet = g_new0 (SysMenuApplet, 1);

  sysmenu_applet->applet = applet;
  sysmenu_applet->menubar = gtk_menu_bar_new();

  panel_menu_bar_object_init (sysmenu_applet->menubar);

  GtkWidget *menuitem = gtk_menu_item_new();

  setup_menuitem (GTK_WIDGET (menuitem),
		  GTK_ICON_SIZE_INVALID, NULL,
		  _("System"));

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),
			     sysmenu_create_menu (sysmenu_applet));

  gtk_menu_shell_append (GTK_MENU_SHELL(sysmenu_applet->menubar), menuitem);

  gtk_container_add (GTK_CONTAINER (applet), sysmenu_applet->menubar);

  return sysmenu_applet;
}

static void
destroy_cb (GtkWidget *object, SysMenuApplet *sysmenu_applet)
{
  g_return_if_fail (sysmenu_applet);
  g_free (sysmenu_applet);
}

static gboolean
sysmenu_applet_fill (PanelApplet *applet)
{
  SysMenuApplet *sysmenu_applet;

  g_set_application_name (_("System Menu"));

  gtk_window_set_default_icon_name ("gnome-computer");
  panel_applet_set_flags (applet, PANEL_APPLET_EXPAND_MINOR);

  sysmenu_applet = create_sysmenu (applet);

  gtk_widget_set_tooltip_text (GTK_WIDGET (applet),
			       _("Change desktop appearance and behavior, get help, or log out"));

  g_signal_connect (sysmenu_applet->menubar,
		    "destroy",
		    G_CALLBACK (destroy_cb),
		    sysmenu_applet);

  gtk_widget_show_all (GTK_WIDGET (applet));

  return TRUE;
}

static gboolean
sysmenu_applet_factory (PanelApplet *applet,
			const gchar *iid,
			gpointer     data)
{
  gboolean retval = FALSE;

  if (!strcmp (iid, "SysMenuApplet"))
    retval = sysmenu_applet_fill (applet);

  if (retval == FALSE) {
    exit (-1);
  }

  return retval;
}

PANEL_APPLET_OUT_PROCESS_FACTORY ("SysMenuAppletFactory",
				  PANEL_TYPE_APPLET,
				  sysmenu_applet_factory,
				  NULL)
