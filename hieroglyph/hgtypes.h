/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgtypes.h
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
#ifndef __HG_TYPES_H__
#define __HG_TYPES_H__

#include <sys/unistd.h>
#include <hieroglyph/hgmacros.h>

G_BEGIN_DECLS

typedef struct _HieroGlyphAllocatorVTable	HgAllocatorVTable;
typedef struct _HieroGlyphAllocator		HgAllocator;
typedef struct _HieroGlyphMemRelocateInfo	HgMemRelocateInfo;
typedef struct _HieroGlyphMemObject		HgMemObject;
typedef struct _HieroGlyphHeap			HgHeap;
typedef struct _HieroGlyphMemPool		HgMemPool;
typedef struct _HieroGlyphObjectVTable		HgObjectVTable;
typedef struct _HieroGlyphObject		HgObject;
typedef struct _HieroGlyphMemSnapshot		HgMemSnapshot;
typedef struct _HieroGlyphValueNode		HgValueNode;
typedef struct _HieroGlyphArray			HgArray;
typedef struct _HieroGlyphDict			HgDict;
typedef struct _HieroGlyphFileObject		HgFileObject;
typedef struct _HieroGlyphFileObjectCallback	HgFileObjectCallback;
typedef struct _HieroGlyphString		HgString;
typedef struct _HieroGlyphColor			HgColor;
typedef struct _HieroGlyphRenderFill		HgRenderFill;
typedef struct _HieroGlyphRenderStroke		HgRenderStroke;
typedef struct _HieroGlyphRenderDebug		HgRenderDebug;
typedef struct _HieroGlyphRender		HgRender;
typedef struct _HieroGlyphPage			HgPage;
typedef struct _HieroGlyphPathNode		HgPathNode;
typedef struct _HieroGlyphPath			HgPath;
typedef struct _HieroGlyphPathBBox		HgPathBBox;
typedef struct _HieroGlyphMatrix		HgMatrix;
typedef struct _HieroGlyphDeviceVTable		HgDeviceVTable;
typedef struct _HieroGlyphDevice		HgDevice;

typedef void                (*HgTraverseFunc)      (gpointer key,
						    gpointer val,
						    gpointer data);
typedef void                (*HgDebugFunc)         (gpointer data);
typedef HgAllocatorVTable * (*HgAllocatorTypeFunc) (void);

/* 32bit variables */
typedef enum {
	HG_FL_RESTORABLE = 1 << 0,  /* no infect */
	HG_FL_COMPLEX    = 1 << 1,  /* no infect */
	HG_FL_LOCK       = 1 << 2,  /* no infect */
	HG_FL_COPYING    = 1 << 3,  /* no infect */
	HG_FL_HGOBJECT   = 1 << 15, /* mark for HgObject */
	HG_FL_MARK1      = 1 << 16, /* infect all child objects - reserved for age of mark */
	HG_FL_MARK2      = 1 << 17, /* infect all child objects - reserved for age of mark */
	HG_FL_MARK3      = 1 << 18, /* infect all child objects - reserved for age of mark */
	HG_FL_MARK4      = 1 << 19, /* infect all child objects - reserved for age of mark */
	HG_FL_MARK5      = 1 << 20, /* infect all child objects - reserved for age of mark */
	HG_FL_MARK6      = 1 << 21, /* infect all child objects - reserved for age of mark */
	HG_FL_MARK7      = 1 << 22, /* infect all child objects - reserved for age of mark */
	HG_FL_MARK8      = 1 << 23, /* infect all child objects - reserved for age of mark */
	HG_FL_RESERVED1  = 1 << 24, /* reserved for heap id */
	HG_FL_RESERVED2  = 1 << 25, /* reserved for heap id */
	HG_FL_RESERVED3  = 1 << 26, /* reserved for heap id */
	HG_FL_RESERVED4  = 1 << 27, /* reserved for heap id */
	HG_FL_RESERVED5  = 1 << 28, /* reserved for heap id */
	HG_FL_RESERVED6  = 1 << 29, /* reserved for heap id */
	HG_FL_RESERVED7  = 1 << 30, /* reserved for heap id */
	HG_FL_RESERVED8  = 1 << 31, /* reserved for heap id */
	HG_FL_END
} HgMemFlags;

typedef enum {
	HG_ST_READABLE    = 1 << 0,
	HG_ST_WRITABLE    = 1 << 1,
	HG_ST_EXECUTABLE  = 1 << 2,
	HG_ST_RESERVED1   = 1 << 16, /* reserved for object-specific data */
	HG_ST_RESERVED2   = 1 << 17, /* reserved for object-specific data */
	HG_ST_RESERVED3   = 1 << 18, /* reserved for object-specific data */
	HG_ST_RESERVED4   = 1 << 19, /* reserved for object-specific data */
	HG_ST_RESERVED5   = 1 << 20, /* reserved for object-specific data */
	HG_ST_RESERVED6   = 1 << 21, /* reserved for object-specific data */
	HG_ST_RESERVED7   = 1 << 22, /* reserved for object-specific data */
	HG_ST_RESERVED8   = 1 << 23, /* reserved for object-specific data */
	HG_ST_RESERVED9   = 1 << 24, /* reserved for vtable ID */
	HG_ST_RESERVED10  = 1 << 25, /* reserved for vtable ID */
	HG_ST_RESERVED11  = 1 << 26, /* reserved for vtable ID */
	HG_ST_RESERVED12  = 1 << 27, /* reserved for vtable ID */
	HG_ST_RESERVED13  = 1 << 28, /* reserved for vtable ID */
	HG_ST_RESERVED14  = 1 << 29, /* reserved for vtable ID */
	HG_ST_RESERVED15  = 1 << 30, /* reserved for vtable ID */
	HG_ST_RESERVED16  = 1 << 31, /* reserved for vtable ID */
	HG_ST_END
} HgObjectStateFlags;

typedef enum {
	HG_DEBUG_GC_MARK = 0,
	HG_DEBUG_GC_ALREADYMARK,
	HG_DEBUG_GC_UNMARK,
	HG_DEBUG_DUMP,
} HgDebugStateType;

typedef enum {
	HG_TYPE_VALUE_BOOLEAN = 1,
	HG_TYPE_VALUE_INTEGER,
	HG_TYPE_VALUE_REAL,
	HG_TYPE_VALUE_NAME,
	HG_TYPE_VALUE_ARRAY,
	HG_TYPE_VALUE_STRING,
	HG_TYPE_VALUE_DICT,
	HG_TYPE_VALUE_NULL,
	HG_TYPE_VALUE_POINTER,
	HG_TYPE_VALUE_MARK,
	HG_TYPE_VALUE_FILE,
	HG_TYPE_VALUE_SNAPSHOT,
	HG_TYPE_VALUE_END
} HgValueType;

typedef enum {
	HG_FILE_TYPE_FILE = 1,
	HG_FILE_TYPE_BUFFER,
	HG_FILE_TYPE_STDIN,
	HG_FILE_TYPE_STDOUT,
	HG_FILE_TYPE_STDERR,
	HG_FILE_TYPE_STATEMENT_EDIT,
	HG_FILE_TYPE_LINE_EDIT,
	HG_FILE_TYPE_BUFFER_WITH_CALLBACK,
	HG_FILE_TYPE_END
} HgFileType;

typedef enum {
	HG_FILE_MODE_READ      = 1 << 0,
	HG_FILE_MODE_WRITE     = 1 << 1,
	HG_FILE_MODE_READWRITE = 1 << 2
} HgFileModeType;

typedef enum {
	HG_FILE_POS_BEGIN   = SEEK_SET,
	HG_FILE_POS_CURRENT = SEEK_CUR,
	HG_FILE_POS_END     = SEEK_END
} HgFilePosType;

typedef enum {
	HG_PAGE_4A0 = 1,
	HG_PAGE_2A0 = 2,

	HG_PAGE_A0  = 10,
	HG_PAGE_A1  = 11,
	HG_PAGE_A2  = 12,
	HG_PAGE_A3  = 13,
	HG_PAGE_A4  = 14,
	HG_PAGE_A5  = 15,
	HG_PAGE_A6  = 16,
	HG_PAGE_A7  = 17,

	HG_PAGE_B0  = 20,
	HG_PAGE_B1  = 21,
	HG_PAGE_B2  = 22,
	HG_PAGE_B3  = 23,
	HG_PAGE_B4  = 24,
	HG_PAGE_B5  = 25,
	HG_PAGE_B6  = 26,
	HG_PAGE_B7  = 27,

	HG_PAGE_JIS_B0 = 30,
	HG_PAGE_JIS_B1 = 31,
	HG_PAGE_JIS_B2 = 32,
	HG_PAGE_JIS_B3 = 33,
	HG_PAGE_JIS_B4 = 34,
	HG_PAGE_JIS_B5 = 35,
	HG_PAGE_JIS_B6 = 36,

	HG_PAGE_C0 = 40,
	HG_PAGE_C1 = 41,
	HG_PAGE_C2 = 42,
	HG_PAGE_C3 = 43,
	HG_PAGE_C4 = 44,
	HG_PAGE_C5 = 45,
	HG_PAGE_C6 = 46,
	HG_PAGE_C7 = 47,

	HG_PAGE_LETTER = 50,
	HG_PAGE_LEGAL  = 51,

	HG_PAGE_JAPAN_POSTCARD = 60,
} HgPageSize;

typedef enum {
	HG_PATH_SETBBOX = 0,
	HG_PATH_MOVETO,
	HG_PATH_RMOVETO,
	HG_PATH_LINETO,
	HG_PATH_RLINETO,
	HG_PATH_CURVETO,
	HG_PATH_RCURVETO,
	HG_PATH_ARC,
	HG_PATH_ARCN,
	HG_PATH_ARCT,
	HG_PATH_CLOSE,
	HG_PATH_UCACHE,

	HG_PATH_MATRIX = 50,
} HgPathType;

typedef enum {
	HG_RENDER_EOFILL = 1,
	HG_RENDER_FILL,
	HG_RENDER_STROKE,
	HG_RENDER_DEBUG,
} HgRenderType;

struct _HieroGlyphAllocatorVTable {
	gboolean        (* initialize)         (HgMemPool     *pool,
						gsize          prealloc);
	gboolean        (* destroy)            (HgMemPool     *pool);
	gboolean        (* resize_pool)        (HgMemPool     *pool,
						gsize          size);
	gpointer        (* alloc)              (HgMemPool     *pool,
						gsize          size,
						guint          flags);
	void            (* free)               (HgMemPool     *pool,
						gpointer       data);
	gpointer        (* resize)             (HgMemObject   *object,
						gsize          size);
	gsize           (* get_size)           (HgMemObject   *object);
	gboolean        (* garbage_collection) (HgMemPool     *pool);
	void            (* gc_mark)            (HgMemPool     *pool);
	gboolean        (* is_safe_object)     (HgMemPool     *pool,
						HgMemObject   *object);
	HgMemSnapshot * (* save_snapshot)      (HgMemPool     *pool);
	gboolean        (* restore_snapshot)   (HgMemPool     *pool,
						HgMemSnapshot *snapshot);
};

struct _HieroGlyphAllocator {
	gpointer                 private;
	gboolean                 used;
	const HgAllocatorVTable *vtable;
};

struct _HieroGlyphMemRelocateInfo {
	gsize diff;
	gsize start;
	gsize end;
};

struct _HieroGlyphMemObject {
	gint32     magic;
	gpointer   subid;
	HgMemPool *pool;
	guint32    flags;
	gpointer   data[];
};

struct _HieroGlyphObjectVTable {
	void     (* free)      (gpointer           data);
	void     (* set_flags) (gpointer           data,
				guint              flags);
	void     (* relocate)  (gpointer           data,
				HgMemRelocateInfo *info);
	gpointer (* dup)       (gpointer           data);
	gpointer (* copy)      (gpointer           data);
	gpointer (* to_string) (gpointer           data);
};

struct _HieroGlyphObject {
	guint           state;
};

struct _HieroGlyphValueNode {
	HgObject    object;
	union {
		gboolean boolean;
		gint32   integer;
		gdouble  real;
		gpointer pointer;
	} v;
};

struct _HieroGlyphFileObjectCallback {
	gsize    (* read)           (gpointer      user_data,
				     gpointer      buffer,
				     gsize         size,
				     gsize         n);
	gsize    (* write)          (gpointer      user_data,
				     gconstpointer buffer,
				     gsize         size,
				     gsize         n);
	gchar    (* getc)           (gpointer      user_data);
	gboolean (* flush)          (gpointer      user_data);
	gboolean (* is_eof)         (gpointer      user_data);
	void     (* clear_eof)      (gpointer      uesr_data);
	gint     (* get_error_code) (gpointer      user_data);
};

struct _HieroGlyphColor {
	gboolean is_rgb;
	union {
		struct {
			gdouble r;
			gdouble g;
			gdouble b;
		} rgb;
		struct {
			gdouble h;
			gdouble s;
			gdouble v;
		} hsv;
	} is;
};

struct _HieroGlyphMatrix {
	gdouble xx;
	gdouble yx;
	gdouble xy;
	gdouble yy;
	gdouble x0;
	gdouble y0;
};

struct _HieroGlyphRenderFill {
	HgRenderType  type;
	HgMatrix      mtx;
	HgPathNode   *path;
	HgColor       color;
};

struct _HieroGlyphRenderStroke {
	HgRenderType  type;
	HgMatrix      mtx;
	HgPathNode   *path;
	HgColor       color;
	gdouble       line_width;
	gint          line_cap;
	gint          line_join;
	gdouble       miter_limit;
	gdouble       dashline_offset;
	HgArray      *dashline_pattern;
};

struct _HieroGlyphRenderDebug {
	HgRenderType  type;
	HgDebugFunc   func;
	gpointer      data;
};

struct _HieroGlyphRender {
	HgObject  object;
	union {
		HgRenderType      type;
		HgRenderFill      fill;
		HgRenderStroke    stroke;
		HgRenderDebug     debug;
	} u;
};

struct _HieroGlyphPage {
	gdouble       width;
	gdouble       height;
	GList        *node;
	GList        *last_node;
};

struct _HieroGlyphPathNode {
	HgObject    object;
	HgPathType  type;
	gdouble     x;
	gdouble     y;
	HgPathNode *prev;
	HgPathNode *next;
};

struct _HieroGlyphPath {
	HgObject    object;
	HgPathNode *node;
	HgPathNode *last_node;
};

struct _HieroGlyphPathBBox {
	gdouble llx;
	gdouble lly;
	gdouble urx;
	gdouble ury;
};

G_END_DECLS

#endif /* __HG_TYPES_H__ */
