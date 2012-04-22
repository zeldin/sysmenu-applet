#define PANEL_GLIB_STR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')

extern void
panel_util_set_tooltip_text (GtkWidget  *widget,
			     const char *text);
extern char *
panel_find_icon (GtkIconTheme  *icon_theme,
		 const char    *icon_name,
		 gint           size);
