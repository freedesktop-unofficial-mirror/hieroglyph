/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgtypes.h
 * Copyright (C) 2005-2007 Akira TAGOH
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
#ifndef __HIEROGLYPH__HGTYPES_H__
#define __HIEROGLYPH__HGTYPES_H__

#include <errno.h>
#include <hieroglyph/hgmacros.h>


G_BEGIN_DECLS

/**
 * typedefs
 */
typedef struct hg_object_header_s		hg_object_header_t;
typedef gint32					hg_integer_t;
typedef gfloat					hg_real_t;
typedef struct hg_name_s			hg_name_t;
typedef struct hg_encoding_name_s		hg_encoding_name_t;
typedef gboolean				hg_boolean_t;
typedef struct hg_string_s			hg_string_t;
typedef struct hg_stringdata_s			hg_stringdata_t;
typedef struct hg_array_s			hg_array_t;
typedef struct hg_arraydata_s			hg_arraydata_t;
typedef struct hg_dict_s			hg_dict_t;
typedef struct hg_dictdata_s			hg_dictdata_t;
typedef struct hg_file_s			hg_file_t;
typedef struct hg_filedata_s			hg_filedata_t;
typedef struct hg_filetable_s			hg_filetable_t;
typedef enum hg_filetype_e			hg_filetype_t;
typedef enum hg_filemode_e			hg_filemode_t;
typedef enum hg_filepos_e			hg_filepos_t;
typedef struct hg_operator_s			hg_operator_t;
typedef struct hg_operatordata_s		hg_operatordata_t;
typedef union hg_attribute_u			hg_attribute_t;
typedef struct _hg_object_s			_hg_object_t;
typedef struct hg_object_s			hg_object_t;
typedef enum hg_object_type_e			hg_object_type_t;
typedef enum hg_stack_type_e			hg_stack_type_t;
typedef struct hg_stackdata_s			hg_stackdata_t;
typedef struct hg_stack_s			hg_stack_t;
typedef enum hg_error_e				hg_error_t;
typedef enum hg_system_encoding_e		hg_system_encoding_t;
typedef enum hg_emulation_type_e		hg_emulation_type_t;
typedef struct hg_vm_s				hg_vm_t;

typedef gboolean (* hg_operator_func_t) (hg_vm_t     *vm,
					 hg_object_t *data);


/**
 * Structures
 */
struct hg_object_header_s {
	guchar  token_type;
	guint16 n_objects;
	guint32 total_length;
};
struct hg_name_s {
	guint16 reserved1;
	guint16 length;
};
struct hg_encoding_name_s {
	gint16  representation;
	guint16 index;
};
struct hg_string_s {
	guint16 length;
	guint16 real_length;
};
struct hg_array_s {
	guint16 length;
	guint16 real_length;
};
struct hg_dict_s {
	guint16 length;
	guint16 used;
};
struct hg_file_s {
	guint16 reserved1;
	guint16 reserved2;
};
struct hg_operator_s {
	guint16 length;
	guint16 reserved1;
};
struct hg_operatordata_s {
	hg_operator_func_t func;
	gchar *name[];
};
union hg_attribute_u {
	struct {
		guint8 is_accessible:1;  /* flagging off this bit means noaccess */
		guint8 is_readable:1;    /* flagging off this bit means readonly */
		guint8 is_executeonly:1; /* flagging on this bit means executeonly */
		guint8 is_locked:1;      /* flagging on this bit means the object is locked. */
		guint8 reserved4:1;
		guint8 reserved5:1;
		guint8 is_global:1;      /* flagging on this bit means the object was allocated at the global memory */
		guint8 is_checked:1;     /* for checking the circular reference */
	} bit;
	guint8 attributes;
};
struct _hg_object_s {
	hg_attribute_t attr;
	union {
		struct {
			guint is_executable:1;
			guint object_type:7;
		} v;
		guchar  type;
	} t;
	union {
		hg_integer_t       integer;
		hg_real_t          real;
		hg_name_t          name;
		hg_encoding_name_t encoding;
		hg_boolean_t       boolean;
		hg_string_t        string;
		hg_array_t         array;
		hg_dict_t          dict;
		hg_file_t          file;
		hg_operator_t      operator;
	} v;
};
struct hg_object_s {
	hg_object_header_t header;
	_hg_object_t       object;
	gpointer           data[];
};

struct hg_arraydata_s {
	gchar                   name[256];
	gpointer                array;
	gpointer                data[];
};
struct hg_stringdata_s {
	gpointer string;
	gpointer data[];
};
struct hg_dictdata_s {
	hg_object_t *key;
	hg_object_t *value;
};
enum hg_filetype_e {
	HG_FILE_TYPE_FILE_IO = 1,
	HG_FILE_TYPE_MMAPPED_IO,
	HG_FILE_TYPE_BUFFER,
	HG_FILE_TYPE_STDIN,
	HG_FILE_TYPE_STDOUT,
	HG_FILE_TYPE_STDERR,
	HG_FILE_TYPE_LINEEDIT,
	HG_FILE_TYPE_STATEMENTEDIT,
	HG_FILE_TYPE_CALLBACK,
	HG_FILE_TYPE_END
};
enum hg_filemode_e {
	HG_FILE_MODE_READ      = 1 << 0,
	HG_FILE_MODE_WRITE     = 1 << 1,
	HG_FILE_MODE_READWRITE = 1 << 2
};
enum hg_filepos_e {
	HG_FILE_SEEK_SET = SEEK_SET,
	HG_FILE_SEEK_CUR = SEEK_CUR,
	HG_FILE_SEEK_END = SEEK_END
};
struct hg_filetable_s {
	gboolean (* open)      (gpointer       user_data,
				error_t       *error);
	gsize    (* read)      (gpointer       user_data,
				gpointer       buffer,
				gsize          size,
				gsize          n,
				error_t       *error);
	gsize    (* write)     (gpointer       user_data,
				gconstpointer  buffer,
				gsize          size,
				gsize          n,
				error_t       *error);
	gboolean (* flush)     (gpointer       user_data,
				error_t       *error);
	gssize   (* seek)      (gpointer       user_data,
				gssize         offset,
				hg_filepos_t   whence,
				error_t       *error);
	void     (* close)     (gpointer       user_data,
				error_t       *error);
	gboolean (* is_closed) (gpointer       user_data);
	gboolean (* is_eof)    (gpointer       user_data);
	void     (* clear_eof) (gpointer       user_data);
};
struct hg_filedata_s {
	union {
		gpointer        buffer;
		hg_filetable_t *table;
	} v;
	gint           fd;
	gsize          filesize;
	gsize          current_position;
	gsize          current_line;
	hg_filetype_t  iotype;
	hg_filemode_t  mode;
	gsize          filename_length;
	gchar         *filename[];
};

enum hg_object_type_e {
	HG_OBJECT_TYPE_NULL = 0,
	HG_OBJECT_TYPE_INTEGER,
	HG_OBJECT_TYPE_REAL,
	HG_OBJECT_TYPE_NAME,
	HG_OBJECT_TYPE_BOOLEAN,
	HG_OBJECT_TYPE_STRING,
	HG_OBJECT_TYPE_EVAL,
	HG_OBJECT_TYPE_ARRAY = 9,
	HG_OBJECT_TYPE_MARK,
	/* extended for hieroglyph */
	HG_OBJECT_TYPE_DICT = 65,
	HG_OBJECT_TYPE_FILE,
	HG_OBJECT_TYPE_OPERATOR,
	HG_OBJECT_TYPE_END = (1 << 7) - 1
};

enum hg_stack_type_e {
	HG_STACK_TYPE_OSTACK,
	HG_STACK_TYPE_ESTACK,
	HG_STACK_TYPE_DSTACK,
	HG_STACK_TYPE_END
};
struct hg_stackdata_s {
	gpointer        data;
	hg_stackdata_t *next;
	hg_stackdata_t *prev;
};
struct hg_stack_s {
	hg_stackdata_t *stack_top;
	gsize           stack_depth;
	gsize		current_depth;
	gpointer        data[];
};

enum hg_error_e {
	HG_e_dictfull = 1,
	HG_e_dictstackoverflow,
	HG_e_dictstackunderflow,
	HG_e_execstackoverflow,
	HG_e_handleerror,
	HG_e_interrupt,
	HG_e_invalidaccess,
	HG_e_invalidexit,
	HG_e_invalidfileaccess,
	HG_e_invalidfont,
	HG_e_invalidrestore,
	HG_e_ioerror,
	HG_e_limitcheck,
	HG_e_nocurrentpoint,
	HG_e_rangecheck,
	HG_e_stackoverflow,
	HG_e_stackunderflow,
	HG_e_syntaxerror,
	HG_e_timeout,
	HG_e_typecheck,
	HG_e_undefined,
	HG_e_undefinedfilename,
	HG_e_undefinedresult,
	HG_e_unmatchedmark,
	HG_e_unregistered,
	HG_e_VMerror,
	HG_e_configurationerror,
	HG_e_undefinedresource,
	HG_e_END,
};

enum hg_system_encoding_e {
	HG_enc_abs = 0,
	HG_enc_add,
	HG_enc_aload,
	HG_enc_anchorsearch,
	HG_enc_and,
	HG_enc_arc,
	HG_enc_arcn,
	HG_enc_arct,
	HG_enc_arcto,
	HG_enc_array,

	HG_enc_ashow,
	HG_enc_astore,
	HG_enc_awidthshow,
	HG_enc_begin,
	HG_enc_bind,
	HG_enc_bitshift,
	HG_enc_ceiling,
	HG_enc_charpath,
	HG_enc_clear,
	HG_enc_cleartomark,

	HG_enc_clip,
	HG_enc_clippath,
	HG_enc_closepath,
	HG_enc_concat,
	HG_enc_concatmatrix,
	HG_enc_copy,
	HG_enc_count,
	HG_enc_counttomark,
	HG_enc_currentcmykcolor,
	HG_enc_currentdash,

	HG_enc_currentdict,
	HG_enc_currentfile,
	HG_enc_currentfont,
	HG_enc_currentgray,
	HG_enc_currentgstate,
	HG_enc_currenthsbcolor,
	HG_enc_currentlinecap,
	HG_enc_currentlinejoin,
	HG_enc_currentlinewidth,
	HG_enc_currentmatrix,

	HG_enc_currentpoint,
	HG_enc_currentrgbcolor,
	HG_enc_currentshared,
	HG_enc_curveto,
	HG_enc_cvi,
	HG_enc_cvlit,
	HG_enc_cvn,
	HG_enc_cvr,
	HG_enc_cvrs,
	HG_enc_cvs,

	HG_enc_cvx,
	HG_enc_def,
	HG_enc_defineusername,
	HG_enc_dict,
	HG_enc_div,
	HG_enc_dtransform,
	HG_enc_dup,
	HG_enc_end,
	HG_enc_eoclip,
	HG_enc_eofill,

	HG_enc_eoviewclip,
	HG_enc_eq,
	HG_enc_exch,
	HG_enc_exec,
	HG_enc_exit,
	HG_enc_file,
	HG_enc_fill,
	HG_enc_findfont,
	HG_enc_flattenpath,
	HG_enc_floor,

	HG_enc_flush,
	HG_enc_flushfile,
	HG_enc_for,
	HG_enc_forall,
	HG_enc_ge,
	HG_enc_get,
	HG_enc_getinterval,
	HG_enc_grestore,
	HG_enc_gsave,
	HG_enc_gstate,

	HG_enc_gt,
	HG_enc_identmatrix,
	HG_enc_idiv,
	HG_enc_idtransform,
	HG_enc_if,
	HG_enc_ifelse,
	HG_enc_image,
	HG_enc_imagemask,
	HG_enc_index,
	HG_enc_ineofill,

	HG_enc_infill,
	HG_enc_initviewclip,
	HG_enc_inueofill,
	HG_enc_inufill,
	HG_enc_invertmatrix,
	HG_enc_itransform,
	HG_enc_known,
	HG_enc_le,
	HG_enc_length,
	HG_enc_lineto,

	HG_enc_load,
	HG_enc_loop,
	HG_enc_lt,
	HG_enc_makefont,
	HG_enc_matrix,
	HG_enc_maxlength,
	HG_enc_mod,
	HG_enc_moveto,
	HG_enc_mul,
	HG_enc_ne,

	HG_enc_neg,
	HG_enc_newpath,
	HG_enc_not,
	HG_enc_null,
	HG_enc_or,
	HG_enc_pathbbox,
	HG_enc_pathforall,
	HG_enc_pop,
	HG_enc_print,
	HG_enc_printobject,

	HG_enc_put,
	HG_enc_putinterval,
	HG_enc_rcurveto,
	HG_enc_read,
	HG_enc_readhexstring,
	HG_enc_readline,
	HG_enc_readstring,
	HG_enc_rectclip,
	HG_enc_rectfill,
	HG_enc_rectstroke,

	HG_enc_rectviewclip,
	HG_enc_repeat,
	HG_enc_restore,
	HG_enc_rlineto,
	HG_enc_rmoveto,
	HG_enc_roll,
	HG_enc_rotate,
	HG_enc_round,
	HG_enc_save,
	HG_enc_scale,

	HG_enc_scalefont,
	HG_enc_search,
	HG_enc_selectfont,
	HG_enc_setbbox,
	HG_enc_setcachedevice,
	HG_enc_setcachedevice2,
	HG_enc_setcharwidth,
	HG_enc_setcmykcolor,
	HG_enc_setdash,
	HG_enc_setfont,

	HG_enc_setgray,
	HG_enc_setgstate,
	HG_enc_sethsbcolor,
	HG_enc_setlinecap,
	HG_enc_setlinejoin,
	HG_enc_setlinewidth,
	HG_enc_setmatrix,
	HG_enc_setrgbcolor,
	HG_enc_setshared,
	HG_enc_shareddict,

	HG_enc_show,
	HG_enc_showpage,
	HG_enc_stop,
	HG_enc_stopped,
	HG_enc_store,
	HG_enc_string,
	HG_enc_stringwidth,
	HG_enc_stroke,
	HG_enc_strokepath,
	HG_enc_sub,

	HG_enc_systemdict,
	HG_enc_token,
	HG_enc_transform,
	HG_enc_translate,
	HG_enc_truncate,
	HG_enc_type,
	HG_enc_uappend,
	HG_enc_ucache,
	HG_enc_ueofill,
	HG_enc_ufill,

	HG_enc_undef,
	HG_enc_upath,
	HG_enc_userdict,
	HG_enc_ustroke,
	HG_enc_viewclip,
	HG_enc_viewclippath,
	HG_enc_where,
	HG_enc_widthshow,
	HG_enc_write,
	HG_enc_writehexstring,

	HG_enc_writeobject,
	HG_enc_writestring,
	HG_enc_wtranslation,
	HG_enc_xor,
	HG_enc_xshow,
	HG_enc_xyshow,
	HG_enc_yshow,
	HG_enc_FontDirectory,
	HG_enc_SharedFontDirectory,
	HG_enc_Courier,

	HG_enc_Courier_Bold,
	HG_enc_Courier_BoldOblique,
	HG_enc_Courier_Oblique,
	HG_enc_Helvetica,
	HG_enc_Helvetica_Bold,
	HG_enc_Helvetica_BoldOblique,
	HG_enc_Helvetica_Oblique,
	HG_enc_Symbol,
	HG_enc_Times_Bold,
	HG_enc_Times_BoldItalic,

	HG_enc_Times_Italic,
	HG_enc_Times_Roman,
	HG_enc_execuserobject,
	HG_enc_currentcolor,
	HG_enc_currentcolorspace,
	HG_enc_currentglobal,
	HG_enc_execform,
	HG_enc_filter,
	HG_enc_findresource,
	HG_enc_globaldict,

	HG_enc_makepattern,
	HG_enc_setcolor,
	HG_enc_setcolorspace,
	HG_enc_setglobal,
	HG_enc_setpagedevice,
	HG_enc_setpattern,

	HG_enc_sym_eq = 256,
	HG_enc_sym_eqeq,
	HG_enc_ISOLatin1Encoding,
	HG_enc_StandardEncoding,

	HG_enc_sym_left_square_bracket,
	HG_enc_sym_right_square_bracket,
	HG_enc_atan,
	HG_enc_banddevice,
	HG_enc_bytesavailable,
	HG_enc_cachestatus,
	HG_enc_closefile,
	HG_enc_colorimage,
	HG_enc_condition,
	HG_enc_copypage,

	HG_enc_cos,
	HG_enc_countdictstack,
	HG_enc_countexecstack,
	HG_enc_cshow,
	HG_enc_currentblackgeneration,
	HG_enc_currentcacheparams,
	HG_enc_currentcolorscreen,
	HG_enc_currentcolortransfer,
	HG_enc_currentcontext,
	HG_enc_currentflat,

	HG_enc_currenthalftone,
	HG_enc_currenthalftonephase,
	HG_enc_currentmiterlimit,
	HG_enc_currentobjectformat,
	HG_enc_currentpacking,
	HG_enc_currentscreen,
	HG_enc_currentstrokeadjust,
	HG_enc_currenttransfer,
	HG_enc_currentundercolorremoval,
	HG_enc_defaultmatrix,

	HG_enc_definefont,
	HG_enc_deletefile,
	HG_enc_detach,
	HG_enc_deviceinfo,
	HG_enc_dictstack,
	HG_enc_echo,
	HG_enc_erasepage,
	HG_enc_errordict,
	HG_enc_execstack,
	HG_enc_executeonly,

	HG_enc_exp,
	HG_enc_false,
	HG_enc_filenameforall,
	HG_enc_fileposition,
	HG_enc_fork,
	HG_enc_framedevice,
	HG_enc_grestoreall,
	HG_enc_handleerror,
	HG_enc_initclip,
	HG_enc_initgraphics,

	HG_enc_initmatrix,
	HG_enc_instroke,
	HG_enc_inustroke,
	HG_enc_join,
	HG_enc_kshow,
	HG_enc_ln,
	HG_enc_lock,
	HG_enc_log,
	HG_enc_mark,
	HG_enc_monitor,

	HG_enc_noaccess,
	HG_enc_notify,
	HG_enc_nulldevice,
	HG_enc_packedarray,
	HG_enc_quit,
	HG_enc_rand,
	HG_enc_rcheck,
	HG_enc_readonly,
	HG_enc_realtime,
	HG_enc_renamefile,

	HG_enc_renderbands,
	HG_enc_resetfile,
	HG_enc_reversepath,
	HG_enc_rootfont,
	HG_enc_rrand,
	HG_enc_run,
	HG_enc_scheck,
	HG_enc_setblackgeneration,
	HG_enc_setcachelimit,
	HG_enc_setcacheparams,

	HG_enc_setcolorscreen,
	HG_enc_setcolortransfer,
	HG_enc_setfileposition,
	HG_enc_setflat,
	HG_enc_sethalftone,
	HG_enc_sethalftonephase,
	HG_enc_setmiterlimit,
	HG_enc_setobjectformat,
	HG_enc_setpacking,
	HG_enc_setscreen,

	HG_enc_setstrokeadjust,
	HG_enc_settransfer,
	HG_enc_setucacheparams,
	HG_enc_setundercolorremoval,
	HG_enc_sin,
	HG_enc_sqrt,
	HG_enc_srand,
	HG_enc_stack,
	HG_enc_status,
	HG_enc_statusdict,

	HG_enc_true,
	HG_enc_ucachestatus,
	HG_enc_undefinefont,
	HG_enc_usertime,
	HG_enc_ustrokepath,
	HG_enc_version,
	HG_enc_vmreclaim,
	HG_enc_vmstatus,
	HG_enc_wait,
	HG_enc_wcheck,

	HG_enc_xcheck,
	HG_enc_yield,
	HG_enc_defineuserobject,
	HG_enc_undefineuserobject,
	HG_enc_UserObjects,
	HG_enc_cleardictstack,
	HG_enc_A,
	HG_enc_B,
	HG_enc_C,
	HG_enc_D,

	HG_enc_E,
	HG_enc_F,
	HG_enc_G,
	HG_enc_H,
	HG_enc_I,
	HG_enc_J,
	HG_enc_K,
	HG_enc_L,
	HG_enc_M,
	HG_enc_N,

	HG_enc_O,
	HG_enc_P,
	HG_enc_Q,
	HG_enc_R,
	HG_enc_S,
	HG_enc_T,
	HG_enc_U,
	HG_enc_V,
	HG_enc_W,
	HG_enc_X,

	HG_enc_Y,
	HG_enc_Z,
	HG_enc_a,
	HG_enc_b,
	HG_enc_c,
	HG_enc_d,
	HG_enc_e,
	HG_enc_f,
	HG_enc_g,
	HG_enc_h,

	HG_enc_i,
	HG_enc_j,
	HG_enc_k,
	HG_enc_l,
	HG_enc_m,
	HG_enc_n,
	HG_enc_o,
	HG_enc_p,
	HG_enc_q,
	HG_enc_r,

	HG_enc_s,
	HG_enc_t,
	HG_enc_u,
	HG_enc_v,
	HG_enc_w,
	HG_enc_x,
	HG_enc_y,
	HG_enc_z,
	HG_enc_setvmthreshold,
	HG_enc_sym_begin_dict_mark,

	HG_enc_sym_end_dict_mark,
	HG_enc_currentcolorrendering,
	HG_enc_currentdevparams,
	HG_enc_currentoverprint,
	HG_enc_currentpagedevice,
	HG_enc_currentsystemparams,
	HG_enc_currentuserparams,
	HG_enc_defineresource,
	HG_enc_findencoding,
	HG_enc_gcheck,

	HG_enc_glyphshow,
	HG_enc_languagelevel,
	HG_enc_product,
	HG_enc_pstack,
	HG_enc_resourceforall,
	HG_enc_resourcestatus,
	HG_enc_revision,
	HG_enc_serialnumber,
	HG_enc_setcolorrendering,
	HG_enc_setdevparams,

	HG_enc_setoverprint,
	HG_enc_setsystemparams,
	HG_enc_setuserparams,
	HG_enc_startjob,
	HG_enc_undefineresource,
	HG_enc_GlobalFontDirectory,
	HG_enc_ASCII85Decode,
	HG_enc_ASCII85Encode,
	HG_enc_ASCIIHexDecode,
	HG_enc_ASCIIHexEncode,

	HG_enc_CCITTFaxDecode,
	HG_enc_CCITTFaxEncode,
	HG_enc_DCTDecode,
	HG_enc_DCTEncode,
	HG_enc_LZWDecode,
	HG_enc_LZWEncode,
	HG_enc_NullEncode,
	HG_enc_RunLengthDecode,
	HG_enc_RunLengthEncode,
	HG_enc_SubFileDecode,

	HG_enc_CIEBasedA,
	HG_enc_CIEBasedABC,
	HG_enc_DeviceCMYK,
	HG_enc_DeviceGray,
	HG_enc_DeviceRGB,
	HG_enc_Indexed,
	HG_enc_Pattern,
	HG_enc_Separation,
	HG_enc_CIEBasedDEF,
	HG_enc_CIEBasedDEFG,

	HG_enc_DeviceN,

	HG_enc_POSTSCRIPT_RESERVED_END,

	HG_enc_END
};
enum hg_emulation_type_e {
	HG_EMU_BEGIN,
	HG_EMU_PS_LEVEL_1,
	HG_EMU_PS_LEVEL_2,
	HG_EMU_PS_LEVEL_3,
	HG_EMU_END
};

struct hg_vm_s {
};

G_END_DECLS

#endif /* __HIEROGLYPH__HGTYPES_H__ */
