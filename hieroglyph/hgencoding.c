/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgencoding.c
 * Copyright (C) 2010 Akira TAGOH
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

#include <glib.h>
#include "hgerror.h"
#include "hgencoding.h"


static GHashTable *__hg_encoding_index_table = NULL;
static const gchar *__hg_system_encodings =
	"\0"
	"abs\0"
	"add\0"
	"aload\0"
	"anchorsearch\0"
	"and\0"
	"arc\0"
	"arcn\0"
	"arct\0"
	"arcto\0"
	"array\0"
	"ashow\0"
	"astore\0"
	"awidthshow\0"
	"begin\0"
	"bind\0"
	"bitshift\0"
	"ceiling\0"
	"charpath\0"
	"clear\0"
	"cleartomark\0"
	"clip\0"
	"clippath\0"
	"closepath\0"
	"concat\0"
	"concatmatrix\0"
	"copy\0"
	"count\0"
	"counttomark\0"
	"currentcmykcolor\0"
	"currentdash\0"
	"currentdict\0"
	"currentfile\0"
	"currentfont\0"
	"currentgray\0"
	"currentgstate\0"
	"currenthsbcolor\0"
	"currentlinecap\0"
	"currentlinejoin\0"
	"currentlinewidth\0"
	"currentmatrix\0"
	"currentpoint\0"
	"currentrgbcolor\0"
	"currentshared\0"
	"curveto\0"
	"cvi\0"
	"cvlit\0"
	"cvn\0"
	"cvr\0"
	"cvrs\0"
	"cvs\0"
	"cvx\0"
	"def\0"
	"defineusername\0"
	"dict\0"
	"div\0"
	"dtransform\0"
	"dup\0"
	"end\0"
	"eoclip\0"
	"eofill\0"
	"eoviewclip\0"
	"eq\0"
	"exch\0"
	"exec\0"
	"exit\0"
	"file\0"
	"fill\0"
	"findfont\0"
	"flattenpath\0"
	"floor\0"
	"flush\0"
	"flushfile\0"
	"for\0"
	"forall\0"
	"ge\0"
	"get\0"
	"getinterval\0"
	"grestore\0"
	"gsave\0"
	"gstate\0"
	"gt\0"
	"identmatrix\0"
	"idiv\0"
	"idtransform\0"
	"if\0"
	"ifelse\0"
	"image\0"
	"imagemask\0"
	"index\0"
	"ineofill\0"
	"infill\0"
	"initviewclip\0"
	"inueofill\0"
	"inufill\0"
	"invertmatrix\0"
	"itransform\0"
	"known\0"
	"le\0"
	"length\0"
	"lineto\0"
	"load\0"
	"loop\0"
	"lt\0"
	"makefont\0"
	"matrix\0"
	"maxlength\0"
	"mod\0"
	"moveto\0"
	"mul\0"
	"ne\0"
	"neg\0"
	"newpath\0"
	"not\0"
	"null\0"
	"or\0"
	"pathbbox\0"
	"pathforall\0"
	"pop\0"
	"print\0"
	"printobject\0"
	"put\0"
	"putinterval\0"
	"rcurveto\0"
	"read\0"
	"readhexstring\0"
	"readline\0"
	"readstring\0"
	"rectclip\0"
	"rectfill\0"
	"rectstroke\0"
	"rectviewclip\0"
	"repeat\0"
	"restore\0"
	"rlineto\0"
	"rmoveto\0"
	"roll\0"
	"rotate\0"
	"round\0"
	"save\0"
	"scale\0"
	"scalefont\0"
	"search\0"
	"selectfont\0"
	"setbbox\0"
	"setcachedevice\0"
	"setcachedevice2\0"
	"setcharwidth\0"
	"setcmykcolor\0"
	"setdash\0"
	"setfont\0"
	"setgray\0"
	"setgstate\0"
	"sethsbcolor\0"
	"setlinecap\0"
	"setlinejoin\0"
	"setlinewidth\0"
	"setmatrix\0"
	"setrgbcolor\0"
	"setshared\0"
	"shareddict\0"
	"show\0"
	"showpage\0"
	"stop\0"
	"stopped\0"
	"store\0"
	"string\0"
	"stringwidth\0"
	"stroke\0"
	"strokepath\0"
	"sub\0"
	"systemdict\0"
	"token\0"
	"transform\0"
	"translate\0"
	"truncate\0"
	"type\0"
	"uappend\0"
	"ucache\0"
	"ueofill\0"
	"ufill\0"
	"undef\0"
	"upath\0"
	"userdict\0"
	"ustroke\0"
	"viewclip\0"
	"viewclippath\0"
	"where\0"
	"widthshow\0"
	"write\0"
	"writehexstring\0"
	"writeobject\0"
	"writestring\0"
	"wtranslation\0"
	"xor\0"
	"xshow\0"
	"xyshow\0"
	"yshow\0"
	"FontDirectory\0"
	"SharedFontDirectory\0"
	"Courier\0"
	"Courier_Bold\0"
	"Courier_BoldOblique\0"
	"Courier_Oblique\0"
	"Helvetica\0"
	"Helvetica_Bold\0"
	"Helvetica_BoldOblique\0"
	"Helvetica_Oblique\0"
	"Symbol\0"
	"Times_Bold\0"
	"Times_BoldItalic\0"
	"Times_Italic\0"
	"Times_Roman\0"
	"execuserobject\0"
	"currentcolor\0"
	"currentcolorspace\0"
	"currentglobal\0"
	"execform\0"
	"filter\0"
	"findresource\0"
	"globaldict\0"
	"makepattern\0"
	"setcolor\0"
	"setcolorspace\0"
	"setglobal\0"
	"setpagedevice\0"
	"setpattern\0"
	"sym_eq\0"
	"sym_eqeq\0"
	"ISOLatin1Encoding\0"
	"StandardEncoding\0"
	"sym_left_square_bracket\0"
	"sym_right_square_bracket\0"
	"atan\0"
	"banddevice\0"
	"bytesavailable\0"
	"cachestatus\0"
	"closefile\0"
	"colorimage\0"
	"condition\0"
	"copypage\0"
	"cos\0"
	"countdictstack\0"
	"countexecstack\0"
	"cshow\0"
	"currentblackgeneration\0"
	"currentcacheparams\0"
	"currentcolorscreen\0"
	"currentcolortransfer\0"
	"currentcontext\0"
	"currentflat\0"
	"currenthalftone\0"
	"currenthalftonephase\0"
	"currentmiterlimit\0"
	"currentobjectformat\0"
	"currentpacking\0"
	"currentscreen\0"
	"currentstrokeadjust\0"
	"currenttransfer\0"
	"currentundercolorremoval\0"
	"defaultmatrix\0"
	"definefont\0"
	"deletefile\0"
	"detach\0"
	"deviceinfo\0"
	"dictstack\0"
	"echo\0"
	"erasepage\0"
	"errordict\0"
	"execstack\0"
	"executeonly\0"
	"exp\0"
	"false\0"
	"filenameforall\0"
	"fileposition\0"
	"fork\0"
	"framedevice\0"
	"grestoreall\0"
	"handleerror\0"
	"initclip\0"
	"initgraphics\0"
	"initmatrix\0"
	"instroke\0"
	"inustroke\0"
	"join\0"
	"kshow\0"
	"ln\0"
	"lock\0"
	"log\0"
	"mark\0"
	"monitor\0"
	"noaccess\0"
	"notify\0"
	"nulldevice\0"
	"packedarray\0"
	"quit\0"
	"rand\0"
	"rcheck\0"
	"readonly\0"
	"realtime\0"
	"renamefile\0"
	"renderbands\0"
	"resetfile\0"
	"reversepath\0"
	"rootfont\0"
	"rrand\0"
	"run\0"
	"scheck\0"
	"setblackgeneration\0"
	"setcachelimit\0"
	"setcacheparams\0"
	"setcolorscreen\0"
	"setcolortransfer\0"
	"setfileposition\0"
	"setflat\0"
	"sethalftone\0"
	"sethalftonephase\0"
	"setmiterlimit\0"
	"setobjectformat\0"
	"setpacking\0"
	"setscreen\0"
	"setstrokeadjust\0"
	"settransfer\0"
	"setucacheparams\0"
	"setundercolorremoval\0"
	"sin\0"
	"sqrt\0"
	"srand\0"
	"stack\0"
	"status\0"
	"statusdict\0"
	"true\0"
	"ucachestatus\0"
	"undefinefont\0"
	"usertime\0"
	"ustrokepath\0"
	"version\0"
	"vmreclaim\0"
	"vmstatus\0"
	"wait\0"
	"wcheck\0"
	"xcheck\0"
	"yield\0"
	"defineuserobject\0"
	"undefineuserobject\0"
	"UserObjects\0"
	"cleardictstack\0"
	"A\0"
	"B\0"
	"C\0"
	"D\0"
	"E\0"
	"F\0"
	"G\0"
	"H\0"
	"I\0"
	"J\0"
	"K\0"
	"L\0"
	"M\0"
	"N\0"
	"O\0"
	"P\0"
	"Q\0"
	"R\0"
	"S\0"
	"T\0"
	"U\0"
	"V\0"
	"W\0"
	"X\0"
	"Y\0"
	"Z\0"
	"a\0"
	"b\0"
	"c\0"
	"d\0"
	"e\0"
	"f\0"
	"g\0"
	"h\0"
	"i\0"
	"j\0"
	"k\0"
	"l\0"
	"m\0"
	"n\0"
	"o\0"
	"p\0"
	"q\0"
	"r\0"
	"s\0"
	"t\0"
	"u\0"
	"v\0"
	"w\0"
	"x\0"
	"y\0"
	"z\0"
	"setvmthreshold\0"
	"sym_begin_dict_mark\0"
	"sym_end_dict_mark\0"
	"currentcolorrendering\0"
	"currentdevparams\0"
	"currentoverprint\0"
	"currentpagedevice\0"
	"currentsystemparams\0"
	"currentuserparams\0"
	"defineresource\0"
	"findencoding\0"
	"gcheck\0"
	"glyphshow\0"
	"languagelevel\0"
	"product\0"
	"pstack\0"
	"resourceforall\0"
	"resourcestatus\0"
	"revision\0"
	"serialnumber\0"
	"setcolorrendering\0"
	"setdevparams\0"
	"setoverprint\0"
	"setsystemparams\0"
	"setuserparams\0"
	"startjob\0"
	"undefineresource\0"
	"GlobalFontDirectory\0"
	"ASCII85Decode\0"
	"ASCII85Encode\0"
	"ASCIIHexDecode\0"
	"ASCIIHexEncode\0"
	"CCITTFaxDecode\0"
	"CCITTFaxEncode\0"
	"DCTDecode\0"
	"DCTEncode\0"
	"LZWDecode\0"
	"LZWEncode\0"
	"NullEncode\0"
	"RunLengthDecode\0"
	"RunLengthEncode\0"
	"SubFileDecode\0"
	"CIEBasedA\0"
	"CIEBasedABC\0"
	"DeviceCMYK\0"
	"DeviceGray\0"
	"DeviceRGB\0"
	"Indexed\0"
	"Pattern\0"
	"Separation\0"
	"CIEBasedDEF\0"
	"CIEBasedDEFG\0"
	"DeviceN\0";
static gint __hg_system_encodings_offset_indexes[] = {
	1,	/* abs */
	5,	/* add */
	9,	/* aload */
	15,	/* anchorsearch */
	28,	/* and */
	32,	/* arc */
	36,	/* arcn */
	41,	/* arct */
	46,	/* arcto */
	52,	/* array */
	58,	/* ashow */
	64,	/* astore */
	71,	/* awidthshow */
	82,	/* begin */
	88,	/* bind */
	93,	/* bitshift */
	102,	/* ceiling */
	110,	/* charpath */
	119,	/* clear */
	125,	/* cleartomark */
	137,	/* clip */
	142,	/* clippath */
	151,	/* closepath */
	161,	/* concat */
	168,	/* concatmatrix */
	181,	/* copy */
	186,	/* count */
	192,	/* counttomark */
	204,	/* currentcmykcolor */
	221,	/* currentdash */
	233,	/* currentdict */
	245,	/* currentfile */
	257,	/* currentfont */
	269,	/* currentgray */
	281,	/* currentgstate */
	295,	/* currenthsbcolor */
	311,	/* currentlinecap */
	326,	/* currentlinejoin */
	342,	/* currentlinewidth */
	359,	/* currentmatrix */
	373,	/* currentpoint */
	386,	/* currentrgbcolor */
	402,	/* currentshared */
	416,	/* curveto */
	424,	/* cvi */
	428,	/* cvlit */
	434,	/* cvn */
	438,	/* cvr */
	442,	/* cvrs */
	447,	/* cvs */
	451,	/* cvx */
	455,	/* def */
	459,	/* defineusername */
	474,	/* dict */
	479,	/* div */
	483,	/* dtransform */
	494,	/* dup */
	498,	/* end */
	502,	/* eoclip */
	509,	/* eofill */
	516,	/* eoviewclip */
	527,	/* eq */
	530,	/* exch */
	535,	/* exec */
	540,	/* exit */
	545,	/* file */
	550,	/* fill */
	555,	/* findfont */
	564,	/* flattenpath */
	576,	/* floor */
	582,	/* flush */
	588,	/* flushfile */
	598,	/* for */
	602,	/* forall */
	609,	/* ge */
	612,	/* get */
	616,	/* getinterval */
	628,	/* grestore */
	637,	/* gsave */
	643,	/* gstate */
	650,	/* gt */
	653,	/* identmatrix */
	665,	/* idiv */
	670,	/* idtransform */
	682,	/* if */
	685,	/* ifelse */
	692,	/* image */
	698,	/* imagemask */
	708,	/* index */
	714,	/* ineofill */
	723,	/* infill */
	730,	/* initviewclip */
	743,	/* inueofill */
	753,	/* inufill */
	761,	/* invertmatrix */
	774,	/* itransform */
	785,	/* known */
	791,	/* le */
	794,	/* length */
	801,	/* lineto */
	808,	/* load */
	813,	/* loop */
	818,	/* lt */
	821,	/* makefont */
	830,	/* matrix */
	837,	/* maxlength */
	847,	/* mod */
	851,	/* moveto */
	858,	/* mul */
	862,	/* ne */
	865,	/* neg */
	869,	/* newpath */
	877,	/* not */
	881,	/* null */
	886,	/* or */
	889,	/* pathbbox */
	898,	/* pathforall */
	909,	/* pop */
	913,	/* print */
	919,	/* printobject */
	931,	/* put */
	935,	/* putinterval */
	947,	/* rcurveto */
	956,	/* read */
	961,	/* readhexstring */
	975,	/* readline */
	984,	/* readstring */
	995,	/* rectclip */
	1004,	/* rectfill */
	1013,	/* rectstroke */
	1024,	/* rectviewclip */
	1037,	/* repeat */
	1044,	/* restore */
	1052,	/* rlineto */
	1060,	/* rmoveto */
	1068,	/* roll */
	1073,	/* rotate */
	1080,	/* round */
	1086,	/* save */
	1091,	/* scale */
	1097,	/* scalefont */
	1107,	/* search */
	1114,	/* selectfont */
	1125,	/* setbbox */
	1133,	/* setcachedevice */
	1148,	/* setcachedevice2 */
	1164,	/* setcharwidth */
	1177,	/* setcmykcolor */
	1190,	/* setdash */
	1198,	/* setfont */
	1206,	/* setgray */
	1214,	/* setgstate */
	1224,	/* sethsbcolor */
	1236,	/* setlinecap */
	1247,	/* setlinejoin */
	1259,	/* setlinewidth */
	1272,	/* setmatrix */
	1282,	/* setrgbcolor */
	1294,	/* setshared */
	1304,	/* shareddict */
	1315,	/* show */
	1320,	/* showpage */
	1329,	/* stop */
	1334,	/* stopped */
	1342,	/* store */
	1348,	/* string */
	1355,	/* stringwidth */
	1367,	/* stroke */
	1374,	/* strokepath */
	1385,	/* sub */
	1389,	/* systemdict */
	1400,	/* token */
	1406,	/* transform */
	1416,	/* translate */
	1426,	/* truncate */
	1435,	/* type */
	1440,	/* uappend */
	1448,	/* ucache */
	1455,	/* ueofill */
	1463,	/* ufill */
	1469,	/* undef */
	1475,	/* upath */
	1481,	/* userdict */
	1490,	/* ustroke */
	1498,	/* viewclip */
	1507,	/* viewclippath */
	1520,	/* where */
	1526,	/* widthshow */
	1536,	/* write */
	1542,	/* writehexstring */
	1557,	/* writeobject */
	1569,	/* writestring */
	1581,	/* wtranslation */
	1594,	/* xor */
	1598,	/* xshow */
	1604,	/* xyshow */
	1611,	/* yshow */
	1617,	/* FontDirectory */
	1631,	/* SharedFontDirectory */
	1651,	/* Courier */
	1659,	/* Courier_Bold */
	1672,	/* Courier_BoldOblique */
	1692,	/* Courier_Oblique */
	1708,	/* Helvetica */
	1718,	/* Helvetica_Bold */
	1733,	/* Helvetica_BoldOblique */
	1755,	/* Helvetica_Oblique */
	1773,	/* Symbol */
	1780,	/* Times_Bold */
	1791,	/* Times_BoldItalic */
	1808,	/* Times_Italic */
	1821,	/* Times_Roman */
	1833,	/* execuserobject */
	1848,	/* currentcolor */
	1861,	/* currentcolorspace */
	1879,	/* currentglobal */
	1893,	/* execform */
	1902,	/* filter */
	1909,	/* findresource */
	1922,	/* globaldict */
	1933,	/* makepattern */
	1945,	/* setcolor */
	1954,	/* setcolorspace */
	1968,	/* setglobal */
	1978,	/* setpagedevice */
	1992,	/* setpattern */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	2003,	/* sym_eq */
	2010,	/* sym_eqeq */
	2019,	/* ISOLatin1Encoding */
	2037,	/* StandardEncoding */
	2054,	/* sym_left_square_bracket */
	2078,	/* sym_right_square_bracket */
	2103,	/* atan */
	2108,	/* banddevice */
	2119,	/* bytesavailable */
	2134,	/* cachestatus */
	2146,	/* closefile */
	2156,	/* colorimage */
	2167,	/* condition */
	2177,	/* copypage */
	2186,	/* cos */
	2190,	/* countdictstack */
	2205,	/* countexecstack */
	2220,	/* cshow */
	2226,	/* currentblackgeneration */
	2249,	/* currentcacheparams */
	2268,	/* currentcolorscreen */
	2287,	/* currentcolortransfer */
	2308,	/* currentcontext */
	2323,	/* currentflat */
	2335,	/* currenthalftone */
	2351,	/* currenthalftonephase */
	2372,	/* currentmiterlimit */
	2390,	/* currentobjectformat */
	2410,	/* currentpacking */
	2425,	/* currentscreen */
	2439,	/* currentstrokeadjust */
	2459,	/* currenttransfer */
	2475,	/* currentundercolorremoval */
	2500,	/* defaultmatrix */
	2514,	/* definefont */
	2525,	/* deletefile */
	2536,	/* detach */
	2543,	/* deviceinfo */
	2554,	/* dictstack */
	2564,	/* echo */
	2569,	/* erasepage */
	2579,	/* errordict */
	2589,	/* execstack */
	2599,	/* executeonly */
	2611,	/* exp */
	2615,	/* false */
	2621,	/* filenameforall */
	2636,	/* fileposition */
	2649,	/* fork */
	2654,	/* framedevice */
	2666,	/* grestoreall */
	2678,	/* handleerror */
	2690,	/* initclip */
	2699,	/* initgraphics */
	2712,	/* initmatrix */
	2723,	/* instroke */
	2732,	/* inustroke */
	2742,	/* join */
	2747,	/* kshow */
	2753,	/* ln */
	2756,	/* lock */
	2761,	/* log */
	2765,	/* mark */
	2770,	/* monitor */
	2778,	/* noaccess */
	2787,	/* notify */
	2794,	/* nulldevice */
	2805,	/* packedarray */
	2817,	/* quit */
	2822,	/* rand */
	2827,	/* rcheck */
	2834,	/* readonly */
	2843,	/* realtime */
	2852,	/* renamefile */
	2863,	/* renderbands */
	2875,	/* resetfile */
	2885,	/* reversepath */
	2897,	/* rootfont */
	2906,	/* rrand */
	2912,	/* run */
	2916,	/* scheck */
	2923,	/* setblackgeneration */
	2942,	/* setcachelimit */
	2956,	/* setcacheparams */
	2971,	/* setcolorscreen */
	2986,	/* setcolortransfer */
	3003,	/* setfileposition */
	3019,	/* setflat */
	3027,	/* sethalftone */
	3039,	/* sethalftonephase */
	3056,	/* setmiterlimit */
	3070,	/* setobjectformat */
	3086,	/* setpacking */
	3097,	/* setscreen */
	3107,	/* setstrokeadjust */
	3123,	/* settransfer */
	3135,	/* setucacheparams */
	3151,	/* setundercolorremoval */
	3172,	/* sin */
	3176,	/* sqrt */
	3181,	/* srand */
	3187,	/* stack */
	3193,	/* status */
	3200,	/* statusdict */
	3211,	/* true */
	3216,	/* ucachestatus */
	3229,	/* undefinefont */
	3242,	/* usertime */
	3251,	/* ustrokepath */
	3263,	/* version */
	3271,	/* vmreclaim */
	3281,	/* vmstatus */
	3290,	/* wait */
	3295,	/* wcheck */
	3302,	/* xcheck */
	3309,	/* yield */
	3315,	/* defineuserobject */
	3332,	/* undefineuserobject */
	3351,	/* UserObjects */
	3363,	/* cleardictstack */
	3378,	/* A */
	3380,	/* B */
	3382,	/* C */
	3384,	/* D */
	3386,	/* E */
	3388,	/* F */
	3390,	/* G */
	3392,	/* H */
	3394,	/* I */
	3396,	/* J */
	3398,	/* K */
	3400,	/* L */
	3402,	/* M */
	3404,	/* N */
	3406,	/* O */
	3408,	/* P */
	3410,	/* Q */
	3412,	/* R */
	3414,	/* S */
	3416,	/* T */
	3418,	/* U */
	3420,	/* V */
	3422,	/* W */
	3424,	/* X */
	3426,	/* Y */
	3428,	/* Z */
	3430,	/* a */
	3432,	/* b */
	3434,	/* c */
	3436,	/* d */
	3438,	/* e */
	3440,	/* f */
	3442,	/* g */
	3444,	/* h */
	3446,	/* i */
	3448,	/* j */
	3450,	/* k */
	3452,	/* l */
	3454,	/* m */
	3456,	/* n */
	3458,	/* o */
	3460,	/* p */
	3462,	/* q */
	3464,	/* r */
	3466,	/* s */
	3468,	/* t */
	3470,	/* u */
	3472,	/* v */
	3474,	/* w */
	3476,	/* x */
	3478,	/* y */
	3480,	/* z */
	3482,	/* setvmthreshold */
	3497,	/* sym_begin_dict_mark */
	3517,	/* sym_end_dict_mark */
	3535,	/* currentcolorrendering */
	3557,	/* currentdevparams */
	3574,	/* currentoverprint */
	3591,	/* currentpagedevice */
	3609,	/* currentsystemparams */
	3629,	/* currentuserparams */
	3647,	/* defineresource */
	3662,	/* findencoding */
	3675,	/* gcheck */
	3682,	/* glyphshow */
	3692,	/* languagelevel */
	3706,	/* product */
	3714,	/* pstack */
	3721,	/* resourceforall */
	3736,	/* resourcestatus */
	3751,	/* revision */
	3760,	/* serialnumber */
	3773,	/* setcolorrendering */
	3791,	/* setdevparams */
	3804,	/* setoverprint */
	3817,	/* setsystemparams */
	3833,	/* setuserparams */
	3847,	/* startjob */
	3856,	/* undefineresource */
	3873,	/* GlobalFontDirectory */
	3893,	/* ASCII85Decode */
	3907,	/* ASCII85Encode */
	3921,	/* ASCIIHexDecode */
	3936,	/* ASCIIHexEncode */
	3951,	/* CCITTFaxDecode */
	3966,	/* CCITTFaxEncode */
	3981,	/* DCTDecode */
	3991,	/* DCTEncode */
	4001,	/* LZWDecode */
	4011,	/* LZWEncode */
	4021,	/* NullEncode */
	4032,	/* RunLengthDecode */
	4048,	/* RunLengthEncode */
	4064,	/* SubFileDecode */
	4078,	/* CIEBasedA */
	4088,	/* CIEBasedABC */
	4100,	/* DeviceCMYK */
	4111,	/* DeviceGray */
	4122,	/* DeviceRGB */
	4132,	/* Indexed */
	4140,	/* Pattern */
	4148,	/* Separation */
	4159,	/* CIEBasedDEF */
	4171,	/* CIEBasedDEFG */
	4184,	/* DeviceN */
};

/*< private >*/

/*< public >*/
/**
 * hg_encoding_init:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_encoding_init(void)
{
	hg_system_encoding_t i;

	if (__hg_encoding_index_table)
		return TRUE;

	__hg_encoding_index_table = g_hash_table_new(g_str_hash,
						     g_str_equal);
	if (!__hg_encoding_index_table)
		return FALSE;
	for (i = 0; i < HG_enc_POSTSCRIPT_RESERVED_END; i++) {
		const gchar *n = hg_encoding_get_system_encoding_name(i);

		if (n && *n != '\0') {
			g_hash_table_insert(__hg_encoding_index_table,
					    (gpointer)n, GINT_TO_POINTER (i+1));
		}
	}

	return TRUE;
}

/**
 * hg_encoding_tini:
 *
 * FIXME
 */
void
hg_encoding_tini(void)
{
	if (__hg_encoding_index_table) {
		g_hash_table_destroy(__hg_encoding_index_table);
		__hg_encoding_index_table = NULL;
	}
}

/**
 * hg_encoding_get_system_encoding_name:
 * @encoding:
 *
 * FIXME
 *
 * Returns:
 */
const gchar *
hg_encoding_get_system_encoding_name(hg_system_encoding_t encoding)
{
	hg_return_val_if_fail (encoding < HG_enc_POSTSCRIPT_RESERVED_END, NULL);

	return &__hg_system_encodings[__hg_system_encodings_offset_indexes[encoding]];
}

/**
 * hg_encoding_lookup_system_encoding:
 * @name:
 *
 * FIXME
 *
 * Returns:
 */
hg_system_encoding_t
hg_encoding_lookup_system_encoding(const gchar *name)
{
	gpointer result;

	hg_return_val_if_fail (name != NULL, HG_enc_END);
	hg_return_val_if_fail (__hg_encoding_index_table != NULL, HG_enc_END);

	result = g_hash_table_lookup(__hg_encoding_index_table, name);
	if (!result)
		return HG_enc_END;

	return GPOINTER_TO_INT (result) - 1;
}
