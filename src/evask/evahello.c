#include <eva/evahello.h>
#include <stdio.h>

void eva_hello_do_action(EvaHello *self /*, parameters */) {
  g_return_if_fail (EVA_IS_HELLO (self));
  printf("EVA: hello\n");
}

