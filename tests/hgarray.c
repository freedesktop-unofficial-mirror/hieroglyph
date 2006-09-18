#include <hieroglyph/hgallocator-bfit.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgvaluenode.h>

int
main(void)
{
	HG_MEM_INIT;

	HgAllocator *allocator;
	HgMemPool *pool;
	HgArray *a, *b;
	HgValueNode *node;
	guint i;

	hg_value_node_init();
	allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
	pool = hg_mem_pool_new(allocator, "test", 128, HG_MEM_RESIZABLE);
	if (pool == NULL) {
		g_print("Failed to create a memory pool.\n");
		return 1;
	}
	a = hg_array_new(pool, 5);
	if (a == NULL) {
		g_print("Failed to create an Array.");
		return 1;
	}

	for (i = 0; i < 5; i++) {
		HG_VALUE_MAKE_INTEGER (pool, node, i + 1);
		if (!hg_array_append(a, node)) {
			g_print("Failed to append a node: %d\n", i);
			return 1;
		}
	}

	for (i = 0; i < 5; i++) {
		node = hg_array_index(a, i);
		if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != (i + 1)) {
			g_print("Failed to get an index %d\n", i);
			g_print("  actual result: %d, expected result: %d\n", HG_VALUE_GET_INTEGER (node), i + 1);
			return 1;
		}
	}
	HG_VALUE_MAKE_INTEGER (pool, node, 6);
	hg_array_replace(a, node, 0);
	node = hg_array_index(a, 0);
	if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != 6) {
		g_print("Failed to get an index %d: take 2\n", 0);
		g_print("  actual result: %d, expected result: %d\n", HG_VALUE_GET_INTEGER (node), 6);
		return 1;
	}
	hg_array_remove(a, 0);
	if (hg_array_length(a) != 4) {
		g_print("Failed to remove Array at index %d\n", 0);
		return 0;
	}
	for (i = 0; i < 4; i++) {
		node = hg_array_index(a, i);
		if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != (i + 2)) {
			g_print("Failed to get an index %d: take 3\n", i);
			g_print("  actual result: %d, expected result: %d\n", HG_VALUE_GET_INTEGER (node), i + 2);
			return 1;
		}
	}

	a = hg_array_new(pool, -1);
	if (a == NULL) {
		g_print("Failed to create an Array.");
		return 1;
	}

	for (i = 0; i <= 256; i++) {
		HG_VALUE_MAKE_INTEGER (pool, node, i + 1);
		if (!hg_array_append(a, node)) {
			g_print("Failed to append a node: %d\n", i);
			return 1;
		}
	}
	for (i = 0; i <= 256; i++) {
		node = hg_array_index(a, i);
		if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != (i + 1)) {
			g_print("Failed to get an index %d\n", i);
			g_print("  actual result: %d, expected result: %d\n", HG_VALUE_GET_INTEGER (node), i + 1);
			return 1;
		}
	}
	b = hg_array_make_subarray(pool, a, 10, 19);
	for (i = 0; i < 10; i++) {
		node = hg_array_index(b, i);
		if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != (i + 11)) {
			g_print("Failed to get an index %d\n", i);
			g_print("  actual result: %d, expected result: %d\n", HG_VALUE_GET_INTEGER (node), i + 11);
			return 1;
		}
	}
	HG_VALUE_MAKE_INTEGER (pool, node, 1010);
	hg_array_replace(b, node, 0);
	node = hg_array_index(b, 0);
	if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != 1010) {
		g_print("Failed to get an index 0 for subarray\n");
		g_print("  actual result: %d, expected result: 1010\n", HG_VALUE_GET_INTEGER (node));
		return 1;
	}
	node = hg_array_index(a, 10);
	if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != 1010) {
		g_print("Failed to get an index 10: from parent array for subarray\n");
		g_print("  actual result: %d, expected result: 1010\n", HG_VALUE_GET_INTEGER (node));
		return 1;
	}

	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_value_node_finalize();
	hg_mem_finalize();

	return 0;
}
