/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hglist.h
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
#ifndef __HG_LIST_H__
#define __HG_LIST_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


void hg_list_init    (void);
void hg_list_finalize(void);

HgList *hg_list_new           (HgMemPool *pool);
HgList *hg_list_append        (HgList    *list,
			       gpointer   data);
HgList *hg_list_append_object (HgList    *list,
			       HgObject  *hobject);
HgList *hg_list_prepend       (HgList    *list,
			       gpointer   data);
HgList *hg_list_prepend_object(HgList    *list,
			       HgObject  *hobject);
guint   hg_list_length        (HgList    *list);
HgList *hg_list_remove        (HgList    *list,
			       gpointer   data);
HgList *hg_list_first         (HgList    *list);
HgList *hg_list_last          (HgList    *list);

#define hg_list_iter_free	hg_mem_free

HgListIter hg_list_iter_new        (HgList        *list);
gboolean   hg_list_get_iter_first  (HgList        *list,
				    HgListIter     iter);
gboolean   hg_list_get_iter_next   (HgList        *list,
				    HgListIter     iter);
HgListIter hg_list_find_iter       (HgList        *list,
				    gconstpointer  data);
HgListIter hg_list_find_iter_custom(HgList        *list,
				    gconstpointer  data,
				    HgCompareFunc  func);
gpointer   hg_list_iter_get_data   (HgListIter     iter);
void       hg_list_iter_set_data   (HgListIter     iter,
				    gpointer       data);
void       hg_list_iter_set_object (HgListIter     iter,
				    HgObject      *hobject);
HgList    *hg_list_iter_delete_link(HgListIter     iter);


G_END_DECLS

#endif /* __HG_LIST_H__ */
