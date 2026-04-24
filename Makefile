CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c99

city_manager: city_manager.c
	$(CC) $(CFLAGS) -o city_manager city_manager.c
