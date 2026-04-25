CC     = gcc
CFLAGS = -Wall -Wextra -g

city_manager: city_manager.c
	$(CC) $(CFLAGS) -o city_manager city_manager.c

clean:
	rm -f city_manager
