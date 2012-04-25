/* sysmenu.c - A Gnome2-workalike system menu
 *
 * Copyright (C) 2012 Marcus Comstedt
 * Copyright (C) 2005 Vincent Untz
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
 *
 * Authors:
 *      Marcus Comstedt <marcus@mc.pp.se>
 *
 * Based on code from panel-menu-items.c in gnome-panel:
 *
 * Authors:
 *      Vincent Untz <vincent@vuntz.net>
 *
 * Based on code from panel-menu-bar.c
 */

#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <panel-applet.h>

#include "menu.h"
#include "launch.h"
#include "util.h"
#include "sysmenu.h"

#define panel_key_file_get_string(key_file, key) \
	 g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP, key, NULL)
#define panel_key_file_get_locale_string(key_file, key) \
	 g_key_file_get_locale_string(key_file, G_KEY_FILE_DESKTOP_GROUP, key, NULL, NULL)

/* Keep in sync with the values defined in gnome-session/session.h */
typedef enum {
  PANEL_SESSION_MANAGER_LOGOUT_MODE_NORMAL = 0,
  PANEL_SESSION_MANAGER_LOGOUT_MODE_NO_CONFIRMATION,
  PANEL_SESSION_MANAGER_LOGOUT_MODE_FORCE
} PanelSessionManagerLogoutType;

typedef enum {
  PANEL_ACTION_NONE = 0,
  PANEL_ACTION_LOCK,
  PANEL_ACTION_LOGOUT,
  PANEL_ACTION_SHUTDOWN,
} PanelActionButtonType;

typedef struct {
  PanelActionButtonType   type;
  char                   *icon_name;
  char                   *text;
  char                   *tooltip;
  char                   *drag_id;
  void                  (*invoke)      (GtkWidget         *widget);
} PanelAction;

static void panel_action_lock_screen (GtkWidget *widget);
static void panel_action_logout (GtkWidget *widget);
static void panel_action_shutdown (GtkWidget *widget);

static PanelAction actions [] = {
  {
    PANEL_ACTION_NONE,
    NULL, NULL, NULL, NULL, NULL,
  },
  {
    PANEL_ACTION_LOCK,
    "system-lock-screen",
    N_("Lock Screen"),
    N_("Protect your computer from unauthorized use"),
    "ACTION:lock:NEW",
    panel_action_lock_screen,
  },
  {
    PANEL_ACTION_LOGOUT,
    "system-log-out",
    /* when changing one of those two strings, don't forget to
     * update the ones in panel-menu-items.c (look for
     * "1" (msgctxt: "panel:showusername")) */
    N_("Log Out..."),
    N_("Log out of this session to log in as a different user"),
    "ACTION:logout:NEW",
    panel_action_logout,
  },
  {
    PANEL_ACTION_SHUTDOWN,
    "system-shutdown",
    N_("Shut Down..."),
    N_("Shut down the computer"),
    "ACTION:shutdown:NEW",
    panel_action_shutdown,
  }
};

static GDBusProxy *
panel_session_manager_get (void)
{
  static GDBusProxy *proxy;
  GError   *error;

  if (!proxy) {
    error = NULL;
    proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
					   G_DBUS_PROXY_FLAGS_NONE,
					   NULL,
					   "org.gnome.SessionManager",
					   "/org/gnome/SessionManager",
					   "org.gnome.SessionManager",
					   NULL, &error);
    if (error) {
      g_warning ("Could not connect to session manager: %s",
		 error->message);
      g_error_free (error);
      return NULL;
    }
  }
  return proxy;
}

static void
panel_session_manager_request_logout (GDBusProxy *proxy,
				      PanelSessionManagerLogoutType  mode)
{
  GVariant *ret;
  GError   *error;

  if (!proxy) {
    g_warning ("Session manager service not available.");
    return;
  }

  error = NULL;
  ret = g_dbus_proxy_call_sync (proxy,
				"Logout",
				g_variant_new ("(u)", mode),
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				&error);

  if (ret)
    g_variant_unref (ret);

  if (error) {
    g_warning ("Could not ask session manager to log out: %s",
	       error->message);
    g_error_free (error);
  }
}

void
panel_session_manager_request_shutdown (GDBusProxy *proxy)
{
  GVariant *ret;
  GError   *error;

  if (!proxy) {
    g_warning ("Session manager service not available.");
    return;
  }

  error = NULL;
  ret = g_dbus_proxy_call_sync (proxy,
				"Shutdown",
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				&error);

  if (ret)
    g_variant_unref (ret);

  if (error) {
    g_warning ("Could not ask session manager to shut down: %s",
	       error->message);
    g_error_free (error);
  }
}

gboolean
panel_session_manager_is_shutdown_available (GDBusProxy *proxy)
{
  GVariant *ret;
  GError   *error;
  gboolean  is_shutdown_available = FALSE;

  if (!proxy) {
    g_warning ("Session manager service not available.");
    return FALSE;
  }

  error = NULL;
  ret = g_dbus_proxy_call_sync (proxy,
				"CanShutdown",
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				&error);

  if (error) {
    g_warning ("Could not ask session manager if shut down is available: %s",
	       error->message);
    g_error_free (error);

    return FALSE;
  } else {
    g_variant_get (ret, "(b)", &is_shutdown_available);
    g_variant_unref (ret);
  }

  return is_shutdown_available;
}

static void
panel_action_lock_screen (GtkWidget *widget)
{
  static GDBusProxy *proxy;
  GVariant *ret;
  GError   *error;

  if (!proxy) {
    error = NULL;
    proxy =
      g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
				     G_DBUS_PROXY_FLAGS_NONE,
				     NULL,
				     "org.gnome.ScreenSaver",
				     "/org/gnome/ScreenSaver",
				     "org.gnome.ScreenSaver",
				     NULL, &error);
    if (error) {
      g_warning ("Could not connect to screensaver: %s",
		 error->message);
      g_error_free (error);
    }
  }

  if (!proxy) {
    g_warning ("Screensaver service not available.");
    return;
  }

  error = NULL;
  ret = g_dbus_proxy_call_sync (proxy,
				"Lock",
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				&error);

  if (ret)
    g_variant_unref (ret);

  if (error) {
    g_warning ("Could not ask screensaver to lock: %s",
	       error->message);
    g_error_free (error);
  }
}

static void
panel_action_logout (GtkWidget *widget)
{
  /* FIXME: we need to use widget to get the screen for the
   * confirmation dialog, see
   * http://bugzilla.gnome.org/show_bug.cgi?id=536914 */
  panel_session_manager_request_logout (panel_session_manager_get (),
					PANEL_SESSION_MANAGER_LOGOUT_MODE_NORMAL);
}

static void
panel_action_shutdown (GtkWidget *widget)
{
  GDBusProxy *proxy;

  proxy = panel_session_manager_get ();
  panel_session_manager_request_shutdown (proxy);
}

static void
panel_menu_item_activate_desktop_file (GtkWidget  *menuitem,
				       const char *path)
{
  panel_launch_desktop_file (path, menuitem_to_screen (menuitem), NULL);
}

static GtkWidget *
panel_menu_item_desktop_new (char      *path,
			     char      *force_name,
			     gboolean   use_icon)
{
  GKeyFile  *key_file;
  gboolean   loaded;
  GtkWidget *item;
  char      *path_freeme;
  char      *full_path;
  char      *uri;
  char      *type;
  gboolean   is_application;
  char      *tryexec;
  char      *icon;
  char      *name;
  char      *comment;

  path_freeme = NULL;

  key_file = g_key_file_new ();

  if (g_path_is_absolute (path)) {
    loaded = g_key_file_load_from_file (key_file, path,
					G_KEY_FILE_NONE, NULL);
    full_path = path;
  } else {
    char *lookup_file;
    char *desktop_path;

    if (!g_str_has_suffix (path, ".desktop")) {
      desktop_path = g_strconcat (path, ".desktop", NULL);
    } else {
      desktop_path = path;
    }

    lookup_file = g_strconcat ("applications", G_DIR_SEPARATOR_S,
			       desktop_path, NULL);
    loaded = g_key_file_load_from_data_dirs (key_file, lookup_file,
					     &path_freeme,
					     G_KEY_FILE_NONE,
					     NULL);
    full_path = path_freeme;
    g_free (lookup_file);

    if (desktop_path != path)
      g_free (desktop_path);
  }

  if (!loaded) {
    g_key_file_free (key_file);
    if (path_freeme)
      g_free (path_freeme);
    return NULL;
  }

  /* For Application desktop files, respect TryExec */
  type = panel_key_file_get_string (key_file, "Type");
  if (!type) {
    g_key_file_free (key_file);
    if (path_freeme)
      g_free (path_freeme);
    return NULL;
  }
  is_application = (strcmp (type, "Application") == 0);
  g_free (type);

  if (is_application) {
    tryexec = panel_key_file_get_string (key_file, "TryExec");
    if (tryexec) {
      char *prog;

      prog = g_find_program_in_path (tryexec);
      g_free (tryexec);

      if (!prog) {
	/* FIXME: we could add some file monitor magic,
	 * so that the menu items appears when the
	 * program appears, but that's really complex
	 * for not a huge benefit */
	g_key_file_free (key_file);
	if (path_freeme)
	  g_free (path_freeme);
	return NULL;
      }

      g_free (prog);
    }
  }

  /* Now, simply build the menu item */
  icon    = panel_key_file_get_locale_string (key_file, "Icon");
  comment = panel_key_file_get_locale_string (key_file, "Comment");

  if (PANEL_GLIB_STR_EMPTY (force_name))
    name = panel_key_file_get_locale_string (key_file, "Name");
  else
    name = g_strdup (force_name);

  if (use_icon) {
    item = panel_image_menu_item_new ();
  } else {
    item = gtk_image_menu_item_new ();
  }

  setup_menu_item_with_icon (item, panel_menu_icon_get_size (),
			     icon, NULL, NULL, name);

  panel_util_set_tooltip_text (item, comment);

  g_signal_connect_data (item, "activate",
			 G_CALLBACK (panel_menu_item_activate_desktop_file),
			 g_strdup (full_path),
			 (GClosureNotify) g_free, 0);
  g_signal_connect (G_OBJECT (item), "button_press_event",
		    G_CALLBACK (menu_dummy_button_press_event), NULL);

  uri = g_filename_to_uri (full_path, NULL, NULL);

  setup_uri_drag (item, uri, icon);
  g_free (uri);

  g_key_file_free (key_file);

  if (icon)
    g_free (icon);

  if (name)
    g_free (name);

  if (comment)
    g_free (comment);

  if (path_freeme)
    g_free (path_freeme);

  return item;
}

static GtkWidget *
panel_menu_items_create_action_item (PanelActionButtonType action_type)
{
  GtkWidget *item;

  item = panel_image_menu_item_new ();
  setup_menu_item_with_icon (item,
			     panel_menu_icon_get_size (),
			     actions[action_type].icon_name,
			     NULL, NULL,
			     actions[action_type].text);

  panel_util_set_tooltip_text (item,
			       actions[action_type].tooltip);

  g_signal_connect (item, "activate",
		    (GCallback)actions[action_type].invoke, NULL);
  g_signal_connect (G_OBJECT (item), "button_press_event",
		    G_CALLBACK (menu_dummy_button_press_event), NULL);
  setup_internal_applet_drag (item, actions[action_type].icon_name,
			      actions[action_type].drag_id);

  return item;
}

#define GDM_FLEXISERVER_COMMAND "gdmflexiserver"
#define GDM_FLEXISERVER_ARGS    "--startnew"

static void
panel_menu_item_activate_switch_user (GtkWidget *menuitem,
				      gpointer   user_data)
{
  GdkScreen *screen;
  GAppInfo  *app_info;

#if 0
  if (panel_lockdown_get_disable_switch_user_s ())
    return;
#endif

  screen = gtk_widget_get_screen (GTK_WIDGET (menuitem));
  app_info = g_app_info_create_from_commandline (GDM_FLEXISERVER_COMMAND " " GDM_FLEXISERVER_ARGS,
						 GDM_FLEXISERVER_COMMAND,
						 G_APP_INFO_CREATE_NONE,
						 NULL);

  if (app_info) {
    GdkAppLaunchContext *launch_context;
    GdkDisplay          *display;

    display = gdk_screen_get_display (screen);
    launch_context = gdk_display_get_app_launch_context (display);
    gdk_app_launch_context_set_screen (launch_context, screen);

    g_app_info_launch (app_info, NULL,
		       G_APP_LAUNCH_CONTEXT (launch_context),
		       NULL);

    g_object_unref (launch_context);
    g_object_unref (app_info);
  }
}

static GtkWidget *
panel_menu_items_create_switch_user (gboolean use_icon)
{
  GtkWidget *item;

  if (use_icon) {
    item = panel_image_menu_item_new ();
  } else {
    item = gtk_image_menu_item_new ();
  }

  setup_menu_item_with_icon (item, panel_menu_icon_get_size (),
			     NULL, NULL, NULL, _("Switch User"));

  g_signal_connect (item, "activate",
		    G_CALLBACK (panel_menu_item_activate_switch_user),
		    NULL);
  g_signal_connect (G_OBJECT (item), "button_press_event",
		    G_CALLBACK (menu_dummy_button_press_event), NULL);

  return item;
}

static void
panel_menu_items_append_lock_logout (GtkWidget *menu)
{
  GList      *children;
  GList      *last;
  GtkWidget  *item;

  children = gtk_container_get_children (GTK_CONTAINER (menu));
  last = g_list_last (children);
  if (last != NULL &&
      GTK_IS_SEPARATOR (last->data))
    item = GTK_WIDGET (last->data);
  else
    item = add_menu_separator (menu);
  g_list_free (children);

#if 0
  panel_lockdown_on_notify (panel_lockdown_get (),
			    NULL,
			    G_OBJECT (item),
			    panel_menu_items_lock_logout_separator_notified,
			    item);
  panel_menu_items_lock_logout_separator_notified (panel_lockdown_get (),
						   item);
#endif

  item = panel_menu_items_create_action_item (PANEL_ACTION_LOCK);
  if (item != NULL) {
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
#if 0
    g_object_bind_property (panel_lockdown_get (),
			    "disable-lock-screen",
			    item,
			    "visible",
			    G_BINDING_SYNC_CREATE|G_BINDING_INVERT_BOOLEAN);
#endif
  }

  item = panel_menu_items_create_switch_user (FALSE);

  if (item != NULL) {
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
#if 0
    g_object_bind_property (panel_lockdown_get (),
			    "disable-switch-user",
			    item,
			    "visible",
			    G_BINDING_SYNC_CREATE|G_BINDING_INVERT_BOOLEAN);
#endif
  }

  item = panel_menu_items_create_action_item (PANEL_ACTION_LOGOUT);

  if (item != NULL) {
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
#if 0
    g_object_bind_property (panel_lockdown_get (),
			    "disable-log-out",
			    item,
			    "visible",
			    G_BINDING_SYNC_CREATE|G_BINDING_INVERT_BOOLEAN);
#endif
  }

  /* FIXME: should be dynamic */
  if (panel_session_manager_is_shutdown_available (panel_session_manager_get ())) {
    item = panel_menu_items_create_action_item (PANEL_ACTION_SHUTDOWN);
    if (item != NULL && !g_getenv("LTSP_CLIENT")) {
      GtkWidget *sep;

      sep = add_menu_separator (menu);

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

#if 0
      g_object_bind_property (panel_lockdown_get (),
			      "disable-log-out",
			      sep,
			      "visible",
			      G_BINDING_SYNC_CREATE|G_BINDING_INVERT_BOOLEAN);
      g_object_bind_property (panel_lockdown_get (),
			      "disable-log-out",
			      item,
			      "visible",
			      G_BINDING_SYNC_CREATE|G_BINDING_INVERT_BOOLEAN);
#endif
    }
  }
}

static void
sysmenu_append_menu (GtkWidget *menu,
		     gpointer   data)
{
  SysMenuApplet *applet;
  gboolean       add_separator;
  GList         *children;
  GList         *last;

  applet = (SysMenuApplet *) data;

  add_separator = FALSE;
  children = gtk_container_get_children (GTK_CONTAINER (menu));
  last = g_list_last (children);

  if (last != NULL)
    add_separator = !GTK_IS_SEPARATOR (GTK_WIDGET (last->data));

  g_list_free (children);

  if (add_separator)
    add_menu_separator (menu);

  GtkWidget *item;

  item = panel_menu_item_desktop_new ("yelp.desktop", NULL, TRUE);
  if (item)
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = panel_menu_item_desktop_new ("gnome-about.desktop", NULL, TRUE);
  if (item)
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  panel_menu_items_append_lock_logout (menu);
}

static GtkWidget *
sysmenu_create_menu (SysMenuApplet *sysmenu)
{
  GtkWidget *menu;

  menu = create_applications_menu ("gnome-settings.menu", NULL, TRUE);

  g_object_set_data (G_OBJECT (menu), "menu_applet", sysmenu);

  g_object_set_data (G_OBJECT (menu),
		     "panel-menu-append-callback",
		     sysmenu_append_menu);
  g_object_set_data (G_OBJECT (menu),
		     "panel-menu-append-callback-data",
		     sysmenu);

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

static void
setenvf (const char *envname, const char *fmt, ...)
{
  va_list args;
  gchar *buffer;
  va_start (args, fmt);
  g_vasprintf (&buffer, fmt, args);
  va_end (args);
  setenv (envname, buffer, TRUE);
  g_free (buffer);
}

static void
prepend_env_path (const char *envname, const char *path, const char *defpath)
{
  const char *oldenv = getenv (envname);
  if (oldenv == NULL || !*oldenv)
    oldenv = defpath;
  setenvf (envname, "%s:%s", path, oldenv);
}

static void
setup_environment (void)
{
  prepend_env_path ("XDG_CONFIG_DIRS", EXTRA_XDG_DATA_DIR, "/etc/xdg");
  prepend_env_path ("XDG_DATA_DIRS", EXTRA_XDG_DATA_DIR, "/usr/local/share/:/usr/share/");
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

#define main factory_main
PANEL_APPLET_OUT_PROCESS_FACTORY ("SysMenuAppletFactory",
				  PANEL_TYPE_APPLET,
				  sysmenu_applet_factory,
				  NULL)
#undef main

int main (int argc, char *argv [])
{
  setup_environment ();
  return factory_main (argc, argv);
}

