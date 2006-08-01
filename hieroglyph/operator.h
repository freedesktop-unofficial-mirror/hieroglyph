/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * operator.h
 * Copyright (C) 2005-2006 Akira TAGOH
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
#ifndef __HG_OPERATOR_H__
#define __HG_OPERATOR_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


typedef enum {
	HG_op_abs = 0,
	HG_op_add,
	HG_op_aload,
	HG_op_anchorsearch,
	HG_op_and,
	HG_op_arc,
	HG_op_arcn,
	HG_op_arct,
	HG_op_arcto,
	HG_op_array,

	HG_op_ashow,
	HG_op_astore,
	HG_op_awidthshow,
	HG_op_begin,
	HG_op_bind,
	HG_op_bitshift,
	HG_op_ceiling,
	HG_op_charpath,
	HG_op_clear,
	HG_op_cleartomark,

	HG_op_clip,
	HG_op_clippath,
	HG_op_closepath,
	HG_op_concat,
	HG_op_concatmatrix,
	HG_op_copy,
	HG_op_count,
	HG_op_counttomark,
	HG_op_currentcmykcolor,
	HG_op_currentdash,

	HG_op_currentdict,
	HG_op_currentfile,
	HG_op_currentfont,
	HG_op_currentgray,
	HG_op_currentgstate,
	HG_op_currenthsbcolor,
	HG_op_currentlinecap,
	HG_op_currentlinejoin,
	HG_op_currentlinewidth,
	HG_op_currentmatrix,

	HG_op_currentpoint,
	HG_op_currentrgbcolor,
	HG_op_currentshared,
	HG_op_curveto,
	HG_op_cvi,
	HG_op_cvlit,
	HG_op_cvn,
	HG_op_cvr,
	HG_op_cvrs,
	HG_op_cvs,

	HG_op_cvx,
	HG_op_def,
	HG_op_defineusername,
	HG_op_dict,
	HG_op_div,
	HG_op_dtransform,
	HG_op_dup,
	HG_op_end,
	HG_op_eoclip,
	HG_op_eofill,

	HG_op_eoviewclip,
	HG_op_eq,
	HG_op_exch,
	HG_op_exec,
	HG_op_exit,
	HG_op_file,
	HG_op_fill,
	HG_op_findfont,
	HG_op_flattenpath,
	HG_op_floor,

	HG_op_flush,
	HG_op_flushfile,
	HG_op_for,
	HG_op_forall,
	HG_op_ge,
	HG_op_get,
	HG_op_getinterval,
	HG_op_grestore,
	HG_op_gsave,
	HG_op_gstate,

	HG_op_gt,
	HG_op_identmatrix,
	HG_op_idiv,
	HG_op_idtransform,
	HG_op_if,
	HG_op_ifelse,
	HG_op_image,
	HG_op_imagemask,
	HG_op_index,
	HG_op_ineofill,

	HG_op_infill,
	HG_op_initviewclip,
	HG_op_inueofill,
	HG_op_inufill,
	HG_op_invertmatrix,
	HG_op_itransform,
	HG_op_known,
	HG_op_le,
	HG_op_length,
	HG_op_lineto,

	HG_op_load,
	HG_op_loop,
	HG_op_lt,
	HG_op_makefont,
	HG_op_matrix,
	HG_op_maxlength,
	HG_op_mod,
	HG_op_moveto,
	HG_op_mul,
	HG_op_ne,

	HG_op_neg,
	HG_op_newpath,
	HG_op_not,
	HG_op_null,
	HG_op_or,
	HG_op_pathbbox,
	HG_op_pathforall,
	HG_op_pop,
	HG_op_print,
	HG_op_printobject,

	HG_op_put,
	HG_op_putinterval,
	HG_op_rcurveto,
	HG_op_read,
	HG_op_readhexstring,
	HG_op_readline,
	HG_op_readstring,
	HG_op_rectclip,
	HG_op_rectfill,
	HG_op_rectstroke,

	HG_op_rectviewclip,
	HG_op_repeat,
	HG_op_restore,
	HG_op_rlineto,
	HG_op_rmoveto,
	HG_op_roll,
	HG_op_rotate,
	HG_op_round,
	HG_op_save,
	HG_op_scale,

	HG_op_scalefont,
	HG_op_search,
	HG_op_selectfont,
	HG_op_setbbox,
	HG_op_setcachedevice,
	HG_op_setcachedevice2,
	HG_op_setcharwidth,
	HG_op_setcmykcolor,
	HG_op_setdash,
	HG_op_setfont,

	HG_op_setgray,
	HG_op_setgstate,
	HG_op_sethsbcolor,
	HG_op_setlinecap,
	HG_op_setlinejoin,
	HG_op_setlinewidth,
	HG_op_setmatrix,
	HG_op_setrgbcolor,
	HG_op_setshared,
	HG_op_shareddict,

	HG_op_show,
	HG_op_showpage,
	HG_op_stop,
	HG_op_stopped,
	HG_op_store,
	HG_op_string,
	HG_op_stringwidth,
	HG_op_stroke,
	HG_op_strokepath,
	HG_op_sub,

	HG_op_systemdict,
	HG_op_token,
	HG_op_transform,
	HG_op_translate,
	HG_op_truncate,
	HG_op_type,
	HG_op_uappend,
	HG_op_ucache,
	HG_op_ueofill,
	HG_op_ufill,

	HG_op_undef,
	HG_op_upath,
	HG_op_userdict,
	HG_op_ustroke,
	HG_op_viewclip,
	HG_op_viewclippath,
	HG_op_where,
	HG_op_widthshow,
	HG_op_write,
	HG_op_writehexstring,

	HG_op_writeobject,
	HG_op_writestring,
	HG_op_wtranslation,
	HG_op_xor,
	HG_op_xshow,
	HG_op_xyshow,
	HG_op_yshow,
	HG_op_FontDirectory,
	HG_op_SharedFontDirectory,
	HG_op_Courier,

	HG_op_Courier_Bold,
	HG_op_Courier_BoldOblique,
	HG_op_Courier_Oblique,
	HG_op_Helvetica,
	HG_op_Helvetica_Bold,
	HG_op_Helvetica_BoldOblique,
	HG_op_Helvetica_Oblique,
	HG_op_Symbol,
	HG_op_Times_Bold,
	HG_op_Times_BoldItalic,

	HG_op_Times_Italic,
	HG_op_Times_Roman,
	HG_op_execuserobject,
	HG_op_currentcolor,
	HG_op_currentcolorspace,
	HG_op_currentglobal,
	HG_op_execform,
	HG_op_filter,
	HG_op_findresource,
	HG_op_globaldict,

	HG_op_makepattern,
	HG_op_setcolor,
	HG_op_setcolorspace,
	HG_op_setglobal,
	HG_op_setpagedevice,
	HG_op_setpattern,

	HG_op_sym_eq = 256,
	HG_op_sym_eqeq,
	HG_op_ISOLatin1Encoding,
	HG_op_StandardEncoding,

	HG_op_sym_left_square_bracket,
	HG_op_sym_right_square_bracket,
	HG_op_atan,
	HG_op_banddevice,
	HG_op_bytesavailable,
	HG_op_cachestatus,
	HG_op_closefile,
	HG_op_colorimage,
	HG_op_condition,
	HG_op_copypage,

	HG_op_cos,
	HG_op_countdictstack,
	HG_op_countexecstack,
	HG_op_cshow,
	HG_op_currentblackgeneration,
	HG_op_currentcacheparams,
	HG_op_currentcolorscreen,
	HG_op_currentcolortransfer,
	HG_op_currentcontext,
	HG_op_currentflat,

	HG_op_currenthalftone,
	HG_op_currenthalftonephase,
	HG_op_currentmiterlimit,
	HG_op_currentobjectformat,
	HG_op_currentpacking,
	HG_op_currentscreen,
	HG_op_currentstrokeadjust,
	HG_op_currenttransfer,
	HG_op_currentundercolorremoval,
	HG_op_defaultmatrix,

	HG_op_definefont,
	HG_op_deletefile,
	HG_op_detach,
	HG_op_deviceinfo,
	HG_op_dictstack,
	HG_op_echo,
	HG_op_erasepage,
	HG_op_errordict,
	HG_op_execstack,
	HG_op_executeonly,

	HG_op_exp,
	HG_op_false,
	HG_op_filenameforall,
	HG_op_fileposition,
	HG_op_fork,
	HG_op_framedevice,
	HG_op_grestoreall,
	HG_op_handleerror,
	HG_op_initclip,
	HG_op_initgraphics,

	HG_op_initmatrix,
	HG_op_instroke,
	HG_op_inustroke,
	HG_op_join,
	HG_op_kshow,
	HG_op_ln,
	HG_op_lock,
	HG_op_log,
	HG_op_mark,
	HG_op_monitor,

	HG_op_noaccess,
	HG_op_notify,
	HG_op_nulldevice,
	HG_op_packedarray,
	HG_op_quit,
	HG_op_rand,
	HG_op_rcheck,
	HG_op_readonly,
	HG_op_realtime,
	HG_op_renamefile,

	HG_op_renderbands,
	HG_op_resetfile,
	HG_op_reversepath,
	HG_op_rootfont,
	HG_op_rrand,
	HG_op_run,
	HG_op_scheck,
	HG_op_setblackgeneration,
	HG_op_setcachelimit,
	HG_op_setcacheparams,

	HG_op_setcolorscreen,
	HG_op_setcolortransfer,
	HG_op_setfileposition,
	HG_op_setflat,
	HG_op_sethalftone,
	HG_op_sethalftonephase,
	HG_op_setmiterlimit,
	HG_op_setobjectformat,
	HG_op_setpacking,
	HG_op_setscreen,

	HG_op_setstrokeadjust,
	HG_op_settransfer,
	HG_op_setucacheparams,
	HG_op_setundercolorremoval,
	HG_op_sin,
	HG_op_sqrt,
	HG_op_srand,
	HG_op_stack,
	HG_op_status,
	HG_op_statusdict,

	HG_op_true,
	HG_op_ucachestatus,
	HG_op_undefinefont,
	HG_op_usertime,
	HG_op_ustrokepath,
	HG_op_version,
	HG_op_vmreclaim,
	HG_op_vmstatus,
	HG_op_wait,
	HG_op_wcheck,

	HG_op_xcheck,
	HG_op_yield,
	HG_op_defineuserobject,
	HG_op_undefineuserobject,
	HG_op_UserObjects,
	HG_op_cleardictstack,
	HG_op_A,
	HG_op_B,
	HG_op_C,
	HG_op_D,
	HG_op_E,
	HG_op_F,
	HG_op_G,
	HG_op_H,
	HG_op_I,
	HG_op_J,
	HG_op_K,
	HG_op_L,
	HG_op_M,
	HG_op_N,

	HG_op_O,
	HG_op_P,
	HG_op_Q,
	HG_op_R,
	HG_op_S,
	HG_op_T,
	HG_op_U,
	HG_op_V,
	HG_op_W,
	HG_op_X,

	HG_op_Y,
	HG_op_Z,
	HG_op_a,
	HG_op_b,
	HG_op_c,
	HG_op_d,
	HG_op_e,
	HG_op_f,
	HG_op_g,
	HG_op_h,

	HG_op_i,
	HG_op_j,
	HG_op_k,
	HG_op_l,
	HG_op_m,
	HG_op_n,
	HG_op_o,
	HG_op_p,
	HG_op_q,
	HG_op_r,

	HG_op_s,
	HG_op_t,
	HG_op_u,
	HG_op_v,
	HG_op_w,
	HG_op_x,
	HG_op_y,
	HG_op_z,
	HG_op_setvmthreshold,
	HG_op_sym_begin_dict_mark,

	HG_op_sym_end_dict_mark,
	HG_op_currentcolorrendering,
	HG_op_currentdevparams,
	HG_op_currentoverprint,
	HG_op_currentpagedevice,
	HG_op_currentsystemparams,
	HG_op_currentuserparams,
	HG_op_defineresource,
	HG_op_findencoding,
	HG_op_gcheck,

	HG_op_glyphshow,
	HG_op_languagelevel,
	HG_op_product,
	HG_op_pstack,
	HG_op_resourceforall,
	HG_op_resourcestatus,
	HG_op_revision,
	HG_op_serialnumber,
	HG_op_setcolorrendering,
	HG_op_setdevparams,

	HG_op_setoverprint,
	HG_op_setsystemparams,
	HG_op_setuserparams,
	HG_op_startjob,
	HG_op_undefineresource,
	HG_op_GlobalFontDirectory,
	HG_op_ASCII85Decode,
	HG_op_ASCII85Encode,
	HG_op_ASCIIHexDecode,
	HG_op_ASCIIHexEncode,

	HG_op_CCITTFaxDecode,
	HG_op_CCITTFaxEncode,
	HG_op_DCTDecode,
	HG_op_DCTEncode,
	HG_op_LZWDecode,
	HG_op_LZWEncode,
	HG_op_NullEncode,
	HG_op_RunLengthDecode,
	HG_op_RunLengthEncode,
	HG_op_SubFileDecode,

	HG_op_CIEBasedA,
	HG_op_CIEBasedABC,
	HG_op_DeviceCMYK,
	HG_op_DeviceGray,
	HG_op_DeviceRGB,
	HG_op_Indexed,
	HG_op_Pattern,
	HG_op_Separation,
	HG_op_CIEBasedDEF,
	HG_op_CIEBasedDEFG,

	HG_op_DeviceN,

	HG_op_POSTSCRIPT_RESERVED_END,

	HG_op_END
} HgOperatorEncoding;


#define _hg_operator_build_operator(vm, pool, sdict, name, func, ret_op) \
	G_STMT_START {							\
		HgValueNode *__hg_key, *__hg_val;			\
									\
		(ret_op) = hg_operator_new((pool), #name, _hg_operator_op_##func); \
		if ((ret_op) == NULL) {					\
			g_warning("Failed to create an operator %s", #name); \
		} else {						\
			__hg_key = hg_vm_get_name_node((vm), #name);	\
			HG_VALUE_MAKE_POINTER (__hg_val, (ret_op));	\
			if (__hg_val == NULL) {				\
				hg_vm_set_error((vm), __hg_key, VM_e_VMerror, FALSE); \
			} else {					\
				hg_object_executable((HgObject *)__hg_val); \
				hg_dict_insert((pool), (sdict), __hg_key, __hg_val); \
			}						\
		}							\
	} G_STMT_END
#define BUILD_OP(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		_hg_operator_build_operator(vm, pool, sdict, name, func, __hg_op); \
		if (__hg_op != NULL) {					\
			__hg_operator_list[HG_op_##name] = __hg_op;	\
		}							\
	} G_STMT_END
#define BUILD_OP_(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		_hg_operator_build_operator(vm, pool, sdict, name, func, __hg_op); \
	} G_STMT_END
#define _hg_operator_set_error(v, o, e)					\
	G_STMT_START {							\
		HgValueNode *__lb_op_node;				\
									\
		HG_VALUE_MAKE_POINTER (__lb_op_node, (o));		\
		hg_vm_set_error((v), __lb_op_node, (e), TRUE);		\
	} G_STMT_END
#define _hg_operator_set_error_from_file(v, o, e)			\
	G_STMT_START {							\
		HgValueNode *__lb_op_node;				\
									\
		HG_VALUE_MAKE_POINTER (__lb_op_node, (o));		\
		hg_vm_set_error_from_file((v), __lb_op_node, (e), TRUE); \
	} G_STMT_END


extern HgOperator *__hg_operator_list[HG_op_END];

HgOperator       *hg_operator_new     (HgMemPool      *pool,
				       const gchar    *name,
				       HgOperatorFunc  func);
gboolean          hg_operator_init    (HgVM           *vm);
gboolean          hg_operator_invoke  (HgOperator     *op,
				       HgVM           *vm);
const gchar      *hg_operator_get_name(HgOperator     *op) G_GNUC_CONST;


G_END_DECLS

#endif /* __HG_OPERATOR_H__ */
