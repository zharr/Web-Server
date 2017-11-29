#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/fcntl.h>
#include "stats.h"
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
sem_t* mutex;
key_t key;
stats_t *stat;
int shmid;

void
handler(int sig) {
  int y = 1;
  if (shmdt(stat) < 0) {
    perror("shmdt failed server");
    y = 0;
  }
  if (shmctl(shmid, IPC_RMID, 0) < 0) {
    perror("shmctl failed server");
    y = 0;
  }
  if (sem_close(mutex) < 0) {
    perror("sem close failed server");
    y = 0;
  }
  if (sem_unlink("yashharrison2111") < 0) {
    perror("sem unlink failed server");
    y = 0;
  }
  if (y == 0) {
    exit(1);
  }

  exit(0);
}


int main(int argc, char *argv[]) {
  size_t size = getpagesize();
  int iter = 0;

  if (argc != 3 || strcmp(argv[1], "-k") != 0) {
    fprintf(stderr, "Usage: stats_server -k [key]\n");
    fflush(stderr);
    exit(1);
    }

  key = atoi(argv[2]);

  if ((shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
    perror("shmget server");
    exit(1);
  }

  stat = shmat(shmid, NULL, 0);
  memset(stat, 0, size);
  if (stat == (stats_t*) -1) {
    perror("shmat server");
    exit(1);
  }
  stats_t *runner = stat;
  int k;
  for (k = 0; k < 16; k++) {
    runner->pid = 0;
    runner->counter = 0;
    runner->priority = 0;
    runner->cpu_secs = 0;
    runner->dead = 1;
    runner->unlink = 0;
    runner++;
  }

  if ((mutex = sem_open("yashharrison2111", O_CREAT, 0664, 1)) == SEM_FAILED) {
      perror("sem_open server");
      exit(1);
  }
  struct sigaction act;
  act.sa_handler = &handler;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGINT);
  memset(&act.sa_flags, 0, sizeof(act.sa_flags));
  if (sigaction(SIGINT, &act, NULL) == -1) {
      perror("can't handle signal server");
      exit(1);
  }
  while (1) {
    int i;
    stats_t *runner = stat;
    for (i = 0; i < 16; i++) {
      // look for clients
      if (runner->dead == 0) {
        fprintf(stdout, "%d %d %s %d %.2f %d\n", iter, runner->pid, runner->arg,
          runner->counter, runner->cpu_secs, runner->priority);
      }
      runner++;
    }

    iter++;
    printf("\n");
    sleep(1);
  }
  if (sem_close(mutex) < 0) {
    perror("sem close failed server");
    exit(1);
  }
  return 0;
}
