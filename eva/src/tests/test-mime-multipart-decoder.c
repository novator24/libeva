
#include "../mime/evamimemultipartdecoder.h"
#include "../evamemory.h"
#include "../evamainloop.h"
#include "../evainit.h"
#include <string.h>

typedef struct _T2MPData T2MPData;
struct _T2MPData
{
  GPtrArray *ptr_array;
  gboolean ended;
};

static gboolean
handle_new_multipart_piece (EvaMimeMultipartDecoder *decoder,
			    gpointer data)
{
  T2MPData *tdata = data;
  EvaMimeMultipartPiece *piece = eva_mime_multipart_decoder_get_piece (decoder);
  if (piece)
    g_ptr_array_add (tdata->ptr_array, piece);
  return TRUE;
}

static gboolean
handle_multipart_shutdown (EvaMimeMultipartDecoder *decoder,
			   gpointer data)
{
  T2MPData *tdata = data;
  tdata->ended = TRUE;
  return FALSE;
}

GPtrArray *text_to_mime_pieces (const char *text, const char *bdy)
{
  const char *pairs[3];
  EvaMimeMultipartDecoder *decoder;
  EvaStream *feed;
  T2MPData data;
  pairs[0] = "boundary";
  pairs[1] = bdy;
  pairs[2] = NULL;
  decoder = eva_mime_multipart_decoder_new ((char**)pairs);
  g_assert (decoder != NULL);
  eva_mime_multipart_decoder_set_mode (decoder, EVA_MIME_MULTIPART_DECODER_MODE_ALWAYS_MEMORY);

  feed = eva_memory_source_static_string (text);
  eva_stream_attach (feed, EVA_STREAM (decoder), NULL);
  data.ptr_array = g_ptr_array_new ();
  data.ended = FALSE;
  eva_mime_multipart_decoder_trap (decoder, handle_new_multipart_piece,
				   handle_multipart_shutdown, &data,
				   NULL);
  while (!data.ended)
    {
      eva_main_loop_run (eva_main_loop_default (), -1, NULL);
    }
  g_object_unref (feed);
  g_object_unref (decoder);
  return data.ptr_array;
}

void free_mime_pieces (GPtrArray *array)
{
  guint i;
  for (i = 0; i < array->len; i++)
    eva_mime_multipart_piece_unref (array->pdata[i]);
  g_ptr_array_free (array, TRUE);
}

int main (int argc, char **argv)
{
  GPtrArray *array;
  EvaMimeMultipartPiece *piece;
  const char *tmp_txt;

  eva_init_without_threads (&argc, &argv);

  /* Example from RFC 2046, Section 5.1.1.  Page 21. */
  array = text_to_mime_pieces (
     "This is the preamble.  It is to be ignored, though it\r\n"
     "is a handy place for composition agents to include an\r\n"
     "explanatory note to non-MIME conformant readers.\r\n"
     "\r\n"
     "--simple boundary\r\n"
     "\r\n"
     "This is implicitly typed plain US-ASCII text.\r\n"
     "It does NOT end with a linebreak.\r\n"
     "--simple boundary\r\n"
     "Content-type: text/plain; charset=us-ascii\r\n"
     "\r\n"
     "This is explicitly typed plain US-ASCII text.\r\n"
     "It DOES end with a linebreak.\r\n"
     "\r\n"
     "--simple boundary--\r\n"
     , "simple boundary");
  g_assert (array->len == 2);

  tmp_txt = "This is implicitly typed plain US-ASCII text.\r\n"
            "It does NOT end with a linebreak.";
  piece = array->pdata[0];
  g_assert (piece != NULL);
  g_assert (piece->is_memory);
  g_assert (piece->content_length == strlen (tmp_txt));
  g_assert (memcmp (tmp_txt, piece->content_data, strlen (tmp_txt)) == 0);

  tmp_txt = "This is explicitly typed plain US-ASCII text.\r\n"
            "It DOES end with a linebreak.\r\n";
  piece = array->pdata[1];
  g_assert (piece != NULL);
  g_assert (piece->is_memory);
  g_assert (piece->content_length == strlen (tmp_txt));
  g_assert (memcmp (tmp_txt, piece->content_data, strlen (tmp_txt)) == 0);
  g_assert (strcmp (piece->type, "text") == 0);
  g_assert (strcmp (piece->subtype, "plain") == 0);

  free_mime_pieces (array);

  return 0;
}
