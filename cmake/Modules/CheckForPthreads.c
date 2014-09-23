#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* runner(void*);

int res = 0;
#ifdef __CLASSIC_C__
int main(){
  int ac;
  char*av[];
#else
int main(int ac, char*av[]){
#endif
  pthread_t tid[2];
  pthread_create(&tid[0], 0, runner, (void*)1);
  pthread_create(&tid[1], 0, runner, (void*)2);

#if defined(__BEOS__) && !defined(__ZETA__) // (no usleep on BeOS 5.)
  usleep(1); // for strange behavior on single-processor sun
#endif

  pthread_join(tid[0], 0);
  pthread_join(tid[1], 0);
  if(ac > 1000){return *av[0];}
  return res;
}

void* runner(void* args)
{
  int cc;
  for ( cc = 0; cc < 10; cc ++ )
    {
    printf("%d CC: %d\n", (int)args, cc);
    }
  res ++;
  return 0;
}
