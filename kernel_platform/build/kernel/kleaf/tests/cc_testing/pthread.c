
#include <stdio.h>
#include <pthread.h>

void* routine(void* unused) {
  printf("child thread!\n");
  return NULL;
}

int main() {
  pthread_t pt;
  pthread_create(&pt, NULL, &routine, NULL);
  printf("main thread!\n");
  pthread_join(pt, NULL);
  return 0;
}
