override CFLAGS=-Wall -Wextra -Xanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Werror
endif

.PHONY: clean all