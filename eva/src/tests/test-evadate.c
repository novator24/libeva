#include "../common/evadate.h"
#include <string.h>

int main()
{
  struct tm out;
  int tzoffset;

  g_assert (eva_date_parse ("Sun Nov  6 08:49:37 1994",
                            &out, &tzoffset,
			    EVA_DATE_FORMAT_ANSI_C));
  g_assert (out.tm_sec == 37);
  g_assert (out.tm_min == 49);
  g_assert (out.tm_hour == 8);
  g_assert (out.tm_mon == 10);
  g_assert (out.tm_mday == 6);
  g_assert (out.tm_wday == 0);
  g_assert (out.tm_year == 94);

  memset (&out, 0, sizeof (out));
  g_assert (eva_date_parse ("Sunday, 06-Nov-94 08:49:37 GMT",
                            &out, &tzoffset,
			    EVA_DATE_FORMAT_1036));
  g_assert (out.tm_sec == 37);
  g_assert (out.tm_min == 49);
  g_assert (out.tm_hour == 8);
  g_assert (out.tm_mon == 10);
  g_assert (out.tm_mday == 6);
  g_assert (out.tm_wday == 0);
  g_assert (out.tm_year == 94);

  memset (&out, 0, sizeof (out));
  g_assert (eva_date_parse ("Sun, 06 Nov 1994 08:49:37 GMT",
                            &out, &tzoffset,
			    EVA_DATE_FORMAT_1123));
  g_assert (out.tm_sec == 37);
  g_assert (out.tm_min == 49);
  g_assert (out.tm_hour == 8);
  g_assert (out.tm_mon == 10);
  g_assert (out.tm_mday == 6);
  g_assert (out.tm_wday == 0);
  g_assert (out.tm_year == 94);


  g_assert (! eva_date_parse ("Sun, 06 Nov 1994 08:49:37 GMT",
                              &out, &tzoffset,
			      EVA_DATE_FORMAT_1036));
  g_assert (! eva_date_parse ("Sun, 06 Nov 1994 08:49:37 GMT",
                              &out, &tzoffset,
			      EVA_DATE_FORMAT_ANSI_C));

  return 0;
}
