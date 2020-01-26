/* registry.h */
#ifndef REGISTRY_H
#define REGISTRY_H


#include <pthread.h>
#include "json.h"


struct Registry_Json
{
    Json *reg;
    int size;
    int capacity;
}typedef Registry_Json;

void printReg(Registry_Json);
void incrRegCapacity(Registry_Json *, int);
void addReg(Registry_Json *, Json, pthread_mutex_t *);
Registry_Json createRegister(pthread_mutex_t *);
void initRegister(Registry_Json *, pthread_mutex_t *);

#endif /* REGISTRY_H */
