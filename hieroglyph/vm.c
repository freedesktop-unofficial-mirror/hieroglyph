/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * vm.c
 * Copyright (C) 2005-2007 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
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

#include <glib/gmem.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgoperator.h>
#include <hieroglyph/hgstack.h>
#include "vm.h"


typedef struct hg_vm_private_s	hg_vm_private_t;

struct hg_vm_private_s {
	hg_vm_t             instance;
	hg_error_t          error;
	hg_emulationtype_t  current_level;
	hg_object_t        *io[HG_FILE_TYPE_END];
	hg_stack_t         *stack[HG_STACK_TYPE_END];
	hg_object_t        *dict;
};


static gboolean _hg_vm_lineedit_open          (gpointer       user_data,
					       error_t       *error);
static gsize    _hg_vm_lineedit_read          (gpointer       user_data,
					       gpointer       buffer,
					       gsize          size,
					       gsize          n,
					       error_t       *error);
static gsize    _hg_vm_lineedit_write         (gpointer       user_data,
					       gconstpointer  buffer,
					       gsize          size,
					       gsize          n,
					       error_t       *error);
static gboolean _hg_vm_lineedit_flush         (gpointer       user_data,
					       error_t       *error);
static gssize   _hg_vm_lineedit_seek          (gpointer       user_data,
					       gssize         offset,
					       hg_filepos_t   whence,
					       error_t       *error);
static void     _hg_vm_lineedit_close         (gpointer       user_data,
					       error_t       *error);
static gboolean _hg_vm_lineedit_is_closed     (gpointer       user_data);
static gboolean _hg_vm_lineedit_is_eof        (gpointer       user_data);
static void     _hg_vm_lineedit_clear_eof     (gpointer       user_data);
static gboolean _hg_vm_statementedit_open     (gpointer       user_data,
					       error_t       *error);
static gsize    _hg_vm_statementedit_read     (gpointer       user_data,
					       gpointer       buffer,
					       gsize          size,
					       gsize          n,
					       error_t       *error);
static gsize    _hg_vm_statementedit_write    (gpointer       user_data,
					       gconstpointer  buffer,
					       gsize          size,
					       gsize          n,
					       error_t       *error);
static gboolean _hg_vm_statementedit_flush    (gpointer       user_data,
					       error_t       *error);
static gssize   _hg_vm_statementedit_seek     (gpointer       user_data,
					       gssize         offset,
					       hg_filepos_t   whence,
					       error_t       *error);
static void     _hg_vm_statementedit_close    (gpointer       user_data,
					       error_t       *error);
static gboolean _hg_vm_statementedit_is_closed(gpointer       user_data);
static gboolean _hg_vm_statementedit_is_eof   (gpointer       user_data);
static void     _hg_vm_statementedit_clear_eof(gpointer       user_data);


static hg_filetable_t _hg_vm_lineedit_table = {
	.open      = _hg_vm_lineedit_open,
	.read      = _hg_vm_lineedit_read,
	.write     = _hg_vm_lineedit_write,
	.flush     = _hg_vm_lineedit_flush,
	.seek      = _hg_vm_lineedit_seek,
	.close     = _hg_vm_lineedit_close,
	.is_closed = _hg_vm_lineedit_is_closed,
	.is_eof    = _hg_vm_lineedit_is_eof,
	.clear_eof = _hg_vm_lineedit_clear_eof,
};
static hg_filetable_t _hg_vm_statementedit_table = {
	.open      = _hg_vm_statementedit_open,
	.read      = _hg_vm_statementedit_read,
	.write     = _hg_vm_statementedit_write,
	.flush     = _hg_vm_statementedit_flush,
	.seek      = _hg_vm_statementedit_seek,
	.close     = _hg_vm_statementedit_close,
	.is_closed = _hg_vm_statementedit_is_closed,
	.is_eof    = _hg_vm_statementedit_is_eof,
	.clear_eof = _hg_vm_statementedit_clear_eof,
};

/*
 * Private functions
 */
static gboolean
_hg_vm_lineedit_open(gpointer  user_data,
		     error_t  *error)
{
	/* XXX */
	return FALSE;
}

static gsize
_hg_vm_lineedit_read(gpointer  user_data,
		     gpointer  buffer,
		     gsize     size,
		     gsize     n,
		     error_t  *error)
{
	/* XXX */
	return 0;
}

static gsize
_hg_vm_lineedit_write(gpointer       user_data,
		      gconstpointer  buffer,
		      gsize          size,
		      gsize          n,
		      error_t       *error)
{
	/* XXX */
	return 0;
}

static gboolean
_hg_vm_lineedit_flush(gpointer  user_data,
		      error_t  *error)
{
	/* XXX */
	return FALSE;
}

static gssize
_hg_vm_lineedit_seek(gpointer      user_data,
		     gssize        offset,
		     hg_filepos_t  whence,
		     error_t      *error)
{
	/* XXX */
	return 0;
}

static void
_hg_vm_lineedit_close(gpointer  user_data,
		      error_t  *error)
{
	/* XXX */
}

static gboolean
_hg_vm_lineedit_is_closed(gpointer user_data)
{
	/* XXX */
	return FALSE;
}

static gboolean
_hg_vm_lineedit_is_eof(gpointer user_data)
{
	/* XXX */
	return FALSE;
}

static void
_hg_vm_lineedit_clear_eof(gpointer user_data)
{
	/* XXX */
}

static gboolean
_hg_vm_statementedit_open(gpointer  user_data,
			  error_t  *error)
{
	/* XXX */
	return FALSE;
}

static gsize
_hg_vm_statementedit_read(gpointer  user_data,
			  gpointer  buffer,
			  gsize     size,
			  gsize     n,
			  error_t  *error)
{
	/* XXX */
	return 0;
}

static gsize
_hg_vm_statementedit_write(gpointer       user_data,
			   gconstpointer  buffer,
			   gsize          size,
			   gsize          n,
			   error_t       *error)
{
	/* XXX */
	return 0;
}

static gboolean
_hg_vm_statementedit_flush(gpointer  user_data,
			   error_t  *error)
{
	/* XXX */
	return FALSE;
}

static gssize
_hg_vm_statementedit_seek(gpointer      user_data,
			  gssize        offset,
			  hg_filepos_t  whence,
			  error_t      *error)
{
	/* XXX */
	return FALSE;
}

static void
_hg_vm_statementedit_close(gpointer  user_data,
		      error_t  *error)
{
	/* XXX */
}

static gboolean
_hg_vm_statementedit_is_closed(gpointer user_data)
{
	/* XXX */
	return FALSE;
}

static gboolean
_hg_vm_statementedit_is_eof(gpointer user_data)
{
	/* XXX */
	return FALSE;
}

static void
_hg_vm_statementedit_clear_eof(gpointer user_data)
{
	/* XXX */
}

/*
 * Public functions
 */
hg_vm_t *
hg_vm_new(void)
{
	hg_vm_private_t *retval = g_new0(hg_vm_private_t, 1);
	hg_stacktype_t i;

	/* initialize VM */
	retval->current_level = HG_EMU_BEGIN;
	for (i = HG_STACK_TYPE_OSTACK; i < HG_STACK_TYPE_END; i++)
		retval->stack[i] = NULL;
	retval->dict = NULL;

	/* complete the initialization here */
	retval->io[HG_FILE_TYPE_STDIN] = hg_object_file_new((hg_vm_t *)retval,
							    "%stdin",
							    HG_FILE_MODE_READ);
	retval->io[HG_FILE_TYPE_STDOUT] = hg_object_file_new((hg_vm_t *)retval,
							     "%stdout",
							     HG_FILE_MODE_WRITE);
	retval->io[HG_FILE_TYPE_STDERR] = hg_object_file_new((hg_vm_t *)retval,
							     "%stderr",
							     HG_FILE_MODE_WRITE);
	retval->io[HG_FILE_TYPE_LINEEDIT] = hg_object_file_new_with_custom((hg_vm_t *)retval,
									   &_hg_vm_lineedit_table,
									   HG_FILE_MODE_READ);
	retval->io[HG_FILE_TYPE_STATEMENTEDIT] = hg_object_file_new_with_custom((hg_vm_t *)retval,
										&_hg_vm_statementedit_table,
										HG_FILE_MODE_READ);

	return (hg_vm_t *)retval;
}

void
hg_vm_destroy(hg_vm_t *vm)
{
	hg_vm_private_t *priv;
	hg_filetype_t i;
	hg_stacktype_t j;

	hg_return_if_fail (vm != NULL);

	priv = (hg_vm_private_t *)vm;

	for (i = 0; i < HG_FILE_TYPE_END; i++) {
		if (priv->io[i])
			hg_object_free(vm, priv->io[i]);
	}
	for (j = 0; j < HG_STACK_TYPE_END; j++) {
		if (priv->stack[j])
			hg_stack_free(vm, priv->stack[j]);
	}
	if (priv->dict)
		hg_object_free(vm, priv->dict);

	g_free(vm);
}

gpointer
hg_vm_malloc(hg_vm_t *vm,
	     gsize    size)
{
	gpointer retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	/* XXX */
	retval = g_malloc(size);

	return retval;
}

gpointer
hg_vm_realloc(hg_vm_t  *vm,
	      gpointer  object,
	      gsize     size)
{
	gpointer retval;

	hg_return_val_if_fail (vm != NULL, NULL);

	/* XXX */
	retval = g_realloc(object, size);

	return retval;
}

void
hg_vm_mfree(hg_vm_t  *vm,
	    gpointer  data)
{
	hg_return_if_fail (vm != NULL);

	/* XXX */
	g_free(data);
}

guchar
hg_vm_get_object_format(hg_vm_t *vm)
{
	hg_return_val_if_fail (vm != NULL, 0);

	/* XXX */
	return 0;
}

void
hg_vm_get_attributes(hg_vm_t        *vm,
		     hg_attribute_t *attr)
{
	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (attr != NULL);

	/* XXX */
	attr->bit.is_accessible = TRUE;
	attr->bit.is_readable = TRUE;
	attr->bit.is_executeonly = FALSE;
	attr->bit.is_locked = FALSE;
	attr->bit.reserved4 = FALSE;
	attr->bit.reserved5 = FALSE;
	attr->bit.is_global = FALSE;
	attr->bit.is_checked = FALSE;
}

void
hg_vm_set_error(hg_vm_t    *vm,
		hg_error_t  error)
{
	hg_vm_private_t *priv;

	hg_return_if_fail (vm != NULL);

	priv = (hg_vm_private_t *)vm;
	if (priv->error == 0) {
		priv->error = error;
	} else {
		/* XXX */
	}
}

hg_error_t
hg_vm_get_error(hg_vm_t *vm)
{
	hg_vm_private_t *priv;

	hg_return_val_if_fail (vm != NULL, HG_e_VMerror);

	priv = (hg_vm_private_t *)vm;

	return priv->error;
}

void
hg_vm_clear_error(hg_vm_t *vm)
{
	hg_vm_private_t *priv;

	hg_return_if_fail (vm != NULL);

	priv = (hg_vm_private_t *)vm;
	priv->error = 0;
}

hg_object_t *
hg_vm_get_io(hg_vm_t       *vm,
	     hg_filetype_t  iotype)
{
	hg_vm_private_t *priv;

	hg_return_val_if_fail (vm != NULL, NULL);

	priv = (hg_vm_private_t *)vm;

	return priv->io[iotype];
}

void
hg_vm_set_io(hg_vm_t     *vm,
	     hg_object_t *object)
{
	hg_vm_private_t *priv;

	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (object != NULL);
	hg_return_if_fail (HG_OBJECT_IS_FILE (object));

	priv = (hg_vm_private_t *)vm;
	priv->io[HG_OBJECT_FILE_DATA (object)->iotype] = object;
}

gboolean
hg_vm_initialize(hg_vm_t *vm)
{
	hg_vm_private_t *priv;
	gboolean retval;

	hg_return_val_if_fail (vm != NULL, FALSE);

	priv = (hg_vm_private_t *)vm;
	priv->stack[HG_STACK_TYPE_OSTACK] = hg_stack_new(vm, 65535);
	priv->stack[HG_STACK_TYPE_ESTACK] = hg_stack_new(vm, 65535);
	priv->stack[HG_STACK_TYPE_DSTACK] = hg_stack_new(vm, 65535);
	priv->dict = hg_object_dict_new(vm, 65535);

	/* push globaldict to dstack */
	hg_stack_push(priv->stack[HG_STACK_TYPE_DSTACK], priv->dict);

	/* initialize operators */
	retval = hg_object_operator_initialize(vm, HG_EMU_PS_LEVEL_3);

	return TRUE;
}

gboolean
hg_vm_finalize(hg_vm_t *vm)
{
	hg_vm_private_t *priv;

	hg_return_val_if_fail (vm != NULL, FALSE);

	priv = (hg_vm_private_t *)vm;
	/* XXX */

	return TRUE;
}

hg_emulationtype_t
hg_vm_get_emulation_level(hg_vm_t *vm)
{
	hg_vm_private_t *priv;

	hg_return_val_if_fail (vm != NULL, HG_EMU_BEGIN);

	priv = (hg_vm_private_t *)vm;

	return priv->current_level;
}

gboolean
hg_vm_set_emulation_level(hg_vm_t            *vm,
			  hg_emulationtype_t  level)
{
	hg_vm_private_t *priv;

	hg_return_val_if_fail (vm != NULL, FALSE);

	priv = (hg_vm_private_t *)vm;

	if (hg_object_operator_finalize(vm, level - 1) == FALSE)
		return FALSE;

	if (hg_object_operator_initialize(vm, level) == FALSE)
		return FALSE;

	priv->current_level = level;

	return TRUE;
}

gboolean
hg_vm_stepi(hg_vm_t  *vm,
	    gboolean *is_proceeded)
{
	hg_vm_private_t *priv;
	hg_stack_t *estack, *ostack;
	hg_object_t *object, *result;
	gboolean retval = TRUE;
	gsize index;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (is_proceeded != NULL, FALSE);

	priv = (hg_vm_private_t *)vm;
	estack = priv->stack[HG_STACK_TYPE_ESTACK];
	ostack = priv->stack[HG_STACK_TYPE_OSTACK];
	object = hg_stack_index(estack, 0);

	*is_proceeded = FALSE;
  evaluate:
	switch (HG_OBJECT_GET_TYPE (object)) {
	    case HG_OBJECT_TYPE_ARRAY:
	    case HG_OBJECT_TYPE_NAME:
		    if (HG_OBJECT_ATTR_IS_EXECUTABLE (object)) {
			    if (HG_OBJECT_GET_TYPE (object) == HG_OBJECT_TYPE_NAME) {
				    result = hg_vm_dict_lookup(vm, object);
				    if (result == NULL) {
					    hg_vm_set_error(vm, HG_e_undefined);
					    return FALSE;
				    }
			    } else {
				    result = object;
				    *is_proceeded = TRUE;
			    }
			    if (!hg_stack_push(estack, result)) {
				    hg_vm_set_error(vm, HG_e_execstackoverflow);
				    return FALSE;
			    }
			    hg_stack_roll(estack, 2, 1);
			    hg_stack_pop(estack);
			    break;
		    }
	    case HG_OBJECT_TYPE_NULL:
	    case HG_OBJECT_TYPE_INTEGER:
	    case HG_OBJECT_TYPE_REAL:
	    case HG_OBJECT_TYPE_BOOLEAN:
	    case HG_OBJECT_TYPE_STRING:
	    case HG_OBJECT_TYPE_DICT:
	    case HG_OBJECT_TYPE_MARK:
		    if (hg_stack_push(ostack, object) == FALSE) {
			    hg_vm_set_error(vm, HG_e_stackoverflow);
			    retval = FALSE;
		    }
		    hg_stack_pop(estack);
		    *is_proceeded = TRUE;
		    break;
	    case HG_OBJECT_TYPE_EVAL:
		    result = hg_vm_dict_lookup(vm, object);
		    if (result == NULL) {
			    hg_vm_set_error(vm, HG_e_undefined);
			    return FALSE;
		    }
		    object = result;
		    goto evaluate;
	    case HG_OBJECT_TYPE_FILE:
		    /* XXX: read a token and push to estack */
		    break;
	    case HG_OBJECT_TYPE_OPERATOR:
		    index = hg_stack_length(estack);
		    retval = hg_object_operator_invoke(vm, object);
		    hg_stack_roll(estack, hg_stack_length(estack) - index + 1, 1);
		    hg_stack_pop(estack);
		    *is_proceeded = TRUE;
		    break;
	    default:
		    g_warning("[BUG] Unknown object type  `%d'", HG_OBJECT_GET_TYPE (object));
		    break;
	}

	return retval;
}

gboolean
hg_vm_step(hg_vm_t *vm)
{
	gboolean retval, is_proceeded;

	while ((retval = hg_vm_stepi(vm, &is_proceeded))) {
		if (is_proceeded)
			break;
	}

	return retval;
}

hg_object_t *
hg_vm_get_dict(hg_vm_t *vm)
{
	hg_vm_private_t *priv;

	hg_return_val_if_fail (vm != NULL, NULL);

	priv = (hg_vm_private_t *)vm;

	return priv->dict;
}

hg_object_t *
hg_vm_dict_lookup(hg_vm_t     *vm,
		  hg_object_t *object)
{
	hg_vm_private_t *priv;
	hg_stack_t *dstack;
	hg_object_t *dict, *retval;
	gsize i, length;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (object != NULL, NULL);

	priv = (hg_vm_private_t *)vm;
	dstack = priv->stack[HG_STACK_TYPE_DSTACK];
	length = hg_stack_length(dstack);
	for (i = 0; i < length; i++) {
		dict = hg_stack_index(dstack, i);
		if ((retval = hg_object_dict_lookup(dict, object)) != NULL)
			return retval;
	}

	return NULL;
}

gboolean
hg_vm_dict_remove(hg_vm_t     *vm,
		  const gchar *name,
		  gboolean     remove_all)
{
	hg_vm_private_t *priv;
	hg_stack_t *dstack;
	hg_object_t *dict, *nobject;
	gsize i, length;
	gboolean retval = FALSE;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (name != NULL, FALSE);

	priv = (hg_vm_private_t *)vm;
	dstack = priv->stack[HG_STACK_TYPE_DSTACK];
	length = hg_stack_length(dstack);
	for (i = 0; i < length; i++) {
		dict = hg_stack_index(dstack, i);
		nobject = hg_vm_name_lookup(vm, name);
		retval |= hg_object_dict_remove(dict, nobject);
		if (retval && remove_all == FALSE)
			break;
	}

	return retval;
}

hg_object_t *
hg_vm_get_currentdict(hg_vm_t *vm)
{
	hg_vm_private_t *priv;
	hg_object_t *retval;
	hg_stack_t *dstack;

	hg_return_val_if_fail (vm != NULL, NULL);

	priv = (hg_vm_private_t *)vm;
	dstack = priv->stack[HG_STACK_TYPE_DSTACK];
	retval = hg_stack_index(dstack, 0);

	return retval;
}

hg_object_t *
hg_vm_name_lookup(hg_vm_t     *vm,
		  const gchar *name)
{
	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (name != NULL, NULL);

	/* XXX */

	return hg_object_name_new(vm, name, FALSE);
}
