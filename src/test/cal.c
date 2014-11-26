#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include "sum.h"

//void (*sum)(int , int); // 부르고자 하는 함수의 포인터

int main()
{
    /*
       int value;

    //void *handle = dlopen("./libmycalcso.so", RTLD_LAZY);
    //sum = dlsym(handle, "sum"); // addvec이란 함수 포인터를 가져옴
    //addvec(x, y, z, 2);
    //value = DRIVER(130, 199);
    printf("%d\n", value);

    value = sum2(131, 199);
    printf("%d\n", value);

     */
    void *handle;
    int (*sum)(int, int);
    char *error;

    handle = dlopen ("./libmycalcso.so", RTLD_LAZY);
    if (!handle) {
	fputs (dlerror(), stderr);
	exit(1);
    }

    sum = dlsym(handle, "sum");
    if ((error = dlerror()) != NULL)  {
	fputs(error, stderr);
	exit(1);
    }

    printf ("%d\n", (*sum)(131,199));
    dlclose(handle);
}
