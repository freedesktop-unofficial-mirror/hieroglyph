/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.c
 * Copyright (C) 2006-2007 Akira TAGOH
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

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib/gstrfuncs.h>
#include <hieroglyph/hgobject.h>
#include <hieroglyph/vm.h>
#include "hgfile.h"


/*
 * private functions
 */
static hg_filetype_t
_hg_object_file_get_io_type(const gchar *filename)
{
	if (strcmp(filename, "%stdin") == 0) {
		return HG_FILE_TYPE_STDIN;
	} else if (strcmp(filename, "%stdout") == 0) {
		return HG_FILE_TYPE_STDOUT;
	} else if (strcmp(filename, "%stderr") == 0) {
		return HG_FILE_TYPE_STDERR;
	} else if (strcmp(filename, "%lineedit") == 0) {
		return HG_FILE_TYPE_LINEEDIT;
	} else if (strcmp(filename, "%statementedit") == 0) {
		return HG_FILE_TYPE_STATEMENTEDIT;
	}

	return HG_FILE_TYPE_FILE_IO;
}

static int
_hg_object_file_get_open_mode(hg_filemode_t mode)
{
	return 0;
}

/*
 * public functions
 */
hg_object_t *
hg_object_file_new(hg_vm_t       *vm,
		   const gchar   *filename,
		   hg_filemode_t  mode)
{
	hg_object_t *retval, *obj, *string;
	gsize len, buflen = 0;
	struct stat st;
	hg_filetype_t iotype;
	hg_filedata_t *data;
	int fd;
	gpointer buffer = NULL;
	gboolean set_io = FALSE;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (filename != NULL, NULL);

	len = strlen(filename);
	iotype = _hg_object_file_get_io_type(filename);
	switch (iotype) {
	    case HG_FILE_TYPE_FILE_IO:
		    if (stat(filename, &st) == -1) {
			    hg_object_file_notify_error(vm, errno);

			    return NULL;
		    }
		    buflen = st.st_size;
		    if ((fd = open(filename, _hg_object_file_get_open_mode(mode))) == -1) {
			    hg_object_file_notify_error(vm, errno);

			    return NULL;
		    }
		    if ((buffer = mmap(NULL, buflen, PROT_READ, MAP_PRIVATE, fd, 0)) != MAP_FAILED) {
			    iotype = HG_FILE_TYPE_MMAPPED_IO;
		    }
		    break;
	    case HG_FILE_TYPE_STDIN:
		    if ((mode & ~HG_FILE_MODE_READ) != 0) {
			    hg_object_file_notify_error(vm, EIO);

			    return NULL;
		    }
		    if ((retval = hg_vm_get_io(vm, iotype)) != NULL)
			    return retval;
		    fd = dup(0);
		    set_io = TRUE;
		    break;
	    case HG_FILE_TYPE_STDOUT:
		    if ((mode & ~HG_FILE_MODE_WRITE) != 0) {
			    hg_object_file_notify_error(vm, EIO);

			    return NULL;
		    }
		    if ((retval = hg_vm_get_io(vm, iotype)) != NULL)
			    return retval;
		    fd = dup(1);
		    set_io = TRUE;
		    break;
	    case HG_FILE_TYPE_STDERR:
		    if ((mode & ~HG_FILE_MODE_WRITE) != 0) {
			    hg_object_file_notify_error(vm, EIO);

			    return NULL;
		    }
		    if ((retval = hg_vm_get_io(vm, iotype)) != NULL)
			    return retval;
		    fd = dup(2);
		    set_io = TRUE;
		    break;
	    case HG_FILE_TYPE_LINEEDIT:
	    case HG_FILE_TYPE_STATEMENTEDIT:
		    if ((mode & ~HG_FILE_MODE_READ) != 0) {
			    hg_object_file_notify_error(vm, EIO);

			    return NULL;
		    }
		    if ((obj = hg_vm_get_io(vm, iotype)) == NULL) {
			    hg_object_file_notify_error(vm, EIO);

			    return NULL;
		    }
		    /* XXX */
		    return hg_object_file_new_from_string(vm, string, HG_FILE_MODE_READ);
	    default:
		    g_warning("Unknown file type `%d' in creating file object", iotype);
		    return NULL;
	}
	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_filedata_t) + sizeof (gchar) * (len + 1)));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_FILE;
		data = HG_OBJECT_FILE_DATA (retval);
		memset(data, 0, sizeof (hg_filedata_t) + sizeof (gchar) * (len + 1));
		data->v.buffer = buffer;
		data->fd = fd;
		data->filesize = buflen;
		data->current_position = 0;
		data->current_line = 0;
		data->iotype = iotype;
		data->mode = mode;
		data->filename_length = len;
		memcpy(data->filename, filename, len);
		if (set_io)
			hg_vm_set_io(vm, retval);
	} else {
		if (buffer)
			munmap(buffer, buflen);
		close(fd);
	}

	return retval;
}

hg_object_t *
hg_object_file_new_from_string(hg_vm_t       *vm,
			       hg_object_t   *string,
			       hg_filemode_t  mode)
{
	hg_object_t *retval;
	static gchar name[] = "%file buffer";
	gsize len = strlen(name);
	hg_filedata_t *data;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (string != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_STRING (string), NULL);

	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_filedata_t) + sizeof (gchar) * (len + 1)));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_FILE;
		data = HG_OBJECT_FILE_DATA (retval);
		data->v.buffer = string;
		data->fd = -1;
		data->filesize = HG_OBJECT_STRING (string)->real_length;
		data->current_position = 0;
		data->current_line = 0;
		data->iotype = HG_FILE_TYPE_BUFFER;
		data->mode = mode;
		data->filename_length = len;
		memcpy(data->filename, name, len);
		data->filename[len] = 0;
	}

	return retval;
}

hg_object_t *
hg_object_file_new_with_custom(hg_vm_t        *vm,
			       hg_filetable_t *table,
			       hg_filemode_t   mode)
{
	hg_object_t *retval;
	static gchar name[] = "%custom file object";
	gsize len = strlen(name);
	hg_filedata_t *data;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (table != NULL, NULL);

	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_filedata_t) + sizeof (gchar) * (len + 1)));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_FILE;
		data = HG_OBJECT_FILE_DATA (retval);
		memset(data, 0, sizeof (hg_filedata_t) + sizeof (gchar) * (len + 1));
		data->v.table = table;
		data->filesize = 0;
		data->current_position = 0;
		data->current_line = 0;
		data->iotype = HG_FILE_TYPE_CALLBACK;
		data->mode = mode;
		data->filename_length = len;
		memcpy(data->filename, name, len);
	}

	return retval;
}

void
hg_object_file_free(hg_vm_t     *vm,
		    hg_object_t *object)
{
	hg_return_if_fail (vm != NULL);
	hg_return_if_fail (object != NULL);
	hg_return_if_fail (HG_OBJECT_IS_FILE (object));

	switch (HG_OBJECT_FILE_DATA (object)->iotype) {
	    default:
		    break;
	}
}

void
hg_object_file_notify_error(hg_vm_t     *vm,
			    error_t      _errno)
{
	hg_error_t error = 0;
	gchar buffer[4096];

	hg_return_if_fail (vm != NULL);

	switch (_errno) {
	    case 0:
		    break;
	    case EACCES:
	    case EBADF:
	    case EEXIST:
	    case ENOTDIR:
	    case ENOTEMPTY:
	    case EPERM:
	    case EROFS:
		    error = HG_e_invalidfileaccess;
		    break;
	    case EAGAIN:
	    case EBUSY:
	    case EIO:
	    case ENOSPC:
		    error = HG_e_ioerror;
		    break;
	    case EMFILE:
		    error = HG_e_limitcheck;
		    break;
	    case ENAMETOOLONG:
	    case ENODEV:
	    case ENOENT:
		    error = HG_e_undefinedfilename;
		    break;
	    case ENOMEM:
		    error = HG_e_VMerror;
		    break;
	    default:
		    strerror_r(_errno, buffer, 4096);
		    g_warning("%s: need to support errno %d\n  %s\n",
			      __PRETTY_FUNCTION__, _errno, buffer);
		    break;
	}
	if (error != 0)
		hg_vm_set_error(vm, error);
}

gboolean
hg_object_file_compare(hg_object_t *object1,
		       hg_object_t *object2)
{
	hg_return_val_if_fail (object1 != NULL, FALSE);
	hg_return_val_if_fail (object2 != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_FILE (object1), FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_FILE (object2), FALSE);

	/* XXX: no copy and dup functionalities are available so far.
	 * so just comparing a pointer should works enough
	 */
	return object1 == object2;
}

gchar *
hg_object_file_dump(hg_object_t *object,
		    gboolean     verbose)
{
	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_FILE (object), NULL);

	return g_strdup("--file--");
}
