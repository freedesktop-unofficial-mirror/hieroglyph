/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgoperator-private.h
 * Copyright (C) 2007 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGOPERATOR_PRIVATE_H__
#define __HIEROGLYPH_HGOPERATOR_PRIVATE_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


#define HG_DEFUNC_BEGIN_OPER(name)					\
	static gboolean							\
	_hg_object_operator_ ## name ## _cb(hg_vm_t     *vm,		\
					    hg_object_t *object)	\
	{								\
		gboolean retval = FALSE;
#define HG_DEFUNC_END_OPER				\
		return retval;				\
	}
#define HG_DEFUNC_UNIMPLEMENTED_OPER(name)				\
	static gboolean							\
	_hg_object_operator_ ## name ## _cb(hg_vm_t     *vm,		\
					    hg_object_t *object)	\
	{								\
		g_warning("%s isn't yet implemented.", __PRETTY_FUNCTION__); \
									\
		return FALSE;						\
	}
#define _hg_object_operator_build(_hg_v_,_hg_i_,_hg_n_)			\
	G_STMT_START {							\
		hg_object_t *_hg_o_, *_hg_d_, *_hg_on_;			\
									\
		if (__hg_system_encoding_names[HG_enc_ ## _hg_i_] != NULL) \
			g_free(__hg_system_encoding_names[HG_enc_ ## _hg_i_]); \
		__hg_system_encoding_names[HG_enc_ ## _hg_i_] = g_strdup(# _hg_n_); \
		_hg_o_ = hg_object_operator_new((_hg_v_), HG_enc_ ## _hg_i_, _hg_object_operator_ ## _hg_i_ ## _cb); \
		if (_hg_o_) {						\
			_hg_d_ = hg_vm_get_currentdict(_hg_v_);		\
			_hg_on_ = hg_vm_name_lookup((_hg_v_), # _hg_n_); \
			if (!hg_object_dict_insert(_hg_d_, _hg_on_, _hg_o_)) { \
				retval = FALSE;				\
			}						\
		} else {						\
			retval = FALSE;					\
		}							\
	} G_STMT_END
#define _hg_object_operator_priv_build(_hg_v_,_hg_n_, _hg_fn_)		\
	G_STMT_START {							\
		hg_object_t *_hg_o_, *_hg_d_, *_hg_on_;			\
									\
		_hg_o_ = hg_object_operator_new_with_custom((_hg_v_), # _hg_n_, _hg_object_operator_ ## _hg_fn_ ## _cb); \
		if (_hg_o_) {						\
			_hg_d_ = hg_vm_get_currentdict(_hg_v_);		\
			_hg_on_ = hg_vm_name_lookup((_hg_v_), # _hg_n_); \
			if (!hg_object_dict_insert(_hg_d_, _hg_on_, _hg_o_)) { \
				retval = FALSE;				\
			}						\
		} else {						\
			retval = FALSE;					\
		}							\
	} G_STMT_END
#define _hg_object_operator_unbuild(_hg_v_,_hg_n_)			\
	G_STMT_START {							\
		if (!hg_vm_dict_remove((_hg_v_), # _hg_n_, TRUE)) {	\
			retval = FALSE;					\
		}							\
	} G_STMT_END

G_END_DECLS

#endif /* __HIEROGLYPH_HGOPERATOR_PRIVATE_H__ */
