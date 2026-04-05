#pragma once
#ifndef CORE_POOL_H
#define CORE_POOL_H

#define CONN_POOL_CAP 512

typedef struct connection connection_t;

typedef struct conn_pool {
    connection_t *stack[CONN_POOL_CAP];
    int top;
} conn_pool_t;

connection_t *conn_pool_get(conn_pool_t *p);
void conn_pool_put(conn_pool_t *p, connection_t *conn);

#endif
