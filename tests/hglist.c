#include <hieroglyph/hgallocator-bfit.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hglist.h>

int
main(void)
{
	HG_MEM_INIT;

	HgAllocator *allocator;
	HgMemPool *pool;
	HgList *list;
	HgListIter iter;
	gint tc1[] = {1, 2, 3, 0};
	gint tc2[] = {1, 3, 0};
	gint i;

	allocator = hg_allocator_new(hg_allocator_bfit_get_vtable());
	pool = hg_mem_pool_new(allocator, "test", 128, HG_MEM_RESIZABLE);
	if (pool == NULL) {
		g_print("Failed to create a memory pool.\n");
		return 1;
	}
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));

	iter = hg_list_iter_new(list);
	if (iter == NULL) {
		g_print("Failed to create an iter.\n");
		return 1;
	}

	for (i = 0; ; i++) {
		if (tc1[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d, but actually %d",
				tc1[i], GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	if (i != 2) {
		g_print("Expected list size 3 but actually was %d\n", i + 1);
		return 1;
	}
	list = hg_list_remove(list, GINT_TO_POINTER (2));
	hg_list_get_iter_first(list, iter);
	for (i = 0; ; i++) {
		if (tc2[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d, but actually %d",
				tc2[i], GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	if (i != 1) {
		g_print("Expected list size 2 but actually was %d\n", i + 1);
		return 1;
	}

	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_mem_finalize();

	return 0;
}
