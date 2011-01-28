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

#define HG_FILE_INIT						\
	HG_STRING_INIT;						\
	(hg_object_register(HG_TYPE_FILE,			\
			    hg_object_file_get_vtable()))
#define HG_IS_QFILE(_q_)				\
	(hg_quark_get_type((_q_)) == HG_TYPE_FILE)

typedef enum _hg_file_io_t			hg_file_io_t;
typedef enum _hg_file_mode_t			hg_file_mode_t;
typedef enum _hg_file_pos_t			hg_file_pos_t;
typedef struct _hg_file_vtable_t		hg_file_vtable_t;
typedef struct _hg_file_t			hg_file_t;
typedef struct _hg_file_io_data_t		hg_file_io_data_t;
typedef struct _hg_file_gc_t			hg_file_gc_t;
typedef void (* hg_file_yybuffer_finalizer_func_t) (gpointer yybuffer,
						    gpointer yydata);

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
	gboolean (* open)      (hg_file_t      *file,
				gpointer        user_data,
				GError        **error);
	void     (* close)     (hg_file_t      *file,
				gpointer        user_data,
				GError        **error);
	gssize   (* read)      (hg_file_t      *file,
				gpointer        user_data,
				gpointer        buffer,
				gsize           size,
				gsize           n,
				GError        **error);
	gssize   (* write)     (hg_file_t      *file,
				gpointer        user_data,
				gconstpointer   buffer,
				gsize           size,
				gsize           n,
				GError        **error);
	gboolean (* flush)     (hg_file_t      *file,
				gpointer        user_data,
				GError        **error);
	gssize   (* seek)      (hg_file_t      *file,
				gpointer        user_data,
				gssize          offset,
				hg_file_pos_t   whence,
				GError        **error);
	gboolean (* is_eof)    (hg_file_t      *file,
				gpointer        user_data);
	void     (* clear_eof) (hg_file_t      *file,
				gpointer        user_data);
};
struct _hg_file_t {
	hg_object_t                       o;
	hg_quark_t                        qfilename;
	hg_file_io_t                      io_type;
	hg_file_mode_t                    mode;
	hg_file_vtable_t                 *vtable;
	gsize                             size;
	gssize                            position;
	gssize                            lineno;
	gboolean                          is_closed;
	hg_file_io_data_t                *user_data;
	hg_file_yybuffer_finalizer_func_t yyfree;
	gpointer                          yybuffer;
	gpointer                          yydata;
};
struct _hg_file_io_data_t {
	hg_quark_t           self;
	hg_gc_iterate_func_t gc_func;
	hg_destroy_func_t    destroy_func;
	gboolean             is_eof;
	gint                 fd;
	gpointer             mmapped_buffer;
};
struct _hg_file_gc_t {
	hg_gc_iterate_func_t  func;
	gpointer              user_data;
	hg_mem_t             *mem;
	hg_file_io_data_t    *data;
};


hg_object_vtable_t     *hg_object_file_get_vtable  (void) G_GNUC_CONST;
const hg_file_vtable_t *hg_file_get_lineedit_vtable(void) G_GNUC_CONST;
hg_file_io_t            hg_file_get_io_type        (const gchar                       *name);
hg_quark_t              hg_file_new                (hg_mem_t                          *mem,
                                                    const gchar                       *name,
                                                    hg_file_mode_t                     mode,
                                                    GError                            **error,
                                                    gpointer                          *ret);
hg_quark_t              hg_file_new_with_vtable    (hg_mem_t                          *mem,
                                                    const gchar                       *name,
                                                    hg_file_mode_t                     mode,
                                                    const hg_file_vtable_t            *vtable,
                                                    gpointer                           user_data,
                                                    GError                            **error,
                                                    gpointer                          *ret);
hg_quark_t              hg_file_new_with_string    (hg_mem_t                          *mem,
                                                    const gchar                       *name,
                                                    hg_file_mode_t                     mode,
                                                    hg_string_t                       *in,
                                                    hg_string_t                       *out,
                                                    GError                            **error,
                                                    gpointer                          *ret);
void                    hg_file_close              (hg_file_t                         *file,
                                                    GError                            **error);
gssize                  hg_file_read               (hg_file_t                         *file,
                                                    gpointer                           buffer,
                                                    gsize                              size,
                                                    gsize                              n,
                                                    GError                            **error);
gssize                  hg_file_write              (hg_file_t                         *file,
                                                    gpointer                           buffer,
                                                    gsize                              size,
                                                    gsize                              n,
                                                    GError                            **error);
gboolean                hg_file_flush              (hg_file_t                         *file,
                                                    GError                            **error);
gssize                  hg_file_seek               (hg_file_t                         *file,
                                                    gssize                             offset,
                                                    hg_file_pos_t                      whence,
                                                    GError                            **error);
gboolean                hg_file_is_closed          (hg_file_t                         *file);
gboolean                hg_file_is_eof             (hg_file_t                         *file);
void                    hg_file_clear_eof          (hg_file_t                         *file);
gssize                  hg_file_append_printf      (hg_file_t                         *file,
                                                    gchar const                       *format,
						    ...);
gssize                  hg_file_append_vprintf     (hg_file_t                         *file,
                                                    gchar const                       *format,
                                                    va_list                            args);
void                    hg_file_set_lineno         (hg_file_t                         *file,
                                                    gssize                             n);
gssize                  hg_file_get_lineno         (hg_file_t                         *file);
gssize                  hg_file_get_position       (hg_file_t                         *file);
void                    hg_file_set_yybuffer       (hg_file_t                         *file,
                                                    gpointer                           yybuffer,
                                                    hg_file_yybuffer_finalizer_func_t  func,
						    gpointer                           user_data);
gpointer                hg_file_get_yybuffer       (hg_file_t                         *file);
void                    hg_file_set_error          (GError                           **error,
						    const gchar                       *function,
						    guint                              errno_);


HG_END_DECLS

#endif /* __HIEROGLYPH_HGFILE_H__ */
