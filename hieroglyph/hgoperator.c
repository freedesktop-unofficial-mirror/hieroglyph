/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgoperator.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gstrfuncs.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgobject.h>
#include <hieroglyph/vm.h>
#include "hgoperator.h"
#include "hgoperator-private.h"


static gchar *__hg_system_encoding_names[HG_enc_END];
static gboolean __hg_operator_initialized = FALSE;


/*
 * private functions
 */
HG_DEFUNC_UNIMPLEMENTED_OPER (private_arraytomark);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_dicttomark);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_for_pos_int_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_for_pos_real_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_forall_array_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_forall_dict_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_forall_string_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_loop_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_repeat_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_stopped_continue);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_findfont);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_definefont);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_stringcvs);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_undefinefont);
HG_DEFUNC_UNIMPLEMENTED_OPER (private_write_eqeq_only);
HG_DEFUNC_UNIMPLEMENTED_OPER (abs);
HG_DEFUNC_UNIMPLEMENTED_OPER (add);
HG_DEFUNC_UNIMPLEMENTED_OPER (aload);
HG_DEFUNC_UNIMPLEMENTED_OPER (and);
HG_DEFUNC_UNIMPLEMENTED_OPER (arc);
HG_DEFUNC_UNIMPLEMENTED_OPER (arcn);
HG_DEFUNC_UNIMPLEMENTED_OPER (arcto);
HG_DEFUNC_UNIMPLEMENTED_OPER (array);
HG_DEFUNC_UNIMPLEMENTED_OPER (ashow);
HG_DEFUNC_UNIMPLEMENTED_OPER (astore);
HG_DEFUNC_UNIMPLEMENTED_OPER (atan);
HG_DEFUNC_UNIMPLEMENTED_OPER (awidthshow);
HG_DEFUNC_UNIMPLEMENTED_OPER (begin);
HG_DEFUNC_UNIMPLEMENTED_OPER (bind);
HG_DEFUNC_UNIMPLEMENTED_OPER (bitshift);
HG_DEFUNC_UNIMPLEMENTED_OPER (bytesavailable);
HG_DEFUNC_UNIMPLEMENTED_OPER (cachestatus);
HG_DEFUNC_UNIMPLEMENTED_OPER (ceiling);
HG_DEFUNC_UNIMPLEMENTED_OPER (charpath);
HG_DEFUNC_UNIMPLEMENTED_OPER (clear);
HG_DEFUNC_UNIMPLEMENTED_OPER (cleardictstack);
HG_DEFUNC_UNIMPLEMENTED_OPER (cleartomark);
HG_DEFUNC_UNIMPLEMENTED_OPER (clip);
HG_DEFUNC_UNIMPLEMENTED_OPER (clippath);
HG_DEFUNC_UNIMPLEMENTED_OPER (closefile);
HG_DEFUNC_UNIMPLEMENTED_OPER (closepath);
HG_DEFUNC_UNIMPLEMENTED_OPER (concat);
HG_DEFUNC_UNIMPLEMENTED_OPER (concatmatrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (copy);
HG_DEFUNC_UNIMPLEMENTED_OPER (copypage);
HG_DEFUNC_UNIMPLEMENTED_OPER (cos);
HG_DEFUNC_UNIMPLEMENTED_OPER (count);
HG_DEFUNC_UNIMPLEMENTED_OPER (countdictstack);
HG_DEFUNC_UNIMPLEMENTED_OPER (countexecstack);
HG_DEFUNC_UNIMPLEMENTED_OPER (counttomark);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentdash);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentdict);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentfile);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentflat);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentfont);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentgray);
HG_DEFUNC_UNIMPLEMENTED_OPER (currenthsbcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentlinecap);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentlinejoin);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentlinewidth);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentmatrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentmiterlimit);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentpoint);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentrgbcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentscreen);
HG_DEFUNC_UNIMPLEMENTED_OPER (currenttransfer);
HG_DEFUNC_UNIMPLEMENTED_OPER (curveto);
HG_DEFUNC_UNIMPLEMENTED_OPER (cvi);
HG_DEFUNC_UNIMPLEMENTED_OPER (cvlit);
HG_DEFUNC_UNIMPLEMENTED_OPER (cvn);
HG_DEFUNC_UNIMPLEMENTED_OPER (cvr);
HG_DEFUNC_UNIMPLEMENTED_OPER (cvrs);
HG_DEFUNC_UNIMPLEMENTED_OPER (cvx);
HG_DEFUNC_UNIMPLEMENTED_OPER (def);
HG_DEFUNC_UNIMPLEMENTED_OPER (defaultmatrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (dict);
HG_DEFUNC_UNIMPLEMENTED_OPER (dictstack);
HG_DEFUNC_UNIMPLEMENTED_OPER (div);
HG_DEFUNC_UNIMPLEMENTED_OPER (dtransform);
HG_DEFUNC_UNIMPLEMENTED_OPER (dup);
HG_DEFUNC_UNIMPLEMENTED_OPER (echo);
HG_DEFUNC_UNIMPLEMENTED_OPER (eexec);
HG_DEFUNC_UNIMPLEMENTED_OPER (end);
HG_DEFUNC_UNIMPLEMENTED_OPER (eoclip);
HG_DEFUNC_UNIMPLEMENTED_OPER (eofill);
HG_DEFUNC_UNIMPLEMENTED_OPER (eq);
HG_DEFUNC_UNIMPLEMENTED_OPER (erasepage);
HG_DEFUNC_UNIMPLEMENTED_OPER (exch);
HG_DEFUNC_UNIMPLEMENTED_OPER (exec);
HG_DEFUNC_UNIMPLEMENTED_OPER (execstack);
HG_DEFUNC_UNIMPLEMENTED_OPER (executeonly);
HG_DEFUNC_UNIMPLEMENTED_OPER (exit);
HG_DEFUNC_UNIMPLEMENTED_OPER (exp);
HG_DEFUNC_UNIMPLEMENTED_OPER (file);
HG_DEFUNC_UNIMPLEMENTED_OPER (fill);
HG_DEFUNC_UNIMPLEMENTED_OPER (flattenpath);
HG_DEFUNC_UNIMPLEMENTED_OPER (flush);
HG_DEFUNC_UNIMPLEMENTED_OPER (flushfile);
HG_DEFUNC_UNIMPLEMENTED_OPER (FontDirectory);
HG_DEFUNC_UNIMPLEMENTED_OPER (for);
HG_DEFUNC_UNIMPLEMENTED_OPER (forall);
HG_DEFUNC_UNIMPLEMENTED_OPER (ge);
HG_DEFUNC_UNIMPLEMENTED_OPER (get);
HG_DEFUNC_UNIMPLEMENTED_OPER (getinterval);
HG_DEFUNC_UNIMPLEMENTED_OPER (grestore);
HG_DEFUNC_UNIMPLEMENTED_OPER (grestoreall);
HG_DEFUNC_UNIMPLEMENTED_OPER (gsave);
HG_DEFUNC_UNIMPLEMENTED_OPER (gt);
HG_DEFUNC_UNIMPLEMENTED_OPER (identmatrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (idiv);
HG_DEFUNC_UNIMPLEMENTED_OPER (idtransform);
HG_DEFUNC_UNIMPLEMENTED_OPER (if);
HG_DEFUNC_UNIMPLEMENTED_OPER (ifelse);
HG_DEFUNC_UNIMPLEMENTED_OPER (image);
HG_DEFUNC_UNIMPLEMENTED_OPER (imagemask);
HG_DEFUNC_UNIMPLEMENTED_OPER (index);
HG_DEFUNC_UNIMPLEMENTED_OPER (initclip);
HG_DEFUNC_UNIMPLEMENTED_OPER (initgraphics);
HG_DEFUNC_UNIMPLEMENTED_OPER (initmatrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (internaldict);
HG_DEFUNC_UNIMPLEMENTED_OPER (invertmatrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (itransform);
HG_DEFUNC_UNIMPLEMENTED_OPER (known);
HG_DEFUNC_UNIMPLEMENTED_OPER (kshow);
HG_DEFUNC_UNIMPLEMENTED_OPER (le);
HG_DEFUNC_UNIMPLEMENTED_OPER (length);
HG_DEFUNC_UNIMPLEMENTED_OPER (lineto);
HG_DEFUNC_UNIMPLEMENTED_OPER (ln);
HG_DEFUNC_UNIMPLEMENTED_OPER (log);
HG_DEFUNC_UNIMPLEMENTED_OPER (loop);
HG_DEFUNC_UNIMPLEMENTED_OPER (lt);
HG_DEFUNC_UNIMPLEMENTED_OPER (makefont);
HG_DEFUNC_UNIMPLEMENTED_OPER (maxlength);
HG_DEFUNC_UNIMPLEMENTED_OPER (mod);
HG_DEFUNC_UNIMPLEMENTED_OPER (moveto);
HG_DEFUNC_UNIMPLEMENTED_OPER (mul);
HG_DEFUNC_UNIMPLEMENTED_OPER (ne);
HG_DEFUNC_UNIMPLEMENTED_OPER (neg);
HG_DEFUNC_UNIMPLEMENTED_OPER (newpath);
HG_DEFUNC_UNIMPLEMENTED_OPER (noaccess);
HG_DEFUNC_UNIMPLEMENTED_OPER (not);
HG_DEFUNC_UNIMPLEMENTED_OPER (nulldevice);
HG_DEFUNC_UNIMPLEMENTED_OPER (or);
HG_DEFUNC_UNIMPLEMENTED_OPER (pathbbox);
HG_DEFUNC_UNIMPLEMENTED_OPER (pathforall);
HG_DEFUNC_UNIMPLEMENTED_OPER (pop);
HG_DEFUNC_UNIMPLEMENTED_OPER (print);
HG_DEFUNC_UNIMPLEMENTED_OPER (put);
HG_DEFUNC_UNIMPLEMENTED_OPER (rand);
HG_DEFUNC_UNIMPLEMENTED_OPER (rcheck);
HG_DEFUNC_UNIMPLEMENTED_OPER (rcurveto);
HG_DEFUNC_UNIMPLEMENTED_OPER (read);
HG_DEFUNC_UNIMPLEMENTED_OPER (readhexstring);
HG_DEFUNC_UNIMPLEMENTED_OPER (readline);
HG_DEFUNC_UNIMPLEMENTED_OPER (readonly);
HG_DEFUNC_UNIMPLEMENTED_OPER (readstring);
HG_DEFUNC_UNIMPLEMENTED_OPER (repeat);
HG_DEFUNC_UNIMPLEMENTED_OPER (resetfile);
HG_DEFUNC_UNIMPLEMENTED_OPER (restore);
HG_DEFUNC_UNIMPLEMENTED_OPER (reversepath);
HG_DEFUNC_UNIMPLEMENTED_OPER (rlineto);
HG_DEFUNC_UNIMPLEMENTED_OPER (rmoveto);
HG_DEFUNC_UNIMPLEMENTED_OPER (roll);
HG_DEFUNC_UNIMPLEMENTED_OPER (rotate);
HG_DEFUNC_UNIMPLEMENTED_OPER (round);
HG_DEFUNC_UNIMPLEMENTED_OPER (rrand);
HG_DEFUNC_UNIMPLEMENTED_OPER (save);
HG_DEFUNC_UNIMPLEMENTED_OPER (scale);
HG_DEFUNC_UNIMPLEMENTED_OPER (scalefont);
HG_DEFUNC_UNIMPLEMENTED_OPER (search);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcachedevice);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcachelimit);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcharwidth);
HG_DEFUNC_UNIMPLEMENTED_OPER (setdash);
HG_DEFUNC_UNIMPLEMENTED_OPER (setflat);
HG_DEFUNC_UNIMPLEMENTED_OPER (setfont);
HG_DEFUNC_UNIMPLEMENTED_OPER (setgray);
HG_DEFUNC_UNIMPLEMENTED_OPER (sethsbcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (setlinecap);
HG_DEFUNC_UNIMPLEMENTED_OPER (setlinejoin);
HG_DEFUNC_UNIMPLEMENTED_OPER (setlinewidth);
HG_DEFUNC_UNIMPLEMENTED_OPER (setmatrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (setmiterlimit);
HG_DEFUNC_UNIMPLEMENTED_OPER (setrgbcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (setscreen);
HG_DEFUNC_UNIMPLEMENTED_OPER (settransfer);
HG_DEFUNC_UNIMPLEMENTED_OPER (show);
HG_DEFUNC_UNIMPLEMENTED_OPER (showpage);
HG_DEFUNC_UNIMPLEMENTED_OPER (sin);
HG_DEFUNC_UNIMPLEMENTED_OPER (sqrt);
HG_DEFUNC_UNIMPLEMENTED_OPER (srand);
HG_DEFUNC_UNIMPLEMENTED_OPER (status);
HG_DEFUNC_UNIMPLEMENTED_OPER (stop);
HG_DEFUNC_UNIMPLEMENTED_OPER (stopped);
HG_DEFUNC_UNIMPLEMENTED_OPER (string);
HG_DEFUNC_UNIMPLEMENTED_OPER (stringwidth);
HG_DEFUNC_UNIMPLEMENTED_OPER (stroke);
HG_DEFUNC_UNIMPLEMENTED_OPER (strokepath);
HG_DEFUNC_UNIMPLEMENTED_OPER (sub);
HG_DEFUNC_UNIMPLEMENTED_OPER (token);
HG_DEFUNC_UNIMPLEMENTED_OPER (transform);
HG_DEFUNC_UNIMPLEMENTED_OPER (translate);
HG_DEFUNC_UNIMPLEMENTED_OPER (truncate);
HG_DEFUNC_UNIMPLEMENTED_OPER (type);
HG_DEFUNC_UNIMPLEMENTED_OPER (usertime);
HG_DEFUNC_UNIMPLEMENTED_OPER (vmstatus);
HG_DEFUNC_UNIMPLEMENTED_OPER (wcheck);
HG_DEFUNC_UNIMPLEMENTED_OPER (where);
HG_DEFUNC_UNIMPLEMENTED_OPER (widthshow);
HG_DEFUNC_UNIMPLEMENTED_OPER (write);
HG_DEFUNC_UNIMPLEMENTED_OPER (writehexstring);
HG_DEFUNC_UNIMPLEMENTED_OPER (writestring);
HG_DEFUNC_UNIMPLEMENTED_OPER (xcheck);
HG_DEFUNC_UNIMPLEMENTED_OPER (xor);

HG_DEFUNC_UNIMPLEMENTED_OPER (arct);
HG_DEFUNC_UNIMPLEMENTED_OPER (colorimage);
HG_DEFUNC_UNIMPLEMENTED_OPER (cshow);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentblackgeneration);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentcacheparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentcmykcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentcolorrendering);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentcolorscreen);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentcolorspace);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentcolortransfer);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentdevparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentgstate);
HG_DEFUNC_UNIMPLEMENTED_OPER (currenthalftone);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentobjectformat);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentoverprint);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentpacking);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentpagedevice);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentshared);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentstrokeadjust);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentsystemparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentundercolorremoval);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentuserparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (defineresource);
HG_DEFUNC_UNIMPLEMENTED_OPER (defineuserobject);
HG_DEFUNC_UNIMPLEMENTED_OPER (deletefile);
HG_DEFUNC_UNIMPLEMENTED_OPER (execform);
HG_DEFUNC_UNIMPLEMENTED_OPER (execuserobject);
HG_DEFUNC_UNIMPLEMENTED_OPER (filenameforall);
HG_DEFUNC_UNIMPLEMENTED_OPER (fileposition);
HG_DEFUNC_UNIMPLEMENTED_OPER (filter);
HG_DEFUNC_UNIMPLEMENTED_OPER (findencoding);
HG_DEFUNC_UNIMPLEMENTED_OPER (findresource);
HG_DEFUNC_UNIMPLEMENTED_OPER (gcheck);
HG_DEFUNC_UNIMPLEMENTED_OPER (GlobalFontDirectory);
HG_DEFUNC_UNIMPLEMENTED_OPER (glyphshow);
HG_DEFUNC_UNIMPLEMENTED_OPER (gstate);
HG_DEFUNC_UNIMPLEMENTED_OPER (ineofill);
HG_DEFUNC_UNIMPLEMENTED_OPER (infill);
HG_DEFUNC_UNIMPLEMENTED_OPER (instroke);
HG_DEFUNC_UNIMPLEMENTED_OPER (inueofill);
HG_DEFUNC_UNIMPLEMENTED_OPER (inufill);
HG_DEFUNC_UNIMPLEMENTED_OPER (inustroke);
HG_DEFUNC_UNIMPLEMENTED_OPER (ISOLatin1Encoding);
HG_DEFUNC_UNIMPLEMENTED_OPER (languagelevel);
HG_DEFUNC_UNIMPLEMENTED_OPER (makepattern);
HG_DEFUNC_UNIMPLEMENTED_OPER (packedarray);
HG_DEFUNC_UNIMPLEMENTED_OPER (printobject);
HG_DEFUNC_UNIMPLEMENTED_OPER (realtime);
HG_DEFUNC_UNIMPLEMENTED_OPER (rectclip);
HG_DEFUNC_UNIMPLEMENTED_OPER (rectfill);
HG_DEFUNC_UNIMPLEMENTED_OPER (rectstroke);
HG_DEFUNC_UNIMPLEMENTED_OPER (renamefile);
HG_DEFUNC_UNIMPLEMENTED_OPER (resourceforall);
HG_DEFUNC_UNIMPLEMENTED_OPER (resourcestatus);
HG_DEFUNC_UNIMPLEMENTED_OPER (rootfont);
HG_DEFUNC_UNIMPLEMENTED_OPER (scheck);
HG_DEFUNC_UNIMPLEMENTED_OPER (selectfont);
HG_DEFUNC_UNIMPLEMENTED_OPER (serialnumber);
HG_DEFUNC_UNIMPLEMENTED_OPER (setbbox);
HG_DEFUNC_UNIMPLEMENTED_OPER (setblackgeneration);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcachedevice2);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcacheparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcmykcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcolor);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcolorrendering);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcolorscreen);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcolorspace);
HG_DEFUNC_UNIMPLEMENTED_OPER (setcolortransfer);
HG_DEFUNC_UNIMPLEMENTED_OPER (setdevparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (setfileposition);
HG_DEFUNC_UNIMPLEMENTED_OPER (setgstate);
HG_DEFUNC_UNIMPLEMENTED_OPER (sethalftone);
HG_DEFUNC_UNIMPLEMENTED_OPER (setobjectformat);
HG_DEFUNC_UNIMPLEMENTED_OPER (setoverprint);
HG_DEFUNC_UNIMPLEMENTED_OPER (setpacking);
HG_DEFUNC_UNIMPLEMENTED_OPER (setpagedevice);
HG_DEFUNC_UNIMPLEMENTED_OPER (setpattern);
HG_DEFUNC_UNIMPLEMENTED_OPER (setshared);
HG_DEFUNC_UNIMPLEMENTED_OPER (setstrokeadjust);
HG_DEFUNC_UNIMPLEMENTED_OPER (setsystemparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (setucacheparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (setundercolorremoval);
HG_DEFUNC_UNIMPLEMENTED_OPER (setuserparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (setvmthreshold);
HG_DEFUNC_UNIMPLEMENTED_OPER (shareddict);
HG_DEFUNC_UNIMPLEMENTED_OPER (SharedFontDirectory);
HG_DEFUNC_UNIMPLEMENTED_OPER (startjob);
HG_DEFUNC_UNIMPLEMENTED_OPER (uappend);
HG_DEFUNC_UNIMPLEMENTED_OPER (ucache);
HG_DEFUNC_UNIMPLEMENTED_OPER (ucachestatus);
HG_DEFUNC_UNIMPLEMENTED_OPER (ueofill);
HG_DEFUNC_UNIMPLEMENTED_OPER (ufill);
HG_DEFUNC_UNIMPLEMENTED_OPER (undef);
HG_DEFUNC_UNIMPLEMENTED_OPER (undefineresource);
HG_DEFUNC_UNIMPLEMENTED_OPER (undefineuserobject);
HG_DEFUNC_UNIMPLEMENTED_OPER (upath);
HG_DEFUNC_UNIMPLEMENTED_OPER (UserObjects);
HG_DEFUNC_UNIMPLEMENTED_OPER (ustroke);
HG_DEFUNC_UNIMPLEMENTED_OPER (ustrokepath);
HG_DEFUNC_UNIMPLEMENTED_OPER (vmreclaim);
HG_DEFUNC_UNIMPLEMENTED_OPER (writeobject);
HG_DEFUNC_UNIMPLEMENTED_OPER (xshow);
HG_DEFUNC_UNIMPLEMENTED_OPER (xyshow);
HG_DEFUNC_UNIMPLEMENTED_OPER (yshow);

HG_DEFUNC_UNIMPLEMENTED_OPER (addglyph);
HG_DEFUNC_UNIMPLEMENTED_OPER (beginbfchar);
HG_DEFUNC_UNIMPLEMENTED_OPER (beginbfrange);
HG_DEFUNC_UNIMPLEMENTED_OPER (begincidchar);
HG_DEFUNC_UNIMPLEMENTED_OPER (begincidrange);
HG_DEFUNC_UNIMPLEMENTED_OPER (begincmap);
HG_DEFUNC_UNIMPLEMENTED_OPER (begincodespacerange);
HG_DEFUNC_UNIMPLEMENTED_OPER (beginnotdefchar);
HG_DEFUNC_UNIMPLEMENTED_OPER (beginnotdefrange);
HG_DEFUNC_UNIMPLEMENTED_OPER (beginrearrangedfont);
HG_DEFUNC_UNIMPLEMENTED_OPER (beginusematrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (cliprestore);
HG_DEFUNC_UNIMPLEMENTED_OPER (clipsave);
HG_DEFUNC_UNIMPLEMENTED_OPER (composefont);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentsmoothness);
HG_DEFUNC_UNIMPLEMENTED_OPER (currentrapparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (endbfchar);
HG_DEFUNC_UNIMPLEMENTED_OPER (endbfrange);
HG_DEFUNC_UNIMPLEMENTED_OPER (endcidchar);
HG_DEFUNC_UNIMPLEMENTED_OPER (endcidrange);
HG_DEFUNC_UNIMPLEMENTED_OPER (endcmap);
HG_DEFUNC_UNIMPLEMENTED_OPER (endcodespacerange);
HG_DEFUNC_UNIMPLEMENTED_OPER (endnotdefchar);
HG_DEFUNC_UNIMPLEMENTED_OPER (endnotdefrange);
HG_DEFUNC_UNIMPLEMENTED_OPER (endrearrangedfont);
HG_DEFUNC_UNIMPLEMENTED_OPER (endusematrix);
HG_DEFUNC_UNIMPLEMENTED_OPER (findcolorrendering);
HG_DEFUNC_UNIMPLEMENTED_OPER (gethalftonename);
HG_DEFUNC_UNIMPLEMENTED_OPER (getpagedevicename);
HG_DEFUNC_UNIMPLEMENTED_OPER (getsubstitutecrd);
HG_DEFUNC_UNIMPLEMENTED_OPER (removeall);
HG_DEFUNC_UNIMPLEMENTED_OPER (removeglyphs);
HG_DEFUNC_UNIMPLEMENTED_OPER (setsmoothness);
HG_DEFUNC_UNIMPLEMENTED_OPER (settrapparams);
HG_DEFUNC_UNIMPLEMENTED_OPER (settrapzone);
HG_DEFUNC_UNIMPLEMENTED_OPER (shfill);
HG_DEFUNC_UNIMPLEMENTED_OPER (startdata);
HG_DEFUNC_UNIMPLEMENTED_OPER (usecmap);
HG_DEFUNC_UNIMPLEMENTED_OPER (usefont);

/*
 * public functions
 */
hg_object_t *
hg_object_operator_new(hg_vm_t              *vm,
		       hg_system_encoding_t  enc,
		       hg_operator_func_t    func)
{
	hg_object_t *retval;
	gint16 length;
	hg_operatordata_t *data;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (__hg_system_encoding_names[enc] != NULL, NULL);

	length = strlen(__hg_system_encoding_names[enc]);
	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_operatordata_t) + sizeof (gchar) * (length + 1)));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_OPERATOR;
		HG_OBJECT_OPERATOR (retval)->length = length;
		data = HG_OBJECT_OPERATOR_DATA (retval);
		data->func = func;
		memcpy(data->name, __hg_system_encoding_names[enc], length);
	}

	return retval;
}

hg_object_t *
hg_object_operator_new_with_custom(hg_vm_t            *vm,
				   gchar              *name,
				   hg_operator_func_t  func)
{
	hg_object_t *retval;
	gint16 length;
	hg_operatordata_t *data;

	hg_return_val_if_fail (vm != NULL, NULL);
	hg_return_val_if_fail (name != NULL, NULL);

	length = strlen(name);
	retval = hg_object_sized_new(vm, hg_n_alignof (sizeof (hg_operatordata_t) + sizeof (gchar) * (length + 1)));
	if (retval != NULL) {
		HG_OBJECT_GET_TYPE (retval) = HG_OBJECT_TYPE_OPERATOR;
		HG_OBJECT_OPERATOR (retval)->length = length;
		data = HG_OBJECT_OPERATOR_DATA (retval);
		data->func = func;
		memcpy(data->name, name, length);
	}

	return retval;
}

gboolean
hg_object_operator_initialize(hg_vm_t            *vm,
			      hg_emulationtype_t  level)
{
	gboolean retval = TRUE;
	hg_emulationtype_t current;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (level < HG_EMU_END, FALSE);

	/* just set a flag to lock up */
	__hg_operator_initialized = TRUE;

	current = hg_vm_get_emulation_level(vm);
	if (level >= HG_EMU_PS_LEVEL_1 && current < level) {
		_hg_object_operator_priv_build(vm, %arraytomark, private_arraytomark);
		_hg_object_operator_priv_build(vm, %dicttomark, private_dicttomark);
		_hg_object_operator_priv_build(vm, %for_pos_int_continue, private_for_pos_int_continue);
		_hg_object_operator_priv_build(vm, %for_pos_real_continue, private_for_pos_real_continue);
		_hg_object_operator_priv_build(vm, %forall_array_continue, private_forall_array_continue);
		_hg_object_operator_priv_build(vm, %forall_dict_continue, private_forall_dict_continue);
		_hg_object_operator_priv_build(vm, %forall_string_continue, private_forall_string_continue);
		_hg_object_operator_priv_build(vm, %loop_continue, private_loop_continue);
		_hg_object_operator_priv_build(vm, %repeat_continue, private_repeat_continue);
		_hg_object_operator_priv_build(vm, %stopped_continue, private_stopped_continue);
		_hg_object_operator_priv_build(vm, .findfont, private_findfont);
		_hg_object_operator_priv_build(vm, .definefont, private_definefont);
		_hg_object_operator_priv_build(vm, .stringcvs, private_stringcvs);
		_hg_object_operator_priv_build(vm, .undefinefont, private_undefinefont);
		_hg_object_operator_priv_build(vm, .write==only, private_write_eqeq_only);
		_hg_object_operator_build(vm, abs, abs);
		_hg_object_operator_build(vm, add, add);
		_hg_object_operator_build(vm, aload, aload);
		_hg_object_operator_build(vm, and, and);
		_hg_object_operator_build(vm, arc, arc);
		_hg_object_operator_build(vm, arcn, arcn);
		_hg_object_operator_build(vm, arcto, arcto);
		_hg_object_operator_build(vm, array, array);
		_hg_object_operator_build(vm, ashow, ashow);
		_hg_object_operator_build(vm, astore, astore);
		_hg_object_operator_build(vm, atan, atan);
		_hg_object_operator_build(vm, awidthshow, awidthshow);
		_hg_object_operator_build(vm, begin, begin);
		_hg_object_operator_build(vm, bind, bind);
		_hg_object_operator_build(vm, bitshift, bitshift);
		_hg_object_operator_build(vm, bytesavailable, bytesavailable);
		_hg_object_operator_build(vm, cachestatus, cachestatus);
		_hg_object_operator_build(vm, ceiling, ceiling);
		_hg_object_operator_build(vm, charpath, charpath);
		_hg_object_operator_build(vm, clear, clear);
		_hg_object_operator_build(vm, cleardictstack, cleardictstack);
		_hg_object_operator_build(vm, cleartomark, cleartomark);
		_hg_object_operator_build(vm, clip, clip);
		_hg_object_operator_build(vm, clippath, clippath);
		_hg_object_operator_build(vm, closefile, closefile);
		_hg_object_operator_build(vm, closepath, closepath);
		_hg_object_operator_build(vm, concat, concat);
		_hg_object_operator_build(vm, concatmatrix, concatmatrix);
		_hg_object_operator_build(vm, copy, copy);
		_hg_object_operator_build(vm, copypage, copypage);
		_hg_object_operator_build(vm, cos, cos);
		_hg_object_operator_build(vm, count, count);
		_hg_object_operator_build(vm, countdictstack, countdictstack);
		_hg_object_operator_build(vm, countexecstack, countexecstack);
		_hg_object_operator_build(vm, counttomark, counttomark);
		_hg_object_operator_build(vm, currentdash, currentdash);
		_hg_object_operator_build(vm, currentdict, currentdict);
		_hg_object_operator_build(vm, currentfile, currentfile);
		_hg_object_operator_build(vm, currentflat, currentflat);
		_hg_object_operator_build(vm, currentfont, currentfont);
		_hg_object_operator_build(vm, currentgray, currentgray);
		_hg_object_operator_build(vm, currenthsbcolor, currenthsbcolor);
		_hg_object_operator_build(vm, currentlinecap, currentlinecap);
		_hg_object_operator_build(vm, currentlinejoin, currentlinejoin);
		_hg_object_operator_build(vm, currentlinewidth, currentlinewidth);
		_hg_object_operator_build(vm, currentmatrix, currentmatrix);
		_hg_object_operator_build(vm, currentmiterlimit, currentmiterlimit);
		_hg_object_operator_build(vm, currentpoint, currentpoint);
		_hg_object_operator_build(vm, currentrgbcolor, currentrgbcolor);
		_hg_object_operator_build(vm, currentscreen, currentscreen);
		_hg_object_operator_build(vm, currenttransfer, currenttransfer);
		_hg_object_operator_build(vm, curveto, curveto);
		_hg_object_operator_build(vm, cvi, cvi);
		_hg_object_operator_build(vm, cvlit, cvlit);
		_hg_object_operator_build(vm, cvn, cvn);
		_hg_object_operator_build(vm, cvr, cvr);
		_hg_object_operator_build(vm, cvrs, cvrs);
		_hg_object_operator_build(vm, cvx, cvx);
		_hg_object_operator_build(vm, def, def);
		_hg_object_operator_build(vm, defaultmatrix, defaultmatrix);
		_hg_object_operator_build(vm, dict, dict);
		_hg_object_operator_build(vm, dictstack, dictstack);
		_hg_object_operator_build(vm, div, div);
		_hg_object_operator_build(vm, dtransform, dtransform);
		_hg_object_operator_build(vm, dup, dup);
		_hg_object_operator_build(vm, echo, echo);
		_hg_object_operator_priv_build(vm, eexec, eexec);
		_hg_object_operator_build(vm, end, end);
		_hg_object_operator_build(vm, eoclip, eoclip);
		_hg_object_operator_build(vm, eofill, eofill);
		_hg_object_operator_build(vm, eq, eq);
		_hg_object_operator_build(vm, erasepage, erasepage);
		_hg_object_operator_build(vm, exch, exch);
		_hg_object_operator_build(vm, exec, exec);
		_hg_object_operator_build(vm, execstack, execstack);
		_hg_object_operator_build(vm, executeonly, executeonly);
		_hg_object_operator_build(vm, exit, exit);
		_hg_object_operator_build(vm, exp, exp);
		_hg_object_operator_build(vm, file, file);
		_hg_object_operator_build(vm, fill, fill);
		_hg_object_operator_build(vm, flattenpath, flattenpath);
		_hg_object_operator_build(vm, flush, flush);
		_hg_object_operator_build(vm, flushfile, flushfile);
		_hg_object_operator_build(vm, FontDirectory, fontdirectory);
		_hg_object_operator_build(vm, for, for);
		_hg_object_operator_build(vm, forall, forall);
		_hg_object_operator_build(vm, ge, ge);
		_hg_object_operator_build(vm, get, get);
		_hg_object_operator_build(vm, getinterval, getinterval);
		_hg_object_operator_build(vm, grestore, grestore);
		_hg_object_operator_build(vm, grestoreall, grestoreall);
		_hg_object_operator_build(vm, gsave, gsave);
		_hg_object_operator_build(vm, gt, gt);
		_hg_object_operator_build(vm, identmatrix, identmatrix);
		_hg_object_operator_build(vm, idiv, idiv);
		_hg_object_operator_build(vm, idtransform, idtransform);
		_hg_object_operator_build(vm, if, if);
		_hg_object_operator_build(vm, ifelse, ifelse);
		_hg_object_operator_build(vm, image, image);
		_hg_object_operator_build(vm, imagemask, imagemask);
		_hg_object_operator_build(vm, index, index);
		_hg_object_operator_build(vm, initclip, initclip);
		_hg_object_operator_build(vm, initgraphics, initgraphics);
		_hg_object_operator_build(vm, initmatrix, initmatrix);
		_hg_object_operator_priv_build(vm, internaldict, internaldict);
		_hg_object_operator_build(vm, invertmatrix, invertmatrix);
		_hg_object_operator_build(vm, itransform, itransform);
		_hg_object_operator_build(vm, known, known);
		_hg_object_operator_build(vm, kshow, kshow);
		_hg_object_operator_build(vm, le, le);
		_hg_object_operator_build(vm, length, length);
		_hg_object_operator_build(vm, lineto, lineto);
		_hg_object_operator_build(vm, ln, ln);
		_hg_object_operator_build(vm, log, log);
		_hg_object_operator_build(vm, loop, loop);
		_hg_object_operator_build(vm, lt, lt);
		_hg_object_operator_build(vm, makefont, makefont);
		_hg_object_operator_build(vm, maxlength, maxlength);
		_hg_object_operator_build(vm, mod, mod);
		_hg_object_operator_build(vm, moveto, moveto);
		_hg_object_operator_build(vm, mul, mul);
		_hg_object_operator_build(vm, ne, ne);
		_hg_object_operator_build(vm, neg, neg);
		_hg_object_operator_build(vm, newpath, newpath);
		_hg_object_operator_build(vm, noaccess, noaccess);
		_hg_object_operator_build(vm, not, not);
		_hg_object_operator_build(vm, nulldevice, nulldevice);
		_hg_object_operator_build(vm, or, or);
		_hg_object_operator_build(vm, pathbbox, pathbbox);
		_hg_object_operator_build(vm, pathforall, pathforall);
		_hg_object_operator_build(vm, pop, pop);
		_hg_object_operator_build(vm, print, print);
		_hg_object_operator_build(vm, put, put);
		_hg_object_operator_build(vm, rand, rand);
		_hg_object_operator_build(vm, rcheck, rcheck);
		_hg_object_operator_build(vm, rcurveto, rcurveto);
		_hg_object_operator_build(vm, read, read);
		_hg_object_operator_build(vm, readhexstring, readhexstring);
		_hg_object_operator_build(vm, readline, readline);
		_hg_object_operator_build(vm, readonly, readonly);
		_hg_object_operator_build(vm, readstring, readstring);
		_hg_object_operator_build(vm, repeat, repeat);
		_hg_object_operator_build(vm, resetfile, resetfile);
		_hg_object_operator_build(vm, restore, restore);
		_hg_object_operator_build(vm, reversepath, reversepath);
		_hg_object_operator_build(vm, rlineto, rlineto);
		_hg_object_operator_build(vm, rmoveto, rmoveto);
		_hg_object_operator_build(vm, roll, roll);
		_hg_object_operator_build(vm, rotate, rotate);
		_hg_object_operator_build(vm, round, round);
		_hg_object_operator_build(vm, rrand, rrand);
		_hg_object_operator_build(vm, save, save);
		_hg_object_operator_build(vm, scale, scale);
		_hg_object_operator_build(vm, scalefont, scalefont);
		_hg_object_operator_build(vm, search, search);
		_hg_object_operator_build(vm, setcachedevice, setcachedevice);
		_hg_object_operator_build(vm, setcachelimit, setcachelimit);
		_hg_object_operator_build(vm, setcharwidth, setcharwidth);
		_hg_object_operator_build(vm, setdash, setdash);
		_hg_object_operator_build(vm, setflat, setflat);
		_hg_object_operator_build(vm, setfont, setfont);
		_hg_object_operator_build(vm, setgray, setgray);
		_hg_object_operator_build(vm, sethsbcolor, sethsbcolor);
		_hg_object_operator_build(vm, setlinecap, setlinecap);
		_hg_object_operator_build(vm, setlinejoin, setlinejoin);
		_hg_object_operator_build(vm, setlinewidth, setlinewidth);
		_hg_object_operator_build(vm, setmatrix, setmatrix);
		_hg_object_operator_build(vm, setmiterlimit, setmiterlimit);
		_hg_object_operator_build(vm, setrgbcolor, setrgbcolor);
		_hg_object_operator_build(vm, setscreen, setscreen);
		_hg_object_operator_build(vm, settransfer, settransfer);
		_hg_object_operator_build(vm, show, show);
		_hg_object_operator_build(vm, showpage, showpage);
		_hg_object_operator_build(vm, sin, sin);
		_hg_object_operator_build(vm, sqrt, sqrt);
		_hg_object_operator_build(vm, srand, srand);
		_hg_object_operator_build(vm, status, status);
		_hg_object_operator_build(vm, stop, stop);
		_hg_object_operator_build(vm, stopped, stopped);
		_hg_object_operator_build(vm, string, string);
		_hg_object_operator_build(vm, stringwidth, stringwidth);
		_hg_object_operator_build(vm, stroke, stroke);
		_hg_object_operator_build(vm, strokepath, strokepath);
		_hg_object_operator_build(vm, sub, sub);
		_hg_object_operator_build(vm, token, token);
		_hg_object_operator_build(vm, transform, transform);
		_hg_object_operator_build(vm, translate, translate);
		_hg_object_operator_build(vm, truncate, truncate);
		_hg_object_operator_build(vm, type, type);
		_hg_object_operator_build(vm, usertime, usertime);
		_hg_object_operator_build(vm, vmstatus, vmstatus);
		_hg_object_operator_build(vm, wcheck, wcheck);
		_hg_object_operator_build(vm, where, where);
		_hg_object_operator_build(vm, widthshow, widthshow);
		_hg_object_operator_build(vm, write, write);
		_hg_object_operator_build(vm, writehexstring, writehexstring);
		_hg_object_operator_build(vm, writestring, writestring);
		_hg_object_operator_build(vm, xcheck, xcheck);
		_hg_object_operator_build(vm, xor, xor);
	}

	if (level >= HG_EMU_PS_LEVEL_2 && current < level) {
		_hg_object_operator_build(vm, arct, arct);
		_hg_object_operator_build(vm, colorimage, colorimage);
		_hg_object_operator_build(vm, cshow, cshow);
		_hg_object_operator_build(vm, currentblackgeneration, currentblackgeneration);
		_hg_object_operator_build(vm, currentcacheparams, currentcacheparams);
		_hg_object_operator_build(vm, currentcmykcolor, currentcmykcolor);
		_hg_object_operator_build(vm, currentcolor, currentcolor);
		_hg_object_operator_build(vm, currentcolorrendering, currentcolorrendering);
		_hg_object_operator_build(vm, currentcolorscreen, currentcolorscreen);
		_hg_object_operator_build(vm, currentcolorspace, currentcolorspace);
		_hg_object_operator_build(vm, currentcolortransfer, currentcolortransfer);
		_hg_object_operator_build(vm, currentdevparams, currentdevparams);
		_hg_object_operator_build(vm, currentgstate, currentgstate);
		_hg_object_operator_build(vm, currenthalftone, currenthalftone);
		_hg_object_operator_build(vm, currentobjectformat, currentobjectformat);
		_hg_object_operator_build(vm, currentoverprint, currentoverprint);
		_hg_object_operator_build(vm, currentpacking, currentpacking);
		_hg_object_operator_build(vm, currentpagedevice, currentpagedevice);
		_hg_object_operator_build(vm, currentshared, currentshared);
		_hg_object_operator_build(vm, currentstrokeadjust, currentstrokeadjust);
		_hg_object_operator_build(vm, currentsystemparams, currentsystemparams);
		_hg_object_operator_build(vm, currentundercolorremoval, currentundercolorremoval);
		_hg_object_operator_build(vm, currentuserparams, currentuserparams);
		_hg_object_operator_build(vm, defineresource, defineresource);
		_hg_object_operator_build(vm, defineuserobject, defineuserobject);
		_hg_object_operator_build(vm, deletefile, deletefile);
		_hg_object_operator_build(vm, execform, execform);
		_hg_object_operator_build(vm, execuserobject, execuserobject);
		_hg_object_operator_build(vm, filenameforall, filenameforall);
		_hg_object_operator_build(vm, fileposition, fileposition);
		_hg_object_operator_build(vm, filter, filter);
		_hg_object_operator_build(vm, findencoding, findencoding);
		_hg_object_operator_build(vm, findresource, findresource);
		_hg_object_operator_build(vm, gcheck, gcheck);
		_hg_object_operator_build(vm, GlobalFontDirectory, globalfontdirectory);
		_hg_object_operator_build(vm, glyphshow, glyphshow);
		_hg_object_operator_build(vm, gstate, gstate);
		_hg_object_operator_build(vm, ineofill, ineofill);
		_hg_object_operator_build(vm, infill, infill);
		_hg_object_operator_build(vm, instroke, instroke);
		_hg_object_operator_build(vm, inueofill, inueofill);
		_hg_object_operator_build(vm, inufill, inufill);
		_hg_object_operator_build(vm, inustroke, inustroke);
		_hg_object_operator_build(vm, ISOLatin1Encoding, isolatin1encoding);
		_hg_object_operator_build(vm, languagelevel, languagelevel);
		_hg_object_operator_build(vm, makepattern, makepattern);
		_hg_object_operator_build(vm, packedarray, packedarray);
		_hg_object_operator_build(vm, printobject, printobject);
		_hg_object_operator_build(vm, realtime, realtime);
		_hg_object_operator_build(vm, rectclip, rectclip);
		_hg_object_operator_build(vm, rectfill, rectfill);
		_hg_object_operator_build(vm, rectstroke, rectstroke);
		_hg_object_operator_build(vm, renamefile, renamefile);
		_hg_object_operator_build(vm, resourceforall, resourceforall);
		_hg_object_operator_build(vm, resourcestatus, resourcestatus);
		_hg_object_operator_build(vm, rootfont, rootfont);
		_hg_object_operator_build(vm, scheck, scheck);
		_hg_object_operator_build(vm, selectfont, selectfont);
		_hg_object_operator_build(vm, serialnumber, serialnumber);
		_hg_object_operator_build(vm, setbbox, setbbox);
		_hg_object_operator_build(vm, setblackgeneration, setblackgeneration);
		_hg_object_operator_build(vm, setcachedevice2, setcachedevice2);
		_hg_object_operator_build(vm, setcacheparams, setcacheparams);
		_hg_object_operator_build(vm, setcmykcolor, setcmykcolor);
		_hg_object_operator_build(vm, setcolor, setcolor);
		_hg_object_operator_build(vm, setcolorrendering, setcolorrendering);
		_hg_object_operator_build(vm, setcolorscreen, setcolorscreen);
		_hg_object_operator_build(vm, setcolorspace, setcolorspace);
		_hg_object_operator_build(vm, setcolortransfer, setcolortransfer);
		_hg_object_operator_build(vm, setdevparams, setdevparams);
		_hg_object_operator_build(vm, setfileposition, setfileposition);
		_hg_object_operator_build(vm, setgstate, setgstate);
		_hg_object_operator_build(vm, sethalftone, sethalftone);
		_hg_object_operator_build(vm, setobjectformat, setobjectformat);
		_hg_object_operator_build(vm, setoverprint, setoverprint);
		_hg_object_operator_build(vm, setpacking, setpacking);
		_hg_object_operator_build(vm, setpagedevice, setpagedevice);
		_hg_object_operator_build(vm, setpattern, setpattern);
		_hg_object_operator_build(vm, setshared, setshared);
		_hg_object_operator_build(vm, setstrokeadjust, setstrokeadjust);
		_hg_object_operator_build(vm, setsystemparams, setsystemparams);
		_hg_object_operator_build(vm, setucacheparams, setucacheparams);
		_hg_object_operator_build(vm, setundercolorremoval, setundercoloremoval);
		_hg_object_operator_build(vm, setuserparams, setuserparams);
		_hg_object_operator_build(vm, setvmthreshold, setvmthreshold);
		_hg_object_operator_build(vm, shareddict, shareddict);
		_hg_object_operator_build(vm, SharedFontDirectory, sharedfontdirectory);
		_hg_object_operator_build(vm, startjob, startjob);
		_hg_object_operator_build(vm, uappend, uappend);
		_hg_object_operator_build(vm, ucache, ucache);
		_hg_object_operator_build(vm, ucachestatus, ucachestatus);
		_hg_object_operator_build(vm, ueofill, ueofill);
		_hg_object_operator_build(vm, ufill, ufill);
		_hg_object_operator_build(vm, undef, undef);
		_hg_object_operator_build(vm, undefineresource, undefineresource);
		_hg_object_operator_build(vm, undefineuserobject, undefineuserobject);
		_hg_object_operator_build(vm, upath, upath);
		_hg_object_operator_build(vm, UserObjects, userobjects);
		_hg_object_operator_build(vm, ustroke, ustroke);
		_hg_object_operator_build(vm, ustrokepath, ustrokepath);
		_hg_object_operator_build(vm, vmreclaim, vmreclaim);
		_hg_object_operator_build(vm, writeobject, writeobject);
		_hg_object_operator_build(vm, xshow, xshow);
		_hg_object_operator_build(vm, xyshow, xyshow);
		_hg_object_operator_build(vm, yshow, yshow);
	}

	if (level >= HG_EMU_PS_LEVEL_3 && current < level) {
		_hg_object_operator_priv_build(vm, addglyph, addglyph);
		_hg_object_operator_priv_build(vm, beginbfchar, beginbfchar);
		_hg_object_operator_priv_build(vm, beginbfrange, beginbfrange);
		_hg_object_operator_priv_build(vm, begincidchar, begincidchar);
		_hg_object_operator_priv_build(vm, begincidrange, begincidrange);
		_hg_object_operator_priv_build(vm, begincmap, begincmap);
		_hg_object_operator_priv_build(vm, begincodespacerange, begincodespacerange);
		_hg_object_operator_priv_build(vm, beginnotdefchar, beginnotdefchar);
		_hg_object_operator_priv_build(vm, beginnotdefrange, beginnotdefrange);
		_hg_object_operator_priv_build(vm, beginrearrangedfont, beginrearrangedfont);
		_hg_object_operator_priv_build(vm, beginusematrix, beginusematrix);
		_hg_object_operator_priv_build(vm, cliprestore, cliprestore);
		_hg_object_operator_priv_build(vm, clipsave, clipsave);
		_hg_object_operator_priv_build(vm, composefont, composefont);
		_hg_object_operator_priv_build(vm, currentsmoothness, currentsmoothness);
		_hg_object_operator_priv_build(vm, currentrapparams, currentrapparams);
		_hg_object_operator_priv_build(vm, endbfchar, endbfchar);
		_hg_object_operator_priv_build(vm, endbfrange, endbfrange);
		_hg_object_operator_priv_build(vm, endcidchar, endcidchar);
		_hg_object_operator_priv_build(vm, endcidrange, endcidrange);
		_hg_object_operator_priv_build(vm, endcmap, endcmap);
		_hg_object_operator_priv_build(vm, endcodespacerange, endcodespacerange);
		_hg_object_operator_priv_build(vm, endnotdefchar, endnotdefchar);
		_hg_object_operator_priv_build(vm, endnotdefrange, endnotdefrange);
		_hg_object_operator_priv_build(vm, endrearrangedfont, endrearrangedfont);
		_hg_object_operator_priv_build(vm, endusematrix, endusematrix);
		_hg_object_operator_priv_build(vm, findcolorrendering, findcolorrendering);
		_hg_object_operator_priv_build(vm, GetHalftoneName, gethalftonename);
		_hg_object_operator_priv_build(vm, GetPageDeviceName, getpagedevicename);
		_hg_object_operator_priv_build(vm, GetSubstituteCRD, getsubstitutecrd);
		_hg_object_operator_priv_build(vm, removeall, removeall);
		_hg_object_operator_priv_build(vm, removeglyphs, removeglyphs);
		_hg_object_operator_priv_build(vm, setsmoothness, setsmoothness);
		_hg_object_operator_priv_build(vm, settrapparams, settrapparams);
		_hg_object_operator_priv_build(vm, settrapzone, settrapzone);
		_hg_object_operator_priv_build(vm, shfill, shfill);
		_hg_object_operator_priv_build(vm, StartData, startdata);
		_hg_object_operator_priv_build(vm, usecmap, usecmap);
		_hg_object_operator_priv_build(vm, usefont, usefont);
	}

	/* reflect to the result */
	__hg_operator_initialized = retval;

	return retval;
}

gboolean
hg_object_operator_finalize(hg_vm_t            *vm,
			    hg_emulationtype_t  level)
{
	gboolean retval = TRUE;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (__hg_operator_initialized, FALSE);
	hg_return_val_if_fail (level < HG_EMU_END, FALSE);

	if (level < HG_EMU_PS_LEVEL_3) {
		_hg_object_operator_unbuild(vm, addglyph);
		_hg_object_operator_unbuild(vm, beginbfchar);
		_hg_object_operator_unbuild(vm, beginbfrange);
		_hg_object_operator_unbuild(vm, begincidchar);
		_hg_object_operator_unbuild(vm, begincidrange);
		_hg_object_operator_unbuild(vm, begincmap);
		_hg_object_operator_unbuild(vm, begincodespacerange);
		_hg_object_operator_unbuild(vm, beginnotdefchar);
		_hg_object_operator_unbuild(vm, beginnotdefrange);
		_hg_object_operator_unbuild(vm, beginrearrangedfont);
		_hg_object_operator_unbuild(vm, beginusematrix);
		_hg_object_operator_unbuild(vm, cliprestore);
		_hg_object_operator_unbuild(vm, clipsave);
		_hg_object_operator_unbuild(vm, composefont);
		_hg_object_operator_unbuild(vm, currentsmoothness);
		_hg_object_operator_unbuild(vm, currentrapparams);
		_hg_object_operator_unbuild(vm, endbfchar);
		_hg_object_operator_unbuild(vm, endbfrange);
		_hg_object_operator_unbuild(vm, endcidchar);
		_hg_object_operator_unbuild(vm, endcidrange);
		_hg_object_operator_unbuild(vm, endcmap);
		_hg_object_operator_unbuild(vm, endcodespacerange);
		_hg_object_operator_unbuild(vm, endnotdefchar);
		_hg_object_operator_unbuild(vm, endnotdefrange);
		_hg_object_operator_unbuild(vm, endrearrangedfont);
		_hg_object_operator_unbuild(vm, endusematrix);
		_hg_object_operator_unbuild(vm, findcolorrendering);
		_hg_object_operator_unbuild(vm, GetHalftoneName);
		_hg_object_operator_unbuild(vm, GetPageDeviceName);
		_hg_object_operator_unbuild(vm, GetSubstituteCRD);
		_hg_object_operator_unbuild(vm, removeall);
		_hg_object_operator_unbuild(vm, removeglyphs);
		_hg_object_operator_unbuild(vm, setsmoothness);
		_hg_object_operator_unbuild(vm, settrapparams);
		_hg_object_operator_unbuild(vm, settrapzone);
		_hg_object_operator_unbuild(vm, shfill);
		_hg_object_operator_unbuild(vm, StartData);
		_hg_object_operator_unbuild(vm, usecmap);
		_hg_object_operator_unbuild(vm, usefont);
	}
	if (level < HG_EMU_PS_LEVEL_2) {
		_hg_object_operator_unbuild(vm, arct);
		_hg_object_operator_unbuild(vm, colorimage);
		_hg_object_operator_unbuild(vm, cshow);
		_hg_object_operator_unbuild(vm, currentblackgeneration);
		_hg_object_operator_unbuild(vm, currentcacheparams);
		_hg_object_operator_unbuild(vm, currentcmykcolor);
		_hg_object_operator_unbuild(vm, currentcolor);
		_hg_object_operator_unbuild(vm, currentcolorrendering);
		_hg_object_operator_unbuild(vm, currentcolorscreen);
		_hg_object_operator_unbuild(vm, currentcolorspace);
		_hg_object_operator_unbuild(vm, currentcolortransfer);
		_hg_object_operator_unbuild(vm, currentdevparams);
		_hg_object_operator_unbuild(vm, currentgstate);
		_hg_object_operator_unbuild(vm, currenthalftone);
		_hg_object_operator_unbuild(vm, currentobjectformat);
		_hg_object_operator_unbuild(vm, currentoverprint);
		_hg_object_operator_unbuild(vm, currentpacking);
		_hg_object_operator_unbuild(vm, currentpagedevice);
		_hg_object_operator_unbuild(vm, currentshared);
		_hg_object_operator_unbuild(vm, currentstrokeadjust);
		_hg_object_operator_unbuild(vm, currentsystemparams);
		_hg_object_operator_unbuild(vm, currentundercolorremoval);
		_hg_object_operator_unbuild(vm, currentuserparams);
		_hg_object_operator_unbuild(vm, defineresource);
		_hg_object_operator_unbuild(vm, defineuserobject);
		_hg_object_operator_unbuild(vm, deletefile);
		_hg_object_operator_unbuild(vm, execform);
		_hg_object_operator_unbuild(vm, execuserobject);
		_hg_object_operator_unbuild(vm, filenameforall);
		_hg_object_operator_unbuild(vm, fileposition);
		_hg_object_operator_unbuild(vm, filter);
		_hg_object_operator_unbuild(vm, findencoding);
		_hg_object_operator_unbuild(vm, findresource);
		_hg_object_operator_unbuild(vm, gcheck);
		_hg_object_operator_unbuild(vm, GlobalFontDirectory);
		_hg_object_operator_unbuild(vm, glyphshow);
		_hg_object_operator_unbuild(vm, gstate);
		_hg_object_operator_unbuild(vm, ineofill);
		_hg_object_operator_unbuild(vm, infill);
		_hg_object_operator_unbuild(vm, instroke);
		_hg_object_operator_unbuild(vm, inueofill);
		_hg_object_operator_unbuild(vm, inufill);
		_hg_object_operator_unbuild(vm, inustroke);
		_hg_object_operator_unbuild(vm, ISOLatin1Encoding);
		_hg_object_operator_unbuild(vm, languagelevel);
		_hg_object_operator_unbuild(vm, makepattern);
		_hg_object_operator_unbuild(vm, packedarray);
		_hg_object_operator_unbuild(vm, printobject);
		_hg_object_operator_unbuild(vm, realtime);
		_hg_object_operator_unbuild(vm, rectclip);
		_hg_object_operator_unbuild(vm, rectfill);
		_hg_object_operator_unbuild(vm, rectstroke);
		_hg_object_operator_unbuild(vm, renamefile);
		_hg_object_operator_unbuild(vm, resourceforall);
		_hg_object_operator_unbuild(vm, resourcestatus);
		_hg_object_operator_unbuild(vm, rootfont);
		_hg_object_operator_unbuild(vm, scheck);
		_hg_object_operator_unbuild(vm, selectfont);
		_hg_object_operator_unbuild(vm, serialnumber);
		_hg_object_operator_unbuild(vm, setbbox);
		_hg_object_operator_unbuild(vm, setblackgeneration);
		_hg_object_operator_unbuild(vm, setcachedevice2);
		_hg_object_operator_unbuild(vm, setcacheparams);
		_hg_object_operator_unbuild(vm, setcmykcolor);
		_hg_object_operator_unbuild(vm, setcolor);
		_hg_object_operator_unbuild(vm, setcolorrendering);
		_hg_object_operator_unbuild(vm, setcolorscreen);
		_hg_object_operator_unbuild(vm, setcolorspace);
		_hg_object_operator_unbuild(vm, setcolortransfer);
		_hg_object_operator_unbuild(vm, setdevparams);
		_hg_object_operator_unbuild(vm, setfileposition);
		_hg_object_operator_unbuild(vm, setgstate);
		_hg_object_operator_unbuild(vm, sethalftone);
		_hg_object_operator_unbuild(vm, setobjectformat);
		_hg_object_operator_unbuild(vm, setoverprint);
		_hg_object_operator_unbuild(vm, setpacking);
		_hg_object_operator_unbuild(vm, setpagedevice);
		_hg_object_operator_unbuild(vm, setpattern);
		_hg_object_operator_unbuild(vm, setshared);
		_hg_object_operator_unbuild(vm, setstrokeadjust);
		_hg_object_operator_unbuild(vm, setsystemparams);
		_hg_object_operator_unbuild(vm, setucacheparams);
		_hg_object_operator_unbuild(vm, setundercolorremoval);
		_hg_object_operator_unbuild(vm, setuserparams);
		_hg_object_operator_unbuild(vm, setvmthreshold);
		_hg_object_operator_unbuild(vm, shareddict);
		_hg_object_operator_unbuild(vm, SharedFontDirectory);
		_hg_object_operator_unbuild(vm, startjob);
		_hg_object_operator_unbuild(vm, uappend);
		_hg_object_operator_unbuild(vm, ucache);
		_hg_object_operator_unbuild(vm, ucachestatus);
		_hg_object_operator_unbuild(vm, ueofill);
		_hg_object_operator_unbuild(vm, ufill);
		_hg_object_operator_unbuild(vm, undef);
		_hg_object_operator_unbuild(vm, undefineresource);
		_hg_object_operator_unbuild(vm, undefineuserobject);
		_hg_object_operator_unbuild(vm, upath);
		_hg_object_operator_unbuild(vm, UserObjects);
		_hg_object_operator_unbuild(vm, ustroke);
		_hg_object_operator_unbuild(vm, ustrokepath);
		_hg_object_operator_unbuild(vm, vmreclaim);
		_hg_object_operator_unbuild(vm, writeobject);
		_hg_object_operator_unbuild(vm, xshow);
		_hg_object_operator_unbuild(vm, xyshow);
		_hg_object_operator_unbuild(vm, yshow);
	}
	if (level < HG_EMU_PS_LEVEL_1) {
		_hg_object_operator_unbuild(vm, %arraytomark);
		_hg_object_operator_unbuild(vm, %dicttomark);
		_hg_object_operator_unbuild(vm, %for_pos_int_continue);
		_hg_object_operator_unbuild(vm, %for_pos_real_continue);
		_hg_object_operator_unbuild(vm, %forall_array_continue);
		_hg_object_operator_unbuild(vm, %forall_dict_continue);
		_hg_object_operator_unbuild(vm, %forall_string_continue);
		_hg_object_operator_unbuild(vm, %loop_continue);
		_hg_object_operator_unbuild(vm, %repeat_continue);
		_hg_object_operator_unbuild(vm, %stopped_continue);
		_hg_object_operator_unbuild(vm, .findfont);
		_hg_object_operator_unbuild(vm, .definefont);
		_hg_object_operator_unbuild(vm, .stringcvs);
		_hg_object_operator_unbuild(vm, .undefinefont);
		_hg_object_operator_unbuild(vm, .write==only);
		_hg_object_operator_unbuild(vm, abs);
		_hg_object_operator_unbuild(vm, add);
		_hg_object_operator_unbuild(vm, aload);
		_hg_object_operator_unbuild(vm, and);
		_hg_object_operator_unbuild(vm, arc);
		_hg_object_operator_unbuild(vm, arcn);
		_hg_object_operator_unbuild(vm, arcto);
		_hg_object_operator_unbuild(vm, array);
		_hg_object_operator_unbuild(vm, ashow);
		_hg_object_operator_unbuild(vm, astore);
		_hg_object_operator_unbuild(vm, atan);
		_hg_object_operator_unbuild(vm, awidthshow);
		_hg_object_operator_unbuild(vm, begin);
		_hg_object_operator_unbuild(vm, bind);
		_hg_object_operator_unbuild(vm, bitshift);
		_hg_object_operator_unbuild(vm, bytesavailable);
		_hg_object_operator_unbuild(vm, cachestatus);
		_hg_object_operator_unbuild(vm, ceiling);
		_hg_object_operator_unbuild(vm, charpath);
		_hg_object_operator_unbuild(vm, clear);
		_hg_object_operator_unbuild(vm, cleardictstack);
		_hg_object_operator_unbuild(vm, cleartomark);
		_hg_object_operator_unbuild(vm, clip);
		_hg_object_operator_unbuild(vm, clippath);
		_hg_object_operator_unbuild(vm, closefile);
		_hg_object_operator_unbuild(vm, closepath);
		_hg_object_operator_unbuild(vm, concat);
		_hg_object_operator_unbuild(vm, concatmatrix);
		_hg_object_operator_unbuild(vm, copy);
		_hg_object_operator_unbuild(vm, copypage);
		_hg_object_operator_unbuild(vm, cos);
		_hg_object_operator_unbuild(vm, count);
		_hg_object_operator_unbuild(vm, countdictstack);
		_hg_object_operator_unbuild(vm, countexecstack);
		_hg_object_operator_unbuild(vm, counttomark);
		_hg_object_operator_unbuild(vm, currentdash);
		_hg_object_operator_unbuild(vm, currentdict);
		_hg_object_operator_unbuild(vm, currentfile);
		_hg_object_operator_unbuild(vm, currentflat);
		_hg_object_operator_unbuild(vm, currentfont);
		_hg_object_operator_unbuild(vm, currentgray);
		_hg_object_operator_unbuild(vm, currenthsbcolor);
		_hg_object_operator_unbuild(vm, currentlinecap);
		_hg_object_operator_unbuild(vm, currentlinejoin);
		_hg_object_operator_unbuild(vm, currentlinewidth);
		_hg_object_operator_unbuild(vm, currentmatrix);
		_hg_object_operator_unbuild(vm, currentmiterlimit);
		_hg_object_operator_unbuild(vm, currentpoint);
		_hg_object_operator_unbuild(vm, currentrgbcolor);
		_hg_object_operator_unbuild(vm, currentscreen);
		_hg_object_operator_unbuild(vm, currenttransfer);
		_hg_object_operator_unbuild(vm, curveto);
		_hg_object_operator_unbuild(vm, cvi);
		_hg_object_operator_unbuild(vm, cvlit);
		_hg_object_operator_unbuild(vm, cvn);
		_hg_object_operator_unbuild(vm, cvr);
		_hg_object_operator_unbuild(vm, cvrs);
		_hg_object_operator_unbuild(vm, cvx);
		_hg_object_operator_unbuild(vm, def);
		_hg_object_operator_unbuild(vm, defaultmatrix);
		_hg_object_operator_unbuild(vm, dict);
		_hg_object_operator_unbuild(vm, dictstack);
		_hg_object_operator_unbuild(vm, div);
		_hg_object_operator_unbuild(vm, dtransform);
		_hg_object_operator_unbuild(vm, dup);
		_hg_object_operator_unbuild(vm, echo);
		_hg_object_operator_unbuild(vm, eexec);
		_hg_object_operator_unbuild(vm, end);
		_hg_object_operator_unbuild(vm, eoclip);
		_hg_object_operator_unbuild(vm, eofill);
		_hg_object_operator_unbuild(vm, eq);
		_hg_object_operator_unbuild(vm, erasepage);
		_hg_object_operator_unbuild(vm, exch);
		_hg_object_operator_unbuild(vm, exec);
		_hg_object_operator_unbuild(vm, execstack);
		_hg_object_operator_unbuild(vm, executeonly);
		_hg_object_operator_unbuild(vm, exit);
		_hg_object_operator_unbuild(vm, exp);
		_hg_object_operator_unbuild(vm, file);
		_hg_object_operator_unbuild(vm, fill);
		_hg_object_operator_unbuild(vm, flattenpath);
		_hg_object_operator_unbuild(vm, flush);
		_hg_object_operator_unbuild(vm, flushfile);
		_hg_object_operator_unbuild(vm, FontDirectory);
		_hg_object_operator_unbuild(vm, for);
		_hg_object_operator_unbuild(vm, forall);
		_hg_object_operator_unbuild(vm, ge);
		_hg_object_operator_unbuild(vm, get);
		_hg_object_operator_unbuild(vm, getinterval);
		_hg_object_operator_unbuild(vm, grestore);
		_hg_object_operator_unbuild(vm, grestoreall);
		_hg_object_operator_unbuild(vm, gsave);
		_hg_object_operator_unbuild(vm, gt);
		_hg_object_operator_unbuild(vm, identmatrix);
		_hg_object_operator_unbuild(vm, idiv);
		_hg_object_operator_unbuild(vm, idtransform);
		_hg_object_operator_unbuild(vm, if);
		_hg_object_operator_unbuild(vm, ifelse);
		_hg_object_operator_unbuild(vm, image);
		_hg_object_operator_unbuild(vm, imagemask);
		_hg_object_operator_unbuild(vm, index);
		_hg_object_operator_unbuild(vm, initclip);
		_hg_object_operator_unbuild(vm, initgraphics);
		_hg_object_operator_unbuild(vm, initmatrix);
		_hg_object_operator_unbuild(vm, internaldict);
		_hg_object_operator_unbuild(vm, invertmatrix);
		_hg_object_operator_unbuild(vm, itransform);
		_hg_object_operator_unbuild(vm, known);
		_hg_object_operator_unbuild(vm, kshow);
		_hg_object_operator_unbuild(vm, le);
		_hg_object_operator_unbuild(vm, length);
		_hg_object_operator_unbuild(vm, lineto);
		_hg_object_operator_unbuild(vm, ln);
		_hg_object_operator_unbuild(vm, log);
		_hg_object_operator_unbuild(vm, loop);
		_hg_object_operator_unbuild(vm, lt);
		_hg_object_operator_unbuild(vm, makefont);
		_hg_object_operator_unbuild(vm, maxlength);
		_hg_object_operator_unbuild(vm, mod);
		_hg_object_operator_unbuild(vm, moveto);
		_hg_object_operator_unbuild(vm, mul);
		_hg_object_operator_unbuild(vm, ne);
		_hg_object_operator_unbuild(vm, neg);
		_hg_object_operator_unbuild(vm, newpath);
		_hg_object_operator_unbuild(vm, noaccess);
		_hg_object_operator_unbuild(vm, not);
		_hg_object_operator_unbuild(vm, nulldevice);
		_hg_object_operator_unbuild(vm, or);
		_hg_object_operator_unbuild(vm, pathbbox);
		_hg_object_operator_unbuild(vm, pathforall);
		_hg_object_operator_unbuild(vm, pop);
		_hg_object_operator_unbuild(vm, print);
		_hg_object_operator_unbuild(vm, put);
		_hg_object_operator_unbuild(vm, rand);
		_hg_object_operator_unbuild(vm, rcheck);
		_hg_object_operator_unbuild(vm, rcurveto);
		_hg_object_operator_unbuild(vm, read);
		_hg_object_operator_unbuild(vm, readhexstring);
		_hg_object_operator_unbuild(vm, readline);
		_hg_object_operator_unbuild(vm, readonly);
		_hg_object_operator_unbuild(vm, readstring);
		_hg_object_operator_unbuild(vm, repeat);
		_hg_object_operator_unbuild(vm, resetfile);
		_hg_object_operator_unbuild(vm, restore);
		_hg_object_operator_unbuild(vm, reversepath);
		_hg_object_operator_unbuild(vm, rlineto);
		_hg_object_operator_unbuild(vm, rmoveto);
		_hg_object_operator_unbuild(vm, roll);
		_hg_object_operator_unbuild(vm, rotate);
		_hg_object_operator_unbuild(vm, round);
		_hg_object_operator_unbuild(vm, rrand);
		_hg_object_operator_unbuild(vm, save);
		_hg_object_operator_unbuild(vm, scale);
		_hg_object_operator_unbuild(vm, scalefont);
		_hg_object_operator_unbuild(vm, search);
		_hg_object_operator_unbuild(vm, setcachedevice);
		_hg_object_operator_unbuild(vm, setcachelimit);
		_hg_object_operator_unbuild(vm, setcharwidth);
		_hg_object_operator_unbuild(vm, setdash);
		_hg_object_operator_unbuild(vm, setflat);
		_hg_object_operator_unbuild(vm, setfont);
		_hg_object_operator_unbuild(vm, setgray);
		_hg_object_operator_unbuild(vm, sethsbcolor);
		_hg_object_operator_unbuild(vm, setlinecap);
		_hg_object_operator_unbuild(vm, setlinejoin);
		_hg_object_operator_unbuild(vm, setlinewidth);
		_hg_object_operator_unbuild(vm, setmatrix);
		_hg_object_operator_unbuild(vm, setmiterlimit);
		_hg_object_operator_unbuild(vm, setrgbcolor);
		_hg_object_operator_unbuild(vm, setscreen);
		_hg_object_operator_unbuild(vm, settransfer);
		_hg_object_operator_unbuild(vm, show);
		_hg_object_operator_unbuild(vm, showpage);
		_hg_object_operator_unbuild(vm, sin);
		_hg_object_operator_unbuild(vm, sqrt);
		_hg_object_operator_unbuild(vm, srand);
		_hg_object_operator_unbuild(vm, status);
		_hg_object_operator_unbuild(vm, stop);
		_hg_object_operator_unbuild(vm, stopped);
		_hg_object_operator_unbuild(vm, string);
		_hg_object_operator_unbuild(vm, stringwidth);
		_hg_object_operator_unbuild(vm, stroke);
		_hg_object_operator_unbuild(vm, strokepath);
		_hg_object_operator_unbuild(vm, sub);
		_hg_object_operator_unbuild(vm, token);
		_hg_object_operator_unbuild(vm, transform);
		_hg_object_operator_unbuild(vm, translate);
		_hg_object_operator_unbuild(vm, truncate);
		_hg_object_operator_unbuild(vm, type);
		_hg_object_operator_unbuild(vm, usertime);
		_hg_object_operator_unbuild(vm, vmstatus);
		_hg_object_operator_unbuild(vm, wcheck);
		_hg_object_operator_unbuild(vm, where);
		_hg_object_operator_unbuild(vm, widthshow);
		_hg_object_operator_unbuild(vm, write);
		_hg_object_operator_unbuild(vm, writehexstring);
		_hg_object_operator_unbuild(vm, writestring);
		_hg_object_operator_unbuild(vm, xcheck);
		_hg_object_operator_unbuild(vm, xor);
	}

	__hg_operator_initialized = FALSE;

	return retval;
}

gboolean
hg_object_operator_invoke(hg_vm_t     *vm,
			  hg_object_t *object)
{
	hg_operatordata_t *data;

	hg_return_val_if_fail (vm != NULL, FALSE);
	hg_return_val_if_fail (object != NULL, FALSE);
	hg_return_val_if_fail (__hg_operator_initialized, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_OPERATOR (object), FALSE);

	data = HG_OBJECT_OPERATOR_DATA (object);

	return data->func(vm, object);
}

gboolean
hg_object_operator_compare(hg_object_t *object1,
			   hg_object_t *object2)
{
	hg_return_val_if_fail (object1 != NULL, FALSE);
	hg_return_val_if_fail (object2 != NULL, FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_OPERATOR (object1), FALSE);
	hg_return_val_if_fail (HG_OBJECT_IS_OPERATOR (object2), FALSE);

	/* XXX: no copy and dup functionalities are available so far.
	 * so just comparing a pointer should works enough
	 */
	return object1 == object2;
}

gchar *
hg_object_operator_dump(hg_object_t *object,
			gboolean     verbose)
{
	hg_operatordata_t *data;

	hg_return_val_if_fail (object != NULL, NULL);
	hg_return_val_if_fail (HG_OBJECT_IS_OPERATOR (object), NULL);

	data = HG_OBJECT_OPERATOR_DATA (object);

	return g_strndup((gchar *)data->name, HG_OBJECT_OPERATOR (object)->length);
}

const gchar *
hg_object_operator_get_name(hg_system_encoding_t encoding)
{
	const gchar *retval;

	hg_return_val_if_fail (encoding < HG_enc_END, NULL);
	hg_return_val_if_fail (__hg_operator_initialized, NULL);

	retval = __hg_system_encoding_names[encoding];

	hg_return_val_if_fail (retval != NULL, NULL);

	return retval;
}
