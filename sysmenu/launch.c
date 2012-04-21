#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

typedef char * (*LookupInDir) (const char *basename, const char *dir);

static char *
_lookup_in_applications_subdir (const char *basename,
				const char *dir)
{
  char *path;

  path = g_build_filename (dir, "applications", basename, NULL);
  if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
    g_free (path);
    return NULL;
  }

  return path;
}

static char *
_panel_g_lookup_in_data_dirs_internal (const char *basename,
				       LookupInDir lookup)
{
  const char * const *system_data_dirs;
  const char          *user_data_dir;
  char                *retval;
  int                  i;

  user_data_dir    = g_get_user_data_dir ();
  system_data_dirs = g_get_system_data_dirs ();

  if ((retval = lookup (basename, user_data_dir)))
    return retval;

  for (i = 0; system_data_dirs[i]; i++)
    if ((retval = lookup (basename, system_data_dirs[i])))
      return retval;

  return NULL;
}

static char *
panel_g_lookup_in_applications_dirs (const char *basename)
{
  return _panel_g_lookup_in_data_dirs_internal (basename,
						_lookup_in_applications_subdir);
}

static GtkWidget *
panel_error_dialog (GtkWindow  *parent,
		    GdkScreen  *screen,
		    const char *dialog_class,
		    gboolean    auto_destroy,
		    const char *primary_text,
		    const char *secondary_text)
{
  GtkWidget *dialog;
  char      *freeme;

  freeme = NULL;

  if (primary_text == NULL) {
    g_warning ("NULL dialog");
    /* No need to translate this, this should NEVER happen */
    freeme = g_strdup_printf ("Error with displaying error "
			      "for dialog of class %s",
			      dialog_class);
    primary_text = freeme;
  }

  dialog = gtk_message_dialog_new (parent, 0, GTK_MESSAGE_ERROR,
				   GTK_BUTTONS_CLOSE, "%s", primary_text);
  if (secondary_text != NULL)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
					      "%s", secondary_text);

  if (screen)
    gtk_window_set_screen (GTK_WINDOW (dialog), screen);

  if (!parent) {
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), FALSE);
    /* FIXME: We need a title in this case, but we don't know what
     * the format should be. Let's put something simple until
     * the following bug gets fixed:
     * http://bugzilla.gnome.org/show_bug.cgi?id=165132 */
    gtk_window_set_title (GTK_WINDOW (dialog), _("Error"));
  }

  gtk_widget_show_all (dialog);

  if (auto_destroy)
    g_signal_connect_swapped (G_OBJECT (dialog), "response",
			      G_CALLBACK (gtk_widget_destroy),
			      G_OBJECT (dialog));

  if (freeme)
    g_free (freeme);

  return dialog;
}

static void
_panel_launch_error_dialog (const gchar *name,
			    GdkScreen   *screen,
			    const gchar *message)
{
  char *primary;

  if (name)
    primary = g_markup_printf_escaped (_("Could not launch '%s'"),
				       name);
  else
    primary = g_strdup (_("Could not launch application"));

  panel_error_dialog (NULL, screen, "cannot_launch", TRUE,
		      primary, message);
  g_free (primary);
}

static gboolean
_panel_launch_handle_error (const gchar  *name,
			    GdkScreen    *screen,
			    GError       *local_error,
			    GError      **error)
{
  if (local_error == NULL)
    return TRUE;

  else if (g_error_matches (local_error,
			    G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    g_error_free (local_error);
    return TRUE;
  }

  else if (error != NULL)
    g_propagate_error (error, local_error);

  else {
    _panel_launch_error_dialog (name, screen, local_error->message);
    g_error_free (local_error);
  }

  return FALSE;
}

static gboolean
panel_app_info_launch_uris (GAppInfo   *appinfo,
			    GList      *uris,
			    GdkScreen  *screen,
			    guint32     timestamp,
			    GError    **error)
{
  GdkAppLaunchContext *context;
  GError              *local_error;
  GdkDisplay          *display;

  g_return_val_if_fail (G_IS_APP_INFO (appinfo), FALSE);
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  display = gdk_screen_get_display (screen);
  context = gdk_display_get_app_launch_context (display);
  gdk_app_launch_context_set_screen (context, screen);
  gdk_app_launch_context_set_timestamp (context, timestamp);

  local_error = NULL;
  g_app_info_launch_uris (appinfo, uris,
			  (GAppLaunchContext *) context,
			  &local_error);

  g_object_unref (context);

  return _panel_launch_handle_error (g_app_info_get_name (appinfo),
				     screen, local_error, error);
}

gboolean
panel_launch_desktop_file (const char  *desktop_file,
			   GdkScreen   *screen,
			   GError     **error)
{
  GDesktopAppInfo *appinfo;
  gboolean         retval;

  g_return_val_if_fail (desktop_file != NULL, FALSE);
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  appinfo = NULL;

  if (g_path_is_absolute (desktop_file))
    appinfo = g_desktop_app_info_new_from_filename (desktop_file);
  else {
    char *full;

    full = panel_g_lookup_in_applications_dirs (desktop_file);
    if (full) {
      appinfo = g_desktop_app_info_new_from_filename (full);
      g_free (full);
    }
  }

  if (appinfo == NULL)
    return FALSE;

  retval = panel_app_info_launch_uris (G_APP_INFO (appinfo), NULL, screen,
				       gtk_get_current_event_time (),
				       error);

  g_object_unref (appinfo);

  return retval;
}
