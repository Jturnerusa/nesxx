#include <stdio.h>
#include "cpu.h"
#include "config.h"

#ifndef TEST

int main(int argc, char *argv[])
{
    run_cpu_tests();
}

#endif


#ifdef TEST

int main(int argc, char *argv[])
{
    printf("Starting unit tests\n");
    run_cpu_tests();
}

#endif
