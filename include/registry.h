/* registry.h */
#ifndef REGISTRY_H
#define REGISTRY_H


#include <pthread.h>
#include "json.h"


struct Registry
{
    Json *reg;
    int size;
    int capacity;
}typedef Registry;


Registry* createRegister(pthread_mutex_t *);
int initRegister(Registry *, pthread_mutex_t *);
void addReg(Registry *, Json *, pthread_mutex_t *);
void incrRegCapacity(Registry *, int);
void printReg(Registry *);


#endif /* REGISTRY_H */
