/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice.h
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

#ifndef __HIEROGLYPH_HGDEVICE_H__
#define __HIEROGLYPH_HGDEVICE_H__

#include <gmodule.h>
#include <hieroglyph/hgtypes.h>
#include <hieroglyph/hggstate.h>

HG_BEGIN_DECLS

typedef struct _hg_device_t		hg_device_t;
typedef struct _hg_pdev_params_t	hg_pdev_params_t;
typedef struct _hg_device_page_size_t	hg_device_page_size_t;
typedef struct _hg_device_axis_t	hg_device_axis_t;
typedef struct _hg_device_bbox_t	hg_device_bbox_t;
typedef enum _hg_device_orientation_t	hg_device_orientation_t;
typedef enum _hg_device_ops_t		hg_device_ops_t;
typedef enum _hg_pdev_params_name_t	hg_pdev_params_name_t;

typedef hg_device_t *	(* hg_device_init_func_t)	(void);
typedef void		(* hg_device_finalize_func_t)	(hg_device_t *device);
typedef hg_bool_t       (* hg_device_operator_t)	(hg_device_t *device,
							 hg_gstate_t *gstate);
typedef hg_quark_t      (* hg_pdev_name_lookup_func_t)	(hg_pdev_params_name_t p,
							 hg_pointer_t          user_data);


struct _hg_device_page_size_t {
	hg_real_t width;
	hg_real_t height;
};
struct _hg_device_axis_t {
	hg_real_t x;
	hg_real_t y;
};
struct _hg_device_bbox_t {
	hg_real_t x1;
	hg_real_t y1;
	hg_real_t x2;
	hg_real_t y2;
};
enum _hg_device_orientation_t {
	HG_ORIENTATION_DEFAULT = 0,
	HG_ORIENTATION_90_DEGREES = 1,
	HG_ORIENTATION_180_DEGREES = 2,
	HG_ORIENTATION_270_DEGREES = 3
};
enum _hg_device_ops_t {
	HG_OPS_NONE = 0,
	HG_OPS_DEACTIVATION = 1,
	HG_OPS_END_OF_JOB = 2,
	HG_OPS_COLLATE = 3,
	HG_OPS_SHOWPAGE = 4
};
enum _hg_pdev_params_name_t {
	HG_pdev_BEGIN = 0,
	HG_pdev_InputAttributes,
	HG_pdev_PageSize,
	HG_pdev_MediaColor,
	HG_pdev_MediaWeight,
	HG_pdev_MediaType,
	HG_pdev_ManualFeed,
	HG_pdev_Orientation,
	HG_pdev_AdvanceMedia,
	HG_pdev_AdvanceDistance,
	HG_pdev_CutMedia,
	HG_pdev_HWResolution,
	HG_pdev_ImagingBBox,
	HG_pdev_Margins,
	HG_pdev_MirrorPrint,
	HG_pdev_NegativePrint,
	HG_pdev_Duplex,
	HG_pdev_Tumble,
	HG_pdev_OutputType,
	HG_pdev_NumCopies,
	HG_pdev_Collate,
	HG_pdev_Jog,
	HG_pdev_OutputFaceUp,
	HG_pdev_Separations,
	HG_pdev_Install,
	HG_pdev_BeginPage,
	HG_pdev_EndPage,
	HG_pdev_END
};
struct _hg_pdev_params_t {
	hg_quark_t              self;

	hg_device_page_size_t   page_size;
	hg_device_axis_t        hw_resolution;
	hg_device_bbox_t	imaging_bbox;
	hg_device_axis_t        margins;
	hg_quark_t              qinput_attributes;
	hg_quark_t              qmedia_color;
	hg_quark_t              qmedia_type;
	hg_quark_t              qoutput_type;
	hg_quark_t              qinstall;
	hg_quark_t              qbegin_page;
	hg_quark_t              qend_page;
	hg_device_orientation_t orientation;
	hg_device_ops_t         advance_media;
	hg_device_ops_t         cut_media;
	hg_device_ops_t         jog;
	hg_real_t               media_weight;
	hg_int_t                advance_distance;
	hg_int_t                num_copies;
	hg_bool_t               manual_feed:1;
	hg_bool_t               mirror_print:1;
	hg_bool_t               negative_print:1;
	hg_bool_t               duplex:1;
	hg_bool_t               tumble:1;
	hg_bool_t               collate:1;
	hg_bool_t               output_faceup:1;
	hg_bool_t               separations:1;
};
struct _hg_device_t {
	/*< private >*/
	GModule                   *module;
	hg_device_finalize_func_t  finalizer;

	/*< protected >*/
	hg_quark_t                 qpdevparams_name[HG_pdev_END];
	hg_pdev_params_t          *params;

	hg_usize_t (* get_page_params_size) (hg_device_t           *device);
	hg_error_t (* gc_mark)              (hg_device_t           *device,
					     hg_gc_iterate_func_t   func,
					     hg_pointer_t           user_data);
	hg_quark_t (* get_page_param)       (hg_device_t           *device,
					     hg_uint_t              index);
	void       (* install)              (hg_device_t           *device,
					     hg_vm_t               *vm,
					     GError               **error);
	hg_bool_t  (* get_ctm)              (hg_device_t           *device,
					     hg_matrix_t           *array);
	hg_bool_t  (* is_pending_draw)      (hg_device_t           *device);
	void       (* draw)                 (hg_device_t           *device);

	hg_device_operator_t eofill;
	hg_device_operator_t fill;
	hg_device_operator_t stroke;

	/*< public >*/
	
};


hg_error_t   hg_device_gc_mark        (hg_device_t                 *device,
				       hg_gc_iterate_func_t         func,
				       hg_pointer_t                 user_data);
hg_device_t *hg_device_open           (hg_mem_t                    *mem,
				       const gchar                 *name);
void         hg_device_close          (hg_device_t                 *device);
hg_quark_t   hg_device_get_page_params(hg_device_t                 *device,
				       hg_mem_t                    *mem,
				       hg_bool_t                    check_dictfull,
				       hg_pdev_name_lookup_func_t   func,
				       hg_pointer_t                 func_user_data,
				       hg_pointer_t                *ret,
				       GError                     **error);
void         hg_device_install        (hg_device_t                 *device,
				       hg_vm_t                     *vm,
				       GError                     **error);
hg_bool_t    hg_device_is_pending_draw(hg_device_t                 *device);
void         hg_device_draw           (hg_device_t                 *device);
hg_bool_t    hg_device_get_ctm        (hg_device_t                 *device,
				       hg_matrix_t                 *array);
hg_bool_t    hg_device_eofill         (hg_device_t                 *device,
				       hg_gstate_t                 *gstate,
				       GError                     **error);
hg_bool_t    hg_device_fill           (hg_device_t                 *device,
				       hg_gstate_t                 *gstate,
				       GError                     **error);
hg_bool_t    hg_device_stroke         (hg_device_t                 *device,
				       hg_gstate_t                 *gstate,
				       GError                     **error);

/* null device */
hg_device_t *hg_device_null_new(hg_mem_t *mem);

/* modules */
hg_device_t *hg_module_init    (void);
void         hg_module_finalize(hg_device_t *device);

HG_END_DECLS

#endif /* __HIEROGLYPH_HGDEVICE_H__ */
