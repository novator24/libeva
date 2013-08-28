#include "evastdio.h"
#include <string.h>

char *
eva_stdio_readline (FILE *fp)
{
  char buf[1025];
  GString *str = NULL;
  while (fgets(buf,sizeof(buf),fp))
    {
      if (str == NULL)
        str = g_string_new ("");
      g_string_append (str, buf);
      if (strchr (buf, '\n'))
        {
          g_string_set_size (str, str->len - 1);
          break;
        }
    }
  return str ? g_string_free (str, FALSE) : NULL;
}

