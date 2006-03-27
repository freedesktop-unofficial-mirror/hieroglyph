#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgarray.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/hgstring.h>
#include <hieroglyph/hgdict.h>


int
main(void)
{
	HgAllocator *allocator;
	HgMemPool *pool;
	HgMemSnapshot *snap;
	HgValueNode *node, *key, *val;
	HgArray *array;
	HgString *string;
	HgDict *dict;

	hg_mem_init();

	allocator = hg_allocator_new();
	pool = hg_mem_pool_new(allocator, "test", 256, TRUE);
	if (pool == NULL) {
		g_print("Failed to create a pool\n");
		return 1;
	}
//	g_print("creating array...\n");
	array = hg_array_new(pool, 2);
//	g_print("creating string...\n");
	string = hg_string_new(pool, 32);
//	g_print("appending string...\n");
	hg_string_append(string, "This is a pen.", -1);
//	g_print("creating node...\n");
	HG_VALUE_MAKE_INTEGER (pool, node, 3);
//	g_print("appending node to array...\n");
	hg_array_append(array, node);
//	g_print("creating node...\n");
	HG_VALUE_MAKE_STRING (node, string);
//	g_print("appending node to array...\n");
	hg_array_append(array, node);
	dict = hg_dict_new(pool, 2);
	HG_VALUE_MAKE_NAME_STATIC (pool, key, "test");
	HG_VALUE_MAKE_BOOLEAN (pool, val, TRUE);
	hg_dict_insert(pool, dict, key, val);

	g_print("creating snapshot...\n");
	snap = hg_mem_pool_save_snapshot(pool);

	node = hg_array_index(array, 0);
	HG_VALUE_SET_INTEGER (node, 10);
	hg_string_insert_c(string, 'a', 2);
	hg_string_insert_c(string, 't', 3);
	HG_VALUE_MAKE_INTEGER (pool, val, 20);
	hg_dict_insert(pool, dict, key, val);
	node = hg_dict_lookup(dict, key);

	if (node == NULL || !HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != 20) {
		g_print("Failed to lookup dict.");
		return 1;
	}

	g_print("restoring snapshot...\n");
	if (!hg_mem_pool_restore_snapshot(pool, snap)) {
		g_print("Failed to restore from snapshot.");
		return 1;
	}
	node = hg_array_index(array, 0);
	if (!HG_IS_VALUE_INTEGER (node) || HG_VALUE_GET_INTEGER (node) != 3) {
		g_print("Failed to restore from snapshot.\n  Expected: 3\n  Actual:  %d\n", HG_VALUE_GET_INTEGER (node));
		return 1;
	}
	node = hg_array_index(array, 1);
	if (!HG_IS_VALUE_STRING (node) || !hg_string_compare_with_raw(HG_VALUE_GET_STRING (node), "That is a pen.", -1)) {
		g_print("Failed to restore from snapshot.\n  Expected: That is a pen.\n  Actual:  %s\n", hg_string_get_string(HG_VALUE_GET_STRING (node)));
		return 1;
	}
	node = hg_dict_lookup(dict, key);

	if (node == NULL || !HG_IS_VALUE_BOOLEAN (node) || HG_VALUE_GET_BOOLEAN (node) != TRUE) {
		g_print("Failed to lookup dict after restoring.");
		return 1;
	}
	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_mem_finalize();

	return 0;
}
