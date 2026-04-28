#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "globals.h"
#include "decepticons.h"

int main() {
    Status *st = malloc(sizeof(Status));
    free(st);
    return EXIT_SUCCESS;
}