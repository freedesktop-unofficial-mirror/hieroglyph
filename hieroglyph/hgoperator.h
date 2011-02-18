/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgoperator.h
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

#ifndef __HIEROGLYPH_HGOPERATOR_H__
#define __HIEROGLYPH_HGOPERATOR_H__

#include <hieroglyph/hgerror.h>
#include <hieroglyph/hgquark.h>
#include <hieroglyph/hgencoding.h>
#include <hieroglyph/hgmessages.h>
#include <hieroglyph/hgname.h>
#include <hieroglyph/hgvm.h>

HG_BEGIN_DECLS

#define HG_QOPER(_v_)				\
	(hg_operator_new(_v_))
#define HG_OPER(_q_)						\
	((hg_system_encoding_t)hg_quark_get_value((_q_)))
#define HG_IS_QOPER(_v_)				\
	(hg_quark_get_type((_v_)) == HG_TYPE_OPER)

typedef hg_bool_t (* hg_operator_func_t) (hg_vm_t  *vm);

#ifdef HG_DEBUG
#define hg_operator_assert(_vm_,_le_,_re_)				\
	HG_STMT_START {							\
		if ((_le_) != (_re_)) {					\
			g_printerr("%s: assertion failed: %s(%ld) == %s(%ld)\n", \
				   __PRETTY_FUNCTION__, #_le_, (_le_), #_re_, (_re_)); \
			hg_operator_invoke(HG_QOPER (HG_enc_private_abort), \
					   (_vm_));			\
		}							\
	} HG_STMT_END

#define INIT_STACK_VALIDATOR						\
	hg_usize_t __hg_stack_odepth HG_GNUC_UNUSED = hg_stack_depth(ostack); \
	hg_usize_t __hg_stack_edepth HG_GNUC_UNUSED = hg_stack_depth(estack); \
	hg_usize_t __hg_stack_ddepth HG_GNUC_UNUSED = hg_stack_depth(dstack); \
	hg_size_t __hg_stack_expected_odepth HG_GNUC_UNUSED = 0;		\
	hg_size_t __hg_stack_expected_edepth HG_GNUC_UNUSED = 0;		\
	hg_size_t __hg_stack_expected_ddepth HG_GNUC_UNUSED = 0
#define SET_EXPECTED_OSTACK_SIZE(_o_)		\
	__hg_stack_expected_odepth = (_o_)
#define SET_EXPECTED_ESTACK_SIZE(_e_)		\
	__hg_stack_expected_edepth = (_e_)
#define SET_EXPECTED_DSTACK_SIZE(_d_)		\
	__hg_stack_expected_ddepth = (_d_)
#define SET_EXPECTED_STACK_SIZE(_o_, _e_, _d_)				\
	HG_STMT_START {							\
		SET_EXPECTED_OSTACK_SIZE (_o_);				\
		SET_EXPECTED_ESTACK_SIZE (_e_);				\
		SET_EXPECTED_DSTACK_SIZE (_d_);				\
	} HG_STMT_END
#define NO_STACK_VALIDATION						\
	return retval
#define VALIDATE_STACK_SIZE						\
	if (retval) {							\
		hg_usize_t __hg_stack_after_odepth = hg_stack_depth(ostack); \
		hg_usize_t __hg_stack_after_edepth = hg_stack_depth(estack); \
		hg_usize_t __hg_stack_after_ddepth = hg_stack_depth(dstack); \
									\
		hg_operator_assert(vm, (__hg_stack_after_odepth - __hg_stack_odepth), __hg_stack_expected_odepth); \
		hg_operator_assert(vm, (__hg_stack_after_edepth - __hg_stack_edepth), __hg_stack_expected_edepth); \
		hg_operator_assert(vm, (__hg_stack_after_ddepth - __hg_stack_ddepth), __hg_stack_expected_ddepth); \
	}
#else
#define hg_operator_assert(_vm_,_e_)
#define INIT_STACK_VALIDATOR
#define SET_EXPECTED_OSTACK_SIZE(_o_)
#define SET_EXPECTED_ESTACK_SIZE(_e_)
#define SET_EXPECTED_DSTACK_SIZE(_d_)
#define SET_EXPECTED_STACK_SIZE(_o_, _e_, _d_)
#define NO_STACK_VALIDATION
#define VALIDATE_STACK_SIZE
#endif
#ifndef PLUGIN
#define OPER_FUNC_NAME(_n_)			\
	_hg_operator_real_ ## _n_
#else
#define OPER_FUNC_NAME(_n_)						\
	_plugin_real_ ## _n_
#define REG_ENC(_k_,_f_)					\
	(hg_operator_add_dynamic(#_k_, OPER_FUNC_NAME (_f_)))
#define REG_OPER(_d_,_k_)						\
	HG_STMT_START {							\
		hg_quark_t __o_name__ = (_k_);				\
		hg_quark_t __op__ = HG_QOPER (__o_name__);		\
									\
		if (!hg_dict_add((_d_),					\
				 __o_name__,				\
				 __op__,				\
				 FALSE))				\
			return FALSE;					\
	} HG_STMT_END
#define REG_VALUE(_d_,_k_,_v_)					\
	HG_STMT_START {						\
		hg_quark_t __o_name__ = HG_QNAME (#_k_);	\
		hg_quark_t __v__ = (_v_);			\
								\
		hg_quark_set_readable(&__v__, TRUE);		\
		if (!hg_dict_add((_d_),				\
				 __o_name__,			\
				 __v__,				\
				 FALSE))			\
			return FALSE;				\
	} HG_STMT_END
#define PROTO_PLUGIN(_n_)						\
	static hg_bool_t _ ## _n_ ## _init    (void);			\
	static hg_bool_t _ ## _n_ ## _finalize(void);			\
	static hg_bool_t _ ## _n_ ## _load    (hg_plugin_t   *plugin,	\
					       hg_pointer_t   vm_);	\
	static hg_bool_t _ ## _n_ ## _unload  (hg_plugin_t   *plugin,	\
					       hg_pointer_t   vm_);	\
	hg_plugin_t     *plugin_new        (hg_mem_t     *mem);		\
									\
	static hg_plugin_vtable_t __ ## _n_ ## _plugin_vtable = {	\
		.init     = _ ## _n_ ## _init,				\
		.finalize = _ ## _n_ ## _finalize,			\
		.load     = _ ## _n_ ## _load,				\
		.unload   = _ ## _n_ ## _unload,			\
	};								\
	static hg_plugin_info_t __ ## _n_ ## _plugin_info = {		\
		.version = HG_PLUGIN_VERSION,				\
		.vtable  = &__ ## _n_ ## _plugin_vtable,		\
	}
#endif

#define PROTO_OPER(_n_)						\
	static hg_bool_t OPER_FUNC_NAME (_n_) (hg_vm_t  *vm);
#define DEFUNC_OPER(_n_)						\
	static hg_bool_t						\
	OPER_FUNC_NAME (_n_) (hg_vm_t *vm)				\
	{								\
		hg_stack_t *ostack HG_GNUC_UNUSED = vm->stacks[HG_VM_STACK_OSTACK]; \
		hg_stack_t *estack HG_GNUC_UNUSED = vm->stacks[HG_VM_STACK_ESTACK]; \
		hg_stack_t *dstack HG_GNUC_UNUSED = vm->stacks[HG_VM_STACK_DSTACK]; \
		hg_quark_t qself HG_GNUC_UNUSED = hg_stack_index(estack, 0); \
		hg_bool_t retval = FALSE;				\
		INIT_STACK_VALIDATOR;					\
		HG_STMT_START
#define DEFUNC_UNIMPLEMENTED_OPER(_n_)					\
	static hg_bool_t						\
	OPER_FUNC_NAME (_n_) (hg_vm_t *vm)				\
	{								\
		hg_warning("%s isn't yet implemented.", #_n_);		\
		hg_vm_set_error(vm,					\
				hg_stack_index(vm->stacks[HG_VM_STACK_ESTACK], 0), \
				HG_e_VMerror);			\
		return FALSE;						\
	}
#define DEFUNC_OPER_END					\
		HG_STMT_END;				\
		VALIDATE_STACK_SIZE;			\
							\
		return retval;				\
	}

#define CHECK_STACK(_s_,_n_)						\
	HG_STMT_START {							\
		if (hg_stack_depth((_s_)) < (_n_)) {			\
			hg_vm_set_error(vm, qself, HG_e_stackunderflow); \
			return FALSE;					\
		}							\
	} HG_STMT_END
#define STACK_PUSH(_s_,_q_)						\
	HG_STMT_START {							\
		if (!hg_stack_push((_s_), (_q_))) {			\
			if ((_s_) == ostack) {				\
				hg_vm_set_error(vm, qself, HG_e_stackoverflow); \
			} else if ((_s_) == estack) {			\
				hg_vm_set_error(vm, qself, HG_e_execstackoverflow); \
			} else if ((_s_) == dstack) {			\
				hg_vm_set_error(vm, qself, HG_e_dictstackoverflow); \
			} else {					\
				hg_vm_set_error(vm, qself, HG_e_limitcheck);	\
			}						\
			return FALSE;					\
		}							\
	} HG_STMT_END


HG_INLINE_FUNC hg_quark_t hg_operator_new(guint encoding);

/**
 * hg_operator_new:
 * @encoding:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_quark_t
hg_operator_new(guint encoding)
{
	hg_quark_t retval = hg_quark_new(HG_TYPE_OPER, encoding);

	hg_quark_set_executable(&retval, TRUE);

	return retval;
}

hg_bool_t        hg_operator_init          (void);
void             hg_operator_tini          (void);
hg_quark_t       hg_operator_add_dynamic   (const hg_char_t     *string,
					    hg_operator_func_t   func);
void             hg_operator_remove_dynamic(hg_uint_t            encoding);
hg_bool_t        hg_operator_invoke        (hg_quark_t           qoper,
					    hg_vm_t             *vm);
const hg_char_t *hg_operator_get_name      (hg_quark_t           qoper);
hg_bool_t        hg_operator_register      (hg_vm_t             *vm,
					    hg_dict_t           *dict,
					    hg_vm_langlevel_t    lang_level);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGOPERATOR_H__ */
