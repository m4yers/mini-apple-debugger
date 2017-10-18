#include <cstdio>
#include <cstring>
#include <unistd.h>

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  const char * str = "Hello, World!\n";
  write(1, str, strlen(str));
  return 0;
}
