#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/fcntl.h>
#include "stats.h"
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
stats_t* stat;
sem_t *mutex;

stats_t*
stats_init(key_t key) {
  int shmid;
  mutex = sem_open("yashharrison2111", 0);
  if (mutex == SEM_FAILED) {
    perror("failed to open sem\n");
    return NULL;
  }
  if (sem_wait(mutex) < 0) {
    perror("sem wait failed\n");
    sem_close(mutex);
    return NULL;
  }
  if ((shmid = shmget(key, getpagesize(), 0666)) == -1) {
    perror("shmget stat_init");
    sem_close(mutex);
    return NULL;
  }
  stat = shmat(shmid, NULL, 0);
  if (stat == (stats_t*) -1) {
    perror("shmat");
    sem_close(mutex);
    return NULL;
  }

  int found = 0;
  int i;
  stats_t *runner = stat;
  for (i = 0; i < 16; i++) {
    if (runner->dead == 1) {
      runner->dead = 0;
      found = 1;
      break;
    }
    runner++;
  }

  if (sem_post(mutex) < 0) {
    perror("sempost failed in init");
    sem_close(mutex);
    return NULL;
  }
  if (found == 0) {
    return NULL;
  }
  return runner;
}

int
stats_unlink(key_t key) {
  int shmid;
  if (sem_wait(mutex) < 0) {
    perror("sem wait failed\n");
    sem_close(mutex);
    return -1;
  }
  int found = 0;
  int i;
  stats_t *runner = stat;
  for (i = 0; i < 16; i++) {
    if (runner->unlink == 1) {
      found = 1;
      runner->dead = 1;
      runner->unlink = 0;
      runner->pid = 0;
      runner->counter = 0;
      runner->cpu_secs = 0;
      runner->priority = 0;
      break;
    }
    runner++;
  }
  shmdt(stat);
  if (sem_post(mutex) < 0) {
    perror("sem post failed\n");
    sem_close(mutex);
    return -1;
  }
  if (sem_close(mutex) < 0) {
    perror("semclose failed");
    return -1;
  }
  if (found == 1) {
    return 0;
  }
  return -1;
}
