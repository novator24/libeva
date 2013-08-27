/*
    EVA - a library to write servers
    Copyright (C) 1999-2000 Dave Benson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    Contact:
        daveb@ffem.org <Dave Benson>
*/

#include "../evabuffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void random_slice(GskBuffer* buf)
{
  GskBuffer tmpbuf;
  char *copy, *copy_at;
  guint orig_size = buf->size;
  eva_buffer_construct (&tmpbuf);
  copy = g_new (char, buf->size);
  copy_at = copy;
  while (1)
    {
      int r;
      r = (rand () % 16384) + 1;
      r = eva_buffer_read (buf, copy_at, r);
      eva_buffer_append (&tmpbuf, copy_at, r);
      if (r == 0)
	break;
    }
  g_assert (copy_at == copy + orig_size);
  g_assert (buf->size == 0);
  g_assert (tmpbuf.size == orig_size);
  copy_at = g_new (char, orig_size);
  g_assert (eva_buffer_read (&tmpbuf, copy_at, orig_size)
	    == orig_size);
  g_assert (eva_buffer_read (&tmpbuf, copy_at, orig_size) == 0);
  g_assert (memcmp (copy, copy_at, orig_size) == 0);
  g_free (copy);
  g_free (copy_at);
}

void count(GskBuffer* buf, int start, int end)
{
  char b[1024];
  while (start <= end)
    {
      sprintf (b, "%d\n", start++);
      eva_buffer_append (buf, b, strlen (b));
    }
}

void decount(GskBuffer* buf, int start, int end)
{
  char b[1024];
  while (start <= end)
    {
      char *rv;
      sprintf (b, "%d", start++);
      rv = eva_buffer_read_line (buf);
      g_assert (rv != NULL);
      g_assert (strcmp (b, rv) == 0);
      g_free (rv);
    }
}

int main(int argc, char** argv)
{

  GskBuffer evabuffer;
  char buf[1024];
  char *str;

  eva_buffer_construct (&evabuffer);
  g_assert (evabuffer.size == 0);
  eva_buffer_append (&evabuffer, "hello", 5);
  g_assert (evabuffer.size == 5);
  g_assert (eva_buffer_read (&evabuffer, buf, sizeof (buf)) == 5);
  g_assert (memcmp (buf, "hello", 5) == 0);
  g_assert (evabuffer.size == 0);
  eva_buffer_destruct (&evabuffer);

  eva_buffer_construct (&evabuffer);
  count (&evabuffer, 1, 100000);
  decount (&evabuffer, 1, 100000);
  g_assert (evabuffer.size == 0);
  eva_buffer_destruct (&evabuffer);

  eva_buffer_construct (&evabuffer);
  eva_buffer_append_string (&evabuffer, "hello\na\nb");
  str = eva_buffer_read_line (&evabuffer);
  g_assert (str);
  g_assert (strcmp (str, "hello") == 0);
  g_free (str);
  str = eva_buffer_read_line (&evabuffer);
  g_assert (str);
  g_assert (strcmp (str, "a") == 0);
  g_free (str);
  g_assert (evabuffer.size == 1);
  g_assert (eva_buffer_read_line (&evabuffer) == NULL);
  eva_buffer_append_char (&evabuffer, '\n');
  str = eva_buffer_read_line (&evabuffer);
  g_assert (str);
  g_assert (strcmp (str, "b") == 0);
  g_free (str);
  g_assert (evabuffer.size == 0);
  eva_buffer_destruct (&evabuffer);

  eva_buffer_construct (&evabuffer);
  eva_buffer_append (&evabuffer, "hello", 5);
  eva_buffer_append_foreign (&evabuffer, "test", 4, NULL, NULL);
  eva_buffer_append (&evabuffer, "hello", 5);
  g_assert (evabuffer.size == 14);
  g_assert (eva_buffer_read (&evabuffer, buf, sizeof (buf)) == 14);
  g_assert (memcmp (buf, "hellotesthello", 14) == 0);
  g_assert (evabuffer.size == 0);

  /* Test that the foreign data really is not being stored in the GskBuffer */
  {
    char test_str[5];
    strcpy (test_str, "test");
    eva_buffer_construct (&evabuffer);
    eva_buffer_append (&evabuffer, "hello", 5);
    eva_buffer_append_foreign (&evabuffer, test_str, 4, NULL, NULL);
    eva_buffer_append (&evabuffer, "hello", 5);
    g_assert (evabuffer.size == 14);
    g_assert (eva_buffer_peek (&evabuffer, buf, sizeof (buf)) == 14);
    g_assert (memcmp (buf, "hellotesthello", 14) == 0);
    test_str[1] = '3';
    g_assert (evabuffer.size == 14);
    g_assert (eva_buffer_read (&evabuffer, buf, sizeof (buf)) == 14);
    g_assert (memcmp (buf, "hellot3sthello", 14) == 0);
    eva_buffer_destruct (&evabuffer);
  }

  /* Test str_index_of */
  {
    GskBuffer buffer = EVA_BUFFER_STATIC_INIT;
    eva_buffer_append_foreign (&buffer, "abc", 3, NULL, NULL);
    eva_buffer_append_foreign (&buffer, "def", 3, NULL, NULL);
    eva_buffer_append_foreign (&buffer, "gad", 3, NULL, NULL);
    g_assert (eva_buffer_str_index_of (&buffer, "cdefg") == 2);
    g_assert (eva_buffer_str_index_of (&buffer, "ad") == 7);
    g_assert (eva_buffer_str_index_of (&buffer, "ab") == 0);
    g_assert (eva_buffer_str_index_of (&buffer, "a") == 0);
    g_assert (eva_buffer_str_index_of (&buffer, "g") == 6);
    eva_buffer_destruct (&buffer);
  }

  return 0;
}
