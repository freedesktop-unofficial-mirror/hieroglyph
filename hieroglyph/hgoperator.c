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
#include "hgquark.h"
#include "hgoperator.h"


static hg_operator_func_t __hg_operator_func_table[HG_enc_END];
static gchar *__hg_operator_name_table[HG_enc_END];
static gboolean __hg_operator_is_initialized = FALSE;

/*< private >*/
#define DEFUNC_OPER(_n_)						\
	static gboolean							\
	_hg_operator_real_ ## _n_(hg_vm_t  *vm,				\
				  GError  **error)			\
	{								\
		hg_stack_t *ostack G_GNUC_UNUSED = vm->stacks[HG_VM_STACK_OSTACK]; \
		hg_stack_t *estack G_GNUC_UNUSED = vm->stacks[HG_VM_STACK_ESTACK]; \
		hg_stack_t *dstack G_GNUC_UNUSED = vm->stacks[HG_VM_STACK_DSTACK]; \
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
		return FALSE;						\
	}

DEFUNC_UNIMPLEMENTED_OPER (abs);
DEFUNC_UNIMPLEMENTED_OPER (add);
DEFUNC_UNIMPLEMENTED_OPER (aload);
DEFUNC_UNIMPLEMENTED_OPER (anchorsearch);
DEFUNC_UNIMPLEMENTED_OPER (and);
DEFUNC_UNIMPLEMENTED_OPER (arc);
DEFUNC_UNIMPLEMENTED_OPER (arcn);
DEFUNC_UNIMPLEMENTED_OPER (arct);
DEFUNC_UNIMPLEMENTED_OPER (arcto);
DEFUNC_UNIMPLEMENTED_OPER (array);
DEFUNC_UNIMPLEMENTED_OPER (ashow);
DEFUNC_UNIMPLEMENTED_OPER (astore);
DEFUNC_UNIMPLEMENTED_OPER (awidthshow);
DEFUNC_UNIMPLEMENTED_OPER (begin);
DEFUNC_UNIMPLEMENTED_OPER (bind);
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
DEFUNC_UNIMPLEMENTED_OPER (copy);
DEFUNC_UNIMPLEMENTED_OPER (count);
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
DEFUNC_UNIMPLEMENTED_OPER (def);
DEFUNC_UNIMPLEMENTED_OPER (defineusername);
DEFUNC_UNIMPLEMENTED_OPER (dict);
DEFUNC_UNIMPLEMENTED_OPER (div);
DEFUNC_UNIMPLEMENTED_OPER (dtransform);
DEFUNC_UNIMPLEMENTED_OPER (dup);
DEFUNC_UNIMPLEMENTED_OPER (end);
DEFUNC_UNIMPLEMENTED_OPER (eoclip);
DEFUNC_UNIMPLEMENTED_OPER (eofill);
DEFUNC_UNIMPLEMENTED_OPER (eoviewclip);
DEFUNC_UNIMPLEMENTED_OPER (eq);
DEFUNC_UNIMPLEMENTED_OPER (exch);
DEFUNC_UNIMPLEMENTED_OPER (exec);
DEFUNC_UNIMPLEMENTED_OPER (exit);
DEFUNC_UNIMPLEMENTED_OPER (file);
DEFUNC_UNIMPLEMENTED_OPER (fill);
DEFUNC_UNIMPLEMENTED_OPER (findfont);
DEFUNC_UNIMPLEMENTED_OPER (flattenpath);
DEFUNC_UNIMPLEMENTED_OPER (floor);
DEFUNC_UNIMPLEMENTED_OPER (flush);
DEFUNC_UNIMPLEMENTED_OPER (flushfile);
DEFUNC_UNIMPLEMENTED_OPER (for);
DEFUNC_UNIMPLEMENTED_OPER (forall);
DEFUNC_UNIMPLEMENTED_OPER (ge);
DEFUNC_UNIMPLEMENTED_OPER (get);
DEFUNC_UNIMPLEMENTED_OPER (getinterval);
DEFUNC_UNIMPLEMENTED_OPER (grestore);
DEFUNC_UNIMPLEMENTED_OPER (gsave);
DEFUNC_UNIMPLEMENTED_OPER (gstate);
DEFUNC_UNIMPLEMENTED_OPER (gt);
DEFUNC_UNIMPLEMENTED_OPER (identmatrix);
DEFUNC_UNIMPLEMENTED_OPER (idiv);
DEFUNC_UNIMPLEMENTED_OPER (idtransform);
DEFUNC_UNIMPLEMENTED_OPER (if);
DEFUNC_UNIMPLEMENTED_OPER (ifelse);
DEFUNC_UNIMPLEMENTED_OPER (image);
DEFUNC_UNIMPLEMENTED_OPER (imagemask);
DEFUNC_UNIMPLEMENTED_OPER (index);
DEFUNC_UNIMPLEMENTED_OPER (ineofill);
DEFUNC_UNIMPLEMENTED_OPER (infill);
DEFUNC_UNIMPLEMENTED_OPER (initviewclip);
DEFUNC_UNIMPLEMENTED_OPER (inueofill);
DEFUNC_UNIMPLEMENTED_OPER (inufill);
DEFUNC_UNIMPLEMENTED_OPER (invertmatrix);
DEFUNC_UNIMPLEMENTED_OPER (itransform);
DEFUNC_UNIMPLEMENTED_OPER (known);
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
DEFUNC_UNIMPLEMENTED_OPER (not);
DEFUNC_UNIMPLEMENTED_OPER (or);
DEFUNC_UNIMPLEMENTED_OPER (pathbbox);
DEFUNC_UNIMPLEMENTED_OPER (pathforall);
DEFUNC_UNIMPLEMENTED_OPER (pop);
DEFUNC_UNIMPLEMENTED_OPER (print);
DEFUNC_UNIMPLEMENTED_OPER (printobject);
DEFUNC_UNIMPLEMENTED_OPER (put);
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
DEFUNC_UNIMPLEMENTED_OPER (roll);
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
DEFUNC_UNIMPLEMENTED_OPER (systemdict);
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
DEFUNC_UNIMPLEMENTED_OPER (globaldict);
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
DEFUNC_UNIMPLEMENTED_OPER (errordict);
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
DEFUNC_UNIMPLEMENTED_OPER (mark);
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

#undef DEFUNC_OPER
#undef DEFUNC_OPER_END
#undef DEFUNC_UNIMPLEMENTED_OPER

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
#define DECL_OPER2(_on_,_n_)						\
	G_STMT_START {							\
		__hg_operator_name_table[HG_enc_ ## _n_] = g_strdup("--" #_on_ "--"); \
		if (__hg_operator_name_table[HG_enc_ ## _n_] == NULL)	\
			return FALSE;					\
		__hg_operator_func_table[HG_enc_ ## _n_] = _hg_operator_real_ ## _n_; \
	} G_STMT_END

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
	DECL_OPER (systemdict);
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
	DECL_OPER (globaldict);
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
	DECL_OPER (errordict);
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
	DECL_OPER (mark);
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
	UNDECL_OPER (errordict);
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
