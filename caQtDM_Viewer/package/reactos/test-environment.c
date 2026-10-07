#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "envDefs.h"

int main(void)
{
    const char *name = "CAQTDM_REACTOS_ENV_TEST";
    const char *value;

    epicsEnvSet(name, "first");
    value = getenv(name);
    if (!value || strcmp(value, "first")) {
        fputs("EPICS environment set failed\n", stderr);
        return 1;
    }
    epicsEnvSet(name, "replacement");
    value = getenv(name);
    if (!value || strcmp(value, "replacement")) {
        fputs("EPICS environment replacement failed\n", stderr);
        return 2;
    }
    epicsEnvUnset(name);
    if (getenv(name)) {
        fputs("EPICS environment unset failed\n", stderr);
        return 3;
    }
    puts("EPICS environment set/replace/unset passed.");
    return 0;
}
