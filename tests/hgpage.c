#include <math.h>
#include <hieroglyph/hgpage.h>

int
test_hg_page_get_size(void)
{
	gdouble w, h;

#define CMP_SIZE(size, ww, hh)					\
	G_STMT_START {						\
		gdouble uw = ww / 25.4 * 72;			\
		gdouble uh = hh / 25.4 * 72;			\
		if (!hg_page_get_size((size), &w, &h))		\
			return 1;				\
		if (fabs(w - uw) > fabs(DBL_EPSILON * w) ||	\
		    fabs(h - uh) > fabs(DBL_EPSILON * h)) {	\
			g_print("expect: %fmm x %fmm\n", (gdouble)ww, (gdouble)hh); \
			g_print("expect: %f x %f\n", (gdouble)uw, (gdouble)uh);	\
			g_print("actual: %f x %f\n", w, h);	\
			return 1;				\
		}						\
	} G_STMT_END

	CMP_SIZE (HG_PAGE_4A0, 1682, 2378);
	CMP_SIZE (HG_PAGE_2A0, 1189, 1682);
	CMP_SIZE (HG_PAGE_A0, 841, 1189);
	CMP_SIZE (HG_PAGE_A1, 594, 841);
	CMP_SIZE (HG_PAGE_A2, 420, 594);
	CMP_SIZE (HG_PAGE_A3, 297, 420);
	CMP_SIZE (HG_PAGE_A4, 210, 297);
	CMP_SIZE (HG_PAGE_A5, 148, 210);
	CMP_SIZE (HG_PAGE_A6, 105, 148);
	CMP_SIZE (HG_PAGE_A7, 74, 105);

	CMP_SIZE (HG_PAGE_B0, 1000, 1414);
	CMP_SIZE (HG_PAGE_B1, 707, 1000);
	CMP_SIZE (HG_PAGE_B2, 500, 707);
	CMP_SIZE (HG_PAGE_B3, 353, 500);
	CMP_SIZE (HG_PAGE_B4, 250, 353);
	CMP_SIZE (HG_PAGE_B5, 176, 250);
	CMP_SIZE (HG_PAGE_B6, 125, 176);
	CMP_SIZE (HG_PAGE_B7, 88, 125);

	CMP_SIZE (HG_PAGE_JIS_B0, 1030, 1456);
	CMP_SIZE (HG_PAGE_JIS_B1, 728, 1030);
	CMP_SIZE (HG_PAGE_JIS_B2, 515, 728);
	CMP_SIZE (HG_PAGE_JIS_B3, 364, 515);
	CMP_SIZE (HG_PAGE_JIS_B4, 257, 364);
	CMP_SIZE (HG_PAGE_JIS_B5, 182, 257);
	CMP_SIZE (HG_PAGE_JIS_B6, 128, 182);

	CMP_SIZE (HG_PAGE_C0, 917, 1297);
	CMP_SIZE (HG_PAGE_C1, 648, 917);
	CMP_SIZE (HG_PAGE_C2, 458, 648);
	CMP_SIZE (HG_PAGE_C3, 324, 458);
	CMP_SIZE (HG_PAGE_C4, 229, 324);
	CMP_SIZE (HG_PAGE_C5, 162, 229);
	CMP_SIZE (HG_PAGE_C6, 114, 162);
	CMP_SIZE (HG_PAGE_C7, 81, 114);

	CMP_SIZE (HG_PAGE_JAPAN_POSTCARD, 100, 148);
	CMP_SIZE (HG_PAGE_LETTER, 215.9, 279.4);
	CMP_SIZE (HG_PAGE_LEGAL, 215.9, 355.6);

	return 0;
}

int
main(void)
{
	if (test_hg_page_get_size() != 0)
		return 1;

	return 0;
}
