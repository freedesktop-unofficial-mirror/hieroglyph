#include <hieroglyph/operator.h>

int
main(void)
{
	if (HG_op_setpattern != 225) {
		g_print("actual result: setpattern = %d\n", HG_op_store);
		return 1;
	}
	if (HG_op_sym_eq != 256) {
		g_print("actual result: /= = %d\n", HG_op_sym_eq);
		return 1;
	}
	if (HG_op_POSTSCRIPT_RESERVED_END != 481) {
		g_print("actual result: %d\n", HG_op_POSTSCRIPT_RESERVED_END);
		return 1;
	}

	return 0;
}
