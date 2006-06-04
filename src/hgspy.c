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
	GtkWidget  *window;
	GtkWidget  *visualizer;
	GThread    *vm_thread;
	LibrettoVM *vm;
	gchar      *file;
};

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

/*
 * Public Functions
 */
int
main(int    argc,
     char **argv)
{
	GModule *module;
	HgSpy *spy;
	GtkWidget *menubar, *vbox;
	GtkUIManager *uiman;
	GtkActionGroup *actions;
	GtkActionEntry action_entries[] = {
		/* toplevel menu */
		{
			.name = "VMMenu",
			.stock_id = NULL,
			.label = _("_VM"),
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
		"    <menu action='HelpMenu'>"
		"      <menuitem action='About'/>"
		"    </menu>"
		"  </menubar>"
		"</ui>";

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
	uiman = gtk_ui_manager_new();
	vbox = gtk_vbox_new(FALSE, 0);
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
	gtk_ui_manager_add_ui_from_string(uiman, uixml, strlen(uixml), NULL);
	gtk_ui_manager_insert_action_group(uiman, actions, 0);

	/* setup accelerators */
	gtk_window_add_accel_group(GTK_WINDOW (spy->window), gtk_ui_manager_get_accel_group(uiman));

	/* widget connections */
	menubar = gtk_ui_manager_get_widget(uiman, "/MenuBar");
	gtk_box_pack_start(GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (vbox), spy->visualizer, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER (spy->window), vbox);

	/* setup signals */
	g_signal_connect(spy->window, "delete-event",
			 G_CALLBACK (gtk_main_quit), NULL);

	gtk_widget_show_all(spy->window);

	gtk_main();

	/* finalize */
	g_module_close(module);

	return 0;
}
