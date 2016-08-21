#include <stdlib.h>
#include <stdio.h>
#include "libscribble/scribble.h"


void this_is_a_silly_test() {
  char* string_test = (char*)malloc(5 * sizeof(char));
  string_test[0] = '3';
  string_test[1] = '2';
  string_test[2] = '1';
  string_test[3] = '?';
  string_test[4] = '\0';
  printf("Within a silly test\n");
  int thirteen = do_a_thing_or_whatever(string_test);
  if(thirteen != 13) {
    printf("What does it all mean?\n");
  } else {
    printf("The name matches the value, so meta\n");
  }
  free(string_test);
}