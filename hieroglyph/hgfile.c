/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.c
 * Copyright (C) 2005-2011 Akira TAGOH
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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "hglineedit.h"
#include "hgmem.h"
#include "hgfile.h"

#include "hgfile.proto.h"

typedef struct _hg_file_io_buffered_data_t {
	hg_file_io_data_t  data;
	hg_string_t       *in;
	hg_string_t       *out;
} hg_file_io_buffered_data_t;


static hg_file_vtable_t __hg_file_io_stdin_vtable = {
	.open      = _hg_file_io_real_stdin_open,
	.close     = _hg_file_io_real_no_close,
	.read      = _hg_file_io_real_file_read,
	.write     = _hg_file_io_real_file_write,
	.flush     = _hg_file_io_real_file_flush,
	.seek      = _hg_file_io_real_file_seek,
	.is_eof    = _hg_file_io_real_file_is_eof,
	.clear_eof = _hg_file_io_real_file_clear_eof,
};
static hg_file_vtable_t __hg_file_io_stdout_vtable = {
	.open      = _hg_file_io_real_stdout_open,
	.close     = _hg_file_io_real_no_close,
	.read      = _hg_file_io_real_file_read,
	.write     = _hg_file_io_real_file_write,
	.flush     = _hg_file_io_real_file_flush,
	.seek      = _hg_file_io_real_file_seek,
	.is_eof    = _hg_file_io_real_file_is_eof,
	.clear_eof = _hg_file_io_real_file_clear_eof,
};
static hg_file_vtable_t __hg_file_io_stderr_vtable = {
	.open      = _hg_file_io_real_stderr_open,
	.close     = _hg_file_io_real_no_close,
	.read      = _hg_file_io_real_file_read,
	.write     = _hg_file_io_real_file_write,
	.flush     = _hg_file_io_real_file_flush,
	.seek      = _hg_file_io_real_file_seek,
	.is_eof    = _hg_file_io_real_file_is_eof,
	.clear_eof = _hg_file_io_real_file_clear_eof,
};
static hg_file_vtable_t __hg_file_io_file_vtable = {
	.open      = _hg_file_io_real_file_open,
	.close     = _hg_file_io_real_file_close,
	.read      = _hg_file_io_real_file_read,
	.write     = _hg_file_io_real_file_write,
	.flush     = _hg_file_io_real_file_flush,
	.seek      = _hg_file_io_real_file_seek,
	.is_eof    = _hg_file_io_real_file_is_eof,
	.clear_eof = _hg_file_io_real_file_clear_eof,
};
static hg_file_vtable_t __hg_file_io_buffered_vtable = {
	.open      = _hg_file_io_real_buffered_open,
	.close     = _hg_file_io_real_buffered_close,
	.read      = _hg_file_io_real_buffered_read,
	.write     = _hg_file_io_real_buffered_write,
	.flush     = _hg_file_io_real_buffered_flush,
	.seek      = _hg_file_io_real_buffered_seek,
	.is_eof    = _hg_file_io_real_buffered_is_eof,
	.clear_eof = _hg_file_io_real_buffered_clear_eof,
};
static hg_file_vtable_t __hg_file_io_lineedit_vtable = {
	.open      = _hg_file_io_real_lineedit_open,
	.close     = _hg_file_io_real_buffered_close,
	.read      = _hg_file_io_real_buffered_read,
	.write     = _hg_file_io_real_buffered_write,
	.flush     = _hg_file_io_real_buffered_flush,
	.seek      = _hg_file_io_real_buffered_seek,
	.is_eof    = _hg_file_io_real_buffered_is_eof,
	.clear_eof = _hg_file_io_real_buffered_clear_eof,
};
HG_THREAD_VAR hg_int_t hg_fileerrno = 0;

HG_DEFINE_VTABLE (file);

/*< private >*/
static hg_usize_t
_hg_object_file_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_file_t));
}

static hg_uint_t
_hg_object_file_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE;
}

static hg_bool_t
_hg_object_file_initialize(hg_object_t *object,
			   va_list      args)
{
	hg_file_t *file = (hg_file_t *)object;
	const hg_char_t *name;
	hg_file_mode_t mode;
	hg_file_vtable_t *vtable;
	hg_pointer_t user_data;

	name = va_arg(args, const hg_char_t *);
	mode = va_arg(args, hg_file_mode_t);
	vtable = va_arg(args, hg_file_vtable_t *);
	user_data = va_arg(args, hg_pointer_t);

	if (name == NULL || name[0] == 0) {
		hg_debug(HG_MSGCAT_FILE, "No filename to open");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_undefinedfilename);
		return FALSE;
	}
	if (vtable == NULL || vtable->open == NULL) {
		hg_debug(HG_MSGCAT_FILE, "No virtual functions");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		return FALSE;
	}

	/* initialize the members first to avoid
	 * segfault on GC
	 */
	memset(HG_STRUCT_MEMBER_P (file, sizeof (hg_object_t)), 0, sizeof (hg_file_t) - sizeof (hg_object_t));
	file->qfilename = HG_QSTRING (file->o.mem, name);
	file->io_type   = hg_file_get_io_type(name);
	file->mode      = mode;
	file->vtable    = vtable;
	file->size      = 0;
	file->position  = 0;
	file->lineno    = 1;
	file->is_closed = TRUE;
	file->user_data = user_data;

	hg_mem_reserved_spool_remove(file->o.mem, file->qfilename);

	return vtable->open(file, file->user_data);
}

static void
_hg_object_file_free(hg_object_t *object)
{
	hg_file_t *file = (hg_file_t *)object;

	hg_return_if_fail (object->type == HG_TYPE_FILE, HG_e_typecheck);

	if (file->vtable && file->vtable->close) {
		if (!file->is_closed)
			file->vtable->close(file, file->user_data);
	}
	if (file->yybuffer && file->yyfree)
		file->yyfree(file->yybuffer, file->yydata);
	hg_mem_free(file->o.mem, file->qfilename);
	if (file->user_data) {
		if (file->user_data->destroy_func)
			file->user_data->destroy_func(file->o.mem, file->user_data);
		else
			hg_mem_free(file->o.mem, file->user_data->self);
		file->user_data = NULL;
	}
}

static hg_quark_t
_hg_object_file_copy(hg_object_t             *object,
		     hg_quark_iterate_func_t  func,
		     hg_pointer_t             user_data,
		     hg_pointer_t            *ret)
{
	hg_return_val_if_fail (object->type == HG_TYPE_FILE, Qnil, HG_e_typecheck);

	return object->self;
}

static hg_char_t *
_hg_object_file_to_cstr(hg_object_t             *object,
			hg_quark_iterate_func_t  func,
			hg_pointer_t             user_data)
{
	hg_return_val_if_fail (object->type == HG_TYPE_FILE, NULL, HG_e_typecheck);

	return g_strdup("-file-");
}

static hg_bool_t
_hg_object_file_gc_mark(hg_object_t          *object,
			hg_gc_iterate_func_t  func,
			hg_pointer_t          user_data)
{
	hg_file_t *file = (hg_file_t *)object;
	hg_file_gc_t g;

	hg_return_val_if_fail (object->type == HG_TYPE_FILE, FALSE, HG_e_typecheck);

	hg_debug(HG_MSGCAT_GC, "file: marking filename");
	if (!func(file->qfilename, user_data))
		return FALSE;
	if (file->user_data) {
		if (!file->user_data->gc_func) {
			hg_debug(HG_MSGCAT_GC, "file: marking user data");
			return hg_mem_gc_mark(file->o.mem, file->user_data->self);
		} else {
			g.func = func;
			g.user_data = user_data;
			g.mem = file->o.mem;
			g.data = file->user_data;

			hg_debug(HG_MSGCAT_GC, "file: marking user data with own gc_func");

			return file->user_data->gc_func(file->user_data->self, &g);
		}
	}

	return TRUE;
}

static hg_bool_t
_hg_object_file_compare(hg_object_t             *o1,
			hg_object_t             *o2,
			hg_quark_compare_func_t  func,
			hg_pointer_t             user_data)
{
	return o1->self == o2->self;
}

static hg_bool_t
_hg_file_io_data_gc_mark(hg_quark_t   qdata,
			 hg_pointer_t user_data)
{
	hg_file_gc_t *g = user_data;

	return hg_mem_gc_mark(g->mem, qdata);
}

static void
_hg_file_io_data_free(hg_mem_t     *mem,
		      hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = (hg_file_io_data_t *)user_data;

	hg_mem_free(mem, data->self);
}

static hg_bool_t
_hg_file_io_buffered_data_gc_mark(hg_quark_t   qdata,
				  hg_pointer_t user_data)
{
	hg_file_gc_t *g = user_data;
	hg_file_io_buffered_data_t *bd = (hg_file_io_buffered_data_t *)g->data;

	if (bd->in) {
		if (!g->func(bd->in->o.self, g->user_data))
			return FALSE;
	}
	if (bd->out) {
		if (!g->func(bd->out->o.self, g->user_data))
			return FALSE;
	}

	return hg_mem_gc_mark(g->mem, qdata);
}

static void
_hg_file_io_buffered_data_free(hg_mem_t     *mem,
			       hg_pointer_t  user_data)
{
	hg_file_io_buffered_data_t *bd = (hg_file_io_buffered_data_t *)user_data;

	if (bd->in)
		hg_object_free(bd->in->o.mem, bd->in->o.self);
	if (bd->out)
		hg_object_free(bd->out->o.mem, bd->out->o.self);

	hg_mem_free(mem, bd->data.self);
}

/** file IO callbacks **/
static hg_bool_t
_hg_file_io_real_stdin_open(hg_file_t    *file,
			    hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;
	hg_quark_t qdata;

	hg_return_val_if_fail (file->mode < HG_FILE_IO_MODE_END, FALSE, HG_e_VMerror);

	if (file->io_type != HG_FILE_IO_STDIN) {
		hg_debug(HG_MSGCAT_FILE, "Invalid I/O request.");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		return FALSE;
	}
	if ((file->mode & HG_FILE_IO_MODE_WRITE) != 0) {
		errno = EBADF;
		goto exception;
	}
	if (data == NULL) {
		qdata = hg_mem_alloc(file->o.mem,
				     sizeof (hg_file_io_data_t),
				     (hg_pointer_t *)&data);
		if (qdata == Qnil) {
			errno = ENOMEM;
			goto exception;
		}
		memset(data, 0, sizeof (hg_file_io_data_t));
		data->self = qdata;
		data->gc_func = _hg_file_io_data_gc_mark;
		data->destroy_func = _hg_file_io_data_free;
	}
	data->fd = 0;
	data->mmapped_buffer = NULL;
	file->user_data = data;

	file->size = 0;
	file->is_closed = FALSE;

	return TRUE;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return FALSE;
}

static hg_bool_t
_hg_file_io_real_stdout_open(hg_file_t    *file,
			     hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;
	hg_quark_t qdata;

	hg_return_val_if_fail (file->mode < HG_FILE_IO_MODE_END, FALSE, HG_e_VMerror);

	if (file->io_type != HG_FILE_IO_STDOUT) {
		hg_debug(HG_MSGCAT_FILE, "Invalid I/O request.");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		return FALSE;
	}
	if ((file->mode & HG_FILE_IO_MODE_READ) != 0) {
		errno = EBADF;
		goto exception;
	}
	if (data == NULL) {
		qdata = hg_mem_alloc(file->o.mem,
				     sizeof (hg_file_io_data_t),
				     (hg_pointer_t *)&data);
		if (qdata == Qnil) {
			errno = ENOMEM;
			goto exception;
		}
		memset(data, 0, sizeof (hg_file_io_data_t));
		data->self = qdata;
		data->gc_func = _hg_file_io_data_gc_mark;
		data->destroy_func = _hg_file_io_data_free;
	}
	data->fd = 1;
	data->mmapped_buffer = NULL;
	file->user_data = data;

	file->size = 0;
	file->is_closed = FALSE;

	return TRUE;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return FALSE;
}

static hg_bool_t
_hg_file_io_real_stderr_open(hg_file_t    *file,
			     hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;
	hg_quark_t qdata;

	hg_return_val_if_fail (file->mode < HG_FILE_IO_MODE_END, FALSE, HG_e_VMerror);

	if (file->io_type != HG_FILE_IO_STDERR) {
		hg_debug(HG_MSGCAT_FILE, "Invalid I/O request.");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		return FALSE;
	}
	if ((file->mode & HG_FILE_IO_MODE_READ) != 0) {
		errno = EBADF;
		goto exception;
	}
	if (data == NULL) {
		qdata = hg_mem_alloc(file->o.mem,
				     sizeof (hg_file_io_data_t),
				     (hg_pointer_t *)&data);
		if (qdata == Qnil) {
			errno = ENOMEM;
			goto exception;
		}
		memset(data, 0, sizeof (hg_file_io_data_t));
		data->self = qdata;
		data->gc_func = _hg_file_io_data_gc_mark;
		data->destroy_func = _hg_file_io_data_free;
	}
	data->fd = 2;
	data->mmapped_buffer = NULL;
	file->user_data = data;

	file->size = 0;
	file->is_closed = FALSE;

	return TRUE;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return FALSE;
}

static void
_hg_file_io_real_no_close(hg_file_t    *file,
			  hg_pointer_t  user_data)
{
	file->is_closed = TRUE;
}

static hg_bool_t
_hg_file_io_real_file_open(hg_file_t    *file,
			   hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;
	hg_quark_t qdata;
	hg_string_t *sfilename;
	hg_char_t *filename;
	struct stat st;
	int fd;
	hg_pointer_t buffer = NULL;
	static hg_int_t flags[] = {
		0,				/* 0 */
		O_RDONLY,			/* HG_FILE_IO_MODE_READ */
		O_WRONLY | O_CREAT | O_TRUNC,	/* HG_FILE_IO_MODE_WRITE */
		O_RDWR,				/* HG_FILE_IO_MODE_READWRITE */
		O_WRONLY | O_CREAT | O_APPEND,	/* HG_FILE_IO_MODE_APPEND */
		O_RDWR,				/* HG_FILE_IO_MODE_READ | HG_FILE_IO_MODE_APPEND */
		O_RDWR | O_CREAT | O_TRUNC,	/* HG_FILE_IO_MODE_WRITE | HG_FILE_IO_MODE_APPEND */
		O_RDWR | O_CREAT | O_APPEND,	/* HG_FILE_IO_MODE_READWRITE | HG_FILE_IO_MODE_APPEND */
	};

	hg_return_val_if_fail (file->mode < HG_FILE_IO_MODE_END, FALSE, HG_e_VMerror);

	if (file->io_type != HG_FILE_IO_FILE) {
		hg_debug(HG_MSGCAT_FILE, "Invalid I/O request.");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		return FALSE;
	}
	if (data == NULL) {
		qdata = hg_mem_alloc(file->o.mem,
				     sizeof (hg_file_io_data_t),
				     (hg_pointer_t *)&data);
		if (qdata == Qnil) {
			errno = ENOMEM;
			goto exception;
		}
		memset(data, 0, sizeof (hg_file_io_data_t));
		data->self = qdata;
		data->gc_func = _hg_file_io_data_gc_mark;
		data->destroy_func = _hg_file_io_data_free;
	}
	data->fd = -1;
	data->mmapped_buffer = NULL;
	file->user_data = data;

	hg_return_val_if_lock_fail (sfilename,
				    file->o.mem,
				    file->qfilename,
				    FALSE);
	filename = hg_string_get_cstr(sfilename);
	hg_mem_unlock_object(file->o.mem, file->qfilename);
	errno = 0;
	if (stat(filename, &st) == -1 && (file->mode & HG_FILE_IO_MODE_READ) != 0) {
		goto exception;
	}
	if ((fd = open(filename, flags[file->mode], S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1) {
		goto exception;
	}
	g_free(filename);
	data->fd = fd;
	if ((buffer = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) != MAP_FAILED) {
		data->mmapped_buffer = buffer;
	}
	file->size = st.st_size;
	file->is_closed = FALSE;

	return TRUE;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return FALSE;
}

static void
_hg_file_io_real_file_close(hg_file_t    *file,
			    hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;

	if (data->mmapped_buffer) {
		munmap(data->mmapped_buffer, file->size);
	}
	if (data->fd != -1) {
		close(data->fd);
	}
	file->is_closed = TRUE;
}

static hg_size_t
_hg_file_io_real_file_read(hg_file_t    *file,
			   hg_pointer_t  user_data,
			   hg_pointer_t  buffer,
			   hg_usize_t    size,
			   hg_usize_t    n)
{
	hg_usize_t retval;
	hg_file_io_data_t *data = user_data;

	if ((file->mode & HG_FILE_IO_MODE_READ) == 0) {
		errno = EBADF;
		goto exception;
	}
	if (data->mmapped_buffer) {
		if ((file->size - file->position) < (size * n)) {
			retval = file->size - file->position;
		} else {
			retval = size * n;
		}
		memcpy(buffer, (hg_char_t *)data->mmapped_buffer + file->position, retval);
//		((hg_char_t *)buffer)[retval] = 0;
		if (retval == 0 &&
		    file->position == file->size)
			data->is_eof = TRUE;
	} else {
		errno = 0;
		retval = read(data->fd, buffer, size * n);
		if (errno)
			goto exception;
		if (size * n != 0 && retval == 0)
			data->is_eof = TRUE;
	}
	file->position += retval;

	return retval;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return -1;
}

static hg_size_t
_hg_file_io_real_file_write(hg_file_t          *file,
			    hg_pointer_t        user_data,
			    const hg_pointer_t  buffer,
			    hg_usize_t          size,
			    hg_usize_t          n)
{
	hg_usize_t retval;
	hg_file_io_data_t *data = user_data;

	if ((file->mode & HG_FILE_IO_MODE_WRITE) == 0) {
		errno = EBADF;
		goto exception;
	}
	errno = 0;
	retval = write(data->fd, buffer, size * n);
	if (errno)
		goto exception;

	return retval;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return -1;
}

static hg_bool_t
_hg_file_io_real_file_flush(hg_file_t    *file,
			    hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;

	if ((file->mode & HG_FILE_IO_MODE_READ)) {
		if (hg_file_seek(file, 0, HG_FILE_POS_END) == -1)
			return FALSE;
	}
	if ((file->mode & HG_FILE_IO_MODE_WRITE)) {
		errno = 0;
		fsync(data->fd);
		if (errno)
			goto exception;
	}

	return TRUE;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return FALSE;
}

static hg_size_t
_hg_file_io_real_file_seek(hg_file_t     *file,
			   hg_pointer_t   user_data,
			   hg_size_t      offset,
			   hg_file_pos_t  whence)
{
	hg_file_io_data_t *data = user_data;
	hg_size_t retval;

	if (data->mmapped_buffer) {
		switch (whence) {
		    case HG_FILE_POS_BEGIN:
			    if (offset < 0)
				    file->position = 0;
			    else if (offset > file->size)
				    file->position = file->size;
			    else
				    file->position = offset;
			    break;
		    case HG_FILE_POS_CURRENT:
			    file->position += offset;
			    if (file->position < 0)
				    file->position = 0;
			    else if (file->position > file->size)
				    file->position = file->size;
			    break;
		    case HG_FILE_POS_END:
			    file->position = file->size + offset;
			    if (file->position < 0)
				    file->position = 0;
			    else if (file->position > file->size)
				    file->position = file->size;
			    break;
		    default:
			    errno = EINVAL;
			    goto exception;
		}
		retval = file->position;
	} else {
		errno = 0;
		retval = lseek(data->fd, offset, whence);
		if (errno)
			goto exception;
	}

	return retval;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return -1;
}

static hg_bool_t
_hg_file_io_real_file_is_eof(hg_file_t    *file,
			     hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;

	return data->is_eof;
}

static void
_hg_file_io_real_file_clear_eof(hg_file_t    *file,
				hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;

	data->is_eof = FALSE;
}

/** buffered IO callbacks **/
static hg_bool_t
_hg_file_io_real_buffered_open(hg_file_t    *file,
			       hg_pointer_t  user_data)
{
	hg_file_io_buffered_data_t *bd = user_data;

	hg_return_val_if_fail (file->mode < HG_FILE_IO_MODE_END, FALSE, HG_e_VMerror);

	switch (file->io_type) {
	    case HG_FILE_IO_FILE:
		    break;
	    case HG_FILE_IO_STDIN:
	    case HG_FILE_IO_LINEEDIT:
	    case HG_FILE_IO_STATEMENTEDIT:
		    if ((file->mode & HG_FILE_IO_MODE_WRITE) != 0) {
			    errno = EBADF;
			    goto exception;
		    }
		    break;
	    case HG_FILE_IO_STDOUT:
	    case HG_FILE_IO_STDERR:
		    if ((file->mode & HG_FILE_IO_MODE_READ) != 0) {
			    errno = EBADF;
			    goto exception;
		    }
		    break;
	    default:
		    hg_debug(HG_MSGCAT_FILE, "Invalid I/O request.");
		    hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		    return FALSE;
	}
	bd->data.fd = -1;
	bd->data.mmapped_buffer = NULL;
	file->size = hg_string_length(bd->in);
	file->is_closed = FALSE;

	return TRUE;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return FALSE;
}

static void
_hg_file_io_real_buffered_close(hg_file_t    *file,
				hg_pointer_t  user_data)
{
	hg_file_io_buffered_data_t *bd = user_data;

	if (bd->in)
		hg_mem_unlock_object(bd->in->o.mem, bd->in->o.self);
	if (bd->out)
		hg_mem_unlock_object(bd->out->o.mem, bd->out->o.self);
	hg_mem_free(file->o.mem, bd->data.self);
	file->user_data = NULL;
	file->is_closed = TRUE;
}

static hg_size_t
_hg_file_io_real_buffered_read(hg_file_t    *file,
			       hg_pointer_t  user_data,
			       hg_pointer_t  buffer,
			       hg_usize_t    size,
			       hg_usize_t    n)
{
	hg_usize_t retval;
	hg_file_io_buffered_data_t *bd = user_data;
	hg_char_t *cstr;

	if ((file->mode & HG_FILE_IO_MODE_READ) == 0) {
		errno = EBADF;
		goto exception;
	}
	if ((file->size - file->position) < (size * n)) {
		retval = file->size - file->position;
	} else {
		retval = size * n;
	}
	cstr = hg_string_get_cstr(bd->in);
	memcpy(buffer, cstr + file->position, retval);
//	((hg_char_t *)buffer)[retval] = 0;
	if (retval == 0 &&
	    file->position == file->size)
		bd->data.is_eof = TRUE;
	file->position += retval;
	g_free(cstr);

	return retval;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return -1;
}

static hg_size_t
_hg_file_io_real_buffered_write(hg_file_t          *file,
				hg_pointer_t        user_data,
				const hg_pointer_t  buffer,
				hg_usize_t          size,
				hg_usize_t          n)
{
	hg_file_io_buffered_data_t *bd = user_data;

	if ((file->mode & HG_FILE_IO_MODE_WRITE) == 0) {
		errno = EBADF;
		goto exception;
	}
	if (!hg_string_append(bd->out, buffer, size * n)) {
		return -1;
	}

	return size * n;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return -1;
}

static hg_bool_t
_hg_file_io_real_buffered_flush(hg_file_t    *file,
				hg_pointer_t  user_data)
{
	if ((file->mode & HG_FILE_IO_MODE_READ)) {
		if (hg_file_seek(file, 0, HG_FILE_POS_END) == -1)
			return FALSE;
	}

	return TRUE;
}

static hg_size_t
_hg_file_io_real_buffered_seek(hg_file_t     *file,
			       hg_pointer_t   user_data,
			       hg_size_t      offset,
			       hg_file_pos_t  whence)
{
	hg_size_t retval;

	switch (whence) {
	    case HG_FILE_POS_BEGIN:
		    if (offset < 0)
			    file->position = 0;
		    else if (offset > file->size)
			    file->position = file->size;
		    else
			    file->position = offset;
		    break;
	    case HG_FILE_POS_CURRENT:
		    file->position += offset;
		    if (file->position < 0)
			    file->position = 0;
		    else if (file->position > file->size)
			    file->position = file->size;
		    break;
	    case HG_FILE_POS_END:
		    file->position = file->size + offset;
		    if (file->position < 0)
			    file->position = 0;
		    else if (file->position > file->size)
			    file->position = file->size;
		    break;
	    default:
		    errno = EINVAL;
		    goto exception;
	}
	retval = file->position;

	return retval;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return -1;
}

static hg_bool_t
_hg_file_io_real_buffered_is_eof(hg_file_t    *file,
				 hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;

	return data->is_eof;
}

static void
_hg_file_io_real_buffered_clear_eof(hg_file_t    *file,
				    hg_pointer_t  user_data)
{
	hg_file_io_data_t *data = user_data;

	data->is_eof = FALSE;
}

/** lineedit I/O callbacks **/
static hg_bool_t
_hg_file_io_real_lineedit_open(hg_file_t    *file,
			       hg_pointer_t  user_data)
{
	hg_quark_t q, qs;
	hg_file_io_buffered_data_t *bd;
	hg_char_t *buf;
	hg_lineedit_t *lineedit = (hg_lineedit_t *)user_data;
	hg_string_t *s;

	hg_return_val_if_fail (file->mode < HG_FILE_IO_MODE_END, FALSE, HG_e_VMerror);

	switch (file->io_type) {
	    case HG_FILE_IO_LINEEDIT:
		    if ((file->mode & HG_FILE_IO_MODE_WRITE) != 0) {
			    errno = EBADF;
			    goto exception;
		    }
		    buf = hg_lineedit_get_line(lineedit, NULL, TRUE);
		    if (buf == NULL) {
			    errno = ENOENT;
			    goto exception;
		    }
		    break;
	    case HG_FILE_IO_STATEMENTEDIT:
		    if ((file->mode & HG_FILE_IO_MODE_WRITE) != 0) {
			    errno = EBADF;
			    goto exception;
		    }
		    buf = hg_lineedit_get_statement(lineedit, NULL, TRUE);
		    if (buf == NULL) {
			    errno = ENOENT;
			    goto exception;
		    }
		    break;
	    default:
		    hg_debug(HG_MSGCAT_FILE, "Invalid I/O request.");
		    hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
		    return FALSE;
	}
	qs = hg_string_new_with_value(file->o.mem,
				      buf, -1,
				      (hg_pointer_t *)&s);
	if (qs == Qnil) {
		errno = ENOENT;
		goto exception;
	}
	q = hg_mem_alloc(file->o.mem,
			  sizeof (hg_file_io_buffered_data_t),
			  (hg_pointer_t *)&bd);
	if (q == Qnil) {
		hg_string_free(s, TRUE);
		errno = ENOMEM;
		goto exception;
	}
	memset(bd, 0, sizeof (hg_file_io_buffered_data_t));
	bd->data.self = q;
	bd->data.gc_func = _hg_file_io_buffered_data_gc_mark;
	bd->data.destroy_func = _hg_file_io_buffered_data_free;
	bd->in = s;
	bd->out = NULL;
	bd->data.fd = -1;
	bd->data.mmapped_buffer = NULL;
	file->user_data = (hg_file_io_data_t *)bd;
	file->size = hg_string_length(bd->in);
	file->is_closed = FALSE;

	hg_mem_reserved_spool_remove(s->o.mem, qs);

	return TRUE;
  exception:
	hg_file_set_error(__PRETTY_FUNCTION__);

	return FALSE;
}

/*< public >*/
/**
 * hg_file_get_lineedit_vtable:
 *
 * FIXME
 *
 * Returns:
 */
const hg_file_vtable_t *
hg_file_get_lineedit_vtable(void)
{
	return &__hg_file_io_lineedit_vtable;
}

/**
 * hg_file_get_io_type:
 * @name:
 *
 * FIXME
 *
 * Returns:
 */
hg_file_io_t
hg_file_get_io_type(const hg_char_t *name)
{
	const hg_char_t *reserved_names[] = {
		"stdin",
		"stdout",
		"stderr",
		"lineedit",
		"statementedit",
		NULL
	};
	hg_int_t i;

	hg_return_val_if_fail (name != NULL, HG_FILE_IO_END, HG_e_VMerror);
	hg_return_val_if_fail (name[0] != 0, HG_FILE_IO_END, HG_e_VMerror);

	if (name[0] != '%')
		return HG_FILE_IO_FILE;

	for (i = 0; reserved_names[i] != NULL; i++) {
		if (strcmp(&name[1], reserved_names[i]) == 0)
			return i + 1;
	}

	return HG_FILE_IO_FILE;
}

/**
 * hg_file_new:
 * @mem:
 * @name:
 * @mode:
 * @error:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_file_new(hg_mem_t        *mem,
	    const hg_char_t *name,
	    hg_file_mode_t   mode,
	    hg_pointer_t    *ret)
{
	hg_file_io_t io;
	hg_file_vtable_t *vtable = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (name != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (name[0] != 0, Qnil, HG_e_VMerror);

	io = hg_file_get_io_type(name);
	switch (io) {
	    case HG_FILE_IO_FILE:
		    vtable = &__hg_file_io_file_vtable;
		    break;
	    case HG_FILE_IO_STDIN:
		    vtable = &__hg_file_io_stdin_vtable;
		    break;
	    case HG_FILE_IO_STDOUT:
		    vtable = &__hg_file_io_stdout_vtable;
		    break;
	    case HG_FILE_IO_STDERR:
		    vtable = &__hg_file_io_stderr_vtable;
		    break;
	    case HG_FILE_IO_LINEEDIT:
	    case HG_FILE_IO_STATEMENTEDIT:
	    default:
		    break;
	}

	return hg_file_new_with_vtable(mem, name, mode, vtable, NULL, ret);
}

/**
 * hg_file_new_with_vtable:
 * @mem:
 * @name:
 * @mode:
 * @vtable:
 * @user_data:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_file_new_with_vtable(hg_mem_t               *mem,
			const hg_char_t        *name,
			hg_file_mode_t          mode,
			const hg_file_vtable_t *vtable,
			hg_pointer_t            user_data,
			hg_pointer_t           *ret)
{
	hg_quark_t retval;
	hg_file_t *f = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (name != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (name[0] != 0, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (vtable != NULL, Qnil, HG_e_VMerror);

	retval = hg_object_new(mem, (hg_pointer_t *)&f, HG_TYPE_FILE, 0, name, mode, vtable, user_data);

	if (retval != Qnil) {
		if (ret)
			*ret = f;
		else
			hg_mem_unlock_object(mem, retval);
	}

	return retval;
}

/**
 * hg_file_new_with_string:
 * @mem:
 * @name:
 * @mode:
 * @buffer:
 * @error:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_file_new_with_string(hg_mem_t        *mem,
			const hg_char_t *name,
			hg_file_mode_t   mode,
			hg_string_t     *in,
			hg_string_t     *out,
			hg_pointer_t    *ret)
{
	hg_quark_t retval, qbdata;
	hg_file_t *f = NULL;
	hg_file_io_buffered_data_t *x;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (name != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (name[0] != 0, Qnil, HG_e_VMerror);

	qbdata = hg_mem_alloc(mem, sizeof (hg_file_io_buffered_data_t), (hg_pointer_t *)&x);
	if (qbdata == Qnil)
		return Qnil;

	memset(x, 0, sizeof (hg_file_io_buffered_data_t));
	x->data.self = qbdata;
	x->data.gc_func = _hg_file_io_buffered_data_gc_mark;
	x->data.destroy_func = _hg_file_io_buffered_data_free;
	x->in = in;
	x->out = out;
	retval = hg_object_new(mem, (hg_pointer_t *)&f, HG_TYPE_FILE, 0,
			       name, mode, &__hg_file_io_buffered_vtable,
			       x);
	if (retval != Qnil) {
		if (x->in) {
			hg_mem_lock_object(x->in->o.mem, x->in->o.self);
			hg_mem_reserved_spool_remove(x->in->o.mem, x->in->o.self);
		}
		if (x->out) {
			hg_mem_lock_object(x->out->o.mem, x->out->o.self);
			hg_mem_reserved_spool_remove(x->out->o.mem, x->out->o.self);
		}
	}

	if (ret)
		*ret = f;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_file_close:
 * @file:
 * @error:
 *
 * FIXME
 */
void
hg_file_close(hg_file_t *file)
{
	hg_return_if_fail (file != NULL, HG_e_typecheck);
	hg_return_if_fail (file->o.type == HG_TYPE_FILE, HG_e_typecheck);
	hg_return_if_fail (file->vtable != NULL, HG_e_VMerror);
	hg_return_if_fail (file->vtable->close != NULL, HG_e_VMerror);

	if (file->is_closed) {
		errno = EBADF;
		hg_file_set_error(__PRETTY_FUNCTION__);
	} else {
		file->vtable->close(file, file->user_data);
	}
}

/**
 * hg_file_read:
 * @file:
 * @buffer:
 * @size:
 * @n:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_file_read(hg_file_t    *file,
	     hg_pointer_t  buffer,
	     hg_usize_t    size,
	     hg_usize_t    n)
{
	hg_return_val_if_fail (file != NULL, -1, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, -1, HG_e_typecheck);
	hg_return_val_if_fail (buffer != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (file->vtable != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (file->vtable->read != NULL, -1, HG_e_VMerror);

	if (file->is_closed) {
		errno = EBADF;
		hg_file_set_error(__PRETTY_FUNCTION__);
		return -1;
	}

	return file->vtable->read(file, file->user_data, buffer, size, n);
}

/**
 * hg_file_write:
 * @file:
 * @buffer:
 * @size:
 * @n:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_file_write(hg_file_t    *file,
	      hg_pointer_t  buffer,
	      hg_usize_t    size,
	      hg_usize_t    n)
{
	hg_return_val_if_fail (file != NULL, -1, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, -1, HG_e_typecheck);
	hg_return_val_if_fail (buffer != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (file->vtable != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (file->vtable->write != NULL, -1, HG_e_VMerror);

	if (file->is_closed) {
		errno = EBADF;
		hg_file_set_error(__PRETTY_FUNCTION__);
		return -1;
	}

	return file->vtable->write(file, file->user_data, buffer, size, n);
}

/**
 * hg_file_flush:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_file_flush(hg_file_t *file)
{
	hg_return_val_if_fail (file != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (file->vtable != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (file->vtable->flush != NULL, FALSE, HG_e_VMerror);

	if (file->is_closed) {
		errno = EBADF;
		hg_file_set_error(__PRETTY_FUNCTION__);
		return FALSE;
	}

	return file->vtable->flush(file, file->user_data);
}

/**
 * hg_file_seek:
 * @file:
 * @offset:
 * @whence:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_file_seek(hg_file_t     *file,
	     hg_size_t      offset,
	     hg_file_pos_t  whence)
{
	hg_return_val_if_fail (file != NULL, -1, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, -1, HG_e_typecheck);
	hg_return_val_if_fail (file->vtable != NULL, -1, HG_e_VMerror);
	hg_return_val_if_fail (file->vtable->seek != NULL, -1, HG_e_VMerror);

	if (file->is_closed) {
		errno = EBADF;
		hg_file_set_error(__PRETTY_FUNCTION__);
		return -1;
	}

	return file->vtable->seek(file, file->user_data, offset, whence);
}

/**
 * hg_file_is_closed:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_file_is_closed(hg_file_t *file)
{
	hg_return_val_if_fail (file != NULL, TRUE, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, TRUE, HG_e_typecheck);

	return file->is_closed;
}

/**
 * hg_file_is_eof:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_file_is_eof(hg_file_t *file)
{
	hg_return_val_if_fail (file != NULL, TRUE, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, TRUE, HG_e_typecheck);
	hg_return_val_if_fail (file->vtable != NULL, TRUE, HG_e_VMerror);
	hg_return_val_if_fail (file->vtable->is_eof != NULL, TRUE, HG_e_VMerror);

	return file->is_closed || file->vtable->is_eof(file, file->user_data);
}

/**
 * hg_file_clear_eof:
 * @file:
 *
 * FIXME
 */
void
hg_file_clear_eof(hg_file_t *file)
{
	hg_return_if_fail (file != NULL, HG_e_typecheck);
	hg_return_if_fail (file->o.type == HG_TYPE_FILE, HG_e_typecheck);
	hg_return_if_fail (file->vtable != NULL, HG_e_VMerror);
	hg_return_if_fail (file->vtable->clear_eof != NULL, HG_e_VMerror);

	file->vtable->clear_eof(file, file->user_data);
}

/**
 * hg_file_append_printf:
 * @file:
 * @format:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_file_append_printf(hg_file_t       *file,
		      hg_char_t const *format,
		      ...)
{
	va_list ap;
	hg_size_t retval;

	va_start(ap, format);
	retval = hg_file_append_vprintf(file, format, ap);
	va_end(ap);

	return retval;
}

/**
 * hg_file_append_vprintf:
 * @file:
 * @format:
 * @args:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_file_append_vprintf(hg_file_t       *file,
		       hg_char_t const *format,
		       va_list          args)
{
	hg_char_t *buffer;
	hg_size_t retval;

	hg_return_val_if_fail (file != NULL, -1, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, -1, HG_e_typecheck);
	hg_return_val_if_fail (format != NULL, -1, HG_e_VMerror);

	buffer = g_strdup_vprintf(format, args);
	retval = hg_file_write(file, buffer, sizeof (hg_char_t), strlen(buffer));
	g_free(buffer);

	return retval;
}

/**
 * hg_file_set_lineno:
 * @file:
 * @n:
 *
 * FIXME
 */
void
hg_file_set_lineno(hg_file_t *file,
		   hg_size_t  n)
{
	hg_return_if_fail (file != NULL, HG_e_typecheck);
	hg_return_if_fail (file->o.type == HG_TYPE_FILE, HG_e_typecheck);
	hg_return_if_fail (n > 0, HG_e_rangecheck);

	file->lineno = n;
}

/**
 * hg_file_get_lineno:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_file_get_lineno(hg_file_t *file)
{
	hg_return_val_if_fail (file != NULL, -1, HG_e_typecheck);

	return file->lineno;
}

/**
 * hg_file_get_position:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_size_t
hg_file_get_position(hg_file_t *file)
{
	hg_return_val_if_fail (file != NULL, -1, HG_e_typecheck);
	hg_return_val_if_fail (file->o.type == HG_TYPE_FILE, -1, HG_e_typecheck);

	return file->position;
}

/**
 * hg_file_set_yybuffer:
 * @file:
 * @yybuffer:
 * @finalizer:
 *
 * FIXME
 */
void
hg_file_set_yybuffer(hg_file_t                        *file,
		     hg_pointer_t                      yybuffer,
		     hg_file_yybuffer_finalizer_func_t func,
		     hg_pointer_t                      user_data)
{
	hg_return_if_fail (file != NULL, HG_e_typecheck);

	if (file->yybuffer &&
	    file->yybuffer != yybuffer) {
		if (file->yyfree) {
			file->yyfree(file->yybuffer, file->yydata);
		}
	}
	file->yybuffer = yybuffer;
	file->yyfree = func;
	file->yydata = user_data;
}

/**
 * hg_file_get_yybuffer:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
hg_pointer_t
hg_file_get_yybuffer(hg_file_t *file)
{
	hg_return_val_if_fail (file != NULL, NULL, HG_e_typecheck);

	return file->yybuffer;
}

/**
 * hg_file_set_error:
 * @function:
 *
 * FIXME
 */
void
hg_file_set_error(const hg_char_t  *function)
{
	hg_char_t *msg;
	hg_error_t e = 0;

	hg_fileerrno = errno;
	msg = g_strdup_printf("%s: %s", function, g_strerror(hg_fileerrno));

	switch (hg_fileerrno) {
	    case 0:
		    /* ??? */
		    break;
	    case EACCES:
	    case EBADF:
	    case EEXIST:
	    case ENOTDIR:
	    case ENOTEMPTY:
	    case EPERM:
	    case EROFS:
		    e = HG_e_invalidaccess;
		    break;
	    case EAGAIN:
	    case EBUSY:
	    case EIO:
	    case ENOSPC:
	    case EINVAL:
		    e = HG_e_ioerror;
		    break;
	    case EMFILE:
		    e = HG_e_limitcheck;
		    break;
	    case ENAMETOOLONG:
	    case ENODEV:
	    case ENOENT:
		    e = HG_e_undefinedfilename;
		    break;
	    case ENOMEM:
		    e = HG_e_VMerror;
		    break;
	    default:
		    hg_warning("No matching error code for: %s", msg);
		    e = HG_e_VMerror;
		    break;
	}
	if (e > 0) {
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, e);
	}
	g_free(msg);
}
