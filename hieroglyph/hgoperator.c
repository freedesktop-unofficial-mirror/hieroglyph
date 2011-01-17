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

#include <stdlib.h>
#include <sys/stat.h>
#include <glib.h>
#include "hgerror.h"
#include "hgbool.h"
#include "hgdevice.h"
#include "hgdict.h"
#include "hggstate.h"
#include "hgint.h"
#include "hgmark.h"
#include "hgmem.h"
#include "hgname.h"
#include "hgnull.h"
#include "hgpath.h"
#include "hgquark.h"
#include "hgreal.h"
#include "hgsnapshot.h"
#include "hgversion.h"
#include "hgvm.h"
#include "hgoperator.h"

#include "hgoperator.proto.h"

static hg_operator_func_t __hg_operator_func_table[HG_enc_END];
static gchar *__hg_operator_name_table[HG_enc_END];
static gboolean __hg_operator_is_initialized = FALSE;

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
G_STMT_START {
	hg_quark_t q;
	hg_file_t *file;

	q = hg_vm_get_io(vm, HG_FILE_IO_STDERR, error);
	file = HG_VM_LOCK (vm, q, error);
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
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <dict> .applyparams - */
DEFUNC_OPER (private_applyparams)
G_STMT_START {
	hg_quark_t arg0;
	hg_dict_t *d;
	GList *l, *keys;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QDICT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	d = HG_VM_LOCK (vm, arg0, error);
	if (d == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	keys = g_hash_table_get_keys(vm->params);
	for (l = keys; l != NULL; l = g_list_next(l)) {
		gchar *name = l->data;
		hg_quark_t qn = HG_QNAME (vm->name, name);

		if (hg_dict_lookup(d, qn, error) == Qnil) {
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
				    g_warning("Unknown parameter type: %d", v->type);
				    break;
			}
			if (q != Qnil)
				hg_dict_add(d, qn, q, FALSE, error);
		}
	}
	g_list_free(keys);

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* - .clearerror - */
DEFUNC_OPER (private_clearerror)
G_STMT_START {
	hg_vm_reset_error(vm);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (private_callbeginpage);
DEFUNC_UNIMPLEMENTED_OPER (private_callendpage);

/* - .callinstall - */
DEFUNC_OPER (private_callinstall)
G_STMT_START {
	hg_device_install(vm->device, vm, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

static hg_quark_t
_hg_operator_pdev_name_lookup(hg_pdev_params_name_t param,
			      gpointer              user_data)
{
	hg_vm_t *vm = (hg_vm_t *)user_data;

	return vm->qpdevparams_name[param];
}

/* - .currentpagedevice <dict> */
DEFUNC_OPER (private_currentpagedevice)
G_STMT_START {
	hg_quark_t q;

	q = hg_device_get_page_params(vm->device,
				      vm->mem[HG_VM_MEM_LOCAL],
				      hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1,
				      _hg_operator_pdev_name_lookup, vm,
				      NULL,
				      error);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_vm_quark_set_default_attributes(vm, &q);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* - .exit - */
DEFUNC_OPER (private_exit)
gssize __n G_GNUC_UNUSED = 0;
G_STMT_START {
	gsize i, j, edepth = hg_stack_depth(estack);
	hg_quark_t q;

	/* almost code is shared with exit.
	 * only difference is to trap on %stopped_continue or not
	 */
	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i, error);
		if (HG_IS_QOPER (q)) {
			if (HG_OPER (q) == HG_enc_protected_for_yield_int_continue ||
			    HG_OPER (q) == HG_enc_protected_for_yield_real_continue) {
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				__n = i + 4;
			} else if (HG_OPER (q) == HG_enc_protected_loop_continue) {
				hg_stack_drop(estack, error);
				__n = i + 1;
			} else if (HG_OPER (q) == HG_enc_protected_repeat_continue) {
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				__n = i + 2;
			} else if (HG_OPER (q) == HG_enc_protected_forall_array_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_dict_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_string_continue) {
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				__n = i + 3;
			} else {
				continue;
			}
			for (j = 0; j < i; j++)
				hg_stack_drop(estack, error);
			break;
		} else if (HG_IS_QFILE (q)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidexit);
			return FALSE;
		}
	}
	if (i == edepth) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidexit);
	} else {
		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (0, -__n, 0);
DEFUNC_OPER_END

/* <filename> .findlibfile <filename> <true>
 * <filename> .findlibfile <false>
 */
DEFUNC_OPER (private_findlibfile)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t arg0, q = Qnil;
	gchar *filename, *cstr;
	gboolean ret = FALSE;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	cstr = hg_vm_string_get_cstr(vm, arg0, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}

	filename = hg_vm_find_libfile(vm, cstr);
	g_free(cstr);

	if (filename) {
		q = HG_QSTRING (hg_vm_get_mem(vm),
				filename);
		g_free(filename);
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		ret = TRUE;
		__flag = TRUE;
	}
	hg_stack_drop(ostack, error);
	if (q != Qnil)
		STACK_PUSH (ostack, q);
	STACK_PUSH (ostack, HG_QBOOL (ret));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__flag ? 1 : 0, 0, 0);
DEFUNC_OPER_END

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

	/* Do not check the permission here.
	 * this operator is likely to put the value into
	 * read-only array. which is copied from the object
	 * in execstack. and this operator is a special
	 * operator to deal with them.
	 */
	if (HG_IS_QARRAY (arg0)) {
		gsize index;

		if (!HG_IS_QINT (arg1)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		index = HG_INT (arg1);
		retval = hg_vm_array_set(vm, arg0, arg2, index, TRUE, error);
		if (!retval) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
	} else if (HG_IS_QDICT (arg0)) {
		retval = hg_vm_dict_add(vm, arg0, arg1, arg2, TRUE, error);
		if (!retval) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
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

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
} G_STMT_END;
VALIDATE_STACK_SIZE (-3, 0, 0);
DEFUNC_OPER_END

/* - .hgrevision <int> */
DEFUNC_OPER (private_hgrevision)
G_STMT_START {
	gint revision = 0;

	sscanf(__hg_rcsid, "$Rev: %d $", &revision);

	STACK_PUSH (ostack, HG_QINT (revision));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* <key> <proc> .odef - */
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
	if (hg_vm_quark_is_executable(vm, &arg1)) {
		hg_array_t *a = HG_VM_LOCK (vm, arg1, error);
		const gchar *name = hg_name_lookup(vm->name, arg0);

		hg_array_set_name(a, name);
		HG_VM_UNLOCK (vm, arg1);

		hg_vm_quark_set_attributes(vm, &arg1, FALSE, FALSE, TRUE, TRUE);
	}
	hg_stack_pop(ostack, error);

	STACK_PUSH (ostack, arg1);
	retval = _hg_operator_real_def(vm, error);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* - .product <string> */
DEFUNC_OPER (private_product)
G_STMT_START {
	hg_quark_t q;

	q = HG_QSTRING (hg_vm_get_mem(vm),
			"Hieroglyph PostScript Interpreter");
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_vm_quark_set_attributes(vm, &q, TRUE, FALSE, FALSE, TRUE);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* <int> .quit - */
DEFUNC_OPER (private_quit)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	hg_vm_shutdown(vm, HG_INT (arg0));

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* - .revision <int> */
DEFUNC_OPER (private_revision)
G_STMT_START {
	gint major = HIEROGLYPH_MAJOR_VERSION;
	gint minor = HIEROGLYPH_MINOR_VERSION;
	gint release = HIEROGLYPH_RELEASE_VERSION;

	STACK_PUSH (ostack,
		    HG_QINT (major * 1000000 + minor * 1000 + release));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
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

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <any> .stringcvs <string> */
DEFUNC_OPER (private_stringcvs)
G_STMT_START {
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0) ||
	    HG_IS_QREAL (arg0) ||
	    HG_IS_QBOOL (arg0) ||
	    HG_IS_QSTRING (arg0) ||
	    HG_IS_QNAME (arg0) ||
	    HG_IS_QOPER (arg0)) {
		q = hg_vm_quark_to_string(vm, arg0, FALSE, NULL, error);
	} else {
		q = HG_QSTRING (hg_vm_get_mem(vm), "--nostringval--");
	}
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <dict> <key> .undef - */
DEFUNC_OPER (private_undef)
G_STMT_START {
	hg_quark_t arg0, arg1;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	hg_vm_dict_remove(vm, arg0, arg1, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <file> <any> .write==only - */
DEFUNC_OPER (private_write_eqeq_only)
G_STMT_START {
	hg_quark_t arg0, arg1, q;
	hg_string_t *s = NULL;
	hg_file_t *f;
	gchar *cstr;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	q = hg_vm_quark_to_string(vm, arg1, TRUE, (gpointer *)&s, error);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);
	if (f == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		hg_string_free(s, TRUE);
		return FALSE;
	}
	cstr = hg_string_get_cstr(s);
	hg_file_write(f, cstr,
		      sizeof (gchar), hg_string_length(s), error);
	HG_VM_UNLOCK (vm, arg0);
	hg_string_free(s, TRUE);
	g_free(cstr);

	if (error && *error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <mark> ... %arraytomark <array> */
DEFUNC_OPER (protected_arraytomark)
G_STMT_START {
	/* %arraytomark is the same to {counttomark array astore exch pop} */
	if (!hg_operator_invoke(HG_QOPER (HG_enc_counttomark), vm, error))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_array), vm, error))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_astore), vm, error))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_exch), vm, error))
		return FALSE;
	if (!hg_operator_invoke(HG_QOPER (HG_enc_pop), vm, error))
		return FALSE;

	retval = TRUE;
} G_STMT_END;
/* no need to validate the stack size.
 * if the result in all above is ok, everything would be fine then.
 */
/* VALIDATE_STACK_SIZE (0, 0, 0); */
DEFUNC_OPER_END

/* <mark> ... %dicttomark <dict> */
DEFUNC_OPER (protected_dicttomark)
gint __n G_GNUC_UNUSED = 0;
G_STMT_START {
	hg_quark_t qk, qv, qd, arg0;
	gsize i;
	hg_dict_t *dict;

	/* %dicttomark is the same to {counttomark dup 2 mod 0 ne {pop errordict /rangecheck get />> exch exec} if 2 idiv dup dict begin {def} repeat pop currentdict end} */
	if (!hg_operator_invoke(HG_QOPER (HG_enc_counttomark), vm, error))
		return FALSE;
	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0)) {
		hg_stack_drop(ostack, error);
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	__n = HG_INT (arg0);
	if ((__n % 2) != 0) {
		hg_stack_drop(ostack, error);
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	qd = hg_dict_new(hg_vm_get_mem(vm),
			 __n / 2,
			 hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1,
			 (gpointer *)&dict);
	if (qd == Qnil) {
		hg_stack_drop(ostack, error);
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_vm_quark_set_default_attributes(vm, &qd);
	for (i = __n; i > 0; i -= 2) {
		qk = hg_stack_index(ostack, i, error);
		qv = hg_stack_index(ostack, i - 1, error);
		hg_dict_add(dict, qk, qv, FALSE, error);
	}
	for (i = 0; i <= (__n + 1); i++)
		hg_stack_drop(ostack, error);

	HG_VM_UNLOCK (vm, qd);

	STACK_PUSH (ostack, qd);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-__n, 0, 0);
DEFUNC_OPER_END

/* <initial> <increment> <limit> <proc> %for_yield_int_continue - */
DEFUNC_OPER (protected_for_yield_int_continue)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t self, proc, limit, inc, qq;
	hg_quark_t *init = NULL;
	gint iinit, iinc, ilimit;

	self  = hg_stack_index(estack, 0, error);
	proc  = hg_stack_index(estack, 1, error);
	limit = hg_stack_index(estack, 2, error);
	inc   = hg_stack_index(estack, 3, error);
	init  = hg_stack_peek(estack, 4, error);

	iinit  = HG_INT (*init);
	iinc   = HG_INT (inc);
	ilimit = HG_INT (limit);

	if ((iinc > 0 && iinit > ilimit) ||
	    (iinc < 0 && iinit < ilimit)) {
		hg_stack_roll(estack, 5, 1, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		__flag = TRUE;
		retval = TRUE;

		break;
	}

	qq = hg_vm_quark_copy(vm, proc, NULL, error);
	if (qq == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	STACK_PUSH (ostack, *init);
	STACK_PUSH (estack, qq);
	STACK_PUSH (estack, self); /* dummy */

	*init = HG_QINT (iinit + iinc);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__flag ? 0 : 1, __flag ? -4 : 2, 0);
DEFUNC_OPER_END

/* <initial> <increment> <limit> <proc> %for_yield_real_continue - */
DEFUNC_OPER (protected_for_yield_real_continue)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t self, proc, limit, inc, qq;
	hg_quark_t *init = NULL;
	gdouble dinit, dinc, dlimit;

	self  = hg_stack_index(estack, 0, error);
	proc  = hg_stack_index(estack, 1, error);
	limit = hg_stack_index(estack, 2, error);
	inc   = hg_stack_index(estack, 3, error);
	init  = hg_stack_peek(estack, 4, error);

	dinit  = HG_REAL (*init);
	dinc   = HG_REAL (inc);
	dlimit = HG_REAL (limit);

	if ((dinc > 0.0 && dinit > dlimit) ||
	    (dinc < 0.0 && dinit < dlimit)) {
		hg_stack_roll(estack, 5, 1, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		__flag = TRUE;
		retval = TRUE;

		break;
	}

	qq = hg_vm_quark_copy(vm, proc, NULL, error);
	if (qq == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	STACK_PUSH (ostack, *init);
	STACK_PUSH (estack, qq);
	STACK_PUSH (estack, self); /* dummy */

	*init = HG_QREAL (dinit + dinc);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__flag ? 0 : 1, __flag ? -4 : 2, 0);
DEFUNC_OPER_END

/* <int> <array> <proc> %forall_array_continue - */
DEFUNC_OPER (protected_forall_array_continue)
gint __n G_GNUC_UNUSED = 0;
G_STMT_START {
	hg_quark_t self, proc, val, q, qq;
	hg_quark_t *n = NULL;
	hg_array_t *a;
	gsize i;

	self = hg_stack_index(estack, 0, error);
	proc = hg_stack_index(estack, 1, error);
	val = hg_stack_index(estack, 2, error);
	n = hg_stack_peek(estack, 3, error);

	a = HG_VM_LOCK (vm, val, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	i = HG_INT (*n);
	if (hg_array_maxlength(a) <= i) {
		HG_VM_UNLOCK (vm, val);
		hg_stack_roll(estack, 4, 1, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);

		retval = TRUE;
		__n = 3;
		break;
	}
	*n = HG_QINT (i + 1);

	qq = hg_array_get(a, i, error);
	HG_VM_UNLOCK (vm, val);

	q = hg_vm_quark_copy(vm, proc, NULL, error);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	STACK_PUSH (ostack, qq);
	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__n != 0 ? 0 : 1, __n != 0 ? -__n : 2, 0);
DEFUNC_OPER_END

/* <int> <dict> <proc> %forall_dict_continue - */
DEFUNC_OPER (protected_forall_dict_continue)
gint __n G_GNUC_UNUSED = 0;
G_STMT_START {
	hg_quark_t self, proc, val, q, qk, qv;
	hg_quark_t *n = NULL;
	hg_dict_t *d;
	gsize i;

	self = hg_stack_index(estack, 0, error);
	proc = hg_stack_index(estack, 1, error);
	val = hg_stack_index(estack, 2, error);
	n = hg_stack_peek(estack, 3, error);

	d = HG_VM_LOCK (vm, val, error);
	if (d == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	i = HG_INT (*n);
	if (hg_dict_length(d) == 0) {
		HG_VM_UNLOCK (vm, val);
		hg_stack_roll(estack, 4, 1, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);

		retval = TRUE;
		__n = 3;
		break;
	}
	*n = HG_QINT (i + 1);

	if (!hg_dict_first_item(d, &qk, &qv, error)) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		HG_VM_UNLOCK (vm, val);

		return FALSE;
	}
	hg_dict_remove(d, qk, error);
	HG_VM_UNLOCK (vm, val);

	q = hg_vm_quark_copy(vm, proc, NULL, error);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	STACK_PUSH (ostack, qk);
	STACK_PUSH (ostack, qv);
	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__n != 0 ? 0 : 2, __n != 0 ? -__n : 2, 0);
DEFUNC_OPER_END

/* <int> <string> <proc> %forall_string_continue - */
DEFUNC_OPER (protected_forall_string_continue)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t self, proc, val, q, qq;
	hg_quark_t *n = NULL;
	hg_string_t *s;
	gsize i;

	self = hg_stack_index(estack, 0, error);
	proc = hg_stack_index(estack, 1, error);
	val = hg_stack_index(estack, 2, error);
	n = hg_stack_peek(estack, 3, error);

	s = HG_VM_LOCK (vm, val, error);
	if (s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	i = HG_INT (*n);
	if (hg_string_length(s) <= i) {
		HG_VM_UNLOCK (vm, val);
		hg_stack_roll(estack, 4, 1, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);

		__flag = retval = TRUE;
		break;
	}
	*n = HG_QINT (i + 1);

	qq = HG_QINT (hg_string_index(s, i));
	HG_VM_UNLOCK (vm, val);

	q = hg_vm_quark_copy(vm, proc, NULL, error);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	STACK_PUSH (ostack, qq);
	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__flag ? 0 : 1, __flag ? -3 : 2, 0);
DEFUNC_OPER_END

/* %loop_continue */
DEFUNC_OPER (protected_loop_continue)
G_STMT_START {
	hg_quark_t self, qproc, q;

	self = hg_stack_index(estack, 0, error);
	qproc = hg_stack_index(estack, 1, error);

	q = hg_vm_quark_copy(vm, qproc, NULL, error);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	STACK_PUSH (estack, q);
	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 2, 0);
DEFUNC_OPER_END

/* <n> <proc> %repeat_continue - */
DEFUNC_OPER (protected_repeat_continue)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t *arg0 = NULL;
	hg_quark_t arg1, self, q;

	arg0 = hg_stack_peek(estack, 2, error);
	arg1 = hg_stack_index(estack, 1, error);
	self = hg_stack_index(estack, 0, error);

	if (HG_INT (*arg0) > 0) {
		*arg0 = HG_QINT (HG_INT (*arg0) - 1);

		q = hg_vm_quark_copy(vm, arg1, NULL, error);
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		STACK_PUSH (estack, q);
		__flag = TRUE;
	} else {
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
		hg_stack_drop(estack, error);
	}

	/* dummy */
	STACK_PUSH (estack, self);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, __flag ? 2 : -2, 0);
DEFUNC_OPER_END

/* %stopped_continue */
DEFUNC_OPER (protected_stopped_continue)
G_STMT_START {
	hg_dict_t *dict;
	hg_quark_t q, qn;
	gboolean ret = FALSE;

	dict = HG_VM_LOCK (vm, vm->qerror, error);
	if (dict == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	qn = HG_QNAME (vm->name, ".stopped");
	q = hg_dict_lookup(dict, qn, error);

	if (q != Qnil &&
	    HG_IS_QBOOL (q) &&
	    HG_BOOL (q)) {
		hg_dict_add(dict, qn, HG_QBOOL (FALSE), FALSE, error);
		hg_vm_clear_error(vm);
		ret = TRUE;
	}

	HG_VM_UNLOCK (vm, vm->qerror);
	STACK_PUSH (ostack, HG_QBOOL (ret));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

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
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);
	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		*ret = HG_QINT (abs(HG_INT (arg0)));
	} else if (HG_IS_QREAL (arg0)) {
		*ret = HG_QREAL (fabs(HG_REAL (arg0)));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <num1> <num2> add <sum> */
DEFUNC_OPER (add)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;
	gdouble d1, d2, dr;
	gboolean is_int = TRUE;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
		is_int = FALSE;
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	dr = d1 + d2;
	if (is_int &&
	    HG_REAL_LE (dr, G_MAXINT32) &&
	    HG_REAL_GE (dr, G_MININT32)) {
		*ret = HG_QINT ((gint32)dr);
	} else {
		*ret = HG_QREAL (dr);
	}
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <array> aload <any> ... <array> */
DEFUNC_OPER (aload)
gssize __n G_GNUC_UNUSED = 0;
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
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_stack_pop(ostack, error);
	__n = len = hg_array_maxlength(a);
	for (i = 0; i < len; i++) {
		q = hg_array_get(a, i, error);
		if (error && *error) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto error;
		}
		STACK_PUSH (ostack, q);
	}
	STACK_PUSH (ostack, arg0);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (__n, 0, 0);
DEFUNC_OPER_END

/* <bool1> <bool2> and <bool3>
 * <int1> <int2> and <int3>
 */
DEFUNC_OPER (and)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QBOOL (arg0) &&
	    HG_IS_QBOOL (arg1)) {
		*ret = HG_QBOOL (HG_BOOL (arg0) & HG_BOOL (arg1));
	} else if (HG_IS_QINT (arg0) &&
		   HG_IS_QINT (arg1)) {
		*ret = HG_QINT (HG_INT (arg0) & HG_INT (arg1));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <x> <y> <r> <angle1> <angle2> arc - */
DEFUNC_OPER (arc)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, arg3, arg4, q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_path_t *path;
	gdouble dx, dy, dr, dangle1, dangle2, dc = 2.0 * M_PI / 360;

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4, error);
	arg1 = hg_stack_index(ostack, 3, error);
	arg2 = hg_stack_index(ostack, 2, error);
	arg3 = hg_stack_index(ostack, 1, error);
	arg4 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		dr = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dr = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg3)) {
		dangle1 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dangle1 = HG_REAL (arg3);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg4)) {
		dangle2 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dangle2 = HG_REAL (arg4);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	if (q == Qnil) {
		path = NULL;
		q = hg_path_new(hg_vm_get_mem(vm), (gpointer *)&path);
		if (q != Qnil)
			hg_gstate_set_path(gstate, q);
	} else {
		path = HG_VM_LOCK (vm, q, error);
	}
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	retval = hg_path_arc(path, dx, dy, dr,
			     dangle1 * dc,
			     dangle2 * dc);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-5, 0, 0);
DEFUNC_OPER_END

/* <x> <y> <r> <angle1> <angle2> arcn - */
DEFUNC_OPER (arcn)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, arg3, arg4, q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_path_t *path;
	gdouble dx, dy, dr, dangle1, dangle2, dc = 2.0 * M_PI / 360;

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4, error);
	arg1 = hg_stack_index(ostack, 3, error);
	arg2 = hg_stack_index(ostack, 2, error);
	arg3 = hg_stack_index(ostack, 1, error);
	arg4 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		dr = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dr = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg3)) {
		dangle1 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dangle1 = HG_REAL (arg3);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg4)) {
		dangle2 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dangle2 = HG_REAL (arg4);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	if (q == Qnil) {
		path = NULL;
		q = hg_path_new(hg_vm_get_mem(vm), (gpointer *)&path);
		if (q != Qnil)
			hg_gstate_set_path(gstate, q);
	} else {
		path = HG_VM_LOCK (vm, q, error);
	}
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	retval = hg_path_arcn(path, dx, dy, dr,
			      dangle1 * dc,
			      dangle2 * dc);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-5, 0, 0);
DEFUNC_OPER_END

/* <x1> <y1> <x2> <y2> <r> arct - */
DEFUNC_OPER (arct)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, arg3, arg4, q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_path_t *path;
	gdouble dx1, dy1, dx2, dy2, dr;

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4, error);
	arg1 = hg_stack_index(ostack, 3, error);
	arg2 = hg_stack_index(ostack, 2, error);
	arg3 = hg_stack_index(ostack, 1, error);
	arg4 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg4)) {
		dr = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dr = HG_REAL (arg4);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_get_current_point(path, NULL, NULL)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto error2;
	}
	retval = hg_path_arcto(path, dx1, dy1, dx2, dy2, dr, NULL);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-5, 0, 0);
DEFUNC_OPER_END

/* <x1> <y1> <x2> <y2> <r> arcto <xt1> <yt1> <xt2> <yt2> */
DEFUNC_OPER (arcto)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, arg3, arg4, q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_path_t *path;
	gdouble dx1, dy1, dx2, dy2, dr, dt[4];

	CHECK_STACK (ostack, 5);

	arg0 = hg_stack_index(ostack, 4, error);
	arg1 = hg_stack_index(ostack, 3, error);
	arg2 = hg_stack_index(ostack, 2, error);
	arg3 = hg_stack_index(ostack, 1, error);
	arg4 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg4)) {
		dr = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dr = HG_REAL (arg4);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_get_current_point(path, NULL, NULL)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto error2;
	}
	retval = hg_path_arcto(path, dx1, dy1, dx2, dy2, dr, dt);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, HG_QREAL (dt[0]));
	STACK_PUSH (ostack, HG_QREAL (dt[1]));
	STACK_PUSH (ostack, HG_QREAL (dt[2]));
	STACK_PUSH (ostack, HG_QREAL (dt[3]));
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <int> array <array> */
DEFUNC_OPER (array)
G_STMT_START {
	hg_quark_t arg0, q, *ret;
	hg_mem_t *m;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_INT (arg0) < 0 ||
	    HG_INT (arg0) > 65535) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	m = hg_vm_get_mem(vm);
	q = hg_array_new(m, HG_INT (arg0), NULL);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	*ret = q;
	hg_mem_reserved_spool_remove(m, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ashow);

/* <any> ... <array> astore <array> */
DEFUNC_OPER (astore)
gssize __n G_GNUC_UNUSED = 0;
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
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	len = hg_array_maxlength(a);
	__n = len;
	if (hg_stack_depth(ostack) < (len + 1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_stackunderflow);
		goto error;
	}
	for (i = 0; i < len; i++) {
		q = hg_stack_index(ostack, len - i, error);
		if (!hg_array_set(a, q, i, FALSE, error))
			goto error;
	}
	for (i = 0; i <= len; i++) {
		if (i == 0)
			hg_stack_pop(ostack, error);
		else
			hg_stack_drop(ostack, error);
	}

	STACK_PUSH (ostack, arg0);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (-__n, 0, 0);
DEFUNC_OPER_END

/* <num> <den> atan <angle> */
DEFUNC_OPER (atan)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;
	double num, den, angle;

	CHECK_STACK (ostack, 2);
	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0)) {
		num = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		num = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		den = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		den = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_REAL_IS_ZERO (num) && HG_REAL_IS_ZERO (den)) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefinedresult);
		return FALSE;
	}
	angle = atan2(num, den) / (2 * M_PI / 360);
	if (angle < 0)
		angle = 360.0 + angle;
	*ret = HG_QREAL (angle);

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (awidthshow);
DEFUNC_UNIMPLEMENTED_OPER (banddevice);

/* <dict> begin - */
DEFUNC_OPER (begin)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QDICT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	hg_stack_pop(ostack, error);
	STACK_PUSH (dstack, arg0);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 1);
DEFUNC_OPER_END

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
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		/* ignore it */
		retval = TRUE;
		break;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	len = hg_array_maxlength(a);
	for (i = 0; i < len; i++) {
		q = hg_array_get(a, i, error);
		if (hg_vm_quark_is_executable(vm, &q)) {
			if (HG_IS_QARRAY (q)) {
				STACK_PUSH (ostack, q);
				_hg_operator_real_bind(vm, error);
				hg_vm_quark_set_writable(vm, &q, FALSE);
				if (!hg_array_set(a, q, i, TRUE, error))
					goto error;
				hg_stack_drop(ostack, error);
			} else if (HG_IS_QNAME (q)) {
				hg_quark_t qop = hg_vm_dstack_lookup(vm, q);

				if (HG_IS_QOPER (qop)) {
					if (!hg_array_set(a, qop, i, TRUE, error))
						goto error;
				}
			}
		}
	}
	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <int1> <shiftt> bitshift <int2> */
DEFUNC_OPER (bitshift)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_INT (arg1) < 0) {
		*ret = HG_QINT (HG_INT (arg0) >> -HG_INT (arg1));
	} else {
		*ret = HG_QINT (HG_INT (arg0) << HG_INT (arg1));
	}
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <file> bytesavailable <int> */
DEFUNC_OPER (bytesavailable)
G_STMT_START {
	hg_quark_t arg0, *ret;
	hg_file_t *f;
	gssize cur_pos;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);

	cur_pos = hg_file_seek(f, 0, HG_FILE_POS_CURRENT, error);
	*ret = HG_QINT (hg_file_seek(f, 0, HG_FILE_POS_END, error));
	hg_file_seek(f, cur_pos, HG_FILE_POS_BEGIN, error);

	HG_VM_UNLOCK (vm, arg0);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (cachestatus);

/* <num1> ceiling <num2> */
DEFUNC_OPER (ceiling)
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		/* nothing to do */
	} else if (HG_IS_QREAL (arg0)) {
		*ret = HG_QREAL (ceil(HG_REAL (arg0)));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (charpath);

/* - clear - */
DEFUNC_OPER (clear)
G_STMT_START {
	hg_stack_clear(ostack);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-__hg_stack_odepth, 0, 0);
DEFUNC_OPER_END

/* - cleardictstack - */
DEFUNC_OPER (cleardictstack)
gssize __n G_GNUC_UNUSED = 0;
G_STMT_START {
	gsize ddepth = hg_stack_depth(dstack);
	gint i;

	if (hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1) {
		__n = ddepth - 2;
	} else {
		__n = ddepth - 3;
	}
	for (i = 0; i < __n; i++) {
		hg_stack_drop(dstack, error);
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, -__n);
DEFUNC_OPER_END

/* <mark> ... cleartomark - */
DEFUNC_OPER (cleartomark)
gssize __n G_GNUC_UNUSED = 0;
G_STMT_START {
	gsize depth = hg_stack_depth(ostack), i, j;
	hg_quark_t q;

	for (i = 0; i < depth; i++) {
		q = hg_stack_index(ostack, i, error);
		if (HG_IS_QMARK (q)) {
			for (j = 0; j <= i; j++) {
				hg_stack_drop(ostack, error);
			}
			retval = TRUE;
			__n = i + 1;
			break;
		}
	}
	if (i == depth)
		hg_vm_set_error(vm, qself, HG_VM_e_unmatchedmark);
} G_STMT_END;
VALIDATE_STACK_SIZE (-__n, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (clip);

/* - clippath - */
DEFUNC_OPER (clippath)
G_STMT_START {
	hg_quark_t q, qq, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg, error);

	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_clippath(gstate);
	qq = hg_vm_quark_copy(vm, q, NULL, error);
	if (qq == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}
	hg_gstate_set_path(gstate, qq);
	retval = TRUE;
  finalize:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <file> closefile - */
DEFUNC_OPER (closefile)
G_STMT_START {
	hg_quark_t arg0;
	hg_file_t *f;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);
	if (f == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_file_close(f, error);
	if (error && *error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		HG_VM_UNLOCK (vm, arg0);

		return FALSE;
	}
	HG_VM_UNLOCK (vm, arg0);

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* - closepath - */
DEFUNC_OPER (closepath)
G_STMT_START {
	hg_quark_t q = Qnil, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_path_t *path;

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_close(path)) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error;
	}
	retval = TRUE;
  error:
	if (q != Qnil)
		HG_VM_UNLOCK (vm, q);
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (colorimage);

/* <matrix> concat - */
DEFUNC_OPER (concat)
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_array_t *a = NULL;
	hg_gstate_t *gstate = NULL;
	hg_matrix_t m1, m2;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (!a)
		return FALSE;
	if (!hg_array_is_matrix(a)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}
	hg_array_to_matrix(a, &m1);
	gstate = HG_VM_LOCK (vm, qg, error);
	if (!gstate)
		goto finalize;
	hg_gstate_get_ctm(gstate, &m2);

	hg_matrix_multiply(&m1, &m2, &m1);

	hg_gstate_set_ctm(gstate, &m1);

	hg_stack_drop(ostack, error);

	retval = TRUE;
  finalize:
	if (gstate)
		HG_VM_UNLOCK (vm, qg);
	if (a)
		HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <matrix1> <matrix2> <matrix3> concatmatrix <matrix3> */
DEFUNC_OPER (concatmatrix)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2;
	hg_array_t *a1, *a2, *a3;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0) ||
	    !HG_IS_QARRAY (arg1) ||
	    !HG_IS_QARRAY (arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1) ||
	    !hg_vm_quark_is_writable(vm, &arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	a1 = HG_VM_LOCK (vm, arg0, error);
	a2 = HG_VM_LOCK (vm, arg1, error);
	a3 = HG_VM_LOCK (vm, arg2, error);

	if (hg_array_maxlength(a1) != 6 ||
	    hg_array_maxlength(a2) != 6 ||
	    hg_array_maxlength(a3) != 6) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto error;
	}
	if (!hg_array_is_matrix(a1) ||
	    !hg_array_is_matrix(a2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		goto error;
	}
	if (!hg_array_matrix_multiply(a1, a2, a3)) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	hg_stack_roll(ostack, 3, 1, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg2);
	HG_VM_UNLOCK (vm, arg1);
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (condition);

static gboolean
_hg_operator_copy_real_traverse_dict(hg_mem_t    *mem,
				     hg_quark_t   qkey,
				     hg_quark_t   qval,
				     gpointer     data,
				     GError     **error)
{
	hg_dict_t *d2 = (hg_dict_t *)data;

	return hg_dict_add(d2, qkey, qval, TRUE, error);
}

/* <any> ... <n> copy <any> ...
 * <array1> <array2> copy <subarray2>
 * <dict1> <dict2> copy <dict2>
 * <string1> <string2> copy <substring2>
 * <packedarray> <array> copy <subarray2>
 * <gstate1> <gstate2> copy <gstate2>
 */
DEFUNC_OPER (copy)
gssize __n G_GNUC_UNUSED = 0;
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
		hg_stack_drop(ostack, error);
		for (i = 0; i < n; i++) {
			if (!hg_stack_push(ostack, hg_stack_index(ostack, n - 1, error))) {
				hg_vm_set_error(vm, qself, HG_VM_e_stackoverflow);
				return FALSE;
			}
		}
		retval = TRUE;
		__n = n;
	} else {
		CHECK_STACK (ostack, 2);

		arg1 = arg0;
		arg0 = hg_stack_index(ostack, 1, error);
		if (HG_IS_QARRAY (arg0) &&
		    HG_IS_QARRAY (arg1)) {
			hg_array_t *a1, *a2;
			gsize len1, len2;

			if (!hg_vm_quark_is_readable(vm, &arg0) ||
			    !hg_vm_quark_is_writable(vm, &arg1)) {
				hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
				return FALSE;
			}

			a1 = HG_VM_LOCK (vm, arg0, error);
			a2 = HG_VM_LOCK (vm, arg1, error);
			if (a1 == NULL || a2 == NULL) {
				hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
				goto a_error;
			}
			len1 = hg_array_maxlength(a1);
			len2 = hg_array_maxlength(a2);
			if (len1 > len2) {
				hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
				goto a_error;
			}
			for (i = 0; i < len1; i++) {
				hg_quark_t qq;

				qq = hg_array_get(a1, i, error);
				if (!hg_array_set(a2, qq, i, TRUE, error))
					goto a_error;
			}
			if (len2 > len1) {
				q = hg_array_make_subarray(a2, 0, len1 - 1, NULL, error);
				if (q == Qnil) {
					hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
					goto a_error;
				}
				hg_vm_quark_set_attributes(vm, &q,
							   hg_vm_quark_is_readable(vm, &arg1),
							   hg_vm_quark_is_writable(vm, &arg1),
							   hg_vm_quark_is_executable(vm, &arg1),
							   hg_vm_quark_is_editable(vm, &arg1));
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
				hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
				return FALSE;
			}

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
			gsize len1, mlen1, mlen2;

			if (!hg_vm_quark_is_readable(vm, &arg0) ||
			    !hg_vm_quark_is_writable(vm, &arg1)) {
				hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
				return FALSE;
			}

			s1 = HG_VM_LOCK (vm, arg0, error);
			s2 = HG_VM_LOCK (vm, arg1, error);
			if (s1 == NULL || s2 == NULL) {
				hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
				goto s_error;
			}
			len1 = hg_string_length(s1);
			mlen1 = hg_string_maxlength(s1);
			mlen2 = hg_string_maxlength(s2);
			if (mlen1 > mlen2) {
				hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
				goto s_error;
			}
			for (i = 0; i < len1; i++) {
				gchar c = hg_string_index(s1, i);

				hg_string_overwrite_c(s2, c, i, error);
			}
			if (mlen2 > mlen1) {
				q = hg_string_make_substring(s2, 0, len1 - 1, NULL, error);
				if (q == Qnil) {
					hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
					goto s_error;
				}
				hg_vm_quark_set_attributes(vm, &q,
							   hg_vm_quark_is_readable(vm, &arg1),
							   hg_vm_quark_is_writable(vm, &arg1),
							   hg_vm_quark_is_executable(vm, &arg1),
							   hg_vm_quark_is_editable(vm, &arg1));
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
			hg_stack_drop(ostack, error);
			hg_stack_drop(ostack, error);

			STACK_PUSH (ostack, q);
		}
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (__n != 0 ? __n - 1 : -1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (copypage);

/* <angle> cos <real> */
DEFUNC_OPER (cos)
G_STMT_START {
	hg_quark_t arg0, *ret;
	gdouble angle;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		angle = (gdouble)HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		angle = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	*ret = HG_QREAL (cos(angle / 180 * M_PI));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* - count <int> */
DEFUNC_OPER (count)
G_STMT_START {
	STACK_PUSH (ostack, HG_QINT (hg_stack_depth(ostack)));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* - countdictstack <int> */
DEFUNC_OPER (countdictstack)
G_STMT_START {
	STACK_PUSH (ostack, HG_QINT (hg_stack_depth(dstack)));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* - countexecstack <int> */
DEFUNC_OPER (countexecstack)
G_STMT_START {
	STACK_PUSH (ostack, HG_QINT (hg_stack_depth(estack)));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* <mark> ... counttomark <int> */
DEFUNC_OPER (counttomark)
G_STMT_START {
	gsize i, depth = hg_stack_depth(ostack);
	hg_quark_t q = Qnil;

	for (i = 0; i < depth; i++) {
		hg_quark_t qq = hg_stack_index(ostack, i, error);

		if (HG_IS_QMARK (qq)) {
			q = HG_QINT (i);
			break;
		}
	}
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_unmatchedmark);
		return FALSE;
	}

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

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
G_STMT_START {
	hg_quark_t q;

	q = hg_stack_index(dstack, 0, error);
	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* - currentfile <file> */
DEFUNC_OPER (currentfile)
G_STMT_START {
	hg_quark_t q = Qnil;
	gsize i, edepth = hg_stack_depth(estack);

	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i, error);
		if (HG_IS_QFILE (q))
			break;
		q = Qnil;
	}
	if (q == Qnil) {
		hg_file_t *f;

		q = hg_file_new_with_string(hg_vm_get_mem(vm), "%nofile",
					    HG_FILE_IO_MODE_READ,
					    NULL, NULL,
					    error,
					    (gpointer *)&f);
		hg_file_close(f, error);
		HG_VM_UNLOCK (vm, q);
	}
	STACK_PUSH (ostack, q);
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

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
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_array_t *a = NULL;
	hg_matrix_t m;
	hg_gstate_t *gstate = NULL;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (!a)
		return FALSE;

	if (hg_array_maxlength(a) != 6) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
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
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (currentmiterlimit);
DEFUNC_UNIMPLEMENTED_OPER (currentobjectformat);
DEFUNC_UNIMPLEMENTED_OPER (currentoverprint);
DEFUNC_UNIMPLEMENTED_OPER (currentpacking);
DEFUNC_UNIMPLEMENTED_OPER (currentpagedevice);

/* - currentpoint <x> <y> */
DEFUNC_OPER (currentpoint)
G_STMT_START {
	hg_quark_t qg = hg_vm_get_gstate(vm), qpath;
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg, error);
	hg_path_t *path = NULL;
	gdouble x, y;

	if (!gstate)
		return FALSE;
	qpath = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, qpath, error);
	if (!path)
		goto finalize;
	if (!hg_path_get_current_point(path, &x, &y)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto finalize;
	}

	STACK_PUSH (ostack, HG_QREAL (x));
	STACK_PUSH (ostack, HG_QREAL (y));

	retval = TRUE;
  finalize:
	if (path)
		HG_VM_UNLOCK (vm, qpath);
	if (gstate)
		HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (2, 0, 0);
DEFUNC_OPER_END

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
G_STMT_START {
	hg_quark_t q;

	q = hg_vm_get_user_params(vm, NULL, error);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* <x1> <y1> <x2> <y2> <x3> <y3> curveto - */
DEFUNC_OPER (curveto)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, arg3, arg4, arg5, q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_path_t *path;
	gdouble dx1, dy1, dx2, dy2, dx3, dy3;

	CHECK_STACK (ostack, 6);

	arg0 = hg_stack_index(ostack, 5, error);
	arg1 = hg_stack_index(ostack, 4, error);
	arg2 = hg_stack_index(ostack, 3, error);
	arg3 = hg_stack_index(ostack, 2, error);
	arg4 = hg_stack_index(ostack, 1, error);
	arg5 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg4)) {
		dx3 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dx3 = HG_REAL (arg4);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg5)) {
		dy3 = HG_INT (arg5);
	} else if (HG_IS_QREAL (arg5)) {
		dy3 = HG_REAL (arg5);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_get_current_point(path, NULL, NULL)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto error2;
	}
	retval = hg_path_curveto(path, dx1, dy1, dx2, dy2, dx3, dy3);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-6, 0, 0);
DEFUNC_OPER_END

/* <num> cvi <int>
 * <string> cvi <int>
 */
DEFUNC_OPER (cvi)
G_STMT_START {
	hg_quark_t arg0, q, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		/* nothing to do */
		retval = TRUE;
	} else if (HG_IS_QREAL (arg0)) {
		gdouble d = HG_REAL (arg0);

		if (d > G_MAXINT32 || d < G_MININT32) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			return FALSE;
		}
		if (isinf(d)) {
			hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
			return FALSE;
		}
		*ret = HG_QINT (d);

		retval = TRUE;
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;

		if (!hg_vm_quark_is_readable(vm, &arg0)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}
		s = HG_VM_LOCK (vm, arg0, error);
		q = hg_file_new_with_string(hg_vm_get_mem(vm),
					    "--%cvi--",
					    HG_FILE_IO_MODE_READ,
					    s,
					    NULL,
					    error,
					    NULL);
		HG_VM_UNLOCK (vm, arg0);
		STACK_PUSH (ostack, q);
		retval = _hg_operator_real_token(vm, error);
		if (retval == TRUE) {
			hg_quark_t qq, qv;

			qq = hg_stack_pop(ostack, error);
			if (!HG_IS_QBOOL (qq)) {
				hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
				return FALSE;
			}
			if (!HG_BOOL (qq)) {
				hg_vm_set_error(vm, qself, HG_VM_e_syntaxerror);
				return FALSE;
			}
			qv = hg_stack_pop(ostack, error);
			if (HG_IS_QINT (qv)) {
				*ret = qv;
			} else if (HG_IS_QREAL (qv)) {
				gdouble d = HG_REAL (qv);

				if (d < G_MININT32 ||
				    d > G_MAXINT32) {
					hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
					return FALSE;
				}
				*ret = HG_QINT ((gint32)d);
			} else {
				hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
				return FALSE;
			}

			retval = TRUE;
		} else {
			hg_stack_drop(ostack, error);

			STACK_PUSH (ostack, qself);
		}
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <any> cvlit <any> */
DEFUNC_OPER (cvlit)
G_STMT_START {
	hg_quark_t *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	hg_vm_quark_set_executable(vm, ret, FALSE);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <string> cvn <name> */
DEFUNC_OPER (cvn)
G_STMT_START {
	hg_quark_t arg0, *ret;
	gchar *cstr;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	cstr = hg_vm_string_get_cstr(vm, arg0, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}
	if (hg_vm_string_length(vm, arg0, error) > 127) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		g_free(cstr);
		return FALSE;
	}
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}

	*ret = HG_QNAME (vm->name, cstr);
	hg_vm_quark_set_executable(vm, ret,
				   hg_vm_quark_is_executable(vm, &arg0));
	g_free(cstr);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <num> cvr <real>
 * <string> cvr <real>
 */
DEFUNC_OPER (cvr)
G_STMT_START {
	hg_quark_t arg0, q, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
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
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}
		s = HG_VM_LOCK (vm, arg0, error);
		q = hg_file_new_with_string(hg_vm_get_mem(vm),
					    "--%cvr--",
					    HG_FILE_IO_MODE_READ,
					    s,
					    NULL,
					    error,
					    NULL);
		HG_VM_UNLOCK (vm, arg0);
		STACK_PUSH (ostack, q);
		retval = _hg_operator_real_token(vm, error);
		if (retval == TRUE) {
			hg_quark_t qq, qv;

			qq = hg_stack_pop(ostack, error);
			if (!HG_IS_QBOOL (qq)) {
				hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
				return FALSE;
			}
			if (!HG_BOOL (qq)) {
				hg_vm_set_error(vm, qself, HG_VM_e_syntaxerror);
				return FALSE;
			}
			qv = hg_stack_pop(ostack, error);
			if (HG_IS_QINT (qv)) {
				qv = HG_QREAL (HG_INT (qv));
			} else if (HG_IS_QREAL (qv)) {
				if (isinf(HG_REAL (qv))) {
					hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
					return FALSE;
				}
			} else {
				hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
				return FALSE;
			}
			*ret = qv;

			retval = TRUE;
		} else {
			hg_stack_drop(ostack, error);

			STACK_PUSH (ostack, qself);
		}
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <num> <radix> <string> cvrs <substring> */
DEFUNC_OPER (cvrs)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, q;
	gdouble d1;
	gint radix;
	hg_string_t *s;
	gboolean is_real = FALSE;
	gchar *cstr = NULL;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_real = TRUE;
	} else if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!HG_IS_QINT (arg1) ||
	    !HG_IS_QSTRING (arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	radix = HG_INT (arg1);
	if (radix < 2 || radix > 36) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_writable(vm, &arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (radix == 10) {
		if (is_real) {
			cstr = g_strdup_printf("%f", d1);
		} else {
			cstr = g_strdup_printf("%d", (gint)d1);
		}
	} else {
		const gchar __radix_to_c[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		GString *str = g_string_new(NULL), *rstr;
		guint x = (guint)d1;
		gsize i;

		do {
			g_string_append_c(str, __radix_to_c[x % radix]);
			x /= radix;
		} while (x != 0);

		rstr = g_string_new(NULL);
		for (i = 0; i < str->len; i++) {
			g_string_append_c(rstr, str->str[str->len - i - 1]);
		}
		g_string_free(str, TRUE);
		cstr = g_string_free(rstr, FALSE);
	}
	s = HG_VM_LOCK (vm, arg2, error);
	if (s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (strlen(cstr) > hg_string_maxlength(s)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto error;
	}
	hg_string_append(s, cstr, -1, error);
	if (hg_string_maxlength(s) > hg_string_length(s)) {
		q = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL, error);
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto error;
		}
		hg_vm_quark_set_attributes(vm, &q,
					   hg_vm_quark_is_readable(vm, &arg2),
					   hg_vm_quark_is_writable(vm, &arg2),
					   hg_vm_quark_is_executable(vm, &arg2),
					   hg_vm_quark_is_editable(vm, &arg2));
	} else {
		q = arg2;
	}
	if (q == arg2)
		hg_stack_pop(ostack, error);
	else
		hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	if (s)
		HG_VM_UNLOCK (vm, arg2);
	g_free(cstr);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <any> cvx <any> */
DEFUNC_OPER (cvx)
G_STMT_START {
	hg_quark_t *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	hg_vm_quark_set_executable(vm, ret, TRUE);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <key> <value> def - */
DEFUNC_OPER (def)
G_STMT_START {
	hg_quark_t arg0, arg1, qd;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	qd = hg_stack_index(dstack, 0, error);

	retval = hg_vm_dict_add(vm, qd, arg0, arg1, FALSE, error);
	if (!retval) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <matrix> defaultmatrix <matrix> */
DEFUNC_OPER (defaultmatrix)
G_STMT_START {
	hg_quark_t arg0;
	hg_array_t *a;
	hg_matrix_t m;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (hg_array_maxlength(a) != 6) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}
	if (!hg_device_get_ctm(vm->device, &m) ||
	    !hg_array_from_matrix(a, &m)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}

	retval = TRUE;
  finalize:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (defineresource);
DEFUNC_UNIMPLEMENTED_OPER (defineusername);
DEFUNC_UNIMPLEMENTED_OPER (defineuserobject);
DEFUNC_UNIMPLEMENTED_OPER (deletefile);
DEFUNC_UNIMPLEMENTED_OPER (detach);
DEFUNC_UNIMPLEMENTED_OPER (deviceinfo);

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
	if (HG_INT (arg0) < 0) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	if (HG_INT (arg0) > G_MAXUSHORT) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		return FALSE;
	}
	ret = hg_dict_new(hg_vm_get_mem(vm),
			  HG_INT (arg0),
			  hg_vm_get_language_level(vm) == HG_LANG_LEVEL_1,
			  NULL);
	if (ret == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_vm_quark_set_default_attributes(vm, &ret);
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, ret);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <array> dictstack <subarray> */
DEFUNC_OPER (dictstack)
G_STMT_START {
	hg_quark_t arg0, q;
	hg_array_t *a;
	gssize i, len, ddepth;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	ddepth = hg_stack_depth(dstack);
	len = hg_array_maxlength(a);
	if (ddepth > len) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto error;
	}
	for (i = 0; i < ddepth; i++) {
		q = hg_stack_index(dstack, ddepth - i - 1, error);
		if (!hg_array_set(a, q, i, FALSE, error))
			goto error;
	}
	if (ddepth != len) {
		q = hg_array_make_subarray(a, 0, ddepth - 1, NULL, error);
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto error;
		}
		hg_vm_quark_set_attributes(vm, &q,
					   hg_vm_quark_is_readable(vm, &arg0),
					   hg_vm_quark_is_writable(vm, &arg0),
					   hg_vm_quark_is_executable(vm, &arg0),
					   hg_vm_quark_is_editable(vm, &arg0));
	} else {
		q = arg0;
	}
	if (q == arg0)
		hg_stack_pop(ostack, error);
	else
		hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <num1> <num2> div <quotient> */
DEFUNC_OPER (div)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;
	gdouble d1, d2;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_REAL_IS_ZERO (d2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefinedresult);
		return FALSE;
	}
	*ret = HG_QREAL (d1 / d2);

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (dtransform);

/* <any> dup <any> <any> */
DEFUNC_OPER (dup)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	STACK_PUSH (ostack, arg0);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (echo);
DEFUNC_UNIMPLEMENTED_OPER (eexec);

/* - end - */
DEFUNC_OPER (end)
G_STMT_START {
	gsize ddepth = hg_stack_depth(dstack);
	hg_vm_langlevel_t lang = hg_vm_get_language_level(vm);

	if ((lang >= HG_LANG_LEVEL_2 && ddepth <= 3) ||
	    (lang == HG_LANG_LEVEL_1 && ddepth <= 2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_dictstackunderflow);
		return FALSE;
	}
	hg_stack_drop(dstack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, -1);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (eoclip);

/* - eofill - */
DEFUNC_OPER (eofill)
G_STMT_START {
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg, error);

	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (!hg_device_eofill(vm->device, gstate, error)) {
		if (error && *error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
		} else {
			hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		}
		goto finalize;
	}
	retval = TRUE;
  finalize:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (eoviewclip);

/* <any1> <any2> eq <bool> */
DEFUNC_OPER (eq)
G_STMT_START {
	hg_quark_t arg0, arg1, *qret;
	gboolean ret = FALSE;

	CHECK_STACK (ostack, 2);

	qret = hg_stack_peek(ostack, 1, error);
	arg0 = *qret;
	arg1 = hg_stack_index(ostack, 0, error);

	if ((HG_IS_QSTRING (arg0) &&
	     !hg_vm_quark_is_readable(vm, &arg0)) ||
	    (HG_IS_QSTRING (arg1) &&
	     !hg_vm_quark_is_readable(vm, &arg1))) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	ret = hg_vm_quark_compare(vm, arg0, arg1);
	if (!ret) {
		if ((HG_IS_QNAME (arg0) ||
		     HG_IS_QEVALNAME (arg0) ||
		     HG_IS_QSTRING (arg0)) &&
		    (HG_IS_QNAME (arg1) ||
		     HG_IS_QEVALNAME (arg1) ||
		     HG_IS_QSTRING (arg1))) {
			gchar *s1 = NULL, *s2 = NULL;

			if (HG_IS_QNAME (arg0) ||
			    HG_IS_QEVALNAME (arg0)) {
				s1 = g_strdup(HG_NAME (vm->name, arg0));
			} else {
				s1 = hg_vm_string_get_cstr(vm, arg0, error);
				if (*error) {
					hg_vm_set_error_from_gerror(vm, qself, *error);
					goto s_error;
				}
			}
			if (HG_IS_QNAME (arg1) ||
			    HG_IS_QEVALNAME (arg1)) {
				s2 = g_strdup(HG_NAME (vm->name, arg1));
			} else {
				s2 = hg_vm_string_get_cstr(vm, arg1, error);
				if (*error) {
					hg_vm_set_error_from_gerror(vm, qself, *error);
					goto s_error;
				}
			}
			if (s1 != NULL && s2 != NULL)
				ret = (strcmp(s1, s2) == 0);
		  s_error:
			g_free(s1);
			g_free(s2);
		} else if ((HG_IS_QINT (arg0) ||
			    HG_IS_QREAL (arg0)) &&
			   (HG_IS_QINT (arg1) ||
			    HG_IS_QREAL (arg1))) {
			gdouble d1, d2;

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
	if (!*error) {
		*qret = HG_QBOOL (ret);
		hg_stack_drop(ostack, error);

		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (erasepage);

/* <any1> <any2> exch <any2> <any1> */
DEFUNC_OPER (exch)
G_STMT_START {
	CHECK_STACK (ostack, 2);

	hg_stack_exch(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <any> exec - */
DEFUNC_OPER (exec)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (hg_vm_quark_is_executable(vm, &arg0)) {
		if (!HG_IS_QOPER (arg0) &&
		    !hg_vm_quark_is_readable(vm, &arg0)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}

		q = hg_vm_quark_copy(vm, arg0, NULL, error);
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		hg_stack_drop(ostack, error);

		STACK_PUSH (estack, q);

		hg_stack_exch(estack, error);
		__flag = TRUE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__flag ? -1 : 0, __flag ? 1 : 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (execform);

/* <array> execstack <subarray> */
DEFUNC_OPER (execstack)
G_STMT_START {
	hg_quark_t arg0, q;
	hg_array_t *a;
	gssize i, len, edepth;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	edepth = hg_stack_depth(estack);
	len = hg_array_maxlength(a);
	if (edepth > len) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto error;
	}
	/* do not include the last node */
	for (i = 0; i < edepth - 1; i++) {
		q = hg_stack_index(estack, edepth - i - 1, error);
		if (!hg_array_set(a, q, i, FALSE, error))
			goto error;
	}
	if (edepth != (len + 1)) {
		q = hg_array_make_subarray(a, 0, edepth - 2, NULL, error);
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto error;
		}
		hg_vm_quark_set_attributes(vm, &q,
					   hg_vm_quark_is_readable(vm, &arg0),
					   hg_vm_quark_is_writable(vm, &arg0),
					   hg_vm_quark_is_executable(vm, &arg0),
					   hg_vm_quark_is_editable(vm, &arg0));
	} else {
		q = arg0;
	}
	if (q == arg0)
		hg_stack_pop(ostack, error);
	else
		hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (execuserobject);

/* <array> executeonly <array>
 * <packedarray> executeonly <packedarray>
 * <file> executeonly <file>
 * <string> executeonly <string>
 */
DEFUNC_OPER (executeonly)
G_STMT_START {
	hg_quark_t *arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_peek(ostack, 0, error);
	if (HG_IS_QARRAY (*arg0) ||
	    HG_IS_QFILE (*arg0) ||
	    HG_IS_QSTRING (*arg0)) {
		if (!hg_vm_quark_is_editable(vm, arg0)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}
		hg_vm_quark_set_readable(vm, arg0, FALSE);
		hg_vm_quark_set_writable(vm, arg0, FALSE);
		hg_vm_quark_set_editable(vm, arg0, TRUE);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* - exit - */
DEFUNC_OPER (exit)
gssize __n G_GNUC_UNUSED = 0;
G_STMT_START {
	gsize i, j, edepth = hg_stack_depth(estack);
	hg_quark_t q;

	/* almost code is shared with .exit.
	 * only difference is to trap on %stopped_continue or not
	 */
	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i, error);
		if (HG_IS_QOPER (q)) {
			if (HG_OPER (q) == HG_enc_protected_for_yield_int_continue ||
			    HG_OPER (q) == HG_enc_protected_for_yield_real_continue) {
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				__n = i + 4;
			} else if (HG_OPER (q) == HG_enc_protected_loop_continue) {
				hg_stack_drop(estack, error);
				__n = i + 1;
			} else if (HG_OPER (q) == HG_enc_protected_repeat_continue) {
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				__n = i + 2;
			} else if (HG_OPER (q) == HG_enc_protected_forall_array_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_dict_continue ||
				   HG_OPER (q) == HG_enc_protected_forall_string_continue) {
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				hg_stack_drop(estack, error);
				__n = i + 3;
			} else if (HG_OPER (q) == HG_enc_protected_stopped_continue) {
				hg_vm_set_error(vm, qself, HG_VM_e_invalidexit);
				return FALSE;
			} else {
				continue;
			}
			for (j = 0; j < i; j++)
				hg_stack_drop(estack, error);
			break;
		} else if (HG_IS_QFILE (q)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidexit);
			return FALSE;
		}
	}
	if (i == edepth) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidexit);
	} else {
		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (0, -__n, 0);
DEFUNC_OPER_END

/* <base> <exponent> exp <real> */
DEFUNC_OPER (exp)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;
	gdouble base, exponent;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		base = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		base = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		exponent = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		exponent = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_REAL_IS_ZERO (base) &&
	    HG_REAL_IS_ZERO (exponent)) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefinedresult);
		return FALSE;
	}
	*ret = HG_QREAL (pow(base, exponent));

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <filename> <access> file -file- */
DEFUNC_OPER (file)
G_STMT_START {
	hg_quark_t arg0, arg1, q;
	hg_file_io_t iotype;
	hg_file_mode_t iomode;
	gchar *filename = NULL, *fmode = NULL;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	filename = hg_vm_string_get_cstr(vm, arg0, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		goto error;
	}
	fmode = hg_vm_string_get_cstr(vm, arg1, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		goto error;
	}

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
		q = hg_vm_get_io(vm, iotype, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto error;
		}
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_undefinedfilename);
			goto error;
		}
	} else {
		q = hg_file_new(hg_vm_get_mem(vm),
				filename, iomode, error, NULL);
		if (q == Qnil) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto error;
		}
		hg_vm_quark_set_attributes(vm, &q,
					   iomode & (HG_FILE_IO_MODE_READ|HG_FILE_IO_MODE_APPEND) ? TRUE : FALSE,
					   iomode & (HG_FILE_IO_MODE_WRITE|HG_FILE_IO_MODE_APPEND) ? TRUE : FALSE,
					   FALSE, TRUE);
	}
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	g_free(filename);
	g_free(fmode);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (filenameforall);
DEFUNC_UNIMPLEMENTED_OPER (fileposition);

/* - fill - */
DEFUNC_OPER (fill)
G_STMT_START {
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg, error);

	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (!hg_device_fill(vm->device, gstate, error)) {
		if (error && *error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
		} else {
			hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		}
		goto finalize;
	}
	retval = TRUE;
  finalize:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (filter);
DEFUNC_UNIMPLEMENTED_OPER (findencoding);
DEFUNC_UNIMPLEMENTED_OPER (findresource);
DEFUNC_UNIMPLEMENTED_OPER (flattenpath);

/* - flush - */
DEFUNC_OPER (flush)
G_STMT_START {
	hg_quark_t q;
	hg_file_t *stdout_;

	q = hg_vm_get_io(vm, HG_FILE_IO_STDOUT, error);
	stdout_ = HG_VM_LOCK (vm, q, error);
	if (stdout_ == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	hg_file_flush(stdout_, error);
	if (error && *error) {
		/* ignore error */
		g_clear_error(error);
	}
	retval = TRUE;

	HG_VM_UNLOCK (vm, q);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <file> flushfile - */
DEFUNC_OPER (flushfile)
G_STMT_START {
	hg_quark_t arg0;
	hg_file_t *f;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);

	if (!hg_file_flush(f, error)) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		goto error;
	}
	retval = TRUE;

	hg_stack_drop(ostack, error);
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <initial> <increment> <limit> <proc> for - */
DEFUNC_OPER (for)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, arg3, q;
	gdouble dinit, dinc, dlimit;
	gboolean fint = TRUE;

	CHECK_STACK (ostack, 4);

	arg0 = hg_stack_index(ostack, 3, error);
	arg1 = hg_stack_index(ostack, 2, error);
	arg2 = hg_stack_index(ostack, 1, error);
	arg3 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QARRAY (arg3) ||
	    !hg_vm_quark_is_executable (vm, &arg3) ||
	    (!HG_IS_QINT (arg2) && !HG_IS_QREAL (arg2)) ||
	    (!HG_IS_QINT (arg1) && !HG_IS_QREAL (arg1)) ||
	    (!HG_IS_QINT (arg0) && !HG_IS_QREAL (arg0))) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
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
				      HG_QNAME (vm->name, "%for_yield_int_continue"),
				      FALSE,
				      error);
		arg0 = HG_QINT ((gint32)dinit);
		arg1 = HG_QINT ((gint32)dinc);
		arg2 = HG_QINT ((gint32)dlimit);
	} else {
		q = hg_vm_dict_lookup(vm,
				      vm->qsystemdict,
				      HG_QNAME (vm->name, "%for_yield_real_continue"),
				      FALSE,
				      error);
		arg0 = HG_QREAL (dinit);
		arg1 = HG_QREAL (dinc);
		arg2 = HG_QREAL (dlimit);
	}
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefined);
		return FALSE;
	}

	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, arg1);
	STACK_PUSH (estack, arg2);
	STACK_PUSH (estack, arg3);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 6, -1, error);

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-4, 5, 0);
DEFUNC_OPER_END

static gboolean
_hg_operator_dup_dict(hg_mem_t    *mem,
		      hg_quark_t   qkey,
		      hg_quark_t   qval,
		      gpointer     data,
		      GError     **error)
{
	hg_dict_t *dict = data;

	return hg_dict_add(dict, qkey, qval, TRUE, error);
}

/* <array> <proc> forall -
 * <packedarray> <proc> forall -
 * <dict> <proc> forall -
 * <string> <proc> forall -
 */
DEFUNC_OPER (forall)
G_STMT_START {
	hg_quark_t arg0, arg1, q = Qnil, qd;
	hg_dict_t *dict, *new_dict;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QARRAY (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_executable(vm, &arg1) ||
	    !hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_IS_QARRAY (arg0)) {
		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME (vm->name, "%forall_array_continue"),
				      FALSE, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
	} else if (HG_IS_QDICT (arg0)) {
		dict = HG_VM_LOCK (vm, arg0, error);
		if (dict == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		qd = hg_dict_new(dict->o.mem,
				 hg_dict_maxlength(dict),
				 dict->raise_dictfull,
				 (gpointer *)&new_dict);
		if (qd == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			HG_VM_UNLOCK (vm, arg0);
			goto d_error;
		}
		hg_vm_quark_set_attributes(vm, &qd,
					   hg_vm_quark_is_readable(vm, &arg0),
					   hg_vm_quark_is_writable(vm, &arg0),
					   hg_vm_quark_is_executable(vm, &arg0),
					   hg_vm_quark_is_editable(vm, &arg0));
		hg_dict_foreach(dict, _hg_operator_dup_dict, new_dict, error);
		HG_VM_UNLOCK (vm, arg0);

		arg0 = qd;

		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME (vm->name, "%forall_dict_continue"),
				      FALSE, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
		}
	  d_error:
		HG_VM_UNLOCK (vm, qd);
	} else if (HG_IS_QSTRING (arg0)) {
		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME (vm->name, "%forall_string_continue"),
				      FALSE, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (q == Qnil) {
		if (!hg_vm_has_error(vm))
			hg_vm_set_error(vm, qself, HG_VM_e_undefined);
		return FALSE;
	}

	STACK_PUSH (estack, HG_QINT (0));
	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, arg1);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 5, -1, error);

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 4, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (fork);
DEFUNC_UNIMPLEMENTED_OPER (framedevice);

/* <any> gcheck <bool> */
DEFUNC_OPER (gcheck)
G_STMT_START {
	hg_quark_t arg0, *qret;
	gboolean ret;

	CHECK_STACK (ostack, 1);

	qret = hg_stack_peek(ostack, 0, error);
	arg0 = *qret;
	if (hg_quark_is_simple_object(arg0) ||
	    HG_IS_QOPER (arg0)) {
		ret = TRUE;
	} else {
		ret = hg_quark_has_same_mem_id(arg0, vm->mem_id[HG_VM_MEM_GLOBAL]);
	}
	*qret = HG_QBOOL (ret);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <num1> <num2> ge <bool>
 * <string1> <string2> ge <bool>
 */
DEFUNC_OPER (ge)
G_STMT_START {
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		gdouble d1, d2;

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
		gchar *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) >= 0);

	  s_error:
		g_free(cs1);
		g_free(cs2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack, error);

		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

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

	if (!hg_vm_quark_is_readable(vm, &arg0)) {
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
		len = hg_array_maxlength(a);
		if (index < 0 || index >= len) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			goto a_error;
		}
		q = hg_array_get(a, index, error);

		retval = TRUE;
	  a_error:
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QDICT (arg0)) {
		q = hg_vm_dict_lookup(vm, arg0, arg1, TRUE, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
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
	if (retval) {
		hg_stack_drop(ostack, error);
		hg_stack_drop(ostack, error);

		STACK_PUSH (ostack, q);
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <array> <index> <count> getinterval <subarray>
 * <packedarray> <index> <count> getinterval <subarray>
 * <string> <index> <count> getinterval <substring>
 */
DEFUNC_OPER (getinterval)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, q = Qnil;
	gssize len, index, count;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg1) ||
	    !HG_IS_QINT (arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	index = HG_INT (arg1);
	count = HG_INT (arg2);

	if (HG_IS_QARRAY (arg0)) {
		hg_array_t *a = HG_VM_LOCK (vm, arg0, error);

		if (a == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		len = hg_array_maxlength(a);
		if (index >= len ||
		    (len - index) < count) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			goto error;
		}
		q = hg_array_make_subarray(a, index, index + count - 1, NULL, error);
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s = HG_VM_LOCK (vm, arg0, error);

		if (s == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		len = hg_string_maxlength(s);
		if (index >= len ||
		    (len - index) < count) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			goto error;
		}
		q = hg_string_make_substring(s, index, index + count - 1, NULL, error);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	hg_vm_quark_set_attributes(vm, &q,
				   hg_vm_quark_is_readable(vm, &arg0),
				   hg_vm_quark_is_writable(vm, &arg0),
				   hg_vm_quark_is_executable(vm, &arg0),
				   hg_vm_quark_is_editable(vm, &arg0));

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (glyphshow);

/* - grestore - */
DEFUNC_OPER (grestore)
G_STMT_START {
	hg_quark_t q;
	hg_gstate_t *g;

	if (hg_stack_depth(vm->stacks[HG_VM_STACK_GSTATE]) > 0) {
		q = hg_stack_index(vm->stacks[HG_VM_STACK_GSTATE], 0, error);

		g = HG_VM_LOCK (vm, q, error);
		if (g == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		hg_vm_set_gstate(vm, q);
		if (g->is_snapshot == Qnil)
			hg_stack_drop(vm->stacks[HG_VM_STACK_GSTATE], error);

		HG_VM_UNLOCK (vm, q);
	}
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* - grestoreall - */
DEFUNC_OPER (grestoreall)
G_STMT_START {
	hg_quark_t q, qq = Qnil;
	hg_gstate_t *g;

	while (hg_stack_depth(vm->stacks[HG_VM_STACK_GSTATE]) > 0 && qq == Qnil) {
		q = hg_stack_index(vm->stacks[HG_VM_STACK_GSTATE], 0, error);

		g = HG_VM_LOCK (vm, q, error);
		if (g == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		hg_vm_set_gstate(vm, q);
		qq = g->is_snapshot;
		if (qq == Qnil)
			hg_stack_drop(vm->stacks[HG_VM_STACK_GSTATE], error);

		HG_VM_UNLOCK (vm, q);
	}
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* - gsave - */
DEFUNC_OPER (gsave)
G_STMT_START {
	hg_quark_t q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_save(gstate, Qnil);

	HG_VM_UNLOCK (vm, qg);

	STACK_PUSH (vm->stacks[HG_VM_STACK_GSTATE], qg);
	hg_vm_set_gstate(vm, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (gstate);

/* <num1> <num2> gt <bool>
 * <string1> <string2> gt <bool>
 */
DEFUNC_OPER (gt)
G_STMT_START {
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		gdouble d1, d2;

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
		gchar *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) > 0);

	  s_error:
		g_free(cs1);
		g_free(cs2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack, error);

		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <matrix> identmatrix <matrix> */
DEFUNC_OPER (identmatrix)
G_STMT_START {
	hg_quark_t arg0;
	hg_array_t *a = NULL;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	a = HG_VM_LOCK (vm, arg0, error);
	if (!a) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (hg_array_maxlength(a) != 6) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto error;
	}
	retval = hg_array_matrix_ident(a);
  error:
	if (a)
		HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <int1> <int2> idiv <quotient> */
DEFUNC_OPER (idiv)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	*ret = HG_QINT (HG_INT (arg0) / HG_INT (arg1));

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (idtransform);

/* <bool> <proc> if - */
DEFUNC_OPER (if)
gboolean __flag G_GNUC_UNUSED = FALSE;
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
	    !hg_vm_quark_is_executable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg1)) {
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
		hg_stack_exch(estack, error);
		__flag = TRUE;
	}
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, __flag ? 1 : 0, 0);
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
	    !hg_vm_quark_is_executable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (!HG_IS_QARRAY (arg2) ||
	    !hg_vm_quark_is_executable(vm, &arg2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg2)) {
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
	hg_stack_exch(estack, error);

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-3, 1, 0);
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
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ineofill);
DEFUNC_UNIMPLEMENTED_OPER (infill);

/* - initclip - */
DEFUNC_OPER (initclip)
G_STMT_START {
	hg_stack_t *es = hg_vm_stack_new(vm, 256);

	if (hg_vm_eval_from_cstring(vm, ".currentpagedevice /PageSize get 0 0 moveto dup 0 get 0 lineto dup aload pop lineto 1 get 0 exch lineto closepath", -1,
				    NULL, es, NULL, TRUE, error)) {
		hg_quark_t qg = hg_vm_get_gstate(vm), qpath;
		hg_gstate_t *gstate = HG_VM_LOCK (vm, qg, error);

		if (gstate == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto finalize;
		}
		qpath = hg_gstate_get_path(gstate);
		hg_gstate_set_clippath(gstate, qpath);
		qpath = hg_path_new(gstate->o.mem, NULL);
		if (qpath == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto e_finalize;
		}
		hg_gstate_set_path(gstate, qpath);

		retval = TRUE;
	  e_finalize:
		HG_VM_UNLOCK (vm, qg);
	}
  finalize:
	hg_stack_free(es);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (initviewclip);
DEFUNC_UNIMPLEMENTED_OPER (instroke);
DEFUNC_UNIMPLEMENTED_OPER (inueofill);
DEFUNC_UNIMPLEMENTED_OPER (inufill);
DEFUNC_UNIMPLEMENTED_OPER (inustroke);

/* <matrix1> <matrix2> invertmatrix <matrix3> */
DEFUNC_OPER (invertmatrix)
G_STMT_START {
	hg_quark_t arg0, arg1;
	hg_array_t *a1 = NULL, *a2 = NULL;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0) ||
	    !HG_IS_QARRAY (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	a1 = HG_VM_LOCK (vm, arg0, error);
	a2 = HG_VM_LOCK (vm, arg1, error);
	if (!a1 || !a2) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}

	if (!hg_array_is_matrix(a1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto error;
	}
	if (hg_array_maxlength(a2) != 6) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto error;
	}

	if (!hg_array_matrix_invert(a1, a2)) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefinedresult);
		goto error;
	}

	hg_stack_exch(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;

  error:
	if (a1)
		HG_VM_UNLOCK (vm, arg0);
	if (a2)
		HG_VM_UNLOCK (vm, arg1);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (itransform);
DEFUNC_UNIMPLEMENTED_OPER (join);

/* <dict> <key> known <bool> */
DEFUNC_OPER (known)
G_STMT_START {
	hg_quark_t arg0, arg1, q, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QDICT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	q = hg_vm_dict_lookup(vm, arg0, arg1, TRUE, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}

	*ret = HG_QBOOL (q != Qnil);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (kshow);

/* - languagelevel <int> */
DEFUNC_OPER (languagelevel)
G_STMT_START {
	STACK_PUSH (ostack, HG_QINT (hg_vm_get_language_level(vm) + 1));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* <num1> <num2> le <bool>
 * <string1> <string2> le <bool>
 */
DEFUNC_OPER (le)
G_STMT_START {
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		gdouble d1, d2;

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
		gchar *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) <= 0);

	  s_error:
		g_free(cs1);
		g_free(cs2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack, error);

		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <array> length <int>
 * <packedarray> length <int>
 * <dict> length <int>
 * <string> length <int>
 * <name> length <int>
 */
DEFUNC_OPER (length)
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;

	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_IS_QARRAY (arg0)) {
		hg_array_t *a = HG_VM_LOCK (vm, arg0, error);

		if (a == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		*ret = HG_QINT (hg_array_maxlength(a));
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QDICT (arg0)) {
		hg_dict_t *d = HG_VM_LOCK (vm, arg0, error);

		if (d == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		*ret = HG_QINT (hg_dict_length(d));
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s = HG_VM_LOCK (vm, arg0, error);

		if (s == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		*ret = HG_QINT (hg_string_maxlength(s));
		HG_VM_UNLOCK (vm, arg0);
	} else if (HG_IS_QNAME (arg0) || HG_IS_QEVALNAME (arg0)) {
		const gchar *n = hg_name_lookup(vm->name, arg0);

		if (n == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		*ret = HG_QINT (strlen(n));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <x> <y> lineto - */
DEFUNC_OPER (lineto)
G_STMT_START {
	hg_quark_t arg0, arg1, q, qg = hg_vm_get_gstate(vm);
	gdouble dx, dy;
	hg_gstate_t *gstate;
	hg_path_t *path;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_get_current_point(path, NULL, NULL)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto error2;
	}
	retval = hg_path_lineto(path, dx, dy);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <num> ln <real> */
DEFUNC_OPER (ln)
G_STMT_START {
	hg_quark_t arg0, *ret;
	gdouble d;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	if (HG_REAL_LE (d, 0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	*ret = HG_QREAL (log(d));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (lock);

/* <num> log <real> */
DEFUNC_OPER (log)
G_STMT_START {
	hg_quark_t arg0, *ret;
	gdouble d;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	if (HG_REAL_LE (d, 0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	*ret = HG_QREAL (log10(d));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <proc> loop - */
DEFUNC_OPER (loop)
G_STMT_START {
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_executable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	q = hg_vm_dict_lookup(vm, vm->qsystemdict,
			      HG_QNAME (vm->name, "%loop_continue"),
			      FALSE, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefined);
		return FALSE;
	}

	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 3, -1, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 2, 0);
DEFUNC_OPER_END

/* <num1> <num2> lt <bool>
 * <string1> <string2> lt <bool>
 */
DEFUNC_OPER (lt)
G_STMT_START {
	hg_quark_t arg0, arg1, q = Qnil, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if ((HG_IS_QINT (arg0) || HG_IS_QREAL (arg0)) &&
	    (HG_IS_QINT (arg1) || HG_IS_QREAL (arg1))) {
		gdouble d1, d2;

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
		gchar *cs1 = NULL, *cs2 = NULL;

		cs1 = hg_vm_string_get_cstr(vm, arg0, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		cs2 = hg_vm_string_get_cstr(vm, arg1, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			goto s_error;
		}
		q = HG_QBOOL (strcmp(cs1, cs2) < 0);

	  s_error:
		g_free(cs1);
		g_free(cs2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (q != Qnil) {
		*ret = q;
		hg_stack_drop(ostack, error);

		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (makefont);
DEFUNC_UNIMPLEMENTED_OPER (makepattern);

/* <dict> maxlength <int> */
DEFUNC_OPER (maxlength)
G_STMT_START {
	hg_quark_t arg0, *ret;
	hg_dict_t *dict;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (!HG_IS_QDICT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	dict = HG_VM_LOCK (vm, arg0, error);
	if (dict == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	*ret = HG_QINT (hg_dict_maxlength(dict));
	HG_VM_UNLOCK (vm, arg0);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <int1> <int2> mod <remainder> */
DEFUNC_OPER (mod)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_INT (arg1) == 0) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefinedresult);
		return FALSE;
	}
	*ret = HG_QINT (HG_INT (arg0) % HG_INT (arg1));

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (monitor);

/* <x> <y> moveto - */
DEFUNC_OPER (moveto)
G_STMT_START {
	hg_quark_t arg0, arg1, q, qg = hg_vm_get_gstate(vm);
	gdouble dx, dy;
	hg_gstate_t *gstate;
	hg_path_t *path;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	if (q == Qnil) {
		path = NULL;
		q = hg_path_new(hg_vm_get_mem(vm), (gpointer *)&path);
		if (q != Qnil)
			hg_gstate_set_path(gstate, q);
	} else {
		path = HG_VM_LOCK (vm, q, error);
	}
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	retval = hg_path_moveto(path, dx, dy);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <num1> <num2> mul <product> */
DEFUNC_OPER (mul)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;
	gdouble d1, d2, dr;
	gboolean is_int = TRUE;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
		is_int = FALSE;
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	dr = d1 * d2;
	if (is_int &&
	    HG_REAL_LE (dr, G_MAXINT32) &&
	    HG_REAL_GE (dr, G_MININT32)) {
		*ret = HG_QINT ((gint32)dr);
	} else {
		*ret = HG_QREAL (dr);
	}
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <any1> <any2> ne <bool> */
DEFUNC_OPER (ne)
G_STMT_START {
	hg_quark_t arg0, arg1, *qret;
	gboolean ret = FALSE;

	CHECK_STACK (ostack, 2);

	qret = hg_stack_peek(ostack, 1, error);
	arg0 = *qret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	ret = !hg_vm_quark_compare(vm, arg0, arg1);
	if (ret) {
		if ((HG_IS_QNAME (arg0) ||
		     HG_IS_QEVALNAME (arg0) ||
		     HG_IS_QSTRING (arg0)) &&
		    (HG_IS_QNAME (arg1) ||
		     HG_IS_QEVALNAME (arg1) ||
		     HG_IS_QSTRING (arg1))) {
			gchar *s1 = NULL, *s2 = NULL;

			if (HG_IS_QNAME (arg0) ||
			    HG_IS_QEVALNAME (arg0)) {
				s1 = g_strdup(HG_NAME (vm->name, arg0));
			} else {
				s1 = hg_vm_string_get_cstr(vm, arg0, error);
				if (*error) {
					hg_vm_set_error_from_gerror(vm, qself, *error);
					goto s_error;
				}
			}
			if (HG_IS_QNAME (arg1) ||
			    HG_IS_QEVALNAME (arg1)) {
				s2 = g_strdup(HG_NAME (vm->name, arg1));
			} else {
				s2 = hg_vm_string_get_cstr(vm, arg1, error);
				if (*error) {
					hg_vm_set_error_from_gerror(vm, qself, *error);
					goto s_error;
				}
			}
			if (s1 != NULL && s2 != NULL)
				ret = (strcmp(s1, s2) != 0);
		  s_error:
			g_free(s1);
			g_free(s2);
		} else if ((HG_IS_QINT (arg0) ||
			    HG_IS_QREAL (arg0)) &&
			   (HG_IS_QINT (arg1) ||
			    HG_IS_QREAL (arg1))) {
			gdouble d1, d2;

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
	if (!*error) {
		*qret = HG_QBOOL (ret);
		hg_stack_drop(ostack, error);

		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <num1> neg <num2> */
DEFUNC_OPER (neg)
G_STMT_START {
	hg_quark_t arg0, *ret;
	gdouble d;
	gboolean is_int = TRUE;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = -HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = -HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	if (is_int &&
	    HG_REAL_LE (d, G_MAXINT32) &&
	    HG_REAL_GE (d, G_MININT32)) {
		*ret = HG_QINT ((gint32)d);
	} else {
		*ret = HG_QREAL (d);
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* - newpath - */
DEFUNC_OPER (newpath)
G_STMT_START {
	hg_quark_t q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;

	q = hg_path_new(vm->mem[HG_VM_MEM_LOCAL], NULL);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_gstate_set_path(gstate, q);

	HG_VM_UNLOCK (vm, qg);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <array> noaccess <array>
 * <packedarray> noaccess <packedarray>
 * <dict> noaccess <dict>
 * <file> noaccess <file>
 * <string> noaccess <string>
 */
DEFUNC_OPER (noaccess)
G_STMT_START {
	hg_quark_t *arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_peek(ostack, 0, error);
	if (HG_IS_QARRAY (*arg0) ||
	    HG_IS_QFILE (*arg0) ||
	    HG_IS_QSTRING (*arg0) ||
	    HG_IS_QDICT (*arg0)) {
		hg_vm_quark_set_attributes(vm, arg0,
					   FALSE, FALSE, FALSE, FALSE);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <bool> not <bool>
 * <int> not <int>
 */
DEFUNC_OPER (not)
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QBOOL (arg0)) {
		*ret = HG_QBOOL (!HG_BOOL (arg0));
	} else if (HG_IS_QINT (arg0)) {
		*ret = HG_QINT (-HG_INT (arg0));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (notify);
DEFUNC_UNIMPLEMENTED_OPER (nulldevice);

/* <bool1> <bool2> or <bool3>
 * <int1> <int2> or <int3>
 */
DEFUNC_OPER (or)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QBOOL (arg0) &&
	    HG_IS_QBOOL (arg1)) {
		*ret = HG_QBOOL (HG_BOOL (arg0) | HG_BOOL (arg1));
	} else if (HG_IS_QINT (arg0) &&
		   HG_IS_QINT (arg1)) {
		*ret = HG_QINT (HG_INT (arg0) | HG_INT (arg1));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (packedarray);

/* - pathbbox <llx> <lly> <urx> <ury> */
DEFUNC_OPER (pathbbox)
G_STMT_START {
	hg_quark_t q, qg = hg_vm_get_gstate(vm);
	hg_path_t *path = NULL;
	hg_path_bbox_t bbox;
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg, error);

	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto finalize;
	}
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}
	if (!hg_path_get_bbox(path,
			      hg_vm_get_language_level(vm) != HG_LANG_LEVEL_1,
			      &bbox, error)) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		goto finalize;
	}
	STACK_PUSH (ostack, HG_QREAL (bbox.llx));
	STACK_PUSH (ostack, HG_QREAL (bbox.lly));
	STACK_PUSH (ostack, HG_QREAL (bbox.urx));
	STACK_PUSH (ostack, HG_QREAL (bbox.ury));

	retval = TRUE;
  finalize:
	if (path)
		HG_VM_UNLOCK (vm, q);
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (4, 0, 0);
DEFUNC_OPER_END

/* <move> <line> <curve> <close> pathforall - */
DEFUNC_UNIMPLEMENTED_OPER (pathforall);

DEFUNC_OPER (pop)
G_STMT_START {
	CHECK_STACK (ostack, 1);

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <string> print - */
DEFUNC_OPER (print)
G_STMT_START {
	hg_quark_t arg0, qstdout;
	hg_file_t *stdout;
	gchar *cstr;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	cstr = hg_vm_string_get_cstr(vm, arg0, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}
	qstdout = hg_vm_get_io(vm, HG_FILE_IO_STDOUT, error);
	stdout = HG_VM_LOCK (vm, qstdout, error);
	if (stdout == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}

	hg_file_write(stdout, cstr,
		      sizeof (gchar),
		      hg_vm_string_length(vm, arg0, error),
		      error);
	if (error && *error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
	} else {
		hg_stack_drop(ostack, error);

		retval = TRUE;
	}
	HG_VM_UNLOCK (vm, qstdout);
  error:
	g_free(cstr);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (printobject);
DEFUNC_UNIMPLEMENTED_OPER (product);

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
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	is_in_global = hg_quark_has_same_mem_id(arg0, vm->mem_id[HG_VM_MEM_GLOBAL]);
	if (is_in_global &&
	    !hg_quark_is_simple_object(arg2) &&
	    hg_quark_has_same_mem_id(arg2, vm->mem_id[HG_VM_MEM_LOCAL])) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_IS_QARRAY (arg0)) {
		gsize index;

		if (!HG_IS_QINT (arg1)) {
			hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
			return FALSE;
		}
		index = HG_INT (arg1);
		retval = hg_vm_array_set(vm, arg0, arg2, index, FALSE, error);
		if (!retval) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
	} else if (HG_IS_QDICT (arg0)) {
		retval = hg_vm_dict_add(vm, arg0, arg1, arg2, FALSE, error);
		if (!retval) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
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

	if (retval) {
		hg_stack_drop(ostack, error);
		hg_stack_drop(ostack, error);
		hg_stack_drop(ostack, error);
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (-3, 0, 0);
DEFUNC_OPER_END

/* - rand <int> */
DEFUNC_OPER (rand)
G_STMT_START {
	hg_quark_t ret;

	ret = HG_QINT (hg_vm_rand_int(vm));

	STACK_PUSH (ostack, ret);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* <array> rcheck <bool>
 * <packedarray> rcheck <bool>
 * <dict> rcheck <bool>
 * <file> rcheck <bool>
 * <string> rcheck <bool>
 */
DEFUNC_OPER (rcheck)
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (!HG_IS_QARRAY (arg0) &&
	    !HG_IS_QDICT (arg0) &&
	    !HG_IS_QFILE (arg0) &&
	    !HG_IS_QSTRING (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	*ret = HG_QBOOL (hg_vm_quark_is_readable(vm, &arg0));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <dx1> <dy1> <dx2> <dy2> <dx3> <dy3> rcurveto - */
DEFUNC_OPER (rcurveto)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, arg3, arg4, arg5, q, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	hg_path_t *path;
	gdouble dx1, dy1, dx2, dy2, dx3, dy3;

	CHECK_STACK (ostack, 6);

	arg0 = hg_stack_index(ostack, 5, error);
	arg1 = hg_stack_index(ostack, 4, error);
	arg2 = hg_stack_index(ostack, 3, error);
	arg3 = hg_stack_index(ostack, 2, error);
	arg4 = hg_stack_index(ostack, 1, error);
	arg5 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx1 = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy1 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		dy1 = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		dx2 = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		dx2 = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg3)) {
		dy2 = HG_INT (arg3);
	} else if (HG_IS_QREAL (arg3)) {
		dy2 = HG_REAL (arg3);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg4)) {
		dx3 = HG_INT (arg4);
	} else if (HG_IS_QREAL (arg4)) {
		dx3 = HG_REAL (arg4);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg5)) {
		dy3 = HG_INT (arg5);
	} else if (HG_IS_QREAL (arg5)) {
		dy3 = HG_REAL (arg5);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_get_current_point(path, NULL, NULL)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto error2;
	}
	retval = hg_path_rcurveto(path, dx1, dy1, dx2, dy2, dx3, dy3);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-6, 0, 0);
DEFUNC_OPER_END

/* <file> read <int> <true>
 * <file> read <false>
 */
DEFUNC_OPER (read)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t arg0;
	hg_file_t *f;
	gchar c[2];
	gboolean eof;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);
	if (f == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_file_read(f, c, sizeof (gchar), 1, error);
	eof = hg_file_is_eof(f);
	HG_VM_UNLOCK (vm, arg0);

	if (eof) {
		hg_stack_drop(ostack, error);

		STACK_PUSH (ostack, HG_QBOOL (FALSE));
		retval = TRUE;
	} else if (*error && error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
	} else {
		hg_stack_drop(ostack, error);

		STACK_PUSH (ostack, HG_QINT ((guchar)c[0]));
		STACK_PUSH (ostack, HG_QBOOL (TRUE));
		__flag = TRUE;
		retval = TRUE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (__flag ? 1 : 0, 0, 0);
DEFUNC_OPER_END

/* <file> <string> readhexstring <substring> <bool> */
DEFUNC_OPER (readhexstring)
G_STMT_START {
	hg_quark_t arg0, arg1, q, qs;
	hg_file_t *f = NULL;
	hg_string_t *s = NULL;
	gchar c[2], hex = 0;
	gint shift = 4;
	gssize ret;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!HG_IS_QSTRING (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	f = HG_VM_LOCK (vm, arg0, error);
	if (f == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg1, error);
	if (s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}

	if (hg_string_maxlength(s) == 0) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}
	hg_string_clear(s);

	while (!hg_file_is_eof(f) && hg_string_length(s) < hg_string_maxlength(s)) {
		ret = hg_file_read(f, c, sizeof (gchar), 1, error);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			hg_vm_set_error(vm, qself, HG_VM_e_ioerror);
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
			hg_string_append_c(s, hex, error);
			hex = 0;
		}
	}
	if (hg_file_is_eof(f)) {
		q = HG_QBOOL (FALSE);
		qs = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL, error);
		if (qs == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto finalize;
		}
		hg_vm_quark_set_attributes(vm, &qs,
					   hg_vm_quark_is_readable(vm, &arg1),
					   hg_vm_quark_is_writable(vm, &arg1),
					   hg_vm_quark_is_executable(vm, &arg1),
					   hg_vm_quark_is_editable(vm, &arg1));
	} else {
		q = HG_QBOOL (TRUE);
		qs = arg1;
	}
	if (qs == arg1)
		hg_stack_pop(ostack, error);
	else
		hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, qs);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  finalize:
	if (f)
		HG_VM_UNLOCK (vm, arg0);
	if (s)
		HG_VM_UNLOCK (vm, arg1);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <file> <string> readline <substring> <bool> */
DEFUNC_OPER (readline)
G_STMT_START {
	hg_quark_t arg0, arg1, q, qs;
	hg_file_t *f = NULL;
	hg_string_t *s = NULL;
	gchar c[2];
	gboolean eol = FALSE;
	gssize ret;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!HG_IS_QSTRING (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	f = HG_VM_LOCK (vm, arg0, error);
	if (f == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg1, error);
	if (s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}

	hg_string_clear(s);

	while (!hg_file_is_eof(f)) {
		if (hg_string_length(s) >= hg_string_maxlength(s)) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			goto finalize;
		}
	  retry:
		ret = hg_file_read(f, c, sizeof (gchar), 1, error);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			hg_vm_set_error(vm, qself, HG_VM_e_ioerror);
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
			hg_file_seek(f, -1, HG_FILE_POS_CURRENT, error);
			break;
		}
		hg_string_append_c(s, c[0], error);
	}
	if (hg_file_is_eof(f)) {
		q = HG_QBOOL (FALSE);
	} else {
		q = HG_QBOOL (TRUE);
	}
	qs = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL, error);
	if (qs == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}
	hg_vm_quark_set_attributes(vm, &qs,
				   hg_vm_quark_is_readable(vm, &arg1),
				   hg_vm_quark_is_writable(vm, &arg1),
				   hg_vm_quark_is_executable(vm, &arg1),
				   hg_vm_quark_is_editable(vm, &arg1));

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, qs);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  finalize:
	if (f)
		HG_VM_UNLOCK (vm, arg0);
	if (s)
		HG_VM_UNLOCK (vm, arg1);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* -array- readonly -array-
 * -packedarray- readonly -packedarray-
 * -dict- readonly -dict-
 * -file- readonly -file-
 * -string- readonly -string-
 */
DEFUNC_OPER (readonly)
G_STMT_START {
	hg_quark_t *arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_peek(ostack, 0, error);
	if (!HG_IS_QARRAY (*arg0) &&
	    !HG_IS_QDICT (*arg0) &&
	    !HG_IS_QFILE (*arg0) &&
	    !HG_IS_QSTRING (*arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	hg_vm_quark_set_writable(vm, arg0, FALSE);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <file> <string> readstring <substring> <bool> */
DEFUNC_OPER (readstring)
G_STMT_START {
	hg_quark_t arg0, arg1, q, qs;
	hg_file_t *f = NULL;
	hg_string_t *s = NULL;
	gchar c[2];
	gssize ret;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QFILE (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!HG_IS_QSTRING (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_writable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	f = HG_VM_LOCK (vm, arg0, error);
	if (f == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	s = HG_VM_LOCK (vm, arg1, error);
	if (s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}

	if (hg_string_maxlength(s) == 0) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}
	hg_string_clear(s);

	while (!hg_file_is_eof(f) && hg_string_length(s) < hg_string_maxlength(s)) {
		ret = hg_file_read(f, c, sizeof (gchar), 1, error);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			hg_vm_set_error(vm, qself, HG_VM_e_ioerror);
			goto finalize;
		}
		hg_string_append_c(s, c[0], error);
	}
	if (hg_file_is_eof(f)) {
		q = HG_QBOOL (FALSE);
		qs = hg_string_make_substring(s, 0, hg_string_length(s) - 1, NULL, error);
		if (qs == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto finalize;
		}
		hg_vm_quark_set_attributes(vm, &qs,
					   hg_vm_quark_is_readable(vm, &arg1),
					   hg_vm_quark_is_writable(vm, &arg1),
					   hg_vm_quark_is_executable(vm, &arg1),
					   hg_vm_quark_is_editable(vm, &arg1));
	} else {
		q = HG_QBOOL (TRUE);
		qs = arg1;
	}
	if (qs == arg1)
		hg_stack_pop(ostack, error);
	else
		hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, qs);
	STACK_PUSH (ostack, q);

	retval = TRUE;
  finalize:
	if (f)
		HG_VM_UNLOCK (vm, arg0);
	if (s)
		HG_VM_UNLOCK (vm, arg1);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (realtime);
DEFUNC_UNIMPLEMENTED_OPER (rectclip);
DEFUNC_UNIMPLEMENTED_OPER (rectfill);
DEFUNC_UNIMPLEMENTED_OPER (rectstroke);
DEFUNC_UNIMPLEMENTED_OPER (rectviewclip);
DEFUNC_UNIMPLEMENTED_OPER (renamefile);
DEFUNC_UNIMPLEMENTED_OPER (renderbands);

/* <int> <proc> repeat - */
DEFUNC_OPER (repeat)
G_STMT_START {
	hg_quark_t arg0, arg1, q;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QINT (arg0) ||
	    !HG_IS_QARRAY (arg1) ||
	    !hg_vm_quark_is_executable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (HG_INT (arg0) < 0) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}

	q = hg_vm_dict_lookup(vm, vm->qsystemdict,
			      HG_QNAME (vm->name, "%repeat_continue"),
			      FALSE, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_undefined);
		return FALSE;
	}

	STACK_PUSH (estack, arg0);
	STACK_PUSH (estack, arg1);
	STACK_PUSH (estack, q);

	hg_stack_roll(estack, 4, -1, error);

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 3, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (resetfile);
DEFUNC_UNIMPLEMENTED_OPER (resourceforall);
DEFUNC_UNIMPLEMENTED_OPER (resourcestatus);

static gboolean
_hg_operator_restore_mark_traverse(hg_mem_t    *mem,
				   hg_quark_t   qdata,
				   gpointer     data,
				   GError     **error)
{
	guint id = hg_quark_get_mem_id(qdata);
	hg_mem_t *m = hg_mem_get(id);

	if (m == NULL) {
		g_set_error(error, HG_ERROR, HG_VM_e_VMerror,
			    "No memory spool found");
		return FALSE;
	}
	hg_mem_restore_mark(m, qdata);

	return TRUE;
}

static gboolean
_hg_operator_restore_mark(hg_mem_t *mem,
			  gpointer  data)
{
	hg_vm_t *vm = (hg_vm_t *)data;
	gsize i;
	GError *err = NULL;

	for (i = 0; i <= HG_VM_STACK_DSTACK; i++) {
		hg_stack_foreach(vm->stacks[i],
				 _hg_operator_restore_mark_traverse,
				 vm, FALSE, &err);
		if (err) {
			g_error_free(err);
			return FALSE;
		}
	}

	return TRUE;
}

/* <save> restore - */
DEFUNC_OPER (restore)
G_STMT_START {
	hg_quark_t arg0, q, qq = Qnil;
	hg_snapshot_t *sn;
	hg_gstate_t *g;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QSNAPSHOT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	/* can't call _hg_operator_real_grestoreall.
	 * check the quark what it's referenced from.
	 * otherwise restore will fails due to the remaining gstate object.
	 */
	while (hg_stack_depth(vm->stacks[HG_VM_STACK_GSTATE]) > 0 && qq != arg0) {
		q = hg_stack_index(vm->stacks[HG_VM_STACK_GSTATE], 0, error);

		g = HG_VM_LOCK (vm, q, error);
		if (g == NULL) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		hg_vm_set_gstate(vm, q);
		qq = g->is_snapshot;
		hg_stack_drop(vm->stacks[HG_VM_STACK_GSTATE], error);

		HG_VM_UNLOCK (vm, q);
	}

	sn = HG_VM_LOCK (vm, arg0, error);
	if (sn == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (!hg_snapshot_restore(sn,
				 &vm->vm_state,
				 _hg_operator_restore_mark,
				 vm)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidrestore);
		goto error;
	}
	retval = TRUE;
	hg_stack_drop(ostack, error);
  error:
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (reversepath);

/* <dx> <dy> rlineto - */
DEFUNC_OPER (rlineto)
G_STMT_START {
	hg_quark_t arg0, arg1, q, qg = hg_vm_get_gstate(vm);
	gdouble dx, dy;
	hg_gstate_t *gstate;
	hg_path_t *path;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_get_current_point(path, NULL, NULL)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto error2;
	}
	retval = hg_path_rlineto(path, dx, dy);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <dx> <dy> rmoveto - */
DEFUNC_OPER (rmoveto)
G_STMT_START {
	hg_quark_t arg0, arg1, q, qg = hg_vm_get_gstate(vm);
	gdouble dx, dy;
	hg_gstate_t *gstate;
	hg_path_t *path;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		dx = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		dx = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		dy = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg1)) {
		dy = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	q = hg_gstate_get_path(gstate);
	path = HG_VM_LOCK (vm, q, error);
	if (path == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	if (!hg_path_get_current_point(path, NULL, NULL)) {
		hg_vm_set_error(vm, qself, HG_VM_e_nocurrentpoint);
		goto error2;
	}
	retval = hg_path_rmoveto(path, dx, dy);
	if (!retval) {
		hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		goto error2;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error2:
	HG_VM_UNLOCK (vm, q);
  error:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

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

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	hg_stack_roll(ostack, HG_INT (arg0), HG_INT (arg1), error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (rootfont);

/* <angle> rotate -
 * <angle> <matrix> rotate <matrix>
 */
DEFUNC_OPER (rotate)
G_STMT_START {
	hg_quark_t arg0, arg1 = Qnil, qg = hg_vm_get_gstate(vm);
	gdouble angle;
	hg_matrix_t ctm;
	hg_array_t *a;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QARRAY (arg0)) {
		CHECK_STACK (ostack, 2);

		arg1 = arg0;
		arg0 = hg_stack_index(ostack, 1, error);
		a = HG_VM_LOCK (vm, arg1, error);
		if (!a)
			return FALSE;
		if (hg_array_maxlength(a) != 6) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			HG_VM_UNLOCK (vm, arg1);
			return FALSE;
		}
		HG_VM_UNLOCK (vm, arg1);
		ctm.mtx.xx = 1;
		ctm.mtx.xy = 0;
		ctm.mtx.yx = 0;
		ctm.mtx.yy = 1;
		ctm.mtx.x0 = 0;
		ctm.mtx.y0 = 0;
	} else {
		gstate = HG_VM_LOCK (vm, qg, error);
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
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	hg_matrix_rotate(&ctm, angle);

	if (arg1 == Qnil) {
		gstate = HG_VM_LOCK (vm, qg, error);
		if (!gstate)
			return FALSE;
		hg_gstate_set_ctm(gstate, &ctm);
		HG_VM_UNLOCK (vm, qg);
	} else {
		a = HG_VM_LOCK (vm, arg1, error);
		if (!a)
			return FALSE;
		hg_array_from_matrix(a, &ctm);
		HG_VM_UNLOCK (vm, arg1);

		hg_stack_exch(ostack, error);
	}

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <num> round <num> */
DEFUNC_OPER (round)
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;

	if (HG_IS_QINT (arg0)) {
		/* no need to proceed anything */
	} else if (HG_IS_QREAL (arg0)) {
		gdouble d = HG_REAL (arg0), dfloat, dr;
		glong base = (glong)d;

		/* glibc's round(3) behaves differently expecting in PostScript.
		 * particularly when rounding halfway.
		 *   glibc's round(3) round halfway cases away from zero.
		 *   PostScript's round halfway cases returns the greater of the two.
		 */
		dfloat = fabs(d - (gdouble)base);
		dr = (gdouble)base + (base < 0 ? -1.0 : 1.0);
		if (dfloat < 0.5) {
			dr = (gdouble)base;
		} if (HG_REAL_EQUAL (dfloat, 0.5)) {
			dr = (gdouble)MAX (base, dr);
		}

		*ret = HG_QREAL (dr);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* - rrand <int> */
DEFUNC_OPER (rrand)
G_STMT_START {
	STACK_PUSH (ostack, HG_QINT (hg_vm_get_rand_seed(vm)));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

/* - save <save> */
DEFUNC_OPER (save)
G_STMT_START {
	hg_quark_t q, qgg = hg_vm_get_gstate(vm), qg;
	hg_snapshot_t *sn;
	hg_gstate_t *gstate;

	q = hg_snapshot_new(vm->mem[HG_VM_MEM_LOCAL],
			    (gpointer *)&sn);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_snapshot_save(sn, &vm->vm_state);
	HG_VM_UNLOCK (vm, q);

	/* save the gstate too */
	gstate = HG_VM_LOCK (vm, qgg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	qg = hg_gstate_save(gstate, q);

	STACK_PUSH (vm->stacks[HG_VM_STACK_GSTATE], qgg);

	HG_VM_UNLOCK (vm, qgg);
	hg_vm_set_gstate(vm, qg);

	STACK_PUSH (ostack, q);

	vm->vm_state.n_save_objects++;

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (scale);
DEFUNC_UNIMPLEMENTED_OPER (scalefont);

/* <string> <seek> search <post> <match> <pre> true
 *                               <string> false
 */
DEFUNC_OPER (search)
gint __n G_GNUC_UNUSED = 0;
G_STMT_START {
	hg_quark_t arg0, arg1, qpost, qmatch, qpre;
	hg_string_t *s = NULL, *seek = NULL;
	gsize len0, len1;
	gchar *cs0 = NULL, *cs1 = NULL, *p;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QSTRING (arg0) ||
	    !HG_IS_QSTRING (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_readable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}

	s = HG_VM_LOCK (vm, arg0, error);
	seek = HG_VM_LOCK (vm, arg1, error);
	if (!s || !seek) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}

	len0 = hg_string_length(s);
	len1 = hg_string_length(seek);
	cs0 = hg_string_get_cstr(s);
	cs1 = hg_string_get_cstr(seek);

	if ((p = strstr(cs0, cs1))) {
		gsize lmatch, lpre;

		lpre = p - cs0;
		lmatch = len1;

		qpre = hg_string_make_substring(s, 0, lpre - 1, NULL, error);
		qmatch = hg_string_make_substring(s, lpre, lpre + len1 - 1, NULL, error);
		qpost = hg_string_make_substring(s, lpre + lmatch, len0 - 1, NULL, error);
		if (qpre == Qnil ||
		    qmatch == Qnil ||
		    qpost == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			goto finalize;
		}

		hg_stack_drop(ostack, error);
		hg_stack_drop(ostack, error);

		STACK_PUSH (ostack, qpost);
		STACK_PUSH (ostack, qmatch);
		STACK_PUSH (ostack, qpre);
		STACK_PUSH (ostack, HG_QBOOL (TRUE));

		__n = 4 - 2;
	} else {
		hg_stack_drop(ostack, error);

		STACK_PUSH (ostack, HG_QBOOL (FALSE));

		__n = 2 - 2;
	}

	retval = TRUE;
  finalize:
	g_free(cs0);
	g_free(cs1);
	if (s)
		HG_VM_UNLOCK (vm, arg0);
	if (seek)
		HG_VM_UNLOCK (vm, arg1);
} G_STMT_END;
VALIDATE_STACK_SIZE (__n, 0, 0);
DEFUNC_OPER_END

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
G_STMT_START {
	hg_quark_t arg0, arg1, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = NULL;
	gdouble offset;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		offset = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		offset = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	if (!hg_gstate_set_dash(gstate, arg0, offset, error)) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		goto finalize;
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
  finalize:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setdevparams);
DEFUNC_UNIMPLEMENTED_OPER (setfileposition);
DEFUNC_UNIMPLEMENTED_OPER (setflat);
DEFUNC_UNIMPLEMENTED_OPER (setfont);

/* <num> setgray - */
DEFUNC_OPER (setgray)
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	gdouble d;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (d > 1.0)
		d = 1.0;
	if (d < 0.0)
		d = 0.0;
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_gstate_set_graycolor(gstate, d);
	HG_VM_UNLOCK (vm, qg);

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setgstate);
DEFUNC_UNIMPLEMENTED_OPER (sethalftone);
DEFUNC_UNIMPLEMENTED_OPER (sethalftonephase);

/* <hue> <saturation> <brightness> sethsbcolor - */
DEFUNC_OPER (sethsbcolor)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	gdouble h, s, b;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		h = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		h = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		s = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		s = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		b = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		b = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (!gstate)
		return FALSE;

	hg_gstate_set_hsbcolor(gstate, h, s, b);

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	HG_VM_UNLOCK (vm, qg);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-3, 0, 0);
DEFUNC_OPER_END

/* <int> setlinecap - */
DEFUNC_OPER (setlinecap)
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	gint32 i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	i = HG_INT (arg0);
	if (i < 0 || i > 2) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_gstate_set_linecap(gstate, i);

	hg_stack_drop(ostack, error);
	HG_VM_UNLOCK (vm, qg);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <int> setlinejoin - */
DEFUNC_OPER (setlinejoin)
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	gint32 i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	i = HG_INT (arg0);
	if (i < 0 || i > 2) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_gstate_set_linejoin(gstate, i);

	hg_stack_drop(ostack, error);
	HG_VM_UNLOCK (vm, qg);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <num> setlinewidth - */
DEFUNC_OPER (setlinewidth)
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	gdouble d;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		d = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_gstate_set_linewidth(gstate, d);

	HG_VM_UNLOCK (vm, qg);

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <matrix> setmatrix - */
DEFUNC_OPER (setmatrix)
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	hg_array_t *a;
	hg_matrix_t m;
	hg_gstate_t *gstate = NULL;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QARRAY (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	a = HG_VM_LOCK (vm, arg0, error);
	if (a == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto finalize;
	}
	if (!hg_array_is_matrix(a) ||
	    !hg_array_to_matrix(a, &m)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}
	hg_gstate_set_ctm(gstate, &m);

	hg_stack_drop(ostack, error);

	retval = TRUE;
  finalize:
	if (gstate)
		HG_VM_UNLOCK (vm, qg);
	HG_VM_UNLOCK (vm, arg0);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <num> setmiterlimit - */
DEFUNC_OPER (setmiterlimit)
G_STMT_START {
	hg_quark_t arg0, qg = hg_vm_get_gstate(vm);
	gdouble miterlen;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QINT (arg0)) {
		miterlen = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		miterlen = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (!hg_gstate_set_miterlimit(gstate, miterlen)) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		goto finalize;
	}

	hg_stack_drop(ostack, error);

	retval = TRUE;
  finalize:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setobjectformat);
DEFUNC_UNIMPLEMENTED_OPER (setoverprint);
DEFUNC_UNIMPLEMENTED_OPER (setpacking);
DEFUNC_UNIMPLEMENTED_OPER (setpagedevice);
DEFUNC_UNIMPLEMENTED_OPER (setpattern);

/* <red> <green> <blue> setrgbcolor - */
DEFUNC_OPER (setrgbcolor)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2, qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate;
	gdouble r, g, b;

	CHECK_STACK (ostack, 3);

	arg0 = hg_stack_index(ostack, 2, error);
	arg1 = hg_stack_index(ostack, 1, error);
	arg2 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0)) {
		r = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		r = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		g = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg0)) {
		g = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (HG_IS_QINT (arg2)) {
		b = HG_INT (arg2);
	} else if (HG_IS_QREAL (arg2)) {
		b = HG_REAL (arg2);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	gstate = HG_VM_LOCK (vm, qg, error);
	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	hg_gstate_set_rgbcolor(gstate, r, g, b);

	HG_VM_UNLOCK (vm, qg);

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-3, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setscreen);
DEFUNC_UNIMPLEMENTED_OPER (setshared);
DEFUNC_UNIMPLEMENTED_OPER (setstrokeadjust);
DEFUNC_UNIMPLEMENTED_OPER (setsystemparams);
DEFUNC_UNIMPLEMENTED_OPER (settransfer);
DEFUNC_UNIMPLEMENTED_OPER (setucacheparams);
DEFUNC_UNIMPLEMENTED_OPER (setundercolorremoval);

/* <dict> setuserparams - */
DEFUNC_OPER (setuserparams)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	hg_vm_set_user_params(vm, arg0, error);
	if (*error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
		return FALSE;
	}

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (setvmthreshold);
DEFUNC_UNIMPLEMENTED_OPER (shareddict);
DEFUNC_UNIMPLEMENTED_OPER (show);
DEFUNC_UNIMPLEMENTED_OPER (showpage);

/* <angle> sin <real> */
DEFUNC_OPER (sin)
G_STMT_START {
	hg_quark_t arg0, *ret;
	gdouble angle;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		angle = (gdouble)HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		angle = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	*ret = HG_QREAL (sin(angle / 180 * M_PI));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <num> sqrt <real> */
DEFUNC_OPER (sqrt)
G_STMT_START {
	hg_quark_t arg0, *ret;
	gdouble d;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (HG_IS_QINT (arg0)) {
		d = (gdouble)HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d = HG_REAL (arg0);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	*ret = HG_QREAL (sqrt(d));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <int> srand - */
DEFUNC_OPER (srand)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	hg_vm_set_rand_seed(vm, HG_INT (arg0));

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (startjob);

/* <file> status <bool>
 * <filename> status <pages> <bytes> <referenced> <created> true
 * <filename> status false
 */
DEFUNC_OPER (status)
gint __n G_GNUC_UNUSED = 0;
G_STMT_START {
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QFILE (arg0)) {
		hg_file_t *f = HG_VM_LOCK (vm, arg0, error);

		if (!f) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		q = HG_QBOOL (!hg_file_is_closed(f));

		HG_VM_UNLOCK (vm, arg0);

		hg_stack_drop(ostack, error);

		STACK_PUSH (ostack, q);

		__n = 1 - 1;
	} else if (HG_IS_QSTRING (arg0)) {
		gchar *filename;
		struct stat st;

		filename = hg_vm_string_get_cstr(vm, arg0, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}

		hg_stack_drop(ostack, error);

		if (lstat(filename, &st) == -1) {
			STACK_PUSH (ostack, HG_QBOOL (FALSE));

			__n = 1 - 1;
		} else {
			STACK_PUSH (ostack, HG_QINT (st.st_blocks));
			STACK_PUSH (ostack, HG_QINT (st.st_size));
			STACK_PUSH (ostack, HG_QINT (st.st_atime));
			STACK_PUSH (ostack, HG_QINT (st.st_mtime));
			STACK_PUSH (ostack, HG_QBOOL (TRUE));

			__n = 5 - 1;
		}
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__n, 0, 0);
DEFUNC_OPER_END

/* - stop - */
DEFUNC_OPER (stop)
gssize __n G_GNUC_UNUSED = 0;
G_STMT_START {
	hg_quark_t q;
	gsize edepth = hg_stack_depth(estack), i;

	for (i = 0; i < edepth; i++) {
		q = hg_stack_index(estack, i, error);
		if (HG_IS_QOPER (q) &&
		    HG_OPER (q) == HG_enc_protected_stopped_continue)
			break;
	}
	if (i == edepth) {
		g_warning("No /stopped operator found.");
		q = hg_stack_pop(estack, error);
		STACK_PUSH (estack, HG_QOPER (HG_enc_private_abort));
		STACK_PUSH (estack, q);
		__n++;
	} else {
		if (!hg_vm_dict_add(vm, vm->qerror,
				    HG_QNAME (vm->name, ".stopped"),
				    HG_QBOOL (TRUE),
				    FALSE, error)) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
		for (; i > 1; i--) {
			__n--;
			hg_stack_drop(estack, error);
		}
	}
	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, __n, 0);
DEFUNC_OPER_END

/* <any> stopped <bool> */
DEFUNC_OPER (stopped)
G_STMT_START {
	hg_quark_t arg0, q;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!hg_quark_is_simple_object(arg0) &&
	    !HG_IS_QOPER (arg0) &&
	    !hg_vm_quark_is_readable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	if (hg_vm_quark_is_executable(vm, &arg0)) {
		q = hg_vm_dict_lookup(vm, vm->qsystemdict,
				      HG_QNAME (vm->name, "%stopped_continue"),
				      FALSE, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
		if (q == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_undefined);
			return FALSE;
		}

		STACK_PUSH (estack, q);
		STACK_PUSH (estack, arg0);

		hg_stack_roll(estack, 3, -1, error);

		hg_stack_drop(ostack, error);
	} else {
		hg_stack_drop(ostack, error);

		STACK_PUSH (ostack, HG_QBOOL (FALSE));
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 2, 0);
DEFUNC_OPER_END

/* <int> string <string> */
DEFUNC_OPER (string)
G_STMT_START {
	hg_quark_t arg0, q;
	gssize len;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	len = HG_INT (arg0);
	if (len < 0 || len > 65535) {
		hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		return FALSE;
	}
	q = hg_string_new(hg_vm_get_mem(vm), len, NULL);
	if (q == Qnil) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}

	hg_stack_drop(ostack, error);

	STACK_PUSH (ostack, q);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (stringwidth);

/* - stroke - */
DEFUNC_OPER (stroke)
G_STMT_START {
	hg_quark_t qg = hg_vm_get_gstate(vm);
	hg_gstate_t *gstate = HG_VM_LOCK (vm, qg, error);

	if (gstate == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	if (!hg_device_stroke(vm->device, gstate, error)) {
		if (error && *error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
		} else {
			hg_vm_set_error(vm, qself, HG_VM_e_limitcheck);
		}
		goto finalize;
	}
	retval = TRUE;
  finalize:
	HG_VM_UNLOCK (vm, qg);
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (strokepath);

/* <num1> <num2> sub <diff> */
DEFUNC_OPER (sub)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;
	gdouble d1, d2;
	gboolean is_int = TRUE;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QINT (arg0)) {
		d1 = HG_INT (arg0);
	} else if (HG_IS_QREAL (arg0)) {
		d1 = HG_REAL (arg0);
		is_int = FALSE;
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		d2 = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		d2 = HG_REAL (arg1);
		is_int = FALSE;
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	if (is_int) {
		*ret = HG_QINT ((gint32)(d1 - d2));
	} else {
		*ret = HG_QREAL (d1 - d2);
	}
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* <file> token <any> true
 *              false
 * <string> token <post> <any> true
 *                false
 */
DEFUNC_OPER (token)
gint __n G_GNUC_UNUSED = 0;
G_STMT_START {
	hg_quark_t arg0, qt;
	gboolean is_proceeded = FALSE;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QFILE (arg0)) {
		if (!hg_vm_quark_is_readable(vm, &arg0)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}

		hg_quark_set_executable(&arg0, TRUE);
		STACK_PUSH (estack, arg0);

		hg_vm_stepi(vm, &is_proceeded);
		if (hg_vm_has_error(vm)) {
			hg_stack_drop(ostack, error); /* the object where the error happened */
			/* We want to notify the place
			 * where the error happened as in token operator
			 * but not in --file--
			 */
			STACK_PUSH (ostack, qself);

			return FALSE;
		} else {
			hg_quark_t q;

			if (is_proceeded) {
				/* EOF may detected */
				hg_stack_drop(ostack, error); /* the file object given for token operator */

				STACK_PUSH (ostack, HG_QBOOL (FALSE));

				__n = 0;
			} else {
				qt = hg_stack_index(estack, 0, error);
				if (HG_IS_QFILE (qt)) {
					/* scanner may detected the procedure.
					 * in this case, the scanned object doesn't appear in the exec stack.
					 */
					q = hg_stack_pop(ostack, error);
				} else {
					q = hg_stack_pop(estack, error);
				}

				hg_stack_drop(estack, error); /* the file object executed above */
				hg_stack_drop(ostack, error); /* the file object given for token operator */

				STACK_PUSH (ostack, q);
				STACK_PUSH (ostack, HG_QBOOL (TRUE));

				__n = 1;
			}
		}

		retval = TRUE;
	} else if (HG_IS_QSTRING (arg0)) {
		hg_string_t *s;
		hg_quark_t qf;

		if (!hg_vm_quark_is_readable(vm, &arg0)) {
			hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
			return FALSE;
		}
		s = HG_VM_LOCK (vm, arg0, error);
		if (!s) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}
		qf = hg_file_new_with_string(hg_vm_get_mem(vm),
					     "%stoken",
					     HG_FILE_IO_MODE_READ,
					     s,
					     NULL,
					     error,
					     NULL);
		HG_VM_UNLOCK (vm, arg0);
		if (qf == Qnil) {
			hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
			return FALSE;
		}

		hg_quark_set_executable(&qf, TRUE);
		STACK_PUSH (estack, qf);

		hg_vm_stepi(vm, &is_proceeded);
		if (hg_vm_has_error(vm)) {
			hg_stack_drop(ostack, error); /* the object where the error happened */
			/* We want to notify the place
			 * where the error happened as in token operator
			 * but not in --file--
			 */
			STACK_PUSH (ostack, qself);

			return FALSE;
		} else {
			hg_quark_t q, qs;
			hg_file_t *f;
			gssize pos;
			guint length;

			if (is_proceeded) {
				/* EOF may detected */
				hg_stack_drop(ostack, error); /* the string object given for token operator */

				STACK_PUSH (ostack, HG_QBOOL (FALSE));

				retval = TRUE;
				__n = 0;
			} else {
				qt = hg_stack_index(estack, 0, error);
				if (HG_IS_QFILE (qt)) {
					/* scanner may detected the procedure.
					 * in this case, the scanned object doesn't appear in the exec stack.
					 */
					q = hg_stack_pop(ostack, error);
				} else {
					q = hg_stack_pop(estack, error);
				}

				f = HG_VM_LOCK (vm, qf, error);
				if (!f) {
					hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
					return FALSE;
				}

				pos = hg_file_seek(f, 0, HG_FILE_POS_CURRENT, error);
				HG_VM_UNLOCK (vm, qf);

				/* safely drop the file object from the stack here.
				 * shouldn't do that during someone is referring it because it may be destroyed by GC.
				 */
				hg_stack_drop(estack, error); /* the file object executed above */

				s = HG_VM_LOCK (vm, arg0, error);
				if (!s) {
					hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
					return FALSE;
				}

				length = hg_string_maxlength(s) - 1;
				qs = hg_string_make_substring(s, pos, length, NULL, error);
				if (qs == Qnil) {
					hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
					goto s_finalize;
				}
				hg_stack_drop(ostack, error); /* the string object given for token operator */

				STACK_PUSH (ostack, qs);
				STACK_PUSH (ostack, q);
				STACK_PUSH (ostack, HG_QBOOL (TRUE));

				retval = TRUE;
				__n = 2;
			  s_finalize:
				HG_VM_UNLOCK (vm, arg0);
			}
		}
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
} G_STMT_END;
VALIDATE_STACK_SIZE (__n, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (transform);

/* <tx> <ty> translate -
 * <tx> <ty> <matrix> translate <matrix>
 */
DEFUNC_OPER (translate)
G_STMT_START {
	hg_quark_t arg0, arg1, arg2 = Qnil, qg = hg_vm_get_gstate(vm);
	gdouble tx, ty;
	hg_matrix_t ctm;
	hg_array_t *a;
	hg_gstate_t *gstate;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (HG_IS_QARRAY (arg1)) {
		CHECK_STACK (ostack, 3);

		arg2 = arg1;
		arg1 = arg0;
		arg0 = hg_stack_index(ostack, 2, error);
		a = HG_VM_LOCK (vm, arg2, error);
		if (!a)
			return FALSE;
		if (hg_array_maxlength(a) != 6) {
			hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
			HG_VM_UNLOCK (vm, arg2);
			return FALSE;
		}
		HG_VM_UNLOCK (vm, arg2);
		ctm.mtx.xx = 1;
		ctm.mtx.xy = 0;
		ctm.mtx.yx = 0;
		ctm.mtx.yy = 1;
		ctm.mtx.x0 = 0;
		ctm.mtx.y0 = 0;
	} else {
		gstate = HG_VM_LOCK (vm, qg, error);
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
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (HG_IS_QINT (arg1)) {
		ty = HG_INT (arg1);
	} else if (HG_IS_QREAL (arg1)) {
		ty = HG_REAL (arg1);
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	hg_matrix_translate(&ctm, tx, ty);

	if (arg2 == Qnil) {
		gstate = HG_VM_LOCK (vm, qg, error);
		if (!gstate)
			return FALSE;
		hg_gstate_set_ctm(gstate, &ctm);
		HG_VM_UNLOCK (vm, qg);
	} else {
		a = HG_VM_LOCK (vm, arg2, error);
		if (!a)
			return FALSE;
		hg_array_from_matrix(a, &ctm);
		HG_VM_UNLOCK (vm, arg2);

		hg_stack_roll(ostack, 3, 1, error);
	}

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <any> type <name> */
DEFUNC_OPER (type)
G_STMT_START {
	hg_quark_t arg0, q, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	q = HG_QNAME (vm->name, hg_quark_get_type_name(arg0));
	hg_vm_quark_set_executable(vm, &q, TRUE); /* ??? */

	*ret = q;

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

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
G_STMT_START {
	gint32 i = hg_vm_get_current_time(vm);

	STACK_PUSH (ostack, HG_QINT (i));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (ustroke);
DEFUNC_UNIMPLEMENTED_OPER (ustrokepath);
DEFUNC_UNIMPLEMENTED_OPER (viewclip);
DEFUNC_UNIMPLEMENTED_OPER (viewclippath);

/* <int> vmreclaim - */
DEFUNC_OPER (vmreclaim)
G_STMT_START {
	hg_quark_t arg0;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QINT (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	switch (HG_INT (arg0)) {
	    case -2:
		    hg_mem_enable_garbage_collector(vm->mem[HG_VM_MEM_GLOBAL], FALSE);
	    case -1:
		    hg_mem_enable_garbage_collector(vm->mem[HG_VM_MEM_LOCAL], FALSE);
		    break;
	    case 0:
		    /* XXX: no multi-context support yet */
		    hg_mem_enable_garbage_collector(vm->mem[HG_VM_MEM_GLOBAL], FALSE);
		    hg_mem_enable_garbage_collector(vm->mem[HG_VM_MEM_LOCAL], FALSE);
		    break;
	    case 2:
		    hg_mem_collect_garbage(vm->mem[HG_VM_MEM_GLOBAL]);
	    case 1:
		    hg_mem_collect_garbage(vm->mem[HG_VM_MEM_LOCAL]);
		    break;
	    default:
		    hg_vm_set_error(vm, qself, HG_VM_e_rangecheck);
		    return FALSE;
	}

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

/* - vmstatus <level> <used> <maximum> */
DEFUNC_OPER (vmstatus)
G_STMT_START {
	hg_mem_t *mem = hg_vm_get_mem(vm);

	STACK_PUSH (ostack, HG_QINT (vm->vm_state.n_save_objects));
	STACK_PUSH (ostack, HG_QINT (hg_mem_get_used_size(mem)));
	STACK_PUSH (ostack, HG_QINT (hg_mem_get_total_size(mem) - hg_mem_get_used_size(mem)));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (3, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (wait);

/* <array> wcheck <bool>
 * <packedarray> wcheck <false>
 * <dict> wcheck <bool>
 * <file> wcheck <bool>
 * <string> wcheck <bool>
 */
DEFUNC_OPER (wcheck)
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;
	if (!HG_IS_QARRAY (arg0) &&
	    !HG_IS_QDICT (arg0) &&
	    !HG_IS_QFILE (arg0) &&
	    !HG_IS_QSTRING (arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	*ret = HG_QBOOL (hg_vm_quark_is_writable(vm, &arg0));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <key> where <dict> <true>
 * <key> where <false>
 */
DEFUNC_OPER (where)
gboolean __flag G_GNUC_UNUSED = FALSE;
G_STMT_START {
	hg_quark_t arg0, q = Qnil, qq = Qnil;
	gsize ddepth = hg_stack_depth(dstack), i;

	CHECK_STACK (ostack, 1);

	arg0 = hg_stack_index(ostack, 0, error);
	for (i = 0; i < ddepth; i++) {
		q = hg_stack_index(dstack, i, error);
		qq = hg_vm_dict_lookup(vm, q, arg0, TRUE, error);
		if (*error) {
			hg_vm_set_error_from_gerror(vm, qself, *error);
			return FALSE;
		}
		if (qq != Qnil)
			break;
	}

	hg_stack_drop(ostack, error);

	if (qq == Qnil) {
		STACK_PUSH (ostack, HG_QBOOL (FALSE));
	} else {
		__flag = TRUE;
		STACK_PUSH (ostack, q);
		STACK_PUSH (ostack, HG_QBOOL (TRUE));
	}

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (__flag ? 1 : 0, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (widthshow);

/* <file> <int> write - */
DEFUNC_OPER (write)
G_STMT_START {
	hg_quark_t arg0, arg1;
	hg_file_t *f;
	gchar c[2];

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);

	if (!HG_IS_QFILE (arg0) ||
	    !HG_IS_QINT (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_writable(vm, &arg0)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);
	if (!f) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		return FALSE;
	}
	c[0] = HG_INT (arg1) % 256;
	retval = (hg_file_write(f, c, sizeof (gchar), 1, error) > 0);
	if (!retval) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
	}
	HG_VM_UNLOCK (vm, arg0);

	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

/* <file> <string> writehexstring - */
DEFUNC_OPER (writehexstring)
G_STMT_START {
	hg_quark_t arg0, arg1;
	hg_file_t *f;
	hg_string_t *s;
	gchar *cstr;
	gsize length, i;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QFILE (arg0) ||
	    !HG_IS_QSTRING (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_writable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);
	s = HG_VM_LOCK (vm, arg1, error);
	if (f == NULL ||
	    s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	cstr = hg_string_get_cstr(s);
	length = hg_string_maxlength(s);
	for (i = 0; i < length; i++) {
		if (hg_file_append_printf(f, "%x", cstr[i] & 0xff) < 0) {
			hg_vm_set_error(vm, qself, HG_VM_e_ioerror);
			goto error;
		}
	}

	retval = TRUE;
	hg_stack_drop(ostack, error);
	hg_stack_drop(ostack, error);
  error:
	HG_VM_UNLOCK (vm, arg0);
	HG_VM_UNLOCK (vm, arg1);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (writeobject);

/* <file> <string> writestring - */
DEFUNC_OPER (writestring)
G_STMT_START {
	hg_quark_t arg0, arg1;
	hg_file_t *f;
	hg_string_t *s;
	gchar *cstr;

	CHECK_STACK (ostack, 2);

	arg0 = hg_stack_index(ostack, 1, error);
	arg1 = hg_stack_index(ostack, 0, error);
	if (!HG_IS_QFILE (arg0) ||
	    !HG_IS_QSTRING (arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}
	if (!hg_vm_quark_is_writable(vm, &arg0) ||
	    !hg_vm_quark_is_readable(vm, &arg1)) {
		hg_vm_set_error(vm, qself, HG_VM_e_invalidaccess);
		return FALSE;
	}
	f = HG_VM_LOCK (vm, arg0, error);
	s = HG_VM_LOCK (vm, arg1, error);
	if (f == NULL ||
	    s == NULL) {
		hg_vm_set_error(vm, qself, HG_VM_e_VMerror);
		goto error;
	}
	cstr = hg_string_get_cstr(s);
	hg_file_write(f, cstr,
		      sizeof (gchar), hg_string_length(s), error);
	if (error && *error) {
		hg_vm_set_error_from_gerror(vm, qself, *error);
	} else {
		retval = TRUE;
		hg_stack_drop(ostack, error);
		hg_stack_drop(ostack, error);
	}
  error:
	HG_VM_UNLOCK (vm, arg0);
	HG_VM_UNLOCK (vm, arg1);
} G_STMT_END;
VALIDATE_STACK_SIZE (-2, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (wtranslation);

/* <any> xcheck <bool> */
DEFUNC_OPER (xcheck)
G_STMT_START {
	hg_quark_t arg0, *ret;

	CHECK_STACK (ostack, 1);

	ret = hg_stack_peek(ostack, 0, error);
	arg0 = *ret;

	*ret = HG_QBOOL (hg_vm_quark_is_executable(vm, &arg0));

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (0, 0, 0);
DEFUNC_OPER_END

/* <bool1> <bool2> xor <bool3>
 * <int1> <int2> xor <int3>
 */
DEFUNC_OPER (xor)
G_STMT_START {
	hg_quark_t arg0, arg1, *ret;

	CHECK_STACK (ostack, 2);

	ret = hg_stack_peek(ostack, 1, error);
	arg0 = *ret;
	arg1 = hg_stack_index(ostack, 0, error);
	if (HG_IS_QBOOL (arg0) &&
	    HG_IS_QBOOL (arg1)) {
		*ret = HG_QBOOL (HG_BOOL (arg0) ^ HG_BOOL (arg1));
	} else if (HG_IS_QINT (arg0) &&
		   HG_IS_QINT (arg1)) {
		*ret = HG_QINT (HG_INT (arg0) ^ HG_INT (arg1));
	} else {
		hg_vm_set_error(vm, qself, HG_VM_e_typecheck);
		return FALSE;
	}

	hg_stack_drop(ostack, error);

	retval = TRUE;
} G_STMT_END;
VALIDATE_STACK_SIZE (-1, 0, 0);
DEFUNC_OPER_END

DEFUNC_UNIMPLEMENTED_OPER (xshow);
DEFUNC_UNIMPLEMENTED_OPER (xyshow);
DEFUNC_UNIMPLEMENTED_OPER (yield);
DEFUNC_UNIMPLEMENTED_OPER (yshow);


#define REG_OPER(_d_,_n_,_o_)						\
	G_STMT_START {							\
		hg_quark_t __o_name__ = hg_name_new_with_encoding((_n_),	\
								  HG_enc_ ## _o_); \
		hg_quark_t __op__ = HG_QOPER (HG_enc_ ## _o_);		\
									\
		if (!hg_dict_add((_d_),					\
				 __o_name__,				\
				 __op__,				\
				 FALSE,					\
				 NULL))					\
			return FALSE;					\
	} G_STMT_END
#define REG_PRIV_OPER(_d_,_n_,_k_,_o_)					\
	G_STMT_START {							\
		hg_quark_t __o_name__ = HG_QNAME ((_n_),#_k_);		\
		hg_quark_t __op__ = HG_QOPER (HG_enc_ ## _o_);		\
									\
		if (!hg_dict_add((_d_),					\
				 __o_name__,				\
				 __op__,				\
				 FALSE,					\
				 NULL))					\
			return FALSE;					\
	} G_STMT_END
#define REG_VALUE(_d_,_n_,_k_,_v_)				\
	G_STMT_START {						\
		hg_quark_t __o_name__ = HG_QNAME ((_n_),#_k_);	\
		hg_quark_t __v__ = (_v_);			\
								\
		hg_vm_quark_set_readable(vm, &__v__, TRUE);	\
		if (!hg_dict_add((_d_),				\
				 __o_name__,			\
				 __v__,				\
				 FALSE,				\
				 NULL))				\
			return FALSE;				\
	} G_STMT_END

static gboolean
_hg_operator_level1_register(hg_vm_t   *vm,
			     hg_dict_t *dict,
			     hg_name_t *name)
{
	REG_VALUE (dict, name, false, HG_QBOOL (FALSE));
	REG_VALUE (dict, name, mark, HG_QMARK);
	REG_VALUE (dict, name, null, HG_QNULL);
	REG_VALUE (dict, name, true, HG_QBOOL (TRUE));
	REG_VALUE (dict, name, [, HG_QMARK);
	REG_VALUE (dict, name, ], HG_QEVALNAME (name, "%arraytomark"));

	REG_PRIV_OPER (dict, name, .abort, private_abort);
	REG_PRIV_OPER (dict, name, .applyparams, private_applyparams);
	REG_PRIV_OPER (dict, name, .clearerror, private_clearerror);
	REG_PRIV_OPER (dict, name, .callbeginpage, private_callbeginpage);
	REG_PRIV_OPER (dict, name, .callendpage, private_callendpage);
	REG_PRIV_OPER (dict, name, .callinstall, private_callinstall);
	REG_PRIV_OPER (dict, name, .currentpagedevice, private_currentpagedevice);
	REG_PRIV_OPER (dict, name, .dicttomark, protected_dicttomark);
	REG_PRIV_OPER (dict, name, .exit, private_exit);
	REG_PRIV_OPER (dict, name, .findlibfile, private_findlibfile);
	REG_PRIV_OPER (dict, name, .forceput, private_forceput);
	REG_PRIV_OPER (dict, name, .hgrevision, private_hgrevision);
	REG_PRIV_OPER (dict, name, .odef, private_odef);
	REG_PRIV_OPER (dict, name, .product, private_product);
	REG_PRIV_OPER (dict, name, .quit, private_quit);
	REG_PRIV_OPER (dict, name, .revision, private_revision);
	REG_PRIV_OPER (dict, name, .setglobal, private_setglobal);
	REG_PRIV_OPER (dict, name, .stringcvs, private_stringcvs);
	REG_PRIV_OPER (dict, name, .undef, private_undef);
	REG_PRIV_OPER (dict, name, .write==only, private_write_eqeq_only);

	REG_PRIV_OPER (dict, name, %arraytomark, protected_arraytomark);
	REG_PRIV_OPER (dict, name, %for_yield_int_continue, protected_for_yield_int_continue);
	REG_PRIV_OPER (dict, name, %for_yield_real_continue, protected_for_yield_real_continue);
	REG_PRIV_OPER (dict, name, %forall_array_continue, protected_forall_array_continue);
	REG_PRIV_OPER (dict, name, %forall_dict_continue, protected_forall_dict_continue);
	REG_PRIV_OPER (dict, name, %forall_string_continue, protected_forall_string_continue);
	REG_PRIV_OPER (dict, name, %loop_continue, protected_loop_continue);
	REG_PRIV_OPER (dict, name, %repeat_continue, protected_repeat_continue);
	REG_PRIV_OPER (dict, name, %stopped_continue, protected_stopped_continue);
	REG_PRIV_OPER (dict, name, eexec, eexec);

	REG_OPER (dict, name, abs);
	REG_OPER (dict, name, add);
	REG_OPER (dict, name, aload);
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
	REG_OPER (dict, name, flattenpath);
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
	REG_OPER (dict, name, loop);
	REG_OPER (dict, name, lt);
	REG_OPER (dict, name, makefont);
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
	REG_OPER (dict, name, type);
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
	REG_OPER (dict, name, initclip);

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
	REG_OPER (dict, name, rand);
	REG_OPER (dict, name, rcheck);
	REG_OPER (dict, name, readonly);

//	REG_OPER (dict, name, renderbands); /* ??? */
	REG_OPER (dict, name, resetfile);
	REG_OPER (dict, name, reversepath);
	REG_OPER (dict, name, rrand);
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

	REG_OPER (dict, name, usertime);
	REG_OPER (dict, name, vmstatus);
//	REG_OPER (dict, name, wait); /* ??? */
	REG_OPER (dict, name, wcheck);

	REG_OPER (dict, name, xcheck);
//	REG_OPER (dict, name, yield); /* ??? */
	REG_OPER (dict, name, cleardictstack);

	REG_OPER (dict, name, sym_begin_dict_mark);

	REG_OPER (dict, name, sym_end_dict_mark);


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
_hg_operator_level2_register(hg_vm_t   *vm,
			     hg_dict_t *dict,
			     hg_name_t *name)
{
	REG_VALUE (dict, name, <<, HG_QMARK);
	REG_VALUE (dict, name, >>, HG_QEVALNAME (name, "%dicttomark"));

	REG_PRIV_OPER (dict, name, %dicttomark, protected_dicttomark);

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
_hg_operator_level3_register(hg_vm_t   *vm,
			     hg_dict_t *dict,
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
		__hg_operator_func_table[HG_enc_ ## _n_] = OPER_FUNC_NAME (_n_); \
	} G_STMT_END
#define DECL_PRIV_OPER(_on_,_n_)						\
	G_STMT_START {							\
		__hg_operator_name_table[HG_enc_ ## _n_] = g_strdup("--" #_on_ "--"); \
		if (__hg_operator_name_table[HG_enc_ ## _n_] == NULL)	\
			return FALSE;					\
		__hg_operator_func_table[HG_enc_ ## _n_] = OPER_FUNC_NAME (_n_); \
	} G_STMT_END

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
#define UNDECL_OPER(_n_)					\
	G_STMT_START {						\
		g_free(__hg_operator_name_table[HG_enc_ ## _n_]);	\
		__hg_operator_name_table[HG_enc_ ## _n_] = NULL;		\
		__hg_operator_func_table[HG_enc_ ## _n_] = NULL;		\
	} G_STMT_END

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
hg_operator_add_dynamic(hg_name_t          *name,
			const gchar        *string,
			hg_operator_func_t  func)
{
	hg_quark_t n = HG_QNAME (name, string);

	if (hg_quark_get_value(n) < HG_enc_builtin_HIEROGLYPH_END)
		return Qnil;

	__hg_operator_name_table[hg_quark_get_value(n)] = g_strdup_printf("--%s--", string);
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
hg_operator_remove_dynamic(guint encoding)
{
	hg_return_if_fail (encoding >= HG_enc_builtin_HIEROGLYPH_END);

	g_free(__hg_operator_name_table[encoding]);
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
gboolean
hg_operator_invoke(hg_quark_t   qoper,
		   hg_vm_t     *vm,
		   GError     **error)
{
	hg_quark_t q;
	GError *err = NULL;
	gboolean retval = TRUE;

	hg_return_val_with_gerror_if_fail (HG_IS_QOPER (qoper), FALSE, error, HG_VM_e_VMerror);
	hg_return_val_with_gerror_if_fail (vm != NULL, FALSE, error, HG_VM_e_VMerror);
	hg_return_val_with_gerror_if_fail ((q = hg_quark_get_value(qoper)) < HG_enc_END, FALSE, error, HG_VM_e_VMerror);

	if (__hg_operator_func_table[q] == NULL) {
		if (__hg_operator_name_table[q] == NULL) {
			g_set_error(&err, HG_ERROR, HG_VM_e_VMerror,
				    "Invalid operators - quark: %lx", qoper);
		} else {
			g_set_error(&err, HG_ERROR, HG_VM_e_VMerror,
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
 * @vm:
 * @dict:
 * @name:
 * @lang_level:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_operator_register(hg_vm_t           *vm,
		     hg_dict_t         *dict,
		     hg_name_t         *name,
		     hg_vm_langlevel_t  lang_level)
{
	hg_return_val_if_fail (dict != NULL, FALSE);
	hg_return_val_if_fail (lang_level < HG_LANG_LEVEL_END, FALSE);

	/* register level 1 built-in operators */
	if (!_hg_operator_level1_register(vm, dict, name))
		return FALSE;

	/* register level 2 built-in operators */
	if (lang_level >= HG_LANG_LEVEL_2 &&
	    !_hg_operator_level2_register(vm, dict, name))
		return FALSE;

	/* register level 3 built-in operators */
	if (lang_level >= HG_LANG_LEVEL_3 &&
	    !_hg_operator_level3_register(vm, dict, name))
		return FALSE;

	return TRUE;
}
