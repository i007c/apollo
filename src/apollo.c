
#include "apollo.h"
#include "logger.h"

#include <stdlib.h>
#include <time.h>

#define LS SECTOR_MAIN_APOLLO
#define PF "\033[32m%f\033[0m"


int main(void) {
    log_info("Starting Apollo Valkan");
    srand(time(NULL));

    return 0;
}

