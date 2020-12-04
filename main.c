#include <stdio.h>
#include "cpu.h"
#include "config.h"

#ifndef UNITTEST

int main(int argc, char *argv[])
{
    return 0;
}

#endif


#ifdef UNITTEST

int main(int argc, char *argv[])
{
    printf("Starting unit tests\n");
    run_cpu_tests();
}

#endif
