/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgencoding.h
 * Copyright (C) 2010-2011 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGENCODING_H__
#define __HIEROGLYPH_HGENCODING_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

gboolean              hg_encoding_init                    (void);
void                  hg_encoding_tini                    (void);
const gchar          *hg_encoding_get_system_encoding_name(hg_system_encoding_t  encoding);
hg_system_encoding_t  hg_encoding_lookup_system_encoding  (const gchar          *name);

G_END_DECLS

#endif /* __HIEROGLYPH_HGENCODING_H__ */
