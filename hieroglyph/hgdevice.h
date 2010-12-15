/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice.h
 * Copyright (C) 2005-2010 Akira TAGOH
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
#ifndef __HIEROGLYPH_HGDEVICE_H__
#define __HIEROGLYPH_HGDEVICE_H__

#include <gmodule.h>
#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS

typedef hg_device_t *	(* hg_device_init_func_t)	(void);
typedef void		(* hg_device_finalize_func_t)	(hg_device_t *device);
typedef gboolean        (* hg_device_operator_t)	(hg_device_t *device,
							 hg_gstate_t *gstate);
typedef hg_quark_t      (* hg_pdev_name_lookup_func_t)	(hg_pdev_params_name_t p,
							 gpointer              user_data);


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
	gdouble                 media_weight;
	gint                    advance_distance;
	gint                    num_copies;
	gboolean                manual_feed:1;
	gboolean                mirror_print:1;
	gboolean                negative_print:1;
	gboolean                duplex:1;
	gboolean                tumble:1;
	gboolean                collate:1;
	gboolean                output_faceup:1;
	gboolean                separations:1;
};
struct _hg_device_t {
	/*< private >*/
	GModule                   *module;
	hg_device_finalize_func_t  finalizer;

	/*< protected >*/
	hg_quark_t                 qpdevparams_name[HG_pdev_END];
	hg_pdev_params_t          *params;

	gsize      (* get_page_params_size) (hg_device_t           *device);
	gboolean   (* gc_mark)              (hg_device_t           *device,
					     hg_gc_iterate_func_t   func,
					     gpointer               user_data,
					     GError               **error);
	hg_quark_t (* get_page_param)       (hg_device_t           *device,
					     guint                  index);
	gboolean   (* get_ctm)              (hg_device_t           *device,
					     hg_matrix_t           *array);

	hg_device_operator_t fill;
	hg_device_operator_t stroke;

	/*< public >*/
	
};


gboolean     hg_device_gc_mark        (hg_device_t                 *device,
				       hg_gc_iterate_func_t         func,
				       gpointer                     user_data,
				       GError                     **error);
hg_device_t *hg_device_open           (hg_mem_t                    *mem,
				       const gchar                 *name);
void         hg_device_close          (hg_device_t                 *device);
hg_quark_t   hg_device_get_page_params(hg_device_t                 *device,
                                       hg_mem_t                    *mem,
                                       gboolean                     check_dictfull,
                                       hg_pdev_name_lookup_func_t   func,
				       gpointer                     func_user_data,
                                       gpointer                    *ret,
                                       GError                     **error);
gboolean     hg_device_get_ctm        (hg_device_t                 *device,
                                       hg_matrix_t                 *array);
gboolean     hg_device_fill           (hg_device_t                 *device,
                                       hg_gstate_t                 *gstate,
                                       GError                     **error);
gboolean     hg_device_stroke         (hg_device_t                 *device,
                                       hg_gstate_t                 *gstate,
                                       GError                     **error);

/* null device */
hg_device_t *hg_device_null_new(hg_mem_t *mem);

/* modules */
hg_device_t *hg_init    (void);
void         hg_finalize(hg_device_t *device);

G_END_DECLS

#endif /* __HIEROGLYPH_HGDEVICE_H__ */
