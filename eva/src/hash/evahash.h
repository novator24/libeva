#ifndef __EVA_HASH_H_
#define __EVA_HASH_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GskHash GskHash;

/* --- public interface --- */
GskHash   *eva_hash_new_md5    (void);
GskHash   *eva_hash_new_sha1   (void);
GskHash   *eva_hash_new_sha256 (void);      /* WARNING: untested */
GskHash   *eva_hash_new_crc32  (gboolean        big_endian);
void       eva_hash_feed       (GskHash        *hash,
                                gconstpointer   data,
				guint           length);
void       eva_hash_feed_str   (GskHash        *hash,
                                const char     *str);
void       eva_hash_done       (GskHash        *hash);
guint      eva_hash_get_size   (GskHash        *hash);
void       eva_hash_get        (GskHash        *hash,
                                guint8         *data_out);
void       eva_hash_get_hex    (GskHash        *hash,
                                gchar          *hex_out);
void       eva_hash_destroy    (GskHash        *hash);


/* --- for implementing new types of hash functions --- */
struct _GskHash
{
  /* The size of the hash-key (in bytes) */
  guint       size;

  /*< protected >*/
  void      (*feed)     (GskHash       *hash,
                         gconstpointer  data,
		         guint          len);
  gpointer  (*done)     (GskHash       *hash);
  void      (*destroy)  (GskHash       *hash);

  /*< private >*/
  guint	      flags;		/* constructor must set this to 0 */
  gpointer    hash_value;
};

G_END_DECLS

#endif
