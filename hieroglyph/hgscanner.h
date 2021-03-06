/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgscanner.h
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
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGSCANNER_H__
#define __HIEROGLYPH_HGSCANNER_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgfile.h>

HG_BEGIN_DECLS

typedef struct _hg_scanner_t		hg_scanner_t;

hg_scanner_t *hg_scanner_new        (hg_mem_t     *mem);
void          hg_scanner_destroy    (hg_scanner_t *scanner);
hg_bool_t     hg_scanner_attach_file(hg_scanner_t *scanner,
                                     hg_file_t    *file);
hg_bool_t     hg_scanner_scan       (hg_scanner_t *scanner,
                                     hg_vm_t      *vm);
hg_quark_t    hg_scanner_get_token  (hg_scanner_t *scanner);
hg_file_t    *hg_scanner_get_infile (hg_scanner_t *scanner);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGSCANNER_H__ */
