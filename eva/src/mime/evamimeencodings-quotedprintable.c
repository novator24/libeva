/* See RFC 2045, Section 6.7 */
#include "evamimeencodings.h"
#include "../evasimplefilter.h"
#include "../evamacros.h"
#include <string.h>
#include <ctype.h>


/* --- common --- */
static GObjectClass *parent_class = NULL;

/* --- typedefs --- */
typedef struct _EvaMimeQuotedPrintableDecoder EvaMimeQuotedPrintableDecoder;
typedef struct _EvaMimeQuotedPrintableDecoderClass EvaMimeQuotedPrintableDecoderClass;
/* --- type macros --- */
GType eva_mime_quoted_printable_decoder_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MIME_QUOTED_PRINTABLE_DECODER			(eva_mime_quoted_printable_decoder_get_type ())
#define EVA_MIME_QUOTED_PRINTABLE_DECODER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MIME_QUOTED_PRINTABLE_DECODER, EvaMimeQuotedPrintableDecoder))
#define EVA_MIME_QUOTED_PRINTABLE_DECODER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MIME_QUOTED_PRINTABLE_DECODER, EvaMimeQuotedPrintableDecoderClass))
#define EVA_MIME_QUOTED_PRINTABLE_DECODER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MIME_QUOTED_PRINTABLE_DECODER, EvaMimeQuotedPrintableDecoderClass))
#define EVA_IS_MIME_QUOTED_PRINTABLE_DECODER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MIME_QUOTED_PRINTABLE_DECODER))
#define EVA_IS_MIME_QUOTED_PRINTABLE_DECODER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MIME_QUOTED_PRINTABLE_DECODER))

/* --- structures --- */
struct _EvaMimeQuotedPrintableDecoderClass 
{
  EvaSimpleFilterClass simple_filter_class;
};
struct _EvaMimeQuotedPrintableDecoder 
{
  EvaSimpleFilter      simple_filter;
};

/* --- EvaSimpleFilter methods --- */

static gboolean
quoteprintable_char_to_hexval (char c, guint8 *val_out, GError **error)
{
  if ('0' <= c && c <= '9')
    *val_out = c - '0';
  else if ('A' <= c && c <= 'F')
    *val_out = c - 'A' + 10;
  else
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN, EVA_ERROR_BAD_FORMAT,
		   _("quoted-printable: error parsing hex value '%c'"), c);
      return FALSE;
    }
  return TRUE;
}
static gboolean
eva_mime_quoted_printable_decoder_process (EvaSimpleFilter *filter,
                                           EvaBuffer       *dst,
                                           EvaBuffer       *src,
                                           GError         **error)
{
  for (;;)
    {
      char buf[3];
      guint p = eva_buffer_peek (src, buf, 3);
      guint ne = 0;
      guint8 hexvalues[2];
      while (ne < p)
	{
	  if (buf[ne] == '=')
	    break;
	  ne++;
	}
      if (ne > 0)
	{
	  eva_buffer_append (dst, buf, ne);
	  eva_buffer_discard (src, ne);
	}
      else if (p <= 1)
	{
	  break;
	}
      else if (p == 2 && buf[1] == '\n')
	{
	  eva_buffer_discard (src, 2);
	}
      else
	{
	  if (buf[1] == '\r' && buf[2] == '\n')
	    eva_buffer_discard (src, 3);
	  else if (buf[1] == '\n')
	    eva_buffer_discard (src, 2);
	  else
	    {
	      if (!quoteprintable_char_to_hexval (buf[1], &hexvalues[0], error)
	       || !quoteprintable_char_to_hexval (buf[2], &hexvalues[1], error))
		return FALSE;
	      eva_buffer_append_char (dst, (hexvalues[0] << 4) | hexvalues[1]);
	      eva_buffer_discard (src, 3);
	    }
	}
    }
  return TRUE;
}

static gboolean
eva_mime_quoted_printable_decoder_flush (EvaSimpleFilter *filter,
                                         EvaBuffer       *dst,
                                         EvaBuffer       *src,
                                         GError         **error)
{
  if (src->size > 0)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN, EVA_ERROR_BAD_FORMAT,
		   _("unprocessable junk in quoted-printable (%u bytes)"),
		   src->size);
      return FALSE;
    }
  return TRUE;
}

/* --- functions --- */
static void
eva_mime_quoted_printable_decoder_init (EvaMimeQuotedPrintableDecoder *mime_quoted_printable_decoder)
{
}

static void
eva_mime_quoted_printable_decoder_class_init (EvaMimeQuotedPrintableDecoderClass *class)
{
  EvaSimpleFilterClass *simple_filter_class = EVA_SIMPLE_FILTER_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  simple_filter_class->process = eva_mime_quoted_printable_decoder_process;
  simple_filter_class->flush = eva_mime_quoted_printable_decoder_flush;
}

GType eva_mime_quoted_printable_decoder_get_type()
{
  static GType mime_quoted_printable_decoder_type = 0;
  if (!mime_quoted_printable_decoder_type)
    {
      static const GTypeInfo mime_quoted_printable_decoder_info =
      {
	sizeof(EvaMimeQuotedPrintableDecoderClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_mime_quoted_printable_decoder_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMimeQuotedPrintableDecoder),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_mime_quoted_printable_decoder_init,
	NULL		/* value_table */
      };
      mime_quoted_printable_decoder_type = g_type_register_static (EVA_TYPE_SIMPLE_FILTER,
                                                  "EvaMimeQuotedPrintableDecoder",
						  &mime_quoted_printable_decoder_info, 0);
    }
  return mime_quoted_printable_decoder_type;
}

/**
 * eva_mime_quoted_printable_decoder_new:
 *
 * Allocate a new MIME encoder which
 * takes quoted-printable encoded data in and gives
 * raw binary data out.
 * (See RFC 2045, Section 6.7).
 *
 * returns: the newly allocated decoder.
 */
EvaStream *
eva_mime_quoted_printable_decoder_new (void)
{
  return g_object_new (EVA_TYPE_MIME_QUOTED_PRINTABLE_DECODER, NULL);
}

/* ================================= Encoder ================================ */
/* --- typedefs --- */
typedef struct _EvaMimeQuotedPrintableEncoder EvaMimeQuotedPrintableEncoder;
typedef struct _EvaMimeQuotedPrintableEncoderClass EvaMimeQuotedPrintableEncoderClass;
/* --- type macros --- */
GType eva_mime_quoted_printable_encoder_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MIME_QUOTED_PRINTABLE_ENCODER			(eva_mime_quoted_printable_encoder_get_type ())
#define EVA_MIME_QUOTED_PRINTABLE_ENCODER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MIME_QUOTED_PRINTABLE_ENCODER, EvaMimeQuotedPrintableEncoder))
#define EVA_MIME_QUOTED_PRINTABLE_ENCODER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MIME_QUOTED_PRINTABLE_ENCODER, EvaMimeQuotedPrintableEncoderClass))
#define EVA_MIME_QUOTED_PRINTABLE_ENCODER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MIME_QUOTED_PRINTABLE_ENCODER, EvaMimeQuotedPrintableEncoderClass))
#define EVA_IS_MIME_QUOTED_PRINTABLE_ENCODER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MIME_QUOTED_PRINTABLE_ENCODER))
#define EVA_IS_MIME_QUOTED_PRINTABLE_ENCODER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MIME_QUOTED_PRINTABLE_ENCODER))

/* --- structures --- */
struct _EvaMimeQuotedPrintableEncoderClass 
{
  EvaSimpleFilterClass simple_filter_class;
};
struct _EvaMimeQuotedPrintableEncoder 
{
  EvaSimpleFilter      simple_filter;
  guint n_chars_in_line;
};
/* --- prototypes --- */

#define IS_ENCODABLE_AS_ITSELF(c)	\
	((33 <= (c) && (c) <= 60) || (62 <= (c) && (c) <= 126))

/* --- EvaSimpleFilter methods --- */
static gboolean
eva_mime_quoted_printable_encoder_process (EvaSimpleFilter *filter,
                                           EvaBuffer       *dst,
                                           EvaBuffer       *src,
                                           GError         **error)
{
  EvaMimeQuotedPrintableEncoder *encoder = EVA_MIME_QUOTED_PRINTABLE_ENCODER (filter);
  guint n_chars_in_line = encoder->n_chars_in_line;
  guint n_peeked;
  char buf[256];
  while ((n_peeked = eva_buffer_peek (src, buf, sizeof(buf))) > 0)
    {
      char *at = buf;
      guint rem = n_peeked;
      while (rem)
	{
	  if (n_chars_in_line > 68)
	    {
	      eva_buffer_append (dst, "=\r\n", 3);
	      n_chars_in_line = 0;
	    }
	  if (IS_ENCODABLE_AS_ITSELF (*at))
	    {
	      eva_buffer_append_char (dst, *at);
	      rem--;
	      at++;
	      n_chars_in_line++;
	      continue;
	    }
	  else if (*at == '\r')
	    {
	      if (rem >= 2)
		{
		  if (at[1] == '\n')
		    {
		      eva_buffer_append (dst, at, 2);
		      rem -= 2;
		      at += 2;
		      n_chars_in_line = 0;
		      continue;
		    }
		}
	      else
		break;
	    }

	  /* encode all other chars as hex */
	  {
	    char tmp[4];
	    g_snprintf (tmp, 4, "=%02X", (guint8) *at);
	    eva_buffer_append (dst, tmp, 3);
	    rem--;
	    at++;
	    n_chars_in_line += 3;
	  }
	}
      eva_buffer_discard (src, n_peeked - rem);
      if (n_peeked < sizeof (buf))
	break;
    }
  encoder->n_chars_in_line = n_chars_in_line;
  return TRUE;
}

static gboolean
eva_mime_quoted_printable_encoder_flush (EvaSimpleFilter *filter,
                                         EvaBuffer       *dst,
                                         EvaBuffer       *src,
                                         GError         **error)
{
  EvaMimeQuotedPrintableEncoder *encoder = EVA_MIME_QUOTED_PRINTABLE_ENCODER (filter);
  g_return_val_if_fail (src->size <= 1, FALSE);
  if (src->size == 1)
    {
      eva_buffer_printf (dst, "=%02X", eva_buffer_read_char (src));
      encoder->n_chars_in_line += 3;
    }
  if (encoder->n_chars_in_line > 0)
    eva_buffer_append (dst, "=\r\n", 3);
  return TRUE;
}

/* --- functions --- */
static void
eva_mime_quoted_printable_encoder_init (EvaMimeQuotedPrintableEncoder *mime_quoted_printable_encoder)
{
}
static void
eva_mime_quoted_printable_encoder_class_init (EvaMimeQuotedPrintableEncoderClass *class)
{
  EvaSimpleFilterClass *simple_filter_class = EVA_SIMPLE_FILTER_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  simple_filter_class->process = eva_mime_quoted_printable_encoder_process;
  simple_filter_class->flush = eva_mime_quoted_printable_encoder_flush;
}

GType eva_mime_quoted_printable_encoder_get_type()
{
  static GType mime_quoted_printable_encoder_type = 0;
  if (!mime_quoted_printable_encoder_type)
    {
      static const GTypeInfo mime_quoted_printable_encoder_info =
      {
	sizeof(EvaMimeQuotedPrintableEncoderClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_mime_quoted_printable_encoder_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMimeQuotedPrintableEncoder),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_mime_quoted_printable_encoder_init,
	NULL		/* value_table */
      };
      mime_quoted_printable_encoder_type = g_type_register_static (EVA_TYPE_SIMPLE_FILTER,
                                                  "EvaMimeQuotedPrintableEncoder",
						  &mime_quoted_printable_encoder_info, 0);
    }
  return mime_quoted_printable_encoder_type;
}

/**
 * eva_mime_quoted_printable_encoder_new:
 *
 * Allocate a new MIME encoder which
 * takes raw binary data in and gives
 * quoted-printable encoded data out.
 * (See RFC 2045, Section 6.7).
 *
 * returns: the newly allocated encoder.
 */
EvaStream *
eva_mime_quoted_printable_encoder_new (void)
{
  return g_object_new (EVA_TYPE_MIME_QUOTED_PRINTABLE_ENCODER, NULL);
}
