/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.h
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
#ifndef __HG_FILE_H__
#define __HG_FILE_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

extern HgFileObject *__hg_file_stdin;
extern HgFileObject *__hg_file_stdout;
extern HgFileObject *__hg_file_stderr;


/* initializer */
void     hg_file_init          (void);
void     hg_file_finalize      (void);
gboolean hg_file_is_initialized(void);

/* file object */
HgFileObject *hg_file_object_new        (HgMemPool  *pool,
					 HgFileType  file_type,
					 ...);
gboolean      hg_file_object_has_error  (HgFileObject *object);
gint          hg_file_object_get_error  (HgFileObject *object);
void          hg_file_object_clear_error(HgFileObject  *object);
gboolean      hg_file_object_is_eof     (HgFileObject  *object);
gsize         hg_file_object_read       (HgFileObject  *object,
					 gpointer       buffer,
					 gsize          size,
					 gsize          n);
gsize         hg_file_object_write      (HgFileObject  *object,
					 gconstpointer  buffer,
					 gsize          size,
					 gsize          n);
gchar         hg_file_object_getc       (HgFileObject  *object);
void          hg_file_object_ungetc     (HgFileObject  *object,
					 gchar          c);
void          hg_file_object_printf     (HgFileObject *object,
					 gchar const  *format,
					 ...) G_GNUC_PRINTF (2, 3);
void          hg_file_object_vprintf    (HgFileObject *object,
					 gchar const  *format,
					 va_list       va_args);

/* for information */
void          hg_stdout_printf        (gchar const *format, ...) G_GNUC_PRINTF (1, 2);
void          hg_stderr_printf        (gchar const *format, ...) G_GNUC_PRINTF (1, 2);


G_END_DECLS

#endif /* __HG_FILE_H__ */
