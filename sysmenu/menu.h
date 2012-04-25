/*
 * Copyright (C) 1997 - 2000 The Free Software Foundation
 * Copyright (C) 2000 Helix Code, Inc.
 * Copyright (C) 2000 Eazel, Inc.
 * Copyright (C) 2004 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

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
