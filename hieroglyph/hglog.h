/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglog.h
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
#ifndef __HG_LOG_H__
#define __HG_LOG_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


typedef void (*HgLogFunc)(HgLogType     log_type,
			  const gchar  *domain,
			  const gchar  *subtype,
			  const gchar  *message,
			  gpointer      user_data);

#define hg_log_info(...)						\
	hg_log(HG_LOG_TYPE_INFO, HG_LOG_DOMAIN, NULL, __VA_ARGS__)
#ifdef DEBUG
#define hg_log_debug(_type_, ...)					\
	hg_log(HG_LOG_TYPE_DEBUG, HG_LOG_DOMAIN, #_type_, __VA_ARGS__)
#else
#define hg_log_debug(_type_, ...)
#endif /* DEBUG */
#define hg_log_warning(...)						\
	hg_log(HG_LOG_TYPE_WARNING, HG_LOG_DOMAIN, NULL, __VA_ARGS__)
#define hg_log_error(...)						\
	hg_log(HG_LOG_TYPE_ERROR, HG_LOG_DOMAIN, NULL, __VA_ARGS__)


gboolean hg_log_init               (void);
void     hg_log_finalize           (void);
void     hg_log_set_default_handler(HgLogFunc    func,
				    gpointer     user_data);
void     hg_log_set_flag           (const gchar *key,
				    gboolean     val);
gchar   *hg_log_get_log_type_header(HgLogType    log_type,
				    const gchar *domain) G_GNUC_MALLOC;
void     hg_log                    (HgLogType    log_type,
				    const gchar *domain,
				    const gchar *subtype,
				    const gchar *format,
				    ...);
void     hg_logv                   (HgLogType    log_type,
				    const gchar *domain,
				    const gchar *subtype,
				    const gchar *format,
				    va_list      va_args);


G_END_DECLS

#endif /* __HG_LOG_H__ */
