#include <../hash/evahash.h>
#include <../evaghelpers.h>
#include <string.h>

typedef struct _HashSumTest HashSumTest;
struct _HashSumTest
{
  const char *str;
  char md5[16];
  char sha1[20];
};

static HashSumTest tests[] =
{
  {
    "a",
    { 0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
      0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61 },
    { 0x86, 0xF7, 0xE4, 0x37, 0xFA, 0xA5, 0xA7, 0xFC,
      0xE1, 0x5D, 0x1D, 0xDC, 0xB9, 0xEA, 0xEA, 0xEA,
      0x37, 0x76, 0x67, 0xB8 }
  }
};
#define TEST_COUNT		G_N_ELEMENTS(tests)

int main ()
{
  EvaHash *h;
  guint i;
  for (i = 0; i < TEST_COUNT; i++)
    {
      guint8 buf[20];

      h = eva_hash_new_md5 ();
      eva_hash_feed_str (h, tests[i].str);
      eva_hash_done (h);
      eva_hash_get (h, buf);
      g_assert (memcmp (buf, tests[i].md5, 16) == 0);
      eva_hash_destroy (h);

      h = eva_hash_new_sha1 ();
      eva_hash_feed_str (h, tests[i].str);
      eva_hash_done (h);
      eva_hash_get (h, buf);
      g_assert (memcmp (buf, tests[i].sha1, 20) == 0);
      eva_hash_destroy (h);
    }
  return 0;
}

