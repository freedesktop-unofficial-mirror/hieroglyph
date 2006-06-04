#include <hieroglyph/hgallocator-bfit.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgdict.h>
#include <hieroglyph/hgvaluenode.h>

int
main(void)
{
	HG_MEM_INIT;

	HgAllocator *allocator;
	HgMemPool *pool;
	HgDict *d;
	HgValueNode *key, *val, *node;

	allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
	pool = hg_mem_pool_new(allocator, "test", 384, FALSE);
	if (pool == NULL) {
		g_print("Failed to create a memory pool.\n");
		return 1;
	}
	d = hg_dict_new(pool, 5);
	if (d == NULL) {
		g_print("Failed to create a dictionary.\n");
		return 1;
	}

	HG_VALUE_MAKE_INTEGER (pool, key, 50);
	HG_VALUE_MAKE_INTEGER (pool, val, 10);
	if (!hg_dict_insert(pool, d, key, val)) {
		g_print("Failed to insert\n");
		return 1;
	}
	node = hg_dict_lookup(d, key);
	if (node == NULL) {
		g_print("Failed to lookup: %d\n", HG_VALUE_GET_INTEGER (key));
		return 1;
	}
	if (!HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != 10) {
		g_print("Failed to lookup value\n");
		g_print("  Expected: %d, Actual: %d\n", 10, HG_VALUE_GET_INTEGER (node));
		return 1;
	}

//	hg_mem_free(d);
	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_mem_finalize();

	return 0;
}
