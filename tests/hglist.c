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
	HgListIter iter, iter2;
	gint tc1[] = {1, 2, 3, 0};
	gint tc2[] = {1, 3, 0};
	gint tc3[] = {2, 3, 0};
	gint tc4[] = {1, 2, 0};
	gint tc5[] = {4, 1, 2, 3, 5, 0};
	gint tc6[] = {4, 1, 2, 3, 0};
	gint tc7[] = {1, 4, 2, 3, 5, 0};
	gint tc8[] = {1, 5, 2, 3, 4, 0};
	gint tc9[] = {2, 3, 4, 1, 5, 0};
	gint tc10[] = {2, 3, 4, 1, 0};
	gint tc11[] = {1, 3, 4, 2, 5, 0};
	gint tc12[] = {1, 3, 4, 5, 2, 0};
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

	/* testcase 2 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));

	list = hg_list_remove(list, GINT_TO_POINTER (1));
	iter = hg_list_iter_new(list);
	if (iter == NULL) {
		g_print("Failed to create an iter.\n");
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc3[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d, but actually %d",
				tc3[i], GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	if (i != 1) {
		g_print("Expected list size 2 but actually was %d\n", i + 1);
		return 1;
	}

	/* testcase 3 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));

	list = hg_list_remove(list, GINT_TO_POINTER (3));
	iter = hg_list_iter_new(list);
	if (iter == NULL) {
		g_print("Failed to create an iter.\n");
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc4[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d, but actually %d",
				tc4[i], GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	if (i != 1) {
		g_print("Expected list size 2 but actually was %d\n", i + 1);
		return 1;
	}

	/* testcase 4 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));

	list = hg_list_remove(list, GINT_TO_POINTER (1));
	if (list != NULL) {
		g_print("Expected list size 0 but actually was %d\n", hg_list_length(list));
		return 1;
	}

	/* testcase 5 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));
	list = hg_list_append(list, GINT_TO_POINTER (5));

	iter = hg_list_iter_new(list);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	hg_list_get_iter_previous(list, iter2);
	list = hg_list_iter_roll(iter, iter2, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 5) {
		g_print("Expected list size 5 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc5[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc5[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	/* testcase 6 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));

	iter = hg_list_iter_new(list);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	list = hg_list_iter_roll(iter, iter2, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 4) {
		g_print("Expected list size 4 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc6[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc6[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	/* testcase 7 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));
	list = hg_list_append(list, GINT_TO_POINTER (5));

	iter = hg_list_iter_new(list);
	hg_list_get_iter_next(list, iter);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	hg_list_get_iter_previous(list, iter2);
	list = hg_list_iter_roll(iter, iter2, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 5) {
		g_print("Expected list size 5 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc7[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc7[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	/* testcase 8 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));
	list = hg_list_append(list, GINT_TO_POINTER (5));

	iter = hg_list_iter_new(list);
	hg_list_get_iter_next(list, iter);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	list = hg_list_iter_roll(iter, iter2, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 5) {
		g_print("Expected list size 5 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc8[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc8[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	/* testcase 9 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));
	list = hg_list_append(list, GINT_TO_POINTER (5));

	iter = hg_list_iter_new(list);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	hg_list_get_iter_previous(list, iter2);
	list = hg_list_iter_roll(iter2, iter, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 5) {
		g_print("Expected list size 5 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc9[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc9[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	/* testcase 10 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));

	iter = hg_list_iter_new(list);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	list = hg_list_iter_roll(iter2, iter, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 4) {
		g_print("Expected list size 4 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc10[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc10[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	/* testcase 11 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));
	list = hg_list_append(list, GINT_TO_POINTER (5));

	iter = hg_list_iter_new(list);
	hg_list_get_iter_next(list, iter);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	hg_list_get_iter_previous(list, iter2);
	list = hg_list_iter_roll(iter2, iter, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 5) {
		g_print("Expected list size 5 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc11[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc11[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	/* testcase 12 */
	list = hg_list_new(pool);
	if (list == NULL) {
		g_print("Failed to create an list.\n");
		return 1;
	}

	list = hg_list_append(list, GINT_TO_POINTER (1));
	list = hg_list_append(list, GINT_TO_POINTER (2));
	list = hg_list_append(list, GINT_TO_POINTER (3));
	list = hg_list_append(list, GINT_TO_POINTER (4));
	list = hg_list_append(list, GINT_TO_POINTER (5));

	iter = hg_list_iter_new(list);
	hg_list_get_iter_next(list, iter);
	iter2 = hg_list_iter_new(list);
	hg_list_get_iter_last(list, iter2);
	list = hg_list_iter_roll(iter2, iter, 1);
	hg_list_get_iter_first(list, iter);
	if (hg_list_length(list) != 5) {
		g_print("Expected list size 5 but actually was %d\n", hg_list_length(list));
		return 1;
	}
	for (i = 0; ; i++) {
		if (tc12[i] != GPOINTER_TO_INT (hg_list_iter_get_data(iter))) {
			g_print("Failed to compare: expected %d[%d], but actually %d\n",
				tc12[i], i, GPOINTER_TO_INT (hg_list_iter_get_data(iter)));
			return 1;
		}
		if (!hg_list_get_iter_next(list, iter))
			break;
	}
	hg_list_iter_free(iter);
	hg_list_iter_free(iter2);

	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);
	hg_mem_finalize();

	return 0;
}
