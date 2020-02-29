#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "./../include/registry.h"


Registry* createRegister(pthread_mutex_t *lock)
{
    Registry *registry = malloc(sizeof(*registry));
    registry->reg = malloc(16 * sizeof(Json));
    registry->size = 0;
    registry->capacity = 16;

    int init_lock = pthread_mutex_init(lock, NULL);

    if(init_lock != 0)
    {
        perror("Mutex Initialization Failed.");
        nextLine();
        return NULL;
    }

    return registry;
}

int initRegister(Registry *registry, pthread_mutex_t *lock)
{
    registry->reg = malloc(16 * sizeof(Json));
    registry->size = 0;
    registry->capacity = 16;

    int init_lock = pthread_mutex_init(lock, NULL);

    if(init_lock != 0)
    {
        perror("Mutex Initialization Failed.");
        nextLine();
        return 0;
    }

    return 1;
}

void addReg(Registry *reg, Json *json, pthread_mutex_t *lock)
{
    pthread_mutex_lock(lock);

    if((reg->size + 2) > reg->capacity)
    {
        incrRegCapacity(reg, 16);
    }

    reg->reg[reg->size++] = *json;

    pthread_mutex_unlock(lock);
}

void incrRegCapacity(Registry *reg, int incr)
{
    Json *old = reg->reg;
    Json *new = malloc((reg->capacity + incr) * sizeof(Registry));

    for(int i = 0; i < reg->size; i++)
    {
        new[i] = old[i];
    }

    reg->reg = new;
    reg->capacity = reg->capacity + incr;
}

void printReg(Registry *reg)
{
    if(reg->size > 0)
    {
        for(int i = 0; i < reg->size; i++)
        {
            putchar('{');
            for(int j = 0; j < reg->reg[i].size; j++)
            {
                String key = reg->reg[i].entry[j].key;

                String value = reg->reg[i].entry[j].value;

                nextLine();
                putchar('\t');
                printS(&key);
                putchar(':');
                printS(&value);
            }
            nextLine();
            putchar('}');
        }
        nextLine();
    }
    else
    {
        printf("{}");
        nextLine();
    }
}
