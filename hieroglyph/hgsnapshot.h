/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgsnapshot.h
 * Copyright (C) 2005-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGSNAPSHOT_H__
#define __HIEROGLYPH_HGSNAPSHOT_H__

#include <hieroglyph/hgobject.h>

G_BEGIN_DECLS

#define HG_SNAPSHOT_INIT					\
	(hg_object_register(HG_TYPE_SNAPSHOT,			\
			    hg_object_snapshot_get_vtable()))
#define HG_IS_QSNAPSHOT(_v_)				\
	(hg_quark_get_type(_v_) == HG_TYPE_SNAPSHOT)


typedef struct _hg_snapshot_t		hg_snapshot_t;

hg_object_vtable_t *hg_object_snapshot_get_vtable(void) G_GNUC_CONST;
hg_quark_t          hg_snapshot_new              (hg_mem_t      *mem,
						  gpointer      *ret);
gboolean            hg_snapshot_restore          (hg_snapshot_t *snapshot);


G_END_DECLS

#endif /* __HIEROGLYPH_HGSNAPSHOT_H__ */