#include <hieroglyph/hglineedit.h>

int
main(void)
{
	gchar *line;

	while (1) {
		line = hg_line_edit_get_line((gpointer)1, NULL, FALSE);
		g_print("->%s\n", line);
		g_free(line);
		line = hg_line_edit_get_statement((gpointer)1, NULL);
		g_print("=>%s\n", line);
		g_free(line);
	}

	return 0;
}
