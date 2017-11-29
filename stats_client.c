#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "stats.h"
#include <time.h>
#include <sys/resource.h>
#include <signal.h>
#include <semaphore.h>
#include <string.h>


key_t key = -1;
stats_t *stat;
void
handler(int sig) {
  if (sig == SIGINT) {
    stat->unlink = 1;
    stats_unlink(key);
    exit(0);
  }
  exit(1);
}

int main(int argc, char *argv[]) {
  int priority =  -1;
  long sleeptime_ns = -1;
  long cputime_ns = -1;
  int c;
  int args = 0;
  opterr = 0;
  while ((c = getopt(argc, argv, "k:p:s:c:")) != -1) {
    switch (c) {
    case 'k':
      key  = atoi(optarg);
      args++;
      break;
    case 'p':
      priority = atoi(optarg);
      args++;
      break;
    case 's':
      sleeptime_ns = atol(optarg);
      args++;
      break;
    case 'c':
      cputime_ns = atol(optarg);
      args++;
      break;
    default:
      break;
    }
  }

  struct timespec start, end;
  struct timespec sleep_time;
  sleep_time.tv_sec = sleeptime_ns / 1000000000;
  sleep_time.tv_nsec = sleeptime_ns % 1000000000;

  // initialize
  stat = stats_init(key);
  if (stat == NULL) {
    perror("stats init failed client");
    exit(1);
  }

  stat->pid = getpid();
  stat->cpu_secs = 0;
  stat->dead = 0;
  stat->unlink = 0;
  stat->counter = 0;
  strncpy(stat->arg, argv[0], sizeof(stat->arg));

  if (priority == -1) {
    priority = getpriority(PRIO_PROCESS, stat->pid);
    stat->priority  = priority;
  }

  if (sleeptime_ns == -1) {
    sleeptime_ns = 1000;
  }

  if (cputime_ns == -1) {
    cputime_ns = 1000000000;
  }

  int rc = setpriority(PRIO_PROCESS, stat->pid, priority);
  if (rc < 0) {
    fprintf(stderr, "error setting priority\n");
    exit(1);
  }
  struct sigaction act;
  act.sa_handler = &handler;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGINT);
  memset(&act.sa_flags, 0, sizeof(act.sa_flags));
  if (sigaction(SIGINT, &act, NULL) == -1) {
    perror("can't handle signal");
    exit(1);
  }

  while (1) {
    // compute
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    while (1) {
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
      uint64_t diff;
      uint64_t sec_diff = end.tv_sec - start.tv_sec;
      diff = 1000000000.0 * sec_diff + end.tv_nsec - start.tv_nsec;
      if (diff >= cputime_ns) {
        break;
      }
    }
    stat->cpu_secs += cputime_ns/1000000000.0;
    stat->counter++;
    if (nanosleep(&sleep_time, NULL) < 0) {
      perror("error in nanosleep\n");
    }
  }
  return 0;
}
