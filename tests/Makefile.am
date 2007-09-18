NULL =
SUBDIRS = ps

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
	test-hgstack					\
	$(NULL)

test_hgobject_SOURCES =					\
	main.c						\
	hgobject.c					\
	$(NULL)
test_hgobject_LDADD = $(LDADDS)
test_hgstack_SOURCES =					\
	hgstack.c					\
	$(NULL)
test_hgstack_LDADD = $(LDADDS)

TESTS =							\
	test-hgobject					\
	test-hgstack					\
	$(NULL)
