#include "../evadebugalloc.h"

void
b(void)
{
  g_malloc (512);
}

int main(int argc, char **argv)
{
  guint i;
  eva_set_debug_mem_vtable (argv[0]);
  g_atexit (eva_print_debug_mem_vtable);

  g_malloc (1024);
  for (i = 0; i < 4; i++)
    b();

  return 0;
}
