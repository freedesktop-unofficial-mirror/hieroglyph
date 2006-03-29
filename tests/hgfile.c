#include <hieroglyph/hgallocator-ffit.h>
#include <hieroglyph/hgmem.h>
#include <hieroglyph/hgfile.h>

int
main(void)
{
	HgAllocator *allocator;
	HgMemPool *pool;
	HgFileObject *file;
	gchar c;

	hg_mem_init();
	hg_file_init();

	allocator = hg_allocator_new();
	pool = hg_mem_pool_new(allocator, "test", 128, TRUE);
	if (pool == NULL) {
		hg_stderr_printf("Failed to create a memory pool.\n");
		return 1;
	}
	hg_stdout_printf("Hello\n");
	hg_stderr_printf("Hello world\n");
	file = hg_file_object_new(pool, HG_FILE_TYPE_FILE, HG_FILE_MODE_READ, "hgfile.c");
	c = hg_file_object_getc(file);
	if (c != '#') {
		hg_stderr_printf("Failed to get a char.\n");
		return 1;
	}
	hg_file_object_ungetc(file, ' ');
	c = hg_file_object_getc(file);
	if (c != ' ') {
		hg_stderr_printf("Failed to unget a char.\n");
		return 1;
	}
	c = hg_file_object_getc(file);
	if (c != 'i') {
		hg_stderr_printf("Failed to get a char [2].\n");
		return 1;
	}

	hg_file_finalize();
	hg_mem_finalize();

	return 0;
}
