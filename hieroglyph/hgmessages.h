/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmessage.h
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

#ifndef __HIEROGLYPH_HGMESSAGE_H__
#define __HIEROGLYPH_HGMESSAGE_H__

#include <stdarg.h>
#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

typedef enum _hg_message_type_t		hg_message_type_t;
typedef enum _hg_message_flags_t	hg_message_flags_t;
typedef enum _hg_message_category_t	hg_message_category_t;
typedef void (* hg_message_func_t)	(hg_message_type_t      type,
					 hg_message_flags_t     flags,
					 hg_message_category_t  category,
					 const hg_char_t       *message,
					 hg_pointer_t           user_data);

enum _hg_message_type_t {
	HG_MSG_0 = 0,
	HG_MSG_FATAL,
	HG_MSG_CRITICAL,
	HG_MSG_WARNING,
	HG_MSG_INFO,
	HG_MSG_DEBUG,
	HG_MSG_END
};
enum _hg_message_flags_t {
	HG_MSG_FLAG_NONE	= 0,
	HG_MSG_FLAG_NO_LINEFEED	= (1 << 0),
	HG_MSG_FLAG_NO_PREFIX	= (1 << 1),
	HG_MSG_FLAG_END
};
enum _hg_message_category_t {
	HG_MSGCAT_0 = 0,
	HG_MSGCAT_TRACE,
	HG_MSGCAT_BITMAP,
	HG_MSGCAT_ALLOC,
	HG_MSGCAT_GC,
	HG_MSGCAT_SNAPSHOT,
	HG_MSGCAT_END
};


hg_message_func_t hg_message_set_default_handler(hg_message_func_t      func,
                                                 hg_pointer_t           user_data);
hg_message_func_t hg_message_set_handler        (hg_message_type_t      type,
                                                 hg_message_func_t      func,
                                                 hg_pointer_t           user_data);
hg_bool_t         hg_message_is_enabled         (hg_message_category_t  category);
void              hg_message_printf             (hg_message_type_t      type,
						 hg_message_flags_t     flags,
                                                 hg_message_category_t  category,
                                                 const hg_char_t       *format,
						 ...);
void              hg_message_vprintf            (hg_message_type_t      type,
						 hg_message_flags_t     flags,
                                                 hg_message_category_t  category,
                                                 const hg_char_t       *format,
                                                 va_list                args);
void              hg_return_if_fail_warning     (const hg_char_t       *pretty_function,
						 const hg_char_t       *expression);


#ifdef HG_HAVE_ISO_VARARGS
/* for(;;) ; so that GCC knows that control doesn't go past hg_fatal().
 * Put space before ending semicolon to avoid C++ build warnings.
 */
#define hg_fatal(...)					\
	HG_STMT_START {					\
		hg_message_printf(HG_MSG_FATAL,		\
				  HG_MSG_FLAG_NONE,	\
				  0,			\
				  __VA_ARGS__);		\
		for (;;) ;				\
	} HG_STMT_END
#define hg_critical(...)			\
	hg_message_printf(HG_MSG_CRITICAL,	\
			  HG_MSG_FLAG_NONE,	\
			  0,			\
			  __VA_ARGS__)
#define hg_warning(...)				\
	hg_message_printf(HG_MSG_WARNING,	\
			  HG_MSG_FLAG_NONE,	\
			  0,			\
			  __VA_ARGS__)
#define hg_info(...)				\
	hg_message_printf(HG_MSG_INFO,		\
			  HG_MSG_FLAG_NONE,	\
			  0,			\
			  __VA_ARGS__)
#define hg_debug(...)				\
	hg_message_printf(HG_MSG_DEBUG,		\
			  HG_MSG_FLAG_NONE,	\
			  __VA_ARGS__)
#define hg_debug0(...)				\
	hg_message_printf(HG_MSG_DEBUG,		\
			  __VA_ARGS__)

#elif defined(HG_HAVE_GNUC_VARARGS)

#define hg_fatal(format...)				\
	HG_STMT_START {					\
		hg_message_printf(HG_MSG_FATAL,		\
				  HG_MSG_FLAG_NONE,	\
				  0,			\
				  format);		\
		for (;;) ;				\
	} HG_STMT_END
#define hg_critical(format...)			\
	hg_message_printf(HG_MSG_CRITICAL,	\
			  HG_MSG_FLAG_NONE,	\
			  0,			\
			  format)
#define hg_warning(format...)			\
	hg_message_printf(HG_MSG_WARNING,	\
			  HG_MSG_FLAG_NONE,	\
			  0,			\
			  format)
#define hg_info(format...)			\
	hg_message_printf(HG_MSG_INFO,		\
			  HG_MSG_FLAG_NONE,	\
			  0,			\
			  format)
#define hg_debug(format...)			\
	hg_message_printf(HG_MSG_DEBUG,		\
			  HG_MSG_FLAG_NONE,	\
			  format)
#define hg_debug0(format...)			\
	hg_message_printf(HG_MSG_DEBUG,		\
			  format)
#else
static void
hg_fatal(const hg_char_t *format,
	 ...)
{
	va_list args;

	va_start(args, format);
	hg_message_vprintf(HG_MSG_FATAL, HG_MSG_FLAG_NONE, 0, format, args);
	va_end(args);

	for (;;) ;
}
static void
hg_critical(const hg_char_t *format,
	    ...)
{
	va_list args;

	va_start(args, format);
	hg_message_vprintf(HG_MSG_CRITICAL, HG_MSG_FLAG_NONE, 0, format, args);
	va_end(args);
}
static void
hg_warning(const hg_char_t *format,
	   ...)
{
	va_list args;

	va_start(args, format);
	hg_message_vprintf(HG_MSG_WARNING, HG_MSG_FLAG_NONE, 0, format, args);
	va_end(args);
}
static void
hg_info(const hg_char_t *format,
	...)
{
	va_list args;

	va_start(args, format);
	hg_message_vprintf(HG_MSG_INFO, HG_MSG_FLAG_NONE, 0, format, args);
	va_end(args);
}
static void
hg_debug(hg_message_category_t  category,
	 const hg_char_t       *format,
	 ...)
{
	va_list args;

	va_start(args, format);
	hg_message_vprintf(HG_MSG_DEBUG, HG_MSG_FLAG_NONE, category, format, args);
	va_end(args);
}
static void
hg_debug0(hg_message_flags_t     flags,
	  hg_message_category_t  category,
	  const hg_char_t       *format,
	  ...)
{
	va_list args;

	va_start(args, format);
	hg_message_vprintf(HG_MSG_DEBUG, flags, category, format, args);
	va_end(args);
}
#endif

#ifdef __GNUC__
#define _hg_return_after_eval_if_fail(__expr__,__eval__)		\
	HG_STMT_START {							\
		if (HG_LIKELY(__expr__)) {				\
		} else {						\
			hg_return_if_fail_warning(__PRETTY_FUNCTION__,	\
						  #__expr__);		\
			__eval__;					\
			return;						\
		}							\
	} HG_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	HG_STMT_START {							\
		if (HG_LIKELY(__expr__)) {				\
		} else {						\
			hg_return_if_fail_warning(__PRETTY_FUNCTION__,	\
						  #__expr__);		\
			__eval__;					\
			return (__val__);				\
		}							\
	} HG_STMT_END
#else /* !__GNUC__ */
#define _hg_return_after_eval_if_fail(__expr__,__eval__)		\
	HG_STMT_START {							\
		if (__expr__) {						\
		} else {						\
			hg_critical("file %s: line %d: assertion `%s' failed", \
				    __FILE__,				\
				    __LINE__,				\
				    #__expr__);				\
			__eval__;					\
			return;						\
		}							\
	} HG_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	HG_STMT_START {							\
		if (__expr__) {						\
		} else {						\
			hg_critical("file %s: line %d: assertion `%s' failed", \
				    __FILE__,				\
				    __LINE__,				\
				    #__expr__);				\
			__eval__;					\
			return (__val__);				\
		}							\
	} HG_STMT_END
#endif /* __GNUC__ */

#define hg_return_if_fail(__expr__)					\
	_hg_return_after_eval_if_fail(__expr__,{})
#define hg_return_val_if_fail(__expr__,__val__)				\
	_hg_return_val_after_eval_if_fail(__expr__,__val__,{})
#define hg_return_after_eval_if_fail(__expr__,__eval__)			\
	_hg_return_after_eval_if_fail(__expr__,__eval__)
#define hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)	\
	_hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__)
#define hg_return_with_gerror_if_fail(__expr__,__err__,__code__)	\
	_hg_return_after_eval_if_fail(__expr__,if(__err__){g_set_error((__err__),HG_ERROR,(__code__),__PRETTY_FUNCTION__);})
#define hg_return_val_with_gerror_if_fail(__expr__,__val__,__err__,__code__) \
	_hg_return_val_after_eval_if_fail(__expr__,__val__,if(__err__){g_set_error((__err__),HG_ERROR, (__code__), __PRETTY_FUNCTION__);})

HG_END_DECLS

#endif /* __HIEROGLYPH_HGMESSAGE_H__ */
