/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem.h
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
#ifndef __HIEROGLYPH_HGMEM_H__
#define __HIEROGLYPH_HGMEM_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgerror.h>

G_BEGIN_DECLS

#define HG_MEM_FLAGS_DEFAULT			HG_MEM_RESTORABLE
#define HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE	(HG_MEM_FLAGS_DEFAULT & ~HG_MEM_RESTORABLE)


hg_mem_t               *hg_mem_get                                 (gint                     id);
hg_mem_t               *hg_mem_new                                 (gsize                    size);
hg_mem_t               *hg_mem_new_with_allocator                  (hg_mem_vtable_t         *allocator,
                                                                    gsize                    size);
void                    hg_mem_destroy                             (gpointer                 data);
gboolean                hg_mem_expand_heap                         (hg_mem_t                *mem,
                                                                    gsize                    size);
void                    hg_mem_set_resizable                       (hg_mem_t                *mem,
                                                                    gboolean                 flag);
void                    hg_mem_add_gc_finalizer                    (hg_mem_t                *mem,
                                                                    hg_quark_t               index,
                                                                    hg_mem_finalizer_func_t  func);
void                    hg_mem_remove_gc_finalizer                 (hg_mem_t                *mem,
                                                                    hg_quark_t               index);
hg_quark_t              hg_mem_alloc                               (hg_mem_t                *mem,
                                                                    gsize                    size,
                                                                    gpointer                *ret);
hg_quark_t              hg_mem_alloc_with_flags                    (hg_mem_t                *mem,
								    gsize                    size,
								    guint                    flags,
								    gpointer                *ret);
hg_quark_t              hg_mem_realloc                             (hg_mem_t                *mem,
                                                                    hg_quark_t               qdata,
                                                                    gsize                    size,
                                                                    gpointer                *ret);
void                    hg_mem_free                                (hg_mem_t                *mem,
                                                                    hg_quark_t               qdata);
gpointer                hg_mem_lock_object                         (hg_mem_t                *mem,
                                                                    hg_quark_t               qdata);
void                    hg_mem_unlock_object                       (hg_mem_t                *mem,
                                                                    hg_quark_t               qdata);
void                    hg_mem_enable_garbage_collector            (hg_mem_t                *mem,
								    gboolean                 flag);
void                    hg_mem_set_garbage_collector               (hg_mem_t                *mem,
                                                                    hg_gc_func_t             func,
                                                                    gpointer                 user_data);
gssize                  hg_mem_collect_garbage                     (hg_mem_t                *mem);
gboolean                hg_mem_gc_mark                             (hg_mem_t                *mem,
                                                                    hg_quark_t               qdata,
                                                                    GError                  **error);
hg_mem_snapshot_data_t *hg_mem_save_snapshot                       (hg_mem_t                *mem);
gboolean                hg_mem_restore_snapshot                    (hg_mem_t                *mem,
                                                                    hg_mem_snapshot_data_t  *snapshot,
								    hg_gc_func_t             func,
								    gpointer                 data);
void                    hg_mem_restore_mark                        (hg_mem_t                *mem,
								    hg_quark_t               qdata);
void                    hg_mem_snapshot_free                       (hg_mem_t                *mem,
                                                                    hg_mem_snapshot_data_t  *snapshot);
gint                    hg_mem_get_id                              (hg_mem_t                *mem);
gsize                   hg_mem_get_total_size                      (hg_mem_t                *mem);
gsize                   hg_mem_get_used_size                       (hg_mem_t                *mem);
void                    hg_mem_reserved_spool_add                  (hg_mem_t                *mem,
                                                                    hg_quark_t               qdata);
void                    hg_mem_reserved_spool_remove               (hg_mem_t                *mem,
                                                                    hg_quark_t               qdata);
void                    hg_mem_reserved_spool_set_garbage_collector(hg_mem_t                *mem,
                                                                    hg_rs_gc_func_t          func,
                                                                    gpointer                 user_data);
gboolean                hg_mem_reserved_spool_foreach              (hg_mem_t                *mem,
                                                                    hg_rs_gc_func_t          func,
                                                                    gpointer                 user_data,
                                                                    GError                  **error);

G_INLINE_FUNC gpointer hg_mem_lock_object_with_gerror(hg_mem_t     *mem,
						      hg_quark_t    quark,
						      const gchar  *pretty_function,
						      GError      **error);

G_INLINE_FUNC gpointer
hg_mem_lock_object_with_gerror(hg_mem_t     *mem,
			       hg_quark_t    quark,
			       const gchar  *pretty_function,
			       GError      **error)
{
	gpointer retval;
	GError *err = NULL;

	hg_return_val_with_gerror_if_fail (mem != NULL, NULL, error, HG_VM_e_VMerror);

	retval = hg_mem_lock_object(mem, quark);
	if (retval == NULL) {
		g_set_error(&err, HG_ERROR, HG_VM_e_VMerror,
			    "%s: Invalid quark to obtain the actual object [mem: %p, quark: 0x%lx]",
			    pretty_function,
			    mem,
			    quark);
	}
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s (code: %d)",
				  err->message,
				  err->code);
		}
		g_error_free(err);
	}

	return retval;
}

#define HG_MEM_LOCK(_m_,_q_,_e_)					\
	hg_mem_lock_object_with_gerror((_m_),(_q_),__PRETTY_FUNCTION__,(_e_))
#define hg_return_val_with_gerror_if_lock_fail(_o_,_m_,_q_,_e_,_v_)	\
	_o_ = HG_MEM_LOCK ((_m_), (_q_), (_e_));			\
	if ((_o_) == NULL)						\
		return _v_;
#define hg_return_with_gerror_if_lock_fail(_o_,_m_,_q_,_e_)		\
	hg_return_val_with_gerror_if_lock_fail(_o_,_m_,_q_,_e_,)
#define hg_return_val_if_lock_fail(_o_,_m_,_q_,_v_)			\
	hg_return_val_with_gerror_if_lock_fail(_o_,_m_,_q_,NULL,_v_)
#define hg_return_if_lock_fail(_o_,_m_,_q_)				\
	hg_return_val_with_gerror_if_lock_fail(_o_,_m_,_q_,NULL,)


G_END_DECLS

#endif /* __HIEROGLYPH_HGMEM_H__ */
