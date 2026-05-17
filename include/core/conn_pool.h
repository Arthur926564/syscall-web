#pragma once
#ifndef CORE_POOL_H
#define CORE_POOL_H

#include "core/connection.h"

#define CONN_POOL_CAP 512


typedef struct conn_pool {
    connection_t *stack[CONN_POOL_CAP];
	connection_t conns[CONN_POOL_CAP];
    int top;
} conn_pool_t;

void conn_pool_init(conn_pool_t *p);
connection_t *conn_pool_get(conn_pool_t *p);
void conn_pool_put(conn_pool_t *p, connection_t *conn);

#endif
