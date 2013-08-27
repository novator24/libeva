#ifndef __EVA_HASH_H_
#define __EVA_HASH_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _EvaHash EvaHash;

/* --- public interface --- */
EvaHash   *eva_hash_new_md5    (void);
EvaHash   *eva_hash_new_sha1   (void);
EvaHash   *eva_hash_new_sha256 (void);      /* WARNING: untested */
EvaHash   *eva_hash_new_crc32  (gboolean        big_endian);
void       eva_hash_feed       (EvaHash        *hash,
                                gconstpointer   data,
				guint           length);
void       eva_hash_feed_str   (EvaHash        *hash,
                                const char     *str);
void       eva_hash_done       (EvaHash        *hash);
guint      eva_hash_get_size   (EvaHash        *hash);
void       eva_hash_get        (EvaHash        *hash,
                                guint8         *data_out);
void       eva_hash_get_hex    (EvaHash        *hash,
                                gchar          *hex_out);
void       eva_hash_destroy    (EvaHash        *hash);


/* --- for implementing new types of hash functions --- */
struct _EvaHash
{
  /* The size of the hash-key (in bytes) */
  guint       size;

  /*< protected >*/
  void      (*feed)     (EvaHash       *hash,
                         gconstpointer  data,
		         guint          len);
  gpointer  (*done)     (EvaHash       *hash);
  void      (*destroy)  (EvaHash       *hash);

  /*< private >*/
  guint	      flags;		/* constructor must set this to 0 */
  gpointer    hash_value;
};

G_END_DECLS

#endif
