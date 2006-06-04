#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgfile.h>
#include <hieroglyph/hgstring.h>
#include <hieroglyph/hgvaluenode.h>
#include <libretto/lbstack.h>
#include <libretto/scanner.h>
#include <libretto/vm.h>

void
print_stack(LibrettoStack *stack)
{
	HgValueNode *node;
	guint i, depth;

	depth = libretto_stack_depth(stack);
	for (i = 0; i < depth; i++) {
		node = libretto_stack_index(stack, depth - i - 1);
		switch (node->type) {
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

int
main(void)
{
	HG_MEM_INIT;

	LibrettoVM *vm;
	LibrettoStack *ostack, *estack;
//	gchar *tokens = "true false moveto /foo 10050 10a 10.5 -1 .5 -1e10 10.0E5 1E 5.2e-2 36#Z 1#0 %foobar\nfoo(test)((test test) test)(foo\nbar)";
	HgValueNode *node;
	HgFileObject *file;
	HgMemPool *pool;

	libretto_vm_init();

	vm = libretto_vm_new(LB_EMULATION_LEVEL_1);
	libretto_vm_startjob(vm, NULL, FALSE);
	pool = libretto_vm_get_current_pool(vm);
	ostack = libretto_vm_get_ostack(vm);
	estack = libretto_vm_get_estack(vm);
	file = hg_file_object_new(pool, HG_FILE_TYPE_BUFFER, HG_FILE_MODE_READ, "*buffer*", "/foo true false null [moveto (test(test test) test) 10050 10a 10.5 -1 .5 -1e10 10.0E5 1E 5.2e-2 36#Z 1#0 //true 10e2.5 1.0ee10 1..5", -1);
	while (!hg_file_object_is_eof(file)) {
		node = libretto_scanner_get_object(vm, file);
		if (node != NULL) {
			libretto_stack_push(ostack, node);
		}
	}
	print_stack(ostack);

	libretto_vm_finalize();

	return 0;
}
