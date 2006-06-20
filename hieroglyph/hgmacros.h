/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmacros.h
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
#ifndef __HG_MACROS_H__
#define __HG_MACROS_H__

#include <glib.h>

G_BEGIN_DECLS

#define HG_MEM_HEADER	0x48474d4f
#define HG_CHECK_MAGIC_CODE(_obj, _magic)		\
	((_obj)->magic == (_magic))
#define HG_SET_MAGIC_CODE(_obj, _magic)			\
	((_obj)->magic = (_magic))

#define HG_STACK_INIT					\
	gpointer __hg_stack_mark;			\
							\
	hg_mem_init_stack_start(&__hg_stack_mark)
#define HG_SET_STACK_END			\
	gpointer __hg_stack_mark;		\
						\
	hg_mem_set_stack_end(&__hg_stack_mark)
#define HG_SET_STACK_END_AGAIN			\
	hg_mem_set_stack_end(&__hg_stack_mark)
#define HG_MEM_INIT				\
	HG_STACK_INIT;				\
	hg_mem_init()

#define HG_MEM_ALIGNMENT	4

/* HgMemObject */
#define HG_MEMOBJ_HEAP_ID_MASK		0xff000000
#define HG_MEMOBJ_MARK_AGE_MASK		0x00ff0000
#define HG_MEMOBJ_HGOBJECT_MASK		0x00008000
#define HG_MEMOBJ_FLAGS_MASK		0x00007fff
#define HG_MEMOBJ_GET_HEAP_ID(_obj)		(((_obj)->flags & HG_MEMOBJ_HEAP_ID_MASK) >> 24)
#define HG_MEMOBJ_SET_HEAP_ID(_obj, _id)				\
	((_obj)->flags = (((_id) << 24) & HG_MEMOBJ_HEAP_ID_MASK)	\
	 | (HG_MEMOBJ_GET_MARK_AGE (_obj) << 16)			\
	 | (HG_MEMOBJ_GET_HGOBJECT_ID (_obj) << 15)			\
	 | HG_MEMOBJ_GET_FLAGS (_obj))
#define HG_MEMOBJ_GET_MARK_AGE(_obj)		(((_obj)->flags & HG_MEMOBJ_MARK_AGE_MASK) >> 16)
#define HG_MEMOBJ_SET_MARK_AGE(_obj, _age)				\
	((_obj)->flags = (HG_MEMOBJ_GET_HEAP_ID (_obj) << 24)		\
	 | (((_age) << 16) & HG_MEMOBJ_MARK_AGE_MASK)			\
	 | (HG_MEMOBJ_GET_HGOBJECT_ID (_obj) << 15)			\
	 | HG_MEMOBJ_GET_FLAGS (_obj))
#define HG_MEMOBJ_GET_HGOBJECT_ID(_obj)		(((_obj)->flags & HG_MEMOBJ_HGOBJECT_MASK) >> 15)
#define HG_MEMOBJ_SET_HGOBJECT_ID(_obj)					\
	((_obj)->flags = (HG_MEMOBJ_GET_HEAP_ID (_obj) << 24)		\
	 | (HG_MEMOBJ_GET_MARK_AGE (_obj) << 16)			\
	 | HG_FL_HGOBJECT						\
	 | HG_MEMOBJ_GET_FLAGS (_obj))
#define HG_MEMOBJ_GET_FLAGS(_obj)		((_obj)->flags & HG_MEMOBJ_FLAGS_MASK)
#define HG_MEMOBJ_SET_FLAGS(_obj, _flags)				\
	((_obj)->flags = (HG_MEMOBJ_GET_HEAP_ID (_obj) << 24)		\
	 | (HG_MEMOBJ_GET_MARK_AGE (_obj) << 16)			\
	 | (HG_MEMOBJ_GET_HGOBJECT_ID (_obj) << 15)			\
	 | ((_flags) & HG_MEMOBJ_FLAGS_MASK))
#define HG_MEMOBJ_INIT_FLAGS(_obj)		(_obj)->flags = 0;
#define HG_MEMOBJ_IS_HGOBJECT(_obj)		(HG_MEMOBJ_GET_HGOBJECT_ID (_obj) == 1)

/* HgObject */
#define HG_OBJECT_VTABLE_ID_MASK	0xff000000
#define HG_OBJECT_USER_DATA_MASK	0x00ff0000
#define HG_OBJECT_STATE_MASK		0x0000ffff
#define HG_OBJECT_GET_VTABLE_ID(_obj)		(((_obj)->state & HG_OBJECT_VTABLE_ID_MASK) >> 24)
#define HG_OBJECT_SET_VTABLE_ID(_obj, _id)				\
	((_obj)->state = (((_id) << 24) & HG_OBJECT_VTABLE_ID_MASK)	\
	 | (HG_OBJECT_GET_USER_DATA (_obj) << 16)			\
	 | HG_OBJECT_GET_STATE (_obj))
#define HG_OBJECT_GET_USER_DATA(_obj)		(((_obj)->state & HG_OBJECT_USER_DATA_MASK) >> 16)
#define HG_OBJECT_SET_USER_DATA(_obj, _data)				\
	((_obj)->state = (HG_OBJECT_GET_VTABLE_ID (_obj) << 24)		\
	 | (((_data) << 16) & HG_OBJECT_USER_DATA_MASK)			\
	 | HG_OBJECT_GET_STATE (_obj))
#define HG_OBJECT_GET_STATE(_obj)		((_obj)->state & HG_OBJECT_STATE_MASK)
#define HG_OBJECT_SET_STATE(_obj,_state)				\
	((_obj)->state = (HG_OBJECT_GET_VTABLE_ID (_obj) << 24)		\
	 | (HG_OBJECT_GET_USER_DATA (_obj) << 16)			\
	 | ((_state) & HG_OBJECT_STATE_MASK))
#define HG_OBJECT_INIT_STATE(_obj)		((_obj)->state = 0)


G_END_DECLS

#endif /* __HG_MACROS_H__ */
