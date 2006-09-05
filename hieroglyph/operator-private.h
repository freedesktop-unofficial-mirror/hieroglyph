/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * operator-private.h
 * Copyright (C) 2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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
#ifndef __HG_OPERATOR_PRIVATE_H__
#define __HG_OPERATOR_PRIVATE_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


#define DEFUNC_OP(func)					\
	static gboolean					\
	_hg_operator_op_##func(HgOperator *op,		\
			       gpointer    data)	\
	{						\
		HgVM *vm = data;			\
		gboolean retval = FALSE;
#define DEFUNC_OP_END				\
		return retval;			\
	}
#define DEFUNC_UNIMPLEMENTED_OP(func)			\
	static gboolean					\
	_hg_operator_op_##func(HgOperator *op,		\
			       gpointer    data)	\
	{						\
		g_warning("%s isn't yet implemented.", __PRETTY_FUNCTION__); \
							\
		return FALSE;				\
	}
#define _hg_operator_build_operator(vm, pool, sdict, name, func, ret_op) \
	hg_operator_build_operator__inline(_hg_operator_op_, vm, pool, sdict, name, func, ret_op)
#define BUILD_OP(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		_hg_operator_build_operator(vm, pool, sdict, name, func, __hg_op); \
		if (__hg_op != NULL) {					\
			__hg_operator_list[HG_op_##name] = __hg_op;	\
		}							\
	} G_STMT_END
#define BUILD_OP_(vm, pool, sdict, name, func)				\
	G_STMT_START {							\
		HgOperator *__hg_op;					\
									\
		_hg_operator_build_operator(vm, pool, sdict, name, func, __hg_op); \
	} G_STMT_END


G_END_DECLS

#endif /* __HG_OPERATOR_PRIVATE_H__ */
