NULL =
SUBDIRS =

INCLUDES =						\
	-I$(top_srcdir)/hieroglyph			\
	$(HG_CFLAGS)					\
	$(CHECK_CFLAGS)					\
	$(NULL)
DEPS =							\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(NULL)
LDADDS =						\
	$(top_builddir)/hieroglyph/libhieroglyph.la	\
	$(CHECK_LIBS)					\
	$(NULL)

noinst_PROGRAMS =					\
	test-hgobject					\
	test-hgarray					\
	test-hgdict					\
	test-hgfile					\
	test-hgstack					\
	test-hgstring					\
	$(NULL)

test_hgobject_SOURCES =					\
	main.c						\
	hgobject.c					\
	$(NULL)
test_hgobject_LDADD = $(LDADDS)
test_hgarray_SOURCES =					\
	main.c						\
	hgarray.c					\
	$(NULL)
test_hgarray_LDADD = $(LDADDS)
test_hgdict_SOURCES =					\
	main.c						\
	hgdict.c					\
	$(NULL)
test_hgdict_LDADD = $(LDADDS)
test_hgfile_SOURCES =					\
	main.c						\
	hgfile.c					\
	$(NULL)
test_hgfile_LDADD = $(LDADDS)
test_hgstack_SOURCES =					\
	hgstack.c					\
	$(NULL)
test_hgstack_LDADD = $(LDADDS)
test_hgstring_SOURCES =					\
	main.c						\
	hgstring.c					\
	$(NULL)
test_hgstring_LDADD = $(LDADDS)

TESTS =							\
	test-hgobject					\
	test-hgarray					\
	test-hgdict					\
	test-hgfile					\
	test-hgstack					\
	test-hgstring					\
	$(NULL)
