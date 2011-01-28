/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgerror.h
 * Copyright (C) 2006-2011 Akira TAGOH
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

#ifndef __HIEROGLYPH_HGERROR_H__
#define __HIEROGLYPH_HGERROR_H__

#include <stdio.h>
#include <errno.h>
#include <glib.h>
#include <hieroglyph/hgmacros.h>

HG_BEGIN_DECLS

#define HG_ERROR	hg_error_quark()

typedef enum _hg_vm_error_t		hg_vm_error_t;

enum _hg_vm_error_t {
	HG_VM_e_dictfull = 1,
	HG_VM_e_dictstackoverflow,
	HG_VM_e_dictstackunderflow,
	HG_VM_e_execstackoverflow,
	HG_VM_e_handleerror,
	HG_VM_e_interrupt,
	HG_VM_e_invalidaccess,
	HG_VM_e_invalidexit,
	HG_VM_e_invalidfileaccess,
	HG_VM_e_invalidfont,
	HG_VM_e_invalidrestore,
	HG_VM_e_ioerror,
	HG_VM_e_limitcheck,
	HG_VM_e_nocurrentpoint,
	HG_VM_e_rangecheck,
	HG_VM_e_stackoverflow,
	HG_VM_e_stackunderflow,
	HG_VM_e_syntaxerror,
	HG_VM_e_timeout,
	HG_VM_e_typecheck,
	HG_VM_e_undefined,
	HG_VM_e_undefinedfilename,
	HG_VM_e_undefinedresult,
	HG_VM_e_unmatchedmark,
	HG_VM_e_unregistered,
	HG_VM_e_VMerror,
	HG_VM_e_configurationerror,
	HG_VM_e_undefinedresource,
	HG_VM_e_END
};

#ifdef HG_DEBUG
/* evaluate x if the debugging build */
#define d(x)	x

#define hg_stacktrace()							\
	HG_STMT_START {							\
		if (hg_is_stacktrace_enabled()) {			\
			gchar *__stacktrace__ = hg_get_stacktrace();	\
									\
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,	\
			      "Stacktrace:\n%s\n", __stacktrace__);	\
			g_free(__stacktrace__);				\
		}							\
	} HG_STMT_END
#else /* !HG_DEBUG */
/* ignore x if not in the debugging build */
#define d(x)

#define hg_stacktrace()

#endif /* HG_DEBUG */

#ifdef __GNUC__
#define _hg_return_if_fail_warning(__domain__,__func__,__expr__)	\
	HG_STMT_START {							\
		g_return_if_fail_warning(__domain__,			\
					 __func__,			\
					 __expr__);			\
	} HG_STMT_END
#define _hg_return_after_eval_if_fail(__expr__,__eval__)		\
	HG_STMT_START {							\
		if (G_LIKELY(__expr__)) {				\
		} else {						\
			_hg_return_if_fail_warning(G_LOG_DOMAIN,	\
						   __PRETTY_FUNCTION__,	\
						   #__expr__);		\
			__eval__;					\
			return;						\
		}							\
	} HG_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	HG_STMT_START {							\
		if (G_LIKELY(__expr__)) {				\
		} else {						\
			_hg_return_if_fail_warning(G_LOG_DOMAIN,	\
						   __PRETTY_FUNCTION__,	\
						   #__expr__);		\
			__eval__;					\
			return (__val__);				\
		}							\
	} HG_STMT_END
#define _hg_gerror_on_fail(__expr__,__err__,__code__)			\
	HG_STMT_START {							\
		hg_stacktrace();					\
		if ((__err__)) {					\
			g_set_error((__err__), HG_ERROR, __code__,	\
				    "%s: assertion `%s' failed",	\
				    __PRETTY_FUNCTION__,		\
				    #__expr__);				\
		}							\
	} HG_STMT_END
#else /* !__GNUC__ */
#define _hg_return_after_eval_if_fail(__expr__,__eval__)		\
	HG_STMT_START {							\
		if (__expr__) {						\
		} else {						\
			g_log(G_LOG_DOMAIN,				\
			      G_LOG_LEVEL_CRITICAL,			\
			      "file %s: line %d: assertion `%s' failed", \
			      __FILE__,					\
			      __LINE__,					\
			      #__expr__);				\
			__eval__;					\
			return;						\
		}							\
	} HG_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	HG_STMT_START {							\
		if (__expr__) {						\
		} else {						\
			g_log(G_LOG_DOMAIN,				\
			      G_LOG_LEVEL_CRITICAL,			\
			      "file %s: line %d: assertion `%s' failed", \
			      __FILE__,					\
			      __LINE__,					\
			      #__expr__);				\
			__eval__;					\
			return (__val__);				\
		}							\
	} HG_STMT_END
#define _hg_error_if_fail(__expr__,__err__)				\
	HG_STMT_START {							\
		hg_stacktrace();					\
		if ((__err__)) {					\
			g_set_error((__err__), HG_ERROR, HG_VM_e_VMerror, \
				    "file %s: line %d: assertion `%s' failed", \
				    __FILE__,				\
				    __LINE__,				\
				    #__expr__);				\
		}							\
	} HG_STMT_END
#endif /* __GNUC__ */

#define hg_return_if_fail(__expr__)					\
	_hg_return_after_eval_if_fail(__expr__,hg_stacktrace())
#define hg_return_val_if_fail(__expr__,__val__)				\
	_hg_return_val_after_eval_if_fail(__expr__,__val__,hg_stacktrace())
#define hg_return_after_eval_if_fail(__expr__,__eval__)			\
	_hg_return_after_eval_if_fail(__expr__,hg_stacktrace();__eval__)
#define hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	_hg_return_val_after_eval_if_fail(__expr__,__val__,hg_stacktrace();__eval__)
#define hg_return_with_gerror_if_fail(__expr__,__err__,__code__)	\
	_hg_return_after_eval_if_fail(__expr__,_hg_gerror_on_fail(__expr__,__err__,__code__))
#define hg_return_val_with_gerror_if_fail(__expr__,__val__,__err__,__code__) \
	_hg_return_val_after_eval_if_fail(__expr__,__val__,_hg_gerror_on_fail(__expr__,__err__,__code__))
#define hg_return_val_with_gerror_after_eval_if_fail(__expr__,__val__,__eval__,__err__,__code__) \
	_hg_return_val_after_eval_if_fail(__expr__,__val__,_hg_gerror_on_fail(__expr__,__err__,__code__);__eval__)


gchar    *hg_get_stacktrace       (void) G_GNUC_MALLOC;
void      hg_use_stacktrace       (gboolean flag);
gboolean  hg_is_stacktrace_enabled(void);
GQuark    hg_error_quark          (void);


HG_END_DECLS

#endif /* __HIEROGLYPH_HGERROR_H__ */
