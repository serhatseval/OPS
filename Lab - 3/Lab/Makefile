override CFLAGS=-Wall -Wextra -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Werror
endif

.PHONY: clean all

all: sop-ca

sop-ca: sop-ca.c	
	gcc $(CFLAGS) -o sop-ca sop-ca.c

clean:
	rm sop-ca
