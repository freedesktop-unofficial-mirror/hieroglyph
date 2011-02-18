/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.h
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGFILE_H__
#define __HIEROGLYPH_HGFILE_H__

#include <unistd.h>
#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgstring.h>

HG_BEGIN_DECLS

#define HG_FILE_INIT							\
	HG_STMT_START {							\
		HG_STRING_INIT;						\
		hg_object_register(HG_TYPE_FILE,			\
				   hg_object_file_get_vtable());	\
	} HG_STMT_END
#define HG_IS_QFILE(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_FILE)

extern HG_THREAD_VAR hg_int_t hg_fileerrno;

typedef enum _hg_file_io_t			hg_file_io_t;
typedef enum _hg_file_mode_t			hg_file_mode_t;
typedef enum _hg_file_pos_t			hg_file_pos_t;
typedef struct _hg_file_vtable_t		hg_file_vtable_t;
typedef struct _hg_file_t			hg_file_t;
typedef struct _hg_file_io_data_t		hg_file_io_data_t;
typedef struct _hg_file_gc_t			hg_file_gc_t;
typedef void (* hg_file_yybuffer_finalizer_func_t) (hg_pointer_t yybuffer,
						    hg_pointer_t yydata);

enum _hg_file_io_t {
	HG_FILE_IO_FILE = 0,
	HG_FILE_IO_STDIN,
	HG_FILE_IO_STDOUT,
	HG_FILE_IO_STDERR,
	HG_FILE_IO_LINEEDIT,
	HG_FILE_IO_STATEMENTEDIT,
	HG_FILE_IO_END
};
enum _hg_file_mode_t {
	HG_FILE_IO_MODE_READ      = 1 << 0,
	HG_FILE_IO_MODE_WRITE     = 1 << 1,
	HG_FILE_IO_MODE_READWRITE = HG_FILE_IO_MODE_READ|HG_FILE_IO_MODE_WRITE,
	HG_FILE_IO_MODE_APPEND    = 1 << 2,
	HG_FILE_IO_MODE_END       = 1 << 3,
};
enum _hg_file_pos_t {
	HG_FILE_POS_BEGIN   = SEEK_SET,
	HG_FILE_POS_CURRENT = SEEK_CUR,
	HG_FILE_POS_END     = SEEK_END
};
struct _hg_file_vtable_t {
	hg_bool_t (* open)      (hg_file_t          *file,
				 hg_pointer_t        user_data);
	void      (* close)     (hg_file_t          *file,
				 hg_pointer_t        user_data);
	hg_size_t (* read)      (hg_file_t          *file,
				 hg_pointer_t        user_data,
				 hg_pointer_t        buffer,
				 hg_usize_t          size,
				 hg_usize_t          n);
	hg_size_t (* write)     (hg_file_t          *file,
				 hg_pointer_t        user_data,
				 const hg_pointer_t  buffer,
				 hg_usize_t          size,
				 hg_usize_t          n);
	hg_bool_t (* flush)     (hg_file_t          *file,
				 hg_pointer_t        user_data);
	hg_size_t (* seek)      (hg_file_t          *file,
				 hg_pointer_t        user_data,
				 hg_size_t           offset,
				 hg_file_pos_t       whence);
	hg_bool_t (* is_eof)    (hg_file_t          *file,
				 hg_pointer_t        user_data);
	void      (* clear_eof) (hg_file_t          *file,
				 hg_pointer_t        user_data);
};
struct _hg_file_t {
	hg_object_t                        o;
	hg_quark_t                         qfilename;
	hg_file_io_t                       io_type;
	hg_file_mode_t                     mode;
	hg_file_vtable_t                  *vtable;
	hg_usize_t                         size;
	hg_size_t                          position;
	hg_size_t                          lineno;
	hg_bool_t                          is_closed;
	hg_file_io_data_t                 *user_data;
	hg_file_yybuffer_finalizer_func_t  yyfree;
	hg_pointer_t                       yybuffer;
	hg_pointer_t                       yydata;
};
struct _hg_file_io_data_t {
	hg_quark_t           self;
	hg_gc_iterate_func_t gc_func;
	hg_destroy_func_t    destroy_func;
	hg_bool_t            is_eof;
	gint                 fd;
	hg_pointer_t         mmapped_buffer;
};
struct _hg_file_gc_t {
	hg_gc_iterate_func_t  func;
	hg_pointer_t          user_data;
	hg_mem_t             *mem;
	hg_file_io_data_t    *data;
};


hg_object_vtable_t     *hg_object_file_get_vtable  (void) G_GNUC_CONST;
const hg_file_vtable_t *hg_file_get_lineedit_vtable(void) G_GNUC_CONST;
hg_file_io_t            hg_file_get_io_type        (const hg_char_t                   *name);
hg_quark_t              hg_file_new                (hg_mem_t                          *mem,
                                                    const hg_char_t                   *name,
                                                    hg_file_mode_t                     mode,
                                                    hg_pointer_t                      *ret);
hg_quark_t              hg_file_new_with_vtable    (hg_mem_t                          *mem,
                                                    const hg_char_t                   *name,
                                                    hg_file_mode_t                     mode,
                                                    const hg_file_vtable_t            *vtable,
                                                    hg_pointer_t                       user_data,
                                                    hg_pointer_t                      *ret);
hg_quark_t              hg_file_new_with_string    (hg_mem_t                          *mem,
                                                    const hg_char_t                   *name,
                                                    hg_file_mode_t                     mode,
                                                    hg_string_t                       *in,
                                                    hg_string_t                       *out,
                                                    hg_pointer_t                      *ret);
void                    hg_file_close              (hg_file_t                         *file);
hg_size_t               hg_file_read               (hg_file_t                         *file,
                                                    hg_pointer_t                       buffer,
                                                    hg_usize_t                         size,
                                                    hg_usize_t                         n);
hg_size_t               hg_file_write              (hg_file_t                         *file,
                                                    hg_pointer_t                       buffer,
                                                    hg_usize_t                         size,
                                                    hg_usize_t                         n);
hg_bool_t               hg_file_flush              (hg_file_t                         *file);
hg_size_t               hg_file_seek               (hg_file_t                         *file,
                                                    hg_size_t                          offset,
                                                    hg_file_pos_t                      whence);
hg_bool_t               hg_file_is_closed          (hg_file_t                         *file);
hg_bool_t               hg_file_is_eof             (hg_file_t                         *file);
void                    hg_file_clear_eof          (hg_file_t                         *file);
hg_size_t               hg_file_append_printf      (hg_file_t                         *file,
                                                    hg_char_t const                   *format,
						    ...);
hg_size_t               hg_file_append_vprintf     (hg_file_t                         *file,
                                                    hg_char_t const                   *format,
                                                    va_list                            args);
void                    hg_file_set_lineno         (hg_file_t                         *file,
                                                    hg_size_t                          n);
hg_size_t               hg_file_get_lineno         (hg_file_t                         *file);
hg_size_t               hg_file_get_position       (hg_file_t                         *file);
void                    hg_file_set_yybuffer       (hg_file_t                         *file,
                                                    hg_pointer_t                       yybuffer,
                                                    hg_file_yybuffer_finalizer_func_t  func,
                                                    hg_pointer_t                       user_data);
hg_pointer_t            hg_file_get_yybuffer       (hg_file_t                         *file);
void                    hg_file_set_error          (const hg_char_t                   *function);


HG_END_DECLS

#endif /* __HIEROGLYPH_HGFILE_H__ */
