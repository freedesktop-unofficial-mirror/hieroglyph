#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgstack.h>
#include <hieroglyph/hgvaluenode.h>
#include <hieroglyph/vm.h>

int
main(void)
{
	HG_MEM_INIT;

	HgVM *vm;
	HgStack *e, *o;
	HgValueNode *node;
	HgMemPool *pool;

	hg_vm_init();

	vm = hg_vm_new(VM_EMULATION_LEVEL_1);
	hg_vm_startjob(vm, NULL, FALSE);

	pool = hg_vm_get_current_pool(vm);
	e = hg_vm_get_estack(vm);
	o = hg_vm_get_ostack(vm);
	node = hg_vm_get_name_node(vm, "add");
	hg_object_executable((HgObject *)node);
	hg_stack_push(e, node);
	HG_VALUE_MAKE_INTEGER (pool, node, 2);
	hg_stack_push(o, node);
	HG_VALUE_MAKE_INTEGER (pool, node, 1);
	hg_stack_push(o, node);
	hg_vm_main(vm);

	hg_vm_finalize();

	return 0;
}
