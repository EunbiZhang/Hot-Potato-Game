CFLAGS=-O3 -fPIC -Wall -Werror

TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.cpp potato.h
	g++ $(CFLAGS) -g -o $@ $<

ringmaster: ringmaster.cpp potato.h
	g++ $(CFLAGS) -g -o $@ $<