#include <dlfcn.h>
#include <stdio.h>
#include <math.h>

int main(int argc, char **argv) {
    printf("Opening library libm\n");
    void *dl_handle = dlopen("libm.so.6", RTLD_LAZY);
    double (*sin)(double) = dlsym(dl_handle, "sin");
    printf("sin(90o) = %f\n", (*sin)(M_PI_2));
    dlclose(dl_handle);
    return 0;
}
