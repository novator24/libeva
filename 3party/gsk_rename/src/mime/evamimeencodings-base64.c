#include "evamimeencodings.h"
#include "../evasimplefilter.h"
#include "../evamacros.h"
#include <string.h>
#include <ctype.h>

/* --- common to base64 encoder and decoder --- */
static GObjectClass *parent_class = NULL;

#define            base64_terminal_char	'='
static const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "abcdefghijklmnopqrstuvwxyz"
			          "0123456789"
			          "+/";
static guint8      ascii_to_base64[256];

#define BASE64_VALUE_WHITESPACE		255
#define BASE64_VALUE_TERMINAL		254
#define BASE64_VALUE_BAD_CHAR		253

/* ==================== EvaMimeBase64Decoder =========================== */
/* --- typedefs --- */
typedef struct _EvaMimeBase64Decoder EvaMimeBase64Decoder;
typedef struct _EvaMimeBase64DecoderClass EvaMimeBase64DecoderClass;
/* --- type macros --- */
GType eva_mime_base64_decoder_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MIME_BASE64_DECODER			(eva_mime_base64_decoder_get_type ())
#define EVA_MIME_BASE64_DECODER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MIME_BASE64_DECODER, EvaMimeBase64Decoder))
#define EVA_MIME_BASE64_DECODER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MIME_BASE64_DECODER, EvaMimeBase64DecoderClass))
#define EVA_MIME_BASE64_DECODER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MIME_BASE64_DECODER, EvaMimeBase64DecoderClass))
#define EVA_IS_MIME_BASE64_DECODER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MIME_BASE64_DECODER))
#define EVA_IS_MIME_BASE64_DECODER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MIME_BASE64_DECODER))

/* --- structures --- */
struct _EvaMimeBase64DecoderClass 
{
  EvaSimpleFilterClass simple_filter_class;
};
struct _EvaMimeBase64Decoder 
{
  EvaSimpleFilter      simple_filter;

  guint8 cur_bits_in_byte;
  guint8 byte;
  guint8 got_terminal : 1;
};

/* --- EvaSimpleFilter methods --- */
static inline gboolean
decoder_process_one (EvaMimeBase64Decoder *decoder,
                     EvaBuffer            *dst,
		     int                   input,
		     GError              **error)
{
  guint cur_bits = decoder->cur_bits_in_byte;
  guint8 cur_value = decoder->byte;
  guint8 b64 = ascii_to_base64[input];
  if (b64 == BASE64_VALUE_WHITESPACE)
    return TRUE;
  if (b64 == BASE64_VALUE_TERMINAL)
    {
      decoder->got_terminal = 1;
      return TRUE;
    }
  if (b64 == BASE64_VALUE_BAD_CHAR)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   EVA_ERROR_BAD_FORMAT,
		   _("did not expect character 0x%02x in base64 stream"),
		   input);
      return FALSE;
    }
  switch (cur_bits)
    {
    case 0:
      cur_value |= (b64 << 2);
      cur_bits = 6;
      break;
    case 2:
      eva_buffer_append_char (dst, cur_value | b64);
      cur_bits = 0;
      cur_value = 0;
      break;
    case 4:
      eva_buffer_append_char (dst, cur_value | (b64 >> 2));
      cur_bits = 2;
      cur_value = b64 << 6;
      break;
    case 6:
      eva_buffer_append_char (dst, cur_value | (b64 >> 4));
      cur_bits = 4;
      cur_value = b64 << 4;
      break;
    }
  decoder->cur_bits_in_byte = cur_bits;
  decoder->byte = cur_value;
  return TRUE;
}


static gboolean
eva_mime_base64_decoder_process (EvaSimpleFilter *filter,
                                 EvaBuffer       *dst,
                                 EvaBuffer       *src,
                                 GError         **error)
{
  EvaMimeBase64Decoder *decoder = EVA_MIME_BASE64_DECODER (filter);
  /* Decode to binary data one character at a time. */
  int c;
  while ((c = eva_buffer_read_char (src)) != -1)
    {
      if (!decoder_process_one (decoder, dst, c, error))
	return FALSE;
    }
  return TRUE;
}

static gboolean
eva_mime_base64_decoder_flush (EvaSimpleFilter *filter,
                               EvaBuffer       *dst,
                               EvaBuffer       *src,
                               GError         **error)
{
  eva_mime_base64_decoder_process (filter, dst, src, error);
  if (!EVA_MIME_BASE64_DECODER (filter)->got_terminal)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   EVA_ERROR_BAD_FORMAT,
		   _("missing terminal '%c' in base64 encoded stream"),
		   base64_terminal_char);
      return FALSE;
    }
  return TRUE;
}

/* --- functions --- */
static void
eva_mime_base64_decoder_init (EvaMimeBase64Decoder *mime_base64_decoder)
{
}

static void
eva_mime_base64_decoder_class_init (EvaMimeBase64DecoderClass *class)
{
  EvaSimpleFilterClass *simple_filter_class = EVA_SIMPLE_FILTER_CLASS (class);
  guint i;
  parent_class = g_type_class_peek_parent (class);
  simple_filter_class->process = eva_mime_base64_decoder_process;
  simple_filter_class->flush = eva_mime_base64_decoder_flush;

  memset (ascii_to_base64, BASE64_VALUE_BAD_CHAR, 256);
  for (i = 1; i < 128; i++)
    if (isspace (i))
      ascii_to_base64[i] = BASE64_VALUE_WHITESPACE;
  ascii_to_base64[base64_terminal_char] = BASE64_VALUE_TERMINAL;
  for (i = 0; i < 64; i++)
    ascii_to_base64[(guint8)base64_chars[i]] = i;
}

GType eva_mime_base64_decoder_get_type()
{
  static GType mime_base64_decoder_type = 0;
  if (!mime_base64_decoder_type)
    {
      static const GTypeInfo mime_base64_decoder_info =
      {
	sizeof(EvaMimeBase64DecoderClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_mime_base64_decoder_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMimeBase64Decoder),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_mime_base64_decoder_init,
	NULL		/* value_table */
      };
      mime_base64_decoder_type = g_type_register_static (EVA_TYPE_SIMPLE_FILTER,
                                                  "EvaMimeBase64Decoder",
						  &mime_base64_decoder_info, 0);
    }
  return mime_base64_decoder_type;
}

/**
 * eva_mime_base64_decoder_new:
 *
 * Allocate a new MIME encoder which
 * takes base64 encoded data in and gives
 * raw binary data out.
 *
 * returns: the newly allocated decoder.
 */
EvaStream *
eva_mime_base64_decoder_new (void)
{
  return g_object_new (EVA_TYPE_MIME_BASE64_DECODER, NULL);
}

/* ==================== EvaMimeBase64Encoder =========================== */

/* --- typedefs --- */
typedef struct _EvaMimeBase64Encoder EvaMimeBase64Encoder;
typedef struct _EvaMimeBase64EncoderClass EvaMimeBase64EncoderClass;
/* --- type macros --- */
GType eva_mime_base64_encoder_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MIME_BASE64_ENCODER			(eva_mime_base64_encoder_get_type ())
#define EVA_MIME_BASE64_ENCODER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MIME_BASE64_ENCODER, EvaMimeBase64Encoder))
#define EVA_MIME_BASE64_ENCODER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MIME_BASE64_ENCODER, EvaMimeBase64EncoderClass))
#define EVA_MIME_BASE64_ENCODER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MIME_BASE64_ENCODER, EvaMimeBase64EncoderClass))
#define EVA_IS_MIME_BASE64_ENCODER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MIME_BASE64_ENCODER))
#define EVA_IS_MIME_BASE64_ENCODER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MIME_BASE64_ENCODER))

/* --- structures --- */
struct _EvaMimeBase64EncoderClass 
{
  EvaSimpleFilterClass simple_filter_class;
};
struct _EvaMimeBase64Encoder 
{
  EvaSimpleFilter      simple_filter;
  guint chars_per_line;
  guint chars_in_this_line;
  guint8 n_bits;	/* 0,2,4 */
  guint8 partial_data;	/* the first n_bits on 32,16,8,4,2,1 used */
};
/* --- prototypes --- */

/* --- EvaSimpleFilter methods --- */
static gboolean
eva_mime_base64_encoder_process (EvaSimpleFilter *filter,
                                 EvaBuffer       *dst,
                                 EvaBuffer       *src,
                                 GError         **error)
{
  EvaMimeBase64Encoder *encoder = EVA_MIME_BASE64_ENCODER (filter);
  guint8 n_bits = encoder->n_bits;
  guint8 partial_data = encoder->partial_data;
  guint chars_in_this_line = encoder->chars_in_this_line;
  guint chars_per_line = encoder->chars_per_line;
  int c;
  while ((c=eva_buffer_read_char (src)) != -1)
    {
      /* Append 6 bits of data to the stream (encoding to the
	 base64 character set).  Uses chars_in_this_line,chars_per_line.
       */
#define APPEND_6BITS_AND_MAYBE_ADD_NEWLINE(value)		\
      G_STMT_START{						\
	eva_buffer_append_char (dst, base64_chars[(value)]);	\
	if (++chars_in_this_line == chars_per_line)		\
	  {							\
	    eva_buffer_append (dst, "\r\n", 2);			\
	    chars_in_this_line = 0;				\
	  }							\
      }G_STMT_END
      switch (n_bits)
	{
	case 0:
	  APPEND_6BITS_AND_MAYBE_ADD_NEWLINE (c>>2);
	  n_bits = 2;
	  partial_data = (c & 3) << 4;
	  break;
	case 2:
	  APPEND_6BITS_AND_MAYBE_ADD_NEWLINE (partial_data | (c>>4));
	  n_bits = 4;
	  partial_data = (c & 15) << 2;
	  break;
	case 4:
	  APPEND_6BITS_AND_MAYBE_ADD_NEWLINE (partial_data | (c>>6));
	  APPEND_6BITS_AND_MAYBE_ADD_NEWLINE (c % (1<<6));
	  n_bits = 0;
	  partial_data = 0;
	  break;
	}
    }
  encoder->n_bits = n_bits;
  encoder->partial_data = partial_data;
  encoder->chars_in_this_line = chars_in_this_line;
  encoder->chars_per_line = chars_per_line;
  return TRUE;
}

static gboolean
eva_mime_base64_encoder_flush (EvaSimpleFilter *filter,
                               EvaBuffer       *dst,
                               EvaBuffer       *src,
                               GError         **error)
{
  EvaMimeBase64Encoder *encoder = EVA_MIME_BASE64_ENCODER (filter);

  /* These are use by APPEND_CHARS_AND_MAYBE_ADD_NEWLINE. */
  guint chars_in_this_line = encoder->chars_in_this_line;
  guint chars_per_line = encoder->chars_per_line;

  g_return_val_if_fail (src->size == 0, FALSE);
  if (encoder->n_bits)
    APPEND_6BITS_AND_MAYBE_ADD_NEWLINE (encoder->partial_data);
  eva_buffer_append (dst, "=\r\n", 3);
  return TRUE;
}

/* --- functions --- */
static void
eva_mime_base64_encoder_init (EvaMimeBase64Encoder *mime_base64_encoder)
{
}

static void
eva_mime_base64_encoder_class_init (EvaMimeBase64EncoderClass *class)
{
  EvaSimpleFilterClass *simple_filter_class = EVA_SIMPLE_FILTER_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  simple_filter_class->process = eva_mime_base64_encoder_process;
  simple_filter_class->flush = eva_mime_base64_encoder_flush;
}

GType eva_mime_base64_encoder_get_type()
{
  static GType mime_base64_encoder_type = 0;
  if (!mime_base64_encoder_type)
    {
      static const GTypeInfo mime_base64_encoder_info =
      {
	sizeof(EvaMimeBase64EncoderClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_mime_base64_encoder_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMimeBase64Encoder),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_mime_base64_encoder_init,
	NULL		/* value_table */
      };
      mime_base64_encoder_type = g_type_register_static (EVA_TYPE_SIMPLE_FILTER,
                                                  "EvaMimeBase64Encoder",
						  &mime_base64_encoder_info, 0);
    }
  return mime_base64_encoder_type;
}

/**
 * eva_mime_base64_encoder_new:
 *
 * Allocate a new MIME encoder which
 * takes raw binary data in and gives
 * base64 encoded data out.
 *
 * returns: the newly allocated encoder.
 */
EvaStream *
eva_mime_base64_encoder_new (void)
{
  return g_object_new (EVA_TYPE_MIME_BASE64_ENCODER, NULL);
}
