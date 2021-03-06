/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgoperator.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "hgerror.h"
#include "hgarray.h"
#include "hgbool.h"
#include "hgdevice.h"
#include "hgdict.h"
#include "hggstate.h"
#include "hgint.h"
#include "hgmark.h"
#include "hgmem.h"
#include "hgnull.h"
#include "hgreal.h"
#include "hgsnapshot.h"
#include "hgutils.h"
#include "hgversion.h"
#include "hgoperator.h"

#include "hgoperator.proto.h"

static hg_operator_func_t __hg_operator_func_table[HG_enc_END];
static hg_char_t *__hg_operator_name_table[HG_enc_END];
static hg_bool_t __hg_operator_is_initialized = FALSE;

/*< private >*/
PROTO_OPER (private_abort);
PROTO_OPER (private_applyparams);
PROTO_OPER (private_clearerror);
PROTO_OPER (private_callbeginpage);
PROTO_OPER (private_callendpage);
PROTO_OPER (private_callinstall);
PROTO_OPER (private_currentpagedevice);
PROTO_OPER (private_exit);
PROTO_OPER (private_findlibfile);
PROTO_OPER (private_forceput);
PROTO_OPER (private_hgrevision);
PROTO_OPER (private_odef);
PROTO_OPER (private_product);
PROTO_OPER (private_quit);
PROTO_OPER (private_revision);
PROTO_OPER (private_setglobal);
PROTO_OPER (private_stringcvs);
PROTO_OPER (private_undef);
PROTO_OPER (private_write_eqeq_only);
PROTO_OPER (protected_arraytomark);
PROTO_OPER (protected_dicttomark);
PROTO_OPER (protected_for_yield_int_continue);
PROTO_OPER (protected_for_yield_real_continue);
PROTO_OPER (protected_forall_array_continue);
PROTO_OPER (protected_forall_dict_continue);
PROTO_OPER (protected_forall_string_continue);
PROTO_OPER (protected_loop_continue);
PROTO_OPER (protected_repeat_continue);
PROTO_OPER (protected_stopped_continue);
PROTO_OPER (ASCII85Decode);
PROTO_OPER (ASCII85Encode);
PROTO_OPER (ASCIIHexDecode);
PROTO_OPER (ASCIIHexEncode);
PROTO_OPER (CCITTFaxDecode);
PROTO_OPER (CCITTFaxEncode);
PROTO_OPER (CIEBasedA);
PROTO_OPER (CIEBasedABC);
PROTO_OPER (CIEBasedDEF);
PROTO_OPER (CIEBasedDEFG);
PROTO_OPER (DCTDecode);
PROTO_OPER (DCTEncode);
PROTO_OPER (DeviceCMYK);
PROTO_OPER (DeviceGray);
PROTO_OPER (DeviceN);
PROTO_OPER (DeviceRGB);
PROTO_OPER (FontDirectory);
PROTO_OPER (GlobalFontDirectory);
PROTO_OPER (ISOLatin1Encoding);
PROTO_OPER (Indexed);
PROTO_OPER (LZWDecode);
PROTO_OPER (LZWEncode);
PROTO_OPER (NullEncode);
PROTO_OPER (Pattern);
PROTO_OPER (RunLengthDecode);
PROTO_OPER (RunLengthEncode);
PROTO_OPER (Separation);
PROTO_OPER (SharedFontDirectory);
/* StandardEncoding is implemented in PostScript */
PROTO_OPER (SubFileDecode);
PROTO_OPER (UserObjects);
PROTO_OPER (abs);
PROTO_OPER (add);
PROTO_OPER (aload);
/* anchorsearch is implemented in PostScript */
PROTO_OPER (and);
PROTO_OPER (arc);
PROTO_OPER (arcn);
PROTO_OPER (arct);
PROTO_OPER (arcto);
PROTO_OPER (array);
PROTO_OPER (ashow);
PROTO_OPER (astore);
PROTO_OPER (atan);
PROTO_OPER (awidthshow);
PROTO_OPER (banddevice);
PROTO_OPER (begin);
PROTO_OPER (bind);
PROTO_OPER (bitshift);
PROTO_OPER (bytesavailable);
PROTO_OPER (cachestatus);
PROTO_OPER (ceiling);
PROTO_OPER (charpath);
PROTO_OPER (clear);
PROTO_OPER (cleardictstack);
PROTO_OPER (cleartomark);
PROTO_OPER (clip);
PROTO_OPER (clippath);
PROTO_OPER (closefile);
PROTO_OPER (closepath);
PROTO_OPER (colorimage);
PROTO_OPER (concat);
PROTO_OPER (concatmatrix);
PROTO_OPER (condition);
PROTO_OPER (copy);
PROTO_OPER (copypage);
PROTO_OPER (cos);
PROTO_OPER (count);
PROTO_OPER (countdictstack);
PROTO_OPER (countexecstack);
PROTO_OPER (counttomark);
PROTO_OPER (cshow);
PROTO_OPER (currentblackgeneration);
PROTO_OPER (currentcacheparams);
PROTO_OPER (currentcmykcolor);
PROTO_OPER (currentcolor);
PROTO_OPER (currentcolorrendering);
PROTO_OPER (currentcolorscreen);
PROTO_OPER (currentcolorspace);
PROTO_OPER (currentcolortransfer);
PROTO_OPER (currentcontext);
PROTO_OPER (currentdash);
PROTO_OPER (currentdevparams);
PROTO_OPER (currentdict);
PROTO_OPER (currentfile);
PROTO_OPER (currentflat);
PROTO_OPER (currentfont);
PROTO_OPER (currentglobal);
PROTO_OPER (currentgray);
PROTO_OPER (currentgstate);
PROTO_OPER (currenthalftone);
PROTO_OPER (currenthalftonephase);
PROTO_OPER (currenthsbcolor);
PROTO_OPER (currentlinecap);
PROTO_OPER (currentlinejoin);
PROTO_OPER (currentlinewidth);
PROTO_OPER (currentmatrix);
PROTO_OPER (currentmiterlimit);
PROTO_OPER (currentobjectformat);
PROTO_OPER (currentoverprint);
PROTO_OPER (currentpacking);
PROTO_OPER (currentpagedevice);
PROTO_OPER (currentpoint);
PROTO_OPER (currentrgbcolor);
PROTO_OPER (currentscreen);
PROTO_OPER (currentshared);
PROTO_OPER (currentstrokeadjust);
PROTO_OPER (currentsystemparams);
PROTO_OPER (currenttransfer);
PROTO_OPER (currentundercolorremoval);
PROTO_OPER (currentuserparams);
PROTO_OPER (curveto);
PROTO_OPER (cvi);
PROTO_OPER (cvlit);
PROTO_OPER (cvn);
PROTO_OPER (cvr);
PROTO_OPER (cvrs);
/* cvs is implemented in PostScript */
PROTO_OPER (cvx);
PROTO_OPER (def);
PROTO_OPER (defaultmatrix);
/* definefont is implemented in PostScript */
PROTO_OPER (defineresource);
PROTO_OPER (defineusername);
PROTO_OPER (defineuserobject);
PROTO_OPER (deletefile);
PROTO_OPER (detach);
PROTO_OPER (deviceinfo);
PROTO_OPER (dict);
PROTO_OPER (dictstack);
PROTO_OPER (div);
PROTO_OPER (dtransform);
PROTO_OPER (dup);
PROTO_OPER (echo);
PROTO_OPER (eexec);
PROTO_OPER (end);
PROTO_OPER (eoclip);
PROTO_OPER (eofill);
PROTO_OPER (eoviewclip);
PROTO_OPER (eq);
PROTO_OPER (erasepage);
PROTO_OPER (exch);
PROTO_OPER (exec);
PROTO_OPER (execform);
PROTO_OPER (execstack);
PROTO_OPER (execuserobject);
PROTO_OPER (executeonly);
/* executive is implemented in PostScript */
PROTO_OPER (exit);
PROTO_OPER (exp);
PROTO_OPER (file);
PROTO_OPER (filenameforall);
PROTO_OPER (fileposition);
PROTO_OPER (fill);
PROTO_OPER (filter);
PROTO_OPER (findencoding);
/* findfont is implemented in PostScript */
PROTO_OPER (findresource);
PROTO_OPER (flattenpath);
/* floor is implemented in PostScript */
PROTO_OPER (flush);
PROTO_OPER (flushfile);
PROTO_OPER (for);
PROTO_OPER (forall);
PROTO_OPER (fork);
PROTO_OPER (framedevice);
PROTO_OPER (gcheck);
PROTO_OPER (ge);
PROTO_OPER (get);
PROTO_OPER (getinterval);
PROTO_OPER (glyphshow);
PROTO_OPER (grestore);
PROTO_OPER (grestoreall);
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
PROTO_OPER (initclip);
/* initgraphics is implemented in PostScript */
/* initmatrix is implemented in PostScript */
PROTO_OPER (initviewclip);
PROTO_OPER (instroke);
PROTO_OPER (inueofill);
PROTO_OPER (inufill);
PROTO_OPER (inustroke);
PROTO_OPER (invertmatrix);
PROTO_OPER (itransform);
PROTO_OPER (join);
PROTO_OPER (known);
PROTO_OPER (kshow);
PROTO_OPER (languagelevel);
PROTO_OPER (le);
PROTO_OPER (length);
PROTO_OPER (lineto);
PROTO_OPER (ln);
/* load is implemented in PostScript */
PROTO_OPER (lock);
PROTO_OPER (log);
PROTO_OPER (loop);
PROTO_OPER (lt);
PROTO_OPER (makefont);
PROTO_OPER (makepattern);
/* matrix is implemented in PostScript */
PROTO_OPER (maxlength);
PROTO_OPER (mod);
PROTO_OPER (monitor);
PROTO_OPER (moveto);
PROTO_OPER (mul);
PROTO_OPER (ne);
PROTO_OPER (neg);
PROTO_OPER (newpath);
PROTO_OPER (noaccess);
PROTO_OPER (not);
PROTO_OPER (notify);
PROTO_OPER (nulldevice);
PROTO_OPER (or);
PROTO_OPER (packedarray);
PROTO_OPER (pathbbox);
PROTO_OPER (pathforall);
PROTO_OPER (pop);
PROTO_OPER (print);
PROTO_OPER (printobject);
PROTO_OPER (product);
/* prompt is implemented in PostScript */
/* pstack is implemented in PostScript */
PROTO_OPER (put);
/* putinterval is implemented in PostScript */
/* quit is implemented in PostScript */
PROTO_OPER (rand);
PROTO_OPER (rcheck);
PROTO_OPER (rcurveto);
PROTO_OPER (read);
PROTO_OPER (readhexstring);
PROTO_OPER (readline);
PROTO_OPER (readonly);
PROTO_OPER (readstring);
PROTO_OPER (realtime);
PROTO_OPER (rectclip);
PROTO_OPER (rectfill);
PROTO_OPER (rectstroke);
PROTO_OPER (rectviewclip);
PROTO_OPER (renamefile);
PROTO_OPER (renderbands);
PROTO_OPER (repeat);
PROTO_OPER (resetfile);
PROTO_OPER (resourceforall);
PROTO_OPER (resourcestatus);
PROTO_OPER (restore);
PROTO_OPER (reversepath);
/* revision is implemented in PostScript */
PROTO_OPER (rlineto);
PROTO_OPER (rmoveto);
PROTO_OPER (roll);
PROTO_OPER (rootfont);
PROTO_OPER (rotate);
PROTO_OPER (round);
PROTO_OPER (rrand);
/* run is implemented in PostScript */
PROTO_OPER (save);
PROTO_OPER (scale);
PROTO_OPER (scalefont);
/* scheck is implemented in PostScript */
PROTO_OPER (search);
PROTO_OPER (selectfont);
PROTO_OPER (serialnumber);
PROTO_OPER (setbbox);
PROTO_OPER (setblackgeneration);
PROTO_OPER (setcachedevice);
PROTO_OPER (setcachedevice2);
PROTO_OPER (setcachelimit);
PROTO_OPER (setcacheparams);
PROTO_OPER (setcharwidth);
PROTO_OPER (setcmykcolor);
PROTO_OPER (setcolor);
PROTO_OPER (setcolorrendering);
PROTO_OPER (setcolorscreen);
PROTO_OPER (setcolorspace);
PROTO_OPER (setcolortransfer);
PROTO_OPER (setdash);
PROTO_OPER (setdevparams);
PROTO_OPER (setfileposition);
PROTO_OPER (setflat);
PROTO_OPER (setfont);
/* setglobal is implemented in PostScript */
PROTO_OPER (setgray);
PROTO_OPER (setgstate);
PROTO_OPER (sethalftone);
PROTO_OPER (sethalftonephase);
PROTO_OPER (sethsbcolor);
PROTO_OPER (setlinecap);
PROTO_OPER (setlinejoin);
PROTO_OPER (setlinewidth);
PROTO_OPER (setmatrix);
PROTO_OPER (setmiterlimit);
PROTO_OPER (setobjectformat);
PROTO_OPER (setoverprint);
PROTO_OPER (setpacking);
PROTO_OPER (setpagedevice);
PROTO_OPER (setpattern);
PROTO_OPER (setrgbcolor);
PROTO_OPER (setscreen);
PROTO_OPER (setshared);
PROTO_OPER (setstrokeadjust);
PROTO_OPER (setsystemparams);
PROTO_OPER (settransfer);
PROTO_OPER (setucacheparams);
PROTO_OPER (setundercolorremoval);
PROTO_OPER (setuserparams);
PROTO_OPER (setvmthreshold);
PROTO_OPER (shareddict);
PROTO_OPER (show);
PROTO_OPER (showpage);
PROTO_OPER (sin);
PROTO_OPER (sqrt);
PROTO_OPER (srand);
/* stack is implemented in PostScript */
/* start is implemented in PostScript */
PROTO_OPER (startjob);
PROTO_OPER (status);
/* statusdict is implemented in PostScript */
PROTO_OPER (stop);
PROTO_OPER (stopped);
/* store is implemented in PostScript */
PROTO_OPER (string);
PROTO_OPER (stringwidth);
PROTO_OPER (stroke);
PROTO_OPER (strokepath);
PROTO_OPER (sub);
PROTO_OPER (token);
PROTO_OPER (transform);
PROTO_OPER (translate);
/* truncate is implemented in PostScript */
PROTO_OPER (type);
PROTO_OPER (uappend);
PROTO_OPER (ucache);
PROTO_OPER (ucachestatus);
PROTO_OPER (ueofill);
PROTO_OPER (ufill);
/* undef is implemented in PostScript */
/* undefinefont is implemented in PostScript */
PROTO_OPER (undefineresource);
PROTO_OPER (undefineuserobject);
PROTO_OPER (upath);
PROTO_OPER (usertime);
PROTO_OPER (ustroke);
PROTO_OPER (ustrokepath);
/* version is implemented in PostScript */
PROTO_OPER (viewclip);
PROTO_OPER (viewclippath);
PROTO_OPER (vmreclaim);
PROTO_OPER (vmstatus);
PROTO_OPER (wait);
PROTO_OPER (wcheck);
PROTO_OPER (where);
PROTO_OPER (widthshow);
PROTO_OPER (write);
PROTO_OPER (writehexstring);
PROTO_OPER (writeobject);
PROTO_OPER (writestring);
PROTO_OPER (wtranslation);
PROTO_OPER (xcheck);
PROTO_OPER (xor);
PROTO_OPER (xshow);
PROTO_OPER (xyshow);
PROTO_OPER (yield);
PROTO_OPER (yshow);
/* << (sym_begin_dict_mark) is implemented as value */
/* >> (sym_end_dict_mark) is implemented as value */
/* = (sym_eq) is implemented in PostScript */
/* == (sym_eqeq) is implemented in PostScript */
/* [ (sym_left_square_bracket) is implemented as value */
/* ] (sym_right_square_bracket) is implemented as value */


/* - .abort - */
DEFUNC_OPER (private_abort)
{
	hg_quark_t q;
	hg_file_t *file;

	q = hg_vm_get_io(vm, HG_FILE_IO_STDERR);
	file = HG_VM_LOCK (vm, q);
	if (file == NULL) {
		g_printerr("  Unable to obtain stderr.\n");
	} else {
		hg_file_append_printf(file, "* Operand stack(%d):\n", hg_stack_depth(vm->stacks[HG_VM_STACK_OSTACK]));
		hg_vm_stack_dump(vm, vm->stacks[HG_VM_STACK_OSTACK], file);
		hg_file_append_printf(file, "\n* Exec stack(%d):\n", hg_stack_depth(vm->stacks[HG_VM_STACK_ESTACK]));
		hg_vm_stack_dump(vm, vm->stacks[HG_VM_STACK_ESTACK], file);
		hg_file_append_printf(file, "\n* Dict stack(%d):\n", hg_stack_depth(vm->stacks[HG_VM_STACK_DSTACK]));
		hg_vm_stack_dump(vm, vm->stacks[HG_VM_STACK_DSTACK], file);

		hg_file_append_printf(file, "\n* Reserved spool in global memory:\n");
		hg_vm_reserved_spool_dump(vm, vm->mem[HG_VM_MEM_GLOBAL], file);
		hg_file_append_printf(file, "\n* Reserved spool in local memory:\n");
		hg_vm_reserved_spool_dump(vm, vm->mem[HG_VM_MEM_LOCAL], file);
		hg_file_append_printf(file, "\nCurrent allocation mode is %s\n", hg_vm_is_global_mem_used(vm) ? "global" : "local");

		HG_VM_UNLOCK (vm, q);
	}
	abort();
} DEFUNC_OPER_END

/* <dict> .applyparams - */
DEFUNC_OPER (private_applyparams)
{
	hg_quark_t arg0;
	hg_dict_t *d;
	GList *l, *keys;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QDICT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	d = HG_VM_LOCK (vm, arg0);
	if (d == NULL) {
		return FALSE;
	}
	keys = g_hash_table_get_keys(vm->params);
	for (l = keys; l != NULL; l = g_list_next(l)) {
		hg_char_t *name = l->data;
		hg_quark_t qn = HG_QNAME (name);

		if (hg_dict_lookup(d, qn) == Qnil) {
			/* only add the value not in the dictionary yet */
			hg_vm_value_t *v = g_hash_table_lookup(vm->params, name);
			hg_quark_t q = Qnil;

			switch (v->type) {
			    case HG_TYPE_BOOL:
				    q = HG_QBOOL (v->u.bool);
				    break;
			    case HG_TYPE_INT:
				    q = HG_QINT (v->u.integer);
				    break;
			    case HG_TYPE_REAL:
				    q = HG_QREAL (v->u.real);
				    break;
			    case HG_TYPE_STRING:
				    q = HG_QSTRING (hg_vm_get_mem(vm), v->u.string);
				    break;
			    default:
				    hg_warning("Unknown parameter type: %d", v->type);
				    break;
			}
			if (q != Qnil)
				hg_dict_add(d, qn, q, FALSE);
		}
	}
	HG_VM_UNLOCK (vm, arg0);
	g_list_free(keys);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* - .clearerror - */
DEFUNC_OPER (private_clearerror)
{
	hg_vm_reset_error(vm);

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (private_callbeginpage);
DEFUNC_UNIMPLEMENTED_OPER (private_callendpage);

/* - .callinstall - */
DEFUNC_OPER (private_callinstall)
{
	hg_device_install(vm->device, vm);

	retval = TRUE;
} DEFUNC_OPER_END

static hg_quark_t
_hg_operator_pdev_name_lookup(hg_pdev_params_name_t param,
			      hg_pointer_t          user_data)
{
	hg_vm_t *vm = (hg_vm_t *)user_data;

	return vm->qpdevparams_name[param];
}

/* - .currentpagedevice <dict> */
DEFUNC_OPER (private_currentpagedevice)
{
	hg_quark_t q;

	q = hg_device_get_page_params(vm->device,
				      vm->mem[HG_VM_MEM_LOCAL],
				      hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1,
				      _hg_operator_pdev_name_lookup, vm,
				      NULL);
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	hg_vm_quark_set_default_acl(vm, &q);

	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* - .exit - */
DEFUNC_OPER (private_exit)
{
	hg_usize_t i, j, edepth = hg_stack_depth(estack);
	hg_quark_t q;

	/* almost code is shared with exit.
	 * only difference is to trap on %stopped_continue or not
	 */
	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i);
		if (HG_IS_QOPER (q)) {
			if (HG_OPER (q) == HG_enc_protected_for_yield_int_continue ||
			    HG_OPER (q) == HG_enc_protected_for_yield_real_continue) {
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 4));
			} else if (HG_OPER (q) == HG_enc_protected_loop_continue) {
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 1));
			} else if (HG_OPER (q) == HG_enc_protected_repeat_continue) {
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 2));
			} else if (HG_OPER (q) == HG_enc_protected_forall_array_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_dict_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_string_continue) {
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 3));
			} else {
				continue;
			}
			for (j = 0; j < i; j++)
				hg_stack_drop(estack);
			break;
		} else if (HG_IS_QFILE (q)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidexit);
		}
	}
	if (i == edepth) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidexit);
	} else {
		retval = TRUE;
	}
} DEFUNC_OPER_END

/* <filename> .findlibfile <filename> <true>
 * <filename> .findlibfile <false>
 */
DEFUNC_OPER (private_findlibfile)
{
	hg_quark_t arg0, q = Qnil;
	hg_char_t *filename, *cstr;
	hg_bool_t ret = FALSE;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	cstr = hg_vm_string_get_cstr(vm, arg0);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}

	filename = hg_find_libfile(cstr);
	hg_free(cstr);

	if (filename) {
		q = HG_QSTRING (hg_vm_get_mem(vm),
				filename);
		g_free(filename);
		if (q == Qnil) {
			hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
		}
		ret = TRUE;
		SET_EXPECTED_OSTACK_SIZE (1);
	}
	hg_stack_drop(ostack);
	if (q != Qnil)
		STACK_PUSH (ostack, q);
	STACK_PUSH (ostack, HG_QBOOL (ret));

	retval = TRUE;
} DEFUNC_OPER_END

/* <array> <index> <any> .forceput -
 * <dict> <key> <any> .forceput -
 * <string> <index> <int> .forceput -
 */
DEFUNC_OPER (private_forceput)
{
	hg_quark_t arg0, arg1, arg2;

	CHECK_STACK (ostack, 3);
	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);

	/* Do not check the permission here.
	 * this operator is likely to put the value into
	 * read-only array. which is copied from the object
	 * in execstack. and this operator is a special
	 * operator to deal with them.
	 */
	if (HG_IS_QARRAY (arg0)) {
		hg_usize_t index;

		if (!HG_IS_QINT (arg1)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		}
		index = HG_INT (arg1);
		retval = hg_vm_array_set(vm, arg0, arg2, index, TRUE);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
	} else if (HG_IS_QDICT (arg0)) {
		retval = hg_vm_dict_add(vm, arg0, arg1, arg2, TRUE);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;

		if (!HG_IS_QINT (arg1) ||
		    !HG_IS_QINT (arg2)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		}
		s = HG_VM_LOCK (vm, arg0);
		if (s == NULL) {
			return FALSE;
		}
		retval = hg_string_overwrite_c(s, arg2, arg1);

		HG_VM_UNLOCK (vm, arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	SET_EXPECTED_OSTACK_SIZE (-3);
} DEFUNC_OPER_END

/* - .hgrevision <int> */
DEFUNC_OPER (private_hgrevision)
{
	hg_int_t revision = 0;

	sscanf(__hg_rcsid, "$Rev: %d $", &revision);

	STACK_PUSH (ostack, HG_QINT (revision));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* <key> <proc> .odef - */
DEFUNC_OPER (private_odef)
{
	hg_quark_t arg0, arg1;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QNAME (arg0) ||
	    !HG_IS_QARRAY (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (hg_vm_quark_is_executable(vm, &arg1)) {
		hg_array_t *a = HG_VM_LOCK (vm, arg1);
		const hg_char_t *name = hg_name_lookup(arg0);

		hg_array_set_name(a, name);
		HG_VM_UNLOCK (vm, arg1);

		hg_vm_quark_set_acl(vm, &arg1, HG_ACL_EXECUTABLE|HG_ACL_ACCESSIBLE);
	}
	hg_stack_pop(ostack);

	STACK_PUSH (ostack, arg1);
	retval = _hg_operator_real_def(vm);
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

/* - .product <string> */
DEFUNC_OPER (private_product)
{
	hg_quark_t q;

	q = HG_QSTRING (hg_vm_get_mem(vm),
			"Hieroglyph PostScript Interpreter");
	if (q == Qnil) {
		return FALSE;
	}
	hg_vm_quark_set_acl(vm, &q, HG_ACL_READABLE|HG_ACL_ACCESSIBLE);

	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* <int> .quit - */
DEFUNC_OPER (private_quit)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	hg_vm_shutdown(vm, HG_INT (arg0));

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* - .revision <int> */
DEFUNC_OPER (private_revision)
{
	hg_int_t major = HIEROGLYPH_MAJOR_VERSION;
	hg_int_t minor = HIEROGLYPH_MINOR_VERSION;
	hg_int_t release = HIEROGLYPH_RELEASE_VERSION;

	STACK_PUSH (ostack,
		    HG_QINT (major * 1000000 + minor * 1000 + release));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* <bool> .setglobal - */
DEFUNC_OPER (private_setglobal)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QBOOL(arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	hg_vm_use_global_mem(vm, HG_BOOL (arg0));

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <any> .stringcvs <string> */
DEFUNC_OPER (private_stringcvs)
{
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0) ||
	    HG_IS_QREAL (arg0) ||
	    HG_IS_QBOOL (arg0) ||
	    HG_IS_QSTRING (arg0) ||
	    HG_IS_QNAME (arg0) ||
	    HG_IS_QOPER (arg0)) {
		q = hg_vm_quark_to_string(vm, arg0, FALSE, NULL);
	} else {
		q = HG_QSTRING (hg_vm_get_mem(vm), "--nostringval--");
	}
	if (q == Qnil) {
		return FALSE;
	}

	hg_stack_drop(ostack);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} DEFUNC_OPER_END

/* <dict> <key> .undef - */
DEFUNC_OPER (private_undef)
{
	hg_quark_t arg0, arg1;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	hg_vm_dict_remove(vm, arg0, arg1);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

/* <file> <any> .write==only - */
DEFUNC_OPER (private_write_eqeq_only)
{
	hg_quark_t arg0, arg1, q;
	hg_string_t *s = NULL;
	hg_file_t *f;
	hg_char_t *cstr;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	q = hg_vm_quark_to_string(vm, arg1, TRUE, (hg_pointer_t *)&s);
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	f = HG_VM_LOCK (vm, arg0);
	if (f == NULL) {
		hg_string_free(s, TRUE);
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	cstr = hg_string_get_cstr(s);
	hg_file_write(f, cstr,
		      sizeof (hg_char_t), hg_string_length(s));
	HG_VM_UNLOCK (vm, arg0);
	hg_string_free(s, TRUE);
	hg_free(cstr);

	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

/* <mark> ... %arraytomark <array> */
DEFUNC_OPER (protected_arraytomark)
{
	/* %arraytomark is the same to {counttomark array astore exch pop} */
	if (!hg_operator_invoke(HG_QOPER (HG_enc_counttomark), vm))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_array), vm))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_astore), vm))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_exch), vm))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_pop), vm))
		return FALSE;

	retval = TRUE;
	/* no need to validate the stack size.
	 * if the result in all above is ok, everything would be fine then.
	 */
	NO_STACK_VALIDATION;
} DEFUNC_OPER_END

/* <mark> ... %dicttomark <dict> */
DEFUNC_OPER (protected_dicttomark)
{
	hg_quark_t qk, qv, qd, arg0;
	hg_usize_t i, j;
	hg_dict_t *dict;

	/* %dicttomark is the same to {counttomark dup 2 mod 0 ne {pop errordict /rangecheck get />> exch exec} if 2 idiv dup dict begin {def} repeat pop currentdict end} */
	if (!hg_operator_invoke(HG_QOPER (HG_enc_counttomark), vm))
		return FALSE;
	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0)) {
		hg_stack_drop(ostack);
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	j = HG_INT (arg0);
	if ((j % 2) != 0) {
		hg_stack_drop(ostack);
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	qd = hg_dict_new(hg_vm_get_mem(vm),
			 j / 2,
			 hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1,
			 (hg_pointer_t *)&dict);
	if (qd == Qnil) {
		hg_stack_drop(ostack);
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	hg_vm_quark_set_default_acl(vm, &qd);
	for (i = j; i > 0; i -= 2) {
		qk = hg_stack_index(ostack, i);
		qv = hg_stack_index(ostack, i - 1);
		hg_dict_add(dict, qk, qv, FALSE);
	}
	for (i = 0; i <= (j + 1); i++)
		hg_stack_drop(ostack);

	HG_VM_UNLOCK (vm, qd);

	STACK_PUSH (ostack, qd);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-j);
} DEFUNC_OPER_END

/* <initial> <increment> <limit> <proc> %for_yield_int_continue - */
DEFUNC_OPER (protected_for_yield_int_continue)
{
	hg_quark_t self, proc, limit, inc, qq;
	hg_quark_t *init = NULL;
	hg_int_t iinit, iinc, ilimit;

	self  = hg_stack_index(estack, 0);
	proc  = hg_stack_index(estack, 1);
	limit = hg_stack_index(estack, 2);
	inc   = hg_stack_index(estack, 3);
	init  = hg_stack_peek(estack, 4);

	iinit  = HG_INT (*init);
	iinc   = HG_INT (inc);
	ilimit = HG_INT (limit);

	if ((iinc > 0 && iinit > ilimit) ||
	    (iinc < 0 && iinit < ilimit)) {
		hg_stack_roll(estack, 5, 1);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		SET_EXPECTED_STACK_SIZE (0, -4, 0);
		retval = TRUE;

		break;
	} else {
		SET_EXPECTED_STACK_SIZE (1, 2, 0);
	}

	qq = hg_vm_quark_copy(vm, proc, NULL);
	if (qq == Qnil) {
		return FALSE;
	}

	STACK_PUSH (ostack, *init);
	STACK_PUSH (estack, qq);
	STACK_PUSH (estack, self); /* dummy */

	*init = HG_QINT (iinit + iinc);

	retval = TRUE;
} DEFUNC_OPER_END

/* <initial> <increment> <limit> <proc> %for_yield_real_continue - */
DEFUNC_OPER (protected_for_yield_real_continue)
{
	hg_quark_t self, proc, limit, inc, qq;
	hg_quark_t *init = NULL;
	hg_real_t dinit, dinc, dlimit;

	self  = hg_stack_index(estack, 0);
	proc  = hg_stack_index(estack, 1);
	limit = hg_stack_index(estack, 2);
	inc   = hg_stack_index(estack, 3);
	init  = hg_stack_peek(estack, 4);

	dinit  = HG_REAL (*init);
	dinc   = HG_REAL (inc);
	dlimit = HG_REAL (limit);

	if ((dinc > 0.0 && dinit > dlimit) ||
	    (dinc < 0.0 && dinit < dlimit)) {
		hg_stack_roll(estack, 5, 1);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		SET_EXPECTED_STACK_SIZE (0, -4, 0);
		retval = TRUE;

		break;
	} else {
		SET_EXPECTED_STACK_SIZE (1, 2, 0);
	}

	qq = hg_vm_quark_copy(vm, proc, NULL);
	if (qq == Qnil) {
		return FALSE;
	}

	STACK_PUSH (ostack, *init);
	STACK_PUSH (estack, qq);
	STACK_PUSH (estack, self); /* dummy */

	*init = HG_QREAL (dinit + dinc);

	retval = TRUE;
} DEFUNC_OPER_END

/* <int> <array> <proc> %forall_array_continue - */
DEFUNC_OPER (protected_forall_array_continue)
{
	hg_quark_t self, proc, val, q, qq;
	hg_quark_t *n = NULL;
	hg_array_t *a;
	hg_usize_t i;

	self = hg_stack_index(estack, 0);
	proc = hg_stack_index(estack, 1);
	val = hg_stack_index(estack, 2);
	n = hg_stack_peek(estack, 3);

	a = HG_VM_LOCK (vm, val);
	if (a == NULL) {
		return FALSE;
	}
	i = HG_INT (*n);
	if (hg_array_maxlength(a) <= i) {
		HG_VM_UNLOCK (vm, val);
		hg_stack_roll(estack, 4, 1);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);

		retval = TRUE;
		SET_EXPECTED_STACK_SIZE (0, -3, 0);
		break;
	} else {
		SET_EXPECTED_STACK_SIZE (1, 2, 0);
	}
	*n = HG_QINT (i + 1);

	qq = hg_array_get(a, i);
	HG_VM_UNLOCK (vm, val);

	q = hg_vm_quark_copy(vm, proc, NULL);
	if (q == Qnil) {
		return FALSE;
	}

	STACK_PUSH (ostack, qq);
	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} DEFUNC_OPER_END

/* <int> <dict> <proc> %forall_dict_continue - */
DEFUNC_OPER (protected_forall_dict_continue)
{
	hg_quark_t self, proc, val, q, qk, qv;
	hg_quark_t *n = NULL;
	hg_dict_t *d;
	hg_usize_t i;

	self = hg_stack_index(estack, 0);
	proc = hg_stack_index(estack, 1);
	val = hg_stack_index(estack, 2);
	n = hg_stack_peek(estack, 3);

	d = HG_VM_LOCK (vm, val);
	if (d == NULL) {
		return FALSE;
	}
	i = HG_INT (*n);
	if (hg_dict_length(d) == 0) {
		HG_VM_UNLOCK (vm, val);
		hg_stack_roll(estack, 4, 1);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);

		retval = TRUE;
		SET_EXPECTED_STACK_SIZE (0, -3, 0);
		break;
	} else {
		SET_EXPECTED_STACK_SIZE (2, 2, 0);
	}
	*n = HG_QINT (i + 1);

	if (!hg_dict_first_item(d, &qk, &qv)) {
		HG_VM_UNLOCK (vm, val);

		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	hg_dict_remove(d, qk);
	HG_VM_UNLOCK (vm, val);

	q = hg_vm_quark_copy(vm, proc, NULL);
	if (q == Qnil) {
		return FALSE;
	}

	STACK_PUSH (ostack, qk);
	STACK_PUSH (ostack, qv);
	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} DEFUNC_OPER_END

/* <int> <string> <proc> %forall_string_continue - */
DEFUNC_OPER (protected_forall_string_continue)
{
	hg_quark_t self, proc, val, q, qq;
	hg_quark_t *n = NULL;
	hg_string_t *s;
	hg_usize_t i;

	self = hg_stack_index(estack, 0);
	proc = hg_stack_index(estack, 1);
	val = hg_stack_index(estack, 2);
	n = hg_stack_peek(estack, 3);

	s = HG_VM_LOCK (vm, val);
	if (s == NULL) {
		return FALSE;
	}
	i = HG_INT (*n);
	if (hg_string_length(s) <= i) {
		HG_VM_UNLOCK (vm, val);
		hg_stack_roll(estack, 4, 1);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);

		retval = TRUE;
		SET_EXPECTED_STACK_SIZE (0, -3, 0);
		break;
	} else {
		SET_EXPECTED_STACK_SIZE (1, 2, 0);
	}
	*n = HG_QINT (i + 1);

	qq = HG_QINT (hg_string_index(s, i));
	HG_VM_UNLOCK (vm, val);

	q = hg_vm_quark_copy(vm, proc, NULL);
	if (q == Qnil) {
		return FALSE;
	}

	STACK_PUSH (ostack, qq);
	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} DEFUNC_OPER_END

/* %loop_continue */
DEFUNC_OPER (protected_loop_continue)
{
	hg_quark_t self, qproc, q;

	self = hg_stack_index(estack, 0);
	qproc = hg_stack_index(estack, 1);

	q = hg_vm_quark_copy(vm, qproc, NULL);
	if (q == Qnil) {
		return FALSE;
	}

	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
	SET_EXPECTED_ESTACK_SIZE (2);
} DEFUNC_OPER_END

/* <n> <proc> %repeat_continue - */
DEFUNC_OPER (protected_repeat_continue)
{
	hg_quark_t *arg0 = NULL;
	hg_quark_t arg1, self, q;

	arg0 = hg_stack_peek(estack, 2);
	arg1 = hg_stack_index(estack, 1);
	self = hg_stack_index(estack, 0);

	if (HG_INT (*arg0) > 0) {
		*arg0 = HG_QINT (HG_INT (*arg0) - 1);

		q = hg_vm_quark_copy(vm, arg1, NULL);
		if (q == Qnil) {
			return FALSE;
		}
		STACK_PUSH (estack, q);
		SET_EXPECTED_ESTACK_SIZE (2);
	} else {
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		hg_stack_drop(estack);
		SET_EXPECTED_ESTACK_SIZE (-2);
	}

	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} DEFUNC_OPER_END

/* %stopped_continue */
DEFUNC_OPER (protected_stopped_continue)
{
	hg_dict_t *dict;
	hg_quark_t q, qn;
	hg_bool_t ret = FALSE;

	dict = HG_VM_LOCK (vm, vm->qerror);
	if (dict == NULL) {
		return FALSE;
	}
	qn = HG_QNAME (".stopped");
	q = hg_dict_lookup(dict, qn);

	if (q != Qnil &&
	    HG_IS_QBOOL (q) &&
	    HG_BOOL (q)) {
		hg_dict_add(dict, qn, HG_QBOOL (FALSE), FALSE);
		hg_vm_clear_error(vm);
		ret = TRUE;
	}

	HG_VM_UNLOCK (vm, vm->qerror);
	STACK_PUSH (ostack, HG_QBOOL (ret));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ASCII85Decode);
DEFUNC_UNIMPLEMENTED_OPER (ASCII85Encode);
DEFUNC_UNIMPLEMENTED_OPER (ASCIIHexDecode);
DEFUNC_UNIMPLEMENTED_OPER (ASCIIHexEncode);
DEFUNC_UNIMPLEMENTED_OPER (CCITTFaxDecode);
DEFUNC_UNIMPLEMENTED_OPER (CCITTFaxEncode);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedA);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedABC);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedDEF);
DEFUNC_UNIMPLEMENTED_OPER (CIEBasedDEFG);
DEFUNC_UNIMPLEMENTED_OPER (DCTDecode);
DEFUNC_UNIMPLEMENTED_OPER (DCTEncode);
DEFUNC_UNIMPLEMENTED_OPER (DeviceCMYK);
DEFUNC_UNIMPLEMENTED_OPER (DeviceGray);
DEFUNC_UNIMPLEMENTED_OPER (DeviceN);
DEFUNC_UNIMPLEMENTED_OPER (DeviceRGB);
DEFUNC_UNIMPLEMENTED_OPER (FontDirectory);
DEFUNC_UNIMPLEMENTED_OPER (GlobalFontDirectory);
DEFUNC_UNIMPLEMENTED_OPER (ISOLatin1Encoding);
DEFUNC_UNIMPLEMENTED_OPER (Indexed);
DEFUNC_UNIMPLEMENTED_OPER (LZWDecode);
DEFUNC_UNIMPLEMENTED_OPER (LZWEncode);
DEFUNC_UNIMPLEMENTED_OPER (NullEncode);
DEFUNC_UNIMPLEMENTED_OPER (Pattern);
DEFUNC_UNIMPLEMENTED_OPER (RunLengthDecode);
DEFUNC_UNIMPLEMENTED_OPER (RunLengthEncode);
DEFUNC_UNIMPLEMENTED_OPER (Separation);
DEFUNC_UNIMPLEMENTED_OPER (SharedFontDirectory);
DEFUNC_UNIMPLEMENTED_OPER (SubFileDecode);
DEFUNC_UNIMPLEMENTED_OPER (UserObjects);

/* <num1> abs <num2> */
DEFUNC_OPER (abs)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);
	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		*ret = HG_QINT (abs(HG_INT (arg0)));
	} else if (HG_IS_QREAL (arg0)) {
		*ret = HG_QREAL (fabs(HG_REAL (arg0)));
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	retval = TRUE;
} DEFUNC_OPER_END

/* <num1> <num2> add <sum> */
DEFUNC_OPER (add)
{
	hg_quark_t arg0, arg1, *ret;
	hg_real_t d1, d2, dr;
	hg_bool_t is_int = TRUE;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
		is_int = FALSE;
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	dr = d1 + d2;
	if (is_int &&
	    HG_REAL_LE (dr, HG_MAXINT) &&
	    HG_REAL_GE (dr, HG_MININT)) {
		*ret = HG_QINT ((hg_int_t)dr);
	} else {
		*ret = HG_QREAL (dr);
	}
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <array> aload <any> ... <array> */
DEFUNC_OPER (aload)
{
	hg_quark_t arg0, q;
	hg_array_t *a;
	hg_usize_t len, i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (a == NULL) {
		return FALSE;
	}
	hg_stack_pop(ostack);
	len = hg_array_maxlength(a);
	SET_EXPECTED_OSTACK_SIZE (len);
	for (i = 0; i < len; i++) {
		q = hg_array_get(a, i);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto error;
		}
		STACK_PUSH (ostack, q);
	}
	STACK_PUSH (ostack, arg0);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

/* <bool1> <bool2> and <bool3>
 * <int1> <int2> and <int3>
 */
DEFUNC_OPER (and)
{
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QBOOL (arg0) &&
	    HG_IS_QBOOL (arg1)) {
		*ret = HG_QBOOL (HG_BOOL (arg0) & HG_BOOL (arg1));
	} else if (HG_IS_QINT (arg0) &&
		   HG_IS_QINT (arg1)) {
		*ret = HG_QINT (HG_INT (arg0) & HG_INT (arg1));
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <x> <y> <r> <angle1> <angle2> arc - */
DEFUNC_OPER (arc)
{
	hg_quark_t arg0, arg1, arg2, arg3, arg4, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t dx, dy, dr, dangle1, dangle2, dc = 2.0 * M_PI / 360;

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4);
	arg1 = hg_stack_index(ostack, 3);
	arg2 = hg_stack_index(ostack, 2);
	arg3 = hg_stack_index(ostack, 1);
	arg4 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg2)) {
		dr = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dr = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg3)) {
		dangle1 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dangle1 = HG_REAL (arg3);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg4)) {
		dangle2 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dangle2 = HG_REAL (arg4);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_arc(gstate, dx, dy, dr,
			       dangle1 * dc,
			       dangle2 * dc);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-5);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <x> <y> <r> <angle1> <angle2> arcn - */
DEFUNC_OPER (arcn)
{
	hg_quark_t arg0, arg1, arg2, arg3, arg4, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t dx, dy, dr, dangle1, dangle2, dc = 2.0 * M_PI / 360;

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4);
	arg1 = hg_stack_index(ostack, 3);
	arg2 = hg_stack_index(ostack, 2);
	arg3 = hg_stack_index(ostack, 1);
	arg4 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg2)) {
		dr = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dr = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg3)) {
		dangle1 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dangle1 = HG_REAL (arg3);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg4)) {
		dangle2 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dangle2 = HG_REAL (arg4);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_arcn(gstate, dx, dy, dr,
				dangle1 * dc,
				dangle2 * dc);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-5);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <x1> <y1> <x2> <y2> <r> arct - */
DEFUNC_OPER (arct)
{
	hg_quark_t arg0, arg1, arg2, arg3, arg4, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t dx1, dy1, dx2, dy2, dr;

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4);
	arg1 = hg_stack_index(ostack, 3);
	arg2 = hg_stack_index(ostack, 2);
	arg3 = hg_stack_index(ostack, 1);
	arg4 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg4)) {
		dr = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dr = HG_REAL (arg4);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_arcto(gstate, dx1, dy1, dx2, dy2, dr, NULL);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-5);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <x1> <y1> <x2> <y2> <r> arcto <xt1> <yt1> <xt2> <yt2> */
DEFUNC_OPER (arcto)
{
	hg_quark_t arg0, arg1, arg2, arg3, arg4, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t dx1, dy1, dx2, dy2, dr, dt[4];

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4);
	arg1 = hg_stack_index(ostack, 3);
	arg2 = hg_stack_index(ostack, 2);
	arg3 = hg_stack_index(ostack, 1);
	arg4 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg4)) {
		dr = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dr = HG_REAL (arg4);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_arcto(gstate, dx1, dy1, dx2, dy2, dr, dt);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, HG_QREAL (dt[0]));
	STACK_PUSH (ostack, HG_QREAL (dt[1]));
	STACK_PUSH (ostack, HG_QREAL (dt[2]));
	STACK_PUSH (ostack, HG_QREAL (dt[3]));
	SET_EXPECTED_OSTACK_SIZE (-1);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <int> array <array> */
DEFUNC_OPER (array)
{
	hg_quark_t arg0, q, *ret;
	hg_mem_t *m;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_INT (arg0) < 0 ||
	    HG_INT (arg0) > 65535) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	m = hg_vm_get_mem(vm);
	q = hg_array_new(m, HG_INT (arg0), NULL);
	if (q == Qnil) {
		return FALSE;
	}
	*ret = q;
	hg_mem_unref(m, q);

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ashow);

/* <any> ... <array> astore <array> */
DEFUNC_OPER (astore)
{
	hg_quark_t arg0, q;
	hg_array_t *a;
	hg_usize_t len, i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (a == NULL) {
		return FALSE;
	}
	len = hg_array_maxlength(a);
	SET_EXPECTED_OSTACK_SIZE (-len);
	if (hg_stack_depth(ostack) < (len + 1)) {
		HG_VM_UNLOCK (vm, arg0);
		hg_error_return (HG_STATUS_FAILED, HG_e_stackunderflow);
	}
	for (i = 0; i < len; i++) {
		q = hg_stack_index(ostack, len - i);
		if (!hg_array_set(a, q, i, FALSE)) {
			HG_VM_UNLOCK (vm, arg0);
			return FALSE;
		}
	}
	for (i = 0; i <= len; i++) {
		if (i == 0)
			hg_stack_pop(ostack);
		else
			hg_stack_drop(ostack);
	}

	STACK_PUSH (ostack, arg0);

	retval = TRUE;
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

/* <num> <den> atan <angle> */
DEFUNC_OPER (atan)
{
	hg_quark_t arg0, arg1, *ret;
	double num, den, angle;

	CHECK_STACK (ostack, 2);
	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0)) {
		num = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		num = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		den = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		den = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_REAL_IS_ZERO (num) && HG_REAL_IS_ZERO (den)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefinedresult);
	}
	angle = atan2(num, den) / (2 * M_PI / 360);
	if (angle < 0)
		angle = 360.0 + angle;
	*ret = HG_QREAL (angle);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (awidthshow);
DEFUNC_UNIMPLEMENTED_OPER (banddevice);

/* <dict> begin - */
DEFUNC_OPER (begin)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QDICT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	hg_stack_pop(ostack);
	STACK_PUSH (dstack, arg0);

	retval = TRUE;
	SET_EXPECTED_STACK_SIZE (-1, 0, 1);
} DEFUNC_OPER_END

/* <proc> bind <proc> */
DEFUNC_OPER (bind)
{
	hg_quark_t arg0, q;
	hg_array_t *a;
	hg_usize_t len, i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		/* ignore it */
		retval = TRUE;
		break;
	}
	a = HG_VM_LOCK (vm, arg0);
	if (a == NULL) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		goto error;
	}
	len = hg_array_maxlength(a);
	for (i = 0; i < len; i++) {
		q = hg_array_get(a, i);
		if (hg_vm_quark_is_executable(vm, &q)) {
			if (HG_IS_QARRAY (q)) {
				STACK_PUSH (ostack, q);
				_hg_operator_real_bind(vm);
				hg_vm_quark_set_writable(vm, &q, FALSE);
				if (!hg_array_set(a, q, i, TRUE))
					goto error;
				hg_stack_drop(ostack);
			} else if (HG_IS_QNAME (q)) {
				hg_quark_t qop = hg_vm_dstack_lookup(vm, q);

				if (HG_IS_QOPER (qop)) {
					if (!hg_array_set(a, qop, i, TRUE))
						goto error;
				}
			}
		}
	}
	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

/* <int1> <shiftt> bitshift <int2> */
DEFUNC_OPER (bitshift)
{
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_INT (arg1) < 0) {
		*ret = HG_QINT (HG_INT (arg0) >> -HG_INT (arg1));
	} else {
		*ret = HG_QINT (HG_INT (arg0) << HG_INT (arg1));
	}
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <file> bytesavailable <int> */
DEFUNC_OPER (bytesavailable)
{
	hg_quark_t arg0, *ret;
	hg_file_t *f;
	hg_size_t cur_pos;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	f = HG_VM_LOCK (vm, arg0);

	cur_pos = hg_file_seek(f, 0, HG_FILE_POS_CURRENT);
	*ret = HG_QINT (hg_file_seek(f, 0, HG_FILE_POS_END));
	hg_file_seek(f, cur_pos, HG_FILE_POS_BEGIN);

	HG_VM_UNLOCK (vm, arg0);

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (cachestatus);

/* <num1> ceiling <num2> */
DEFUNC_OPER (ceiling)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		/* nothing to do */
	} else if (HG_IS_QREAL (arg0)) {
		*ret = HG_QREAL (ceil(HG_REAL (arg0)));
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (charpath);

/* - clear - */
DEFUNC_OPER (clear)
{
	hg_stack_clear(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-__hg_stack_odepth);
} DEFUNC_OPER_END

/* - cleardictstack - */
DEFUNC_OPER (cleardictstack)
{
	hg_usize_t ddepth = hg_stack_depth(dstack);
	hg_int_t i, j;

	if (hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1) {
		j = ddepth - 2;
	} else {
		j = ddepth - 3;
	}
	for (i = 0; i < j; i++) {
		hg_stack_drop(dstack);
	}

	retval = TRUE;
	SET_EXPECTED_DSTACK_SIZE (-j);
} DEFUNC_OPER_END

/* <mark> ... cleartomark - */
DEFUNC_OPER (cleartomark)
{
	hg_usize_t depth = hg_stack_depth(ostack), i, j;
	hg_quark_t q;

	for (i = 0; i < depth; i++) {
		q = hg_stack_index(ostack, i);
		if (HG_IS_QMARK (q)) {
			for (j = 0; j <= i; j++) {
				hg_stack_drop(ostack);
			}
			retval = TRUE;
			SET_EXPECTED_OSTACK_SIZE (-(i + 1));
			break;
		}
	}
	if (i == depth)
		hg_error_return (HG_STATUS_FAILED, HG_e_unmatchedmark);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (clip);

/* - clippath - */
DEFUNC_OPER (clippath)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg);

	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_clippath(gstate);
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <file> closefile - */
DEFUNC_OPER (closefile)
{
	hg_quark_t arg0;
	hg_file_t *f;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	f = HG_VM_LOCK (vm, arg0);
	if (f == NULL) {
		return FALSE;
	}
	hg_file_close(f);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		HG_VM_UNLOCK (vm, arg0);

		return FALSE;
	}
	HG_VM_UNLOCK (vm, arg0);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* - closepath - */
DEFUNC_OPER (closepath)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_closepath(gstate);
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (colorimage);

/* <matrix> concat - */
DEFUNC_OPER (concat)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_array_t *a = NULL;
	hg_gstate_t *gstate = NULL;
	hg_matrix_t m1, m2;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (!a)
		return FALSE;
	if (!hg_array_is_matrix(a)) {
		HG_VM_UNLOCK (vm, arg0);
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	hg_array_to_matrix(a, &m1);
	gstate = HG_VM_LOCK (vm, qg);
	if (!gstate)
		goto finalize;
	hg_gstate_get_ctm(gstate, &m2);

	hg_matrix_multiply(&m1, &m2, &m1);

	hg_gstate_set_ctm(gstate, &m1);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
  finalize:
	if (gstate)
		HG_VM_UNLOCK (vm, qg);
	if (a)
		HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

/* <matrix1> <matrix2> <matrix3> concatmatrix <matrix3> */
DEFUNC_OPER (concatmatrix)
{
	hg_quark_t arg0, arg1, arg2;
	hg_array_t *a1, *a2, *a3;
	hg_matrix_t m1, m2, m3;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0) ||
	    !HG_IS_QARRAY (arg1) ||
	    !HG_IS_QARRAY (arg2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1) ||
	    !hg_vm_quark_is_writable(vm, &arg2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	a1 = HG_VM_LOCK (vm, arg0);
	a2 = HG_VM_LOCK (vm, arg1);
	a3 = HG_VM_LOCK (vm, arg2);

	if (hg_array_maxlength(a1) != 6 ||
	    hg_array_maxlength(a2) != 6 ||
	    hg_array_maxlength(a3) != 6) {
		e = HG_e_rangecheck;
		goto error;
	}
	if (!hg_array_is_matrix(a1) ||
	    !hg_array_is_matrix(a2)) {
		e = HG_e_typecheck;
		goto error;
	}
	hg_array_to_matrix(a1, &m1);
	hg_array_to_matrix(a2, &m2);
	hg_matrix_multiply(&m1, &m2, &m3);
	hg_array_from_matrix(a3, &m3);

	hg_stack_roll(ostack, 3, 1);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	HG_VM_UNLOCK (vm, arg2);
	HG_VM_UNLOCK (vm, arg1);
	HG_VM_UNLOCK (vm, arg0);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (condition);

static hg_bool_t
_hg_operator_copy_real_traverse_dict(hg_mem_t     *mem,
				     hg_quark_t    qkey,
				     hg_quark_t    qval,
				     hg_pointer_t  data)
{
	hg_dict_t *d2 = (hg_dict_t *)data;

	return hg_dict_add(d2, qkey, qval, TRUE);
}

/* <any> ... <n> copy <any> ...
 * <array1> <array2> copy <subarray2>
 * <dict1> <dict2> copy <dict2>
 * <string1> <string2> copy <substring2>
 * <packedarray> <array> copy <subarray2>
 * <gstate1> <gstate2> copy <gstate2>
 */
DEFUNC_OPER (copy)
{
	hg_quark_t arg0, arg1, q = Qnil;
	hg_usize_t i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0)) {
		hg_usize_t n = HG_INT (arg0);

		if (n < 0 || n >= hg_stack_depth(ostack)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
		}
		hg_stack_drop(ostack);
		for (i = 0; i < n; i++) {
			if (!hg_stack_push(ostack, hg_stack_index(ostack, n - 1))) {
				hg_error_return (HG_STATUS_FAILED, HG_e_stackoverflow);
			}
		}
		retval = TRUE;
		SET_EXPECTED_OSTACK_SIZE (n != 0 ? n - 1 : -1);
	} else {
		CHECK_STACK (ostack, 2);

		arg1 = arg0;
		arg0 = hg_stack_index(ostack, 1);
		if (HG_IS_QARRAY (arg0) &&
		    HG_IS_QARRAY (arg1)) {
			hg_array_t *a1, *a2;
			hg_usize_t len1, len2;

			if (!hg_vm_quark_is_readable(vm, &arg0) ||
			    !hg_vm_quark_is_writable(vm, &arg1)) {
				hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
			}

			a1 = HG_VM_LOCK (vm, arg0);
			a2 = HG_VM_LOCK (vm, arg1);
			if (a1 == NULL || a2 == NULL) {
				goto a_error;
			}
			len1 = hg_array_maxlength(a1);
			len2 = hg_array_maxlength(a2);
			if (len1 > len2) {
				HG_VM_UNLOCK (vm, arg0);
				HG_VM_UNLOCK (vm, arg1);

				hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
			}
			for (i = 0; i < len1; i++) {
				hg_quark_t qq;

				qq = hg_array_get(a1, i);
				if (!hg_array_set(a2, qq, i, TRUE))
					goto a_error;
			}
			if (len2 > len1) {
				hg_quark_acl_t acl = 0;

				q = hg_array_make_subarray(a2, 0, len1 - 1, NULL);
				if (q == Qnil) {
					hg_error_t e = hg_errno;

					HG_VM_UNLOCK (vm, arg0);
					HG_VM_UNLOCK (vm, arg1);
					hg_errno = e;

					return FALSE;
				}
				if (hg_vm_quark_is_readable(vm, &arg1))
					acl |= HG_ACL_READABLE;
				if (hg_vm_quark_is_writable(vm, &arg1))
					acl |= HG_ACL_WRITABLE;
				if (hg_vm_quark_is_executable(vm, &arg1))
					acl |= HG_ACL_EXECUTABLE;
				if (hg_vm_quark_is_accessible(vm, &arg1))
					acl |= HG_ACL_ACCESSIBLE;
				hg_vm_quark_set_acl(vm, &q, acl);
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

			if (!hg_vm_quark_is_readable(vm, &arg0) ||
			    !hg_vm_quark_is_writable(vm, &arg1)) {
				hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
			}

			d1 = HG_VM_LOCK (vm, arg0);
			d2 = HG_VM_LOCK (vm, arg1);
			if (d1 == NULL || d2 == NULL) {
				goto d_error;
			}
			if (hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1) {
				if (hg_dict_length(d2) != 0 ||
				    hg_dict_maxlength(d1) != hg_dict_maxlength(d2)) {
					HG_VM_UNLOCK (vm, arg0);
					HG_VM_UNLOCK (vm, arg1);

					hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
				}
			}
			hg_dict_foreach(d1, _hg_operator_copy_real_traverse_dict, d2);
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
			hg_usize_t len1, mlen1, mlen2;

			if (!hg_vm_quark_is_readable(vm, &arg0) ||
			    !hg_vm_quark_is_writable(vm, &arg1)) {
				hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
			}

			s1 = HG_VM_LOCK (vm, arg0);
			s2 = HG_VM_LOCK (vm, arg1);
			if (s1 == NULL || s2 == NULL) {
				goto s_error;
			}
			len1 = hg_string_length(s1);
			mlen1 = hg_string_maxlength(s1);
			mlen2 = hg_string_maxlength(s2);
			if (mlen1 > mlen2) {
				HG_VM_UNLOCK (vm, arg0);
				HG_VM_UNLOCK (vm, arg1);

				hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
			}
			for (i = 0; i < len1; i++) {
				hg_char_t c = hg_string_index(s1, i);

				hg_string_overwrite_c(s2, c, i);
			}
			if (mlen2 > mlen1) {
				hg_quark_acl_t acl = 0;

				q = hg_string_make_substring(s2, 0, len1 - 1, NULL);
				if (q == Qnil) {
					goto s_error;
				}
				if (hg_vm_quark_is_readable(vm, &arg1))
					acl |= HG_ACL_READABLE;
				if (hg_vm_quark_is_writable(vm, &arg1))
					acl |= HG_ACL_WRITABLE;
				if (hg_vm_quark_is_executable(vm, &arg1))
					acl |= HG_ACL_EXECUTABLE;
				if (hg_vm_quark_is_accessible(vm, &arg1))
					acl |= HG_ACL_ACCESSIBLE;
				hg_vm_quark_set_acl(vm, &q, acl);
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
			hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		}

		if (retval) {
			hg_stack_drop(ostack);
			hg_stack_drop(ostack);

			STACK_PUSH (ostack, q);
			SET_EXPECTED_OSTACK_SIZE (-1);
		}
	}
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (copypage);

/* <angle> cos <real> */
DEFUNC_OPER (cos)
{
	hg_quark_t arg0, *ret;
	hg_real_t angle;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		angle = (hg_real_t)HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		angle = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	*ret = HG_QREAL (cos(angle / 180 * M_PI));

	retval = TRUE;
} DEFUNC_OPER_END

/* - count <int> */
DEFUNC_OPER (count)
{
	STACK_PUSH (ostack, HG_QINT (hg_stack_depth(ostack)));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* - countdictstack <int> */
DEFUNC_OPER (countdictstack)
{
	STACK_PUSH (ostack, HG_QINT (hg_stack_depth(dstack)));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* - countexecstack <int> */
DEFUNC_OPER (countexecstack)
{
	STACK_PUSH (ostack, HG_QINT (hg_stack_depth(estack)));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* <mark> ... counttomark <int> */
DEFUNC_OPER (counttomark)
{
	hg_usize_t i, depth = hg_stack_depth(ostack);
	hg_quark_t q = Qnil;

	for (i = 0; i < depth; i++) {
		hg_quark_t qq = hg_stack_index(ostack, i);

		if (HG_IS_QMARK (qq)) {
			q = HG_QINT (i);
			break;
		}
	}
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_unmatchedmark);
	}

	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (cshow);
DEFUNC_UNIMPLEMENTED_OPER (currentblackgeneration);
DEFUNC_UNIMPLEMENTED_OPER (currentcacheparams);
DEFUNC_UNIMPLEMENTED_OPER (currentcmykcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentcolorrendering);
DEFUNC_UNIMPLEMENTED_OPER (currentcolorscreen);
DEFUNC_UNIMPLEMENTED_OPER (currentcolortransfer);
DEFUNC_UNIMPLEMENTED_OPER (currentcontext);
DEFUNC_UNIMPLEMENTED_OPER (currentdash);
DEFUNC_UNIMPLEMENTED_OPER (currentdevparams);

/* - currentdict <dict> */
DEFUNC_OPER (currentdict)
{
	hg_quark_t q;

	q = hg_stack_index(dstack, 0);
	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* - currentfile <file> */
DEFUNC_OPER (currentfile)
{
	hg_quark_t q = Qnil;
	hg_usize_t i, edepth = hg_stack_depth(estack);

	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i);
		if (HG_IS_QFILE (q))
			break;
		q = Qnil;
	}
	if (q == Qnil) {
		hg_file_t *f;

		q = hg_file_new_with_string(hg_vm_get_mem(vm), "%nofile",
					    HG_FILE_IO_MODE_READ,
					    NULL, NULL,
					    (hg_pointer_t *)&f);
		hg_file_close(f);
		HG_VM_UNLOCK (vm, q);
	}
	STACK_PUSH (ostack, q);
	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (currentflat);
DEFUNC_UNIMPLEMENTED_OPER (currentfont);
DEFUNC_UNIMPLEMENTED_OPER (currentglobal);
DEFUNC_UNIMPLEMENTED_OPER (currentgray);
DEFUNC_UNIMPLEMENTED_OPER (currentgstate);
DEFUNC_UNIMPLEMENTED_OPER (currenthalftone);
DEFUNC_UNIMPLEMENTED_OPER (currenthalftonephase);
DEFUNC_UNIMPLEMENTED_OPER (currenthsbcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentlinecap);
DEFUNC_UNIMPLEMENTED_OPER (currentlinejoin);
DEFUNC_UNIMPLEMENTED_OPER (currentlinewidth);

/* <matrix> currentmatrix <matrix> */
DEFUNC_OPER (currentmatrix)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_array_t *a = NULL;
	hg_matrix_t m;
	hg_gstate_t *gstate = NULL;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (!a)
		return FALSE;

	if (hg_array_maxlength(a) != 6) {
		HG_VM_UNLOCK (vm, arg0);

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (!gstate)
		goto finalize;

	hg_gstate_get_ctm(gstate, &m);
	hg_array_from_matrix(a, &m);

	retval = TRUE;
  finalize:
	if (gstate)
		HG_VM_UNLOCK (vm, qg);
	if (a)
		HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (currentmiterlimit);
DEFUNC_UNIMPLEMENTED_OPER (currentobjectformat);
DEFUNC_UNIMPLEMENTED_OPER (currentoverprint);
DEFUNC_UNIMPLEMENTED_OPER (currentpacking);
DEFUNC_UNIMPLEMENTED_OPER (currentpagedevice);

/* - currentpoint <x> <y> */
DEFUNC_OPER (currentpoint)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg);
	hg_real_t x, y;

	if (!gstate)
		return FALSE;
	retval = hg_gstate_get_current_point(gstate, &x, &y);
	if (!retval)
		goto finalize;

	STACK_PUSH (ostack, HG_QREAL (x));
	STACK_PUSH (ostack, HG_QREAL (y));

	SET_EXPECTED_OSTACK_SIZE (2);
  finalize:
	if (gstate)
		HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (currentrgbcolor);
DEFUNC_UNIMPLEMENTED_OPER (currentscreen);
DEFUNC_UNIMPLEMENTED_OPER (currentcolorspace);
DEFUNC_UNIMPLEMENTED_OPER (currentshared);
DEFUNC_UNIMPLEMENTED_OPER (currentstrokeadjust);
DEFUNC_UNIMPLEMENTED_OPER (currentsystemparams);
DEFUNC_UNIMPLEMENTED_OPER (currenttransfer);
DEFUNC_UNIMPLEMENTED_OPER (currentundercolorremoval);

/* - currentuserparams <dict> */
DEFUNC_OPER (currentuserparams)
{
	hg_quark_t q;

	q = hg_vm_get_user_params(vm, NULL);
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}

	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* <x1> <y1> <x2> <y2> <x3> <y3> curveto - */
DEFUNC_OPER (curveto)
{
	hg_quark_t arg0, arg1, arg2, arg3, arg4, arg5, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t dx1, dy1, dx2, dy2, dx3, dy3;

	CHECK_STACK (ostack, 6);

	arg0 = hg_stack_index(ostack, 5);
	arg1 = hg_stack_index(ostack, 4);
	arg2 = hg_stack_index(ostack, 3);
	arg3 = hg_stack_index(ostack, 2);
	arg4 = hg_stack_index(ostack, 1);
	arg5 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg4)) {
		dx3 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dx3 = HG_REAL (arg4);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg5)) {
		dy3 = HG_INT (arg5);
	} else if (HG_IS_QREAL (arg5)) {
		dy3 = HG_REAL (arg5);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	retval = hg_gstate_curveto(gstate, dx1, dy1, dx2, dy2, dx3, dy3);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-6);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <num> cvi <int>
 * <string> cvi <int>
 */
DEFUNC_OPER (cvi)
{
	hg_quark_t arg0, q, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		/* nothing to do */
		retval = TRUE;
	} else if (HG_IS_QREAL (arg0)) {
		hg_real_t d = HG_REAL (arg0);

		if (d > HG_MAXINT || d < HG_MININT) {
			hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
		}
		if (isinf(d)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
		}
		*ret = HG_QINT (d);

		retval = TRUE;
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;

		if (!hg_vm_quark_is_readable(vm, &arg0)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
		}
		s = HG_VM_LOCK (vm, arg0);
		q = hg_file_new_with_string(hg_vm_get_mem(vm),
					    "--%cvi--",
					    HG_FILE_IO_MODE_READ,
					    s,
					    NULL,
					    NULL);
		HG_VM_UNLOCK (vm, arg0);
		STACK_PUSH (ostack, q);
		retval = _hg_operator_real_token(vm);
		if (retval == TRUE) {
			hg_quark_t qq, qv;

			qq = hg_stack_pop(ostack);
			if (!HG_IS_QBOOL (qq)) {
				hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
			}
			if (!HG_BOOL (qq)) {
				hg_error_return (HG_STATUS_FAILED, HG_e_syntaxerror);
			}
			qv = hg_stack_pop(ostack);
			if (HG_IS_QINT (qv)) {
				*ret = qv;
			} else if (HG_IS_QREAL (qv)) {
				hg_real_t d = HG_REAL (qv);

				if (d < HG_MININT ||
				    d > HG_MAXINT) {
					hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
				}
				*ret = HG_QINT ((hg_int_t)d);
			} else {
				hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
			}

			retval = TRUE;
		} else {
			hg_stack_drop(ostack);

			STACK_PUSH (ostack, qself);
		}
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
} DEFUNC_OPER_END

/* <any> cvlit <any> */
DEFUNC_OPER (cvlit)
{
	hg_quark_t *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	hg_vm_quark_set_executable(vm, ret, FALSE);

	retval = TRUE;
} DEFUNC_OPER_END

/* <string> cvn <name> */
DEFUNC_OPER (cvn)
{
	hg_quark_t arg0, *ret;
	hg_char_t *cstr;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	cstr = hg_vm_string_get_cstr(vm, arg0);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}
	if (hg_vm_string_length(vm, arg0) > 127) {
		hg_free(cstr);
		hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
	}
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}

	*ret = HG_QNAME (cstr);
	hg_vm_quark_set_executable(vm, ret,
				   hg_vm_quark_is_executable(vm, &arg0));
	hg_free(cstr);

	retval = TRUE;
} DEFUNC_OPER_END

/* <num> cvr <real>
 * <string> cvr <real>
 */
DEFUNC_OPER (cvr)
{
	hg_quark_t arg0, q, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		*ret = HG_QREAL (HG_INT (arg0));

		retval = TRUE;
	} else if (HG_IS_QREAL (arg0)) {
		/* no need to proceed anything */
		retval = TRUE;
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;

		if (!hg_vm_quark_is_readable(vm, &arg0)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
		}
		s = HG_VM_LOCK (vm, arg0);
		q = hg_file_new_with_string(hg_vm_get_mem(vm),
					    "--%cvr--",
					    HG_FILE_IO_MODE_READ,
					    s,
					    NULL,
					    NULL);
		HG_VM_UNLOCK (vm, arg0);
		STACK_PUSH (ostack, q);
		retval = _hg_operator_real_token(vm);
		if (retval == TRUE) {
			hg_quark_t qq, qv;

			qq = hg_stack_pop(ostack);
			if (!HG_IS_QBOOL (qq)) {
				hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
			}
			if (!HG_BOOL (qq)) {
				hg_error_return (HG_STATUS_FAILED, HG_e_syntaxerror);
			}
			qv = hg_stack_pop(ostack);
			if (HG_IS_QINT (qv)) {
				qv = HG_QREAL (HG_INT (qv));
			} else if (HG_IS_QREAL (qv)) {
				if (isinf(HG_REAL (qv))) {
					hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
				}
			} else {
				hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
			}
			*ret = qv;

			retval = TRUE;
		} else {
			/* drop -file- */
			hg_stack_exch(ostack);
			hg_stack_drop(ostack);
			/* We want to notify the place
			 * where the error happened as in cvr operator
			 * but not in -token-
			 */
			hg_stack_drop(ostack);

			STACK_PUSH (ostack, qself);
		}
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
} DEFUNC_OPER_END

/* <num> <radix> <string> cvrs <substring> */
DEFUNC_OPER (cvrs)
{
	hg_quark_t arg0, arg1, arg2, q;
	hg_real_t d1;
	hg_int_t radix;
	hg_string_t *s;
	hg_bool_t is_real = FALSE;
	hg_char_t *cstr = NULL;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);
	if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_real = TRUE;
	} else if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!HG_IS_QINT (arg1) ||
	    !HG_IS_QSTRING (arg2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	radix = HG_INT (arg1);
	if (radix < 2 || radix > 36) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (radix == 10) {
		if (is_real) {
			cstr = hg_strdup_printf("%f", d1);
		} else {
			cstr = hg_strdup_printf("%d", (hg_int_t)d1);
		}
	} else {
		const hg_char_t __radix_to_c[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		GString *str = g_string_new(NULL), *rstr;
		hg_uint_t x = (hg_uint_t)d1;
		hg_usize_t i;

		do {
			g_string_append_c(str, __radix_to_c[x % radix]);
			x /= radix;
		} while (x != 0);

		rstr = g_string_new(NULL);
		for (i = 0; i < str->len; i++) {
			g_string_append_c(rstr, str->str[str->len - i - 1]);
		}
		cstr = hg_strdup(rstr->str);
		g_string_free(str, TRUE);
		g_string_free(rstr, TRUE);
	}
	s = HG_VM_LOCK (vm, arg2);
	if (s == NULL) {
		e = HG_ERROR_GET_REASON (hg_errno);
		goto error;
	}
	if (strlen(cstr) > hg_string_maxlength(s)) {
		e = HG_e_rangecheck;
		goto error;
	}
	hg_string_append(s, cstr, -1);
	if (hg_string_maxlength(s) > hg_string_length(s)) {
		hg_quark_acl_t acl = 0;

		q = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL);
		if (q == Qnil) {
			e = HG_e_VMerror;
			goto error;
		}
		if (hg_vm_quark_is_readable(vm, &arg2))
			acl |= HG_ACL_READABLE;
		if (hg_vm_quark_is_writable(vm, &arg2))
			acl |= HG_ACL_WRITABLE;
		if (hg_vm_quark_is_executable(vm, &arg2))
			acl |= HG_ACL_EXECUTABLE;
		if (hg_vm_quark_is_accessible(vm, &arg2))
			acl |= HG_ACL_ACCESSIBLE;
		hg_vm_quark_set_acl(vm, &q, acl);
	} else {
		q = arg2;
	}
	if (q == arg2)
		hg_stack_pop(ostack);
	else
		hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	if (s)
		HG_VM_UNLOCK (vm, arg2);
	hg_free(cstr);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

/* <any> cvx <any> */
DEFUNC_OPER (cvx)
{
	hg_quark_t *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	hg_vm_quark_set_executable(vm, ret, TRUE);

	retval = TRUE;
} DEFUNC_OPER_END

/* <key> <value> def - */
DEFUNC_OPER (def)
{
	hg_quark_t arg0, arg1, qd;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	qd = hg_stack_index(dstack, 0);

	retval = hg_vm_dict_add(vm, qd, arg0, arg1, FALSE);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

/* <matrix> defaultmatrix <matrix> */
DEFUNC_OPER (defaultmatrix)
{
	hg_quark_t arg0;
	hg_array_t *a;
	hg_matrix_t m;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (a == NULL) {
		return FALSE;
	}
	if (hg_array_maxlength(a) != 6) {
		HG_VM_UNLOCK (vm, arg0);

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	if (!hg_device_get_ctm(vm->device, &m) ||
	    !hg_array_from_matrix(a, &m)) {
		HG_VM_UNLOCK (vm, arg0);

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}

	retval = TRUE;
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (defineresource);
DEFUNC_UNIMPLEMENTED_OPER (defineusername);
DEFUNC_UNIMPLEMENTED_OPER (defineuserobject);
DEFUNC_UNIMPLEMENTED_OPER (deletefile);
DEFUNC_UNIMPLEMENTED_OPER (detach);
DEFUNC_UNIMPLEMENTED_OPER (deviceinfo);

/* <int> dict <dict> */
DEFUNC_OPER (dict)
{
	hg_quark_t arg0, ret;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_INT (arg0) < 0) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	if (HG_INT (arg0) > G_MAXUSHORT) {
		hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
	}
	ret = hg_dict_new(hg_vm_get_mem(vm),
			  HG_INT (arg0),
			  hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1,
			  NULL);
	if (ret == Qnil) {
		return FALSE;
	}
	hg_vm_quark_set_default_acl(vm, &ret);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, ret);

	retval = TRUE;
} DEFUNC_OPER_END

/* <array> dictstack <subarray> */
DEFUNC_OPER (dictstack)
{
	hg_quark_t arg0, q;
	hg_array_t *a;
	hg_size_t i, len, ddepth;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (a == NULL) {
		return FALSE;
	}
	ddepth = hg_stack_depth(dstack);
	len = hg_array_maxlength(a);
	if (ddepth > len) {
		HG_VM_UNLOCK (vm, arg0);

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	for (i = 0; i < ddepth; i++) {
		q = hg_stack_index(dstack, ddepth - i - 1);
		if (!hg_array_set(a, q, i, FALSE))
			goto error;
	}
	if (ddepth != len) {
		hg_quark_acl_t acl = 0;

		q = hg_array_make_subarray(a, 0, ddepth - 1, NULL);
		if (q == Qnil) {
			goto error;
		}
		if (hg_vm_quark_is_readable(vm, &arg0))
			acl |= HG_ACL_READABLE;
		if (hg_vm_quark_is_writable(vm, &arg0))
			acl |= HG_ACL_WRITABLE;
		if (hg_vm_quark_is_executable(vm, &arg0))
			acl |= HG_ACL_EXECUTABLE;
		if (hg_vm_quark_is_accessible(vm, &arg0))
			acl |= HG_ACL_ACCESSIBLE;
		hg_vm_quark_set_acl(vm, &q, acl);
	} else {
		q = arg0;
	}
	if (q == arg0)
		hg_stack_pop(ostack);
	else
		hg_stack_drop(ostack);

	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

/* <num1> <num2> div <quotient> */
DEFUNC_OPER (div)
{
	hg_quark_t arg0, arg1, *ret;
	hg_real_t d1, d2;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_REAL_IS_ZERO (d2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefinedresult);
	}
	*ret = HG_QREAL (d1 / d2);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (dtransform);

/* <any> dup <any> <any> */
DEFUNC_OPER (dup)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	STACK_PUSH (ostack, arg0);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (echo);
DEFUNC_UNIMPLEMENTED_OPER (eexec);

/* - end - */
DEFUNC_OPER (end)
{
	hg_usize_t ddepth = hg_stack_depth(dstack);
	hg_vm_langlevel_t lang = hg_vm_get_language_level(vm);

	if ((lang >= HG_LANG_LEVEL_2 && ddepth <= 3) ||
	    (lang == HG_LANG_LEVEL_1 && ddepth <= 2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_dictstackunderflow);
	}
	hg_stack_drop(dstack);

	retval = TRUE;
	SET_EXPECTED_DSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (eoclip);

/* - eofill - */
DEFUNC_OPER (eofill)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg);

	if (gstate == NULL) {
		return FALSE;
	}
	if (!hg_device_eofill(vm->device, gstate)) {
		HG_VM_UNLOCK (vm, qg);
		if (HG_ERROR_IS_SUCCESS0 ()) {
			hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
		}
		return FALSE;
	}
	HG_VM_UNLOCK (vm, qg);
	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (eoviewclip);

/* <any1> <any2> eq <bool> */
DEFUNC_OPER (eq)
{
	hg_quark_t arg0, arg1, *qret;
	hg_bool_t ret = FALSE;

	CHECK_STACK (ostack, 2);

	qret = hg_stack_peek(ostack, 1);
	arg0 = *qret;
	arg1 = hg_stack_index(ostack, 0);

	if ((HG_IS_QSTRING (arg0) &&
	     !hg_vm_quark_is_readable(vm, &arg0)) ||
	    (HG_IS_QSTRING (arg1) &&
	     !hg_vm_quark_is_readable(vm, &arg1))) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	ret = hg_vm_quark_compare(vm, arg0, arg1);
	if (!ret) {
		if ((HG_IS_QNAME (arg0) ||
		     HG_IS_QEVALNAME (arg0) ||
		     HG_IS_QSTRING (arg0)) &&
		    (HG_IS_QNAME (arg1) ||
		     HG_IS_QEVALNAME (arg1) ||
		     HG_IS_QSTRING (arg1))) {
			hg_char_t *s1 = NULL, *s2 = NULL;

			if (HG_IS_QNAME (arg0) ||
			    HG_IS_QEVALNAME (arg0)) {
				s1 = hg_strdup(HG_NAME (arg0));
			} else {
				s1 = hg_vm_string_get_cstr(vm, arg0);
				if (!HG_ERROR_IS_SUCCESS0 ()) {
					goto s_error;
				}
			}
			if (HG_IS_QNAME (arg1) ||
			    HG_IS_QEVALNAME (arg1)) {
				s2 = hg_strdup(HG_NAME (arg1));
			} else {
				s2 = hg_vm_string_get_cstr(vm, arg1);
				if (!HG_ERROR_IS_SUCCESS0 ()) {
					goto s_error;
				}
			}
			if (s1 != NULL && s2 != NULL)
				ret = (strcmp(s1, s2) == 0);
		  s_error:
			hg_free(s1);
			hg_free(s2);
		} else if ((HG_IS_QINT (arg0) ||
			    HG_IS_QREAL (arg0)) &&
			   (HG_IS_QINT (arg1) ||
			    HG_IS_QREAL (arg1))) {
			hg_real_t d1, d2;

			if (HG_IS_QINT (arg0))
				d1 = HG_INT (arg0);
			else
				d1 = HG_REAL (arg0);
			if (HG_IS_QINT (arg1))
				d2 = HG_INT (arg1);
			else
				d2 = HG_REAL (arg1);

			ret = HG_REAL_EQUAL (d1, d2);
		}
	}
	if (HG_ERROR_IS_SUCCESS0 ()) {
		*qret = HG_QBOOL (ret);
		hg_stack_drop(ostack);

		retval = TRUE;
	}
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (erasepage);

/* <any1> <any2> exch <any2> <any1> */
DEFUNC_OPER (exch)
{
	CHECK_STACK (ostack, 2);

	hg_stack_exch(ostack);

	retval = TRUE;
} DEFUNC_OPER_END

/* <any> exec - */
DEFUNC_OPER (exec)
{
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (hg_vm_quark_is_executable(vm, &arg0)) {
		if (!HG_IS_QOPER (arg0) &&
		    !hg_vm_quark_is_readable(vm, &arg0)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
		}

		q = hg_vm_quark_copy(vm, arg0, NULL);
		if (q == Qnil) {
			return FALSE;
		}
		hg_stack_drop(ostack);

		STACK_PUSH (estack, q);

		hg_stack_exch(estack);
		SET_EXPECTED_STACK_SIZE (-1, 1, 0);
	}

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (execform);

/* <array> execstack <subarray> */
DEFUNC_OPER (execstack)
{
	hg_quark_t arg0, q;
	hg_array_t *a;
	hg_size_t i, len, edepth;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (a == NULL) {
		return FALSE;
	}
	edepth = hg_stack_depth(estack);
	len = hg_array_maxlength(a);
	if (edepth > len) {
		HG_VM_UNLOCK (vm, arg0);

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	/* do not include the last node */
	for (i = 0; i < edepth - 1; i++) {
		q = hg_stack_index(estack, edepth - i - 1);
		if (!hg_array_set(a, q, i, FALSE))
			goto error;
	}
	if (edepth != (len + 1)) {
		hg_quark_acl_t acl = 0;

		q = hg_array_make_subarray(a, 0, edepth - 2, NULL);
		if (q == Qnil) {
			goto error;
		}
		if (hg_vm_quark_is_readable(vm, &arg0))
			acl |= HG_ACL_READABLE;
		if (hg_vm_quark_is_writable(vm, &arg0))
			acl |= HG_ACL_WRITABLE;
		if (hg_vm_quark_is_executable(vm, &arg0))
			acl |= HG_ACL_EXECUTABLE;
		if (hg_vm_quark_is_accessible(vm, &arg0))
			acl |= HG_ACL_ACCESSIBLE;
		hg_vm_quark_set_acl(vm, &q, acl);
	} else {
		q = arg0;
	}
	if (q == arg0)
		hg_stack_pop(ostack);
	else
		hg_stack_drop(ostack);

	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (execuserobject);

/* <array> executeonly <array>
 * <packedarray> executeonly <packedarray>
 * <file> executeonly <file>
 * <string> executeonly <string>
 */
DEFUNC_OPER (executeonly)
{
	hg_quark_t *arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_peek(ostack, 0);
	if (HG_IS_QARRAY (*arg0) ||
	    HG_IS_QFILE (*arg0) ||
	    HG_IS_QSTRING (*arg0)) {
		if (!hg_vm_quark_is_accessible(vm, arg0)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
		}
		hg_vm_quark_set_readable(vm, arg0, FALSE);
		hg_vm_quark_set_writable(vm, arg0, FALSE);
		hg_vm_quark_set_accessible(vm, arg0, TRUE);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	retval = TRUE;
} DEFUNC_OPER_END

/* - exit - */
DEFUNC_OPER (exit)
{
	hg_usize_t i, j, edepth = hg_stack_depth(estack);
	hg_quark_t q;

	/* almost code is shared with .exit.
	 * only difference is to trap on %stopped_continue or not
	 */
	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i);
		if (HG_IS_QOPER (q)) {
			if (HG_OPER (q) == HG_enc_protected_for_yield_int_continue ||
			    HG_OPER (q) == HG_enc_protected_for_yield_real_continue) {
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 4));
			} else if (HG_OPER (q) == HG_enc_protected_loop_continue) {
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 1));
			} else if (HG_OPER (q) == HG_enc_protected_repeat_continue) {
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 2));
			} else if (HG_OPER (q) == HG_enc_protected_forall_array_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_dict_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_string_continue) {
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				hg_stack_drop(estack);
				SET_EXPECTED_ESTACK_SIZE (-(i + 3));
			} else if (HG_OPER (q) == HG_enc_protected_stopped_continue) {
				hg_error_return (HG_STATUS_FAILED, HG_e_invalidexit);
			} else {
				continue;
			}
			for (j = 0; j < i; j++)
				hg_stack_drop(estack);
			break;
		} else if (HG_IS_QFILE (q)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidexit);
		}
	}
	if (i == edepth) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidexit);
	} else {
		retval = TRUE;
	}
} DEFUNC_OPER_END

/* <base> <exponent> exp <real> */
DEFUNC_OPER (exp)
{
	hg_quark_t arg0, arg1, *ret;
	hg_real_t base, exponent;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		base = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		base = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		exponent = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		exponent = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_REAL_IS_ZERO (base) &&
	    HG_REAL_IS_ZERO (exponent)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefinedresult);
	}
	*ret = HG_QREAL (pow(base, exponent));

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <filename> <access> file -file- */
DEFUNC_OPER (file)
{
	hg_quark_t arg0, arg1, q;
	hg_file_io_t iotype;
	hg_file_mode_t iomode;
	hg_char_t *filename = NULL, *fmode = NULL;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	filename = hg_vm_string_get_cstr(vm, arg0);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		goto error;
	}
	fmode = hg_vm_string_get_cstr(vm, arg1);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		goto error;
	}

	iotype = hg_file_get_io_type(filename);
	if (fmode == NULL) {
		e = HG_e_VMerror;
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
		    e = HG_e_invalidfileaccess;
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
			    e = HG_e_invalidfileaccess;
			    goto error;
		}
	} else if (fmode[1] != 0) {
		e = HG_e_invalidfileaccess;
		goto error;
	}
	if (iotype != HG_FILE_IO_FILE) {
		q = hg_vm_get_io(vm, iotype);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto error;
		}
		if (q == Qnil) {
			e = HG_e_undefinedfilename;
			goto error;
		}
	} else {
		hg_quark_acl_t acl = HG_ACL_ACCESSIBLE;

		q = hg_file_new(hg_vm_get_mem(vm),
				filename, iomode, NULL);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto error;
		}
		if (q == Qnil) {
			e = HG_e_undefinedfilename;
			goto error;
		}
		if (iomode & (HG_FILE_IO_MODE_READ|HG_FILE_IO_MODE_APPEND))
			acl |= HG_ACL_READABLE;
		if (iomode & (HG_FILE_IO_MODE_WRITE|HG_FILE_IO_MODE_APPEND))
			acl |= HG_ACL_WRITABLE;
		hg_vm_quark_set_acl(vm, &q, acl);
	}
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
  error:
	hg_free(filename);
	hg_free(fmode);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (filenameforall);
DEFUNC_UNIMPLEMENTED_OPER (fileposition);

/* - fill - */
DEFUNC_OPER (fill)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg);

	if (gstate == NULL) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	if (!hg_device_fill(vm->device, gstate)) {
		hg_error_t e = hg_errno;

		HG_VM_UNLOCK (vm, qg);
		if (HG_ERROR_IS_SUCCESS (e)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
		}
		hg_errno = e;

		return FALSE;
	}
	retval = TRUE;
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (filter);
DEFUNC_UNIMPLEMENTED_OPER (findencoding);
DEFUNC_UNIMPLEMENTED_OPER (findresource);
DEFUNC_UNIMPLEMENTED_OPER (flattenpath);

/* - flush - */
DEFUNC_OPER (flush)
{
	hg_quark_t q;
	hg_file_t *stdout_;

	q = hg_vm_get_io(vm, HG_FILE_IO_STDOUT);
	stdout_ = HG_VM_LOCK (vm, q);
	if (stdout_ == NULL) {
		return FALSE;
	}

	hg_file_flush(stdout_);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		/* ignore error */
		hg_errno = 0;
	}
	retval = TRUE;

	HG_VM_UNLOCK (vm, q);
} DEFUNC_OPER_END

/* <file> flushfile - */
DEFUNC_OPER (flushfile)
{
	hg_quark_t arg0;
	hg_file_t *f;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	f = HG_VM_LOCK (vm, arg0);

	if (!hg_file_flush(f)) {
		goto error;
	}
	retval = TRUE;

	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-1);
  error:
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

/* <initial> <increment> <limit> <proc> for - */
DEFUNC_OPER (for)
{
	hg_quark_t arg0, arg1, arg2, arg3, q;
	hg_real_t dinit, dinc, dlimit;
	hg_bool_t fint = TRUE;

	CHECK_STACK (ostack, 4);

	arg0 = hg_stack_index(ostack, 3);
	arg1 = hg_stack_index(ostack, 2);
	arg2 = hg_stack_index(ostack, 1);
	arg3 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg3) ||
	    !hg_vm_quark_is_executable (vm, &arg3) ||
	    (!HG_IS_QINT (arg2) && !HG_IS_QREAL (arg2)) ||
	    (!HG_IS_QINT (arg1) && !HG_IS_QREAL (arg1)) ||
	    (!HG_IS_QINT (arg0) && !HG_IS_QREAL (arg0))) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QREAL (arg0) ||
	    HG_IS_QREAL (arg1) ||
	    HG_IS_QREAL (arg2))
		fint = FALSE;

	dinit  = (HG_IS_QINT (arg0) ? HG_INT (arg0) : HG_REAL (arg0));
	dinc   = (HG_IS_QINT (arg1) ? HG_INT (arg1) : HG_REAL (arg1));
	dlimit = (HG_IS_QINT (arg2) ? HG_INT (arg2) : HG_REAL (arg2));

	if (fint) {
		q = hg_vm_dict_lookup(vm,
				      vm->qsystemdict,
				      HG_QNAME ("%for_yield_int_continue"),
				      FALSE);
		arg0 = HG_QINT ((hg_int_t)dinit);
		arg1 = HG_QINT ((hg_int_t)dinc);
		arg2 = HG_QINT ((hg_int_t)dlimit);
	} else {
		q = hg_vm_dict_lookup(vm,
				      vm->qsystemdict,
				      HG_QNAME ("%for_yield_real_continue"),
				      FALSE);
		arg0 = HG_QREAL (dinit);
		arg1 = HG_QREAL (dinc);
		arg2 = HG_QREAL (dlimit);
	}
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefined);
	}

	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, arg1);
	STACK_PUSH (estack, arg2);
	STACK_PUSH (estack, arg3);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 6, -1);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_STACK_SIZE (-4, 5, 0);
} DEFUNC_OPER_END

/* <array> <proc> forall -
 * <packedarray> <proc> forall -
 * <dict> <proc> forall -
 * <string> <proc> forall -
 */
DEFUNC_OPER (forall)
{
	hg_quark_t arg0, arg1, q = Qnil, qd;
	hg_dict_t *dict;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_executable(vm, &arg1) ||
	    !hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (HG_IS_QARRAY (arg0)) {
		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME ("%forall_array_continue"),
				      FALSE);
	} else if (HG_IS_QDICT (arg0)) {
		dict = HG_VM_LOCK (vm, arg0);
		if (dict == NULL)
			return FALSE;
		qd = hg_dict_dup(dict, NULL);
		if (qd == Qnil) {
			HG_VM_UNLOCK (vm, arg0);

			hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
		}
		HG_VM_UNLOCK (vm, arg0);

		arg0 = qd;

		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME ("%forall_dict_continue"),
				      FALSE);
	} else if (HG_IS_QSTRING (arg0)) {
		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME ("%forall_string_continue"),
				      FALSE);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefined);
	}

	STACK_PUSH (estack, HG_QINT (0));
	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, arg1);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 5, -1);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_STACK_SIZE (-2, 4, 0);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (fork);
DEFUNC_UNIMPLEMENTED_OPER (framedevice);

/* <any> gcheck <bool> */
DEFUNC_OPER (gcheck)
{
	hg_quark_t arg0, *qret;
	hg_bool_t ret;

	CHECK_STACK (ostack, 1);

	qret = hg_stack_peek(ostack, 0);
	arg0 = *qret;
	if (hg_quark_is_simple_object(arg0) ||
	    HG_IS_QOPER (arg0)) {
		ret = TRUE;
	} else {
		ret = hg_quark_has_mem_id(arg0, vm->mem_id[HG_VM_MEM_GLOBAL]);
	}
	*qret = HG_QBOOL (ret);

	retval = TRUE;
} DEFUNC_OPER_END

/* <num1> <num2> ge <bool>
 * <string1> <string2> ge <bool>
 */
DEFUNC_OPER (ge)
{
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		hg_real_t d1, d2;

		if (HG_IS_QINT (arg0))
			d1 = HG_INT (arg0);
		else
			d1 = HG_REAL (arg0);
		if (HG_IS_QINT (arg1))
			d2 = HG_INT (arg1);
		else
			d2 = HG_INT (arg1);

		q = HG_QBOOL (HG_REAL_GE (d1, d2));
	} else if (HG_IS_QSTRING (arg0) &&
		   HG_IS_QSTRING (arg1)) {
		hg_char_t *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) >= 0);

	  s_error:
		hg_free(cs1);
		hg_free(cs2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack);

		retval = TRUE;
	}
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <array> <index> get <any>
 * <packedarray> <index> get <any>
 * <dict> <key> get <any>
 * <string> <index> get <int>
 */
DEFUNC_OPER (get)
{
	hg_quark_t arg0, arg1, q = Qnil;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (HG_IS_QARRAY (arg0)) {
		hg_array_t *a;
		hg_size_t index, len;

		if (!HG_IS_QINT (arg1)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		}
		a = HG_VM_LOCK (vm, arg0);
		if (a == NULL) {
			return FALSE;
		}
		index = HG_INT (arg1);
		len = hg_array_maxlength(a);
		if (index < 0 || index >= len) {
			HG_VM_UNLOCK (vm, arg0);

			hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
		}
		q = hg_array_get(a, index);

		retval = TRUE;
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QDICT (arg0)) {
		q = hg_vm_dict_lookup(vm, arg0, arg1, TRUE);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
		if (q == Qnil) {
			hg_error_return (HG_STATUS_FAILED, HG_e_undefined);
		}
		retval = TRUE;
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;
		hg_size_t index, len;
		hg_char_t c;

		if (!HG_IS_QINT (arg1)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		}
		s = HG_VM_LOCK (vm, arg0);
		if (s == NULL) {
			return FALSE;
		}
		index = HG_INT (arg1);
		len = hg_string_length(s);
		if (index < 0 || index >= len) {
			HG_VM_UNLOCK (vm, arg0);

			hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
		}
		c = hg_string_index(s, index);
		q = HG_QINT (c);

		retval = TRUE;
		HG_VM_UNLOCK (vm, arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (retval) {
		hg_stack_drop(ostack);
		hg_stack_drop(ostack);

		STACK_PUSH (ostack, q);
	}
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <array> <index> <count> getinterval <subarray>
 * <packedarray> <index> <count> getinterval <subarray>
 * <string> <index> <count> getinterval <substring>
 */
DEFUNC_OPER (getinterval)
{
	hg_quark_t arg0, arg1, arg2, q = Qnil;
	hg_size_t len, index, count;
	hg_quark_acl_t acl = 0;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg1) ||
	    !HG_IS_QINT (arg2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	index = HG_INT (arg1);
	count = HG_INT (arg2);

	if (HG_IS_QARRAY (arg0)) {
		hg_array_t *a = HG_VM_LOCK (vm, arg0);

		if (a == NULL) {
			return FALSE;
		}
		len = hg_array_maxlength(a);
		if (index >= len ||
		    (len - index) < count) {
			e = HG_e_rangecheck;
			goto error;
		}
		q = hg_array_make_subarray(a, index, index + count - 1, NULL);
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s = HG_VM_LOCK (vm, arg0);

		if (s == NULL) {
			return FALSE;
		}
		len = hg_string_maxlength(s);
		if (index >= len ||
		    (len - index) < count) {
			e = HG_e_rangecheck;
			goto error;
		}
		q = hg_string_make_substring(s, index, index + count - 1, NULL);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (q == Qnil) {
		e = HG_e_VMerror;
		goto error;
	}
	if (hg_vm_quark_is_readable(vm, &arg0))
		acl |= HG_ACL_READABLE;
	if (hg_vm_quark_is_writable(vm, &arg0))
		acl |= HG_ACL_WRITABLE;
	if (hg_vm_quark_is_executable(vm, &arg0))
		acl |= HG_ACL_EXECUTABLE;
	if (hg_vm_quark_is_accessible(vm, &arg0))
		acl |= HG_ACL_ACCESSIBLE;
	hg_vm_quark_set_acl(vm, &q, acl);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, q);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	HG_VM_UNLOCK (vm, arg0);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (glyphshow);

/* - grestore - */
DEFUNC_OPER (grestore)
{
	hg_quark_t q;
	hg_gstate_t *g;

	if (hg_stack_depth(vm->stacks[HG_VM_STACK_GSTATE]) > 0) {
		q = hg_stack_index(vm->stacks[HG_VM_STACK_GSTATE], 0);

		g = HG_VM_LOCK (vm, q);
		if (g == NULL) {
			return FALSE;
		}
		hg_vm_set_gstate(vm, q);
		if (g->is_snapshot == Qnil)
			hg_stack_drop(vm->stacks[HG_VM_STACK_GSTATE]);

		HG_VM_UNLOCK (vm, q);
	}
	retval = TRUE;
} DEFUNC_OPER_END

/* - grestoreall - */
DEFUNC_OPER (grestoreall)
{
	hg_quark_t q, qq = Qnil;
	hg_gstate_t *g;

	while (hg_stack_depth(vm->stacks[HG_VM_STACK_GSTATE]) > 0 && qq == Qnil) {
		q = hg_stack_index(vm->stacks[HG_VM_STACK_GSTATE], 0);

		g = HG_VM_LOCK (vm, q);
		if (g == NULL) {
			return FALSE;
		}
		hg_vm_set_gstate(vm, q);
		qq = g->is_snapshot;
		if (qq == Qnil)
			hg_stack_drop(vm->stacks[HG_VM_STACK_GSTATE]);

		HG_VM_UNLOCK (vm, q);
	}
	retval = TRUE;
} DEFUNC_OPER_END

/* - gsave - */
DEFUNC_OPER (gsave)
{
	hg_quark_t q, qg = hg_vm_get_gstate(vm);

	q = hg_vm_quark_copy(vm, qg, NULL);
	if (q == Qnil) {
		return FALSE;
	}
	STACK_PUSH (vm->stacks[HG_VM_STACK_GSTATE], qg);
	hg_vm_set_gstate(vm, q);

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (gstate);

/* <num1> <num2> gt <bool>
 * <string1> <string2> gt <bool>
 */
DEFUNC_OPER (gt)
{
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		hg_real_t d1, d2;

		if (HG_IS_QINT (arg0))
			d1 = HG_INT (arg0);
		else
			d1 = HG_REAL (arg0);
		if (HG_IS_QINT (arg1))
			d2 = HG_INT (arg1);
		else
			d2 = HG_INT (arg1);

		q = HG_QBOOL (d1 > d2);
	} else if (HG_IS_QSTRING (arg0) &&
		   HG_IS_QSTRING (arg1)) {
		hg_char_t *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) > 0);

	  s_error:
		hg_free(cs1);
		hg_free(cs2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack);

		retval = TRUE;
	}
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <matrix> identmatrix <matrix> */
DEFUNC_OPER (identmatrix)
{
	hg_quark_t arg0;
	hg_array_t *a = NULL;
	hg_matrix_t m;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	a = HG_VM_LOCK (vm, arg0);
	if (!a) {
		return FALSE;
	}
	if (hg_array_maxlength(a) != 6) {
		HG_VM_UNLOCK (vm, arg0);

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	hg_matrix_init_identity(&m);
	hg_array_from_matrix(a, &m);

	retval = TRUE;
	HG_VM_UNLOCK (vm, arg0);
} DEFUNC_OPER_END

/* <int1> <int2> idiv <quotient> */
DEFUNC_OPER (idiv)
{
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	*ret = HG_QINT (HG_INT (arg0) / HG_INT (arg1));

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (idtransform);

/* <bool> <proc> if - */
DEFUNC_OPER (if)
{
	hg_quark_t arg0, arg1, q;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QBOOL (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!HG_IS_QARRAY (arg1) ||
	    !hg_vm_quark_is_executable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (HG_BOOL (arg0)) {
		q = hg_vm_quark_copy(vm, arg1, NULL);
		if (q == Qnil) {
			return FALSE;
		}
		STACK_PUSH (estack, q);
		hg_stack_exch(estack);
		SET_EXPECTED_ESTACK_SIZE (1);
	}
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

/* <bool> <proc1> <proc2> ifelse - */
DEFUNC_OPER (ifelse)
{
	hg_quark_t arg0, arg1, arg2, q;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);
	if (!HG_IS_QBOOL (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!HG_IS_QARRAY (arg1) ||
	    !hg_vm_quark_is_executable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (!HG_IS_QARRAY (arg2) ||
	    !hg_vm_quark_is_executable(vm, &arg2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg2)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (HG_BOOL (arg0)) {
		q = hg_vm_quark_copy(vm, arg1, NULL);
	} else {
		q = hg_vm_quark_copy(vm, arg2, NULL);
	}
	if (q == Qnil) {
		return FALSE;
	}
	STACK_PUSH (estack, q);
	hg_stack_exch(estack);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_STACK_SIZE (-3, 1, 0);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (image);
DEFUNC_UNIMPLEMENTED_OPER (imagemask);

/* <any> ... <n> index <any> ... */
DEFUNC_OPER (index)
{
	hg_quark_t arg0, q;
	hg_usize_t n;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	n = HG_INT (arg0);

	if (n < 0 || (n + 1) >= hg_stack_depth(ostack)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	q = hg_stack_index(ostack, n + 1);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ineofill);
DEFUNC_UNIMPLEMENTED_OPER (infill);

/* - initclip - */
DEFUNC_OPER (initclip)
{
	const hg_char_t ps[] = ".currentpagedevice /PageSize get 0 0 moveto dup 0 get 0 lineto dup aload pop lineto 1 get 0 exch lineto closepath";

	if (hg_vm_eval_cstring(vm, ps, -1, TRUE)) {
		hg_quark_t qg = hg_vm_get_gstate(vm);
		hg_gstate_t *gstate = HG_VM_LOCK (vm, qg);

		if (gstate) {
			retval = hg_gstate_initclip(gstate);
			HG_VM_UNLOCK (vm, qg);
		}
	}
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (initviewclip);
DEFUNC_UNIMPLEMENTED_OPER (instroke);
DEFUNC_UNIMPLEMENTED_OPER (inueofill);
DEFUNC_UNIMPLEMENTED_OPER (inufill);
DEFUNC_UNIMPLEMENTED_OPER (inustroke);

/* <matrix1> <matrix2> invertmatrix <matrix3> */
DEFUNC_OPER (invertmatrix)
{
	hg_quark_t arg0, arg1;
	hg_array_t *a1 = NULL, *a2 = NULL;
	hg_matrix_t m1, m2;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0) ||
	    !HG_IS_QARRAY (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	a1 = HG_VM_LOCK (vm, arg0);
	a2 = HG_VM_LOCK (vm, arg1);
	if (!a1 || !a2) {
		e = HG_e_VMerror;
		goto error;
	}

	if (!hg_array_is_matrix(a1)) {
		e = HG_e_rangecheck;
		goto error;
	}
	if (hg_array_maxlength(a2) != 6) {
		e = HG_e_rangecheck;
		goto error;
	}
	hg_array_to_matrix(a1, &m1);
	if (!hg_matrix_invert(&m1, &m2)) {
		e = HG_e_undefinedresult;
		goto error;
	}
	hg_array_from_matrix(a2, &m2);

	hg_stack_exch(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
  error:
	if (a1)
		HG_VM_UNLOCK (vm, arg0);
	if (a2)
		HG_VM_UNLOCK (vm, arg1);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (itransform);
DEFUNC_UNIMPLEMENTED_OPER (join);

/* <dict> <key> known <bool> */
DEFUNC_OPER (known)
{
	hg_quark_t arg0, arg1, q, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QDICT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	q = hg_vm_dict_lookup(vm, arg0, arg1, TRUE);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}

	*ret = HG_QBOOL (q != Qnil);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (kshow);

/* - languagelevel <int> */
DEFUNC_OPER (languagelevel)
{
	STACK_PUSH (ostack, HG_QINT (hg_vm_get_language_level(vm) + 1));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* <num1> <num2> le <bool>
 * <string1> <string2> le <bool>
 */
DEFUNC_OPER (le)
{
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		hg_real_t d1, d2;

		if (HG_IS_QINT (arg0))
			d1 = HG_INT (arg0);
		else
			d1 = HG_REAL (arg0);
		if (HG_IS_QINT (arg1))
			d2 = HG_INT (arg1);
		else
			d2 = HG_INT (arg1);

		q = HG_QBOOL (HG_REAL_LE (d1, d2));
	} else if (HG_IS_QSTRING (arg0) &&
		   HG_IS_QSTRING (arg1)) {
		hg_char_t *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) <= 0);

	  s_error:
		hg_free(cs1);
		hg_free(cs2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack);

		retval = TRUE;
	}
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <array> length <int>
 * <packedarray> length <int>
 * <dict> length <int>
 * <string> length <int>
 * <name> length <int>
 */
DEFUNC_OPER (length)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;

	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (HG_IS_QARRAY (arg0)) {
		hg_array_t *a = HG_VM_LOCK (vm, arg0);

		if (a == NULL) {
			return FALSE;
		}
		*ret = HG_QINT (hg_array_maxlength(a));
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QDICT (arg0)) {
		hg_dict_t *d = HG_VM_LOCK (vm, arg0);

		if (d == NULL) {
			return FALSE;
		}
		*ret = HG_QINT (hg_dict_length(d));
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s = HG_VM_LOCK (vm, arg0);

		if (s == NULL) {
			return FALSE;
		}
		*ret = HG_QINT (hg_string_maxlength(s));
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QNAME (arg0) || HG_IS_QEVALNAME (arg0)) {
		const hg_char_t *n = hg_name_lookup(arg0);

		if (n == NULL) {
			hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
		}
		*ret = HG_QINT (strlen(n));
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	retval = TRUE;
} DEFUNC_OPER_END

/* <x> <y> lineto - */
DEFUNC_OPER (lineto)
{
	hg_quark_t arg0, arg1, qg = hg_vm_get_gstate(vm);
	hg_real_t dx, dy;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_lineto(gstate, dx, dy);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <num> ln <real> */
DEFUNC_OPER (ln)
{
	hg_quark_t arg0, *ret;
	hg_real_t d;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	if (HG_REAL_LE (d, 0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	*ret = HG_QREAL (log(d));

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (lock);

/* <num> log <real> */
DEFUNC_OPER (log)
{
	hg_quark_t arg0, *ret;
	hg_real_t d;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	if (HG_REAL_LE (d, 0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	*ret = HG_QREAL (log10(d));

	retval = TRUE;
} DEFUNC_OPER_END

/* <proc> loop - */
DEFUNC_OPER (loop)
{
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_executable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	q = hg_vm_dict_lookup(vm, vm->qsystemdict,
			      HG_QNAME ("%loop_continue"),
			      FALSE);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefined);
	}

	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 3, -1);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_STACK_SIZE (-1, 2, 0);
} DEFUNC_OPER_END

/* <num1> <num2> lt <bool>
 * <string1> <string2> lt <bool>
 */
DEFUNC_OPER (lt)
{
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		hg_real_t d1, d2;

		if (HG_IS_QINT (arg0))
			d1 = HG_INT (arg0);
		else
			d1 = HG_REAL (arg0);
		if (HG_IS_QINT (arg1))
			d2 = HG_INT (arg1);
		else
			d2 = HG_INT (arg1);

		q = HG_QBOOL (d1 < d2);
	} else if (HG_IS_QSTRING (arg0) &&
		   HG_IS_QSTRING (arg1)) {
		hg_char_t *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) < 0);

	  s_error:
		hg_free(cs1);
		hg_free(cs2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack);

		retval = TRUE;
	}
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (makefont);
DEFUNC_UNIMPLEMENTED_OPER (makepattern);

/* <dict> maxlength <int> */
DEFUNC_OPER (maxlength)
{
	hg_quark_t arg0, *ret;
	hg_dict_t *dict;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (!HG_IS_QDICT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	dict = HG_VM_LOCK (vm, arg0);
	if (dict == NULL) {
		return FALSE;
	}
	*ret = HG_QINT (hg_dict_maxlength(dict));
	HG_VM_UNLOCK (vm, arg0);

	retval = TRUE;
} DEFUNC_OPER_END

/* <int1> <int2> mod <remainder> */
DEFUNC_OPER (mod)
{
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_INT (arg1) == 0) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefinedresult);
	}
	*ret = HG_QINT (HG_INT (arg0) % HG_INT (arg1));

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (monitor);

/* <x> <y> moveto - */
DEFUNC_OPER (moveto)
{
	hg_quark_t arg0, arg1, qg = hg_vm_get_gstate(vm);
	hg_real_t dx, dy;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_moveto(gstate, dx, dy);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <num1> <num2> mul <product> */
DEFUNC_OPER (mul)
{
	hg_quark_t arg0, arg1, *ret;
	hg_real_t d1, d2, dr;
	hg_bool_t is_int = TRUE;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
		is_int = FALSE;
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	dr = d1 * d2;
	if (is_int &&
	    HG_REAL_LE (dr, HG_MAXINT) &&
	    HG_REAL_GE (dr, HG_MININT)) {
		*ret = HG_QINT ((hg_int_t)dr);
	} else {
		*ret = HG_QREAL (dr);
	}
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <any1> <any2> ne <bool> */
DEFUNC_OPER (ne)
{
	hg_quark_t arg0, arg1, *qret;
	hg_bool_t ret = FALSE;

	CHECK_STACK (ostack, 2);

	qret = hg_stack_peek(ostack, 1);
	arg0 = *qret;
	arg1 = hg_stack_index(ostack, 0);
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	ret = !hg_vm_quark_compare(vm, arg0, arg1);
	if (ret) {
		if ((HG_IS_QNAME (arg0) ||
		     HG_IS_QEVALNAME (arg0) ||
		     HG_IS_QSTRING (arg0)) &&
		    (HG_IS_QNAME (arg1) ||
		     HG_IS_QEVALNAME (arg1) ||
		     HG_IS_QSTRING (arg1))) {
			hg_char_t *s1 = NULL, *s2 = NULL;

			if (HG_IS_QNAME (arg0) ||
			    HG_IS_QEVALNAME (arg0)) {
				s1 = hg_strdup(HG_NAME (arg0));
			} else {
				s1 = hg_vm_string_get_cstr(vm, arg0);
				if (!HG_ERROR_IS_SUCCESS0 ()) {
					goto s_error;
				}
			}
			if (HG_IS_QNAME (arg1) ||
			    HG_IS_QEVALNAME (arg1)) {
				s2 = hg_strdup(HG_NAME (arg1));
			} else {
				s2 = hg_vm_string_get_cstr(vm, arg1);
				if (!HG_ERROR_IS_SUCCESS0 ()) {
					goto s_error;
				}
			}
			if (s1 != NULL && s2 != NULL)
				ret = (strcmp(s1, s2) != 0);
		  s_error:
			hg_free(s1);
			hg_free(s2);
		} else if ((HG_IS_QINT (arg0) ||
			    HG_IS_QREAL (arg0)) &&
			   (HG_IS_QINT (arg1) ||
			    HG_IS_QREAL (arg1))) {
			hg_real_t d1, d2;

			if (HG_IS_QINT (arg0))
				d1 = HG_INT (arg0);
			else
				d1 = HG_REAL (arg0);
			if (HG_IS_QINT (arg1))
				d2 = HG_INT (arg1);
			else
				d2 = HG_REAL (arg1);

			ret = !HG_REAL_EQUAL (d1, d2);
		}
	}
	if (HG_ERROR_IS_SUCCESS0 ()) {
		*qret = HG_QBOOL (ret);
		hg_stack_drop(ostack);

		retval = TRUE;
	}
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <num1> neg <num2> */
DEFUNC_OPER (neg)
{
	hg_quark_t arg0, *ret;
	hg_real_t d;
	hg_bool_t is_int = TRUE;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = -HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = -HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	if (is_int &&
	    HG_REAL_LE (d, HG_MAXINT) &&
	    HG_REAL_GE (d, HG_MININT)) {
		*ret = HG_QINT ((hg_int_t)d);
	} else {
		*ret = HG_QREAL (d);
	}

	retval = TRUE;
} DEFUNC_OPER_END

/* - newpath - */
DEFUNC_OPER (newpath)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	retval = hg_gstate_newpath(gstate);

	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <array> noaccess <array>
 * <packedarray> noaccess <packedarray>
 * <dict> noaccess <dict>
 * <file> noaccess <file>
 * <string> noaccess <string>
 */
DEFUNC_OPER (noaccess)
{
	hg_quark_t *arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_peek(ostack, 0);
	if (HG_IS_QARRAY (*arg0) ||
	    HG_IS_QFILE (*arg0) ||
	    HG_IS_QSTRING (*arg0) ||
	    HG_IS_QDICT (*arg0)) {
		hg_vm_quark_set_acl(vm, arg0, 0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	retval = TRUE;
} DEFUNC_OPER_END

/* <bool> not <bool>
 * <int> not <int>
 */
DEFUNC_OPER (not)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QBOOL (arg0)) {
		*ret = HG_QBOOL (!HG_BOOL (arg0));
	} else if (HG_IS_QINT (arg0)) {
		*ret = HG_QINT (-HG_INT (arg0));
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (notify);
DEFUNC_UNIMPLEMENTED_OPER (nulldevice);

/* <bool1> <bool2> or <bool3>
 * <int1> <int2> or <int3>
 */
DEFUNC_OPER (or)
{
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QBOOL (arg0) &&
	    HG_IS_QBOOL (arg1)) {
		*ret = HG_QBOOL (HG_BOOL (arg0) | HG_BOOL (arg1));
	} else if (HG_IS_QINT (arg0) &&
		   HG_IS_QINT (arg1)) {
		*ret = HG_QINT (HG_INT (arg0) | HG_INT (arg1));
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (packedarray);

/* - pathbbox <llx> <lly> <urx> <ury> */
DEFUNC_OPER (pathbbox)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_path_bbox_t bbox;
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg);

	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_pathbbox(gstate,
				    hg_vm_get_language_level(vm) != HG_LANG_LEVEL_1,
				    &bbox);
	if (!retval)
		goto finalize;

	STACK_PUSH (ostack, HG_QREAL (bbox.llx));
	STACK_PUSH (ostack, HG_QREAL (bbox.lly));
	STACK_PUSH (ostack, HG_QREAL (bbox.urx));
	STACK_PUSH (ostack, HG_QREAL (bbox.ury));

	SET_EXPECTED_OSTACK_SIZE (4);
  finalize:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <move> <line> <curve> <close> pathforall - */
DEFUNC_UNIMPLEMENTED_OPER (pathforall);

DEFUNC_OPER (pop)
{
	CHECK_STACK (ostack, 1);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <string> print - */
DEFUNC_OPER (print)
{
	hg_quark_t arg0, qstdout;
	hg_file_t *stdout;
	hg_char_t *cstr;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	cstr = hg_vm_string_get_cstr(vm, arg0);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}
	qstdout = hg_vm_get_io(vm, HG_FILE_IO_STDOUT);
	stdout = HG_VM_LOCK (vm, qstdout);
	if (stdout == NULL) {
		goto error;
	}

	hg_file_write(stdout, cstr,
		      sizeof (hg_char_t),
		      hg_vm_string_length(vm, arg0));
	if (HG_ERROR_IS_SUCCESS0 ()) {
		hg_stack_drop(ostack);

		retval = TRUE;
	}
	HG_VM_UNLOCK (vm, qstdout);
	SET_EXPECTED_OSTACK_SIZE (-1);
  error:
	hg_free(cstr);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (printobject);
DEFUNC_UNIMPLEMENTED_OPER (product);

/* <array> <index> <any> put -
 * <dict> <key> <value> put -
 * <string> <index> <int> put -
 */
DEFUNC_OPER (put)
{
	hg_quark_t arg0, arg1, arg2;
	hg_bool_t is_in_global;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	is_in_global = hg_quark_has_mem_id(arg0, vm->mem_id[HG_VM_MEM_GLOBAL]);
	if (is_in_global &&
	    !hg_quark_is_simple_object(arg2) &&
	    hg_quark_has_mem_id(arg2, vm->mem_id[HG_VM_MEM_LOCAL])) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (HG_IS_QARRAY (arg0)) {
		hg_usize_t index;

		if (!HG_IS_QINT (arg1)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		}
		index = HG_INT (arg1);
		retval = hg_vm_array_set(vm, arg0, arg2, index, FALSE);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
	} else if (HG_IS_QDICT (arg0)) {
		retval = hg_vm_dict_add(vm, arg0, arg1, arg2, FALSE);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;

		if (!HG_IS_QINT (arg1) ||
		    !HG_IS_QINT (arg2)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		}
		s = HG_VM_LOCK (vm, arg0);
		if (s == NULL) {
			return FALSE;
		}
		retval = hg_string_overwrite_c(s, arg2, arg1);

		HG_VM_UNLOCK (vm, arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	if (retval) {
		hg_stack_drop(ostack);
		hg_stack_drop(ostack);
		hg_stack_drop(ostack);
		SET_EXPECTED_OSTACK_SIZE (-3);
	}
} DEFUNC_OPER_END

/* - rand <int> */
DEFUNC_OPER (rand)
{
	hg_quark_t ret;

	ret = HG_QINT (hg_vm_rand_int(vm));

	STACK_PUSH (ostack, ret);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* <array> rcheck <bool>
 * <packedarray> rcheck <bool>
 * <dict> rcheck <bool>
 * <file> rcheck <bool>
 * <string> rcheck <bool>
 */
DEFUNC_OPER (rcheck)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (!HG_IS_QARRAY (arg0) &&
	    !HG_IS_QDICT (arg0) &&
	    !HG_IS_QFILE (arg0) &&
	    !HG_IS_QSTRING (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	*ret = HG_QBOOL (hg_vm_quark_is_readable(vm, &arg0));

	retval = TRUE;
} DEFUNC_OPER_END

/* <dx1> <dy1> <dx2> <dy2> <dx3> <dy3> rcurveto - */
DEFUNC_OPER (rcurveto)
{
	hg_quark_t arg0, arg1, arg2, arg3, arg4, arg5, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t dx1, dy1, dx2, dy2, dx3, dy3;

	CHECK_STACK (ostack, 6);

	arg0 = hg_stack_index(ostack, 5);
	arg1 = hg_stack_index(ostack, 4);
	arg2 = hg_stack_index(ostack, 3);
	arg3 = hg_stack_index(ostack, 2);
	arg4 = hg_stack_index(ostack, 1);
	arg5 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg4)) {
		dx3 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dx3 = HG_REAL (arg4);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg5)) {
		dy3 = HG_INT (arg5);
	} else if (HG_IS_QREAL (arg5)) {
		dy3 = HG_REAL (arg5);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_rcurveto(gstate, dx1, dy1, dx2, dy2, dx3, dy3);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-6);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <file> read <int> <true>
 * <file> read <false>
 */
DEFUNC_OPER (read)
{
	hg_quark_t arg0;
	hg_file_t *f;
	hg_char_t c[2];
	hg_bool_t eof;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	f = HG_VM_LOCK (vm, arg0);
	if (f == NULL) {
		return FALSE;
	}
	hg_file_read(f, c, sizeof (hg_char_t), 1);
	eof = hg_file_is_eof(f);
	HG_VM_UNLOCK (vm, arg0);

	if (eof) {
		hg_stack_drop(ostack);

		STACK_PUSH (ostack, HG_QBOOL (FALSE));
		retval = TRUE;
	} else if (HG_ERROR_IS_SUCCESS0 ()) {
		hg_stack_drop(ostack);

		STACK_PUSH (ostack, HG_QINT ((guchar)c[0]));
		STACK_PUSH (ostack, HG_QBOOL (TRUE));
		SET_EXPECTED_OSTACK_SIZE (1);
		retval = TRUE;
	}
} DEFUNC_OPER_END

/* <file> <string> readhexstring <substring> <bool> */
DEFUNC_OPER (readhexstring)
{
	hg_quark_t arg0, arg1, q, qs;
	hg_file_t *f = NULL;
	hg_string_t *s = NULL;
	hg_char_t c[2], hex = 0;
	hg_int_t shift = 4;
	hg_size_t ret;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!HG_IS_QSTRING (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	f = HG_VM_LOCK (vm, arg0);
	if (f == NULL) {
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg1);
	if (s == NULL) {
		e = HG_e_VMerror;
		goto finalize;
	}

	if (hg_string_maxlength(s) == 0) {
		e = HG_e_rangecheck;
		goto finalize;
	}
	hg_string_clear(s);

	while (!hg_file_is_eof(f) && hg_string_length(s) < hg_string_maxlength(s)) {
		ret = hg_file_read(f, c, sizeof (hg_char_t), 1);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			e = HG_e_ioerror;
			goto finalize;
		}
		if (c[0] >= '0' && c[0] <= '9') {
			hex |= ((c[0] - '0') << shift);
			shift = shift == 0 ? 4 : 0;
		} else if ((c[0] >= 'a' && c[0] <= 'f') ||
			   (c[0] >= 'A' && c[0] <= 'F')) {
			hex |= ((g_ascii_tolower(c[0]) - 'a' + 10) << shift);
			shift = shift == 0 ? 4 : 0;
		} else {
			continue;
		}
		if (shift == 4) {
			hg_string_append_c(s, hex);
			hex = 0;
		}
	}
	if (hg_file_is_eof(f)) {
		hg_quark_acl_t acl = 0;

		q = HG_QBOOL (FALSE);
		qs = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL);
		if (qs == Qnil) {
			e = HG_e_VMerror;
			goto finalize;
		}
		if (hg_vm_quark_is_readable(vm, &arg1))
			acl |= HG_ACL_READABLE;
		if (hg_vm_quark_is_writable(vm, &arg1))
			acl |= HG_ACL_WRITABLE;
		if (hg_vm_quark_is_executable(vm, &arg1))
			acl |= HG_ACL_EXECUTABLE;
		if (hg_vm_quark_is_accessible(vm, &arg1))
			acl |= HG_ACL_ACCESSIBLE;
		hg_vm_quark_set_acl(vm, &qs, acl);
	} else {
		q = HG_QBOOL (TRUE);
		qs = arg1;
	}
	if (qs == arg1)
		hg_stack_pop(ostack);
	else
		hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, qs);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  finalize:
	if (f)
		HG_VM_UNLOCK (vm, arg0);
	if (s)
		HG_VM_UNLOCK (vm, arg1);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

/* <file> <string> readline <substring> <bool> */
DEFUNC_OPER (readline)
{
	hg_quark_t arg0, arg1, q, qs;
	hg_file_t *f = NULL;
	hg_string_t *s = NULL;
	hg_char_t c[2];
	hg_bool_t eol = FALSE;
	hg_size_t ret;
	hg_quark_acl_t acl = 0;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!HG_IS_QSTRING (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	f = HG_VM_LOCK (vm, arg0);
	if (f == NULL) {
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg1);
	if (s == NULL) {
		e = HG_e_VMerror;
		goto finalize;
	}

	hg_string_clear(s);

	while (!hg_file_is_eof(f)) {
		if (hg_string_length(s) >= hg_string_maxlength(s)) {
			e = HG_e_rangecheck;
			goto finalize;
		}
	  retry:
		ret = hg_file_read(f, c, sizeof (hg_char_t), 1);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			e = HG_e_ioerror;
			goto finalize;
		}
		if (c[0] == '\r') {
			if (eol)
				goto rollback;
			eol = TRUE;
			goto retry;
		} else if (c[0] == '\n') {
			break;
		}
		if (eol) {
		  rollback:
			hg_file_seek(f, -1, HG_FILE_POS_CURRENT);
			break;
		}
		hg_string_append_c(s, c[0]);
	}
	if (hg_file_is_eof(f)) {
		q = HG_QBOOL (FALSE);
	} else {
		q = HG_QBOOL (TRUE);
	}
	qs = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL);
	if (qs == Qnil) {
		e = HG_e_VMerror;
		goto finalize;
	}
	if (hg_vm_quark_is_readable(vm, &arg1))
		acl |= HG_ACL_READABLE;
	if (hg_vm_quark_is_writable(vm, &arg1))
		acl |= HG_ACL_WRITABLE;
	if (hg_vm_quark_is_executable(vm, &arg1))
		acl |= HG_ACL_EXECUTABLE;
	if (hg_vm_quark_is_accessible(vm, &arg1))
		acl |= HG_ACL_ACCESSIBLE;
	hg_vm_quark_set_acl(vm, &qs, acl);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, qs);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  finalize:
	if (f)
		HG_VM_UNLOCK (vm, arg0);
	if (s)
		HG_VM_UNLOCK (vm, arg1);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

/* -array- readonly -array-
 * -packedarray- readonly -packedarray-
 * -dict- readonly -dict-
 * -file- readonly -file-
 * -string- readonly -string-
 */
DEFUNC_OPER (readonly)
{
	hg_quark_t *arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_peek(ostack, 0);
	if (!HG_IS_QARRAY (*arg0) &&
	    !HG_IS_QDICT (*arg0) &&
	    !HG_IS_QFILE (*arg0) &&
	    !HG_IS_QSTRING (*arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	hg_vm_quark_set_writable(vm, arg0, FALSE);

	retval = TRUE;
} DEFUNC_OPER_END

/* <file> <string> readstring <substring> <bool> */
DEFUNC_OPER (readstring)
{
	hg_quark_t arg0, arg1, q, qs;
	hg_file_t *f = NULL;
	hg_string_t *s = NULL;
	hg_char_t c[2];
	hg_size_t ret;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QFILE (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!HG_IS_QSTRING (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	f = HG_VM_LOCK (vm, arg0);
	if (f == NULL) {
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg1);
	if (s == NULL) {
		e = HG_e_VMerror;
		goto finalize;
	}

	if (hg_string_maxlength(s) == 0) {
		e = HG_e_rangecheck;
		goto finalize;
	}
	hg_string_clear(s);

	while (!hg_file_is_eof(f) && hg_string_length(s) < hg_string_maxlength(s)) {
		ret = hg_file_read(f, c, sizeof (hg_char_t), 1);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			e = HG_e_ioerror;
			goto finalize;
		}
		hg_string_append_c(s, c[0]);
	}
	if (hg_file_is_eof(f)) {
		hg_quark_acl_t acl = 0;

		q = HG_QBOOL (FALSE);
		qs = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL);
		if (qs == Qnil) {
			e = HG_e_VMerror;
			goto finalize;
		}
		if (hg_vm_quark_is_readable(vm, &arg1))
			acl |= HG_ACL_READABLE;
		if (hg_vm_quark_is_writable(vm, &arg1))
			acl |= HG_ACL_WRITABLE;
		if (hg_vm_quark_is_executable(vm, &arg1))
			acl |= HG_ACL_EXECUTABLE;
		if (hg_vm_quark_is_accessible(vm, &arg1))
			acl |= HG_ACL_ACCESSIBLE;
		hg_vm_quark_set_acl(vm, &qs, acl);
	} else {
		q = HG_QBOOL (TRUE);
		qs = arg1;
	}
	if (qs == arg1)
		hg_stack_pop(ostack);
	else
		hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	STACK_PUSH (ostack, qs);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  finalize:
	if (f)
		HG_VM_UNLOCK (vm, arg0);
	if (s)
		HG_VM_UNLOCK (vm, arg1);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (realtime);
DEFUNC_UNIMPLEMENTED_OPER (rectclip);
DEFUNC_UNIMPLEMENTED_OPER (rectfill);
DEFUNC_UNIMPLEMENTED_OPER (rectstroke);
DEFUNC_UNIMPLEMENTED_OPER (rectviewclip);
DEFUNC_UNIMPLEMENTED_OPER (renamefile);
DEFUNC_UNIMPLEMENTED_OPER (renderbands);

/* <int> <proc> repeat - */
DEFUNC_OPER (repeat)
{
	hg_quark_t arg0, arg1, q;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QARRAY (arg1) ||
	    !hg_vm_quark_is_executable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (HG_INT (arg0) < 0) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}

	q = hg_vm_dict_lookup(vm, vm->qsystemdict,
			      HG_QNAME ("%repeat_continue"),
			      FALSE);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}
	if (q == Qnil) {
		hg_error_return (HG_STATUS_FAILED, HG_e_undefined);
	}

	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, arg1);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 4, -1);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_STACK_SIZE (-2, 3, 0);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (resetfile);
DEFUNC_UNIMPLEMENTED_OPER (resourceforall);
DEFUNC_UNIMPLEMENTED_OPER (resourcestatus);

/* <save> restore - */
DEFUNC_OPER (restore)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!hg_vm_snapshot_restore(vm, arg0)) {
		return FALSE;
	}

	retval = TRUE;
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (reversepath);

/* <dx> <dy> rlineto - */
DEFUNC_OPER (rlineto)
{
	hg_quark_t arg0, arg1, qg = hg_vm_get_gstate(vm);
	hg_real_t dx, dy;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	retval = hg_gstate_rlineto(gstate, dx, dy);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <dx> <dy> rmoveto - */
DEFUNC_OPER (rmoveto)
{
	hg_quark_t arg0, arg1, qg = hg_vm_get_gstate(vm);
	hg_real_t dx, dy;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	retval = hg_gstate_rmoveto(gstate, dx, dy);
	if (!retval)
		goto error;

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

/* <any> ... <n> <j> roll <any> ... */
DEFUNC_OPER (roll)
{
	hg_quark_t arg0, arg1;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_INT (arg0) < 0) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	CHECK_STACK (ostack, HG_INT (arg0) + 2);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	hg_stack_roll(ostack, HG_INT (arg0), HG_INT (arg1));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (rootfont);

/* <angle> rotate -
 * <angle> <matrix> rotate <matrix>
 */
DEFUNC_OPER (rotate)
{
	hg_quark_t arg0, arg1 = Qnil, qg = hg_vm_get_gstate(vm);
	hg_real_t angle;
	hg_matrix_t ctm;
	hg_array_t *a;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (HG_IS_QARRAY (arg0)) {
		CHECK_STACK (ostack, 2);

		arg1 = arg0;
		arg0 = hg_stack_index(ostack, 1);
		a = HG_VM_LOCK (vm, arg1);
		if (!a)
			return FALSE;
		if (hg_array_maxlength(a) != 6) {
			HG_VM_UNLOCK (vm, arg1);

			hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
		}
		HG_VM_UNLOCK (vm, arg1);
		ctm.mtx.xx = 1;
		ctm.mtx.xy = 0;
		ctm.mtx.yx = 0;
		ctm.mtx.yy = 1;
		ctm.mtx.x0 = 0;
		ctm.mtx.y0 = 0;
	} else {
		gstate = HG_VM_LOCK (vm, qg);
		if (!gstate)
			return FALSE;
		hg_gstate_get_ctm(gstate, &ctm);
		HG_VM_UNLOCK (vm, qg);
	}
	if (HG_IS_QINT (arg0)) {
		angle = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		angle = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	hg_matrix_rotate(&ctm, angle);

	if (arg1 == Qnil) {
		gstate = HG_VM_LOCK (vm, qg);
		if (!gstate)
			return FALSE;
		hg_gstate_set_ctm(gstate, &ctm);
		HG_VM_UNLOCK (vm, qg);
	} else {
		a = HG_VM_LOCK (vm, arg1);
		if (!a)
			return FALSE;
		hg_array_from_matrix(a, &ctm);
		HG_VM_UNLOCK (vm, arg1);

		hg_stack_exch(ostack);
	}

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <num> round <num> */
DEFUNC_OPER (round)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;

	if (HG_IS_QINT (arg0)) {
		/* no need to proceed anything */
	} else if (HG_IS_QREAL (arg0)) {
		hg_real_t d = HG_REAL (arg0), dfloat, dr;
		hg_long_t base = (hg_long_t)d;

		/* glibc's round(3) behaves differently expecting in PostScript.
		 * particularly when rounding halfway.
		 *   glibc's round(3) round halfway cases away from zero.
		 *   PostScript's round halfway cases returns the greater of the two.
		 */
		dfloat = fabs(d - (hg_real_t)base);
		dr = (hg_real_t)base + (base < 0 ? -1.0 : 1.0);
		if (dfloat < 0.5) {
			dr = (hg_real_t)base;
		} if (HG_REAL_EQUAL (dfloat, 0.5)) {
			dr = (hg_real_t)MAX (base, dr);
		}

		*ret = HG_QREAL (dr);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	retval = TRUE;
} DEFUNC_OPER_END

/* - rrand <int> */
DEFUNC_OPER (rrand)
{
	STACK_PUSH (ostack, HG_QINT (hg_vm_get_rand_seed(vm)));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

/* - save <save> */
DEFUNC_OPER (save)
{
	if (!hg_vm_snapshot_save(vm)) {
		return FALSE;
	}

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (scale);
DEFUNC_UNIMPLEMENTED_OPER (scalefont);

/* <string> <seek> search <post> <match> <pre> true
 *                               <string> false
 */
DEFUNC_OPER (search)
{
	hg_quark_t arg0, arg1, qpost, qmatch, qpre;
	hg_string_t *s = NULL, *seek = NULL;
	hg_usize_t len0, len1;
	hg_char_t *cs0 = NULL, *cs1 = NULL, *p;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QSTRING (arg0) ||
	    !HG_IS_QSTRING (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}

	s = HG_VM_LOCK (vm, arg0);
	seek = HG_VM_LOCK (vm, arg1);
	if (!s || !seek) {
		e = HG_e_VMerror;
		goto finalize;
	}

	len0 = hg_string_length(s);
	len1 = hg_string_length(seek);
	cs0 = hg_string_get_cstr(s);
	cs1 = hg_string_get_cstr(seek);

	if ((p = strstr(cs0, cs1))) {
		hg_usize_t lmatch, lpre;

		lpre = p - cs0;
		lmatch = len1;

		qpre = hg_string_make_substring(s, 0, lpre - 1, NULL);
		qmatch = hg_string_make_substring(s, lpre, lpre + len1 - 1, NULL);
		qpost = hg_string_make_substring(s, lpre + lmatch, len0 - 1, NULL);
		if (qpre == Qnil ||
		    qmatch == Qnil ||
		    qpost == Qnil) {
			e = HG_e_VMerror;
			goto finalize;
		}

		hg_stack_drop(ostack);
		hg_stack_drop(ostack);

		STACK_PUSH (ostack, qpost);
		STACK_PUSH (ostack, qmatch);
		STACK_PUSH (ostack, qpre);
		STACK_PUSH (ostack, HG_QBOOL (TRUE));

		SET_EXPECTED_OSTACK_SIZE (4 - 2);
	} else {
		hg_stack_drop(ostack);

		STACK_PUSH (ostack, HG_QBOOL (FALSE));

		SET_EXPECTED_OSTACK_SIZE (2 - 2);
	}

	retval = TRUE;
  finalize:
	hg_free(cs0);
	hg_free(cs1);
	if (s)
		HG_VM_UNLOCK (vm, arg0);
	if (seek)
		HG_VM_UNLOCK (vm, arg1);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (selectfont);
DEFUNC_UNIMPLEMENTED_OPER (serialnumber);
DEFUNC_UNIMPLEMENTED_OPER (setbbox);
DEFUNC_UNIMPLEMENTED_OPER (setblackgeneration);
DEFUNC_UNIMPLEMENTED_OPER (setcachedevice);
DEFUNC_UNIMPLEMENTED_OPER (setcachedevice2);
DEFUNC_UNIMPLEMENTED_OPER (setcachelimit);
DEFUNC_UNIMPLEMENTED_OPER (setcacheparams);
DEFUNC_UNIMPLEMENTED_OPER (setcharwidth);
DEFUNC_UNIMPLEMENTED_OPER (setcmykcolor);
DEFUNC_UNIMPLEMENTED_OPER (setcolor);
DEFUNC_UNIMPLEMENTED_OPER (setcolorrendering);
DEFUNC_UNIMPLEMENTED_OPER (setcolorscreen);
DEFUNC_UNIMPLEMENTED_OPER (setcolorspace);
DEFUNC_UNIMPLEMENTED_OPER (setcolortransfer);

/* <array> <offset> setdash - */
DEFUNC_OPER (setdash)
{
	hg_quark_t arg0, arg1, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = NULL;
	hg_real_t offset;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		offset = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		offset = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}

	if (!hg_gstate_set_dash(gstate, arg0, offset)) {
		goto finalize;
	}

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
  finalize:
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setdevparams);
DEFUNC_UNIMPLEMENTED_OPER (setfileposition);
DEFUNC_UNIMPLEMENTED_OPER (setflat);
DEFUNC_UNIMPLEMENTED_OPER (setfont);

/* <num> setgray - */
DEFUNC_OPER (setgray)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_real_t d;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (d > 1.0)
		d = 1.0;
	if (d < 0.0)
		d = 0.0;
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	hg_gstate_set_graycolor(gstate, d);
	HG_VM_UNLOCK (vm, qg);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setgstate);
DEFUNC_UNIMPLEMENTED_OPER (sethalftone);
DEFUNC_UNIMPLEMENTED_OPER (sethalftonephase);

/* <hue> <saturation> <brightness> sethsbcolor - */
DEFUNC_OPER (sethsbcolor)
{
	hg_quark_t arg0, arg1, arg2, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t h, s, b;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		h = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		h = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		s = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		s = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg2)) {
		b = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		b = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (!gstate)
		return FALSE;

	hg_gstate_set_hsbcolor(gstate, h, s, b);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	HG_VM_UNLOCK (vm, qg);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-3);
} DEFUNC_OPER_END

/* <int> setlinecap - */
DEFUNC_OPER (setlinecap)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_int_t i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	i = HG_INT (arg0);
	if (i < 0 || i > 2) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	hg_gstate_set_linecap(gstate, i);

	hg_stack_drop(ostack);
	HG_VM_UNLOCK (vm, qg);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <int> setlinejoin - */
DEFUNC_OPER (setlinejoin)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_int_t i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	i = HG_INT (arg0);
	if (i < 0 || i > 2) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	hg_gstate_set_linejoin(gstate, i);

	hg_stack_drop(ostack);
	HG_VM_UNLOCK (vm, qg);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <num> setlinewidth - */
DEFUNC_OPER (setlinewidth)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_real_t d;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	hg_gstate_set_linewidth(gstate, d);

	HG_VM_UNLOCK (vm, qg);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <matrix> setmatrix - */
DEFUNC_OPER (setmatrix)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_array_t *a;
	hg_matrix_t m;
	hg_gstate_t *gstate = NULL;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QARRAY (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	a = HG_VM_LOCK (vm, arg0);
	if (a == NULL) {
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		e = HG_e_VMerror;
		goto finalize;
	}
	if (!hg_array_is_matrix(a) ||
	    !hg_array_to_matrix(a, &m)) {
		e = HG_e_rangecheck;
		goto finalize;
	}
	hg_gstate_set_ctm(gstate, &m);

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
  finalize:
	if (gstate)
		HG_VM_UNLOCK (vm, qg);
	HG_VM_UNLOCK (vm, arg0);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

/* <num> setmiterlimit - */
DEFUNC_OPER (setmiterlimit)
{
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_real_t miterlen;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (HG_IS_QINT (arg0)) {
		miterlen = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		miterlen = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	if (!hg_gstate_set_miterlimit(gstate, miterlen)) {
		HG_VM_UNLOCK (vm, qg);

		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setobjectformat);
DEFUNC_UNIMPLEMENTED_OPER (setoverprint);
DEFUNC_UNIMPLEMENTED_OPER (setpacking);
DEFUNC_UNIMPLEMENTED_OPER (setpagedevice);
DEFUNC_UNIMPLEMENTED_OPER (setpattern);

/* <red> <green> <blue> setrgbcolor - */
DEFUNC_OPER (setrgbcolor)
{
	hg_quark_t arg0, arg1, arg2, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_real_t r, g, b;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2);
	arg1 = hg_stack_index(ostack, 1);
	arg2 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0)) {
		r = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		r = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	if (HG_IS_QINT (arg1)) {
		g = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg0)) {
		g = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	if (HG_IS_QINT (arg2)) {
		b = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		b = HG_REAL (arg2);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	gstate = HG_VM_LOCK (vm, qg);
	if (gstate == NULL) {
		return FALSE;
	}
	hg_gstate_set_rgbcolor(gstate, r, g, b);

	HG_VM_UNLOCK (vm, qg);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-3);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setscreen);
DEFUNC_UNIMPLEMENTED_OPER (setshared);
DEFUNC_UNIMPLEMENTED_OPER (setstrokeadjust);
DEFUNC_UNIMPLEMENTED_OPER (setsystemparams);
DEFUNC_UNIMPLEMENTED_OPER (settransfer);
DEFUNC_UNIMPLEMENTED_OPER (setucacheparams);
DEFUNC_UNIMPLEMENTED_OPER (setundercolorremoval);

/* <dict> setuserparams - */
DEFUNC_OPER (setuserparams)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	hg_vm_set_user_params(vm, arg0);
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		return FALSE;
	}

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setvmthreshold);
DEFUNC_UNIMPLEMENTED_OPER (shareddict);
DEFUNC_UNIMPLEMENTED_OPER (show);
DEFUNC_UNIMPLEMENTED_OPER (showpage);

/* <angle> sin <real> */
DEFUNC_OPER (sin)
{
	hg_quark_t arg0, *ret;
	hg_real_t angle;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		angle = (hg_real_t)HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		angle = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	*ret = HG_QREAL (sin(angle / 180 * M_PI));

	retval = TRUE;
} DEFUNC_OPER_END

/* <num> sqrt <real> */
DEFUNC_OPER (sqrt)
{
	hg_quark_t arg0, *ret;
	hg_real_t d;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = (hg_real_t)HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	*ret = HG_QREAL (sqrt(d));

	retval = TRUE;
} DEFUNC_OPER_END

/* <int> srand - */
DEFUNC_OPER (srand)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	hg_vm_set_rand_seed(vm, HG_INT (arg0));

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (startjob);

/* <file> status <bool>
 * <filename> status <pages> <bytes> <referenced> <created> true
 * <filename> status false
 */
DEFUNC_OPER (status)
{
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);

	if (HG_IS_QFILE (arg0)) {
		hg_file_t *f = HG_VM_LOCK (vm, arg0);

		if (!f) {
			return FALSE;
		}
		q = HG_QBOOL (!hg_file_is_closed(f));

		HG_VM_UNLOCK (vm, arg0);

		hg_stack_drop(ostack);

		STACK_PUSH (ostack, q);

		SET_EXPECTED_OSTACK_SIZE (1 - 1);
	} else if (HG_IS_QSTRING (arg0)) {
		hg_char_t *filename;
		struct stat st;

		filename = hg_vm_string_get_cstr(vm, arg0);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}

		hg_stack_drop(ostack);

		if (lstat(filename, &st) == -1) {
			STACK_PUSH (ostack, HG_QBOOL (FALSE));

			SET_EXPECTED_OSTACK_SIZE (1 - 1);
		} else {
			STACK_PUSH (ostack, HG_QINT (st.st_blocks));
			STACK_PUSH (ostack, HG_QINT (st.st_size));
			STACK_PUSH (ostack, HG_QINT (st.st_atime));
			STACK_PUSH (ostack, HG_QINT (st.st_mtime));
			STACK_PUSH (ostack, HG_QBOOL (TRUE));

			SET_EXPECTED_OSTACK_SIZE (5 - 1);
		}
		hg_free(filename);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	retval = TRUE;
} DEFUNC_OPER_END

/* - stop - */
DEFUNC_OPER (stop)
{
	hg_quark_t q;
	hg_usize_t edepth = hg_stack_depth(estack), i;
	hg_size_t n = 0;

	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i);
		if (HG_IS_QOPER (q) &&
		    HG_OPER (q) == HG_enc_protected_stopped_continue)
			break;
	}
	if (i == edepth) {
		hg_warning("No /stopped operator found.");
		q = hg_stack_pop(estack);
		STACK_PUSH (estack, HG_QOPER (HG_enc_private_abort));
		STACK_PUSH (estack, q);
		n++;
	} else {
		if (!hg_vm_dict_add(vm, vm->qerror,
				    HG_QNAME (".stopped"),
				    HG_QBOOL (TRUE),
				    FALSE)) {
			return FALSE;
		}
		for (; i > 1; i--) {
			n--;
			hg_stack_drop(estack);
		}
	}
	retval = TRUE;
	SET_EXPECTED_ESTACK_SIZE (n);
} DEFUNC_OPER_END

/* <any> stopped <bool> */
DEFUNC_OPER (stopped)
{
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!hg_quark_is_simple_object(arg0) &&
	    !HG_IS_QOPER (arg0) &&
	    !hg_vm_quark_is_readable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	if (hg_vm_quark_is_executable(vm, &arg0)) {
		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME ("%stopped_continue"),
				      FALSE);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
		if (q == Qnil) {
			hg_error_return (HG_STATUS_FAILED, HG_e_undefined);
		}

		STACK_PUSH (estack, q);
		STACK_PUSH (estack, arg0);

		hg_stack_roll(estack, 3, -1);

		hg_stack_drop(ostack);
	} else {
		hg_stack_drop(ostack);

		STACK_PUSH (ostack, HG_QBOOL (FALSE));
	}

	retval = TRUE;
	SET_EXPECTED_STACK_SIZE (-1, 2, 0);
} DEFUNC_OPER_END

/* <int> string <string> */
DEFUNC_OPER (string)
{
	hg_quark_t arg0, q;
	hg_size_t len;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	len = HG_INT (arg0);
	if (len < 0 || len > 65535) {
		hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}
	q = hg_string_new(hg_vm_get_mem(vm), len, NULL);
	if (q == Qnil) {
		return FALSE;
	}

	hg_stack_drop(ostack);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (stringwidth);

/* - stroke - */
DEFUNC_OPER (stroke)
{
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg);

	if (gstate == NULL) {
		return FALSE;
	}
	if (!hg_device_stroke(vm->device, gstate)) {
		hg_error_t e = hg_errno;

		HG_VM_UNLOCK (vm, qg);
		if (HG_ERROR_IS_SUCCESS (e)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_limitcheck);
		}
		hg_errno = e;

		return FALSE;
	}
	retval = TRUE;
	HG_VM_UNLOCK (vm, qg);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (strokepath);

/* <num1> <num2> sub <diff> */
DEFUNC_OPER (sub)
{
	hg_quark_t arg0, arg1, *ret;
	hg_real_t d1, d2;
	hg_bool_t is_int = TRUE;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
		is_int = FALSE;
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	if (is_int) {
		*ret = HG_QINT ((hg_int_t)(d1 - d2));
	} else {
		*ret = HG_QREAL (d1 - d2);
	}
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* <file> token <any> true
 *              false
 * <string> token <post> <any> true
 *                false
 */
DEFUNC_OPER (token)
{
	hg_quark_t arg0, qt;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (HG_IS_QFILE (arg0)) {
		if (!hg_vm_quark_is_readable(vm, &arg0)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
		}

		hg_quark_set_executable(&arg0, TRUE);
		STACK_PUSH (estack, arg0);

		if (!hg_vm_translate(vm, HG_TRANS_FLAG_TOKEN)) {
			if (!hg_vm_has_error(vm)) {
				/* EOF detected */
				qt = hg_stack_index(ostack, 0);
				hg_stack_drop(ostack); /* the file object given for token operator */

				STACK_PUSH (ostack, HG_QBOOL (FALSE));

				SET_EXPECTED_OSTACK_SIZE (0);
			} else {
				hg_stack_drop(ostack); /* the object where the error happened */
				/* We want to notify the place
				 * where the error happened as in token operator
				 * but not in --file--
				 */
				STACK_PUSH (ostack, qself);

				return FALSE;
			}
		} else {
			hg_quark_t q;

			qt = hg_stack_index(estack, 0);
			if (HG_IS_QFILE (qt)) {
				/* scanner may detected the procedure.
				 * in this case, the scanned object doesn't appear in the exec stack.
				 */
				q = hg_stack_pop(ostack);
			} else {
				q = hg_stack_pop(estack);
			}

			hg_stack_drop(estack); /* the file object executed above */
			hg_stack_drop(ostack); /* the file object given for token operator */

			STACK_PUSH (ostack, q);
			STACK_PUSH (ostack, HG_QBOOL (TRUE));

			SET_EXPECTED_OSTACK_SIZE (1);
		}

		retval = TRUE;
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;
		hg_quark_t qf;

		if (!hg_vm_quark_is_readable(vm, &arg0)) {
			hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
		}
		s = HG_VM_LOCK (vm, arg0);
		if (!s) {
			return FALSE;
		}
		qf = hg_file_new_with_string(hg_vm_get_mem(vm),
					     "%stoken",
					     HG_FILE_IO_MODE_READ,
					     s,
					     NULL,
					     NULL);
		HG_VM_UNLOCK (vm, arg0);
		if (qf == Qnil) {
			return FALSE;
		}

		hg_quark_set_executable(&qf, TRUE);
		STACK_PUSH (estack, qf);

		if (!hg_vm_translate(vm, HG_TRANS_FLAG_TOKEN)) {
			if (!hg_vm_has_error(vm)) {
				/* EOF detected */
				hg_stack_drop(ostack); /* the string object given for token operator */

				STACK_PUSH (ostack, HG_QBOOL (FALSE));

				retval = TRUE;
				SET_EXPECTED_OSTACK_SIZE (0);
			} else {
				hg_stack_drop(ostack); /* the object where the error happened */
				/* We want to notify the place
				 * where the error happened as in token operator
				 * but not in --file--
				 */
				STACK_PUSH (ostack, qself);

				return FALSE;
			}
		} else {
			hg_quark_t q, qs;
			hg_file_t *f;
			hg_size_t pos;
			hg_uint_t length;

			qt = hg_stack_index(estack, 0);
			if (HG_IS_QFILE (qt)) {
				/* scanner may detected the procedure.
				 * in this case, the scanned object doesn't appear in the exec stack.
				 */
				q = hg_stack_pop(ostack);
			} else {
				q = hg_stack_pop(estack);
			}

			f = HG_VM_LOCK (vm, qf);
			if (!f)
				return FALSE;
			pos = hg_file_seek(f, 0, HG_FILE_POS_CURRENT);
			HG_VM_UNLOCK (vm, qf);

			/* safely drop the file object from the stack here.
			 * shouldn't do that during someone is referring it because it may be destroyed by GC.
			 */
			hg_stack_drop(estack); /* the file object executed above */

			s = HG_VM_LOCK (vm, arg0);
			if (!s) {
				return FALSE;
			}

			length = hg_string_maxlength(s) - 1;
			qs = hg_string_make_substring(s, pos, length, NULL);
			if (qs == Qnil) {
				HG_VM_UNLOCK (vm, arg0);

				hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
			}
			hg_stack_drop(ostack); /* the string object given for token operator */

			STACK_PUSH (ostack, qs);
			STACK_PUSH (ostack, q);
			STACK_PUSH (ostack, HG_QBOOL (TRUE));

			retval = TRUE;
			SET_EXPECTED_OSTACK_SIZE (2);
			HG_VM_UNLOCK (vm, arg0);
		}
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (transform);

/* <tx> <ty> translate -
 * <tx> <ty> <matrix> translate <matrix>
 */
DEFUNC_OPER (translate)
{
	hg_quark_t arg0, arg1, arg2 = Qnil, qg = hg_vm_get_gstate(vm);
	hg_real_t tx, ty;
	hg_matrix_t ctm;
	hg_array_t *a;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (HG_IS_QARRAY (arg1)) {
		CHECK_STACK (ostack, 3);

		arg2 = arg1;
		arg1 = arg0;
		arg0 = hg_stack_index(ostack, 2);
		a = HG_VM_LOCK (vm, arg2);
		if (!a)
			return FALSE;
		if (hg_array_maxlength(a) != 6) {
			HG_VM_UNLOCK (vm, arg2);

			hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
		}
		HG_VM_UNLOCK (vm, arg2);
		ctm.mtx.xx = 1;
		ctm.mtx.xy = 0;
		ctm.mtx.yx = 0;
		ctm.mtx.yy = 1;
		ctm.mtx.x0 = 0;
		ctm.mtx.y0 = 0;
	} else {
		gstate = HG_VM_LOCK (vm, qg);
		if (!gstate)
			return FALSE;
		hg_gstate_get_ctm(gstate, &ctm);
		HG_VM_UNLOCK (vm, qg);
	}
	if (HG_IS_QINT (arg0)) {
		tx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		tx = HG_REAL (arg0);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (HG_IS_QINT (arg1)) {
		ty = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		ty = HG_REAL (arg1);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	hg_matrix_translate(&ctm, tx, ty);

	if (arg2 == Qnil) {
		gstate = HG_VM_LOCK (vm, qg);
		if (!gstate)
			return FALSE;
		hg_gstate_set_ctm(gstate, &ctm);
		HG_VM_UNLOCK (vm, qg);
	} else {
		a = HG_VM_LOCK (vm, arg2);
		if (!a)
			return FALSE;
		hg_array_from_matrix(a, &ctm);
		HG_VM_UNLOCK (vm, arg2);

		hg_stack_roll(ostack, 3, 1);
	}

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

/* <any> type <name> */
DEFUNC_OPER (type)
{
	hg_quark_t arg0, q, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	q = HG_QNAME (hg_quark_get_type_name(arg0));
	hg_vm_quark_set_executable(vm, &q, TRUE); /* ??? */

	*ret = q;

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (uappend);
DEFUNC_UNIMPLEMENTED_OPER (ucache);
DEFUNC_UNIMPLEMENTED_OPER (ucachestatus);
DEFUNC_UNIMPLEMENTED_OPER (ueofill);
DEFUNC_UNIMPLEMENTED_OPER (ufill);
DEFUNC_UNIMPLEMENTED_OPER (undefineresource);
DEFUNC_UNIMPLEMENTED_OPER (undefineuserobject);
DEFUNC_UNIMPLEMENTED_OPER (upath);

/* - usertime <int> */
DEFUNC_OPER (usertime)
{
	hg_int_t i = hg_vm_get_current_time(vm);

	STACK_PUSH (ostack, HG_QINT (i));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ustroke);
DEFUNC_UNIMPLEMENTED_OPER (ustrokepath);
DEFUNC_UNIMPLEMENTED_OPER (viewclip);
DEFUNC_UNIMPLEMENTED_OPER (viewclippath);

/* <int> vmreclaim - */
DEFUNC_OPER (vmreclaim)
{
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	if (!HG_IS_QINT (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	switch (HG_INT (arg0)) {
	    case -2:
		    hg_mem_spool_enable_gc(vm->mem[HG_VM_MEM_GLOBAL], FALSE);
	    case -1:
		    hg_mem_spool_enable_gc(vm->mem[HG_VM_MEM_LOCAL], FALSE);
		    break;
	    case 0:
		    /* XXX: no multi-context support yet */
		    hg_mem_spool_enable_gc(vm->mem[HG_VM_MEM_GLOBAL], FALSE);
		    hg_mem_spool_enable_gc(vm->mem[HG_VM_MEM_LOCAL], FALSE);
		    break;
	    case 2:
		    hg_mem_spool_run_gc(vm->mem[HG_VM_MEM_GLOBAL]);
	    case 1:
		    hg_mem_spool_run_gc(vm->mem[HG_VM_MEM_LOCAL]);
		    break;
	    default:
		    hg_error_return (HG_STATUS_FAILED, HG_e_rangecheck);
	}

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

/* - vmstatus <level> <used> <maximum> */
DEFUNC_OPER (vmstatus)
{
	hg_mem_t *mem = hg_vm_get_mem(vm);

	STACK_PUSH (ostack, HG_QINT (vm->vm_state->n_save_objects));
	STACK_PUSH (ostack, HG_QINT (hg_mem_spool_get_used_size(mem)));
	STACK_PUSH (ostack, HG_QINT (hg_mem_spool_get_total_size(mem) - hg_mem_spool_get_used_size(mem)));

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (3);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (wait);

/* <array> wcheck <bool>
 * <packedarray> wcheck <false>
 * <dict> wcheck <bool>
 * <file> wcheck <bool>
 * <string> wcheck <bool>
 */
DEFUNC_OPER (wcheck)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;
	if (!HG_IS_QARRAY (arg0) &&
	    !HG_IS_QDICT (arg0) &&
	    !HG_IS_QFILE (arg0) &&
	    !HG_IS_QSTRING (arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	*ret = HG_QBOOL (hg_vm_quark_is_writable(vm, &arg0));

	retval = TRUE;
} DEFUNC_OPER_END

/* <key> where <dict> <true>
 * <key> where <false>
 */
DEFUNC_OPER (where)
{
	hg_quark_t arg0, q = Qnil, qq = Qnil;
	hg_usize_t ddepth = hg_stack_depth(dstack), i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0);
	for (i = 0; i < ddepth; i++) {
		q = hg_stack_index(dstack, i);
		qq = hg_vm_dict_lookup(vm, q, arg0, TRUE);
		if (!HG_ERROR_IS_SUCCESS0 ()) {
			return FALSE;
		}
		if (qq != Qnil)
			break;
	}

	hg_stack_drop(ostack);

	if (qq == Qnil) {
		STACK_PUSH (ostack, HG_QBOOL (FALSE));
	} else {
		STACK_PUSH (ostack, q);
		STACK_PUSH (ostack, HG_QBOOL (TRUE));
		SET_EXPECTED_OSTACK_SIZE (1);
	}

	retval = TRUE;
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (widthshow);

/* <file> <int> write - */
DEFUNC_OPER (write)
{
	hg_quark_t arg0, arg1;
	hg_file_t *f;
	hg_char_t c[2];

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);

	if (!HG_IS_QFILE (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	f = HG_VM_LOCK (vm, arg0);
	if (!f) {
		return FALSE;
	}
	c[0] = HG_INT (arg1) % 256;
	retval = (hg_file_write(f, c, sizeof (hg_char_t), 1) > 0);
	HG_VM_UNLOCK (vm, arg0);

	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-2);
} DEFUNC_OPER_END

/* <file> <string> writehexstring - */
DEFUNC_OPER (writehexstring)
{
	hg_quark_t arg0, arg1;
	hg_file_t *f;
	hg_string_t *s;
	hg_char_t *cstr = NULL;
	hg_usize_t length, i;
	hg_error_reason_t e = 0;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QFILE (arg0) ||
	    !HG_IS_QSTRING (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	f = HG_VM_LOCK (vm, arg0);
	s = HG_VM_LOCK (vm, arg1);
	if (f == NULL ||
	    s == NULL) {
		e = HG_e_VMerror;
		goto error;
	}
	cstr = hg_string_get_cstr(s);
	length = hg_string_maxlength(s);
	for (i = 0; i < length; i++) {
		if (hg_file_append_printf(f, "%x", cstr[i] & 0xff) < 0) {
			e = HG_e_ioerror;
			goto error;
		}
	}

	retval = TRUE;
	hg_stack_drop(ostack);
	hg_stack_drop(ostack);
	SET_EXPECTED_OSTACK_SIZE (-2);
  error:
	HG_VM_UNLOCK (vm, arg0);
	HG_VM_UNLOCK (vm, arg1);
	hg_free(cstr);
	if (e)
		hg_error_return (HG_STATUS_FAILED, e);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (writeobject);

/* <file> <string> writestring - */
DEFUNC_OPER (writestring)
{
	hg_quark_t arg0, arg1;
	hg_file_t *f;
	hg_string_t *s;
	hg_char_t *cstr;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1);
	arg1 = hg_stack_index(ostack, 0);
	if (!HG_IS_QFILE (arg0) ||
	    !HG_IS_QSTRING (arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}
	if (!hg_vm_quark_is_writable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_error_return (HG_STATUS_FAILED, HG_e_invalidaccess);
	}
	f = HG_VM_LOCK (vm, arg0);
	if (!f)
		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	s = HG_VM_LOCK (vm, arg1);
	if (!s) {
		HG_VM_UNLOCK (vm, arg0);

		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	}
	cstr = hg_string_get_cstr(s);
	hg_file_write(f, cstr,
		      sizeof (hg_char_t), hg_string_length(s));
	hg_free(cstr);
	if (HG_ERROR_IS_SUCCESS0 ()) {
		retval = TRUE;
		hg_stack_drop(ostack);
		hg_stack_drop(ostack);
	}
	SET_EXPECTED_OSTACK_SIZE (-2);
	HG_VM_UNLOCK (vm, arg0);
	HG_VM_UNLOCK (vm, arg1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (wtranslation);

/* <any> xcheck <bool> */
DEFUNC_OPER (xcheck)
{
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0);
	arg0 = *ret;

	*ret = HG_QBOOL (hg_vm_quark_is_executable(vm, &arg0));

	retval = TRUE;
} DEFUNC_OPER_END

/* <bool1> <bool2> xor <bool3>
 * <int1> <int2> xor <int3>
 */
DEFUNC_OPER (xor)
{
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0);
	if (HG_IS_QBOOL (arg0) &&
	    HG_IS_QBOOL (arg1)) {
		*ret = HG_QBOOL (HG_BOOL (arg0) ^ HG_BOOL (arg1));
	} else if (HG_IS_QINT (arg0) &&
		   HG_IS_QINT (arg1)) {
		*ret = HG_QINT (HG_INT (arg0) ^ HG_INT (arg1));
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_typecheck);
	}

	hg_stack_drop(ostack);

	retval = TRUE;
	SET_EXPECTED_OSTACK_SIZE (-1);
} DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (xshow);
DEFUNC_UNIMPLEMENTED_OPER (xyshow);
DEFUNC_UNIMPLEMENTED_OPER (yield);
DEFUNC_UNIMPLEMENTED_OPER (yshow);


#define REG_OPER(_d_,_o_)						\
	HG_STMT_START {							\
		hg_quark_t __o_name__ = hg_name_new_with_encoding(HG_enc_ ## _o_); \
		hg_quark_t __op__ = HG_QOPER (HG_enc_ ## _o_);		\
									\
		if (!hg_dict_add((_d_),					\
				 __o_name__,				\
				 __op__,				\
				 FALSE))				\
			return FALSE;					\
	} HG_STMT_END
#define REG_PRIV_OPER(_d_,_k_,_o_)					\
	HG_STMT_START {							\
		hg_quark_t __o_name__ = HG_QNAME (#_k_);		\
		hg_quark_t __op__ = HG_QOPER (HG_enc_ ## _o_);		\
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
		hg_vm_quark_set_readable(vm, &__v__, TRUE);	\
		if (!hg_dict_add((_d_),				\
				 __o_name__,			\
				 __v__,				\
				 FALSE))			\
			return FALSE;				\
	} HG_STMT_END

static hg_bool_t
_hg_operator_level1_register(hg_vm_t   *vm,
			     hg_dict_t *dict)
{
	REG_VALUE (dict, false, HG_QBOOL (FALSE));
	REG_VALUE (dict, mark, HG_QMARK);
	REG_VALUE (dict, null, HG_QNULL);
	REG_VALUE (dict, true, HG_QBOOL (TRUE));
	REG_VALUE (dict, [, HG_QMARK);
	REG_VALUE (dict, ], HG_QEVALNAME ("%arraytomark"));

	REG_PRIV_OPER (dict, .abort, private_abort);
	REG_PRIV_OPER (dict, .applyparams, private_applyparams);
	REG_PRIV_OPER (dict, .clearerror, private_clearerror);
	REG_PRIV_OPER (dict, .callbeginpage, private_callbeginpage);
	REG_PRIV_OPER (dict, .callendpage, private_callendpage);
	REG_PRIV_OPER (dict, .callinstall, private_callinstall);
	REG_PRIV_OPER (dict, .currentpagedevice, private_currentpagedevice);
	REG_PRIV_OPER (dict, .dicttomark, protected_dicttomark);
	REG_PRIV_OPER (dict, .exit, private_exit);
	REG_PRIV_OPER (dict, .findlibfile, private_findlibfile);
	REG_PRIV_OPER (dict, .forceput, private_forceput);
	REG_PRIV_OPER (dict, .hgrevision, private_hgrevision);
	REG_PRIV_OPER (dict, .odef, private_odef);
	REG_PRIV_OPER (dict, .product, private_product);
	REG_PRIV_OPER (dict, .quit, private_quit);
	REG_PRIV_OPER (dict, .revision, private_revision);
	REG_PRIV_OPER (dict, .setglobal, private_setglobal);
	REG_PRIV_OPER (dict, .stringcvs, private_stringcvs);
	REG_PRIV_OPER (dict, .undef, private_undef);
	REG_PRIV_OPER (dict, .write==only, private_write_eqeq_only);

	REG_PRIV_OPER (dict, %arraytomark, protected_arraytomark);
	REG_PRIV_OPER (dict, %for_yield_int_continue, protected_for_yield_int_continue);
	REG_PRIV_OPER (dict, %for_yield_real_continue, protected_for_yield_real_continue);
	REG_PRIV_OPER (dict, %forall_array_continue, protected_forall_array_continue);
	REG_PRIV_OPER (dict, %forall_dict_continue, protected_forall_dict_continue);
	REG_PRIV_OPER (dict, %forall_string_continue, protected_forall_string_continue);
	REG_PRIV_OPER (dict, %loop_continue, protected_loop_continue);
	REG_PRIV_OPER (dict, %repeat_continue, protected_repeat_continue);
	REG_PRIV_OPER (dict, %stopped_continue, protected_stopped_continue);
	REG_PRIV_OPER (dict, eexec, eexec);

	REG_OPER (dict, abs);
	REG_OPER (dict, add);
	REG_OPER (dict, aload);
	REG_OPER (dict, and);
	REG_OPER (dict, arc);
	REG_OPER (dict, arcn);
	REG_OPER (dict, arcto);
	REG_OPER (dict, array);
	REG_OPER (dict, ashow);
	REG_OPER (dict, astore);
	REG_OPER (dict, awidthshow);
	REG_OPER (dict, begin);
	REG_OPER (dict, bind);
	REG_OPER (dict, bitshift);
	REG_OPER (dict, ceiling);
	REG_OPER (dict, charpath);
	REG_OPER (dict, clear);
	REG_OPER (dict, cleartomark);
	REG_OPER (dict, clip);
	REG_OPER (dict, clippath);
	REG_OPER (dict, closepath);
	REG_OPER (dict, concat);
	REG_OPER (dict, concatmatrix);
	REG_OPER (dict, copy);
	REG_OPER (dict, count);
	REG_OPER (dict, counttomark);
	REG_OPER (dict, currentdash);
	REG_OPER (dict, currentdict);
	REG_OPER (dict, currentfile);
	REG_OPER (dict, currentfont);
	REG_OPER (dict, currentgray);
	REG_OPER (dict, currenthsbcolor);
	REG_OPER (dict, currentlinecap);
	REG_OPER (dict, currentlinejoin);
	REG_OPER (dict, currentlinewidth);
	REG_OPER (dict, currentmatrix);
	REG_OPER (dict, currentpoint);
	REG_OPER (dict, currentrgbcolor);
	REG_OPER (dict, curveto);
	REG_OPER (dict, cvi);
	REG_OPER (dict, cvlit);
	REG_OPER (dict, cvn);
	REG_OPER (dict, cvr);
	REG_OPER (dict, cvrs);
	REG_OPER (dict, cvx);
	REG_OPER (dict, def);
//	REG_OPER (dict, defineusername); /* ??? */
	REG_OPER (dict, dict);
	REG_OPER (dict, div);
	REG_OPER (dict, dtransform);
	REG_OPER (dict, dup);
	REG_OPER (dict, end);
	REG_OPER (dict, eoclip);
	REG_OPER (dict, eofill);
//	REG_OPER (dict, eoviewclip); /* ??? */
	REG_OPER (dict, eq);
	REG_OPER (dict, exch);
	REG_OPER (dict, exec);
	REG_OPER (dict, exit);
	REG_OPER (dict, file);
	REG_OPER (dict, fill);
	REG_OPER (dict, flattenpath);
	REG_OPER (dict, flush);
	REG_OPER (dict, flushfile);
	REG_OPER (dict, for);
	REG_OPER (dict, forall);
	REG_OPER (dict, ge);
	REG_OPER (dict, get);
	REG_OPER (dict, getinterval);
	REG_OPER (dict, grestore);
	REG_OPER (dict, gsave);
	REG_OPER (dict, gt);
	REG_OPER (dict, identmatrix);
	REG_OPER (dict, idiv);
	REG_OPER (dict, idtransform);
	REG_OPER (dict, if);
	REG_OPER (dict, ifelse);
	REG_OPER (dict, image);
	REG_OPER (dict, imagemask);
	REG_OPER (dict, index);
//	REG_OPER (dict, initviewclip); /* ??? */
	REG_OPER (dict, invertmatrix);
	REG_OPER (dict, itransform);
	REG_OPER (dict, known);
	REG_OPER (dict, le);
	REG_OPER (dict, length);
	REG_OPER (dict, lineto);
	REG_OPER (dict, loop);
	REG_OPER (dict, lt);
	REG_OPER (dict, makefont);
	REG_OPER (dict, maxlength);
	REG_OPER (dict, mod);
	REG_OPER (dict, moveto);
	REG_OPER (dict, mul);
	REG_OPER (dict, ne);
	REG_OPER (dict, neg);
	REG_OPER (dict, newpath);
	REG_OPER (dict, not);
	REG_OPER (dict, or);
	REG_OPER (dict, pathbbox);
	REG_OPER (dict, pathforall);
	REG_OPER (dict, pop);
	REG_OPER (dict, print);
	REG_OPER (dict, put);
	REG_OPER (dict, rcurveto);
	REG_OPER (dict, read);
	REG_OPER (dict, readhexstring);
	REG_OPER (dict, readline);
	REG_OPER (dict, readstring);
//	REG_OPER (dict, rectviewclip); /* ??? */
	REG_OPER (dict, repeat);
	REG_OPER (dict, restore);
	REG_OPER (dict, rlineto);
	REG_OPER (dict, rmoveto);
	REG_OPER (dict, roll);
	REG_OPER (dict, rotate);
	REG_OPER (dict, round);
	REG_OPER (dict, save);
	REG_OPER (dict, scale);
	REG_OPER (dict, scalefont);
	REG_OPER (dict, search);
	REG_OPER (dict, setcachedevice);
	REG_OPER (dict, setcharwidth);
	REG_OPER (dict, setdash);
	REG_OPER (dict, setfont);
	REG_OPER (dict, setgray);
	REG_OPER (dict, setgstate);
	REG_OPER (dict, sethsbcolor);
	REG_OPER (dict, setlinecap);
	REG_OPER (dict, setlinejoin);
	REG_OPER (dict, setlinewidth);
	REG_OPER (dict, setmatrix);
	REG_OPER (dict, setrgbcolor);
	REG_OPER (dict, show);
	REG_OPER (dict, showpage);
	REG_OPER (dict, stop);
	REG_OPER (dict, stopped);
	REG_OPER (dict, store);
	REG_OPER (dict, string);
	REG_OPER (dict, stringwidth);
	REG_OPER (dict, stroke);
	REG_OPER (dict, strokepath);
	REG_OPER (dict, sub);
	REG_OPER (dict, token);
	REG_OPER (dict, transform);
	REG_OPER (dict, translate);
	REG_OPER (dict, type);
//	REG_OPER (dict, viewclip); /* ??? */
//	REG_OPER (dict, viewclippath); /* ??? */
	REG_OPER (dict, where);
	REG_OPER (dict, widthshow);
	REG_OPER (dict, write);
	REG_OPER (dict, writehexstring);
	REG_OPER (dict, writestring);
//	REG_OPER (dict, wtranslation); /* ??? */
	REG_OPER (dict, xor);

	REG_OPER (dict, FontDirectory);

	REG_OPER (dict, atan);
//	REG_OPER (dict, banddevice); /* ??? */
	REG_OPER (dict, bytesavailable);
	REG_OPER (dict, cachestatus);
	REG_OPER (dict, closefile);
//	REG_OPER (dict, condition); /* ??? */
	REG_OPER (dict, copypage);

	REG_OPER (dict, cos);
	REG_OPER (dict, countdictstack);
	REG_OPER (dict, countexecstack);
//	REG_OPER (dict, currentcontext); /* ??? */
	REG_OPER (dict, currentflat);

//	REG_OPER (dict, currenthalftonephase); /* ??? */
	REG_OPER (dict, currentmiterlimit);
	REG_OPER (dict, currentscreen);
	REG_OPER (dict, currenttransfer);
	REG_OPER (dict, defaultmatrix);

//	REG_OPER (dict, detach); /* ??? */
//	REG_OPER (dict, deviceinfo); /* ??? */
	REG_OPER (dict, dictstack);
	REG_OPER (dict, echo);
	REG_OPER (dict, erasepage);
	REG_OPER (dict, execstack);
	REG_OPER (dict, executeonly);

	REG_OPER (dict, exp);
//	REG_OPER (dict, fork); /* ??? */
//	REG_OPER (dict, framedevice); /* ??? */
	REG_OPER (dict, grestoreall);
	REG_OPER (dict, initclip);

//	REG_OPER (dict, join); /* ??? */
	REG_OPER (dict, kshow);
	REG_OPER (dict, ln);
//	REG_OPER (dict, lock); /* ??? */
	REG_OPER (dict, log);
//	REG_OPER (dict, monitor); /* ??? */

	REG_OPER (dict, noaccess);
//	REG_OPER (dict, notify); /* ??? */
	REG_OPER (dict, nulldevice);
	REG_OPER (dict, packedarray);
	REG_OPER (dict, rand);
	REG_OPER (dict, rcheck);
	REG_OPER (dict, readonly);

//	REG_OPER (dict, renderbands); /* ??? */
	REG_OPER (dict, resetfile);
	REG_OPER (dict, reversepath);
	REG_OPER (dict, rrand);
	REG_OPER (dict, setcachelimit);
	REG_OPER (dict, setflat);
//	REG_OPER (dict, sethalftonephase); /* ??? */
	REG_OPER (dict, setmiterlimit);
	REG_OPER (dict, setscreen);

	REG_OPER (dict, settransfer);
	REG_OPER (dict, sin);
	REG_OPER (dict, sqrt);
	REG_OPER (dict, srand);
	REG_OPER (dict, stack);
	REG_OPER (dict, status);

	REG_OPER (dict, usertime);
	REG_OPER (dict, vmstatus);
//	REG_OPER (dict, wait); /* ??? */
	REG_OPER (dict, wcheck);

	REG_OPER (dict, xcheck);
//	REG_OPER (dict, yield); /* ??? */
	REG_OPER (dict, cleardictstack);

	REG_OPER (dict, sym_begin_dict_mark);

	REG_OPER (dict, sym_end_dict_mark);


#if 0
	REG_OPER (dict, ASCII85Decode);
	REG_OPER (dict, ASCII85Encode);
	REG_OPER (dict, ASCIIHexDecode);
	REG_OPER (dict, ASCIIHexEncode);

	REG_OPER (dict, CCITTFaxDecode);
	REG_OPER (dict, CCITTFaxEncode);
	REG_OPER (dict, DCTDecode);
	REG_OPER (dict, DCTEncode);
	REG_OPER (dict, LZWDecode);
	REG_OPER (dict, LZWEncode);
	REG_OPER (dict, NullEncode);
	REG_OPER (dict, RunLengthDecode);
	REG_OPER (dict, RunLengthEncode);
	REG_OPER (dict, SubFileDecode);

	REG_OPER (dict, CIEBasedA);
	REG_OPER (dict, CIEBasedABC);
	REG_OPER (dict, DeviceCMYK);
	REG_OPER (dict, DeviceGray);
	REG_OPER (dict, DeviceRGB);
	REG_OPER (dict, Indexed);
	REG_OPER (dict, Pattern);
	REG_OPER (dict, Separation);
	REG_OPER (dict, CIEBasedDEF);
	REG_OPER (dict, CIEBasedDEFG);

	REG_OPER (dict, DeviceN);
#endif

	return TRUE;
}

static hg_bool_t
_hg_operator_level2_register(hg_vm_t   *vm,
			     hg_dict_t *dict)
{
	REG_VALUE (dict, <<, HG_QMARK);
	REG_VALUE (dict, >>, HG_QEVALNAME ("%dicttomark"));

	REG_PRIV_OPER (dict, %dicttomark, protected_dicttomark);

	REG_OPER (dict, arct);
	REG_OPER (dict, colorimage);
	REG_OPER (dict, currentcmykcolor);
	REG_OPER (dict, currentgstate);
	REG_OPER (dict, currentshared);
	REG_OPER (dict, gstate);
	REG_OPER (dict, ineofill);
	REG_OPER (dict, infill);
	REG_OPER (dict, inueofill);
	REG_OPER (dict, inufill);
	REG_OPER (dict, printobject);
	REG_OPER (dict, rectclip);
	REG_OPER (dict, rectfill);
	REG_OPER (dict, rectstroke);
	REG_OPER (dict, selectfont);
	REG_OPER (dict, setbbox);
	REG_OPER (dict, setcachedevice2);
	REG_OPER (dict, setcmykcolor);
	REG_OPER (dict, setshared);
	REG_OPER (dict, shareddict);
	REG_OPER (dict, uappend);
	REG_OPER (dict, ucache);
	REG_OPER (dict, ueofill);
	REG_OPER (dict, ufill);
	REG_OPER (dict, upath);
	REG_OPER (dict, ustroke);
	REG_OPER (dict, writeobject);
	REG_OPER (dict, xshow);
	REG_OPER (dict, xyshow);
	REG_OPER (dict, yshow);
	REG_OPER (dict, SharedFontDirectory);
	REG_OPER (dict, execuserobject);
	REG_OPER (dict, currentcolor);
	REG_OPER (dict, currentcolorspace);
	REG_OPER (dict, currentglobal);
	REG_OPER (dict, execform);
	REG_OPER (dict, filter);
	REG_OPER (dict, findresource);

	REG_OPER (dict, makepattern);
	REG_OPER (dict, setcolor);
	REG_OPER (dict, setcolorspace);
	REG_OPER (dict, setglobal);
	REG_OPER (dict, setpagedevice);
	REG_OPER (dict, setpattern);

	REG_OPER (dict, cshow);
	REG_OPER (dict, currentblackgeneration);
	REG_OPER (dict, currentcacheparams);
	REG_OPER (dict, currentcolorscreen);
	REG_OPER (dict, currentcolortransfer);
	REG_OPER (dict, currenthalftone);
	REG_OPER (dict, currentobjectformat);
	REG_OPER (dict, currentpacking);
	REG_OPER (dict, currentstrokeadjust);
	REG_OPER (dict, currentundercolorremoval);
	REG_OPER (dict, deletefile);
	REG_OPER (dict, filenameforall);
	REG_OPER (dict, fileposition);
	REG_OPER (dict, instroke);
	REG_OPER (dict, inustroke);
	REG_OPER (dict, realtime);
	REG_OPER (dict, renamefile);
	REG_OPER (dict, rootfont);
	REG_OPER (dict, setblackgeneration);
	REG_OPER (dict, setcacheparams);

	REG_OPER (dict, setcolorscreen);
	REG_OPER (dict, setcolortransfer);
	REG_OPER (dict, setfileposition);
	REG_OPER (dict, sethalftone);
	REG_OPER (dict, setobjectformat);
	REG_OPER (dict, setpacking);
	REG_OPER (dict, setstrokeadjust);
	REG_OPER (dict, setucacheparams);
	REG_OPER (dict, setundercolorremoval);
	REG_OPER (dict, ucachestatus);
	REG_OPER (dict, ustrokepath);
	REG_OPER (dict, vmreclaim);
	REG_OPER (dict, defineuserobject);
	REG_OPER (dict, undefineuserobject);
	REG_OPER (dict, UserObjects);
	REG_OPER (dict, setvmthreshold);
	REG_OPER (dict, currentcolorrendering);
	REG_OPER (dict, currentdevparams);
	REG_OPER (dict, currentoverprint);
	REG_OPER (dict, currentpagedevice);
	REG_OPER (dict, currentsystemparams);
	REG_OPER (dict, currentuserparams);
	REG_OPER (dict, defineresource);
	REG_OPER (dict, findencoding);
	REG_OPER (dict, gcheck);
	REG_OPER (dict, glyphshow);
	REG_OPER (dict, languagelevel);
	REG_OPER (dict, product);
	REG_OPER (dict, resourceforall);
	REG_OPER (dict, resourcestatus);
	REG_OPER (dict, serialnumber);
	REG_OPER (dict, setcolorrendering);
	REG_OPER (dict, setdevparams);

	REG_OPER (dict, setoverprint);
	REG_OPER (dict, setsystemparams);
	REG_OPER (dict, setuserparams);
	REG_OPER (dict, startjob);
	REG_OPER (dict, undefineresource);
	REG_OPER (dict, GlobalFontDirectory);

	return TRUE;
}

static hg_bool_t
_hg_operator_level3_register(hg_vm_t   *vm,
			     hg_dict_t *dict)
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
hg_bool_t
hg_operator_init(void)
{
#define DECL_OPER(_n_)							\
	HG_STMT_START {							\
		__hg_operator_name_table[HG_enc_ ## _n_] = hg_strdup("--" #_n_ "--"); \
		if (__hg_operator_name_table[HG_enc_ ## _n_] == NULL)	\
			return FALSE;					\
		__hg_operator_func_table[HG_enc_ ## _n_] = OPER_FUNC_NAME (_n_); \
	} HG_STMT_END
#define DECL_PRIV_OPER(_on_,_n_)						\
	HG_STMT_START {							\
		__hg_operator_name_table[HG_enc_ ## _n_] = hg_strdup("--" #_on_ "--"); \
		if (__hg_operator_name_table[HG_enc_ ## _n_] == NULL)	\
			return FALSE;					\
		__hg_operator_func_table[HG_enc_ ## _n_] = OPER_FUNC_NAME (_n_); \
	} HG_STMT_END

	DECL_PRIV_OPER (.abort, private_abort);
	DECL_PRIV_OPER (.applyparams, private_applyparams);
	DECL_PRIV_OPER (.clearerror, private_clearerror);
	DECL_PRIV_OPER (.callbeginpage, private_callbeginpage);
	DECL_PRIV_OPER (.callendpage, private_callendpage);
	DECL_PRIV_OPER (.callinstall, private_callinstall);
	DECL_PRIV_OPER (.currentpagedevice, private_currentpagedevice);
	DECL_PRIV_OPER (.exit, private_exit);
	DECL_PRIV_OPER (.findlibfile, private_findlibfile);
	DECL_PRIV_OPER (.forceput, private_forceput);
	DECL_PRIV_OPER (.hgrevision, private_hgrevision);
	DECL_PRIV_OPER (.odef, private_odef);
	DECL_PRIV_OPER (.product, private_product);
	DECL_PRIV_OPER (.quit, private_quit);
	DECL_PRIV_OPER (.revision, private_revision);
	DECL_PRIV_OPER (.setglobal, private_setglobal);
	DECL_PRIV_OPER (.stringcvs, private_stringcvs);
	DECL_PRIV_OPER (.undef, private_undef);
	DECL_PRIV_OPER (.write==only, private_write_eqeq_only);

	DECL_PRIV_OPER (%arraytomark, protected_arraytomark);
	DECL_PRIV_OPER (%dicttomark, protected_dicttomark);
	DECL_PRIV_OPER (%for_yield_int_continue, protected_for_yield_int_continue);
	DECL_PRIV_OPER (%for_yield_real_continue, protected_for_yield_real_continue);
	DECL_PRIV_OPER (%forall_array_continue, protected_forall_array_continue);
	DECL_PRIV_OPER (%forall_dict_continue, protected_forall_dict_continue);
	DECL_PRIV_OPER (%forall_string_continue, protected_forall_string_continue);
	DECL_PRIV_OPER (%loop_continue, protected_loop_continue);
	DECL_PRIV_OPER (%repeat_continue, protected_repeat_continue);
	DECL_PRIV_OPER (%stopped_continue, protected_stopped_continue);

	DECL_OPER (ASCII85Decode);
	DECL_OPER (ASCII85Encode);
	DECL_OPER (ASCIIHexDecode);
	DECL_OPER (ASCIIHexEncode);
	DECL_OPER (CCITTFaxDecode);
	DECL_OPER (CCITTFaxEncode);
	DECL_OPER (CIEBasedA);
	DECL_OPER (CIEBasedABC);
	DECL_OPER (CIEBasedDEF);
	DECL_OPER (CIEBasedDEFG);
	DECL_OPER (DCTDecode);
	DECL_OPER (DCTEncode);
	DECL_OPER (DeviceCMYK);
	DECL_OPER (DeviceGray);
	DECL_OPER (DeviceN);
	DECL_OPER (DeviceRGB);
	DECL_OPER (FontDirectory);
	DECL_OPER (GlobalFontDirectory);
	DECL_OPER (ISOLatin1Encoding);
	DECL_OPER (Indexed);
	DECL_OPER (LZWDecode);
	DECL_OPER (LZWEncode);
	DECL_OPER (NullEncode);
	DECL_OPER (Pattern);
	DECL_OPER (RunLengthDecode);
	DECL_OPER (RunLengthEncode);
	DECL_OPER (Separation);
	DECL_OPER (SharedFontDirectory);
	DECL_OPER (SubFileDecode);
	DECL_OPER (UserObjects);
	DECL_OPER (abs);
	DECL_OPER (add);
	DECL_OPER (aload);
	DECL_OPER (and);
	DECL_OPER (arc);
	DECL_OPER (arcn);
	DECL_OPER (arct);
	DECL_OPER (arcto);
	DECL_OPER (array);
	DECL_OPER (ashow);
	DECL_OPER (astore);
	DECL_OPER (atan);
	DECL_OPER (awidthshow);
	DECL_OPER (banddevice);
	DECL_OPER (begin);
	DECL_OPER (bind);
	DECL_OPER (bitshift);
	DECL_OPER (bytesavailable);
	DECL_OPER (cachestatus);
	DECL_OPER (ceiling);
	DECL_OPER (charpath);
	DECL_OPER (clear);
	DECL_OPER (cleardictstack);
	DECL_OPER (cleartomark);
	DECL_OPER (clip);
	DECL_OPER (clippath);
	DECL_OPER (closefile);
	DECL_OPER (closepath);
	DECL_OPER (colorimage);
	DECL_OPER (concat);
	DECL_OPER (concatmatrix);
	DECL_OPER (condition);
	DECL_OPER (copy);
	DECL_OPER (copypage);
	DECL_OPER (cos);
	DECL_OPER (count);
	DECL_OPER (countdictstack);
	DECL_OPER (countexecstack);
	DECL_OPER (counttomark);
	DECL_OPER (cshow);
	DECL_OPER (currentblackgeneration);
	DECL_OPER (currentcacheparams);
	DECL_OPER (currentcmykcolor);
	DECL_OPER (currentcolor);
	DECL_OPER (currentcolorrendering);
	DECL_OPER (currentcolorscreen);
	DECL_OPER (currentcolorspace);
	DECL_OPER (currentcolortransfer);
	DECL_OPER (currentcontext);
	DECL_OPER (currentdash);
	DECL_OPER (currentdevparams);
	DECL_OPER (currentdict);
	DECL_OPER (currentfile);
	DECL_OPER (currentflat);
	DECL_OPER (currentfont);
	DECL_OPER (currentglobal);
	DECL_OPER (currentgray);
	DECL_OPER (currentgstate);
	DECL_OPER (currenthalftone);
	DECL_OPER (currenthalftonephase);
	DECL_OPER (currenthsbcolor);
	DECL_OPER (currentlinecap);
	DECL_OPER (currentlinejoin);
	DECL_OPER (currentlinewidth);
	DECL_OPER (currentmatrix);
	DECL_OPER (currentmiterlimit);
	DECL_OPER (currentobjectformat);
	DECL_OPER (currentoverprint);
	DECL_OPER (currentpacking);
	DECL_OPER (currentpagedevice);
	DECL_OPER (currentpoint);
	DECL_OPER (currentrgbcolor);
	DECL_OPER (currentscreen);
	DECL_OPER (currentshared);
	DECL_OPER (currentstrokeadjust);
	DECL_OPER (currentsystemparams);
	DECL_OPER (currenttransfer);
	DECL_OPER (currentundercolorremoval);
	DECL_OPER (currentuserparams);
	DECL_OPER (curveto);
	DECL_OPER (cvi);
	DECL_OPER (cvlit);
	DECL_OPER (cvn);
	DECL_OPER (cvr);
	DECL_OPER (cvrs);
	DECL_OPER (cvx);
	DECL_OPER (def);
	DECL_OPER (defaultmatrix);
	DECL_OPER (defineresource);
	DECL_OPER (defineusername);
	DECL_OPER (defineuserobject);
	DECL_OPER (deletefile);
	DECL_OPER (detach);
	DECL_OPER (deviceinfo);
	DECL_OPER (dict);
	DECL_OPER (dictstack);
	DECL_OPER (div);
	DECL_OPER (dtransform);
	DECL_OPER (dup);
	DECL_OPER (echo);
	DECL_OPER (eexec);
	DECL_OPER (end);
	DECL_OPER (eoclip);
	DECL_OPER (eofill);
	DECL_OPER (eoviewclip);
	DECL_OPER (eq);
	DECL_OPER (erasepage);
	DECL_OPER (exch);
	DECL_OPER (exec);
	DECL_OPER (execform);
	DECL_OPER (execstack);
	DECL_OPER (execuserobject);
	DECL_OPER (executeonly);
	DECL_OPER (exit);
	DECL_OPER (exp);
	DECL_OPER (file);
	DECL_OPER (filenameforall);
	DECL_OPER (fileposition);
	DECL_OPER (fill);
	DECL_OPER (filter);
	DECL_OPER (findencoding);
	DECL_OPER (findresource);
	DECL_OPER (flattenpath);
	DECL_OPER (flush);
	DECL_OPER (flushfile);
	DECL_OPER (for);
	DECL_OPER (forall);
	DECL_OPER (fork);
	DECL_OPER (framedevice);
	DECL_OPER (gcheck);
	DECL_OPER (ge);
	DECL_OPER (get);
	DECL_OPER (getinterval);
	DECL_OPER (glyphshow);
	DECL_OPER (grestore);
	DECL_OPER (grestoreall);
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
	DECL_OPER (initclip);
	DECL_OPER (initviewclip);
	DECL_OPER (instroke);
	DECL_OPER (inueofill);
	DECL_OPER (inufill);
	DECL_OPER (inustroke);
	DECL_OPER (invertmatrix);
	DECL_OPER (itransform);
	DECL_OPER (join);
	DECL_OPER (known);
	DECL_OPER (kshow);
	DECL_OPER (languagelevel);
	DECL_OPER (le);
	DECL_OPER (length);
	DECL_OPER (lineto);
	DECL_OPER (ln);
	DECL_OPER (lock);
	DECL_OPER (log);
	DECL_OPER (loop);
	DECL_OPER (lt);
	DECL_OPER (makefont);
	DECL_OPER (makepattern);
	DECL_OPER (maxlength);
	DECL_OPER (mod);
	DECL_OPER (monitor);
	DECL_OPER (moveto);
	DECL_OPER (mul);
	DECL_OPER (ne);
	DECL_OPER (neg);
	DECL_OPER (newpath);
	DECL_OPER (noaccess);
	DECL_OPER (not);
	DECL_OPER (notify);
	DECL_OPER (nulldevice);
	DECL_OPER (or);
	DECL_OPER (packedarray);
	DECL_OPER (pathbbox);
	DECL_OPER (pathforall);
	DECL_OPER (pop);
	DECL_OPER (print);
	DECL_OPER (printobject);
	DECL_OPER (product);
	DECL_OPER (put);
	DECL_OPER (rand);
	DECL_OPER (rcheck);
	DECL_OPER (rcurveto);
	DECL_OPER (read);
	DECL_OPER (readhexstring);
	DECL_OPER (readline);
	DECL_OPER (readonly);
	DECL_OPER (readstring);
	DECL_OPER (realtime);
	DECL_OPER (rectclip);
	DECL_OPER (rectfill);
	DECL_OPER (rectstroke);
	DECL_OPER (rectviewclip);
	DECL_OPER (renamefile);
	DECL_OPER (renderbands);
	DECL_OPER (repeat);
	DECL_OPER (resetfile);
	DECL_OPER (resourceforall);
	DECL_OPER (resourcestatus);
	DECL_OPER (restore);
	DECL_OPER (reversepath);
	DECL_OPER (rlineto);
	DECL_OPER (rmoveto);
	DECL_OPER (roll);
	DECL_OPER (rootfont);
	DECL_OPER (rotate);
	DECL_OPER (round);
	DECL_OPER (rrand);
	DECL_OPER (save);
	DECL_OPER (scale);
	DECL_OPER (scalefont);
	DECL_OPER (search);
	DECL_OPER (selectfont);
	DECL_OPER (serialnumber);
	DECL_OPER (setbbox);
	DECL_OPER (setblackgeneration);
	DECL_OPER (setcachedevice);
	DECL_OPER (setcachedevice2);
	DECL_OPER (setcachelimit);
	DECL_OPER (setcacheparams);
	DECL_OPER (setcharwidth);
	DECL_OPER (setcmykcolor);
	DECL_OPER (setcolor);
	DECL_OPER (setcolorrendering);
	DECL_OPER (setcolorscreen);
	DECL_OPER (setcolorspace);
	DECL_OPER (setcolortransfer);
	DECL_OPER (setdash);
	DECL_OPER (setdevparams);
	DECL_OPER (setfileposition);
	DECL_OPER (setflat);
	DECL_OPER (setfont);
	DECL_OPER (setgray);
	DECL_OPER (setgstate);
	DECL_OPER (sethalftone);
	DECL_OPER (sethalftonephase);
	DECL_OPER (sethsbcolor);
	DECL_OPER (setlinecap);
	DECL_OPER (setlinejoin);
	DECL_OPER (setlinewidth);
	DECL_OPER (setmatrix);
	DECL_OPER (setmiterlimit);
	DECL_OPER (setobjectformat);
	DECL_OPER (setoverprint);
	DECL_OPER (setpacking);
	DECL_OPER (setpagedevice);
	DECL_OPER (setpattern);
	DECL_OPER (setrgbcolor);
	DECL_OPER (setscreen);
	DECL_OPER (setshared);
	DECL_OPER (setstrokeadjust);
	DECL_OPER (setsystemparams);
	DECL_OPER (settransfer);
	DECL_OPER (setucacheparams);
	DECL_OPER (setundercolorremoval);
	DECL_OPER (setuserparams);
	DECL_OPER (setvmthreshold);
	DECL_OPER (shareddict);
	DECL_OPER (show);
	DECL_OPER (showpage);
	DECL_OPER (sin);
	DECL_OPER (sqrt);
	DECL_OPER (srand);
	DECL_OPER (startjob);
	DECL_OPER (status);
	DECL_OPER (stop);
	DECL_OPER (stopped);
	DECL_OPER (string);
	DECL_OPER (stringwidth);
	DECL_OPER (stroke);
	DECL_OPER (strokepath);
	DECL_OPER (sub);
	DECL_OPER (token);
	DECL_OPER (transform);
	DECL_OPER (translate);
	DECL_OPER (type);
	DECL_OPER (uappend);
	DECL_OPER (ucache);
	DECL_OPER (ucachestatus);
	DECL_OPER (ueofill);
	DECL_OPER (ufill);
	DECL_OPER (undefineresource);
	DECL_OPER (undefineuserobject);
	DECL_OPER (upath);
	DECL_OPER (usertime);
	DECL_OPER (ustroke);
	DECL_OPER (ustrokepath);
	DECL_OPER (viewclip);
	DECL_OPER (viewclippath);
	DECL_OPER (vmreclaim);
	DECL_OPER (vmstatus);
	DECL_OPER (wait);
	DECL_OPER (wcheck);
	DECL_OPER (where);
	DECL_OPER (widthshow);
	DECL_OPER (write);
	DECL_OPER (writehexstring);
	DECL_OPER (writeobject);
	DECL_OPER (writestring);
	DECL_OPER (wtranslation);
	DECL_OPER (xcheck);
	DECL_OPER (xor);
	DECL_OPER (xshow);
	DECL_OPER (xyshow);
	DECL_OPER (yield);
	DECL_OPER (yshow);

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
#define UNDECL_OPER(_n_)						\
	HG_STMT_START {							\
		hg_free(__hg_operator_name_table[HG_enc_ ## _n_]);	\
		__hg_operator_name_table[HG_enc_ ## _n_] = NULL;	\
		__hg_operator_func_table[HG_enc_ ## _n_] = NULL;	\
	} HG_STMT_END

	UNDECL_OPER (private_abort);
	UNDECL_OPER (private_applyparams);
	UNDECL_OPER (private_clearerror);
	UNDECL_OPER (private_callbeginpage);
	UNDECL_OPER (private_callendpage);
	UNDECL_OPER (private_callinstall);
	UNDECL_OPER (private_currentpagedevice);
	UNDECL_OPER (private_exit);
	UNDECL_OPER (private_findlibfile);
	UNDECL_OPER (private_forceput);
	UNDECL_OPER (private_hgrevision);
	UNDECL_OPER (private_odef);
	UNDECL_OPER (private_product);
	UNDECL_OPER (private_quit);
	UNDECL_OPER (private_revision);
	UNDECL_OPER (private_setglobal);
	UNDECL_OPER (private_stringcvs);
	UNDECL_OPER (private_undef);
	UNDECL_OPER (private_write_eqeq_only);
	UNDECL_OPER (protected_arraytomark);
	UNDECL_OPER (protected_dicttomark);
	UNDECL_OPER (protected_for_yield_int_continue);
	UNDECL_OPER (protected_for_yield_real_continue);
	UNDECL_OPER (protected_forall_array_continue);
	UNDECL_OPER (protected_forall_dict_continue);
	UNDECL_OPER (protected_forall_string_continue);
	UNDECL_OPER (protected_loop_continue);
	UNDECL_OPER (protected_repeat_continue);
	UNDECL_OPER (protected_stopped_continue);

	UNDECL_OPER (ASCII85Decode);
	UNDECL_OPER (ASCII85Encode);
	UNDECL_OPER (ASCIIHexDecode);
	UNDECL_OPER (ASCIIHexEncode);
	UNDECL_OPER (CCITTFaxDecode);
	UNDECL_OPER (CCITTFaxEncode);
	UNDECL_OPER (CIEBasedA);
	UNDECL_OPER (CIEBasedABC);
	UNDECL_OPER (CIEBasedDEF);
	UNDECL_OPER (CIEBasedDEFG);
	UNDECL_OPER (DCTDecode);
	UNDECL_OPER (DCTEncode);
	UNDECL_OPER (DeviceCMYK);
	UNDECL_OPER (DeviceGray);
	UNDECL_OPER (DeviceN);
	UNDECL_OPER (DeviceRGB);
	UNDECL_OPER (FontDirectory);
	UNDECL_OPER (GlobalFontDirectory);
	UNDECL_OPER (Indexed);
	UNDECL_OPER (LZWDecode);
	UNDECL_OPER (LZWEncode);
	UNDECL_OPER (NullEncode);
	UNDECL_OPER (Pattern);
	UNDECL_OPER (RunLengthDecode);
	UNDECL_OPER (RunLengthEncode);
	UNDECL_OPER (Separation);
	UNDECL_OPER (SharedFontDirectory);
	UNDECL_OPER (SubFileDecode);
	UNDECL_OPER (UserObjects);
	UNDECL_OPER (abs);
	UNDECL_OPER (add);
	UNDECL_OPER (aload);
	UNDECL_OPER (and);
	UNDECL_OPER (arc);
	UNDECL_OPER (arcn);
	UNDECL_OPER (arct);
	UNDECL_OPER (arcto);
	UNDECL_OPER (array);
	UNDECL_OPER (ashow);
	UNDECL_OPER (astore);
	UNDECL_OPER (atan);
	UNDECL_OPER (awidthshow);
	UNDECL_OPER (banddevice);
	UNDECL_OPER (begin);
	UNDECL_OPER (bind);
	UNDECL_OPER (bitshift);
	UNDECL_OPER (bytesavailable);
	UNDECL_OPER (cachestatus);
	UNDECL_OPER (ceiling);
	UNDECL_OPER (charpath);
	UNDECL_OPER (clear);
	UNDECL_OPER (cleardictstack);
	UNDECL_OPER (cleartomark);
	UNDECL_OPER (clip);
	UNDECL_OPER (clippath);
	UNDECL_OPER (closefile);
	UNDECL_OPER (closepath);
	UNDECL_OPER (colorimage);
	UNDECL_OPER (concat);
	UNDECL_OPER (concatmatrix);
	UNDECL_OPER (condition);
	UNDECL_OPER (copy);
	UNDECL_OPER (copypage);
	UNDECL_OPER (cos);
	UNDECL_OPER (count);
	UNDECL_OPER (countdictstack);
	UNDECL_OPER (countexecstack);
	UNDECL_OPER (counttomark);
	UNDECL_OPER (cshow);
	UNDECL_OPER (currentblackgeneration);
	UNDECL_OPER (currentcacheparams);
	UNDECL_OPER (currentcmykcolor);
	UNDECL_OPER (currentcolor);
	UNDECL_OPER (currentcolorrendering);
	UNDECL_OPER (currentcolorscreen);
	UNDECL_OPER (currentcolorspace);
	UNDECL_OPER (currentcolortransfer);
	UNDECL_OPER (currentcontext);
	UNDECL_OPER (currentdash);
	UNDECL_OPER (currentdevparams);
	UNDECL_OPER (currentdict);
	UNDECL_OPER (currentfile);
	UNDECL_OPER (currentflat);
	UNDECL_OPER (currentfont);
	UNDECL_OPER (currentglobal);
	UNDECL_OPER (currentgray);
	UNDECL_OPER (currentgstate);
	UNDECL_OPER (currenthalftone);
	UNDECL_OPER (currenthalftonephase);
	UNDECL_OPER (currenthsbcolor);
	UNDECL_OPER (currentlinecap);
	UNDECL_OPER (currentlinejoin);
	UNDECL_OPER (currentlinewidth);
	UNDECL_OPER (currentmatrix);
	UNDECL_OPER (currentmiterlimit);
	UNDECL_OPER (currentobjectformat);
	UNDECL_OPER (currentoverprint);
	UNDECL_OPER (currentpacking);
	UNDECL_OPER (currentpagedevice);
	UNDECL_OPER (currentpoint);
	UNDECL_OPER (currentrgbcolor);
	UNDECL_OPER (currentscreen);
	UNDECL_OPER (currentshared);
	UNDECL_OPER (currentstrokeadjust);
	UNDECL_OPER (currentsystemparams);
	UNDECL_OPER (currenttransfer);
	UNDECL_OPER (currentundercolorremoval);
	UNDECL_OPER (currentuserparams);
	UNDECL_OPER (curveto);
	UNDECL_OPER (cvi);
	UNDECL_OPER (cvlit);
	UNDECL_OPER (cvn);
	UNDECL_OPER (cvr);
	UNDECL_OPER (cvrs);
	UNDECL_OPER (cvx);
	UNDECL_OPER (def);
	UNDECL_OPER (defaultmatrix);
	UNDECL_OPER (defineresource);
	UNDECL_OPER (defineusername);
	UNDECL_OPER (defineuserobject);
	UNDECL_OPER (deletefile);
	UNDECL_OPER (detach);
	UNDECL_OPER (deviceinfo);
	UNDECL_OPER (dict);
	UNDECL_OPER (dictstack);
	UNDECL_OPER (div);
	UNDECL_OPER (dtransform);
	UNDECL_OPER (dup);
	UNDECL_OPER (echo);
	UNDECL_OPER (eexec);
	UNDECL_OPER (end);
	UNDECL_OPER (eoclip);
	UNDECL_OPER (eofill);
	UNDECL_OPER (eoviewclip);
	UNDECL_OPER (eq);
	UNDECL_OPER (erasepage);
	UNDECL_OPER (exch);
	UNDECL_OPER (exec);
	UNDECL_OPER (execform);
	UNDECL_OPER (execstack);
	UNDECL_OPER (execuserobject);
	UNDECL_OPER (executeonly);
	UNDECL_OPER (exit);
	UNDECL_OPER (exp);
	UNDECL_OPER (file);
	UNDECL_OPER (filenameforall);
	UNDECL_OPER (fileposition);
	UNDECL_OPER (fill);
	UNDECL_OPER (filter);
	UNDECL_OPER (findencoding);
	UNDECL_OPER (findresource);
	UNDECL_OPER (flattenpath);
	UNDECL_OPER (flush);
	UNDECL_OPER (flushfile);
	UNDECL_OPER (for);
	UNDECL_OPER (forall);
	UNDECL_OPER (fork);
	UNDECL_OPER (framedevice);
	UNDECL_OPER (gcheck);
	UNDECL_OPER (ge);
	UNDECL_OPER (get);
	UNDECL_OPER (getinterval);
	UNDECL_OPER (globaldict);
	UNDECL_OPER (glyphshow);
	UNDECL_OPER (grestore);
	UNDECL_OPER (grestoreall);
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
	UNDECL_OPER (initclip);
	UNDECL_OPER (initviewclip);
	UNDECL_OPER (instroke);
	UNDECL_OPER (inueofill);
	UNDECL_OPER (inufill);
	UNDECL_OPER (inustroke);
	UNDECL_OPER (invertmatrix);
	UNDECL_OPER (itransform);
	UNDECL_OPER (join);
	UNDECL_OPER (known);
	UNDECL_OPER (kshow);
	UNDECL_OPER (languagelevel);
	UNDECL_OPER (le);
	UNDECL_OPER (length);
	UNDECL_OPER (lineto);
	UNDECL_OPER (ln);
	UNDECL_OPER (lock);
	UNDECL_OPER (log);
	UNDECL_OPER (loop);
	UNDECL_OPER (lt);
	UNDECL_OPER (makefont);
	UNDECL_OPER (makepattern);
	UNDECL_OPER (mark);
	UNDECL_OPER (maxlength);
	UNDECL_OPER (mod);
	UNDECL_OPER (monitor);
	UNDECL_OPER (moveto);
	UNDECL_OPER (mul);
	UNDECL_OPER (ne);
	UNDECL_OPER (neg);
	UNDECL_OPER (newpath);
	UNDECL_OPER (noaccess);
	UNDECL_OPER (not);
	UNDECL_OPER (notify);
	UNDECL_OPER (nulldevice);
	UNDECL_OPER (or);
	UNDECL_OPER (packedarray);
	UNDECL_OPER (pathbbox);
	UNDECL_OPER (pathforall);
	UNDECL_OPER (pop);
	UNDECL_OPER (print);
	UNDECL_OPER (printobject);
	UNDECL_OPER (product);
	UNDECL_OPER (put);
	UNDECL_OPER (rand);
	UNDECL_OPER (rcheck);
	UNDECL_OPER (rcurveto);
	UNDECL_OPER (read);
	UNDECL_OPER (readhexstring);
	UNDECL_OPER (readline);
	UNDECL_OPER (readonly);
	UNDECL_OPER (readstring);
	UNDECL_OPER (realtime);
	UNDECL_OPER (rectclip);
	UNDECL_OPER (rectfill);
	UNDECL_OPER (rectstroke);
	UNDECL_OPER (rectviewclip);
	UNDECL_OPER (renamefile);
	UNDECL_OPER (renderbands);
	UNDECL_OPER (repeat);
	UNDECL_OPER (resetfile);
	UNDECL_OPER (resourceforall);
	UNDECL_OPER (resourcestatus);
	UNDECL_OPER (restore);
	UNDECL_OPER (reversepath);
	UNDECL_OPER (rlineto);
	UNDECL_OPER (rmoveto);
	UNDECL_OPER (roll);
	UNDECL_OPER (rootfont);
	UNDECL_OPER (rotate);
	UNDECL_OPER (round);
	UNDECL_OPER (rrand);
	UNDECL_OPER (save);
	UNDECL_OPER (scale);
	UNDECL_OPER (scalefont);
	UNDECL_OPER (search);
	UNDECL_OPER (selectfont);
	UNDECL_OPER (serialnumber);
	UNDECL_OPER (setbbox);
	UNDECL_OPER (setblackgeneration);
	UNDECL_OPER (setcachedevice);
	UNDECL_OPER (setcachedevice2);
	UNDECL_OPER (setcachelimit);
	UNDECL_OPER (setcacheparams);
	UNDECL_OPER (setcharwidth);
	UNDECL_OPER (setcmykcolor);
	UNDECL_OPER (setcolor);
	UNDECL_OPER (setcolorrendering);
	UNDECL_OPER (setcolorscreen);
	UNDECL_OPER (setcolorspace);
	UNDECL_OPER (setcolortransfer);
	UNDECL_OPER (setdash);
	UNDECL_OPER (setdevparams);
	UNDECL_OPER (setfileposition);
	UNDECL_OPER (setflat);
	UNDECL_OPER (setfont);
	UNDECL_OPER (setglobal);
	UNDECL_OPER (setgray);
	UNDECL_OPER (setgstate);
	UNDECL_OPER (sethalftone);
	UNDECL_OPER (sethalftonephase);
	UNDECL_OPER (sethsbcolor);
	UNDECL_OPER (setlinecap);
	UNDECL_OPER (setlinejoin);
	UNDECL_OPER (setlinewidth);
	UNDECL_OPER (setmatrix);
	UNDECL_OPER (setmiterlimit);
	UNDECL_OPER (setobjectformat);
	UNDECL_OPER (setoverprint);
	UNDECL_OPER (setpacking);
	UNDECL_OPER (setpagedevice);
	UNDECL_OPER (setpattern);
	UNDECL_OPER (setrgbcolor);
	UNDECL_OPER (setscreen);
	UNDECL_OPER (setshared);
	UNDECL_OPER (setstrokeadjust);
	UNDECL_OPER (setsystemparams);
	UNDECL_OPER (settransfer);
	UNDECL_OPER (setucacheparams);
	UNDECL_OPER (setundercolorremoval);
	UNDECL_OPER (setuserparams);
	UNDECL_OPER (setvmthreshold);
	UNDECL_OPER (shareddict);
	UNDECL_OPER (show);
	UNDECL_OPER (showpage);
	UNDECL_OPER (sin);
	UNDECL_OPER (sqrt);
	UNDECL_OPER (srand);
	UNDECL_OPER (startjob);
	UNDECL_OPER (status);
	UNDECL_OPER (stop);
	UNDECL_OPER (stopped);
	UNDECL_OPER (string);
	UNDECL_OPER (stringwidth);
	UNDECL_OPER (stroke);
	UNDECL_OPER (strokepath);
	UNDECL_OPER (sub);
	UNDECL_OPER (systemdict);
	UNDECL_OPER (token);
	UNDECL_OPER (transform);
	UNDECL_OPER (translate);
	UNDECL_OPER (type);
	UNDECL_OPER (uappend);
	UNDECL_OPER (ucache);
	UNDECL_OPER (ucachestatus);
	UNDECL_OPER (ueofill);
	UNDECL_OPER (ufill);
	UNDECL_OPER (undefineresource);
	UNDECL_OPER (undefineuserobject);
	UNDECL_OPER (upath);
	UNDECL_OPER (usertime);
	UNDECL_OPER (ustroke);
	UNDECL_OPER (ustrokepath);
	UNDECL_OPER (viewclip);
	UNDECL_OPER (viewclippath);
	UNDECL_OPER (vmreclaim);
	UNDECL_OPER (vmstatus);
	UNDECL_OPER (wait);
	UNDECL_OPER (wcheck);
	UNDECL_OPER (where);
	UNDECL_OPER (widthshow);
	UNDECL_OPER (write);
	UNDECL_OPER (writehexstring);
	UNDECL_OPER (writeobject);
	UNDECL_OPER (writestring);
	UNDECL_OPER (wtranslation);
	UNDECL_OPER (xcheck);
	UNDECL_OPER (xor);
	UNDECL_OPER (xshow);
	UNDECL_OPER (xyshow);
	UNDECL_OPER (yield);
	UNDECL_OPER (yshow);

#undef UNDECL_OPER

	__hg_operator_is_initialized = FALSE;
}

/**
 * hg_operator_add_dynamic:
 * @name:
 * @string:
 * @func:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_operator_add_dynamic(const hg_char_t    *string,
			hg_operator_func_t  func)
{
	hg_quark_t n = HG_QNAME (string);

	if (hg_quark_get_value(n) < HG_enc_builtin_HIEROGLYPH_END)
		return Qnil;

	__hg_operator_name_table[hg_quark_get_value(n)] = hg_strdup_printf("--%s--", string);
	__hg_operator_func_table[hg_quark_get_value(n)] = func;

	return n;
}

/**
 * hg_operator_remove_dynamic:
 * @encoding:
 *
 * FIXME
 */
void
hg_operator_remove_dynamic(hg_uint_t encoding)
{
	hg_return_if_fail (encoding >= HG_enc_builtin_HIEROGLYPH_END, HG_e_VMerror);

	hg_free(__hg_operator_name_table[encoding]);
	__hg_operator_name_table[encoding] = NULL;
	__hg_operator_func_table[encoding] = NULL;
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
hg_bool_t
hg_operator_invoke(hg_quark_t  qoper,
		   hg_vm_t    *vm)
{
	hg_quark_t q;
	hg_bool_t retval = TRUE;

	hg_return_val_if_fail (HG_IS_QOPER (qoper), FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vm != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail ((q = hg_quark_get_value(qoper)) < HG_enc_END, FALSE, HG_e_VMerror);

	if (__hg_operator_func_table[q] == NULL) {
		if (__hg_operator_name_table[q] == NULL) {
			hg_warning("Invalid operators - quark: %lx", qoper);
		} else {
			hg_warning("%s operator isn't yet implemented.",
				    __hg_operator_name_table[q]);
		}

		hg_error_return (HG_STATUS_FAILED, HG_e_VMerror);
	} else {
		retval = __hg_operator_func_table[q] (vm);
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
const hg_char_t *
hg_operator_get_name(hg_quark_t qoper)
{
	hg_return_val_if_fail (HG_IS_QOPER (qoper), NULL, HG_e_VMerror);

	return __hg_operator_name_table[hg_quark_get_value(qoper)];
}

/**
 * hg_operator_register:
 * @vm:
 * @dict:
 * @name:
 * @lang_level:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_operator_register(hg_vm_t           *vm,
		     hg_dict_t         *dict,
		     hg_vm_langlevel_t  lang_level)
{
	hg_return_val_if_fail (dict != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (lang_level < HG_LANG_LEVEL_END, FALSE, HG_e_VMerror);

	/* register level 1 built-in operators */
	if (!_hg_operator_level1_register(vm, dict))
		return FALSE;

	/* register level 2 built-in operators */
	if (lang_level >= HG_LANG_LEVEL_2 &&
	    !_hg_operator_level2_register(vm, dict))
		return FALSE;

	/* register level 3 built-in operators */
	if (lang_level >= HG_LANG_LEVEL_3 &&
	    !_hg_operator_level3_register(vm, dict))
		return FALSE;

	return TRUE;
}
