#include <hieroglyph/hgallocator-ffit.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgstring.h>

int
main(void)
{
	HgAllocator *allocator;
	HgMemPool *pool;
	HgString *s;

	hg_mem_init();
	allocator = hg_allocator_new();
	pool = hg_mem_pool_new(allocator, "test", 256, TRUE);
	if (pool == NULL) {
		g_print("Failed to create a memory pool.\n");
		return 1;
	}
	s = hg_string_new(pool, 256);
	if (s == NULL) {
		g_print("Failed to create a string.\n");
		return 1;
	}

	hg_string_append(s, "This is a test program for HgString.", -1);
	if (!hg_string_compare_with_raw(s, "This is a test program for HgString.", -1)) {
		g_print("Failed to compare: %s\n", hg_string_get_string(s));
		return 1;
	}

	hg_string_insert_c(s, 'a', 2);
	hg_string_insert_c(s, 't', 3);
	if (!hg_string_compare_with_raw(s, "That is a test program for HgString.", -1)) {
		g_print("Failed to compare: %s\n", hg_string_get_string(s));
		return 1;
	}

	s = hg_string_new(pool, -1);
	if (s == NULL) {
		g_print("Failed to create a string2.\n");
		return 1;
	}
	hg_string_append(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", -1);
	hg_string_append(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", -1);
	hg_string_append(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", -1);
	hg_string_append(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", -1);
	hg_string_append(s, "0123456789", -1);
	hg_string_append(s, "0123456789", -1);
	hg_string_append(s, "0123456789", -1);
	hg_string_append(s, "0123456789", -1);
	hg_string_append_c(s, ' ');
	hg_string_append_c(s, 't');
	hg_string_append_c(s, 'e');
	hg_string_append_c(s, 's');
	hg_string_append_c(s, 't');
	hg_string_append_c(s, ' ');
	hg_string_append_c(s, 't');
	hg_string_append_c(s, 'e');
	hg_string_append_c(s, 's');
	hg_string_append_c(s, 't');
	if (!hg_string_compare_with_raw(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789012345678901234567890123456789 test test", -1)) {
		g_print("Failed to compare (take 2): %s\n", hg_string_get_string(s));
		return 1;
	}

	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_mem_finalize();

	return 0;
}
