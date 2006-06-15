/* 
 * hgspy.c
 * Copyright (C) 2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <hieroglyph/hgmem.h>
#include <libretto/vm.h>
#include "visualizer.h"


typedef struct _HieroGlyphSpy	HgSpy;

struct _HieroGlyphSpy {
	GtkWidget    *window;
	GtkWidget    *visualizer;
	GtkWidget    *total_vm_size;
	GtkWidget    *used_vm_size;
	GtkWidget    *free_vm_size;
	GtkUIManager *ui;
	GThread      *vm_thread;
	LibrettoVM   *vm;
	gchar        *file;
};

static void _hgspy_update_vm_status(HgMemoryVisualizer *visual,
				    HgSpy              *spy,
				    const gchar        *name);

static gpointer __hg_spy_helper_get_widget = NULL;

/*
 * Private Functions
 */
static gpointer
_hgspy_vm_thread(gpointer data)
{
	HG_MEM_INIT;

	HgSpy *spy = data;

	libretto_vm_init();
	spy->vm = libretto_vm_new(LB_EMULATION_LEVEL_1);
	libretto_vm_startjob(spy->vm, spy->file, TRUE);
	libretto_vm_finalize();

	return NULL;
}

static void
_hgspy_run_vm(HgSpy       *spy,
	      const gchar *file)
{
	if (spy->vm_thread != NULL) {
	}
	spy->file = g_strdup(file);
	spy->vm_thread = g_thread_create(_hgspy_vm_thread, spy, FALSE, NULL);
}

static void
_hgspy_about_email_cb(GtkAboutDialog *about,
		      const gchar    *link,
		      gpointer        data)
{
}

static void
_hgspy_about_url_cb(GtkAboutDialog *about,
		    const gchar    *link,
		    gpointer        data)
{
}

static void
_hgspy_action_menubar_run_cb(GtkAction *action,
			      HgSpy     *spy)
{
	_hgspy_run_vm(spy, NULL);
}

static void
_hgspy_action_menubar_open_cb(GtkAction *action,
			      HgSpy     *spy)
{
	GtkWidget *dialog;
	gint result;
	GtkFileFilter *filter;

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("PostScript file"));
	gtk_file_filter_add_mime_type(filter, "application/postscript");

	dialog = gtk_file_chooser_dialog_new(_("Open a PostScript file"),
					     GTK_WINDOW (spy->window),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_OK,
					     NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	result = gtk_dialog_run(GTK_DIALOG (dialog));
	switch (result) {
	    case GTK_RESPONSE_OK:
		    _hgspy_run_vm(spy,
				  gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog)));
		    break;
	    case GTK_RESPONSE_CANCEL:
	    default:
		    break;
	}
	gtk_widget_destroy(dialog);
}

static void
_hgspy_action_menubar_quit_cb(GtkAction *action,
			      HgSpy     *spy)
{
	gtk_main_quit();
}

static void
_hgspy_action_menubar_about_cb(GtkAction *action,
			       HgSpy     *spy)
{
	const gchar *authors[] = {
		"Akira TAGOH",
		NULL
	};
	const gchar *license =
		"This library is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU Lesser General Public\n"
		"License as published by the Free Software Foundation; either\n"
		"version 2 of the License, or (at your option) any later version.\n"
		"\n"
		"This library is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
		"Lesser General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU Lesser General Public\n"
		"License along with this library; if not, write to the\n"
		"Free Software Foundation, Inc., 59 Temple Place - Suite 330,\n"
		"Boston, MA 02111-1307, USA.\n";

	gtk_about_dialog_set_email_hook(_hgspy_about_email_cb, NULL, NULL);
	gtk_about_dialog_set_url_hook(_hgspy_about_url_cb, NULL, NULL);
	gtk_show_about_dialog(GTK_WINDOW (spy->window),
			      "name", "Memory visualizer",
			      "version", PACKAGE_VERSION,
			      "copyright", "(C) 2006 Akira TAGOH",
			      "license", license,
			      "website", "http://hieroglyph.freedesktop.org/",
			      "comments", "for Hieroglyph memory management system",
			      "authors", authors,
			      NULL);
}

static void
_hgspy_radio_menu_pool_activate_cb(GtkMenuItem *menuitem,
				   gpointer     data)
{
	const gchar *name;
	HgSpy *spy = data;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM (menuitem))) {
		name = g_object_get_data(G_OBJECT (menuitem), "user-data");
		hg_memory_visualizer_change_pool(HG_MEMORY_VISUALIZER (spy->visualizer), name);
	}
}

static void
_hgspy_pool_updated_cb(HgMemoryVisualizer *visual,
		       gpointer            list,
		       gpointer            data)
{
	GSList *slist = list, *l, *sigwidget = NULL;
	HgSpy *spy = data;
	GtkWidget *bin, *menu, *menuitem = NULL;
	gulong sigid;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));

	bin = gtk_ui_manager_get_widget(spy->ui, "/MenuBar/ViewMenu/PoolMenu");
	menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM (bin));
	sigwidget = (GSList *)g_object_get_data(G_OBJECT (menu), "signal-widget-list");
	for (l = sigwidget; l != NULL; l = g_slist_next(l)) {
		sigid = (gulong)g_object_get_data(G_OBJECT (l->data), "signal-id");
		if (sigid > 0)
			g_signal_handler_disconnect(l->data, sigid);
	}
	gtk_menu_item_remove_submenu(GTK_MENU_ITEM (bin));
	menu = gtk_menu_new();
	sigwidget = NULL;
	for (l = slist; l != NULL; l = g_slist_next(l)) {
		if (menuitem == NULL) {
			menuitem = gtk_radio_menu_item_new_with_label(NULL, l->data);
		} else {
			menuitem = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM (menuitem), l->data);
		}
		sigid = g_signal_connect(menuitem, "activate",
					 G_CALLBACK (_hgspy_radio_menu_pool_activate_cb), spy);
		sigwidget = g_slist_append(sigwidget, menuitem);
		g_object_set_data(G_OBJECT (menuitem), "signal-id", (gpointer)sigid);
		g_object_set_data(G_OBJECT (menuitem), "user-data", l->data);
		gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_show(menuitem);
	}
	if (sigwidget == NULL) {
		menuitem = gtk_menu_item_new_with_label(_("None"));
		gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_set_sensitive(menuitem, FALSE);
		gtk_widget_show(menuitem);
	}
	g_object_set_data(G_OBJECT (menu), "signal-widget-list", sigwidget);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM (bin), menu);
}

static void
_hgspy_draw_updated_cb(HgMemoryVisualizer *visual,
		       gpointer            data)
{
	HgSpy *spy = data;
	const gchar *name;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (data != NULL);

	name = hg_memory_visualizer_get_current_pool_name(visual);
	_hgspy_update_vm_status(visual, spy, name);
}

static void
_hgspy_update_vm_status(HgMemoryVisualizer *visual,
			HgSpy              *spy,
			const gchar        *name)
{
	gsize total, used;
	gchar *p;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (spy != NULL);

	if (name) {
		total = hg_memory_visualizer_get_max_size(visual, name);
		p = g_strdup_printf("%d", total);
		gtk_entry_set_text(GTK_ENTRY (spy->total_vm_size), p);
		g_free(p);
		used = hg_memory_visualizer_get_used_size(visual, name);
		p = g_strdup_printf("%d", used);
		gtk_entry_set_text(GTK_ENTRY (spy->used_vm_size), p);
		g_free(p);
		p = g_strdup_printf("%d", total - used);
		gtk_entry_set_text(GTK_ENTRY (spy->free_vm_size), p);
		g_free(p);
	} else {
		gtk_entry_set_text(GTK_ENTRY (spy->total_vm_size), "");
		gtk_entry_set_text(GTK_ENTRY (spy->used_vm_size), "");
		gtk_entry_set_text(GTK_ENTRY (spy->free_vm_size), "");
	}
}

/*
 * Public Functions
 */
int
main(int    argc,
     char **argv)
{
	GModule *module;
	HgSpy *spy;
	GtkWidget *menubar, *vbox, *none, *table, *label;
	GtkActionGroup *actions;
	GtkActionEntry action_entries[] = {
		/* toplevel menu */
		{
			.name = "VMMenu",
			.stock_id = NULL,
			.label = _("V_M"),
			.accelerator = NULL,
			.tooltip = NULL,
			.callback = NULL
		},
		{
			.name = "ViewMenu",
			.stock_id = NULL,
			.label = _("_View"),
			.accelerator = NULL,
			.tooltip = NULL,
			.callback = NULL
		},
		{
			.name = "HelpMenu",
			.stock_id = NULL,
			.label = _("_Help"),
			.accelerator = NULL,
			.tooltip = NULL,
			.callback = NULL
		},
		/* submenu - VM */
		{
			.name = "Run",
			.stock_id = GTK_STOCK_EXECUTE,
			.label = NULL,
			.accelerator = NULL,
			.tooltip = _("Run VM interactively"),
			.callback = G_CALLBACK (_hgspy_action_menubar_run_cb)
		},
		{
			.name = "Open",
			.stock_id = GTK_STOCK_OPEN,
			.label = NULL,
			.accelerator = "<control>O",
			.tooltip = _("Run a PostScript file on VM"),
			.callback = G_CALLBACK (_hgspy_action_menubar_open_cb)
		},
		{
			.name = "Quit",
			.stock_id = GTK_STOCK_QUIT,
			.label = NULL,
			.accelerator = "<control>Q",
			.tooltip = _("Quit"),
			.callback = G_CALLBACK (_hgspy_action_menubar_quit_cb)
		},
		/* submenu - View */
		{
			.name = "PoolMenu",
			.stock_id = NULL,
			.label = _("Pool"),
			.accelerator = NULL,
			.tooltip = _("Pool to be visualized."),
			.callback = NULL
		},
		/* submenu - Pool */
		{
			.name = "PoolNone",
			.stock_id = NULL,
			.label = _("None"),
			.accelerator = NULL,
			.tooltip = NULL,
			.callback = NULL
		},
		/* submenu - Help */
		{
			.name = "About",
			.stock_id = GTK_STOCK_ABOUT,
			.label = NULL,
			.accelerator = NULL,
			.tooltip = _("About"),
			.callback = G_CALLBACK (_hgspy_action_menubar_about_cb)
		}
	};
	gchar *uixml =
		"<ui>"
		"  <menubar name='MenuBar'>"
		"    <menu action='VMMenu'>"
		"      <menuitem action='Run'/>"
		"      <menuitem action='Open'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
		"    </menu>"
		"    <menu action='ViewMenu'>"
		"      <menu action='PoolMenu'>"
		"        <menuitem action='PoolNone'/>"
		"      </menu>"
		"    </menu>"
		"    <menu action='HelpMenu'>"
		"      <menuitem action='About'/>"
		"    </menu>"
		"  </menubar>"
		"</ui>";
	guint i = 0;

	if ((module = g_module_open("libhgspy-helper.so", 0)) == NULL) {
		g_warning("Failed g_module_open: %s", g_module_error());
		return 1;
	}
	if (!g_module_symbol(module, "hg_spy_helper_get_widget", &__hg_spy_helper_get_widget)) {
		g_warning("Failed g_module_symbol: %s", g_module_error());
		g_module_close(module);
		return 1;
	}

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, HGSPY_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* HAVE_BIND_TEXTDOMAIN_CODESET */
	textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

	g_thread_init(NULL);
	gtk_init(&argc, &argv);

	spy = g_new(HgSpy, 1);
	spy->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	spy->vm_thread = NULL;
	actions = gtk_action_group_new("MenuBar");
	spy->ui = gtk_ui_manager_new();
	vbox = gtk_vbox_new(FALSE, 0);
	table = gtk_table_new(3, 2, FALSE);
	spy->total_vm_size = gtk_entry_new();
	spy->used_vm_size = gtk_entry_new();
	spy->free_vm_size = gtk_entry_new();
	spy->visualizer = ((GtkWidget * (*) (void))__hg_spy_helper_get_widget) ();

	if (spy->visualizer == NULL) {
		g_warning("Failed to initialize a helper module.");
		return 1;
	}

	/* setup action groups */
	gtk_action_group_add_actions(actions,
				     action_entries,
				     sizeof (action_entries) / sizeof (GtkActionEntry),
				     spy);

	/* setup UI */
	gtk_window_set_default_size(GTK_WINDOW (spy->window), 100, 100);
	gtk_window_set_title(GTK_WINDOW (spy->window), "Memory Visualizer for Hieroglyph");
	gtk_ui_manager_add_ui_from_string(spy->ui, uixml, strlen(uixml), NULL);
	gtk_ui_manager_insert_action_group(spy->ui, actions, 0);
	none = gtk_ui_manager_get_widget(spy->ui, "/MenuBar/ViewMenu/PoolMenu/PoolNone");
	gtk_widget_set_sensitive(none, FALSE);
	gtk_editable_set_editable(GTK_EDITABLE (spy->total_vm_size), FALSE);
	gtk_editable_set_editable(GTK_EDITABLE (spy->used_vm_size), FALSE);
	gtk_editable_set_editable(GTK_EDITABLE (spy->free_vm_size), FALSE);
	GTK_WIDGET_UNSET_FLAGS (spy->total_vm_size, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (spy->used_vm_size, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (spy->free_vm_size, GTK_CAN_FOCUS);

	/* setup accelerators */
	gtk_window_add_accel_group(GTK_WINDOW (spy->window), gtk_ui_manager_get_accel_group(spy->ui));

	/* widget connections */
	menubar = gtk_ui_manager_get_widget(spy->ui, "/MenuBar");
	gtk_box_pack_start(GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (vbox), spy->visualizer, TRUE, TRUE, 0);
	/* total */
	label = gtk_label_new(_("Total memory size:"));
	gtk_table_attach(GTK_TABLE (table), label,
			 0, 1, i, i + 1,
			 0, GTK_FILL,
			 0, 0);
	gtk_table_attach(GTK_TABLE (table), spy->total_vm_size,
			 1, 2, i, i + 1,
			 GTK_FILL | GTK_EXPAND, GTK_FILL,
			 0, 0);
	i++;
	/* used */
	label = gtk_label_new(_("Used memory size:"));
	gtk_table_attach(GTK_TABLE (table), label,
			 0, 1, i, i + 1,
			 0, GTK_FILL,
			 0, 0);
	gtk_table_attach(GTK_TABLE (table), spy->used_vm_size,
			 1, 2, i, i + 1,
			 GTK_FILL | GTK_EXPAND, GTK_FILL,
			 0, 0);
	i++;
	/* free */
	label = gtk_label_new(_("Free memory size:"));
	gtk_table_attach(GTK_TABLE (table), label,
			 0, 1, i, i + 1,
			 0, GTK_FILL,
			 0, 0);
	gtk_table_attach(GTK_TABLE (table), spy->free_vm_size,
			 1, 2, i, i + 1,
			 GTK_FILL | GTK_EXPAND, GTK_FILL,
			 0, 0);
	i++;

	gtk_box_pack_end(GTK_BOX (vbox), table, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER (spy->window), vbox);

	/* setup signals */
	g_signal_connect(spy->visualizer, "pool-updated",
			 G_CALLBACK (_hgspy_pool_updated_cb), spy);
	g_signal_connect(spy->visualizer, "draw-updated",
			 G_CALLBACK (_hgspy_draw_updated_cb), spy);
	g_signal_connect(spy->window, "delete-event",
			 G_CALLBACK (gtk_main_quit), NULL);

	gtk_widget_show_all(spy->window);

	gtk_main();

	/* finalize */
	g_module_close(module);

	return 0;
}
