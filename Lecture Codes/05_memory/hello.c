#include "hello.h"

#include <stdio.h>

int sop_number = 2;

void hello() {
    printf("Hello my friend! :)\n");
}

int foo(int x) {
    return x + sop_number;
}
