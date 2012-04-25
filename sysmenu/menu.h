extern void
setup_menuitem (GtkWidget   *menuitem,
		GtkIconSize  icon_size,
		GtkWidget   *image,
		const char  *title);
extern GtkWidget *
create_applications_menu (const char *menu_file,
			  const char *menu_path,
			  gboolean    always_show_image);
extern GtkWidget *
add_menu_separator (GtkWidget *menu);
extern GdkScreen *
menuitem_to_screen (GtkWidget *menuitem);
extern GtkWidget *
panel_image_menu_item_new (void);
extern gboolean
menu_dummy_button_press_event (GtkWidget      *menuitem,
			       GdkEventButton *event);
GtkIconSize
panel_menu_icon_get_size (void);
extern void
setup_menu_item_with_icon (GtkWidget   *item,
			   GtkIconSize  icon_size,
			   const char  *icon_name,
			   const char  *stock_id,
			   GIcon       *gicon,
			   const char  *title);
extern void
setup_uri_drag (GtkWidget  *menuitem,
		const char *uri,
		const char *icon);
extern void
setup_internal_applet_drag (GtkWidget  *menuitem,
			    const char *icon_name,
			    const char *drag_id);
