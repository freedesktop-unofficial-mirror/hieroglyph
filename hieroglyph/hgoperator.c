/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgoperator.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hgerror.h"
#include "hgbool.h"
#include "hgdict.h"
#include "hgint.h"
#include "hgmark.h"
#include "hgname.h"
#include "hgnull.h"
#include "hgquark.h"
#include "hgvm.h"
#include "hgoperator.h"


static hg_operator_func_t __hg_operator_func_table[HG_enc_END];
static gchar *__hg_operator_name_table[HG_enc_END];
static gboolean __hg_operator_is_initialized = FALSE;

/*< private >*/
#define PROTO_OPER(_n_)							\
	static gboolean _hg_operator_real_ ## _n_(hg_vm_t  *vm,		\
						  GError  **error);
#define DEFUNC_OPER(_n_)						\
	static gboolean							\
	_hg_operator_real_ ## _n_(hg_vm_t  *vm,				\
				  GError  **error)			\
	{								\
		hg_stack_t *ostack G_GNUC_UNUSED = vm->stacks[HG_VM_STACK_OSTACK]; \
		hg_stack_t *estack G_GNUC_UNUSED = vm->stacks[HG_VM_STACK_ESTACK]; \
		hg_stack_t *dstack G_GNUC_UNUSED = vm->stacks[HG_VM_STACK_DSTACK]; \
		hg_quark_t qself G_GNUC_UNUSED = hg_stack_index(estack, 0, error); \
		gboolean retval = FALSE;

#define DEFUNC_OPER_END					\
		return retval;				\
	}

#define DEFUNC_UNIMPLEMENTED_OPER(_n_)					\
	static gboolean							\
	_hg_operator_real_ ## _n_(hg_vm_t  *vm,				\
				  GError  **error)			\
	{								\
		g_warning("%s isn't yet implemented.", #_n_);		\
		hg_vm_set_error(vm,					\
				hg_stack_index(vm->stacks[HG_VM_STACK_ESTACK], 0, error), \
				HG_VM_e_VMerror);					\
		return FALSE;						\
	}

#define CHECK_STACK(_s_,_n_)						\
	G_STMT_START {							\
		if (hg_stack_depth((_s_)) < (_n_)) {			\
			hg_vm_set_error(vm, qself, HG_VM_e_stackunderflow); \
			return FALSE;					\
		}							\
	} G_STMT_END
#define STACK_PUSH_AS_IS(_s_,_q_)					\
	G_STMT_START {							\
		if (!hg_stack_push((_s_), (_q_))) {			\
			if ((_s_) == ostack) {				\
				hg_vm_set_error(vm, qself, HG_VM_e_stackoverflow); \
			} else if ((_s_) == estack) {			\
				hg_vm_set_error(vm, qself, HG_VM_e_execstackoverflow); \
			} else {					\
				hg_vm_set_error(vm, qself, HG_VM_e_dictstackoverflow); \
			}						\
			return FALSE;					\
		}							\
	} G_STMT_END
#define STACK_PUSH(_s_,_q_)						\
	G_STMT_START {							\
		hg_quark_t _qq_ = (_q_);				\
		hg_vm_quark_set_attributes(vm, &_qq_);			\
		STACK_PUSH_AS_IS ((_s_), _qq_);				\
	} G_STMT_END


PROTO_OPER (private_forceput);
PROTO_OPER (private_odef);
PROTO_OPER (private_setglobal);
PROTO_OPER (private_undef);
PROTO_OPER (abs);
PROTO_OPER (add);
PROTO_OPER (aload);
PROTO_OPER (anchorsearch);
PROTO_OPER (and);
PROTO_OPER (arc);
PROTO_OPER (arcn);
PROTO_OPER (arct);
PROTO_OPER (arcto);
PROTO_OPER (array);
PROTO_OPER (ashow);
PROTO_OPER (astore);
PROTO_OPER (awidthshow);
PROTO_OPER (begin);
PROTO_OPER (bind);
PROTO_OPER (bitshift);
PROTO_OPER (ceiling);
PROTO_OPER (charpath);
PROTO_OPER (clear);
PROTO_OPER (cleartomark);
PROTO_OPER (clip);
PROTO_OPER (clippath);
PROTO_OPER (closepath);
PROTO_OPER (concat);
PROTO_OPER (concatmatrix);
PROTO_OPER (copy);
PROTO_OPER (count);
PROTO_OPER (counttomark);
PROTO_OPER (currentcmykcolor);
PROTO_OPER (currentdash);
PROTO_OPER (currentdict);
PROTO_OPER (currentfile);
PROTO_OPER (currentfont);
PROTO_OPER (currentgray);
PROTO_OPER (currentgstate);
PROTO_OPER (currenthsbcolor);
PROTO_OPER (currentlinecap);
PROTO_OPER (currentlinejoin);
PROTO_OPER (currentlinewidth);
PROTO_OPER (currentmatrix);
PROTO_OPER (currentpoint);
PROTO_OPER (currentrgbcolor);
PROTO_OPER (currentshared);
PROTO_OPER (curveto);
PROTO_OPER (cvi);
PROTO_OPER (cvlit);
PROTO_OPER (cvn);
PROTO_OPER (cvr);
PROTO_OPER (cvrs);
PROTO_OPER (cvs);
PROTO_OPER (cvx);
PROTO_OPER (def);
PROTO_OPER (defineusername);
PROTO_OPER (dict);
PROTO_OPER (div);
PROTO_OPER (dtransform);
PROTO_OPER (dup);
PROTO_OPER (end);
PROTO_OPER (eoclip);
PROTO_OPER (eofill);
PROTO_OPER (eoviewclip);
PROTO_OPER (eq);
PROTO_OPER (exch);
PROTO_OPER (exec);
PROTO_OPER (exit);
PROTO_OPER (file);
PROTO_OPER (fill);
PROTO_OPER (findfont);
PROTO_OPER (flattenpath);
PROTO_OPER (floor);
PROTO_OPER (flush);
PROTO_OPER (flushfile);
PROTO_OPER (for);
PROTO_OPER (forall);
PROTO_OPER (ge);
PROTO_OPER (get);
PROTO_OPER (getinterval);
PROTO_OPER (grestore);
PROTO_OPER (gsave);
PROTO_OPER (gstate);
PROTO_OPER (gt);
PROTO_OPER (identmatrix);
PROTO_OPER (idiv);
PROTO_OPER (idtransform);
PROTO_OPER (if);
PROTO_OPER (ifelse);
PROTO_OPER (image);
PROTO_OPER (imagemask);
PROTO_OPER (index);
PROTO_OPER (ineofill);
PROTO_OPER (infill);
PROTO_OPER (initviewclip);
PROTO_OPER (inueofill);
PROTO_OPER (inufill);
PROTO_OPER (invertmatrix);
PROTO_OPER (itransform);
PROTO_OPER (known);
PROTO_OPER (le);
PROTO_OPER (length);
PROTO_OPER (lineto);
PROTO_OPER (load);
PROTO_OPER (loop);
PROTO_OPER (lt);
PROTO_OPER (makefont);
PROTO_OPER (matrix);
PROTO_OPER (maxlength);
PROTO_OPER (mod);
PROTO_OPER (moveto);
PROTO_OPER (mul);
PROTO_OPER (ne);
PROTO_OPER (neg);
PROTO_OPER (newpath);
PROTO_OPER (not);
PROTO_OPER (or);
PROTO_OPER (pathbbox);
PROTO_OPER (pathforall);
PROTO_OPER (pop);
PROTO_OPER (print);
PROTO_OPER (printobject);
PROTO_OPER (put);
PROTO_OPER (putinterval);
PROTO_OPER (rcurveto);
PROTO_OPER (read);
PROTO_OPER (readhexstring);
PROTO_OPER (readline);
PROTO_OPER (readstring);
PROTO_OPER (rectclip);
PROTO_OPER (rectfill);
PROTO_OPER (rectstroke);
PROTO_OPER (rectviewclip);
PROTO_OPER (repeat);
PROTO_OPER (restore);
PROTO_OPER (rlineto);
PROTO_OPER (rmoveto);
PROTO_OPER (roll);
PROTO_OPER (rotate);
PROTO_OPER (round);
PROTO_OPER (save);
PROTO_OPER (scale);
PROTO_OPER (scalefont);
PROTO_OPER (search);
PROTO_OPER (selectfont);
PROTO_OPER (setbbox);
PROTO_OPER (setcachedevice);
PROTO_OPER (setcachedevice2);
PROTO_OPER (setcharwidth);
PROTO_OPER (setcmykcolor);
PROTO_OPER (setdash);
PROTO_OPER (setfont);
PROTO_OPER (setgray);
PROTO_OPER (setgstate);
PROTO_OPER (sethsbcolor);
PROTO_OPER (setlinecap);
PROTO_OPER (setlinejoin);
PROTO_OPER (setlinewidth);
PROTO_OPER (setmatrix);
PROTO_OPER (setrgbcolor);
PROTO_OPER (setshared);
PROTO_OPER (shareddict);
PROTO_OPER (show);
PROTO_OPER (showpage);
PROTO_OPER (stop);
PROTO_OPER (stopped);
PROTO_OPER (store);
PROTO_OPER (string);
PROTO_OPER (stringwidth);
PROTO_OPER (stroke);
PROTO_OPER (strokepath);
PROTO_OPER (sub);
PROTO_OPER (token);
PROTO_OPER (transform);
PROTO_OPER (translate);
PROTO_OPER (truncate);
PROTO_OPER (type);
PROTO_OPER (uappend);
PROTO_OPER (ucache);
PROTO_OPER (ueofill);
PROTO_OPER (ufill);
PROTO_OPER (undef);
PROTO_OPER (upath);
PROTO_OPER (userdict);
PROTO_OPER (ustroke);
PROTO_OPER (viewclip);
PROTO_OPER (viewclippath);
PROTO_OPER (where);
PROTO_OPER (widthshow);
PROTO_OPER (write);
PROTO_OPER (writehexstring);
PROTO_OPER (writeobject);
PROTO_OPER (writestring);
PROTO_OPER (wtranslation);
PROTO_OPER (xor);
PROTO_OPER (xshow);
PROTO_OPER (xyshow);
PROTO_OPER (yshow);
PROTO_OPER (FontDirectory);
PROTO_OPER (SharedFontDirectory);
PROTO_OPER (execuserobject);
PROTO_OPER (currentcolor);
PROTO_OPER (currentcolorspace);
PROTO_OPER (currentglobal);
PROTO_OPER (execform);
PROTO_OPER (filter);
PROTO_OPER (findresource);
PROTO_OPER (makepattern);
PROTO_OPER (setcolor);
PROTO_OPER (setcolorspace);
PROTO_OPER (setglobal);
PROTO_OPER (setpagedevice);
PROTO_OPER (setpattern);
PROTO_OPER (sym_eq);
PROTO_OPER (sym_eqeq);
PROTO_OPER (ISOLatin1Encoding);
PROTO_OPER (StandardEncoding);
PROTO_OPER (sym_left_square_bracket);
PROTO_OPER (sym_right_square_bracket);
PROTO_OPER (atan);
PROTO_OPER (banddevice);
PROTO_OPER (bytesavailable);
PROTO_OPER (cachestatus);
PROTO_OPER (closefile);
PROTO_OPER (colorimage);
PROTO_OPER (condition);
PROTO_OPER (copypage);
PROTO_OPER (cos);
PROTO_OPER (countdictstack);
PROTO_OPER (countexecstack);
PROTO_OPER (cshow);
PROTO_OPER (currentblackgeneration);
PROTO_OPER (currentcacheparams);
PROTO_OPER (currentcolorscreen);
PROTO_OPER (currentcolortransfer);
PROTO_OPER (currentcontext);
PROTO_OPER (currentflat);
PROTO_OPER (currenthalftone);
PROTO_OPER (currenthalftonephase);
PROTO_OPER (currentmiterlimit);
PROTO_OPER (currentobjectformat);
PROTO_OPER (currentpacking);
PROTO_OPER (currentscreen);
PROTO_OPER (currentstrokeadjust);
PROTO_OPER (currenttransfer);
PROTO_OPER (currentundercolorremoval);
PROTO_OPER (defaultmatrix);
PROTO_OPER (definefont);
PROTO_OPER (deletefile);
PROTO_OPER (detach);
PROTO_OPER (deviceinfo);
PROTO_OPER (dictstack);
PROTO_OPER (echo);
PROTO_OPER (erasepage);
PROTO_OPER (execstack);
PROTO_OPER (executeonly);
PROTO_OPER (exp);
PROTO_OPER (filenameforall);
PROTO_OPER (fileposition);
PROTO_OPER (fork);
PROTO_OPER (framedevice);
PROTO_OPER (grestoreall);
PROTO_OPER (handleerror);
PROTO_OPER (initclip);
PROTO_OPER (initgraphics);
PROTO_OPER (initmatrix);
PROTO_OPER (instroke);
PROTO_OPER (inustroke);
PROTO_OPER (join);
PROTO_OPER (kshow);
PROTO_OPER (ln);
PROTO_OPER (lock);
PROTO_OPER (log);
PROTO_OPER (monitor);
PROTO_OPER (noaccess);
PROTO_OPER (notify);
PROTO_OPER (nulldevice);
PROTO_OPER (packedarray);
PROTO_OPER (quit);
PROTO_OPER (rand);
PROTO_OPER (rcheck);
PROTO_OPER (readonly);
PROTO_OPER (realtime);
PROTO_OPER (renamefile);
PROTO_OPER (renderbands);
PROTO_OPER (resetfile);
PROTO_OPER (reversepath);
PROTO_OPER (rootfont);
PROTO_OPER (rrand);
PROTO_OPER (run);
PROTO_OPER (scheck);
PROTO_OPER (setblackgeneration);
PROTO_OPER (setcachelimit);
PROTO_OPER (setcacheparams);
PROTO_OPER (setcolorscreen);
PROTO_OPER (setcolortransfer);
PROTO_OPER (setfileposition);
PROTO_OPER (setflat);
PROTO_OPER (sethalftone);
PROTO_OPER (sethalftonephase);
PROTO_OPER (setmiterlimit);
PROTO_OPER (setobjectformat);
PROTO_OPER (setpacking);
PROTO_OPER (setscreen);
PROTO_OPER (setstrokeadjust);
PROTO_OPER (settransfer);
PROTO_OPER (setucacheparams);
PROTO_OPER (setundercolorremoval);
PROTO_OPER (sin);
PROTO_OPER (sqrt);
PROTO_OPER (srand);
PROTO_OPER (stack);
PROTO_OPER (status);
PROTO_OPER (statusdict);
PROTO_OPER (ucachestatus);
PROTO_OPER (undefinefont);
PROTO_OPER (usertime);
PROTO_OPER (ustrokepath);
PROTO_OPER (version);
PROTO_OPER (vmreclaim);
PROTO_OPER (vmstatus);
PROTO_OPER (wait);
PROTO_OPER (wcheck);
PROTO_OPER (xcheck);
PROTO_OPER (yield);
PROTO_OPER (defineuserobject);
PROTO_OPER (undefineuserobject);
PROTO_OPER (UserObjects);
PROTO_OPER (cleardictstack);
PROTO_OPER (setvmthreshold);
PROTO_OPER (sym_begin_dict_mark);
PROTO_OPER (sym_end_dict_mark);
PROTO_OPER (currentcolorrendering);
PROTO_OPER (currentdevparams);
PROTO_OPER (currentoverprint);
PROTO_OPER (currentpagedevice);
PROTO_OPER (currentsystemparams);
PROTO_OPER (currentuserparams);
PROTO_OPER (defineresource);
PROTO_OPER (findencoding);
PROTO_OPER (gcheck);
PROTO_OPER (glyphshow);
PROTO_OPER (languagelevel);
PROTO_OPER (product);
PROTO_OPER (pstack);
PROTO_OPER (resourceforall);
PROTO_OPER (resourcestatus);
PROTO_OPER (revision);
PROTO_OPER (serialnumber);
PROTO_OPER (setcolorrendering);
PROTO_OPER (setdevparams);
PROTO_OPER (setoverprint);
PROTO_OPER (setsystemparams);
PROTO_OPER (setuserparams);
PROTO_OPER (startjob);
PROTO_OPER (undefineresource);
PROTO_OPER (GlobalFontDirectory);
PROTO_OPER (ASCII85Decode);
PROTO_OPER (ASCII85Encode);
PROTO_OPER (ASCIIHexDecode);
PROTO_OPER (ASCIIHexEncode);
PROTO_OPER (CCITTFaxDecode);
PROTO_OPER (CCITTFaxEncode);
PROTO_OPER (DCTDecode);
PROTO_OPER (DCTEncode);
PROTO_OPER (LZWDecode);
PROTO_OPER (LZWEncode);
PROTO_OPER (NullEncode);
PROTO_OPER (RunLengthDecode);
PROTO_OPER (RunLengthEncode);
PROTO_OPER (SubFileDecode);
PROTO_OPER (CIEBasedA);
PROTO_OPER (CIEBasedABC);
PROTO_OPER (DeviceCMYK);
PROTO_OPER (DeviceGray);
PROTO_OPER (DeviceRGB);
PROTO_OPER (Indexed);
PROTO_OPER (Pattern);
PROTO_OPER (Separation);
PROTO_OPER (CIEBasedDEF);
PROTO_OPER (CIEBasedDEFG);
PROTO_OPER (DeviceN);


/* <array> <index> <any> .forceput -
 * <dict> <key> <any> .forceput -
 * <string> <index> <int> .forceput -
 */
DEFUNC_OPER (private_forceput)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2;

	CHECK_STACK (ostack, 3);
	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);
	if (!hg_quark_is_writable(arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_IS_QARRAY (arg0)) {
		gsize index;
		hg_array_t *a;

		if (!HG_IS_QINT (arg1)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		a = HG_VM_LOCK (vm, arg0, error);
		if (a == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		index = HG_INT (arg1);
		if (hg_array_length(a) < index) {
			HG_VM_UNLOCK (vm, arg0);
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			return FALSE;
		}
		retval = hg_array_set(a, arg2, index, error);

		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QDICT (arg0)) {
		hg_dict_t *d;

		d = HG_VM_LOCK (vm, arg0, error);
		if (d == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		retval = hg_dict_add(d, arg1, arg2);

		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;

		if (!HG_IS_QINT (arg1) ||
		    !HG_IS_QINT (arg2)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		s = HG_VM_LOCK (vm, arg0, error);
		if (s == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		retval = hg_string_overwrite_c(s, arg2, arg1, error);

		HG_VM_UNLOCK (vm, arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);
} G_STMT_END;
DEFUNC_OPER_END

/* <dict> <key> <proc> .odef - */
DEFUNC_OPER (private_odef)
G_STMT_START {
	hg_quark_t arg0, arg1;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QNAME (arg0) ||
	    !HG_IS_QARRAY (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (hg_quark_is_executable(arg1)) {
		hg_array_t *a = HG_VM_LOCK (vm, arg1, error);
		const gchar *name = hg_name_lookup(vm->name, arg0);

		hg_array_set_name(a, name);
		HG_VM_UNLOCK (vm, arg1);

		hg_quark_set_access_bits(&arg1, FALSE, FALSE, TRUE);
	}
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, arg1);
	retval = _hg_operator_real_def(vm, error);
} G_STMT_END;
DEFUNC_OPER_END

/* <bool> .setglobal - */
DEFUNC_OPER (private_setglobal)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QBOOL(arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	hg_vm_use_global_mem(vm, HG_BOOL (arg0));

	hg_stack_pop(ostack, error);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

/* <dict> <key> .undef - */
DEFUNC_OPER (private_undef)
G_STMT_START {
	hg_quark_t arg0, arg1;
	hg_dict_t *dict;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QDICT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_quark_is_writable(arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	dict = HG_VM_LOCK (vm, arg0, error);
	if (dict == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_dict_remove(dict, arg1);

	HG_VM_UNLOCK (vm, arg0);

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (abs);
DEFUNC_UNIMPLEMENTED_OPER (add);
DEFUNC_UNIMPLEMENTED_OPER (aload);
DEFUNC_UNIMPLEMENTED_OPER (anchorsearch);
DEFUNC_UNIMPLEMENTED_OPER (and);
DEFUNC_UNIMPLEMENTED_OPER (arc);
DEFUNC_UNIMPLEMENTED_OPER (arcn);
DEFUNC_UNIMPLEMENTED_OPER (arct);
DEFUNC_UNIMPLEMENTED_OPER (arcto);

/* <int> array <array> */
DEFUNC_OPER (array)
G_STMT_START {
	hg_quark_t arg0, q;
	hg_array_t *a;
	gsize i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_INT (arg0) < 0 ||
	    HG_INT (arg0) > 65535) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	q = hg_array_new(hg_vm_get_mem(vm), HG_INT (arg0), (gpointer *)&a);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	for (i = 0; i < HG_INT (arg0); i++) {
		hg_array_set(a, HG_QNULL, i, error);
	}
	HG_VM_UNLOCK (vm, q);

	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ashow);

/* <any> ... <array> astore <array> */
DEFUNC_OPER (astore)
G_STMT_START {
	hg_quark_t arg0, q;
	hg_array_t *a;
	gsize len, i;
	gboolean is_in_global;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_quark_is_writable(arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	len = hg_array_length(a);
	if (hg_stack_depth(ostack) < (len + 1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_stackunderflow);
		goto error;
	}
	is_in_global = hg_quark_has_same_mem_id(arg0, vm->mem_id[HG_VM_MEM_GLOBAL]);
	for (i = 0; i < len; i++) {
		q = hg_stack_index(ostack, len - i, error);
		if (is_in_global) {
			if (!hg_quark_is_simple_object(q) &&
			    hg_quark_has_same_mem_id(q, vm->mem_id[HG_VM_MEM_LOCAL])) {
				hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
				goto error;
			}
		}
		hg_array_set(a, q, i, error);
	}
	for (i = 0; i <= len; i++)
		hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, arg0);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (awidthshow);
DEFUNC_UNIMPLEMENTED_OPER (begin);

/* <proc> bind <proc> */
DEFUNC_OPER (bind)
G_STMT_START {
	hg_quark_t arg0, q;
	hg_array_t *a;
	gsize len, i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_quark_is_writable(arg0)) {
		/* ignore it */
		return TRUE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	len = hg_array_length(a);
	for (i = 0; i < len; i++) {
		q = hg_array_get(a, i, error);
		if (hg_quark_is_executable(q)) {
			if (HG_IS_QARRAY (q)) {
				STACK_PUSH (ostack, q);
				_hg_operator_real_bind(vm, error);
				hg_stack_pop(ostack, error);
			} else if (HG_IS_QNAME (q)) {
				hg_quark_t qop = hg_vm_dict_lookup(vm, q);

				if (HG_IS_QOPER (qop)) {
					hg_array_set(a, qop, i, error);
				}
			}
		}
	}
	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (bitshift);
DEFUNC_UNIMPLEMENTED_OPER (ceiling);
DEFUNC_UNIMPLEMENTED_OPER (charpath);
DEFUNC_UNIMPLEMENTED_OPER (clear);
DEFUNC_UNIMPLEMENTED_OPER (cleartomark);
DEFUNC_UNIMPLEMENTED_OPER (clip);
DEFUNC_UNIMPLEMENTED_OPER (clippath);
DEFUNC_UNIMPLEMENTED_OPER (closepath);
DEFUNC_UNIMPLEMENTED_OPER (concat);
DEFUNC_UNIMPLEMENTED_OPER (concatmatrix);

static gboolean
_hg_operator_copy_real_traverse_dict(hg_mem_t    *mem,
				     hg_quark_t   qkey,
				     hg_quark_t   qval,
				     gpointer     data,
				     GError     **error)
{
	hg_dict_t *d2 = (hg_dict_t *)data;

	hg_dict_add(d2, qkey, qval);

	return TRUE;
}

/* <any> ... <n> copy <any> ...
 * <array1> <array2> copy <subarray2>
 * <dict1> <dict2> copy <dict2>
 * <string1> <string2> copy <substring2>
 * <packedarray> <array> copy <subarray2>
 * <gstate1> <gstate2> copy <gstate2>
 */
DEFUNC_OPER (copy)
G_STMT_START {
	hg_quark_t arg0, arg1, q = Qnil;
	gsize i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0)) {
		gsize n = HG_INT (arg0);

		if (n < 0 || n >= hg_stack_depth(ostack)) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			return FALSE;
		}
		hg_stack_pop(ostack, error);
		for (i = 0; i < n; i++) {
			if (!hg_stack_push(ostack, hg_stack_index(ostack, n - 1, error))) {
				hg_vm_set_error(vm, qself, HG_VM_e_stackoverflow);
				return FALSE;
			}
		}
		retval = TRUE;
	} else {
		CHECK_STACK (ostack, 2);

		arg1 = arg0;
		arg0 = hg_stack_index(ostack, 1, error);
		if (!hg_quark_is_readable(arg0) ||
		    !hg_quark_is_writable(arg1)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}
		if (HG_IS_QARRAY (arg0) &&
		    HG_IS_QARRAY (arg1)) {
			hg_array_t *a1, *a2;
			gsize len1, len2;

			a1 = HG_VM_LOCK (vm, arg0, error);
			a2 = HG_VM_LOCK (vm, arg1, error);
			if (a1 == NULL || a2 == NULL) {
				hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
				goto a_error;
			}
			len1 = hg_array_length(a1);
			len2 = hg_array_length(a2);
			if (len1 > len2) {
				hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
				goto a_error;
			}
			for (i = 0; i < len1; i++) {
				hg_quark_t qq;

				qq = hg_array_get(a1, i, error);
				hg_array_set(a2, qq, i, error);
			}
			if (len2 > len1) {
				q = hg_array_make_subarray(a2, 0, len1 - 1, NULL, error);
				if (q == Qnil) {
					hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
					goto a_error;
				}
			} else {
				q = arg1;
			}
			retval = TRUE;
		  a_error:
			if (a1)
				HG_VM_UNLOCK (vm, arg0);
			if (a2)
				HG_VM_UNLOCK (vm, arg1);
		} else if (HG_IS_QDICT (arg0) &&
			   HG_IS_QDICT (arg1)) {
			hg_dict_t *d1, *d2;

			d1 = HG_VM_LOCK (vm, arg0, error);
			d2 = HG_VM_LOCK (vm, arg1, error);
			if (d1 == NULL || d2 == NULL) {
				hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
				goto d_error;
			}
			if (hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1) {
				if (hg_dict_length(d2) != 0 ||
				    hg_dict_maxlength(d1) != hg_dict_maxlength(d2)) {
					hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
					goto d_error;
				}
			}
			hg_dict_foreach(d1, _hg_operator_copy_real_traverse_dict, d2, error);
			q = arg1;

			retval = TRUE;
		  d_error:
			if (d1)
				HG_VM_UNLOCK (vm, arg0);
			if (d2)
				HG_VM_UNLOCK (vm, arg1);
		} else if (HG_IS_QSTRING (arg0) &&
			   HG_IS_QSTRING (arg1)) {
			hg_string_t *s1, *s2;
			gsize len, len1, len2;

			s1 = HG_VM_LOCK (vm, arg0, error);
			s2 = HG_VM_LOCK (vm, arg1, error);
			if (s1 == NULL || s2 == NULL) {
				hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
				goto s_error;
			}
			len = hg_string_length(s1);
			len1 = hg_string_maxlength(s1);
			len2 = hg_string_maxlength(s2);
			if (len1 > len2) {
				hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
				goto s_error;
			}
			for (i = 0; i < len; i++) {
				gchar c = hg_string_index(s1, i);

				hg_string_overwrite_c(s2, c, i, error);
			}
			if (len2 > len1) {
				q = hg_string_make_substring(s2, 0, len1 - 1, NULL, error);
				if (q == Qnil) {
					hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
					goto s_error;
				}
			} else {
				q = arg1;
			}
			retval = TRUE;
		  s_error:
			if (s1)
				HG_VM_UNLOCK (vm, arg0);
			if (s2)
				HG_VM_UNLOCK (vm, arg1);
		} else {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}

		if (retval) {
			hg_stack_pop(ostack, error);
			hg_stack_pop(ostack, error);

			STACK_PUSH (ostack, q);
		}
	}
} G_STMT_END;
DEFUNC_OPER_END

/* - count <int> */
DEFUNC_OPER (count)
G_STMT_START {
	STACK_PUSH (ostack, HG_QINT (hg_stack_depth(ostack)));

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (counttomark);
DEFUNC_UNIMPLEMENTED_OPER (currentcmykcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentdash);
DEFUNC_UNIMPLEMENTED_OPER (currentdict);
DEFUNC_UNIMPLEMENTED_OPER (currentfile);
DEFUNC_UNIMPLEMENTED_OPER (currentfont);
DEFUNC_UNIMPLEMENTED_OPER (currentgray);
DEFUNC_UNIMPLEMENTED_OPER (currentgstate);
DEFUNC_UNIMPLEMENTED_OPER (currenthsbcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentlinecap);
DEFUNC_UNIMPLEMENTED_OPER (currentlinejoin);
DEFUNC_UNIMPLEMENTED_OPER (currentlinewidth);
DEFUNC_UNIMPLEMENTED_OPER (currentmatrix);
DEFUNC_UNIMPLEMENTED_OPER (currentpoint);
DEFUNC_UNIMPLEMENTED_OPER (currentrgbcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentshared);
DEFUNC_UNIMPLEMENTED_OPER (curveto);
DEFUNC_UNIMPLEMENTED_OPER (cvi);
DEFUNC_UNIMPLEMENTED_OPER (cvlit);
DEFUNC_UNIMPLEMENTED_OPER (cvn);
DEFUNC_UNIMPLEMENTED_OPER (cvr);
DEFUNC_UNIMPLEMENTED_OPER (cvrs);
DEFUNC_UNIMPLEMENTED_OPER (cvs);
DEFUNC_UNIMPLEMENTED_OPER (cvx);

/* <key> <value> def - */
DEFUNC_OPER (def)
G_STMT_START {
	hg_quark_t arg0, arg1, qd;
	gboolean is_dict_global;
	hg_dict_t *dict;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	qd = hg_stack_index(dstack, 0, error);
	if (!hg_quark_is_writable(qd)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	is_dict_global = hg_quark_has_same_mem_id(qd, vm->mem_id[HG_VM_MEM_GLOBAL]);
	if (is_dict_global) {
		if (!hg_quark_is_simple_object(arg0) &&
		    hg_quark_has_same_mem_id(arg0, vm->mem_id[HG_VM_MEM_LOCAL])) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}
		if (!hg_quark_is_simple_object(arg1) &&
		    hg_quark_has_same_mem_id(arg1, vm->mem_id[HG_VM_MEM_LOCAL])) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}
	}
	dict = HG_VM_LOCK (vm, qd, error);
	if (dict == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1) {
		if (hg_dict_length(dict) == hg_dict_maxlength(dict) &&
		    hg_dict_lookup(dict, arg0) == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_dictfull);
			goto error;
		}
	}
	retval = hg_dict_add(dict, arg0, arg1);

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);
  error:
	HG_VM_UNLOCK (vm, qd);
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (defineusername);

/* <int> dict <dict> */
DEFUNC_OPER (dict)
G_STMT_START {
	hg_quark_t arg0, ret;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_INT (arg0) > G_MAXUSHORT) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		return FALSE;
	}
	ret = hg_dict_new(hg_vm_get_mem(vm),
			  HG_INT (arg0),
			  NULL);
	if (ret == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, ret);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (div);
DEFUNC_UNIMPLEMENTED_OPER (dtransform);

/* <any> dup <any> */
DEFUNC_OPER (dup)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	STACK_PUSH (ostack, arg0);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (end);
DEFUNC_UNIMPLEMENTED_OPER (eoclip);
DEFUNC_UNIMPLEMENTED_OPER (eofill);
DEFUNC_UNIMPLEMENTED_OPER (eoviewclip);
DEFUNC_UNIMPLEMENTED_OPER (eq);
DEFUNC_UNIMPLEMENTED_OPER (exch);
DEFUNC_UNIMPLEMENTED_OPER (exec);
DEFUNC_UNIMPLEMENTED_OPER (exit);

/* <filename> <access> file -file- */
DEFUNC_OPER (file)
G_STMT_START {
	hg_quark_t arg0, arg1, q;
	hg_file_io_t iotype;
	hg_file_mode_t iomode;
	hg_string_t *fname, *faccess;
	gchar *filename, *fmode;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QSTRING (arg0) ||
	    !HG_IS_QSTRING (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	fname = HG_VM_LOCK (vm, arg0, error);
	faccess = HG_VM_LOCK (vm, arg1, error);
	if (fname == NULL || faccess == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	filename = g_strdup(hg_string_get_static_cstr(fname));
	fmode = g_strdup(hg_string_get_static_cstr(faccess));
	HG_VM_UNLOCK (vm, arg0);
	HG_VM_UNLOCK (vm, arg1);

	iotype = hg_file_get_io_type(filename);
	if (fmode == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	switch (fmode[0]) {
	    case 'r':
		    iomode = HG_FILE_IO_MODE_READ;
		    break;
	    case 'w':
		    iomode = HG_FILE_IO_MODE_WRITE;
		    break;
	    case 'a':
		    iomode = HG_FILE_IO_MODE_APPEND;
		    break;
	    default:
		    hg_vm_set_error(vm, qself, HG_VM_e_invalidfileaccess);
		    goto error;
	}
	if (fmode[1] == '+') {
		switch (iomode) {
		    case HG_FILE_IO_MODE_READ:
		    case HG_FILE_IO_MODE_WRITE:
			    iomode |= HG_FILE_IO_MODE_APPEND;
			    break;
		    case HG_FILE_IO_MODE_APPEND:
			    iomode |= HG_FILE_IO_MODE_READWRITE;
			    break;
		    default:
			    hg_vm_set_error(vm, qself, HG_VM_e_invalidfileaccess);
			    goto error;
		}
	} else if (fmode[1] != 0) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidfileaccess);
		goto error;
	}
	if (iotype != HG_FILE_IO_FILE) {
		q = hg_vm_get_io(vm, iotype);
	} else {
		q = hg_file_new(hg_vm_get_mem(vm),
				filename, iomode, error, NULL);
		if (q == Qnil) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto error;
		}
		hg_quark_set_access_bits(&q,
					 iomode & (HG_FILE_IO_MODE_READ|HG_FILE_IO_MODE_APPEND),
					 iomode & (HG_FILE_IO_MODE_WRITE|HG_FILE_IO_MODE_APPEND),
					 TRUE);
	}
	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	g_free(filename);
	g_free(fmode);
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (fill);
DEFUNC_UNIMPLEMENTED_OPER (findfont);
DEFUNC_UNIMPLEMENTED_OPER (flattenpath);
DEFUNC_UNIMPLEMENTED_OPER (floor);
DEFUNC_UNIMPLEMENTED_OPER (flush);
DEFUNC_UNIMPLEMENTED_OPER (flushfile);
DEFUNC_UNIMPLEMENTED_OPER (for);
DEFUNC_UNIMPLEMENTED_OPER (forall);
DEFUNC_UNIMPLEMENTED_OPER (ge);

/* <array> <index> get <any>
 * <packedarray> <index> get <any>
 * <dict> <key> get <any>
 * <string> <index> get <int>
 */
DEFUNC_OPER (get)
G_STMT_START {
	hg_quark_t arg0, arg1, q = Qnil;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!hg_quark_is_readable(arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_IS_QARRAY (arg0)) {
		hg_array_t *a;
		gssize index, len;

		if (!HG_IS_QINT (arg1)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		a = HG_VM_LOCK (vm, arg0, error);
		if (a == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		index = HG_INT (arg1);
		len = hg_array_length(a);
		if (index < 0 || index >= len) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			goto a_error;
		}
		q = hg_array_get(a, index, error);

		retval = TRUE;
	  a_error:
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QDICT (arg0)) {
		hg_dict_t *d;

		d = HG_VM_LOCK (vm, arg0, error);
		if (d == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		q = hg_dict_lookup(d, arg1);
		HG_VM_UNLOCK (vm, arg0);

		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_undefined);
			return FALSE;
		}
		retval = TRUE;
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;
		gssize index, len;
		gchar c;

		if (!HG_IS_QINT (arg1)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		s = HG_VM_LOCK (vm, arg0, error);
		if (s == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		index = HG_INT (arg1);
		len = hg_string_length(s);
		if (index < 0 || index >= len) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			goto s_error;
		}
		c = hg_string_index(s, index);
		q = HG_QINT (c);

		retval = TRUE;
	  s_error:
		HG_VM_UNLOCK (vm, arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, q);
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (getinterval);
DEFUNC_UNIMPLEMENTED_OPER (grestore);
DEFUNC_UNIMPLEMENTED_OPER (gsave);
DEFUNC_UNIMPLEMENTED_OPER (gstate);
DEFUNC_UNIMPLEMENTED_OPER (gt);
DEFUNC_UNIMPLEMENTED_OPER (identmatrix);

/* <int1> <int2> idiv <quotient> */
DEFUNC_OPER (idiv)
G_STMT_START {
	hg_quark_t arg0, arg1, q;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	q = HG_QINT (HG_INT (arg0) / HG_INT (arg1));

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (idtransform);

/* <bool> <proc> if - */
DEFUNC_OPER (if)
G_STMT_START {
	hg_quark_t arg0, arg1, q;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QBOOL (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!HG_IS_QARRAY (arg1) ||
	    !hg_quark_is_executable(arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_quark_is_readable(arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_BOOL (arg0)) {
		q = hg_vm_quark_copy(vm, arg1, NULL, error);
		if (error && *error) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		STACK_PUSH (estack, q);
		hg_stack_roll(estack, 2, 1, error);
	}
	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

/* <bool> <proc1> <proc2> ifelse - */
DEFUNC_OPER (ifelse)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, q;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QBOOL (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!HG_IS_QARRAY (arg1) ||
	    !hg_quark_is_executable(arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_quark_is_readable(arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (!HG_IS_QARRAY (arg2) ||
	    !hg_quark_is_executable(arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_quark_is_readable(arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_BOOL (arg0)) {
		q = hg_vm_quark_copy(vm, arg1, NULL, error);
	} else {
		q = hg_vm_quark_copy(vm, arg2, NULL, error);
	}
	if (error && *error) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	STACK_PUSH (estack, q);
	hg_stack_roll(estack, 2, 1, error);

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (image);
DEFUNC_UNIMPLEMENTED_OPER (imagemask);

/* <any> ... <n> index <any> ... */
DEFUNC_OPER (index)
G_STMT_START {
	hg_quark_t arg0, q;
	gsize n;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	n = HG_INT (arg0);

	if (n < 0 || (n + 1) >= hg_stack_depth(ostack)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	q = hg_stack_index(ostack, n + 1, error);
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ineofill);
DEFUNC_UNIMPLEMENTED_OPER (infill);
DEFUNC_UNIMPLEMENTED_OPER (initviewclip);
DEFUNC_UNIMPLEMENTED_OPER (inueofill);
DEFUNC_UNIMPLEMENTED_OPER (inufill);
DEFUNC_UNIMPLEMENTED_OPER (invertmatrix);
DEFUNC_UNIMPLEMENTED_OPER (itransform);

/* <dict> <key> known <bool> */
DEFUNC_OPER (known)
G_STMT_START {
	hg_quark_t arg0, arg1;
	hg_dict_t *d;
	gboolean ret;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QDICT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_quark_is_readable(arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	d = HG_VM_LOCK (vm, arg0, error);
	if (d == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	ret = hg_dict_lookup(d, arg1);

	HG_VM_UNLOCK (vm, arg0);

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, HG_QBOOL (ret != Qnil));

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (le);
DEFUNC_UNIMPLEMENTED_OPER (length);
DEFUNC_UNIMPLEMENTED_OPER (lineto);
DEFUNC_UNIMPLEMENTED_OPER (load);
DEFUNC_UNIMPLEMENTED_OPER (loop);
DEFUNC_UNIMPLEMENTED_OPER (lt);
DEFUNC_UNIMPLEMENTED_OPER (makefont);
DEFUNC_UNIMPLEMENTED_OPER (matrix);
DEFUNC_UNIMPLEMENTED_OPER (maxlength);
DEFUNC_UNIMPLEMENTED_OPER (mod);
DEFUNC_UNIMPLEMENTED_OPER (moveto);
DEFUNC_UNIMPLEMENTED_OPER (mul);
DEFUNC_UNIMPLEMENTED_OPER (ne);
DEFUNC_UNIMPLEMENTED_OPER (neg);
DEFUNC_UNIMPLEMENTED_OPER (newpath);

/* <bool> not <bool>
 * <int> not <int>
 */
DEFUNC_OPER (not)
G_STMT_START {
	hg_quark_t arg0, ret;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QBOOL (arg0)) {
		ret = HG_QBOOL (!HG_BOOL (arg0));
	} else if (HG_IS_QINT (arg0)) {
		ret = HG_QINT (-HG_INT (arg0));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, ret);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (or);
DEFUNC_UNIMPLEMENTED_OPER (pathbbox);
DEFUNC_UNIMPLEMENTED_OPER (pathforall);
DEFUNC_UNIMPLEMENTED_OPER (pop);
DEFUNC_UNIMPLEMENTED_OPER (print);
DEFUNC_UNIMPLEMENTED_OPER (printobject);

/* <array> <index> <any> put -
 * <dict> <key> <value> put -
 * <string> <index> <int> put -
 */
DEFUNC_OPER (put)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2;
	gboolean is_in_global;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);
	if (!hg_quark_is_writable(arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	is_in_global = hg_quark_has_same_mem_id(arg2, vm->mem_id[HG_VM_MEM_GLOBAL]);
	if (is_in_global &&
	    !hg_quark_is_simple_object(arg0) &&
	    hg_quark_has_same_mem_id(arg0, vm->mem_id[HG_VM_MEM_LOCAL])) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_IS_QARRAY (arg0)) {
		gsize index;
		hg_array_t *a;

		if (!HG_IS_QINT (arg1)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		a = HG_VM_LOCK (vm, arg0, error);
		if (a == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		index = HG_INT (arg1);
		if (hg_array_length(a) < index) {
			HG_VM_UNLOCK (vm, arg0);
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			return FALSE;
		}
		retval = hg_array_set(a, arg2, index, error);

		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QDICT (arg0)) {
		hg_dict_t *d;

		d = HG_VM_LOCK (vm, arg0, error);
		if (d == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		if (hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1 &&
		    hg_dict_length(d) == hg_dict_maxlength(d) &&
		    hg_dict_lookup(d, arg1) == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_dictfull);
			goto d_error;
		}
		retval = hg_dict_add(d, arg1, arg2);
	  d_error:
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;

		if (!HG_IS_QINT (arg1) ||
		    !HG_IS_QINT (arg2)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		s = HG_VM_LOCK (vm, arg0, error);
		if (s == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		retval = hg_string_overwrite_c(s, arg2, arg1, error);

		HG_VM_UNLOCK (vm, arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (putinterval);
DEFUNC_UNIMPLEMENTED_OPER (rcurveto);
DEFUNC_UNIMPLEMENTED_OPER (read);
DEFUNC_UNIMPLEMENTED_OPER (readhexstring);
DEFUNC_UNIMPLEMENTED_OPER (readline);
DEFUNC_UNIMPLEMENTED_OPER (readstring);
DEFUNC_UNIMPLEMENTED_OPER (rectclip);
DEFUNC_UNIMPLEMENTED_OPER (rectfill);
DEFUNC_UNIMPLEMENTED_OPER (rectstroke);
DEFUNC_UNIMPLEMENTED_OPER (rectviewclip);
DEFUNC_UNIMPLEMENTED_OPER (repeat);
DEFUNC_UNIMPLEMENTED_OPER (restore);
DEFUNC_UNIMPLEMENTED_OPER (rlineto);
DEFUNC_UNIMPLEMENTED_OPER (rmoveto);

/* <any> ... <n> <j> roll <any> ... */
DEFUNC_OPER (roll)
G_STMT_START {
	hg_quark_t arg0, arg1;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_INT (arg0) < 0) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	CHECK_STACK (ostack, HG_INT (arg0) + 2);

	hg_stack_pop(ostack, error);
	hg_stack_pop(ostack, error);

	hg_stack_roll(ostack, HG_INT (arg0), HG_INT (arg1), error);

	retval = TRUE;
} G_STMT_END;
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (rotate);
DEFUNC_UNIMPLEMENTED_OPER (round);
DEFUNC_UNIMPLEMENTED_OPER (save);
DEFUNC_UNIMPLEMENTED_OPER (scale);
DEFUNC_UNIMPLEMENTED_OPER (scalefont);
DEFUNC_UNIMPLEMENTED_OPER (search);
DEFUNC_UNIMPLEMENTED_OPER (selectfont);
DEFUNC_UNIMPLEMENTED_OPER (setbbox);
DEFUNC_UNIMPLEMENTED_OPER (setcachedevice);
DEFUNC_UNIMPLEMENTED_OPER (setcachedevice2);
DEFUNC_UNIMPLEMENTED_OPER (setcharwidth);
DEFUNC_UNIMPLEMENTED_OPER (setcmykcolor);
DEFUNC_UNIMPLEMENTED_OPER (setdash);
DEFUNC_UNIMPLEMENTED_OPER (setfont);
DEFUNC_UNIMPLEMENTED_OPER (setgray);
DEFUNC_UNIMPLEMENTED_OPER (setgstate);
DEFUNC_UNIMPLEMENTED_OPER (sethsbcolor);
DEFUNC_UNIMPLEMENTED_OPER (setlinecap);
DEFUNC_UNIMPLEMENTED_OPER (setlinejoin);
DEFUNC_UNIMPLEMENTED_OPER (setlinewidth);
DEFUNC_UNIMPLEMENTED_OPER (setmatrix);
DEFUNC_UNIMPLEMENTED_OPER (setrgbcolor);
DEFUNC_UNIMPLEMENTED_OPER (setshared);
DEFUNC_UNIMPLEMENTED_OPER (shareddict);
DEFUNC_UNIMPLEMENTED_OPER (show);
DEFUNC_UNIMPLEMENTED_OPER (showpage);
DEFUNC_UNIMPLEMENTED_OPER (stop);
DEFUNC_UNIMPLEMENTED_OPER (stopped);
DEFUNC_UNIMPLEMENTED_OPER (store);
DEFUNC_UNIMPLEMENTED_OPER (string);
DEFUNC_UNIMPLEMENTED_OPER (stringwidth);
DEFUNC_UNIMPLEMENTED_OPER (stroke);
DEFUNC_UNIMPLEMENTED_OPER (strokepath);
DEFUNC_UNIMPLEMENTED_OPER (sub);
DEFUNC_UNIMPLEMENTED_OPER (token);
DEFUNC_UNIMPLEMENTED_OPER (transform);
DEFUNC_UNIMPLEMENTED_OPER (translate);
DEFUNC_UNIMPLEMENTED_OPER (truncate);
DEFUNC_UNIMPLEMENTED_OPER (type);
DEFUNC_UNIMPLEMENTED_OPER (uappend);
DEFUNC_UNIMPLEMENTED_OPER (ucache);
DEFUNC_UNIMPLEMENTED_OPER (ueofill);
DEFUNC_UNIMPLEMENTED_OPER (ufill);
DEFUNC_UNIMPLEMENTED_OPER (undef);
DEFUNC_UNIMPLEMENTED_OPER (upath);
DEFUNC_UNIMPLEMENTED_OPER (userdict);
DEFUNC_UNIMPLEMENTED_OPER (ustroke);
DEFUNC_UNIMPLEMENTED_OPER (viewclip);
DEFUNC_UNIMPLEMENTED_OPER (viewclippath);
DEFUNC_UNIMPLEMENTED_OPER (where);
DEFUNC_UNIMPLEMENTED_OPER (widthshow);
DEFUNC_UNIMPLEMENTED_OPER (write);
DEFUNC_UNIMPLEMENTED_OPER (writehexstring);
DEFUNC_UNIMPLEMENTED_OPER (writeobject);
DEFUNC_UNIMPLEMENTED_OPER (writestring);
DEFUNC_UNIMPLEMENTED_OPER (wtranslation);
DEFUNC_UNIMPLEMENTED_OPER (xor);
DEFUNC_UNIMPLEMENTED_OPER (xshow);
DEFUNC_UNIMPLEMENTED_OPER (xyshow);
DEFUNC_UNIMPLEMENTED_OPER (yshow);
DEFUNC_UNIMPLEMENTED_OPER (FontDirectory);
DEFUNC_UNIMPLEMENTED_OPER (SharedFontDirectory);
DEFUNC_UNIMPLEMENTED_OPER (execuserobject);
DEFUNC_UNIMPLEMENTED_OPER (currentcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentcolorspace);
DEFUNC_UNIMPLEMENTED_OPER (currentglobal);
DEFUNC_UNIMPLEMENTED_OPER (execform);
DEFUNC_UNIMPLEMENTED_OPER (filter);
DEFUNC_UNIMPLEMENTED_OPER (findresource);
DEFUNC_UNIMPLEMENTED_OPER (makepattern);
DEFUNC_UNIMPLEMENTED_OPER (setcolor);
DEFUNC_UNIMPLEMENTED_OPER (setcolorspace);
DEFUNC_UNIMPLEMENTED_OPER (setglobal);
DEFUNC_UNIMPLEMENTED_OPER (setpagedevice);
DEFUNC_UNIMPLEMENTED_OPER (setpattern);
DEFUNC_UNIMPLEMENTED_OPER (sym_eq);
DEFUNC_UNIMPLEMENTED_OPER (sym_eqeq);
DEFUNC_UNIMPLEMENTED_OPER (ISOLatin1Encoding);
DEFUNC_UNIMPLEMENTED_OPER (StandardEncoding);
DEFUNC_UNIMPLEMENTED_OPER (sym_left_square_bracket);
DEFUNC_UNIMPLEMENTED_OPER (sym_right_square_bracket);
DEFUNC_UNIMPLEMENTED_OPER (atan);
DEFUNC_UNIMPLEMENTED_OPER (banddevice);
DEFUNC_UNIMPLEMENTED_OPER (bytesavailable);
DEFUNC_UNIMPLEMENTED_OPER (cachestatus);
DEFUNC_UNIMPLEMENTED_OPER (closefile);
DEFUNC_UNIMPLEMENTED_OPER (colorimage);
DEFUNC_UNIMPLEMENTED_OPER (condition);
DEFUNC_UNIMPLEMENTED_OPER (copypage);
DEFUNC_UNIMPLEMENTED_OPER (cos);
DEFUNC_UNIMPLEMENTED_OPER (countdictstack);
DEFUNC_UNIMPLEMENTED_OPER (countexecstack);
DEFUNC_UNIMPLEMENTED_OPER (cshow);
DEFUNC_UNIMPLEMENTED_OPER (currentblackgeneration);
DEFUNC_UNIMPLEMENTED_OPER (currentcacheparams);
DEFUNC_UNIMPLEMENTED_OPER (currentcolorscreen);
DEFUNC_UNIMPLEMENTED_OPER (currentcolortransfer);
DEFUNC_UNIMPLEMENTED_OPER (currentcontext);
DEFUNC_UNIMPLEMENTED_OPER (currentflat);
DEFUNC_UNIMPLEMENTED_OPER (currenthalftone);
DEFUNC_UNIMPLEMENTED_OPER (currenthalftonephase);
DEFUNC_UNIMPLEMENTED_OPER (currentmiterlimit);
DEFUNC_UNIMPLEMENTED_OPER (currentobjectformat);
DEFUNC_UNIMPLEMENTED_OPER (currentpacking);
DEFUNC_UNIMPLEMENTED_OPER (currentscreen);
DEFUNC_UNIMPLEMENTED_OPER (currentstrokeadjust);
DEFUNC_UNIMPLEMENTED_OPER (currenttransfer);
DEFUNC_UNIMPLEMENTED_OPER (currentundercolorremoval);
DEFUNC_UNIMPLEMENTED_OPER (defaultmatrix);
DEFUNC_UNIMPLEMENTED_OPER (definefont);
DEFUNC_UNIMPLEMENTED_OPER (deletefile);
DEFUNC_UNIMPLEMENTED_OPER (detach);
DEFUNC_UNIMPLEMENTED_OPER (deviceinfo);
DEFUNC_UNIMPLEMENTED_OPER (dictstack);
DEFUNC_UNIMPLEMENTED_OPER (echo);
DEFUNC_UNIMPLEMENTED_OPER (erasepage);
DEFUNC_UNIMPLEMENTED_OPER (execstack);
DEFUNC_UNIMPLEMENTED_OPER (executeonly);
DEFUNC_UNIMPLEMENTED_OPER (exp);
DEFUNC_UNIMPLEMENTED_OPER (filenameforall);
DEFUNC_UNIMPLEMENTED_OPER (fileposition);
DEFUNC_UNIMPLEMENTED_OPER (fork);
DEFUNC_UNIMPLEMENTED_OPER (framedevice);
DEFUNC_UNIMPLEMENTED_OPER (grestoreall);
DEFUNC_UNIMPLEMENTED_OPER (handleerror);
DEFUNC_UNIMPLEMENTED_OPER (initclip);
DEFUNC_UNIMPLEMENTED_OPER (initgraphics);
DEFUNC_UNIMPLEMENTED_OPER (initmatrix);
DEFUNC_UNIMPLEMENTED_OPER (instroke);
DEFUNC_UNIMPLEMENTED_OPER (inustroke);
DEFUNC_UNIMPLEMENTED_OPER (join);
DEFUNC_UNIMPLEMENTED_OPER (kshow);
DEFUNC_UNIMPLEMENTED_OPER (ln);
DEFUNC_UNIMPLEMENTED_OPER (lock);
DEFUNC_UNIMPLEMENTED_OPER (log);
DEFUNC_UNIMPLEMENTED_OPER (monitor);
DEFUNC_UNIMPLEMENTED_OPER (noaccess);
DEFUNC_UNIMPLEMENTED_OPER (notify);
DEFUNC_UNIMPLEMENTED_OPER (nulldevice);
DEFUNC_UNIMPLEMENTED_OPER (packedarray);
DEFUNC_UNIMPLEMENTED_OPER (quit);
DEFUNC_UNIMPLEMENTED_OPER (rand);
DEFUNC_UNIMPLEMENTED_OPER (rcheck);
DEFUNC_UNIMPLEMENTED_OPER (readonly);
DEFUNC_UNIMPLEMENTED_OPER (realtime);
DEFUNC_UNIMPLEMENTED_OPER (renamefile);
DEFUNC_UNIMPLEMENTED_OPER (renderbands);
DEFUNC_UNIMPLEMENTED_OPER (resetfile);
DEFUNC_UNIMPLEMENTED_OPER (reversepath);
DEFUNC_UNIMPLEMENTED_OPER (rootfont);
DEFUNC_UNIMPLEMENTED_OPER (rrand);
DEFUNC_UNIMPLEMENTED_OPER (run);
DEFUNC_UNIMPLEMENTED_OPER (scheck);
DEFUNC_UNIMPLEMENTED_OPER (setblackgeneration);
DEFUNC_UNIMPLEMENTED_OPER (setcachelimit);
DEFUNC_UNIMPLEMENTED_OPER (setcacheparams);
DEFUNC_UNIMPLEMENTED_OPER (setcolorscreen);
DEFUNC_UNIMPLEMENTED_OPER (setcolortransfer);
DEFUNC_UNIMPLEMENTED_OPER (setfileposition);
DEFUNC_UNIMPLEMENTED_OPER (setflat);
DEFUNC_UNIMPLEMENTED_OPER (sethalftone);
DEFUNC_UNIMPLEMENTED_OPER (sethalftonephase);
DEFUNC_UNIMPLEMENTED_OPER (setmiterlimit);
DEFUNC_UNIMPLEMENTED_OPER (setobjectformat);
DEFUNC_UNIMPLEMENTED_OPER (setpacking);
DEFUNC_UNIMPLEMENTED_OPER (setscreen);
DEFUNC_UNIMPLEMENTED_OPER (setstrokeadjust);
DEFUNC_UNIMPLEMENTED_OPER (settransfer);
DEFUNC_UNIMPLEMENTED_OPER (setucacheparams);
DEFUNC_UNIMPLEMENTED_OPER (setundercolorremoval);
DEFUNC_UNIMPLEMENTED_OPER (sin);
DEFUNC_UNIMPLEMENTED_OPER (sqrt);
DEFUNC_UNIMPLEMENTED_OPER (srand);
DEFUNC_UNIMPLEMENTED_OPER (stack);
DEFUNC_UNIMPLEMENTED_OPER (status);
DEFUNC_UNIMPLEMENTED_OPER (statusdict);
DEFUNC_UNIMPLEMENTED_OPER (ucachestatus);
DEFUNC_UNIMPLEMENTED_OPER (undefinefont);
DEFUNC_UNIMPLEMENTED_OPER (usertime);
DEFUNC_UNIMPLEMENTED_OPER (ustrokepath);
DEFUNC_UNIMPLEMENTED_OPER (version);
DEFUNC_UNIMPLEMENTED_OPER (vmreclaim);
DEFUNC_UNIMPLEMENTED_OPER (vmstatus);
DEFUNC_UNIMPLEMENTED_OPER (wait);
DEFUNC_UNIMPLEMENTED_OPER (wcheck);
DEFUNC_UNIMPLEMENTED_OPER (xcheck);
DEFUNC_UNIMPLEMENTED_OPER (yield);
DEFUNC_UNIMPLEMENTED_OPER (defineuserobject);
DEFUNC_UNIMPLEMENTED_OPER (undefineuserobject);
DEFUNC_UNIMPLEMENTED_OPER (UserObjects);
DEFUNC_UNIMPLEMENTED_OPER (cleardictstack);
DEFUNC_UNIMPLEMENTED_OPER (setvmthreshold);
DEFUNC_UNIMPLEMENTED_OPER (sym_begin_dict_mark);
DEFUNC_UNIMPLEMENTED_OPER (sym_end_dict_mark);
DEFUNC_UNIMPLEMENTED_OPER (currentcolorrendering);
DEFUNC_UNIMPLEMENTED_OPER (currentdevparams);
DEFUNC_UNIMPLEMENTED_OPER (currentoverprint);
DEFUNC_UNIMPLEMENTED_OPER (currentpagedevice);
DEFUNC_UNIMPLEMENTED_OPER (currentsystemparams);
DEFUNC_UNIMPLEMENTED_OPER (currentuserparams);
DEFUNC_UNIMPLEMENTED_OPER (defineresource);
DEFUNC_UNIMPLEMENTED_OPER (findencoding);
DEFUNC_UNIMPLEMENTED_OPER (gcheck);
DEFUNC_UNIMPLEMENTED_OPER (glyphshow);
DEFUNC_UNIMPLEMENTED_OPER (languagelevel);
DEFUNC_UNIMPLEMENTED_OPER (product);
DEFUNC_UNIMPLEMENTED_OPER (pstack);
DEFUNC_UNIMPLEMENTED_OPER (resourceforall);
DEFUNC_UNIMPLEMENTED_OPER (resourcestatus);
DEFUNC_UNIMPLEMENTED_OPER (revision);
DEFUNC_UNIMPLEMENTED_OPER (serialnumber);
DEFUNC_UNIMPLEMENTED_OPER (setcolorrendering);
DEFUNC_UNIMPLEMENTED_OPER (setdevparams);
DEFUNC_UNIMPLEMENTED_OPER (setoverprint);
DEFUNC_UNIMPLEMENTED_OPER (setsystemparams);
DEFUNC_UNIMPLEMENTED_OPER (setuserparams);
DEFUNC_UNIMPLEMENTED_OPER (startjob);
DEFUNC_UNIMPLEMENTED_OPER (undefineresource);
DEFUNC_UNIMPLEMENTED_OPER (GlobalFontDirectory);
DEFUNC_UNIMPLEMENTED_OPER (ASCII85Decode);
DEFUNC_UNIMPLEMENTED_OPER (ASCII85Encode);
DEFUNC_UNIMPLEMENTED_OPER (ASCIIHexDecode);
DEFUNC_UNIMPLEMENTED_OPER (ASCIIHexEncode);
DEFUNC_UNIMPLEMENTED_OPER (CCITTFaxDecode);
DEFUNC_UNIMPLEMENTED_OPER (CCITTFaxEncode);
DEFUNC_UNIMPLEMENTED_OPER (DCTDecode);
DEFUNC_UNIMPLEMENTED_OPER (DCTEncode);
DEFUNC_UNIMPLEMENTED_OPER (LZWDecode);
DEFUNC_UNIMPLEMENTED_OPER (LZWEncode);
DEFUNC_UNIMPLEMENTED_OPER (NullEncode);
DEFUNC_UNIMPLEMENTED_OPER (RunLengthDecode);
DEFUNC_UNIMPLEMENTED_OPER (RunLengthEncode);
DEFUNC_UNIMPLEMENTED_OPER (SubFileDecode);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedA);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedABC);
DEFUNC_UNIMPLEMENTED_OPER (DeviceCMYK);
DEFUNC_UNIMPLEMENTED_OPER (DeviceGray);
DEFUNC_UNIMPLEMENTED_OPER (DeviceRGB);
DEFUNC_UNIMPLEMENTED_OPER (Indexed);
DEFUNC_UNIMPLEMENTED_OPER (Pattern);
DEFUNC_UNIMPLEMENTED_OPER (Separation);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedDEF);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedDEFG);
DEFUNC_UNIMPLEMENTED_OPER (DeviceN);

#undef PROTO_OPER
#undef DEFUNC_OPER
#undef DEFUNC_OPER_END
#undef DEFUNC_UNIMPLEMENTED_OPER

#define REG_OPER(_d_,_n_,_o_)						\
	G_STMT_START {							\
		hg_quark_t __o_name__ = hg_name_new_with_encoding((_n_),	\
								  HG_enc_ ## _o_); \
		hg_quark_t __op__ = HG_QOPER (HG_enc_ ## _o_);		\
									\
		hg_quark_set_executable(&__op__, TRUE);			\
		if (!hg_dict_add((_d_),					\
				 __o_name__,				\
				 __op__))				\
			return FALSE;					\
	} G_STMT_END
#define REG_PRIV_OPER(_d_,_n_,_k_,_o_)					\
	G_STMT_START {							\
		hg_quark_t __o_name__ = HG_QNAME ((_n_),#_k_);		\
		hg_quark_t __op__ = HG_QOPER (HG_enc_ ## _o_);		\
									\
		hg_quark_set_executable(&__op__, TRUE);			\
		if (!hg_dict_add((_d_),					\
				 __o_name__,				\
				 __op__))				\
			return FALSE;					\
	} G_STMT_END
#define REG_VALUE(_d_,_n_,_k_,_v_)				\
	G_STMT_START {						\
		hg_quark_t __o_name__ = HG_QNAME ((_n_),#_k_);	\
								\
		if (!hg_dict_add((_d_),				\
				 __o_name__,			\
				 (_v_)))			\
			return FALSE;				\
	} G_STMT_END

static gboolean
_hg_operator_level1_register(hg_dict_t *dict,
			     hg_name_t *name)
{
	REG_VALUE (dict, name, false, HG_QBOOL (FALSE));
	REG_VALUE (dict, name, mark, HG_QMARK);
	REG_VALUE (dict, name, null, HG_QNULL);
	REG_VALUE (dict, name, true, HG_QBOOL (TRUE));

	REG_PRIV_OPER (dict, name, .forceput, private_forceput);
	REG_PRIV_OPER (dict, name, .odef, private_odef);
	REG_PRIV_OPER (dict, name, .setglobal, private_setglobal);
	REG_PRIV_OPER (dict, name, .undef, private_undef);

	REG_OPER (dict, name, abs);
	REG_OPER (dict, name, add);
	REG_OPER (dict, name, aload);
	REG_OPER (dict, name, anchorsearch);
	REG_OPER (dict, name, and);
	REG_OPER (dict, name, arc);
	REG_OPER (dict, name, arcn);
	REG_OPER (dict, name, arcto);
	REG_OPER (dict, name, array);
	REG_OPER (dict, name, ashow);
	REG_OPER (dict, name, astore);
	REG_OPER (dict, name, awidthshow);
	REG_OPER (dict, name, begin);
	REG_OPER (dict, name, bind);
	REG_OPER (dict, name, bitshift);
	REG_OPER (dict, name, ceiling);
	REG_OPER (dict, name, charpath);
	REG_OPER (dict, name, clear);
	REG_OPER (dict, name, cleartomark);
	REG_OPER (dict, name, clip);
	REG_OPER (dict, name, clippath);
	REG_OPER (dict, name, closepath);
	REG_OPER (dict, name, concat);
	REG_OPER (dict, name, concatmatrix);
	REG_OPER (dict, name, copy);
	REG_OPER (dict, name, count);
	REG_OPER (dict, name, counttomark);
	REG_OPER (dict, name, currentdash);
	REG_OPER (dict, name, currentdict);
	REG_OPER (dict, name, currentfile);
	REG_OPER (dict, name, currentfont);
	REG_OPER (dict, name, currentgray);
	REG_OPER (dict, name, currenthsbcolor);
	REG_OPER (dict, name, currentlinecap);
	REG_OPER (dict, name, currentlinejoin);
	REG_OPER (dict, name, currentlinewidth);
	REG_OPER (dict, name, currentmatrix);
	REG_OPER (dict, name, currentpoint);
	REG_OPER (dict, name, currentrgbcolor);
	REG_OPER (dict, name, curveto);
	REG_OPER (dict, name, cvi);
	REG_OPER (dict, name, cvlit);
	REG_OPER (dict, name, cvn);
	REG_OPER (dict, name, cvr);
	REG_OPER (dict, name, cvrs);
	REG_OPER (dict, name, cvs);
	REG_OPER (dict, name, cvx);
	REG_OPER (dict, name, def);
//	REG_OPER (dict, name, defineusername); /* ??? */
	REG_OPER (dict, name, dict);
	REG_OPER (dict, name, div);
	REG_OPER (dict, name, dtransform);
	REG_OPER (dict, name, dup);
	REG_OPER (dict, name, end);
	REG_OPER (dict, name, eoclip);
	REG_OPER (dict, name, eofill);
//	REG_OPER (dict, name, eoviewclip); /* ??? */
	REG_OPER (dict, name, eq);
	REG_OPER (dict, name, exch);
	REG_OPER (dict, name, exec);
	REG_OPER (dict, name, exit);
	REG_OPER (dict, name, file);
	REG_OPER (dict, name, fill);
	REG_OPER (dict, name, findfont);
	REG_OPER (dict, name, flattenpath);
	REG_OPER (dict, name, floor);
	REG_OPER (dict, name, flush);
	REG_OPER (dict, name, flushfile);
	REG_OPER (dict, name, for);
	REG_OPER (dict, name, forall);
	REG_OPER (dict, name, ge);
	REG_OPER (dict, name, get);
	REG_OPER (dict, name, getinterval);
	REG_OPER (dict, name, grestore);
	REG_OPER (dict, name, gsave);
	REG_OPER (dict, name, gt);
	REG_OPER (dict, name, identmatrix);
	REG_OPER (dict, name, idiv);
	REG_OPER (dict, name, idtransform);
	REG_OPER (dict, name, if);
	REG_OPER (dict, name, ifelse);
	REG_OPER (dict, name, image);
	REG_OPER (dict, name, imagemask);
	REG_OPER (dict, name, index);
//	REG_OPER (dict, name, initviewclip); /* ??? */
	REG_OPER (dict, name, invertmatrix);
	REG_OPER (dict, name, itransform);
	REG_OPER (dict, name, known);
	REG_OPER (dict, name, le);
	REG_OPER (dict, name, length);
	REG_OPER (dict, name, lineto);
	REG_OPER (dict, name, load);
	REG_OPER (dict, name, loop);
	REG_OPER (dict, name, lt);
	REG_OPER (dict, name, makefont);
	REG_OPER (dict, name, matrix);
	REG_OPER (dict, name, maxlength);
	REG_OPER (dict, name, mod);
	REG_OPER (dict, name, moveto);
	REG_OPER (dict, name, mul);
	REG_OPER (dict, name, ne);
	REG_OPER (dict, name, neg);
	REG_OPER (dict, name, newpath);
	REG_OPER (dict, name, not);
	REG_OPER (dict, name, or);
	REG_OPER (dict, name, pathbbox);
	REG_OPER (dict, name, pathforall);
	REG_OPER (dict, name, pop);
	REG_OPER (dict, name, print);
	REG_OPER (dict, name, put);
	REG_OPER (dict, name, putinterval);
	REG_OPER (dict, name, rcurveto);
	REG_OPER (dict, name, read);
	REG_OPER (dict, name, readhexstring);
	REG_OPER (dict, name, readline);
	REG_OPER (dict, name, readstring);
//	REG_OPER (dict, name, rectviewclip); /* ??? */
	REG_OPER (dict, name, repeat);
	REG_OPER (dict, name, restore);
	REG_OPER (dict, name, rlineto);
	REG_OPER (dict, name, rmoveto);
	REG_OPER (dict, name, roll);
	REG_OPER (dict, name, rotate);
	REG_OPER (dict, name, round);
	REG_OPER (dict, name, save);
	REG_OPER (dict, name, scale);
	REG_OPER (dict, name, scalefont);
	REG_OPER (dict, name, search);
	REG_OPER (dict, name, setcachedevice);
	REG_OPER (dict, name, setcharwidth);
	REG_OPER (dict, name, setdash);
	REG_OPER (dict, name, setfont);
	REG_OPER (dict, name, setgray);
	REG_OPER (dict, name, setgstate);
	REG_OPER (dict, name, sethsbcolor);
	REG_OPER (dict, name, setlinecap);
	REG_OPER (dict, name, setlinejoin);
	REG_OPER (dict, name, setlinewidth);
	REG_OPER (dict, name, setmatrix);
	REG_OPER (dict, name, setrgbcolor);
	REG_OPER (dict, name, show);
	REG_OPER (dict, name, showpage);
	REG_OPER (dict, name, stop);
	REG_OPER (dict, name, stopped);
	REG_OPER (dict, name, store);
	REG_OPER (dict, name, string);
	REG_OPER (dict, name, stringwidth);
	REG_OPER (dict, name, stroke);
	REG_OPER (dict, name, strokepath);
	REG_OPER (dict, name, sub);
	REG_OPER (dict, name, token);
	REG_OPER (dict, name, transform);
	REG_OPER (dict, name, translate);
	REG_OPER (dict, name, truncate);
	REG_OPER (dict, name, type);
	REG_OPER (dict, name, userdict);
//	REG_OPER (dict, name, viewclip); /* ??? */
//	REG_OPER (dict, name, viewclippath); /* ??? */
	REG_OPER (dict, name, where);
	REG_OPER (dict, name, widthshow);
	REG_OPER (dict, name, write);
	REG_OPER (dict, name, writehexstring);
	REG_OPER (dict, name, writestring);
//	REG_OPER (dict, name, wtranslation); /* ??? */
	REG_OPER (dict, name, xor);

	REG_OPER (dict, name, FontDirectory);
	REG_OPER (dict, name, sym_eq);
	REG_OPER (dict, name, sym_eqeq);

//	REG_OPER (dict, name, ISOLatin1Encoding);
//	REG_OPER (dict, name, StandardEncoding);

	REG_OPER (dict, name, sym_left_square_bracket);
	REG_OPER (dict, name, sym_right_square_bracket);
	REG_OPER (dict, name, atan);
//	REG_OPER (dict, name, banddevice); /* ??? */
	REG_OPER (dict, name, bytesavailable);
	REG_OPER (dict, name, cachestatus);
	REG_OPER (dict, name, closefile);
//	REG_OPER (dict, name, condition); /* ??? */
	REG_OPER (dict, name, copypage);

	REG_OPER (dict, name, cos);
	REG_OPER (dict, name, countdictstack);
	REG_OPER (dict, name, countexecstack);
//	REG_OPER (dict, name, currentcontext); /* ??? */
	REG_OPER (dict, name, currentflat);

//	REG_OPER (dict, name, currenthalftonephase); /* ??? */
	REG_OPER (dict, name, currentmiterlimit);
	REG_OPER (dict, name, currentscreen);
	REG_OPER (dict, name, currenttransfer);
	REG_OPER (dict, name, defaultmatrix);

	REG_OPER (dict, name, definefont);
//	REG_OPER (dict, name, detach); /* ??? */
//	REG_OPER (dict, name, deviceinfo); /* ??? */
	REG_OPER (dict, name, dictstack);
	REG_OPER (dict, name, echo);
	REG_OPER (dict, name, erasepage);
	REG_OPER (dict, name, execstack);
	REG_OPER (dict, name, executeonly);

	REG_OPER (dict, name, exp);
//	REG_OPER (dict, name, fork); /* ??? */
//	REG_OPER (dict, name, framedevice); /* ??? */
	REG_OPER (dict, name, grestoreall);
	REG_OPER (dict, name, handleerror);
	REG_OPER (dict, name, initclip);
	REG_OPER (dict, name, initgraphics);

	REG_OPER (dict, name, initmatrix);
//	REG_OPER (dict, name, join); /* ??? */
	REG_OPER (dict, name, kshow);
	REG_OPER (dict, name, ln);
//	REG_OPER (dict, name, lock); /* ??? */
	REG_OPER (dict, name, log);
//	REG_OPER (dict, name, monitor); /* ??? */

	REG_OPER (dict, name, noaccess);
//	REG_OPER (dict, name, notify); /* ??? */
	REG_OPER (dict, name, nulldevice);
	REG_OPER (dict, name, packedarray);
	REG_OPER (dict, name, quit);
	REG_OPER (dict, name, rand);
	REG_OPER (dict, name, rcheck);
	REG_OPER (dict, name, readonly);

//	REG_OPER (dict, name, renderbands); /* ??? */
	REG_OPER (dict, name, resetfile);
	REG_OPER (dict, name, reversepath);
	REG_OPER (dict, name, rrand);
	REG_OPER (dict, name, run);
	REG_OPER (dict, name, setcachelimit);
	REG_OPER (dict, name, setflat);
//	REG_OPER (dict, name, sethalftonephase); /* ??? */
	REG_OPER (dict, name, setmiterlimit);
	REG_OPER (dict, name, setscreen);

	REG_OPER (dict, name, settransfer);
	REG_OPER (dict, name, sin);
	REG_OPER (dict, name, sqrt);
	REG_OPER (dict, name, srand);
	REG_OPER (dict, name, stack);
	REG_OPER (dict, name, status);
	REG_OPER (dict, name, statusdict);

	REG_OPER (dict, name, usertime);
	REG_OPER (dict, name, version);
	REG_OPER (dict, name, vmstatus);
//	REG_OPER (dict, name, wait); /* ??? */
	REG_OPER (dict, name, wcheck);

	REG_OPER (dict, name, xcheck);
//	REG_OPER (dict, name, yield); /* ??? */
	REG_OPER (dict, name, cleardictstack);

	REG_OPER (dict, name, sym_begin_dict_mark);

	REG_OPER (dict, name, sym_end_dict_mark);


	REG_OPER (dict, name, pstack);

#if 0
	REG_OPER (dict, name, ASCII85Decode);
	REG_OPER (dict, name, ASCII85Encode);
	REG_OPER (dict, name, ASCIIHexDecode);
	REG_OPER (dict, name, ASCIIHexEncode);

	REG_OPER (dict, name, CCITTFaxDecode);
	REG_OPER (dict, name, CCITTFaxEncode);
	REG_OPER (dict, name, DCTDecode);
	REG_OPER (dict, name, DCTEncode);
	REG_OPER (dict, name, LZWDecode);
	REG_OPER (dict, name, LZWEncode);
	REG_OPER (dict, name, NullEncode);
	REG_OPER (dict, name, RunLengthDecode);
	REG_OPER (dict, name, RunLengthEncode);
	REG_OPER (dict, name, SubFileDecode);

	REG_OPER (dict, name, CIEBasedA);
	REG_OPER (dict, name, CIEBasedABC);
	REG_OPER (dict, name, DeviceCMYK);
	REG_OPER (dict, name, DeviceGray);
	REG_OPER (dict, name, DeviceRGB);
	REG_OPER (dict, name, Indexed);
	REG_OPER (dict, name, Pattern);
	REG_OPER (dict, name, Separation);
	REG_OPER (dict, name, CIEBasedDEF);
	REG_OPER (dict, name, CIEBasedDEFG);

	REG_OPER (dict, name, DeviceN);
#endif

	return TRUE;
}

static gboolean
_hg_operator_level2_register(hg_dict_t *dict,
			     hg_name_t *name)
{
	REG_OPER (dict, name, arct);
	REG_OPER (dict, name, colorimage);
	REG_OPER (dict, name, currentcmykcolor);
	REG_OPER (dict, name, currentgstate);
	REG_OPER (dict, name, currentshared);
	REG_OPER (dict, name, gstate);
	REG_OPER (dict, name, ineofill);
	REG_OPER (dict, name, infill);
	REG_OPER (dict, name, inueofill);
	REG_OPER (dict, name, inufill);
	REG_OPER (dict, name, printobject);
	REG_OPER (dict, name, rectclip);
	REG_OPER (dict, name, rectfill);
	REG_OPER (dict, name, rectstroke);
	REG_OPER (dict, name, selectfont);
	REG_OPER (dict, name, setbbox);
	REG_OPER (dict, name, setcachedevice2);
	REG_OPER (dict, name, setcmykcolor);
	REG_OPER (dict, name, setshared);
	REG_OPER (dict, name, shareddict);
	REG_OPER (dict, name, uappend);
	REG_OPER (dict, name, ucache);
	REG_OPER (dict, name, ueofill);
	REG_OPER (dict, name, ufill);
	REG_OPER (dict, name, undef);
	REG_OPER (dict, name, upath);
	REG_OPER (dict, name, ustroke);
	REG_OPER (dict, name, writeobject);
	REG_OPER (dict, name, xshow);
	REG_OPER (dict, name, xyshow);
	REG_OPER (dict, name, yshow);
	REG_OPER (dict, name, SharedFontDirectory);
	REG_OPER (dict, name, execuserobject);
	REG_OPER (dict, name, currentcolor);
	REG_OPER (dict, name, currentcolorspace);
	REG_OPER (dict, name, currentglobal);
	REG_OPER (dict, name, execform);
	REG_OPER (dict, name, filter);
	REG_OPER (dict, name, findresource);

	REG_OPER (dict, name, makepattern);
	REG_OPER (dict, name, setcolor);
	REG_OPER (dict, name, setcolorspace);
	REG_OPER (dict, name, setglobal);
	REG_OPER (dict, name, setpagedevice);
	REG_OPER (dict, name, setpattern);

	REG_OPER (dict, name, cshow);
	REG_OPER (dict, name, currentblackgeneration);
	REG_OPER (dict, name, currentcacheparams);
	REG_OPER (dict, name, currentcolorscreen);
	REG_OPER (dict, name, currentcolortransfer);
	REG_OPER (dict, name, currenthalftone);
	REG_OPER (dict, name, currentobjectformat);
	REG_OPER (dict, name, currentpacking);
	REG_OPER (dict, name, currentstrokeadjust);
	REG_OPER (dict, name, currentundercolorremoval);
	REG_OPER (dict, name, deletefile);
	REG_OPER (dict, name, filenameforall);
	REG_OPER (dict, name, fileposition);
	REG_OPER (dict, name, instroke);
	REG_OPER (dict, name, inustroke);
	REG_OPER (dict, name, realtime);
	REG_OPER (dict, name, renamefile);
	REG_OPER (dict, name, rootfont);
	REG_OPER (dict, name, scheck);
	REG_OPER (dict, name, setblackgeneration);
	REG_OPER (dict, name, setcacheparams);

	REG_OPER (dict, name, setcolorscreen);
	REG_OPER (dict, name, setcolortransfer);
	REG_OPER (dict, name, setfileposition);
	REG_OPER (dict, name, sethalftone);
	REG_OPER (dict, name, setobjectformat);
	REG_OPER (dict, name, setpacking);
	REG_OPER (dict, name, setstrokeadjust);
	REG_OPER (dict, name, setucacheparams);
	REG_OPER (dict, name, setundercolorremoval);
	REG_OPER (dict, name, ucachestatus);
	REG_OPER (dict, name, undefinefont);
	REG_OPER (dict, name, ustrokepath);
	REG_OPER (dict, name, vmreclaim);
	REG_OPER (dict, name, defineuserobject);
	REG_OPER (dict, name, undefineuserobject);
	REG_OPER (dict, name, UserObjects);
	REG_OPER (dict, name, setvmthreshold);
	REG_OPER (dict, name, currentcolorrendering);
	REG_OPER (dict, name, currentdevparams);
	REG_OPER (dict, name, currentoverprint);
	REG_OPER (dict, name, currentpagedevice);
	REG_OPER (dict, name, currentsystemparams);
	REG_OPER (dict, name, currentuserparams);
	REG_OPER (dict, name, defineresource);
	REG_OPER (dict, name, findencoding);
	REG_OPER (dict, name, gcheck);
	REG_OPER (dict, name, glyphshow);
	REG_OPER (dict, name, languagelevel);
	REG_OPER (dict, name, product);
	REG_OPER (dict, name, resourceforall);
	REG_OPER (dict, name, resourcestatus);
	REG_OPER (dict, name, revision);
	REG_OPER (dict, name, serialnumber);
	REG_OPER (dict, name, setcolorrendering);
	REG_OPER (dict, name, setdevparams);

	REG_OPER (dict, name, setoverprint);
	REG_OPER (dict, name, setsystemparams);
	REG_OPER (dict, name, setuserparams);
	REG_OPER (dict, name, startjob);
	REG_OPER (dict, name, undefineresource);
	REG_OPER (dict, name, GlobalFontDirectory);

	return TRUE;
}

static gboolean
_hg_operator_level3_register(hg_dict_t *dict,
			     hg_name_t *name)
{
	return TRUE;
}

#undef REG_OPER

/*< public >*/
/**
 * hg_operator_init:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_operator_init(void)
{
#define DECL_OPER(_n_)							\
	G_STMT_START {							\
		__hg_operator_name_table[HG_enc_ ## _n_] = g_strdup("--" #_n_ "--"); \
		if (__hg_operator_name_table[HG_enc_ ## _n_] == NULL)	\
			return FALSE;					\
		__hg_operator_func_table[HG_enc_ ## _n_] = _hg_operator_real_ ## _n_; \
	} G_STMT_END
#define DECL_PRIV_OPER(_on_,_n_)						\
	G_STMT_START {							\
		__hg_operator_name_table[HG_enc_ ## _n_] = g_strdup("--" #_on_ "--"); \
		if (__hg_operator_name_table[HG_enc_ ## _n_] == NULL)	\
			return FALSE;					\
		__hg_operator_func_table[HG_enc_ ## _n_] = _hg_operator_real_ ## _n_; \
	} G_STMT_END

	DECL_PRIV_OPER (.forceput, private_forceput);
	DECL_PRIV_OPER (.odef, private_odef);
	DECL_PRIV_OPER (.setglobal, private_setglobal);
	DECL_PRIV_OPER (.undef, private_undef);

	DECL_OPER (abs);
	DECL_OPER (add);
	DECL_OPER (aload);
	DECL_OPER (anchorsearch);
	DECL_OPER (and);
	DECL_OPER (arc);
	DECL_OPER (arcn);
	DECL_OPER (arct);
	DECL_OPER (arcto);
	DECL_OPER (array);
	DECL_OPER (ashow);
	DECL_OPER (astore);
	DECL_OPER (awidthshow);
	DECL_OPER (begin);
	DECL_OPER (bind);
	DECL_OPER (bitshift);
	DECL_OPER (ceiling);
	DECL_OPER (charpath);
	DECL_OPER (clear);
	DECL_OPER (cleartomark);
	DECL_OPER (clip);
	DECL_OPER (clippath);
	DECL_OPER (closepath);
	DECL_OPER (concat);
	DECL_OPER (concatmatrix);
	DECL_OPER (copy);
	DECL_OPER (count);
	DECL_OPER (counttomark);
	DECL_OPER (currentcmykcolor);
	DECL_OPER (currentdash);
	DECL_OPER (currentdict);
	DECL_OPER (currentfile);
	DECL_OPER (currentfont);
	DECL_OPER (currentgray);
	DECL_OPER (currentgstate);
	DECL_OPER (currenthsbcolor);
	DECL_OPER (currentlinecap);
	DECL_OPER (currentlinejoin);
	DECL_OPER (currentlinewidth);
	DECL_OPER (currentmatrix);
	DECL_OPER (currentpoint);
	DECL_OPER (currentrgbcolor);
	DECL_OPER (currentshared);
	DECL_OPER (curveto);
	DECL_OPER (cvi);
	DECL_OPER (cvlit);
	DECL_OPER (cvn);
	DECL_OPER (cvr);
	DECL_OPER (cvrs);
	DECL_OPER (cvs);
	DECL_OPER (cvx);
	DECL_OPER (def);
	DECL_OPER (defineusername);
	DECL_OPER (dict);
	DECL_OPER (div);
	DECL_OPER (dtransform);
	DECL_OPER (dup);
	DECL_OPER (end);
	DECL_OPER (eoclip);
	DECL_OPER (eofill);
	DECL_OPER (eoviewclip);
	DECL_OPER (eq);
	DECL_OPER (exch);
	DECL_OPER (exec);
	DECL_OPER (exit);
	DECL_OPER (file);
	DECL_OPER (fill);
	DECL_OPER (findfont);
	DECL_OPER (flattenpath);
	DECL_OPER (floor);
	DECL_OPER (flush);
	DECL_OPER (flushfile);
	DECL_OPER (for);
	DECL_OPER (forall);
	DECL_OPER (ge);
	DECL_OPER (get);
	DECL_OPER (getinterval);
	DECL_OPER (grestore);
	DECL_OPER (gsave);
	DECL_OPER (gstate);
	DECL_OPER (gt);
	DECL_OPER (identmatrix);
	DECL_OPER (idiv);
	DECL_OPER (idtransform);
	DECL_OPER (if);
	DECL_OPER (ifelse);
	DECL_OPER (image);
	DECL_OPER (imagemask);
	DECL_OPER (index);
	DECL_OPER (ineofill);
	DECL_OPER (infill);
	DECL_OPER (initviewclip);
	DECL_OPER (inueofill);
	DECL_OPER (inufill);
	DECL_OPER (invertmatrix);
	DECL_OPER (itransform);
	DECL_OPER (known);
	DECL_OPER (le);
	DECL_OPER (length);
	DECL_OPER (lineto);
	DECL_OPER (load);
	DECL_OPER (loop);
	DECL_OPER (lt);
	DECL_OPER (makefont);
	DECL_OPER (matrix);
	DECL_OPER (maxlength);
	DECL_OPER (mod);
	DECL_OPER (moveto);
	DECL_OPER (mul);
	DECL_OPER (ne);
	DECL_OPER (neg);
	DECL_OPER (newpath);
	DECL_OPER (not);
	DECL_OPER (or);
	DECL_OPER (pathbbox);
	DECL_OPER (pathforall);
	DECL_OPER (pop);
	DECL_OPER (print);
	DECL_OPER (printobject);
	DECL_OPER (put);
	DECL_OPER (putinterval);
	DECL_OPER (rcurveto);
	DECL_OPER (read);
	DECL_OPER (readhexstring);
	DECL_OPER (readline);
	DECL_OPER (readstring);
	DECL_OPER (rectclip);
	DECL_OPER (rectfill);
	DECL_OPER (rectstroke);
	DECL_OPER (rectviewclip);
	DECL_OPER (repeat);
	DECL_OPER (restore);
	DECL_OPER (rlineto);
	DECL_OPER (rmoveto);
	DECL_OPER (roll);
	DECL_OPER (rotate);
	DECL_OPER (round);
	DECL_OPER (save);
	DECL_OPER (scale);
	DECL_OPER (scalefont);
	DECL_OPER (search);
	DECL_OPER (selectfont);
	DECL_OPER (setbbox);
	DECL_OPER (setcachedevice);
	DECL_OPER (setcachedevice2);
	DECL_OPER (setcharwidth);
	DECL_OPER (setcmykcolor);
	DECL_OPER (setdash);
	DECL_OPER (setfont);
	DECL_OPER (setgray);
	DECL_OPER (setgstate);
	DECL_OPER (sethsbcolor);
	DECL_OPER (setlinecap);
	DECL_OPER (setlinejoin);
	DECL_OPER (setlinewidth);
	DECL_OPER (setmatrix);
	DECL_OPER (setrgbcolor);
	DECL_OPER (setshared);
	DECL_OPER (shareddict);
	DECL_OPER (show);
	DECL_OPER (showpage);
	DECL_OPER (stop);
	DECL_OPER (stopped);
	DECL_OPER (store);
	DECL_OPER (string);
	DECL_OPER (stringwidth);
	DECL_OPER (stroke);
	DECL_OPER (strokepath);
	DECL_OPER (sub);
	DECL_OPER (token);
	DECL_OPER (transform);
	DECL_OPER (translate);
	DECL_OPER (truncate);
	DECL_OPER (type);
	DECL_OPER (uappend);
	DECL_OPER (ucache);
	DECL_OPER (ueofill);
	DECL_OPER (ufill);
	DECL_OPER (undef);
	DECL_OPER (upath);
	DECL_OPER (userdict);
	DECL_OPER (ustroke);
	DECL_OPER (viewclip);
	DECL_OPER (viewclippath);
	DECL_OPER (where);
	DECL_OPER (widthshow);
	DECL_OPER (write);
	DECL_OPER (writehexstring);
	DECL_OPER (writeobject);
	DECL_OPER (writestring);
	DECL_OPER (wtranslation);
	DECL_OPER (xor);
	DECL_OPER (xshow);
	DECL_OPER (xyshow);
	DECL_OPER (yshow);
	DECL_OPER (FontDirectory);
	DECL_OPER (SharedFontDirectory);
	DECL_OPER (execuserobject);
	DECL_OPER (currentcolor);
	DECL_OPER (currentcolorspace);
	DECL_OPER (currentglobal);
	DECL_OPER (execform);
	DECL_OPER (filter);
	DECL_OPER (findresource);
	DECL_OPER (makepattern);
	DECL_OPER (setcolor);
	DECL_OPER (setcolorspace);
	DECL_OPER (setglobal);
	DECL_OPER (setpagedevice);
	DECL_OPER (setpattern);
	DECL_OPER (sym_eq);
	DECL_OPER (sym_eqeq);
	DECL_OPER (ISOLatin1Encoding);
	DECL_OPER (StandardEncoding);
	DECL_OPER (sym_left_square_bracket);
	DECL_OPER (sym_right_square_bracket);
	DECL_OPER (atan);
	DECL_OPER (banddevice);
	DECL_OPER (bytesavailable);
	DECL_OPER (cachestatus);
	DECL_OPER (closefile);
	DECL_OPER (colorimage);
	DECL_OPER (condition);
	DECL_OPER (copypage);
	DECL_OPER (cos);
	DECL_OPER (countdictstack);
	DECL_OPER (countexecstack);
	DECL_OPER (cshow);
	DECL_OPER (currentblackgeneration);
	DECL_OPER (currentcacheparams);
	DECL_OPER (currentcolorscreen);
	DECL_OPER (currentcolortransfer);
	DECL_OPER (currentcontext);
	DECL_OPER (currentflat);
	DECL_OPER (currenthalftone);
	DECL_OPER (currenthalftonephase);
	DECL_OPER (currentmiterlimit);
	DECL_OPER (currentobjectformat);
	DECL_OPER (currentpacking);
	DECL_OPER (currentscreen);
	DECL_OPER (currentstrokeadjust);
	DECL_OPER (currenttransfer);
	DECL_OPER (currentundercolorremoval);
	DECL_OPER (defaultmatrix);
	DECL_OPER (definefont);
	DECL_OPER (deletefile);
	DECL_OPER (detach);
	DECL_OPER (deviceinfo);
	DECL_OPER (dictstack);
	DECL_OPER (echo);
	DECL_OPER (erasepage);
	DECL_OPER (execstack);
	DECL_OPER (executeonly);
	DECL_OPER (exp);
	DECL_OPER (filenameforall);
	DECL_OPER (fileposition);
	DECL_OPER (fork);
	DECL_OPER (framedevice);
	DECL_OPER (grestoreall);
	DECL_OPER (handleerror);
	DECL_OPER (initclip);
	DECL_OPER (initgraphics);
	DECL_OPER (initmatrix);
	DECL_OPER (instroke);
	DECL_OPER (inustroke);
	DECL_OPER (join);
	DECL_OPER (kshow);
	DECL_OPER (ln);
	DECL_OPER (lock);
	DECL_OPER (log);
	DECL_OPER (monitor);
	DECL_OPER (noaccess);
	DECL_OPER (notify);
	DECL_OPER (nulldevice);
	DECL_OPER (packedarray);
	DECL_OPER (quit);
	DECL_OPER (rand);
	DECL_OPER (rcheck);
	DECL_OPER (readonly);
	DECL_OPER (realtime);
	DECL_OPER (renamefile);
	DECL_OPER (renderbands);
	DECL_OPER (resetfile);
	DECL_OPER (reversepath);
	DECL_OPER (rootfont);
	DECL_OPER (rrand);
	DECL_OPER (run);
	DECL_OPER (scheck);
	DECL_OPER (setblackgeneration);
	DECL_OPER (setcachelimit);
	DECL_OPER (setcacheparams);
	DECL_OPER (setcolorscreen);
	DECL_OPER (setcolortransfer);
	DECL_OPER (setfileposition);
	DECL_OPER (setflat);
	DECL_OPER (sethalftone);
	DECL_OPER (sethalftonephase);
	DECL_OPER (setmiterlimit);
	DECL_OPER (setobjectformat);
	DECL_OPER (setpacking);
	DECL_OPER (setscreen);
	DECL_OPER (setstrokeadjust);
	DECL_OPER (settransfer);
	DECL_OPER (setucacheparams);
	DECL_OPER (setundercolorremoval);
	DECL_OPER (sin);
	DECL_OPER (sqrt);
	DECL_OPER (srand);
	DECL_OPER (stack);
	DECL_OPER (status);
	DECL_OPER (statusdict);
	DECL_OPER (ucachestatus);
	DECL_OPER (undefinefont);
	DECL_OPER (usertime);
	DECL_OPER (ustrokepath);
	DECL_OPER (version);
	DECL_OPER (vmreclaim);
	DECL_OPER (vmstatus);
	DECL_OPER (wait);
	DECL_OPER (wcheck);
	DECL_OPER (xcheck);
	DECL_OPER (yield);
	DECL_OPER (defineuserobject);
	DECL_OPER (undefineuserobject);
	DECL_OPER (UserObjects);
	DECL_OPER (cleardictstack);
	DECL_OPER (setvmthreshold);
	DECL_OPER (sym_begin_dict_mark);
	DECL_OPER (sym_end_dict_mark);
	DECL_OPER (currentcolorrendering);
	DECL_OPER (currentdevparams);
	DECL_OPER (currentoverprint);
	DECL_OPER (currentpagedevice);
	DECL_OPER (currentsystemparams);
	DECL_OPER (currentuserparams);
	DECL_OPER (defineresource);
	DECL_OPER (findencoding);
	DECL_OPER (gcheck);
	DECL_OPER (glyphshow);
	DECL_OPER (languagelevel);
	DECL_OPER (product);
	DECL_OPER (pstack);
	DECL_OPER (resourceforall);
	DECL_OPER (resourcestatus);
	DECL_OPER (revision);
	DECL_OPER (serialnumber);
	DECL_OPER (setcolorrendering);
	DECL_OPER (setdevparams);
	DECL_OPER (setoverprint);
	DECL_OPER (setsystemparams);
	DECL_OPER (setuserparams);
	DECL_OPER (startjob);
	DECL_OPER (undefineresource);
	DECL_OPER (GlobalFontDirectory);
	DECL_OPER (ASCII85Decode);
	DECL_OPER (ASCII85Encode);
	DECL_OPER (ASCIIHexDecode);
	DECL_OPER (ASCIIHexEncode);
	DECL_OPER (CCITTFaxDecode);
	DECL_OPER (CCITTFaxEncode);
	DECL_OPER (DCTDecode);
	DECL_OPER (DCTEncode);
	DECL_OPER (LZWDecode);
	DECL_OPER (LZWEncode);
	DECL_OPER (NullEncode);
	DECL_OPER (RunLengthDecode);
	DECL_OPER (RunLengthEncode);
	DECL_OPER (SubFileDecode);
	DECL_OPER (CIEBasedA);
	DECL_OPER (CIEBasedABC);
	DECL_OPER (DeviceCMYK);
	DECL_OPER (DeviceGray);
	DECL_OPER (DeviceRGB);
	DECL_OPER (Indexed);
	DECL_OPER (Pattern);
	DECL_OPER (Separation);
	DECL_OPER (CIEBasedDEF);
	DECL_OPER (CIEBasedDEFG);
	DECL_OPER (DeviceN);

#undef DECL_OPER
#undef DECL_OPER2

	__hg_operator_is_initialized = TRUE;

	return TRUE;
}

/**
 * hg_operator_tini:
 *
 * FIXME
 */
void
hg_operator_tini(void)
{
#define UNDECL_OPER(_n_)					\
	G_STMT_START {						\
		g_free(__hg_operator_name_table[HG_enc_ ## _n_]);	\
		__hg_operator_name_table[HG_enc_ ## _n_] = NULL;		\
		__hg_operator_func_table[HG_enc_ ## _n_] = NULL;		\
	} G_STMT_END

	UNDECL_OPER (abs);
	UNDECL_OPER (add);
	UNDECL_OPER (aload);
	UNDECL_OPER (anchorsearch);
	UNDECL_OPER (and);
	UNDECL_OPER (arc);
	UNDECL_OPER (arcn);
	UNDECL_OPER (arct);
	UNDECL_OPER (arcto);
	UNDECL_OPER (array);
	UNDECL_OPER (ashow);
	UNDECL_OPER (astore);
	UNDECL_OPER (awidthshow);
	UNDECL_OPER (begin);
	UNDECL_OPER (bind);
	UNDECL_OPER (bitshift);
	UNDECL_OPER (ceiling);
	UNDECL_OPER (charpath);
	UNDECL_OPER (clear);
	UNDECL_OPER (cleartomark);
	UNDECL_OPER (clip);
	UNDECL_OPER (clippath);
	UNDECL_OPER (closepath);
	UNDECL_OPER (concat);
	UNDECL_OPER (concatmatrix);
	UNDECL_OPER (copy);
	UNDECL_OPER (count);
	UNDECL_OPER (counttomark);
	UNDECL_OPER (currentcmykcolor);
	UNDECL_OPER (currentdash);
	UNDECL_OPER (currentdict);
	UNDECL_OPER (currentfile);
	UNDECL_OPER (currentfont);
	UNDECL_OPER (currentgray);
	UNDECL_OPER (currentgstate);
	UNDECL_OPER (currenthsbcolor);
	UNDECL_OPER (currentlinecap);
	UNDECL_OPER (currentlinejoin);
	UNDECL_OPER (currentlinewidth);
	UNDECL_OPER (currentmatrix);
	UNDECL_OPER (currentpoint);
	UNDECL_OPER (currentrgbcolor);
	UNDECL_OPER (currentshared);
	UNDECL_OPER (curveto);
	UNDECL_OPER (cvi);
	UNDECL_OPER (cvlit);
	UNDECL_OPER (cvn);
	UNDECL_OPER (cvr);
	UNDECL_OPER (cvrs);
	UNDECL_OPER (cvs);
	UNDECL_OPER (cvx);
	UNDECL_OPER (def);
	UNDECL_OPER (defineusername);
	UNDECL_OPER (dict);
	UNDECL_OPER (div);
	UNDECL_OPER (dtransform);
	UNDECL_OPER (dup);
	UNDECL_OPER (end);
	UNDECL_OPER (eoclip);
	UNDECL_OPER (eofill);
	UNDECL_OPER (eoviewclip);
	UNDECL_OPER (eq);
	UNDECL_OPER (exch);
	UNDECL_OPER (exec);
	UNDECL_OPER (exit);
	UNDECL_OPER (file);
	UNDECL_OPER (fill);
	UNDECL_OPER (findfont);
	UNDECL_OPER (flattenpath);
	UNDECL_OPER (floor);
	UNDECL_OPER (flush);
	UNDECL_OPER (flushfile);
	UNDECL_OPER (for);
	UNDECL_OPER (forall);
	UNDECL_OPER (ge);
	UNDECL_OPER (get);
	UNDECL_OPER (getinterval);
	UNDECL_OPER (grestore);
	UNDECL_OPER (gsave);
	UNDECL_OPER (gstate);
	UNDECL_OPER (gt);
	UNDECL_OPER (identmatrix);
	UNDECL_OPER (idiv);
	UNDECL_OPER (idtransform);
	UNDECL_OPER (if);
	UNDECL_OPER (ifelse);
	UNDECL_OPER (image);
	UNDECL_OPER (imagemask);
	UNDECL_OPER (index);
	UNDECL_OPER (ineofill);
	UNDECL_OPER (infill);
	UNDECL_OPER (initviewclip);
	UNDECL_OPER (inueofill);
	UNDECL_OPER (inufill);
	UNDECL_OPER (invertmatrix);
	UNDECL_OPER (itransform);
	UNDECL_OPER (known);
	UNDECL_OPER (le);
	UNDECL_OPER (length);
	UNDECL_OPER (lineto);
	UNDECL_OPER (load);
	UNDECL_OPER (loop);
	UNDECL_OPER (lt);
	UNDECL_OPER (makefont);
	UNDECL_OPER (matrix);
	UNDECL_OPER (maxlength);
	UNDECL_OPER (mod);
	UNDECL_OPER (moveto);
	UNDECL_OPER (mul);
	UNDECL_OPER (ne);
	UNDECL_OPER (neg);
	UNDECL_OPER (newpath);
	UNDECL_OPER (not);
	UNDECL_OPER (or);
	UNDECL_OPER (pathbbox);
	UNDECL_OPER (pathforall);
	UNDECL_OPER (pop);
	UNDECL_OPER (print);
	UNDECL_OPER (printobject);
	UNDECL_OPER (put);
	UNDECL_OPER (putinterval);
	UNDECL_OPER (rcurveto);
	UNDECL_OPER (read);
	UNDECL_OPER (readhexstring);
	UNDECL_OPER (readline);
	UNDECL_OPER (readstring);
	UNDECL_OPER (rectclip);
	UNDECL_OPER (rectfill);
	UNDECL_OPER (rectstroke);
	UNDECL_OPER (rectviewclip);
	UNDECL_OPER (repeat);
	UNDECL_OPER (restore);
	UNDECL_OPER (rlineto);
	UNDECL_OPER (rmoveto);
	UNDECL_OPER (roll);
	UNDECL_OPER (rotate);
	UNDECL_OPER (round);
	UNDECL_OPER (save);
	UNDECL_OPER (scale);
	UNDECL_OPER (scalefont);
	UNDECL_OPER (search);
	UNDECL_OPER (selectfont);
	UNDECL_OPER (setbbox);
	UNDECL_OPER (setcachedevice);
	UNDECL_OPER (setcachedevice2);
	UNDECL_OPER (setcharwidth);
	UNDECL_OPER (setcmykcolor);
	UNDECL_OPER (setdash);
	UNDECL_OPER (setfont);
	UNDECL_OPER (setgray);
	UNDECL_OPER (setgstate);
	UNDECL_OPER (sethsbcolor);
	UNDECL_OPER (setlinecap);
	UNDECL_OPER (setlinejoin);
	UNDECL_OPER (setlinewidth);
	UNDECL_OPER (setmatrix);
	UNDECL_OPER (setrgbcolor);
	UNDECL_OPER (setshared);
	UNDECL_OPER (shareddict);
	UNDECL_OPER (show);
	UNDECL_OPER (showpage);
	UNDECL_OPER (stop);
	UNDECL_OPER (stopped);
	UNDECL_OPER (store);
	UNDECL_OPER (string);
	UNDECL_OPER (stringwidth);
	UNDECL_OPER (stroke);
	UNDECL_OPER (strokepath);
	UNDECL_OPER (sub);
	UNDECL_OPER (systemdict);
	UNDECL_OPER (token);
	UNDECL_OPER (transform);
	UNDECL_OPER (translate);
	UNDECL_OPER (truncate);
	UNDECL_OPER (type);
	UNDECL_OPER (uappend);
	UNDECL_OPER (ucache);
	UNDECL_OPER (ueofill);
	UNDECL_OPER (ufill);
	UNDECL_OPER (undef);
	UNDECL_OPER (upath);
	UNDECL_OPER (userdict);
	UNDECL_OPER (ustroke);
	UNDECL_OPER (viewclip);
	UNDECL_OPER (viewclippath);
	UNDECL_OPER (where);
	UNDECL_OPER (widthshow);
	UNDECL_OPER (write);
	UNDECL_OPER (writehexstring);
	UNDECL_OPER (writeobject);
	UNDECL_OPER (writestring);
	UNDECL_OPER (wtranslation);
	UNDECL_OPER (xor);
	UNDECL_OPER (xshow);
	UNDECL_OPER (xyshow);
	UNDECL_OPER (yshow);
	UNDECL_OPER (FontDirectory);
	UNDECL_OPER (SharedFontDirectory);
	UNDECL_OPER (execuserobject);
	UNDECL_OPER (currentcolor);
	UNDECL_OPER (currentcolorspace);
	UNDECL_OPER (currentglobal);
	UNDECL_OPER (execform);
	UNDECL_OPER (filter);
	UNDECL_OPER (findresource);
	UNDECL_OPER (globaldict);
	UNDECL_OPER (makepattern);
	UNDECL_OPER (setcolor);
	UNDECL_OPER (setcolorspace);
	UNDECL_OPER (setglobal);
	UNDECL_OPER (setpagedevice);
	UNDECL_OPER (setpattern);
	UNDECL_OPER (sym_eq);
	UNDECL_OPER (sym_eqeq);
	UNDECL_OPER (ISOLatin1Encoding);
	UNDECL_OPER (StandardEncoding);
	UNDECL_OPER (sym_left_square_bracket);
	UNDECL_OPER (sym_right_square_bracket);
	UNDECL_OPER (atan);
	UNDECL_OPER (banddevice);
	UNDECL_OPER (bytesavailable);
	UNDECL_OPER (cachestatus);
	UNDECL_OPER (closefile);
	UNDECL_OPER (colorimage);
	UNDECL_OPER (condition);
	UNDECL_OPER (copypage);
	UNDECL_OPER (cos);
	UNDECL_OPER (countdictstack);
	UNDECL_OPER (countexecstack);
	UNDECL_OPER (cshow);
	UNDECL_OPER (currentblackgeneration);
	UNDECL_OPER (currentcacheparams);
	UNDECL_OPER (currentcolorscreen);
	UNDECL_OPER (currentcolortransfer);
	UNDECL_OPER (currentcontext);
	UNDECL_OPER (currentflat);
	UNDECL_OPER (currenthalftone);
	UNDECL_OPER (currenthalftonephase);
	UNDECL_OPER (currentmiterlimit);
	UNDECL_OPER (currentobjectformat);
	UNDECL_OPER (currentpacking);
	UNDECL_OPER (currentscreen);
	UNDECL_OPER (currentstrokeadjust);
	UNDECL_OPER (currenttransfer);
	UNDECL_OPER (currentundercolorremoval);
	UNDECL_OPER (defaultmatrix);
	UNDECL_OPER (definefont);
	UNDECL_OPER (deletefile);
	UNDECL_OPER (detach);
	UNDECL_OPER (deviceinfo);
	UNDECL_OPER (dictstack);
	UNDECL_OPER (echo);
	UNDECL_OPER (erasepage);
	UNDECL_OPER (execstack);
	UNDECL_OPER (executeonly);
	UNDECL_OPER (exp);
	UNDECL_OPER (filenameforall);
	UNDECL_OPER (fileposition);
	UNDECL_OPER (fork);
	UNDECL_OPER (framedevice);
	UNDECL_OPER (grestoreall);
	UNDECL_OPER (handleerror);
	UNDECL_OPER (initclip);
	UNDECL_OPER (initgraphics);
	UNDECL_OPER (initmatrix);
	UNDECL_OPER (instroke);
	UNDECL_OPER (inustroke);
	UNDECL_OPER (join);
	UNDECL_OPER (kshow);
	UNDECL_OPER (ln);
	UNDECL_OPER (lock);
	UNDECL_OPER (log);
	UNDECL_OPER (mark);
	UNDECL_OPER (monitor);
	UNDECL_OPER (noaccess);
	UNDECL_OPER (notify);
	UNDECL_OPER (nulldevice);
	UNDECL_OPER (packedarray);
	UNDECL_OPER (quit);
	UNDECL_OPER (rand);
	UNDECL_OPER (rcheck);
	UNDECL_OPER (readonly);
	UNDECL_OPER (realtime);
	UNDECL_OPER (renamefile);
	UNDECL_OPER (renderbands);
	UNDECL_OPER (resetfile);
	UNDECL_OPER (reversepath);
	UNDECL_OPER (rootfont);
	UNDECL_OPER (rrand);
	UNDECL_OPER (run);
	UNDECL_OPER (scheck);
	UNDECL_OPER (setblackgeneration);
	UNDECL_OPER (setcachelimit);
	UNDECL_OPER (setcacheparams);
	UNDECL_OPER (setcolorscreen);
	UNDECL_OPER (setcolortransfer);
	UNDECL_OPER (setfileposition);
	UNDECL_OPER (setflat);
	UNDECL_OPER (sethalftone);
	UNDECL_OPER (sethalftonephase);
	UNDECL_OPER (setmiterlimit);
	UNDECL_OPER (setobjectformat);
	UNDECL_OPER (setpacking);
	UNDECL_OPER (setscreen);
	UNDECL_OPER (setstrokeadjust);
	UNDECL_OPER (settransfer);
	UNDECL_OPER (setucacheparams);
	UNDECL_OPER (setundercolorremoval);
	UNDECL_OPER (sin);
	UNDECL_OPER (sqrt);
	UNDECL_OPER (srand);
	UNDECL_OPER (stack);
	UNDECL_OPER (status);
	UNDECL_OPER (statusdict);
	UNDECL_OPER (ucachestatus);
	UNDECL_OPER (undefinefont);
	UNDECL_OPER (usertime);
	UNDECL_OPER (ustrokepath);
	UNDECL_OPER (version);
	UNDECL_OPER (vmreclaim);
	UNDECL_OPER (vmstatus);
	UNDECL_OPER (wait);
	UNDECL_OPER (wcheck);
	UNDECL_OPER (xcheck);
	UNDECL_OPER (yield);
	UNDECL_OPER (defineuserobject);
	UNDECL_OPER (undefineuserobject);
	UNDECL_OPER (UserObjects);
	UNDECL_OPER (cleardictstack);
	UNDECL_OPER (setvmthreshold);
	UNDECL_OPER (sym_begin_dict_mark);
	UNDECL_OPER (sym_end_dict_mark);
	UNDECL_OPER (currentcolorrendering);
	UNDECL_OPER (currentdevparams);
	UNDECL_OPER (currentoverprint);
	UNDECL_OPER (currentpagedevice);
	UNDECL_OPER (currentsystemparams);
	UNDECL_OPER (currentuserparams);
	UNDECL_OPER (defineresource);
	UNDECL_OPER (findencoding);
	UNDECL_OPER (gcheck);
	UNDECL_OPER (glyphshow);
	UNDECL_OPER (languagelevel);
	UNDECL_OPER (product);
	UNDECL_OPER (pstack);
	UNDECL_OPER (resourceforall);
	UNDECL_OPER (resourcestatus);
	UNDECL_OPER (revision);
	UNDECL_OPER (serialnumber);
	UNDECL_OPER (setcolorrendering);
	UNDECL_OPER (setdevparams);
	UNDECL_OPER (setoverprint);
	UNDECL_OPER (setsystemparams);
	UNDECL_OPER (setuserparams);
	UNDECL_OPER (startjob);
	UNDECL_OPER (undefineresource);
	UNDECL_OPER (GlobalFontDirectory);
	UNDECL_OPER (ASCII85Decode);
	UNDECL_OPER (ASCII85Encode);
	UNDECL_OPER (ASCIIHexDecode);
	UNDECL_OPER (ASCIIHexEncode);
	UNDECL_OPER (CCITTFaxDecode);
	UNDECL_OPER (CCITTFaxEncode);
	UNDECL_OPER (DCTDecode);
	UNDECL_OPER (DCTEncode);
	UNDECL_OPER (LZWDecode);
	UNDECL_OPER (LZWEncode);
	UNDECL_OPER (NullEncode);
	UNDECL_OPER (RunLengthDecode);
	UNDECL_OPER (RunLengthEncode);
	UNDECL_OPER (SubFileDecode);
	UNDECL_OPER (CIEBasedA);
	UNDECL_OPER (CIEBasedABC);
	UNDECL_OPER (DeviceCMYK);
	UNDECL_OPER (DeviceGray);
	UNDECL_OPER (DeviceRGB);
	UNDECL_OPER (Indexed);
	UNDECL_OPER (Pattern);
	UNDECL_OPER (Separation);
	UNDECL_OPER (CIEBasedDEF);
	UNDECL_OPER (CIEBasedDEFG);
	UNDECL_OPER (DeviceN);

#undef UNDECL_OPER

	__hg_operator_is_initialized = FALSE;
}

/**
 * hg_operator_invoke:
 * @qoper:
 * @vm:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_operator_invoke(hg_quark_t   qoper,
		   hg_vm_t     *vm,
		   GError     **error)
{
	hg_quark_t q;
	GError *err = NULL;
	gboolean retval = TRUE;

	hg_return_val_with_gerror_if_fail (HG_IS_QOPER (qoper), FALSE, error);
	hg_return_val_with_gerror_if_fail (vm != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail ((q = hg_quark_get_value(qoper)) < HG_enc_END, FALSE, error);

	if (__hg_operator_func_table[q] == NULL) {
		if (__hg_operator_name_table[q] == NULL) {
			g_set_error(&err, HG_ERROR, EINVAL,
				    "Invalid operators - quark: %lx", qoper);
		} else {
			g_set_error(&err, HG_ERROR, EINVAL,
				    "%s operator isn't yet implemented.",
				    __hg_operator_name_table[q]);
		}
		retval = FALSE;
	} else {
		retval = __hg_operator_func_table[q] (vm, &err);
	}

	if (err) {
		if (error) {
			*error = g_error_copy(err);
		}
		g_error_free(err);
		retval = FALSE;
	}

	return retval;
}

/**
 * hg_operator_get_name:
 * @qoper:
 *
 * FIXME
 *
 * Returns:
 */
const gchar *
hg_operator_get_name(hg_quark_t qoper)
{
	hg_return_val_if_fail (HG_IS_QOPER (qoper), NULL);

	return __hg_operator_name_table[hg_quark_get_value(qoper)];
}

/**
 * hg_operator_register:
 * @dict:
 * @name:
 * @lang_level:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_operator_register(hg_dict_t         *dict,
		     hg_name_t         *name,
		     hg_vm_langlevel_t  lang_level)
{
	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (lang_level < HG_LANG_LEVEL_END, FALSE);

	/* register level 1 built-in operators */
	if (!_hg_operator_level1_register(dict, name))
		return FALSE;

	/* register level 2 built-in operators */
	if (lang_level >= HG_LANG_LEVEL_2 &&
	    !_hg_operator_level2_register(dict, name))
		return FALSE;

	/* register level 3 built-in operators */
	if (lang_level >= HG_LANG_LEVEL_3 &&
	    !_hg_operator_level3_register(dict, name))
		return FALSE;

	return TRUE;
}
