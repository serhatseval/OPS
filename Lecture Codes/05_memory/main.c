#include "hello.h"

#include <stdio.h>

int main() {
    hello();
    printf("&sop_number = %p\n", (void *) &sop_number);
    return foo(sop_number);
}
