#include <libretto/lbtypes.h>

int
main(void)
{
	if (LB_op_setpattern != 225) {
		g_print("actual result: setpattern = %d\n", LB_op_store);
		return 1;
	}
	if (LB_op_sym_eq != 256) {
		g_print("actual result: /= = %d\n", LB_op_sym_eq);
		return 1;
	}
	if (LB_op_POSTSCRIPT_RESERVED_END != 481) {
		g_print("actual result: %d\n", LB_op_POSTSCRIPT_RESERVED_END);
		return 1;
	}

	return 0;
}
