/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * lbtypes.h
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
#ifndef __LIBRETTO_TYPES_H__
#define __LIBRETTO_TYPES_H__

#include <hieroglyph/hgtypes.h>


typedef struct _LibrettoGraphics		LibrettoGraphics;
typedef struct _LibrettoGraphicState		LibrettoGraphicState;
typedef struct _LibrettoOperator		LibrettoOperator;
typedef struct _LibrettoStack			LibrettoStack;
typedef struct _LibrettoVirtualMachine		LibrettoVM;

typedef gboolean (* LibrettoOperatorFunc)	(LibrettoOperator *op,
						 gpointer          vm);


typedef enum {
	LB_EMULATION_LEVEL_1 = 1,
	LB_EMULATION_LEVEL_2,
	LB_EMULATION_LEVEL_3
} LibrettoEmulationType;

typedef enum {
	LB_IO_STDIN = 1,
	LB_IO_STDOUT,
	LB_IO_STDERR,
} LibrettoIOType;

typedef enum {
	LB_e_dictfull = 1,
	LB_e_dictstackoverflow,
	LB_e_dictstackunderflow,
	LB_e_execstackoverflow,
	LB_e_handleerror,
	LB_e_interrupt,
	LB_e_invalidaccess,
	LB_e_invalidexit,
	LB_e_invalidfileaccess,
	LB_e_invalidfont,
	LB_e_invalidrestore,
	LB_e_ioerror,
	LB_e_limitcheck,
	LB_e_nocurrentpoint,
	LB_e_rangecheck,
	LB_e_stackoverflow,
	LB_e_stackunderflow,
	LB_e_syntaxerror,
	LB_e_timeout,
	LB_e_typecheck,
	LB_e_undefined,
	LB_e_undefinedfilename,
	LB_e_undefinedresult,
	LB_e_unmatchedmark,
	LB_e_unregistered,
	LB_e_VMerror,
	LB_e_configurationerror,
	LB_e_undefinedresource,
	LB_e_END,
} LibrettoVMError;

typedef enum {
	LB_op_abs = 0,
	LB_op_add,
	LB_op_aload,
	LB_op_anchorsearch,
	LB_op_and,
	LB_op_arc,
	LB_op_arcn,
	LB_op_arct,
	LB_op_arcto,
	LB_op_array,

	LB_op_ashow,
	LB_op_astore,
	LB_op_awidthshow,
	LB_op_begin,
	LB_op_bind,
	LB_op_bitshift,
	LB_op_ceiling,
	LB_op_charpath,
	LB_op_clear,
	LB_op_cleartomark,

	LB_op_clip,
	LB_op_clippath,
	LB_op_closepath,
	LB_op_concat,
	LB_op_concatmatrix,
	LB_op_copy,
	LB_op_count,
	LB_op_counttomark,
	LB_op_currentcmykcolor,
	LB_op_currentdash,

	LB_op_currentdict,
	LB_op_currentfile,
	LB_op_currentfont,
	LB_op_currentgray,
	LB_op_currentgstate,
	LB_op_currenthsbcolor,
	LB_op_currentlinecap,
	LB_op_currentlinejoin,
	LB_op_currentlinewidth,
	LB_op_currentmatrix,

	LB_op_currentpoint,
	LB_op_currentrgbcolor,
	LB_op_currentshared,
	LB_op_curveto,
	LB_op_cvi,
	LB_op_cvlit,
	LB_op_cvn,
	LB_op_cvr,
	LB_op_cvrs,
	LB_op_cvs,

	LB_op_cvx,
	LB_op_def,
	LB_op_defineusername,
	LB_op_dict,
	LB_op_div,
	LB_op_dtransform,
	LB_op_dup,
	LB_op_end,
	LB_op_eoclip,
	LB_op_eofill,

	LB_op_eoviewclip,
	LB_op_eq,
	LB_op_exch,
	LB_op_exec,
	LB_op_exit,
	LB_op_file,
	LB_op_fill,
	LB_op_findfont,
	LB_op_flattenpath,
	LB_op_floor,

	LB_op_flush,
	LB_op_flushfile,
	LB_op_for,
	LB_op_forall,
	LB_op_ge,
	LB_op_get,
	LB_op_getinterval,
	LB_op_grestore,
	LB_op_gsave,
	LB_op_gstate,

	LB_op_gt,
	LB_op_identmatrix,
	LB_op_idiv,
	LB_op_idtransform,
	LB_op_if,
	LB_op_ifelse,
	LB_op_image,
	LB_op_imagemask,
	LB_op_index,
	LB_op_ineofill,

	LB_op_infill,
	LB_op_initviewclip,
	LB_op_inueofill,
	LB_op_inufill,
	LB_op_invertmatrix,
	LB_op_itransform,
	LB_op_known,
	LB_op_le,
	LB_op_length,
	LB_op_lineto,

	LB_op_load,
	LB_op_loop,
	LB_op_lt,
	LB_op_makefont,
	LB_op_matrix,
	LB_op_maxlength,
	LB_op_mod,
	LB_op_moveto,
	LB_op_mul,
	LB_op_ne,

	LB_op_neg,
	LB_op_newpath,
	LB_op_not,
	LB_op_null,
	LB_op_or,
	LB_op_pathbbox,
	LB_op_pathforall,
	LB_op_pop,
	LB_op_print,
	LB_op_printobject,

	LB_op_put,
	LB_op_putinterval,
	LB_op_rcurveto,
	LB_op_read,
	LB_op_readhexstring,
	LB_op_readline,
	LB_op_readstring,
	LB_op_rectclip,
	LB_op_rectfill,
	LB_op_rectstroke,

	LB_op_rectviewclip,
	LB_op_repeat,
	LB_op_restore,
	LB_op_rlineto,
	LB_op_rmoveto,
	LB_op_roll,
	LB_op_rotate,
	LB_op_round,
	LB_op_save,
	LB_op_scale,

	LB_op_scalefont,
	LB_op_search,
	LB_op_selectfont,
	LB_op_setbbox,
	LB_op_setcachedevice,
	LB_op_setcachedevice2,
	LB_op_setcharwidth,
	LB_op_setcmykcolor,
	LB_op_setdash,
	LB_op_setfont,

	LB_op_setgray,
	LB_op_setgstate,
	LB_op_sethsbcolor,
	LB_op_setlinecap,
	LB_op_setlinejoin,
	LB_op_setlinewidth,
	LB_op_setmatrix,
	LB_op_setrgbcolor,
	LB_op_setshared,
	LB_op_shareddict,

	LB_op_show,
	LB_op_showpage,
	LB_op_stop,
	LB_op_stopped,
	LB_op_store,
	LB_op_string,
	LB_op_stringwidth,
	LB_op_stroke,
	LB_op_strokepath,
	LB_op_sub,

	LB_op_systemdict,
	LB_op_token,
	LB_op_transform,
	LB_op_translate,
	LB_op_truncate,
	LB_op_type,
	LB_op_uappend,
	LB_op_ucache,
	LB_op_ueofill,
	LB_op_ufill,

	LB_op_undef,
	LB_op_upath,
	LB_op_userdict,
	LB_op_ustroke,
	LB_op_viewclip,
	LB_op_viewclippath,
	LB_op_where,
	LB_op_widthshow,
	LB_op_write,
	LB_op_writehexstring,

	LB_op_writeobject,
	LB_op_writestring,
	LB_op_wtranslation,
	LB_op_xor,
	LB_op_xshow,
	LB_op_xyshow,
	LB_op_yshow,
	LB_op_FontDirectory,
	LB_op_SharedFontDirectory,
	LB_op_Courier,

	LB_op_Courier_Bold,
	LB_op_Courier_BoldOblique,
	LB_op_Courier_Oblique,
	LB_op_Helvetica,
	LB_op_Helvetica_Bold,
	LB_op_Helvetica_BoldOblique,
	LB_op_Helvetica_Oblique,
	LB_op_Symbol,
	LB_op_Times_Bold,
	LB_op_Times_BoldItalic,

	LB_op_Times_Italic,
	LB_op_Times_Roman,
	LB_op_execuserobject,
	LB_op_currentcolor,
	LB_op_currentcolorspace,
	LB_op_currentglobal,
	LB_op_execform,
	LB_op_filter,
	LB_op_findresource,
	LB_op_globaldict,

	LB_op_makepattern,
	LB_op_setcolor,
	LB_op_setcolorspace,
	LB_op_setglobal,
	LB_op_setpagedevice,
	LB_op_setpattern,

	LB_op_sym_eq = 256,
	LB_op_sym_eqeq,
	LB_op_ISOLatin1Encoding,
	LB_op_StandardEncoding,

	LB_op_sym_left_square_bracket,
	LB_op_sym_right_square_bracket,
	LB_op_atan,
	LB_op_banddevice,
	LB_op_bytesavailable,
	LB_op_cachestatus,
	LB_op_closefile,
	LB_op_colorimage,
	LB_op_condition,
	LB_op_copypage,

	LB_op_cos,
	LB_op_countdictstack,
	LB_op_countexecstack,
	LB_op_cshow,
	LB_op_currentblackgeneration,
	LB_op_currentcacheparams,
	LB_op_currentcolorscreen,
	LB_op_currentcolortransfer,
	LB_op_currentcontext,
	LB_op_currentflat,

	LB_op_currenthalftone,
	LB_op_currenthalftonephase,
	LB_op_currentmiterlimit,
	LB_op_currentobjectformat,
	LB_op_currentpacking,
	LB_op_currentscreen,
	LB_op_currentstrokeadjust,
	LB_op_currenttransfer,
	LB_op_currentundercolorremoval,
	LB_op_defaultmatrix,

	LB_op_definefont,
	LB_op_deletefile,
	LB_op_detach,
	LB_op_deviceinfo,
	LB_op_dictstack,
	LB_op_echo,
	LB_op_erasepage,
	LB_op_errordict,
	LB_op_execstack,
	LB_op_executeonly,

	LB_op_exp,
	LB_op_false,
	LB_op_filenameforall,
	LB_op_fileposition,
	LB_op_fork,
	LB_op_framedevice,
	LB_op_grestoreall,
	LB_op_handleerror,
	LB_op_initclip,
	LB_op_initgraphics,

	LB_op_initmatrix,
	LB_op_instroke,
	LB_op_inustroke,
	LB_op_join,
	LB_op_kshow,
	LB_op_ln,
	LB_op_lock,
	LB_op_log,
	LB_op_mark,
	LB_op_monitor,

	LB_op_noaccess,
	LB_op_notify,
	LB_op_nulldevice,
	LB_op_packedarray,
	LB_op_quit,
	LB_op_rand,
	LB_op_rcheck,
	LB_op_readonly,
	LB_op_realtime,
	LB_op_renamefile,

	LB_op_renderbands,
	LB_op_resetfile,
	LB_op_reversepath,
	LB_op_rootfont,
	LB_op_rrand,
	LB_op_run,
	LB_op_scheck,
	LB_op_setblackgeneration,
	LB_op_setcachelimit,
	LB_op_setcacheparams,

	LB_op_setcolorscreen,
	LB_op_setcolortransfer,
	LB_op_setfileposition,
	LB_op_setflat,
	LB_op_sethalftone,
	LB_op_sethalftonephase,
	LB_op_setmiterlimit,
	LB_op_setobjectformat,
	LB_op_setpacking,
	LB_op_setscreen,

	LB_op_setstrokeadjust,
	LB_op_settransfer,
	LB_op_setucacheparams,
	LB_op_setundercolorremoval,
	LB_op_sin,
	LB_op_sqrt,
	LB_op_srand,
	LB_op_stack,
	LB_op_status,
	LB_op_statusdict,

	LB_op_true,
	LB_op_ucachestatus,
	LB_op_undefinefont,
	LB_op_usertime,
	LB_op_ustrokepath,
	LB_op_version,
	LB_op_vmreclaim,
	LB_op_vmstatus,
	LB_op_wait,
	LB_op_wcheck,

	LB_op_xcheck,
	LB_op_yield,
	LB_op_defineuserobject,
	LB_op_undefineuserobject,
	LB_op_UserObjects,
	LB_op_cleardictstack,
	LB_op_A,
	LB_op_B,
	LB_op_C,
	LB_op_D,
	LB_op_E,
	LB_op_F,
	LB_op_G,
	LB_op_H,
	LB_op_I,
	LB_op_J,
	LB_op_K,
	LB_op_L,
	LB_op_M,
	LB_op_N,

	LB_op_O,
	LB_op_P,
	LB_op_Q,
	LB_op_R,
	LB_op_S,
	LB_op_T,
	LB_op_U,
	LB_op_V,
	LB_op_W,
	LB_op_X,

	LB_op_Y,
	LB_op_Z,
	LB_op_a,
	LB_op_b,
	LB_op_c,
	LB_op_d,
	LB_op_e,
	LB_op_f,
	LB_op_g,
	LB_op_h,

	LB_op_i,
	LB_op_j,
	LB_op_k,
	LB_op_l,
	LB_op_m,
	LB_op_n,
	LB_op_o,
	LB_op_p,
	LB_op_q,
	LB_op_r,

	LB_op_s,
	LB_op_t,
	LB_op_u,
	LB_op_v,
	LB_op_w,
	LB_op_x,
	LB_op_y,
	LB_op_z,
	LB_op_setvmthreshold,
	LB_op_sym_begin_dict_mark,

	LB_op_sym_end_dict_mark,
	LB_op_currentcolorrendering,
	LB_op_currentdevparams,
	LB_op_currentoverprint,
	LB_op_currentpagedevice,
	LB_op_currentsystemparams,
	LB_op_currentuserparams,
	LB_op_defineresource,
	LB_op_findencoding,
	LB_op_gcheck,

	LB_op_glyphshow,
	LB_op_languagelevel,
	LB_op_product,
	LB_op_pstack,
	LB_op_resourceforall,
	LB_op_resourcestatus,
	LB_op_revision,
	LB_op_serialnumber,
	LB_op_setcolorrendering,
	LB_op_setdevparams,

	LB_op_setoverprint,
	LB_op_setsystemparams,
	LB_op_setuserparams,
	LB_op_startjob,
	LB_op_undefineresource,
	LB_op_GlobalFontDirectory,
	LB_op_ASCII85Decode,
	LB_op_ASCII85Encode,
	LB_op_ASCIIHexDecode,
	LB_op_ASCIIHexEncode,

	LB_op_CCITTFaxDecode,
	LB_op_CCITTFaxEncode,
	LB_op_DCTDecode,
	LB_op_DCTEncode,
	LB_op_LZWDecode,
	LB_op_LZWEncode,
	LB_op_NullEncode,
	LB_op_RunLengthDecode,
	LB_op_RunLengthEncode,
	LB_op_SubFileDecode,

	LB_op_CIEBasedA,
	LB_op_CIEBasedABC,
	LB_op_DeviceCMYK,
	LB_op_DeviceGray,
	LB_op_DeviceRGB,
	LB_op_Indexed,
	LB_op_Pattern,
	LB_op_Separation,
	LB_op_CIEBasedDEF,
	LB_op_CIEBasedDEFG,

	LB_op_DeviceN,

	LB_op_POSTSCRIPT_RESERVED_END,

	LB_op_END
} LibrettoOperatorEncoding;

struct _LibrettoGraphicState {
	HgObject  object;

	/* device independent parameters */
	HgMatrix   ctm;
	HgMatrix   snapshot_matrix;
	gdouble    x;
	gdouble    y;
	HgPath    *path;
	HgPath    *clip_path;
	/* FIXME: clip path stack */
	HgArray   *color_space;
	HgColor    color;
	HgDict    *font;
	gdouble    line_width;
	gint       line_cap;
	gint       line_join;
	gdouble    miter_limit;
	gdouble    dashline_offset;
	HgArray   *dashline_pattern;
	gboolean   stroke_correction;

	/* device dependent parameters */
	HgDict    *color_rendering;
	gboolean   over_printing;
	HgArray   *black_generator;
	HgArray   *black_corrector;
	HgArray   *transfer;
	/* FIXME: halftone */
	gdouble    smoothing;
	gdouble    shading;
	gpointer   device;
};

struct _LibrettoGraphics {
	HgObject              object;
	HgMemPool            *pool;
	HgPage               *current_page;
	GList                *pages;
	LibrettoGraphicState *current_gstate;
	GList                *gstate_stack;
};


#endif /* __LIBRETTO_TYPES_H__ */
