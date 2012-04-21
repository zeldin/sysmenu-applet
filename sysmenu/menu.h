extern void
setup_menuitem (GtkWidget   *menuitem,
		GtkIconSize  icon_size,
		GtkWidget   *image,
		const char  *title);
extern GtkWidget *
create_applications_menu (const char *menu_file,
			  const char *menu_path,
			  gboolean    always_show_image);
