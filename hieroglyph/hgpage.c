/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpage.c
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hgpage.h"
#include "hglog.h"


/*
 * Private Functions
 */
static HgPage *
_hg_page_new(void)
{
	HgPage *retval;

	retval = g_new(HgPage, 1);
	retval->width = 0;
	retval->height = 0;
	retval->node = NULL;
	retval->last_node = NULL;

	return retval;
}

/*
 * Public Functions
 */
HgPage *
hg_page_new(void)
{
	return hg_page_new_with_pagesize(HG_PAGE_A4);
}

HgPage *
hg_page_new_with_pagesize(HgPageSize size)
{
	gdouble width, height;

	if (!hg_page_get_size(size, &width, &height)) {
		hg_log_warning("Failed to get a paper size.");
		return NULL;
	}
	return hg_page_new_with_size(width, height);
}

HgPage *
hg_page_new_with_size(gdouble width,
		      gdouble height)
{
	HgPage *retval;

	retval = _hg_page_new();
	if (retval == NULL) {
		hg_log_warning("Failed to create a page object.");
		return NULL;
	}
	retval->width = width;
	retval->height = height;

	return retval;
}

void
hg_page_destroy(HgPage *page)
{
	g_return_if_fail (page != NULL);

	if (page->node)
		g_list_free(page->node);

	g_free(page);
}

gboolean
hg_page_get_size(HgPageSize  size,
		 gdouble    *width,
		 gdouble    *height)
{
	guint i, j, w, h;

	g_return_val_if_fail (width != NULL, FALSE);
	g_return_val_if_fail (height != NULL, FALSE);

	switch (size) {
	    case HG_PAGE_4A0:
		    *width = 1682;
		    *height = 2378;
		    break;
	    case HG_PAGE_2A0:
		    *width = 1189;
		    *height = 1682;
		    break;
	    case HG_PAGE_A0:
	    case HG_PAGE_A1:
	    case HG_PAGE_A2:
	    case HG_PAGE_A3:
	    case HG_PAGE_A4:
	    case HG_PAGE_A5:
	    case HG_PAGE_A6:
	    case HG_PAGE_A7:
		    i = size - HG_PAGE_A0;
		    if ((i % 2) == 0) {
			    w = 841;
			    h = 1189;
		    } else {
			    w = 594;
			    h = 841;
		    }
		    for (j = 0; j < i / 2; j++) {
			    w /= 2;
			    h /= 2;
		    }
		    *width = w;
		    *height = h;
		    break;
	    case HG_PAGE_B0:
	    case HG_PAGE_B1:
	    case HG_PAGE_B2:
	    case HG_PAGE_B3:
	    case HG_PAGE_B4:
	    case HG_PAGE_B5:
	    case HG_PAGE_B6:
	    case HG_PAGE_B7:
		    i = size - HG_PAGE_B0;
		    if ((i % 2) == 0) {
			    w = 1000;
			    h = 1414;
		    } else {
			    w = 707;
			    h = 1000;
		    }
		    for (j = 0; j < i / 2; j++) {
			    w /= 2;
			    h /= 2;
		    }
		    *width = w;
		    *height = h;
		    break;
	    case HG_PAGE_JIS_B0:
	    case HG_PAGE_JIS_B1:
	    case HG_PAGE_JIS_B2:
	    case HG_PAGE_JIS_B3:
	    case HG_PAGE_JIS_B4:
	    case HG_PAGE_JIS_B5:
	    case HG_PAGE_JIS_B6:
		    i = size - HG_PAGE_JIS_B0;
		    if ((i % 2) == 0) {
			    w = 1030;
			    h = 1456;
		    } else {
			    w = 728;
			    h = 1030;
		    }
		    for (j = 0; j < i / 2; j++) {
			    w /= 2;
			    h /= 2;
		    }
		    *width = w;
		    *height = h;
		    break;
	    case HG_PAGE_C0:
	    case HG_PAGE_C1:
	    case HG_PAGE_C2:
	    case HG_PAGE_C3:
	    case HG_PAGE_C4:
	    case HG_PAGE_C5:
	    case HG_PAGE_C6:
	    case HG_PAGE_C7:
		    i = size - HG_PAGE_C0;
		    if ((i % 2) == 0) {
			    w = 917;
			    h = 1297;
		    } else {
			    w = 648;
			    h = 917;
		    }
		    for (j = 0; j < i / 2; j++) {
			    w /= 2;
			    h /= 2;
		    }
		    *width = w;
		    *height = h;
		    break;
	    case HG_PAGE_LETTER:
		    *width = 215.9;
		    *height = 279.4;
		    break;
	    case HG_PAGE_LEGAL:
		    *width = 215.9;
		    *height = 355.6;
		    break;
	    case HG_PAGE_JAPAN_POSTCARD:
		    *width = 100;
		    *height = 148;
		    break;
	    default:
		    hg_log_warning("Unknown page size type %d", size);
		    *width = 0;
		    *height = 0;
		    return FALSE;
	}
	/* convert mm to unit: 1mm = 1/25.4 inch : 1 inch = 72 unit */
	*width = (*width / 25.4 * 72);
	*height = (*height / 25.4 * 72);

	return TRUE;
}

void
hg_page_append_node(HgPage   *page,
		    HgRender *render)
{
	GList *l;

	g_return_if_fail (page != NULL);
	g_return_if_fail (render != NULL);

	l = g_list_alloc();
	l->data = render;
	if (page->last_node == NULL) {
		page->last_node = page->node = l;
	} else {
		page->last_node->next = l;
		l->prev = page->last_node;
		page->last_node = l;
	}
}
