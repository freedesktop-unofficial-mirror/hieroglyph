#include <fcntl.h>
#include <hieroglyph/hgfilter.h>

int
main(void)
{
	HgFilter *filter;
	int retval = 1;
	int fd;

	hieroglyph_init();

	filter = hieroglyph_filter_new("ps");
	fd = open("./tests/test.ps", O_RDONLY);
	if (hieroglyph_filter_open_stream(filter, fd, "test.ps")) {
		hieroglyph_filter_close_stream(filter);
	}
	if (filter != NULL) {
		hieroglyph_object_unref(HG_OBJECT (filter));
		retval = 0;
	}

	return retval;
}
