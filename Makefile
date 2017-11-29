GCC=gcc
CFLAGS=-Wall -g -Werror

all: library


library: stats.c
	export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
	$(CC) -c -fpic stats.c $(CLAGS)
	$(CC) -shared -o libstats.so stats.o
	$(CC) -o stats_server stats_server.c -Wall -g -Werror -lstats -L. libstats.so -lpthread
	$(CC) -o stats_client stats_client.c -Wall -g -Werror -lstats -L. libstats.so -lpthread

clean:
	$(RM) stats_server
	$(RM) stats_client
	$(RM) libstats.so 
