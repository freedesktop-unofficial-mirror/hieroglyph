/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdevice.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "hgarray.h"
#include "hgbool.h"
#include "hgdict.h"
#include "hgerror.h"
#include "hggstate.h"
#include "hgint.h"
#include "hgmatrix.h"
#include "hgmem.h"
#include "hgnull.h"
#include "hgoperator.h"
#include "hgpath.h"
#include "hgreal.h"
#include "hgdevice.h"

#include "hgdevice.proto.h"

/*< private >*/
static hg_bool_t
_hg_device_init_page_params(hg_device_t *device,
			    hg_mem_t    *mem)
{
	hg_type_t type;
	hg_array_t *a;

	if (!device->params) {
		hg_quark_t q = hg_mem_alloc(mem, sizeof (hg_pdev_params_t), (hg_pointer_t *)&device->params);

		if (q == Qnil)
			return FALSE;
		device->params->self = q;
	}

	type = hg_quark_get_type(device->params->qinput_attributes);
	if (type != HG_TYPE_DICT &&
	    type != HG_TYPE_NULL) {
		device->params->qinput_attributes = Qnil;
	}

	type = hg_quark_get_type(device->params->qmedia_color);
	if (type != HG_TYPE_STRING &&
	    type != HG_TYPE_NULL) {
		device->params->qmedia_color = Qnil;
	}

	type = hg_quark_get_type(device->params->qmedia_type);
	if (type != HG_TYPE_STRING &&
	    type != HG_TYPE_NULL) {
		device->params->qmedia_type = Qnil;
	}

	type = hg_quark_get_type(device->params->qoutput_type);
	if (type != HG_TYPE_STRING &&
	    type != HG_TYPE_NULL) {
		device->params->qoutput_type = Qnil;
	}

	type = hg_quark_get_type(device->params->qinstall);
	if (type != HG_TYPE_ARRAY) {
		device->params->qinstall = hg_array_new(mem, 1, (hg_pointer_t *)&a);
		if (device->params->qinstall == Qnil)
			return FALSE;
		if (!hg_array_set(a, HG_QOPER (HG_enc_private_callinstall), 0, FALSE)) {
			hg_mem_unlock_object(mem, device->params->qinstall);
			return FALSE;
		}
		hg_mem_unlock_object(mem, device->params->qinstall);
	}
	hg_quark_set_executable(&device->params->qinstall, TRUE);
	hg_mem_unref(mem, device->params->qinstall);

	type = hg_quark_get_type(device->params->qbegin_page);
	if (type != HG_TYPE_ARRAY) {
		device->params->qbegin_page = hg_array_new(mem, 1, (hg_pointer_t *)&a);
		if (device->params->qbegin_page == Qnil)
			return FALSE;
		if (!hg_array_set(a, HG_QOPER (HG_enc_private_callbeginpage), 0, FALSE)) {
			hg_mem_unlock_object(mem, device->params->qbegin_page);
			return FALSE;
		}
		hg_mem_unlock_object(mem, device->params->qbegin_page);
	}
	hg_quark_set_executable(&device->params->qbegin_page, TRUE);
	hg_mem_unref(mem, device->params->qbegin_page);

	type = hg_quark_get_type(device->params->qend_page);
	if (type != HG_TYPE_ARRAY) {
		device->params->qend_page = hg_array_new(mem, 1, (hg_pointer_t *)&a);
		if (device->params->qend_page == Qnil)
			return FALSE;
		if (!hg_array_set(a, HG_QOPER (HG_enc_private_callendpage), 0, FALSE)) {
			hg_mem_unlock_object(mem, device->params->qend_page);
			return FALSE;
		}
		hg_mem_unlock_object(mem, device->params->qend_page);
	}
	hg_quark_set_executable(&device->params->qend_page, TRUE);
	hg_mem_unref(mem, device->params->qend_page);

	return TRUE;
}

static hg_device_t *
_hg_device_open(hg_mem_t        *mem,
		const hg_char_t *modulename)
{
	GModule *module;
	hg_device_t *retval = NULL;
	hg_pointer_t dev_init = NULL, dev_fini = NULL;

	if ((module = g_module_open(modulename, G_MODULE_BIND_LAZY|G_MODULE_BIND_LOCAL)) != NULL) {
		g_module_symbol(module, "hg_module_init", &dev_init);
		g_module_symbol(module, "hg_module_finalize", &dev_fini);

		if (!dev_init || !dev_fini) {
			hg_warning(g_module_error());
			g_module_close(module);

			return NULL;
		}

		retval = ((hg_device_init_func_t)dev_init)();

		if (retval) {
			retval->module = module;
			retval->finalizer = dev_fini;

			_hg_device_init_page_params(retval, mem);
		}
	} else {
		hg_debug(HG_MSGCAT_DEVICE, "%s", g_module_error());
	}

	return retval;
}

/*< public >*/

/**
 * hg_device_gc_mark:
 * @device:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_device_gc_mark(hg_device_t          *device,
		  hg_gc_iterate_func_t  func,
		  hg_pointer_t          user_data)
{
	hg_return_val_if_fail (device != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, FALSE, HG_e_VMerror);

	if (device->params) {
		hg_mem_t *mem = hg_mem_spool_get(hg_quark_get_mem_id(device->params->self));

		if (!hg_mem_gc_mark(mem, device->params->self))
			return FALSE;
		if (!func(device->params->qinput_attributes, user_data))
			return FALSE;
		if (!func(device->params->qmedia_color, user_data))
			return FALSE;
		if (!func(device->params->qmedia_type, user_data))
			return FALSE;
		if (!func(device->params->qoutput_type, user_data))
			return FALSE;
		if (!func(device->params->qinstall, user_data))
			return FALSE;
		if (!func(device->params->qbegin_page, user_data))
			return FALSE;
		if (!func(device->params->qend_page, user_data))
			return FALSE;

		if (device->gc_mark) {
			if (!device->gc_mark(device, func, user_data))
				return FALSE;
		}
	}

	return TRUE;
}

/**
 * hg_device_open:
 * @mem:
 * @name:
 *
 * FIXME
 *
 * Returns:
 */
hg_device_t *
hg_device_open(hg_mem_t        *mem,
	       const hg_char_t *name)
{
	hg_device_t *retval = NULL;
	hg_char_t *basename, *modulename, *fullname;
	const hg_char_t *modpath;

	hg_return_val_if_fail (name != NULL, NULL, HG_e_VMerror);

	basename = g_path_get_basename(name);
	modulename = g_strdup_printf("libhgdev-%s.so", basename);
	if ((modpath = g_getenv("HIEROGLYPH_DEVICE_PATH")) != NULL) {
		hg_char_t **path_list = g_strsplit(modpath, G_SEARCHPATH_SEPARATOR_S, -1);
		hg_char_t *p, *path;
		hg_int_t i = 0;
		hg_usize_t len;

		for (i = 0; path_list[i] != NULL; i++) {
			p = path_list[i];

			while (*p && g_ascii_isspace(*p))
				p++;
			len = strlen(p);
			while (len > 0 && g_ascii_isspace(p[len - 1]))
				len--;
			path = g_strndup(p, len);
			if (path[0] != 0) {
				fullname = g_build_filename(path, modulename, NULL);
				retval = _hg_device_open(mem, fullname);
				g_free(fullname);
			}
			g_free(path);
			if (retval)
				break;
		}

		g_strfreev(path_list);
	}
	if (retval == NULL) {
		fullname = g_build_filename(HIEROGLYPH_DEVICEDIR, modulename, NULL);
		retval = _hg_device_open(mem, fullname);
		g_free(fullname);
	}
	if (retval == NULL) {
		hg_warning("No such device module: %s", basename);
	}

	g_free(modulename);
	g_free(basename);

	return retval;
}

/**
 * hg_device_destroy:
 * @device:
 *
 * FIXME
 */
void
hg_device_close(hg_device_t *device)
{
	GModule *module;

	hg_return_if_fail (device != NULL, HG_e_VMerror);
	hg_return_if_fail (device->finalizer != NULL, HG_e_VMerror);

	module = device->module;
	device->finalizer(device);

	if (module)
		g_module_close(module);
}

/**
 * hg_device_get_page_params:
 * @device:
 * @mem:
 * @check_dictfull:
 * @func:
 * @func_user_data:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_device_get_page_params(hg_device_t                *device,
			  hg_mem_t                   *mem,
			  hg_bool_t                   check_dictfull,
			  hg_pdev_name_lookup_func_t  func,
			  hg_pointer_t                func_user_data,
			  hg_pointer_t               *ret)
{
	hg_quark_t retval, q = Qnil, qn;
	hg_dict_t *d;
	hg_int_t i;
	hg_array_t *a;
	hg_usize_t size;

	hg_return_val_if_fail (device != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (device->params != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);
	hg_return_val_if_fail (func != NULL, Qnil, HG_e_VMerror);

	if (device->get_page_params_size)
		size = device->get_page_params_size(device);
	else
		size = HG_pdev_END;
	retval = hg_dict_new(mem, size, check_dictfull, (hg_pointer_t *)&d);
	if (retval == Qnil) {
		goto finalize;
	}
	for (i = HG_pdev_BEGIN + 1; i < size; i++) {
		q = Qnil;
		switch (i) {
		    case HG_pdev_InputAttributes:
			    if (device->params->qinput_attributes == Qnil) {
				    q = HG_QNULL;
			    } else {
				    q = hg_object_quark_copy(mem, device->params->qinput_attributes, NULL);
				    if (q == Qnil)
					    goto finalize;
			    }
			    break;
		    case HG_pdev_PageSize:
			    q = hg_array_new(mem, 2, (hg_pointer_t *)&a);
			    if (q == Qnil) {
				    goto finalize;
			    }
			    if (!hg_array_set(a, HG_QREAL (device->params->page_size.width), 0, FALSE))
				    goto error;
			    if (!hg_array_set(a, HG_QREAL (device->params->page_size.height), 1, FALSE))
				    goto error;
			    hg_mem_unlock_object(mem, q);
			    break;
		    case HG_pdev_MediaColor:
			    if (device->params->qmedia_color == Qnil) {
				    q = HG_QNULL;
			    } else {
				    q = hg_object_quark_copy(mem, device->params->qmedia_color, NULL);
				    if (q == Qnil)
					    goto finalize;
			    }
			    break;
		    case HG_pdev_MediaWeight:
			    q = HG_QREAL (device->params->media_weight);
			    break;
		    case HG_pdev_MediaType:
			    if (device->params->qmedia_type == Qnil) {
				    q = HG_QNULL;
			    } else {
				    q = hg_object_quark_copy(mem, device->params->qmedia_type, NULL);
				    if (q == Qnil)
					    goto finalize;
			    }
			    break;
		    case HG_pdev_ManualFeed:
			    q = HG_QBOOL (device->params->manual_feed);
			    break;
		    case HG_pdev_Orientation:
			    q = HG_QINT (device->params->orientation);
			    break;
		    case HG_pdev_AdvanceMedia:
			    q = HG_QINT (device->params->advance_media);
			    break;
		    case HG_pdev_AdvanceDistance:
			    q = HG_QINT (device->params->advance_distance);
			    break;
		    case HG_pdev_CutMedia:
			    q = HG_QINT (device->params->cut_media);
			    break;
		    case HG_pdev_HWResolution:
			    q = hg_array_new(mem, 2, (hg_pointer_t *)&a);
			    if (q == Qnil) {
				    goto finalize;
			    }
			    if (!hg_array_set(a, HG_QREAL (device->params->hw_resolution.x), 0, FALSE))
				    goto error;
			    if (!hg_array_set(a, HG_QREAL (device->params->hw_resolution.y), 1, FALSE))
				    goto error;
			    hg_mem_unlock_object(mem, q);
			    break;
		    case HG_pdev_ImagingBBox:
			    if (HG_REAL_IS_ZERO (device->params->imaging_bbox.x1) &&
				HG_REAL_IS_ZERO (device->params->imaging_bbox.y1) &&
				HG_REAL_IS_ZERO (device->params->imaging_bbox.x2) &&
				HG_REAL_IS_ZERO (device->params->imaging_bbox.y2)) {
				    q = HG_QNULL;
			    } else {
				    q = hg_array_new(mem, 4, (hg_pointer_t *)&a);
				    if (q == Qnil) {
					    goto finalize;
				    }
				    if (!hg_array_set(a, HG_QREAL (device->params->imaging_bbox.x1), 0, FALSE))
					    goto error;
				    if (!hg_array_set(a, HG_QREAL (device->params->imaging_bbox.y1), 1, FALSE))
					    goto error;
				    if (!hg_array_set(a, HG_QREAL (device->params->imaging_bbox.x2), 2, FALSE))
					    goto error;
				    if (!hg_array_set(a, HG_QREAL (device->params->imaging_bbox.y2), 3, FALSE))
					    goto error;
				    hg_mem_unlock_object(mem, q);
			    }
			    break;
		    case HG_pdev_Margins:
			    q = hg_array_new(mem, 2, (hg_pointer_t *)&a);
			    if (q == Qnil) {
				    goto finalize;
			    }
			    if (!hg_array_set(a, HG_QREAL (device->params->margins.x), 0, FALSE))
				    goto error;
			    if (!hg_array_set(a, HG_QREAL (device->params->margins.y), 1, FALSE))
				    goto error;
			    hg_mem_unlock_object(mem, q);
			    break;
		    case HG_pdev_MirrorPrint:
			    q = HG_QBOOL (device->params->mirror_print);
			    break;
		    case HG_pdev_NegativePrint:
			    q = HG_QBOOL (device->params->negative_print);
			    break;
		    case HG_pdev_Duplex:
			    q = HG_QBOOL (device->params->duplex);
			    break;
		    case HG_pdev_Tumble:
			    q = HG_QBOOL (device->params->tumble);
			    break;
		    case HG_pdev_OutputType:
			    if (device->params->qoutput_type == Qnil) {
				    q = HG_QNULL;
			    } else {
				    q = hg_object_quark_copy(mem, device->params->qoutput_type, NULL);
				    if (q == Qnil)
					    goto finalize;
			    }
			    break;
		    case HG_pdev_NumCopies:
			    if (device->params->num_copies < 0)
				    q = HG_QNULL;
			    else
				    q = HG_QINT (device->params->num_copies);
			    break;
		    case HG_pdev_Collate:
			    q = HG_QBOOL (device->params->collate);
			    break;
		    case HG_pdev_Jog:
			    q = HG_QINT (device->params->jog);
			    break;
		    case HG_pdev_OutputFaceUp:
			    q = HG_QBOOL (device->params->output_faceup);
			    break;
		    case HG_pdev_Separations:
			    q = HG_QBOOL (device->params->separations);
			    break;
		    case HG_pdev_Install:
			    q = hg_object_quark_copy(mem, device->params->qinstall, NULL);
			    if (q == Qnil)
				    goto finalize;
			    break;
		    case HG_pdev_BeginPage:
			    q = hg_object_quark_copy(mem, device->params->qbegin_page, NULL);
			    if (q == Qnil)
				    goto finalize;
			    break;
		    case HG_pdev_EndPage:
			    q = hg_object_quark_copy(mem, device->params->qend_page, NULL);
			    if (q == Qnil)
				    goto finalize;
			    break;
		    default:
			    if (device->get_page_param) {
				    if ((q = device->get_page_param(device, i)) != Qnil)
					    break;
			    }
			    hg_warning("Unknown pagedevice parameter: %d\n", i);
			    break;
		}
		if (q != Qnil) {
			qn = func(i, func_user_data);
			if (!hg_dict_add(d, qn, q, FALSE)) {
			  error:
				hg_object_free(mem, q);
				goto finalize;
			}
		}
	}

	if (ret)
		*ret = d;
	else
		hg_mem_unlock_object(d->o.mem, retval);
  finalize:
	if (!HG_ERROR_IS_SUCCESS0 ()) {
		hg_object_free(d->o.mem, retval);
		retval = Qnil;
	}

	return retval;
}

/**
 * hg_device_install:
 * @device:
 * @vm:
 *
 * FIXME
 */
void
hg_device_install(hg_device_t *device,
		  hg_vm_t     *vm)
{
	hg_return_if_fail (device != NULL, HG_e_VMerror);
	hg_return_if_fail (vm != NULL, HG_e_VMerror);

	if (device->install)
		device->install(device, vm);
}

/**
 * hg_device_is_pending_draw:
 * @device:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_device_is_pending_draw(hg_device_t *device)
{
	hg_return_val_if_fail (device != NULL, FALSE, HG_e_VMerror);

	if (device->is_pending_draw)
		return device->is_pending_draw(device);

	return FALSE;
}

/**
 * hg_device_draw:
 * @device:
 *
 * FIXME
 */
void
hg_device_draw(hg_device_t *device)
{
	hg_return_if_fail (device != NULL, HG_e_VMerror);

	if (device->draw)
		device->draw(device);
}

/**
 * hg_device_get_ctm:
 * @device:
 * @matrix:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_device_get_ctm(hg_device_t *device,
		  hg_matrix_t *matrix)
{
	hg_return_val_if_fail (device != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (matrix != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (device->get_ctm != NULL, FALSE, HG_e_VMerror);

	return device->get_ctm(device, matrix);
}

/**
 * hg_device_eofill:
 * @device:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_device_eofill(hg_device_t *device,
		 hg_gstate_t *gstate)
{
	hg_bool_t retval;

	hg_return_val_if_fail (device != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (device->eofill != NULL, FALSE, HG_e_VMerror);

	retval = device->eofill(device, gstate);
	if (retval) {
		hg_quark_t qpath = hg_path_new(gstate->o.mem, NULL);

		if (qpath == Qnil) {
			return FALSE;
		} else {
			hg_gstate_set_path(gstate, qpath);
		}
	}

	return retval;
}

/**
 * hg_device_fill:
 * @device:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_device_fill(hg_device_t *device,
	       hg_gstate_t *gstate)
{
	hg_bool_t retval;

	hg_return_val_if_fail (device != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (device->fill != NULL, FALSE, HG_e_VMerror);

	retval = device->fill(device, gstate);
	if (retval) {
		hg_quark_t qpath = hg_path_new(gstate->o.mem, NULL);

		if (qpath == Qnil) {
			return FALSE;
		} else {
			hg_gstate_set_path(gstate, qpath);
		}
	}

	return retval;
}

/**
 * hg_device_stroke:
 * @device:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_device_stroke(hg_device_t *device,
		 hg_gstate_t *gstate)
{
	hg_bool_t retval;

	hg_return_val_if_fail (device != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (device->stroke != NULL, FALSE, HG_e_VMerror);

	retval = device->stroke(device, gstate);
	if (retval) {
		hg_quark_t qpath = hg_path_new(gstate->o.mem, NULL);

		if (qpath == Qnil) {
			return FALSE;
		} else {
			hg_gstate_set_path(gstate, qpath);
		}
	}

	return retval;
}

/* null device */
/*< private >*/

typedef struct _hg_null_device_t	hg_null_device_t;

struct _hg_null_device_t {
	hg_device_t parent;
};

static hg_bool_t
_hg_device_null_nop(hg_device_t *device,
		    hg_gstate_t *gstate)
{
	return TRUE;
}

static void
_hg_device_null_destroy(hg_device_t *device)
{
	hg_null_device_t *devnul = (hg_null_device_t *)device;

	g_free(devnul);
}

static hg_bool_t
_hg_device_null_get_ctm(hg_device_t *device,
			hg_matrix_t *matrix)
{
	hg_matrix_init_identity(matrix);

	return TRUE;
}

/*< public >*/
hg_device_t *
hg_device_null_new(hg_mem_t *mem)
{
	hg_null_device_t *retval = g_new0(hg_null_device_t, 1);

	retval->parent.finalizer = _hg_device_null_destroy;
	retval->parent.get_ctm = _hg_device_null_get_ctm;
	retval->parent.fill = _hg_device_null_nop;
	retval->parent.stroke = _hg_device_null_nop;

	if (!_hg_device_init_page_params((hg_device_t *)retval, mem)) {
		g_free(retval);
		return NULL;
	}

	return (hg_device_t *)retval;
}
