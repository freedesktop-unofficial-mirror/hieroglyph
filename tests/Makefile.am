NULL =

INCLUDES =						\
	-I$(top_srcdir)/hieroglyph			\
	$(HG_CFLAGS)					\
	$(NULL)
DEPS =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)
LDADDS =						\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)

noinst_PROGRAMS =					\
	test-hgbtree					\
	test-hgmem					\
	test-hgfile					\
	test-hgstring					\
	test-hgarray					\
	test-hgdict					\
	test-hgpage					\
	test-hglineedit					\
	test-snapshot					\
	test-scanner					\
	test-vm						\
	test-operatorencoding				\
	bench1-hgallocator-bfit				\
	bench2-hgallocator-bfit				\
	$(NULL)

test_hgbtree_SOURCES =					\
	hgbtree.c					\
	$(NULL)
test_hgbtree_LDADD = $(LDADDS)

test_hgmem_SOURCES =					\
	hgmem.c						\
	$(NULL)
test_hgmem_LDADD = $(LDADDS)

test_hgfile_SOURCES =					\
	hgfile.c					\
	$(NULL)
test_hgfile_LDADD = $(LDADDS)

test_hgstring_SOURCES =					\
	hgstring.c					\
	$(NULL)
test_hgstring_LDADD = $(LDADDS)

test_hgarray_SOURCES =					\
	hgarray.c					\
	$(NULL)
test_hgarray_LDADD = $(LDADDS)

test_hgdict_SOURCES =					\
	hgdict.c					\
	$(NULL)
test_hgdict_LDADD = $(LDADDS)

test_hgpage_SOURCES =					\
	hgpage.c					\
	$(NULL)
test_hgpage_LDADD = $(LDADDS)

test_hglineedit_SOURCES =				\
	hglineedit.c					\
	$(NULL)
test_hglineedit_LDADD = $(LDADDS)

test_snapshot_SOURCES =					\
	snapshot.c					\
	$(NULL)
test_snapshot_LDADD = $(LDADDS)

test_scanner_SOURCES =					\
	scanner.c					\
	$(NULL)
test_scanner_LDADD = 					\
	$(LDADDS)					\
	$(NULL)

test_vm_SOURCES =					\
	vm.c						\
	$(NULL)
test_vm_LDADD = 					\
	$(LDADDS)					\
	$(NULL)

test_operatorencoding_SOURCES =				\
	operatorencoding.c				\
	$(NULL)
test_operatorencoding_LDADD =				\
	$(LDADDS)					\
	$(NULL)

bench1_hgallocator_bfit_SOURCES =			\
	hgallocator-bfit-bench1.c			\
	$(NULL)
bench1_hgallocator_bfit_LDADD = $(LDADDS)

bench2_hgallocator_bfit_SOURCES =			\
	hgallocator-bfit-bench2.c			\
	$(NULL)
bench2_hgallocator_bfit_LDADD = $(LDADDS)

TESTS =							\
	test-hgbtree					\
	test-hgmem					\
	test-hgfile					\
	test-hgstring					\
	test-hgarray					\
	test-hgdict					\
	test-hgpage					\
	test-snapshot					\
	test-scanner					\
	test-vm						\
	test-operatorencoding				\
	$(NULL)
