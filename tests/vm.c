#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgvaluenode.h>
#include <libretto/lbstack.h>
#include <libretto/vm.h>

int
main(void)
{
	LibrettoVM *vm;
	LibrettoStack *e, *o;
	HgValueNode *node;
	HgMemPool *pool;

	libretto_vm_init();

	vm = libretto_vm_new(LB_EMULATION_LEVEL_1);
	libretto_vm_startjob(vm, NULL, FALSE);

	pool = libretto_vm_get_current_pool(vm);
	e = libretto_vm_get_estack(vm);
	o = libretto_vm_get_ostack(vm);
	node = libretto_vm_get_name_node(vm, "add");
	hg_object_executable((HgObject *)node);
	libretto_stack_push(e, node);
	HG_VALUE_MAKE_INTEGER (pool, node, 2);
	libretto_stack_push(o, node);
	HG_VALUE_MAKE_INTEGER (pool, node, 1);
	libretto_stack_push(o, node);
	libretto_vm_main(vm);

	libretto_vm_finalize();

	return 0;
}
