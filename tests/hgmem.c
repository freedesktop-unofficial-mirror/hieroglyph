#include <stdio.h>
#include <string.h>
#include <hieroglyph/hgmem.h>

int
main(void)
{
	HgAllocator *allocator;
	HgMemPool *pool;
	gchar *s, *s2;

	hg_mem_init();
	allocator = hg_allocator_new();
	pool = hg_mem_pool_new(allocator, "test", 256, TRUE);
	if (pool == NULL) {
		g_print("Failed to create a pool.\n");
		return 1;
	}
	s = hg_mem_alloc(pool, 16);
	printf("%p\n", s);
	s2 = hg_mem_alloc(pool, 256);
	printf("%p:%p\n", s, s2);
	if (s == NULL || s2 == NULL) {
		g_print("Failed to alloc a memory: %p %p.\n", s, s2);
		return 1;
	}
	strcpy(s, "test");
	strcpy(s2, "test");
	if (strcmp(s, s2)) {
		g_print("Failed to compare the memory.\n");
		return 1;
	}
	hg_mem_free(s2);
	hg_mem_free(s);
	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_mem_finalize();

	return 0;
}
