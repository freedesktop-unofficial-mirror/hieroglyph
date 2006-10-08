#include <stdlib.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgstack.h>
#include <hieroglyph/hgstring.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/scanner.h>
#include <hieroglyph/vm.h>

void
print_stack(HgStack *stack)
{
	HgValueNode *node;
	guint i, depth;

	depth = hg_stack_depth(stack);
	for (i = 0; i < depth; i++) {
		node = hg_stack_index(stack, depth - i - 1);
		switch (HG_VALUE_GET_VALUE_TYPE (node)) {
		    case HG_TYPE_VALUE_BOOLEAN:
			    g_print("  bool:%s\n", (HG_VALUE_GET_BOOLEAN (node) ? "true" : "false"));
			    break;
		    case HG_TYPE_VALUE_NAME:
			    if (hg_object_is_executable((HgObject *)node))
				    g_print("  name:%s\n", HG_VALUE_GET_NAME (node));
			    else
				    g_print("  name:/%s\n", HG_VALUE_GET_NAME (node));
			    break;
		    case HG_TYPE_VALUE_NULL:
			    g_print("  null:null\n");
			    break;
		    case HG_TYPE_VALUE_MARK:
			    g_print("  --mark--\n");
			    break;
		    case HG_TYPE_VALUE_STRING:
			    g_print("  string:(%s)\n", hg_string_get_string(HG_VALUE_GET_STRING (node)));
			    break;
		    case HG_TYPE_VALUE_INTEGER:
			    g_print("  int:%d\n", HG_VALUE_GET_INTEGER (node));
			    break;
		    case HG_TYPE_VALUE_REAL:
			    g_print("  real:%f\n", HG_VALUE_GET_REAL (node));
			    break;
		    default:
			    g_print("pooooo\n");
			    break;
		}
	}
}

gboolean
test(HgVM        *vm,
     HgMemPool   *pool,
     const gchar *expression,
     HgValueNode *expected_node)
{
	HgFileObject *file = hg_file_object_new(pool, HG_FILE_TYPE_BUFFER,
						HG_FILE_MODE_READ, "*buffer*",
						expression, -1);
	HgValueNode *node;
	HgString *expected_string, *actual_string;
	gboolean retval = TRUE;

	expected_string = hg_object_to_string((HgObject *)expected_node);
	node = hg_scanner_get_object(vm, file);
	if (node != NULL) {
		if (expected_node != NULL &&
		    !hg_value_node_compare_content(expected_node, node)) {
			actual_string = hg_object_to_string((HgObject *)node);
			g_print("Expression: %s, Expected result is %s [%s] but Actual result was %s [%s]\n",
				expression, hg_string_get_string(expected_string),
				hg_value_node_get_type_name(HG_VALUE_GET_VALUE_TYPE (expected_node)),
				hg_string_get_string(actual_string),
				hg_value_node_get_type_name(HG_VALUE_GET_VALUE_TYPE (node)));
			retval = FALSE;
		} else if (expected_node == NULL) {
			actual_string = hg_object_to_string((HgObject *)node);
			g_print("Expression: %s, Expected result is NULL but Actual result was %s [%s]\n",
				expression,
				hg_string_get_string(actual_string),
				hg_value_node_get_type_name(HG_VALUE_GET_VALUE_TYPE (node)));
			retval = FALSE;
		}
	} else {
		if (expected_node != NULL) {
			g_print("Expression: %s, Expected result is %s [%s] but failed to parse it.\n",
				expression, hg_string_get_string(expected_string),
				hg_value_node_get_type_name(HG_VALUE_GET_VALUE_TYPE (expected_node)));
			retval = FALSE;
		}
	}
	if (hg_object_is_executable((HgObject *)expected_node) &&
	    !hg_object_is_executable((HgObject *)node)) {
		g_print("Expression: %s, Expected an executable, but it was not.\n",
			expression);
		retval = FALSE;
	} else if (!hg_object_is_executable((HgObject *)expected_node) &&
		   hg_object_is_executable((HgObject *)node)) {
		g_print("Expression: %s, Expected a non-executable, but it was not.\n",
			expression);
		retval = FALSE;
	}

	return retval;
}

int
main(void)
{
	HG_MEM_INIT;

	HgVM *vm;
	HgMemPool *pool;
	HgFileObject *file;
	HgValueNode *node;
	HgStack *ostack;

	hg_vm_init();

	vm = hg_vm_new(VM_EMULATION_LEVEL_1);
	hg_vm_startjob(vm, NULL, FALSE);
	pool = hg_vm_get_current_pool(vm);
	ostack = hg_vm_get_ostack(vm);

#define do_test_null(_vm_, _pool_, _exp_)				\
	G_STMT_START {							\
		if (!test((_vm_), (_pool_), (_exp_), NULL)) {		\
			exit(1);					\
		}							\
	} G_STMT_END
#define do_test(_vm_, _pool_, _exp_, _type_, _val_)			\
	G_STMT_START {							\
		HgValueNode *__node__;					\
		HG_VALUE_MAKE_ ## _type_ ((_pool_), __node__, (_val_));	\
		if (!test((_vm_), (_pool_), (_exp_), __node__)) {	\
			exit(1);					\
		}							\
	} G_STMT_END
#define do_test_name(_vm_, _pool_, _exp_, _val_, _exec_)		\
	G_STMT_START {							\
		HgValueNode *__node__;					\
		HG_VALUE_MAKE_NAME_STATIC ((_pool_), __node__, (_val_)); \
		if ((_exec_))						\
			hg_object_executable((HgObject *)__node__);	\
		if (!test((_vm_), (_pool_), (_exp_), __node__)) {	\
			exit(1);					\
		}							\
	} G_STMT_END

	do_test_name(vm, pool, "/foo", "foo", FALSE);
	do_test_name(vm, pool, "true", "true", TRUE);
	do_test_name(vm, pool, "false", "false", TRUE);
	do_test(vm, pool, "//true", BOOLEAN, TRUE);
	do_test(vm, pool, "//false", BOOLEAN, FALSE);
	do_test_name(vm, pool, "null", "null", TRUE);
	do_test(vm, pool, "//null", NULL, NULL);
	do_test(vm, pool, "10050", INTEGER, 10050);
	do_test(vm, pool, "2147483647", INTEGER, 2147483647);
	do_test(vm, pool, "-2147483648", INTEGER, 0x80000000);
	do_test_name(vm, pool, "10a", "10a", TRUE);
	do_test(vm, pool, "10.5", REAL, 10.5);
	do_test(vm, pool, "-1", INTEGER, -1);
	do_test(vm, pool, ".5", REAL, 0.5);
	do_test(vm, pool, "-1e10", REAL, -1e+10);
	do_test(vm, pool, "10.0E5", REAL, 10.0e5);
	do_test_name(vm, pool, "1E", "1E", TRUE);
	do_test(vm, pool, "5.2e-2", REAL, 5.2e-2);
	do_test(vm, pool, "36#Z", INTEGER, 35);
	do_test(vm, pool, "1#0", INTEGER, 0);
	do_test_name(vm, pool, "=", "=", TRUE);
	do_test_null(vm, pool, " ");
	do_test_null(vm, pool, "\n");

	file = hg_file_object_new(pool, HG_FILE_TYPE_BUFFER, HG_FILE_MODE_READ, "*buffer*", "/foo true false null [moveto (test(test test) test) 10050 10a 10.5 -1 .5 -1e10 10.0E5 1E 5.2e-2 36#Z 1#0 //true 10e2.5 1.0ee10 1..5", -1);
	while (!hg_file_object_is_eof(file)) {
		node = hg_scanner_get_object(vm, file);
		if (node != NULL) {
			hg_stack_push(ostack, node);
		}
	}
	print_stack(ostack);

	hg_vm_finalize();

	return 0;
}
