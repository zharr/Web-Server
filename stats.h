typedef struct {
  int pid;
  int counter;
  int priority;
  double cpu_secs;
  int dead;
  char arg[15];
  int unlink;
} stats_t;

int
stats_unlink(key_t key);

stats_t*
stats_init(key_t key);
