#include <stdio.h>
#include <string.h>
#include <hieroglyph/hgallocator-ffit.h>
#include <hieroglyph/hgmem.h>

int
main(void)
{
	HgAllocator *allocator;
	HgMemPool *pool;
	gint i;
	gchar *s;

	hg_mem_init();
	allocator = hg_allocator_new(hg_allocator_ffit_get_vtable());
	pool = hg_mem_pool_new(allocator, "test", 90000000, FALSE);
	if (pool == NULL) {
		g_print("Failed to create a pool.\n");
		return 1;
	}
	for (i = 0; i < 1000000; i++) {
		s = hg_mem_alloc(pool, 16);
	}
	s = NULL;
	hg_mem_garbage_collection(pool);
	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_mem_finalize();

	return 0;
}
