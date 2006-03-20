#include <hieroglyph/hgdevice.h>
#include <hieroglyph/hgvalue-integer.h>
#include <hieroglyph/hgvalue-string.h>

int
main(void)
{
	HgDevice *device;
	int retval = 1;

	hieroglyph_init();

	device = hieroglyph_device_new("ps-embedded");
	if (device != NULL) {
/*
		cairo_select_font_face(cr, "Sans",
				       CAIRO_FONT_SLANT_NORMAL,
				       CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, 20);
*/

		hieroglyph_device_move_to(device,
					  hieroglyph_value_integer_new(100),
					  hieroglyph_value_integer_new(100));
		hieroglyph_device_show(device,
				       hieroglyph_value_string_new("日本語"));
//		cairo_show_page(cr);

		hieroglyph_object_unref(HG_OBJECT (device));
//		cairo_destroy(cr);
//		cairo_surface_destroy(cs);
		retval = 0;
	}

	return retval;
}
