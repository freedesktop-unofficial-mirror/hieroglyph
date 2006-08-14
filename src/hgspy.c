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
#include <errno.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgstack.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/vm.h>
#include <hieroglyph/operator.h>
#include "visualizer.h"


typedef struct _HieroGlyphSpy	HgSpy;

struct _HieroGlyphSpy {
	GtkWidget    *window;
	GtkWidget    *status;
	GtkWidget    *visualizer;
	GtkWidget    *total_vm_size;
	GtkWidget    *used_vm_size;
	GtkWidget    *free_vm_size;
	GtkWidget    *textview;
	GtkWidget    *entry;
	GtkWidget    *prompt;
	GtkUIManager *ui;
	GThread      *vm_thread;
	HgVM         *vm;
	gchar        *file;
	gint          error;
	gchar        *statementedit_buffer;
	gboolean      destroyed;
	guint         gc_status_ctxt_id;
	guint         gc_status_msg_id;
};

static void _hgspy_update_vm_status         (HgMemoryVisualizer *visual,
					     HgSpy              *spy,
					     const gchar        *name);
static void _hgspy_insert_text_into_textview(HgSpy              *spy,
					     const gchar        *text,
					     gsize               length);

static gpointer __hg_spy_helper_get_widget = NULL;

/*
 * Private Functions
 */
static gboolean
_hgspy_ask_dialog(HgSpy       *spy,
		  const gchar *message)
{
	gboolean retval = TRUE;
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW (spy->window),
						   GTK_DIALOG_MODAL,
						   GTK_MESSAGE_QUESTION,
						   GTK_BUTTONS_YES_NO,
						   _(message));
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result) {
	    case GTK_RESPONSE_YES:
		    retval = FALSE;
		    break;
	    case GTK_RESPONSE_NO:
	    default:
		    break;
	}

	gtk_widget_destroy(dialog);

	return !retval;
}

static gboolean
_hgspy_op_private_statementedit(HgOperator *op,
				gpointer    data)
{
	HgVM *vm = data;
	gboolean retval = FALSE;
	gsize ret;
	HgFileObject *stdin = hg_vm_get_io(vm, VM_IO_STDIN), *file;
	gchar buffer[1025];
	HgMemPool *pool = hg_vm_get_current_pool(vm);
	HgValueNode *node;
	HgStack *ostack = hg_vm_get_ostack(vm);

	while (1) {
		ret = hg_file_object_read(stdin, buffer, sizeof (gchar), 1024);
		if (ret == 0) {
			_hg_operator_set_error(vm, op, VM_e_undefinedfilename);
			break;
		}
		file = hg_file_object_new(pool, HG_FILE_TYPE_BUFFER,
					  HG_FILE_MODE_READ,
					  "%statementedit",
					  buffer, -1);
		if (file == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		HG_VALUE_MAKE_FILE (node, file);
		if (node == NULL) {
			_hg_operator_set_error(vm, op, VM_e_VMerror);
			break;
		}
		retval = hg_stack_push(ostack, node);
		break;
	}

	return retval;
}

static gsize
_hgspy_file_read_cb(gpointer user_data,
		    gpointer buffer,
		    gsize    size,
		    gsize    n)
{
	HgSpy *spy = user_data;
	gsize retval = 0;
	HgStack *ostack = hg_vm_get_ostack(spy->vm);
	gchar *prompt;
	/* depends on hg_init.ps. */
	guint depth = hg_stack_depth(ostack) - 2;

	if (depth > 0)
		prompt = g_strdup_printf("PS[%d]>", depth);
	else
		prompt = g_strdup("PS>");

	gdk_threads_enter();
	gtk_label_set_text(GTK_LABEL (spy->prompt), prompt);
	gdk_flush();
	gdk_threads_leave();

	g_free(prompt);

	while (spy->statementedit_buffer == NULL && !spy->destroyed)
		sleep(1);

	if (spy->statementedit_buffer) {
		size_t len = strlen(spy->statementedit_buffer);
		gpointer p;

		if (len > size * n)
			retval = size * n;
		else
			retval = len;
		strncpy(buffer, spy->statementedit_buffer, retval);
		((gchar *)buffer)[retval] = 0;
		p = spy->statementedit_buffer;
		spy->statementedit_buffer = NULL;
		g_free(p);

		/* to be thread-safe */
		gdk_threads_enter();

		gtk_widget_set_sensitive(spy->entry, TRUE);
		gtk_widget_grab_focus(spy->entry);

		gdk_flush();
		gdk_threads_leave();
	}

	return retval;
}

static gsize
_hgspy_file_write_cb(gpointer      user_data,
		     gconstpointer buffer,
		     gsize         size,
		     gsize         n)
{
	HgSpy *spy = user_data;

	/* to be thread-safe */
	gdk_threads_enter();

	_hgspy_insert_text_into_textview(spy, buffer, size * n);

	gdk_flush();
	gdk_threads_leave();

	spy->error = 0;

	return size * n;
}

static gchar
_hgspy_file_getc_cb(gpointer user_data)
{
	g_print("FIXME: getc\n");

	return 0;
}

static gboolean
_hgspy_file_flush_cb(gpointer user_data)
{
	g_print("FIXME: flush\n");

	return FALSE;
}

static gssize
_hgspy_file_seek_cb(gpointer      user_data,
		    gssize        offset,
		    HgFilePosType whence)
{
	HgSpy *spy = user_data;

	g_print("FIXME: seek.\n");
	spy->error = ESPIPE;

	return -1;
}

static void
_hgspy_file_close_cb(gpointer user_data)
{
	g_print("FIXME: close.\n");
}

static gboolean
_hgspy_file_is_eof_cb(gpointer user_data)
{
	return FALSE;
}

static void
_hgspy_file_clear_eof_cb(gpointer user_data)
{
	g_print("FIXME: clear_eof\n");
}

static gint
_hgspy_file_get_error_code_cb(gpointer user_data)
{
	HgSpy *spy = user_data;

	return spy->error;
}

static gpointer
_hgspy_vm_thread(gpointer data)
{
	HG_MEM_INIT;

	HgSpy *spy = data;
	HgFileObjectCallback callback = {
		.read           = _hgspy_file_read_cb,
		.write          = _hgspy_file_write_cb,
		.getc           = _hgspy_file_getc_cb,
		.flush          = _hgspy_file_flush_cb,
		.seek           = _hgspy_file_seek_cb,
		.close          = _hgspy_file_close_cb,
		.is_eof         = _hgspy_file_is_eof_cb,
		.clear_eof      = _hgspy_file_clear_eof_cb,
		.get_error_code = _hgspy_file_get_error_code_cb,
	};
	HgFileObject * volatile in, * volatile out;
	HgMemPool *pool;
	HgOperator *op;
	HgValueNode *key, *val;
	HgDict *dict;
	gint fd;
	gchar *filename = NULL;
	GString *buffer;

	hg_vm_init();
	spy->vm = hg_vm_new(VM_EMULATION_LEVEL_1);
	pool = hg_vm_get_current_pool(spy->vm);
	dict = hg_vm_get_dict_systemdict(spy->vm);
	in = hg_file_object_new(pool,
				HG_FILE_TYPE_BUFFER_WITH_CALLBACK,
				HG_FILE_MODE_READ,
				"stdin with callback",
				&callback,
				spy);
	out = hg_file_object_new(pool,
				 HG_FILE_TYPE_BUFFER_WITH_CALLBACK,
				 HG_FILE_MODE_WRITE,
				 "stdout with callback",
				 &callback,
				 spy);
	hg_vm_set_io(spy->vm, VM_IO_STDIN, in);
	hg_vm_set_io(spy->vm, VM_IO_STDOUT, out);
	hg_vm_set_io(spy->vm, VM_IO_STDERR, out);

	op = hg_operator_new(pool, "...statementedit", _hgspy_op_private_statementedit);
	if (op == NULL) {
		g_warning("Failed to create an operator");
		return NULL;
	}
	key = hg_vm_get_name_node(spy->vm, "...statementedit");
	HG_VALUE_MAKE_POINTER (val, op);
	if (val == NULL) {
		hg_vm_set_error(spy->vm, key, VM_e_VMerror, FALSE);
	} else {
		hg_object_executable((HgObject *)val);
		hg_dict_insert(pool, dict, key, val);
	}
	fd = g_file_open_tmp("hgspy-XXXXXX", &filename, NULL);
	if (fd == -1) {
		g_warning("Failed to create an initialize file.");
		return NULL;
	}
	buffer = g_string_new(NULL);
	g_string_append(buffer, "/..statementedit {//null //null ...statementedit exch pop exch pop} bind def ");
	if (spy->file != NULL) {
		g_string_append_printf(buffer, "(%s) run", spy->file);
	} else {
		g_string_append(buffer, ".startjobserver executive");
	}
	write(fd, buffer->str, buffer->len);
	close(fd);
	hg_vm_startjob(spy->vm, filename, TRUE);
	unlink(filename);
	g_free(filename);
	hg_mem_free(spy->vm);
	spy->vm = NULL;
	hg_vm_finalize();
	spy->destroyed = FALSE;

	return NULL;
}

static void
_hgspy_run_vm(HgSpy       *spy,
	      const gchar *file)
{
	if (spy->vm_thread != NULL) {
		if (!_hgspy_ask_dialog(spy, N_("HieroglyphVM is still running.\nAre you sure that you really want to restart?"))) {
			return;
		}
		g_thread_exit(spy->vm_thread);
		hg_mem_free(spy->vm);
		hg_vm_finalize();
	}
	spy->file = g_strdup(file);
	spy->vm_thread = g_thread_create(_hgspy_vm_thread, spy, FALSE, NULL);
}

static gboolean
_hgspy_quit_cb(GtkWidget   *widget,
	       GdkEventAny *event,
	       gpointer     data)
{
	HgSpy *spy = data;
	gboolean retval = TRUE;

	if (spy->vm) {
		retval = _hgspy_ask_dialog(spy, N_("Hieroglyph VM is still running.\nAre you sure that you really want to quit?"));
	} else {
		retval = FALSE;
	}
	if (retval) {
		gtk_widget_unrealize(spy->window);
		gtk_main_quit();
	}

	return TRUE;
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
	_hgspy_quit_cb(spy->window, NULL, spy);
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
_hgspy_gc_started_cb(HgMemoryVisualizer *visual,
		     gpointer            data)
{
	HgSpy *spy = data;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (data != NULL);

	spy->gc_status_msg_id = gtk_statusbar_push(GTK_STATUSBAR (spy->status),
						   spy->gc_status_ctxt_id,
						   _("Collecting the garbages..."));
}

static void
_hgspy_gc_finished_cb(HgMemoryVisualizer *visual,
		     gpointer            data)
{
	HgSpy *spy = data;

	g_return_if_fail (HG_IS_MEMORY_VISUALIZER (visual));
	g_return_if_fail (data != NULL);

	gtk_statusbar_remove(GTK_STATUSBAR (spy->status),
			     spy->gc_status_ctxt_id,
			     spy->gc_status_msg_id);
}

static void
_hgspy_entry_activate_cb(GtkWidget *widget,
			 gpointer   data)
{
	GtkEntry *entry;
	HgSpy *spy = data;
	const gchar *text;
	gchar *echoback;

	g_return_if_fail (GTK_IS_ENTRY (widget));
	g_return_if_fail (data != NULL);

	entry = GTK_ENTRY (widget);
	if (spy->statementedit_buffer) {
		g_warning("FIXME: something lost.");
	}
	text = gtk_entry_get_text(entry);
	if (text == NULL || text[0] == 0)
		return;
	if (spy->vm) {
		echoback = g_strdup_printf("%s%s\n",
					   gtk_label_get_text(GTK_LABEL (spy->prompt)),
					   text);
		_hgspy_insert_text_into_textview(spy, echoback, strlen(echoback));
		g_free(echoback);
		spy->statementedit_buffer = g_strdup(text);
		gtk_widget_set_sensitive(spy->entry, FALSE);
	}
	gtk_entry_set_text(entry, "");
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
		p = g_strdup_printf("%" G_GSIZE_FORMAT, total);
		gtk_entry_set_text(GTK_ENTRY (spy->total_vm_size), p);
		g_free(p);
		used = hg_memory_visualizer_get_used_size(visual, name);
		p = g_strdup_printf("%" G_GSIZE_FORMAT, used);
		gtk_entry_set_text(GTK_ENTRY (spy->used_vm_size), p);
		g_free(p);
		p = g_strdup_printf("%" G_GSIZE_FORMAT, total - used);
		gtk_entry_set_text(GTK_ENTRY (spy->free_vm_size), p);
		g_free(p);
	} else {
		gtk_entry_set_text(GTK_ENTRY (spy->total_vm_size), "");
		gtk_entry_set_text(GTK_ENTRY (spy->used_vm_size), "");
		gtk_entry_set_text(GTK_ENTRY (spy->free_vm_size), "");
	}
}

static void
_hgspy_insert_text_into_textview(HgSpy       *spy,
				 const gchar *text,
				 gsize        length)
{
	GtkTextBuffer *textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW (spy->textview));
	GtkTextIter iter;
	gdouble val;

	gtk_text_buffer_get_end_iter(textbuf, &iter);
	gtk_text_buffer_insert(textbuf, &iter, text, length);
	val = GTK_TEXT_VIEW (spy->textview)->vadjustment->upper;
	gtk_adjustment_set_value(GTK_TEXT_VIEW (spy->textview)->vadjustment,
				 val);
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
	GtkWidget *menubar, *vbox, *none, *table, *label, *hbox, *vbox2, *scrolled, *hbox2;
	GtkWidget *vpaned, *vbox3;
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
	gdk_threads_init();
	gtk_init(&argc, &argv);

	spy = g_new(HgSpy, 1);
	spy->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	spy->vm_thread = NULL;
	spy->vm = NULL;
	spy->file = NULL;
	spy->error = 0;
	spy->statementedit_buffer = NULL;
	spy->destroyed = FALSE;
	spy->gc_status_ctxt_id = 0;
	spy->gc_status_msg_id = 0;

	actions = gtk_action_group_new("MenuBar");
	spy->ui = gtk_ui_manager_new();
	vbox = gtk_vbox_new(FALSE, 0);
	vbox2 = gtk_vbox_new(FALSE, 0);
	vbox3 = gtk_vbox_new(FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	hbox2 = gtk_hbox_new(FALSE, 0);
	table = gtk_table_new(3, 2, FALSE);
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	vpaned = gtk_vpaned_new();
	spy->total_vm_size = gtk_entry_new();
	spy->used_vm_size = gtk_entry_new();
	spy->free_vm_size = gtk_entry_new();
	spy->visualizer = ((GtkWidget * (*) (void))__hg_spy_helper_get_widget) ();
	spy->textview = gtk_text_view_new();
	spy->entry = gtk_entry_new();
	spy->prompt = gtk_label_new("PS>");
	spy->status = gtk_statusbar_new();

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
	gtk_window_set_default_size(GTK_WINDOW (spy->window), 400, 300);
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
	GTK_WIDGET_UNSET_FLAGS (spy->textview, GTK_CAN_FOCUS);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_text_view_set_editable(GTK_TEXT_VIEW (spy->textview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (spy->textview), GTK_WRAP_WORD_CHAR);
	spy->gc_status_ctxt_id = gtk_statusbar_get_context_id(GTK_STATUSBAR (spy->status),
							      "Status for GC");

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

	gtk_container_add(GTK_CONTAINER (scrolled), spy->textview);
	gtk_box_pack_start(GTK_BOX (vbox2), scrolled, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (hbox2), spy->prompt, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (hbox2), spy->entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (hbox), table, FALSE, FALSE, 0);
	gtk_paned_pack1(GTK_PANED (vpaned), vbox, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED (vpaned), hbox, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX (vbox3), vpaned, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX (vbox3), spy->status, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER (spy->window), vbox3);

	/* setup signals */
	g_signal_connect(spy->visualizer, "pool-updated",
			 G_CALLBACK (_hgspy_pool_updated_cb), spy);
	g_signal_connect(spy->visualizer, "draw-updated",
			 G_CALLBACK (_hgspy_draw_updated_cb), spy);
	g_signal_connect(spy->visualizer, "gc-started",
			 G_CALLBACK (_hgspy_gc_started_cb), spy);
	g_signal_connect(spy->visualizer, "gc-finished",
			 G_CALLBACK (_hgspy_gc_finished_cb), spy);
	g_signal_connect(spy->window, "delete-event",
			 G_CALLBACK (_hgspy_quit_cb), spy);
	g_signal_connect(spy->entry, "activate",
			 G_CALLBACK (_hgspy_entry_activate_cb), spy);

	gtk_widget_show_all(spy->window);
	gtk_widget_grab_focus(spy->entry);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	if (spy->vm) {
		spy->destroyed = TRUE;
		while (spy->destroyed)
			sleep(1);
	}

	/* finalize */
	g_module_close(module);

	return 0;
}
