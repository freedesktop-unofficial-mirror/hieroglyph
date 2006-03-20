/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * scanner.h
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
#ifndef __LIBRETTO_SCANNER_H__
#define __LIBRETTO_SCANNER_H__

#include <hieroglyph/hgtypes.h>
#include <libretto/lbtypes.h>


G_BEGIN_DECLS

typedef enum {
	LB_C_NULL,
	LB_C_CONTROL,
	LB_C_NAME,
	LB_C_SPACE,
	LB_C_NUMERAL,
	LB_C_BINARY,
} LibrettoScannerCharType;

HgValueNode *libretto_scanner_get_object(LibrettoVM *vm,
					 HgFileObject *file);

G_END_DECLS


#endif /* __LIBRETTO_SCANNER_H__ */
