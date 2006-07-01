/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.c
 * Copyright (C) 2005-2006 Akira TAGOH
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
#include <config.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "hgfile.h"
#include "hgallocator-bfit.h"
#include "hgmem.h"
#include "hgstring.h"
#include "hglineedit.h"


typedef struct _HieroGlyphFileBuffer	HgFileBuffer;

struct _HieroGlyphFileBuffer {
	gchar *buffer;
	gint32 bufsize;
	gint32 pos;
};

struct _HieroGlyphFileObject {
	HgObject    object;
	gchar      *filename;
	gint        error;
	guint       access_mode;
	gchar       ungetc;
	gboolean    is_eof : 1;
	union {
		struct {
			gint         fd;
			gboolean     is_mmap;
			HgFileBuffer mmap;
		} file;
		HgFileBuffer          buf;
		struct {
			gpointer              user_data;
			HgFileObjectCallback *vtable;
		} callback;
	} is;
};


static void     _hg_file_object_real_free     (gpointer           data);
static void     _hg_file_object_real_set_flags(gpointer           data,
					       guint              flags);
static void     _hg_file_object_real_relocate (gpointer           data,
					       HgMemRelocateInfo *info);
static gpointer _hg_file_object_real_to_string(gpointer           data);


HgFileObject *__hg_file_stdin = NULL;
HgFileObject *__hg_file_stdout = NULL;
HgFileObject *__hg_file_stderr = NULL;
static gboolean __hg_file_is_initialized = FALSE;
static HgMemPool *__hg_file_mem_pool = NULL;
static HgAllocator *__hg_file_allocator = NULL;
static HgObjectVTable __hg_file_vtable = {
	.free      = _hg_file_object_real_free,
	.set_flags = _hg_file_object_real_set_flags,
	.relocate  = _hg_file_object_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = _hg_file_object_real_to_string,
};

/*
 * Private Functions
 */
static void
_hg_file_object_real_free(gpointer data)
{
	HgFileObject *file = data;

	switch (HG_FILE_GET_FILE_TYPE (file)) {
	    case HG_FILE_TYPE_FILE:
		    if (file->is.file.is_mmap)
			    munmap(file->is.file.mmap.buffer, file->is.file.mmap.bufsize);
		    if (file->is.file.fd != -1)
			    close(file->is.file.fd);
		    break;
	    case HG_FILE_TYPE_BUFFER:
	    case HG_FILE_TYPE_STDIN:
	    case HG_FILE_TYPE_STDOUT:
	    case HG_FILE_TYPE_STDERR:
	    case HG_FILE_TYPE_STATEMENT_EDIT:
	    case HG_FILE_TYPE_LINE_EDIT:
	    case HG_FILE_TYPE_BUFFER_WITH_CALLBACK:
		    break;
	    default:
		    g_warning("Unknown file type %d was given to be freed.", HG_FILE_GET_FILE_TYPE (file));
		    break;
	}
}

static void
_hg_file_object_real_set_flags(gpointer data,
			       guint    flags)
{
	HgFileObject *file = data;
	HgMemObject *obj;

	if (file->filename) {
		hg_mem_get_object__inline(file->filename, obj);
		if (obj == NULL) {
			g_warning("Invalid object %p to be marked: HgFileObject->filename", file->filename);
		} else {
#ifdef DEBUG_GC
			G_STMT_START {
				if ((flags & HG_MEMOBJ_MARK_AGE_MASK) != 0) {
					g_print("%s: marking filename %p\n", __FUNCTION__, obj);
				}
			} G_STMT_END;
#endif /* DEBUG_GC */
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
	if ((HG_FILE_GET_FILE_TYPE (file) == HG_FILE_TYPE_BUFFER ||
	     HG_FILE_GET_FILE_TYPE (file) == HG_FILE_TYPE_STATEMENT_EDIT ||
	     HG_FILE_GET_FILE_TYPE (file) == HG_FILE_TYPE_LINE_EDIT) &&
	    file->is.buf.buffer) {
		hg_mem_get_object__inline(file->is.buf.buffer, obj);
		if (obj == NULL) {
			g_warning("Invalid object %p to be marked: HgFileObject->buffer", file->is.buf.buffer);
		} else {
#ifdef DEBUG_GC
		G_STMT_START {
			if ((flags & HG_MEMOBJ_MARK_AGE_MASK) != 0) {
				g_print("%s: marking buffer %p\n", __FUNCTION__, obj);
			}
		} G_STMT_END;
#endif /* DEBUG_GC */
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_file_object_real_relocate(gpointer           data,
			      HgMemRelocateInfo *info)
{
	HgFileObject *file = data;

	if ((gsize)file->filename >= info->start &&
	    (gsize)file->filename <= info->end) {
		file->filename = (gchar *)((gsize)file->filename + info->diff);
	}
	if ((HG_FILE_GET_FILE_TYPE (file) == HG_FILE_TYPE_BUFFER ||
	     HG_FILE_GET_FILE_TYPE (file) == HG_FILE_TYPE_STATEMENT_EDIT ||
	     HG_FILE_GET_FILE_TYPE (file) == HG_FILE_TYPE_LINE_EDIT) &&
	    file->is.buf.buffer) {
		if ((gsize)file->is.buf.buffer >= info->start &&
		    (gsize)file->is.buf.buffer <= info->end) {
			file->is.buf.buffer = (gchar *)((gsize)file->is.buf.buffer + info->diff);
		}
	}
}

static gpointer
_hg_file_object_real_to_string(gpointer data)
{
	HgMemObject *obj;
	HgString *retval;
	HgFileObject *file = data;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;

	retval = hg_string_new(obj->pool, -1);
	if (HG_FILE_GET_FILE_TYPE (file) == HG_FILE_TYPE_BUFFER) {
		/* it shows as string to evaluate it */
		hg_string_append_c(retval, '(');
		hg_string_append(retval, file->is.buf.buffer + file->is.buf.pos, file->is.buf.bufsize - file->is.buf.pos);
		hg_string_append(retval, ")/-file-", -1);
	} else {
		hg_string_append(retval, "-file-", -1);
	}
	hg_string_fix_string_size(retval);

	return retval;
}

static gint
_hg_file_get_access_mode(guint mode)
{
	static gint modes[] = {
		0, /* 0 */
		O_RDONLY,                      /* HG_OPEN_MODE_READ */
		O_WRONLY | O_CREAT | O_TRUNC,  /* HG_OPEN_MODE_WRITE */
		O_RDWR,                        /* HG_OPEN_MODE_READ | HG_OPEN_MODE_WRITE */
		O_WRONLY | O_CREAT | O_APPEND, /* HG_OPEN_MODE_READWRITE */
		O_RDWR,                        /* HG_OPEN_MODE_READ | HG_OPEN_MODE_READWRITE */
		O_RDWR | O_CREAT | O_TRUNC,    /* HG_OPEN_MODE_WRITE | HG_OPEN_MODE_READWRITE */
		O_RDWR | O_CREAT | O_APPEND,   /* HG_OPEN_MODE_READ | HG_OPEN_MODE_WRITE | HG_OPEN_MODE_READWRITE */
	};

	return modes[mode];
}

/*
 * Public Functions
 */

/* initializer */
void
hg_file_init(void)
{
	hg_mem_init();

	if (!__hg_file_is_initialized) {
		__hg_file_allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
		__hg_file_mem_pool = hg_mem_pool_new(__hg_file_allocator,
						     "Memory pool for HgFileObject",
						     sizeof (HgFileObject) * 128,
						     FALSE);
		__hg_file_stdin = hg_file_object_new(__hg_file_mem_pool,
						     HG_FILE_TYPE_STDIN);
		hg_mem_add_root_node(__hg_file_mem_pool, __hg_file_stdin);
		__hg_file_stdout = hg_file_object_new(__hg_file_mem_pool,
						      HG_FILE_TYPE_STDOUT);
		hg_object_writable((HgObject *)__hg_file_stdout);
		hg_mem_add_root_node(__hg_file_mem_pool, __hg_file_stdout);
		__hg_file_stderr = hg_file_object_new(__hg_file_mem_pool,
						      HG_FILE_TYPE_STDERR);
		hg_object_writable((HgObject *)__hg_file_stderr);
		hg_mem_add_root_node(__hg_file_mem_pool, __hg_file_stderr);

		__hg_file_is_initialized = TRUE;
	}
}

void
hg_file_finalize(void)
{
	if (__hg_file_is_initialized) {
		hg_mem_pool_destroy(__hg_file_mem_pool);
		hg_allocator_destroy(__hg_file_allocator);

		__hg_file_is_initialized = FALSE;
	}
}

gboolean
hg_file_is_initialized(void)
{
	return __hg_file_is_initialized;
}

/* file object */
HgFileObject *
hg_file_object_new(HgMemPool  *pool,
		   HgFileType  file_type,
		   ...)
{
	HgFileObject *retval;
	va_list ap;
	gchar *p;
	gsize len;
	gint flag;

	g_return_val_if_fail (pool != NULL, NULL);

	retval = hg_mem_alloc_with_flags(pool, sizeof (HgFileObject),
					 HG_FL_HGOBJECT);
	if (retval == NULL) {
		g_warning("Failed to create a file object.");
		return NULL;
	}
	HG_OBJECT_INIT_STATE (&retval->object);
	HG_OBJECT_SET_STATE (&retval->object, hg_mem_pool_get_default_access_mode(pool));
	hg_object_set_vtable(&retval->object, &__hg_file_vtable);

	HG_FILE_SET_FILE_TYPE (retval, file_type);
	/* initialize filename here to avoid a warning
	   when allocating a memory for this and run GC. */
	retval->filename = NULL;
	retval->error = 0;
	retval->ungetc = 0;
	retval->is_eof = FALSE;
	va_start(ap, file_type);
	switch (file_type) {
	    case HG_FILE_TYPE_FILE:
		    retval->access_mode = (guint)va_arg(ap, guint);
		    p = (gchar *)va_arg(ap, gchar *);
		    len = strlen(p);
		    retval->filename = hg_mem_alloc(pool, len + 1);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, p, len);
		    retval->filename[len] = 0;
		    errno = 0;
		    retval->is.file.fd = open(retval->filename,
					      _hg_file_get_access_mode(retval->access_mode));
		    retval->error = errno;
		    if (retval->is.file.fd != -1) {
			    struct stat s;

			    stat(retval->filename, &s);
			    retval->is.file.mmap.buffer = mmap(NULL,
							       s.st_size,
							       PROT_READ,
							       MAP_PRIVATE,
							       retval->is.file.fd,
							       0);
			    if (retval->is.file.mmap.buffer != MAP_FAILED) {
				    retval->is.file.mmap.bufsize = s.st_size;
				    retval->is.file.mmap.pos = 0;
				    retval->is.file.is_mmap = TRUE;
			    } else {
				    retval->is.file.is_mmap = FALSE;
			    }
		    }
		    break;
	    case HG_FILE_TYPE_BUFFER:
		    retval->access_mode = (guint)va_arg(ap, guint);
		    p = (gchar *)va_arg(ap, gchar *);
		    len = strlen(p);
		    retval->filename = hg_mem_alloc(pool, len + 1);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, p, len);
		    retval->filename[len] = 0;
		    p = (gchar *)va_arg(ap, gchar *);
		    retval->is.buf.bufsize = (gint32)va_arg(ap, gint32);
		    if (retval->is.buf.bufsize < 0)
			    retval->is.buf.bufsize = strlen(p);
		    retval->is.buf.buffer = NULL;
		    retval->is.buf.buffer = hg_mem_alloc(pool, retval->is.buf.bufsize);
		    if (retval->is.buf.buffer == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    memcpy(retval->is.buf.buffer, p, retval->is.buf.bufsize);
		    retval->is.buf.pos = 0;
		    break;
	    case HG_FILE_TYPE_STDIN:
		    retval->filename = hg_mem_alloc(pool, 6);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, "stdin", 5);
		    retval->filename[5] = 0;
		    retval->is.file.fd = 0;
		    retval->is.file.is_mmap = FALSE;
		    retval->access_mode = HG_FILE_MODE_READ;
		    flag = fcntl(0, F_GETFL);
		    fcntl(0, F_SETFL, flag | O_NONBLOCK);
		    break;
	    case HG_FILE_TYPE_STDOUT:
		    retval->filename = hg_mem_alloc(pool, 7);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, "stdout", 6);
		    retval->filename[6] = 0;
		    retval->is.file.fd = 1;
		    retval->is.file.is_mmap = FALSE;
		    retval->access_mode = HG_FILE_MODE_WRITE;
		    break;
	    case HG_FILE_TYPE_STDERR:
		    retval->filename = hg_mem_alloc(pool, 7);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, "stderr", 6);
		    retval->filename[6] = 0;
		    retval->is.file.fd = 2;
		    retval->is.file.is_mmap = FALSE;
		    retval->access_mode = HG_FILE_MODE_WRITE;
		    break;
	    case HG_FILE_TYPE_STATEMENT_EDIT:
		    retval->filename = hg_mem_alloc(pool, 15);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, "%statementedit", 14);
		    retval->filename[14] = 0;
		    p = hg_line_edit_get_statement(__hg_file_stdin, NULL);
		    if (p == NULL) {
			    g_warning("Failed to read a statement.");
			    return NULL;
		    }
		    retval->access_mode = HG_FILE_MODE_READ;
		    retval->is.buf.bufsize = strlen(p);
		    retval->is.buf.buffer = NULL;
		    retval->is.buf.buffer = hg_mem_alloc(pool, retval->is.buf.bufsize);
		    if (retval->is.buf.buffer == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    memcpy(retval->is.buf.buffer, p, retval->is.buf.bufsize);
		    g_free(p);
		    retval->is.buf.pos = 0;
		    break;
	    case HG_FILE_TYPE_LINE_EDIT:
		    retval->filename = hg_mem_alloc(pool, 10);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, "%lineedit", 9);
		    retval->filename[9] = 0;
		    p = hg_line_edit_get_line(__hg_file_stdin, NULL, TRUE);
		    if (p == NULL) {
			    g_warning("Failed to read a statement.");
			    return NULL;
		    }
		    retval->access_mode = HG_FILE_MODE_READ;
		    retval->is.buf.bufsize = strlen(p);
		    retval->is.buf.buffer = NULL;
		    retval->is.buf.buffer = hg_mem_alloc(pool, retval->is.buf.bufsize);
		    if (retval->is.buf.buffer == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    memcpy(retval->is.buf.buffer, p, retval->is.buf.bufsize);
		    g_free(p);
		    retval->is.buf.pos = 0;
		    break;
	    case HG_FILE_TYPE_BUFFER_WITH_CALLBACK:
		    retval->access_mode = (guint)va_arg(ap, guint);
		    p = (gchar *)va_arg(ap, gchar *);
		    len = strlen(p);
		    retval->filename = hg_mem_alloc(pool, len + 1);
		    if (retval->filename == NULL) {
			    g_warning("Failed to allocate a memory for file object.");
			    return NULL;
		    }
		    strncpy(retval->filename, p, len);
		    retval->filename[len] = 0;
		    retval->is.callback.vtable = (HgFileObjectCallback *)va_arg(ap, HgFileObjectCallback *);
		    retval->is.callback.user_data = (gpointer)va_arg(ap, gpointer);
		    break;
	    default:
		    g_warning("Unknown file type %d\n", HG_FILE_GET_FILE_TYPE (retval));
		    retval = NULL;
		    break;
	}
	va_end(ap);

	return retval;
}

gboolean
hg_file_object_has_error(HgFileObject *object)
{
	g_return_val_if_fail (object != NULL, TRUE);

	switch (HG_FILE_GET_FILE_TYPE (object)) {
	    case HG_FILE_TYPE_FILE:
		    if (object->is.file.fd == -1)
			    return TRUE;
		    break;
	    case HG_FILE_TYPE_STATEMENT_EDIT:
	    case HG_FILE_TYPE_LINE_EDIT:
		    if (object->is.buf.bufsize == 0) {
			    object->error = ENOENT;
			    return TRUE;
		    }
	    case HG_FILE_TYPE_BUFFER:
		    if (object->is.buf.buffer == NULL) {
			    object->error = EACCES;
			    return TRUE;
		    }
		    break;
	    case HG_FILE_TYPE_STDIN:
	    case HG_FILE_TYPE_STDOUT:
	    case HG_FILE_TYPE_STDERR:
		    break;
	    case HG_FILE_TYPE_BUFFER_WITH_CALLBACK:
		    object->error = object->is.callback.vtable->get_error_code(object->is.callback.user_data);
		    break;
	    default:
		    g_warning("[BUG] Invalid file type %d was given to check the error.", HG_FILE_GET_FILE_TYPE (object));
		    return TRUE;
	}

	return object->error != 0;
}

gint
hg_file_object_get_error(HgFileObject *object)
{
	g_return_val_if_fail (object != NULL, EIO);

	return object->error;
}

void
hg_file_object_clear_error(HgFileObject *object)
{
	g_return_if_fail (object != NULL);

	object->error = 0;
}

gboolean
hg_file_object_is_eof(HgFileObject *object)
{
	g_return_val_if_fail (object != NULL, TRUE);

	return object->is_eof;
}

gsize
hg_file_object_read(HgFileObject *object,
		    gpointer      buffer,
		    gsize         size,
		    gsize         n)
{
	gsize retval = 0;

	g_return_val_if_fail (object != NULL, 0);
	g_return_val_if_fail (buffer != NULL, 0);
	g_return_val_if_fail ((object->access_mode & HG_FILE_MODE_READ) != 0, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)object), FALSE);

	/* FIXME: need to handle ungetc here properly */
	if (object->ungetc != 0) {
		g_warning("FIXME: ungetc handling not yet implemented!!");
	}
	switch (HG_FILE_GET_FILE_TYPE (object)) {
	    case HG_FILE_TYPE_FILE:
		    if (object->is.file.is_mmap) {
			    if ((object->is.file.mmap.bufsize - object->is.file.mmap.pos) < (size * n))
				    retval = object->is.file.mmap.bufsize - object->is.file.mmap.pos;
			    else
				    retval = size * n;
			    memcpy(buffer, object->is.file.mmap.buffer + object->is.file.mmap.pos, retval);
			    ((gchar *)buffer)[retval] = 0;
			    object->is.file.mmap.pos += retval;
			    if (retval == 0 &&
				object->is.file.mmap.pos == object->is.file.mmap.bufsize)
				    object->is_eof = TRUE;
			    break;
		    }
	    case HG_FILE_TYPE_STDIN:
		    errno = 0;
		    retval = read(object->is.file.fd, buffer, size * n);
		    object->error = errno;
		    if (size * n != 0 && retval == 0)
			    object->is_eof = TRUE;
		    break;
	    case HG_FILE_TYPE_BUFFER:
	    case HG_FILE_TYPE_STATEMENT_EDIT:
	    case HG_FILE_TYPE_LINE_EDIT:
		    if ((object->is.buf.bufsize - object->is.buf.pos) < (size * n))
			    retval = object->is.buf.bufsize - object->is.buf.pos;
		    else
			    retval = size * n;
		    memcpy(buffer, object->is.buf.buffer + object->is.buf.pos, retval);
		    ((gchar *)buffer)[retval] = 0;
		    object->is.buf.pos += retval;
		    if (retval == 0 &&
			object->is.buf.pos == object->is.buf.bufsize)
			    object->is_eof = TRUE;
		    break;
	    case HG_FILE_TYPE_BUFFER_WITH_CALLBACK:
		    retval = object->is.callback.vtable->read(object->is.callback.user_data, buffer, size, n);
		    object->is_eof = object->is.callback.vtable->is_eof(object->is.callback.user_data);
		    object->error = object->is.callback.vtable->get_error_code(object->is.callback.user_data);
		    break;
	    default:
		    g_warning("Invalid file type %d was given to be read.", HG_FILE_GET_FILE_TYPE (object));
		    object->error = EACCES;
		    break;
	}

	return retval;
}

gsize
hg_file_object_write(HgFileObject  *object,
		     gconstpointer  buffer,
		     gsize          size,
		     gsize          n)
{
	gsize retval = 0;

	g_return_val_if_fail (object != NULL, 0);
	g_return_val_if_fail (buffer != NULL, 0);
	g_return_val_if_fail ((object->access_mode & HG_FILE_MODE_WRITE) != 0, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)object), FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)object), FALSE);

	switch (HG_FILE_GET_FILE_TYPE (object)) {
	    case HG_FILE_TYPE_FILE:
	    case HG_FILE_TYPE_STDOUT:
	    case HG_FILE_TYPE_STDERR:
		    errno = 0;
		    retval = write(object->is.file.fd, buffer, size * n);
		    object->error = errno;
		    break;
	    case HG_FILE_TYPE_BUFFER:
		    if ((object->is.buf.bufsize - object->is.buf.pos) < (size * n))
			    retval = object->is.buf.bufsize - object->is.buf.pos;
		    else
			    retval = size * n;
		    memcpy(object->is.buf.buffer + object->is.buf.pos, buffer, retval);
		    ((gchar *)buffer)[retval] = 0;
		    object->is.buf.pos += retval;
		    if (retval == 0 &&
			object->is.buf.pos == object->is.buf.bufsize)
			    object->error = EIO;
		    break;
	    case HG_FILE_TYPE_BUFFER_WITH_CALLBACK:
		    retval = object->is.callback.vtable->write(object->is.callback.user_data, buffer, size, n);
		    object->error = object->is.callback.vtable->get_error_code(object->is.callback.user_data);
		    break;
	    default:
		    g_warning("Invalid file type %d to be wrriten.", HG_FILE_GET_FILE_TYPE (object));
		    object->error = EACCES;
		    break;
	}

	return retval;
}

gchar
hg_file_object_getc(HgFileObject *object)
{
	gchar retval = 0;

	g_return_val_if_fail (object != NULL, 0);
	g_return_val_if_fail ((object->access_mode & HG_FILE_MODE_READ) != 0, 0);
	g_return_val_if_fail (hg_object_is_readable((HgObject *)object), FALSE);

	if (object->ungetc != 0) {
		retval = object->ungetc;
		object->ungetc = 0;
	} else {
		switch (HG_FILE_GET_FILE_TYPE (object)) {
		    case HG_FILE_TYPE_FILE:
			    if (object->is.file.is_mmap) {
				    if (object->is.file.mmap.pos == object->is.file.mmap.bufsize)
					    object->is_eof = TRUE;
				    if (object->is.file.mmap.pos < object->is.file.mmap.bufsize)
					    retval = object->is.file.mmap.buffer[object->is.file.mmap.pos++];
				    break;
			    }
		    case HG_FILE_TYPE_STDIN:
			    errno = 0;
			    if (read(object->is.file.fd, &retval, 1) == 0)
				    object->is_eof = TRUE;
			    object->error = errno;
			    break;
		    case HG_FILE_TYPE_BUFFER:
		    case HG_FILE_TYPE_STATEMENT_EDIT:
		    case HG_FILE_TYPE_LINE_EDIT:
			    if (object->is.buf.pos == object->is.buf.bufsize)
				    object->is_eof = TRUE;
			    if (object->is.buf.pos < object->is.buf.bufsize)
				    retval = object->is.buf.buffer[object->is.buf.pos++];
			    break;
		    case HG_FILE_TYPE_BUFFER_WITH_CALLBACK:
			    retval = object->is.callback.vtable->getc(object->is.callback.user_data);
			    object->is_eof = object->is.callback.vtable->is_eof(object->is.callback.user_data);
			    object->error = object->is.callback.vtable->get_error_code(object->is.callback.user_data);
			    break;
		    default:
			    g_warning("Invalid file type %d was given to be get a character.", HG_FILE_GET_FILE_TYPE (object));
			    break;
		}
	}

	return retval;
}

void
hg_file_object_ungetc(HgFileObject *object,
		      gchar         c)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail ((object->access_mode & HG_FILE_MODE_READ) != 0);
	g_return_if_fail (object->ungetc == 0);
	g_return_if_fail (hg_object_is_readable((HgObject *)object));
	g_return_if_fail (hg_object_is_writable((HgObject *)object));

	object->is_eof = FALSE;
	object->ungetc = c;
}

gboolean
hg_file_object_flush(HgFileObject *object)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (object != NULL, FALSE);
	g_return_val_if_fail (hg_object_is_writable((HgObject *)object), FALSE);

	switch (HG_FILE_GET_FILE_TYPE (object)) {
	    case HG_FILE_TYPE_FILE:
	    case HG_FILE_TYPE_STDOUT:
	    case HG_FILE_TYPE_STDERR:
		    sync();
		    retval = TRUE;
		    break;
	    case HG_FILE_TYPE_BUFFER:
		    object->is.buf.pos = 0;
		    retval = TRUE;
		    break;
	    case HG_FILE_TYPE_BUFFER_WITH_CALLBACK:
		    retval = object->is.callback.vtable->flush(object->is.callback.user_data);
		    break;
	    default:
		    g_warning("Invalid file type %d was given to be flushed.", HG_FILE_GET_FILE_TYPE (object));
		    break;
	}

	return retval;
}

void
hg_file_object_printf(HgFileObject *object,
		      gchar const *format,
		      ...)
{
	va_list ap;

	va_start(ap, format);
	hg_file_object_vprintf(object, format, ap);
	va_end(ap);
}

void
hg_file_object_vprintf(HgFileObject *object,
		       gchar const  *format,
		       va_list       va_args)
{
	gchar *buffer;

	g_return_if_fail (object != NULL);
	g_return_if_fail (format != NULL);
	g_return_if_fail (hg_object_is_readable((HgObject *)object));
	g_return_if_fail (hg_object_is_writable((HgObject *)object));

	buffer = g_strdup_vprintf(format, va_args);
	hg_file_object_write(object, buffer, sizeof (gchar), strlen(buffer));
	g_free(buffer);
}

void
hg_stdout_printf(gchar const *format, ...)
{
	va_list ap;

	if (!hg_file_is_initialized())
		hg_file_init();

	va_start(ap, format);
	hg_file_object_vprintf(__hg_file_stdout, format, ap);
	va_end(ap);
}

void
hg_stderr_printf(gchar const *format, ...)
{
	va_list ap;

	if (!hg_file_is_initialized())
		hg_file_init();

	va_start(ap, format);
	hg_file_object_vprintf(__hg_file_stderr, format, ap);
	va_end(ap);
}
