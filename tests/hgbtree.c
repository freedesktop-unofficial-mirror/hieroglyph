#include <stdio.h>
#include <string.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgallocator-libc.h>
#include <hieroglyph/hgbtree.h>

void
tree2string(HgBTreePage *page, GString *string)
{
	static gint depth = 0;
	guint16 i;

	if (page == NULL) {
		g_string_append_c(string, '.');
		return;
	}
	g_string_append_c(string, '(');
	depth++;
	for (i = 0; i < page->n_data; i++) {
		tree2string(page->page[i], string);
		g_string_append_printf(string, "%d", GPOINTER_TO_INT(page->val[i]));
	}
	tree2string(page->page[page->n_data], string);
	g_string_append_c(string, ')');
	depth--;
}

int
main(void)
{
	HG_MEM_INIT;

	HgAllocator *allocator = hg_allocator_new(hg_allocator_libc_get_vtable());
	HgMemPool *pool = hg_mem_pool_new(allocator, "btree_test", 256, FALSE);
	HgBTree *tree = hg_btree_new(pool, 2);
	GString *string = g_string_new(NULL);
	gchar *test1 = "(.0.1.2.3.)";
	gchar *test2 = "((.0.1.)2(.3.4.))";
	gchar *test = "(((((.0.1.)2(.3.4.)5(.6.7.))8((.9.10.)11(.12.13.)14(.15.16.))17((.18.19.)20(.21.22.)23(.24.25.)))26(((.27.28.)29(.30.31.)32(.33.34.))35((.36.37.)38(.39.40.)41(.42.43.))44((.45.46.)47(.48.49.)50(.51.52.)))53(((.54.55.)56(.57.58.)59(.60.61.))62((.63.64.)65(.66.67.)68(.69.70.))71((.72.73.)74(.75.76.)77(.78.79.))))80((((.81.82.)83(.84.85.)86(.87.88.))89((.90.91.)92(.93.94.)95(.96.97.))98((.99.100.)101(.102.103.)104(.105.106.)))107(((.108.109.)110(.111.112.)113(.114.115.))116((.117.118.)119(.120.121.)122(.123.124.))125((.126.127.)128(.129.130.)131(.132.133.)))134(((.135.136.)137(.138.139.)140(.141.142.))143((.144.145.)146(.147.148.)149(.150.151.))152((.153.154.)155(.156.157.)158(.159.160.))))161((((.162.163.)164(.165.166.)167(.168.169.))170((.171.172.)173(.174.175.)176(.177.178.))179((.180.181.)182(.183.184.)185(.186.187.)))188(((.189.190.)191(.192.193.)194(.195.196.))197((.198.199.)200(.201.202.)203(.204.205.))206((.207.208.)209(.210.211.)212(.213.214.)))215(((.216.217.)218(.219.220.)221(.222.223.))224((.225.226.)227(.228.229.)230(.231.232.))233((.234.235.)236(.237.238.)239(.240.241.))242((.243.244.)245(.246.247.)248(.249.250.)251(.252.253.254.255.)))))";
	gint i;
	HgBTreeIter iter = hg_btree_iter_new();

	hg_btree_add(tree, GINT_TO_POINTER (0), GINT_TO_POINTER (0));
	hg_btree_add(tree, GINT_TO_POINTER (1), GINT_TO_POINTER (1));
	hg_btree_add(tree, GINT_TO_POINTER (2), GINT_TO_POINTER (2));
	hg_btree_add(tree, GINT_TO_POINTER (3), GINT_TO_POINTER (3));
	hg_btree_get_iter_first(tree, iter);
	if (iter->val != 0) {
		printf("failed to get an iterator 0\n");
		return 1;
	}
	hg_btree_get_iter_next(tree, iter);
	if (iter->val != GINT_TO_POINTER (1)) {
		printf("failed to get an iterator 1\n");
		return 1;
	}
	hg_btree_get_iter_next(tree, iter);
	if (iter->val != GINT_TO_POINTER (2)) {
		printf("failed to get an iterator 2\n");
		return 1;
	}
	hg_btree_get_iter_next(tree, iter);
	if (iter->val != GINT_TO_POINTER (3)) {
		printf("failed to get an iterator 3\n");
		return 1;
	}
	tree2string(tree->root, string);
	if (strcmp(string->str, test1)) {
		printf("error.\n  Expected: %s\n  Actual: %s\n", test1, string->str);
		return 1;
	}
	g_string_erase(string, 0, -1);
	hg_btree_add(tree, GINT_TO_POINTER (4), GINT_TO_POINTER (4));
	if (hg_btree_is_iter_valid(tree, iter)) {
		printf("failed to test if an iterator is valid.\n");
		return 1;
	}
	if (!hg_btree_update_iter(tree, iter)) {
		printf("failed to update an iterator\n");
		return 1;
	}
	hg_btree_get_iter_next(tree, iter);
	if (iter->val != GINT_TO_POINTER (4)) {
		printf("failed to get an iterator 4\n");
		return 1;
	}
	tree2string(tree->root, string);
	if (strcmp(string->str, test2)) {
		printf("error.\n  Expected: %s\n  Actual: %s\n", test2, string->str);
		return 1;
	}
	g_string_erase(string, 0, -1);
	for (i = 5; i < 256; i++) {
		hg_btree_add(tree, GINT_TO_POINTER (i), GINT_TO_POINTER (i));
	}
	tree2string(tree->root, string);
	if (strcmp(string->str, test)) {
		printf("error.\n  Expected: %s\n  Actual: %s\n", test, string->str);
		return 1;
	}
	if (hg_btree_length(tree) != 256) {
		printf("error.\n  Expected: 256\n  Actual: %u\n", hg_btree_length(tree));
		return 1;
	}
	hg_btree_iter_free(iter);
	g_string_free(string, TRUE);
	hg_mem_free(tree);
	hg_mem_pool_destroy(pool);
	hg_allocator_destroy(allocator);

	return 0;
}
