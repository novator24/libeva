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

#ifndef __EVA_BUFFER_H_
#define __EVA_BUFFER_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GskBuffer GskBuffer;
typedef struct _GskBufferFragment GskBufferFragment;

struct _GskBufferFragment
{
  GskBufferFragment    *next;
  char                 *buf;
  guint                 buf_max_size;	/* allocation size of buf */
  guint                 buf_start;	/* offset in buf of valid data */
  guint                 buf_length;	/* length of valid data in buf */
  
  gboolean              is_foreign;
  GDestroyNotify        destroy;
  gpointer              destroy_data;
};

struct _GskBuffer
{
  guint			size;

  GskBufferFragment    *first_frag;
  GskBufferFragment    *last_frag;
};

#define EVA_BUFFER_STATIC_INIT		{ 0, NULL, NULL }


void     eva_buffer_construct           (GskBuffer       *buffer);

guint    eva_buffer_read                (GskBuffer    *buffer,
                                         gpointer      data,
                                         guint         max_length);
guint    eva_buffer_peek                (const GskBuffer* buffer,
                                         gpointer      data,
                                         guint         max_length);
int      eva_buffer_discard             (GskBuffer    *buffer,
                                         guint         max_discard);
char    *eva_buffer_read_line           (GskBuffer    *buffer);

char    *eva_buffer_parse_string0       (GskBuffer    *buffer);
                        /* Returns first char of buffer, or -1. */
int      eva_buffer_peek_char           (const GskBuffer *buffer);
int      eva_buffer_read_char           (GskBuffer    *buffer);

/* 
 * Appending to the buffer.
 */
void     eva_buffer_append              (GskBuffer    *buffer, 
                                         gconstpointer data,
                                         guint         length);
void     eva_buffer_append_string       (GskBuffer    *buffer, 
                                         const char   *string);
void     eva_buffer_append_char         (GskBuffer    *buffer, 
                                         char          character);
void     eva_buffer_append_repeated_char(GskBuffer    *buffer, 
                                         char          character,
                                         gsize         count);
#define eva_buffer_append_zeros(buffer, count) \
  eva_buffer_append_repeated_char ((buffer), 0, (count))

/* XXX: eva_buffer_append_repeated_data() is UNIMPLEMENTED */
void     eva_buffer_append_repeated_data(GskBuffer    *buffer, 
                                         gconstpointer data_to_repeat,
                                         gsize         data_length,
                                         gsize         count);


void     eva_buffer_append_string0      (GskBuffer    *buffer,
                                         const char   *string);

void     eva_buffer_append_foreign      (GskBuffer    *buffer,
                                         gconstpointer data,
					 int           length,
					 GDestroyNotify destroy,
					 gpointer      destroy_data);

void     eva_buffer_printf              (GskBuffer    *buffer,
					 const char   *format,
					 ...) G_GNUC_PRINTF(2,3);
void     eva_buffer_vprintf             (GskBuffer    *buffer,
					 const char   *format,
					 va_list       args);

/* Take all the contents from src and append
 * them to dst, leaving src empty.
 */
guint    eva_buffer_drain               (GskBuffer    *dst,
                                         GskBuffer    *src);

/* Like `drain', but only transfers some of the data. */
guint    eva_buffer_transfer            (GskBuffer    *dst,
                                         GskBuffer    *src,
					 guint         max_transfer);

/* file-descriptor mucking */
int      eva_buffer_writev              (GskBuffer       *read_from,
                                         int              fd);
int      eva_buffer_writev_len          (GskBuffer       *read_from,
                                         int              fd,
					 guint            max_bytes);
int      eva_buffer_read_in_fd          (GskBuffer       *write_to,
                                         int              read_from);

/*
 * Scanning the buffer.
 */
int      eva_buffer_index_of            (GskBuffer    *buffer,
                                         char          char_to_find);
int      eva_buffer_str_index_of        (GskBuffer    *buffer,
                                         const char   *str_to_find);
int      eva_buffer_polystr_index_of    (GskBuffer    *buffer,
                                         char        **strings);

/* This deallocates memory used by the buffer-- you are responsible
 * for the allocation and deallocation of the GskBuffer itself. */
void     eva_buffer_destruct            (GskBuffer    *to_destroy);

/* Free all unused buffer fragments. */
void     eva_buffer_cleanup_recycling_bin ();


/* intended for use on the stack */
typedef struct _GskBufferIterator GskBufferIterator;
struct _GskBufferIterator
{
  GskBufferFragment *fragment;
  guint in_cur;
  guint cur_length;
  const guint8 *cur_data;
  guint offset;
};

#define eva_buffer_iterator_offset(iterator)	((iterator)->offset)
void     eva_buffer_iterator_construct (GskBufferIterator *iterator,
           			        GskBuffer         *to_iterate);
guint    eva_buffer_iterator_peek      (GskBufferIterator *iterator,
           			        gpointer           out,
           			        guint              max_length);
guint    eva_buffer_iterator_read      (GskBufferIterator *iterator,
           			        gpointer           out,
           			        guint              max_length);
guint    eva_buffer_iterator_skip      (GskBufferIterator *iterator,
           			        guint              max_length);
gboolean eva_buffer_iterator_find_char (GskBufferIterator *iterator,
					char               c);


G_END_DECLS

#endif
