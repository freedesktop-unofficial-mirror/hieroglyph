/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgutils.h
 * Copyright (C) 2005-2011 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#if !defined (__HG_H_INSIDE__) && !defined (HG_COMPILATION)
#error "Only <hieroglyph/hg.h> can be included directly."
#endif

#ifndef __HIEROGLYPH_HGUTILS_H__
#define __HIEROGLYPH_HGUTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hieroglyph/hgtypes.h>

HG_BEGIN_DECLS

HG_INLINE_FUNC hg_pointer_t  hg_malloc        (hg_usize_t       size);
HG_INLINE_FUNC hg_pointer_t  hg_malloc0       (hg_usize_t       size);
HG_INLINE_FUNC void          hg_free          (hg_pointer_t     data);
HG_INLINE_FUNC hg_char_t    *hg_strdup        (const hg_char_t *string);
HG_INLINE_FUNC hg_char_t    *hg_strndup       (const hg_char_t *string,
					       hg_usize_t       len);
HG_INLINE_FUNC hg_char_t    *hg_strdup_printf (const hg_char_t *format,
					       ...);
HG_INLINE_FUNC hg_char_t    *hg_strdup_vprintf(const hg_char_t *format,
					       va_list          args);

/**
 * hg_malloc:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_pointer_t
hg_malloc(hg_usize_t size)
{
	return malloc(size);
}

/**
 * hg_malloc0:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_pointer_t
hg_malloc0(hg_usize_t size)
{
	return calloc(1, size);
}

/**
 * hg_realloc:
 * @data:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_pointer_t
hg_realloc(hg_pointer_t data,
	   hg_usize_t   size)
{
	return realloc(data, size);
}

/**
 * hg_free:
 * @data:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC void
hg_free(hg_pointer_t data)
{
	if (data)
		free(data);
}

/**
 * hg_strdup:
 * @string:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_char_t *
hg_strdup(const hg_char_t *string)
{
	return strdup(string);
}

/**
 * hg_strndup:
 * @string:
 * @len:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_char_t *
hg_strndup(const hg_char_t *string,
	   hg_usize_t       len)
{
	return strndup(string, len);
}

/**
 * hg_strdup_printf:
 * @format:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_char_t *
hg_strdup_printf(const hg_char_t *format,
		 ...)
{
	va_list args;
	hg_char_t *retval;

	va_start(args, format);

	retval = hg_strdup_vprintf(format, args);

	va_end(args);

	return retval;
}

/**
 * hg_strdup_vprintf:
 * @format:
 * @args:
 *
 * FIXME
 *
 * Returns:
 */
HG_INLINE_FUNC hg_char_t *
hg_strdup_vprintf(const hg_char_t *format,
		  va_list          args)
{
	char c;
	int size;
	hg_char_t *retval;
	va_list ap;

	va_copy(ap, args);

	size = vsnprintf(&c, 1, format, ap) + 1;

	va_end(ap);

	retval = hg_malloc(sizeof (hg_char_t) * size);
	if (retval) {
		vsprintf(retval, format, args);
	}

	return retval;
}

hg_char_t *hg_find_libfile(const hg_char_t *file);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGUTILS_H__ */
