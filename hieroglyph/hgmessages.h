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
#include <hieroglyph/hgerror.h>

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
	HG_MSGCAT_DEBUG,	/* 1 */
	HG_MSGCAT_TRACE,	/* 2 */
	HG_MSGCAT_BITMAP,	/* 4 */
	HG_MSGCAT_ALLOC,	/* 8 */
	HG_MSGCAT_GC,		/* 16 */
	HG_MSGCAT_SNAPSHOT,	/* 32 */
	HG_MSGCAT_MEM,		/* 64 */
	HG_MSGCAT_ARRAY,	/* 128 */
	HG_MSGCAT_DEVICE,	/* 256 */
	HG_MSGCAT_DICT,		/* 512 */
	HG_MSGCAT_FILE,		/* 1024 */
	HG_MSGCAT_GSTATE,	/* 2048 */
	HG_MSGCAT_PATH,		/* 4096 */
	HG_MSGCAT_PLUGIN,	/* 8192 */
	HG_MSGCAT_STRING,	/* 16384 */
	HG_MSGCAT_VM,		/* 32768 */
	HG_MSGCAT_SCAN,		/* 65536 */
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
#define hg_debug(_c_,...)			\
	hg_message_printf(HG_MSG_DEBUG,		\
			  HG_MSG_FLAG_NONE,	\
			  (_c_),		\
			  __VA_ARGS__)
#define hg_debug0(_c_,_f_,...)			\
	hg_message_printf(HG_MSG_DEBUG,		\
			  (_f_),		\
			  (_c_),		\
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
#define hg_debug(_c_,format...)			\
	hg_message_printf(HG_MSG_DEBUG,		\
			  HG_MSG_FLAG_NONE,	\
			  (_c_),		\
			  format)
#define hg_debug0(_c_,_f_,format...)		\
	hg_message_printf(HG_MSG_DEBUG,		\
			  (_f_),		\
			  (_c_),		\
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
hg_debug0(hg_message_category_t  category,
	  hg_message_flags_t     flags,
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
#define _hg_return_after_eval_if_fail(__expr__,__eval__,_reason_)	\
	HG_STMT_START {							\
		if (HG_LIKELY(__expr__)) {				\
		} else {						\
			hg_return_if_fail_warning(__PRETTY_FUNCTION__,	\
						  #__expr__);		\
			__eval__;					\
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED,(_reason_)); \
			return;						\
		}							\
	} HG_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__,_reason_) \
	HG_STMT_START {							\
		if (HG_LIKELY(__expr__)) {				\
		} else {						\
			hg_return_if_fail_warning(__PRETTY_FUNCTION__,	\
						  #__expr__);		\
			__eval__;					\
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED,(_reason_)); \
			return (__val__);				\
		}							\
	} HG_STMT_END
#else /* !__GNUC__ */
#define _hg_return_after_eval_if_fail(__expr__,__eval__,_reason_)	\
	HG_STMT_START {							\
		if (__expr__) {						\
		} else {						\
			hg_critical("file %s: line %d: assertion `%s' failed", \
				    __FILE__,				\
				    __LINE__,				\
				    #__expr__);				\
			__eval__;					\
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED,(_reason_)); \
			return;						\
		}							\
	} HG_STMT_END
#define _hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__,_reason_) \
	HG_STMT_START {							\
		if (__expr__) {						\
		} else {						\
			hg_critical("file %s: line %d: assertion `%s' failed", \
				    __FILE__,				\
				    __LINE__,				\
				    #__expr__);				\
			__eval__;					\
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED,(_reason_)); \
			return (__val__);				\
		}							\
	} HG_STMT_END
#endif /* __GNUC__ */

#define hg_return_if_fail(__expr__,_reason_)			\
	_hg_return_after_eval_if_fail(__expr__,{},_reason_)
#define hg_return_val_if_fail(__expr__,__val__,_reason_)		\
	_hg_return_val_after_eval_if_fail(__expr__,__val__,{},_reason_)
#define hg_return_after_eval_if_fail(__expr__,__eval__,_reason_)	\
	_hg_return_after_eval_if_fail(__expr__,__eval__,_reason_)
#define hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__,_reason_) \
	_hg_return_val_after_eval_if_fail(__expr__,__val__,__eval__,_reason_)

HG_END_DECLS

#endif /* __HIEROGLYPH_HGMESSAGE_H__ */
