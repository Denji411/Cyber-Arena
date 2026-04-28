#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "decepticons.h"

void *decepticon(void* arg) {
    Status *st = (Status*)arg;
    
    pthread_exit(NULL);
}