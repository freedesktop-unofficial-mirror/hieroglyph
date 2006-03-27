/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * operator.h
 * Copyright (C) 2005-2006 Akira TAGOH
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
#ifndef __LIBRETTO_OPERATOR_H__
#define __LIBRETTO_OPERATOR_H__

#include <hieroglyph/hgtypes.h>
#include <libretto/lbtypes.h>

G_BEGIN_DECLS


#define _libretto_operator_set_error(v, o, e)				\
	G_STMT_START {							\
		HgValueNode *__lb_op_node;				\
									\
		HG_VALUE_MAKE_POINTER (__lb_op_node, (o));		\
		libretto_vm_set_error((v), __lb_op_node, (e), TRUE);	\
	} G_STMT_END
#define _libretto_operator_set_error_from_file(v, o, e)			\
	G_STMT_START {							\
		HgValueNode *__lb_op_node;				\
									\
		HG_VALUE_MAKE_POINTER (__lb_op_node, (o));		\
		libretto_vm_set_error_from_file((v), __lb_op_node, (e), TRUE); \
	} G_STMT_END


extern LibrettoOperator *__lb_operator_list[LB_op_END];

LibrettoOperator *libretto_operator_new     (HgMemPool            *pool,
					     const gchar          *name,
					     LibrettoOperatorFunc  func);
gboolean          libretto_operator_init    (LibrettoVM           *vm);
gboolean          libretto_operator_invoke  (LibrettoOperator     *op,
					     LibrettoVM           *vm);
const gchar      *libretto_operator_get_name(LibrettoOperator     *op) G_GNUC_CONST;


G_END_DECLS

#endif /* __LIBRETTO_OPERATOR_H__ */
