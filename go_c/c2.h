#ifndef CGO_EXAMPLE_H
#define CGO_EXAMPLE_H

#include <stdint.h>

typedef struct nprofile {
    char name[20];
    uint32_t ipv4;
    int id;
    int port;
} nprofile_t;

nprofile_t* create_profile();
void free_profile(nprofile_t *nrf);

#endif // CGO_EXAMPLE_H

