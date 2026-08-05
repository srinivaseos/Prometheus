#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef PGW_H
#define PGW_H

typedef struct xiuspgw xiuspgw_t;
typedef struct pgw_session pgw_session_t;

void pgw_session__free( pgw_session_t * sess);
pgw_session_t * pgw_session__allocate();

#endif


