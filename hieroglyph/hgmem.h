/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem.h
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

#ifndef __HIEROGLYPH_HGMEM_H__
#define __HIEROGLYPH_HGMEM_H__

#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hgerror.h>
#include <hieroglyph/hgmessages.h>
#include <hieroglyph/hgallocator.h>

HG_BEGIN_DECLS

#define HG_MEM_FLAGS_DEFAULT			HG_MEM_FLAG_RESTORABLE
#define HG_MEM_FLAGS_DEFAULT_WITHOUT_RESTORABLE	(HG_MEM_FLAGS_DEFAULT & ~HG_MEM_FLAG_RESTORABLE)

typedef enum _hg_mem_type_t		hg_mem_type_t;
typedef hg_cb_BOOL__QUARK_QUARK_t	hg_rs_gc_func_t;
typedef hg_allocator_snapshot_data_t	hg_mem_snapshot_data_t;

typedef void (* hg_mem_finalizer_func_t) (hg_mem_t    *mem,
					  hg_quark_t  index);
typedef hg_bool_t (* hg_gc_func_t)	(hg_mem_t     *mem,
					 hg_pointer_t  user_data);

/* hgmem.h */
enum _hg_mem_type_t {
	HG_MEM_TYPE_MASTER = 0,
	HG_MEM_TYPE_GLOBAL,
	HG_MEM_TYPE_LOCAL,
	HG_MEM_TYPE_END
};

/* memory spool management */
hg_mem_t      *hg_mem_spool_get               (hg_int_t             id);
hg_mem_t      *hg_mem_spool_new               (hg_mem_type_t        type,
                                               hg_usize_t           size);
hg_mem_t      *hg_mem_spool_new_with_allocator(hg_mem_vtable_t     *allocator,
                                               hg_mem_type_t        type,
                                               hg_usize_t           size);
void           hg_mem_spool_destroy           (hg_pointer_t         data);
hg_bool_t      hg_mem_spool_expand_heap       (hg_mem_t            *mem,
                                               hg_usize_t           size);
void           hg_mem_spool_set_resizable     (hg_mem_t            *mem,
                                               hg_bool_t            flag);
hg_int_t       hg_mem_spool_add_gc_marker     (hg_mem_t            *mem,
                                               hg_gc_mark_func_t    func);
void           hg_mem_spool_remove_gc_marker  (hg_mem_t            *mem,
                                               hg_int_t             marker_id);
hg_int_t       hg_mem_spool_add_finalizer     (hg_mem_t            *mem,
                                               hg_finalizer_func_t  func);
void           hg_mem_spool_remove_finalizer  (hg_mem_t            *mem,
                                               hg_int_t             finalizer_id);
hg_mem_type_t  hg_mem_spool_get_type          (hg_mem_t            *mem);
void           hg_mem_spool_enable_gc         (hg_mem_t            *mem,
                                               hg_bool_t            flag);
void           hg_mem_spool_set_gc_procedure  (hg_mem_t            *mem,
                                               hg_gc_func_t         func,
                                               hg_pointer_t         user_data);
hg_size_t      hg_mem_spool_run_gc            (hg_mem_t            *mem);
hg_usize_t     hg_mem_spool_get_total_size    (hg_mem_t            *mem);
hg_usize_t     hg_mem_spool_get_used_size     (hg_mem_t            *mem);

/* memory management */
hg_quark_t   hg_mem_alloc           (hg_mem_t                 *mem,
                                     hg_usize_t                size,
                                     hg_pointer_t             *ret);
hg_quark_t   hg_mem_alloc_with_flags(hg_mem_t                 *mem,
                                     hg_usize_t                size,
                                     hg_uint_t                 flags,
                                     hg_pointer_t             *ret);
hg_quark_t   hg_mem_realloc         (hg_mem_t                 *mem,
                                     hg_quark_t                qdata,
                                     hg_usize_t                size,
                                     hg_pointer_t             *ret);
void         hg_mem_free            (hg_mem_t                 *mem,
                                     hg_quark_t                qdata);
void         hg_mem_set_gc_marker   (hg_mem_t                 *mem,
                                     hg_quark_t                qdata,
                                     hg_int_t                  marker_id);
void         hg_mem_set_finalizer   (hg_mem_t                 *mem,
                                     hg_quark_t                qdata,
                                     hg_int_t                  finalizer_id);
hg_pointer_t hg_mem_lock_object     (hg_mem_t                 *mem,
                                     hg_quark_t                qdata);
void         hg_mem_unlock_object   (hg_mem_t                 *mem,
                                     hg_quark_t                qdata);
hg_bool_t    hg_mem_gc_mark         (hg_mem_t                 *mem,
                                     hg_quark_t                qdata);
hg_int_t     hg_mem_get_id          (hg_mem_t                 *mem);
void         hg_mem_ref             (hg_mem_t                 *mem,
                                     hg_quark_t                qdata);
void         hg_mem_unref           (hg_mem_t                 *mem,
                                     hg_quark_t                qdata);
hg_bool_t    hg_mem_foreach         (hg_mem_t                 *mem,
                                     hg_block_iter_flags_t     flags,
                                     hg_quark_iterator_func_t  func,
                                     hg_pointer_t              user_data);


HG_INLINE_FUNC hg_pointer_t _hg_mem_lock_object(hg_mem_t        *mem,
						hg_quark_t       quark,
						const hg_char_t *pretty_function);

HG_INLINE_FUNC hg_pointer_t
_hg_mem_lock_object(hg_mem_t        *mem,
		    hg_quark_t       quark,
		    const hg_char_t *pretty_function)
{
	hg_pointer_t retval;

	hg_return_val_if_fail (mem != NULL, NULL, HG_e_VMerror);

	retval = hg_mem_lock_object(mem, quark);
	if (retval == NULL) {
		hg_debug(HG_MSGCAT_MEM, "%s: Invalid quark to obtain the actual object [mem: %p, quark: 0x%lx]",
			 pretty_function, mem, quark);
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
	}

	return retval;
}

#define HG_MEM_LOCK(_m_,_q_)					\
	_hg_mem_lock_object((_m_),(_q_),__PRETTY_FUNCTION__)
#define hg_return_val_if_lock_fail(_o_,_m_,_q_,_v_)	\
	_o_ = HG_MEM_LOCK ((_m_), (_q_));		\
	if ((_o_) == NULL)				\
		return _v_;
#define hg_return_if_lock_fail(_o_,_m_,_q_)		\
	hg_return_val_if_lock_fail(_o_,_m_,_q_,)


HG_END_DECLS

#endif /* __HIEROGLYPH_HGMEM_H__ */
