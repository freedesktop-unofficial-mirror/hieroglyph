/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmacros.h
 * Copyright (C) 2006-2007 Akira TAGOH
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
#ifndef __HIEROGLYPH__HGMACROS_H__
#define __HIEROGLYPH__HGMACROS_H__

#include <stdio.h>
#include <glib/gmacros.h>
#include <glib/gtypes.h>
#include <glib/gmem.h>
#include <glib/gmessages.h>
#include <hieroglyph/utils.h>


G_BEGIN_DECLS

#define hg_n_alignof(_hg_n_)						\
	((((_hg_n_) / ALIGNOF_VOID_P) + (((_hg_n_) % ALIGNOF_VOID_P) ? 1 : 0)) * ALIGNOF_VOID_P)
#ifdef DEBUG
#define hg_stacktrace()							\
	G_STMT_START {							\
		if (hg_is_stacktrace_enabled()) {			\
			gchar *__stacktrace__ = hg_get_stacktrace();	\
									\
			fprintf(stderr, "Stacktraces:\n%s\n", __stacktrace__); \
			g_free(__stacktrace__);				\
		}							\
	} G_STMT_END
#else /* !DEBUG */
#define hg_stacktrace()
#endif /* DEBUG */
#ifdef __GNUC__
#define _hg_return_if_fail_warning(__domain__,__func__,__expr__)	\
	G_STMT_START {							\
		if (hg_allow_warning_messages()) {			\
			g_return_if_fail_warning(__domain__,		\
						 __func__,		\
						 __expr__);		\
		}							\
	} G_STMT_END
#define _hg_return_after_eval_if_fail(__expr__,__eval__)		\
	G_STMT_START {							\
		if (G_LIKELY(__expr__)) {				\
		} else {						\
			_hg_return_if_fail_warning(G_LOG_DOMAIN,	\
						   __PRETTY_FUNCTION__,	\
						   #__expr__);		\
			__eval__;					\
			return;						\
		}							\
	} G_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	G_STMT_START {							\
		if (G_LIKELY(__expr__)) {				\
		} else {						\
			_hg_return_if_fail_warning(G_LOG_DOMAIN,	\
						   __PRETTY_FUNCTION__,	\
						   #__expr__);		\
			__eval__;					\
			return (__val__);				\
		}							\
	} G_STMT_END
#else /* !__GNUC__ */
#define _hg_return_after_eval_if_fail(__expr__,__eval__)		\
	G_STMT_START {							\
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
	} G_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	G_STMT_START {							\
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
	} G_STMT_END
#endif /* __GNUC__ */

#define hg_return_if_fail(__expr__)					\
	_hg_return_after_eval_if_fail(__expr__,hg_stacktrace())
#define hg_return_val_if_fail(__expr__,__val__)				\
	_hg_return_val_after_eval_if_fail(__expr__,__val__,hg_stacktrace())
#define hg_return_after_eval_if_fail(__expr__,__eval__)			\
	_hg_return_after_eval_if_fail(__expr__,hg_stacktrace();__eval__)
#define hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	_hg_return_val_after_eval_if_fail(__expr__,__val__,hg_stacktrace();__eval__)


G_END_DECLS

#endif /* __HIEROGLYPH_HGMACROS_H__ */
