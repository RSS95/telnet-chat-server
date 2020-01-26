/* json.h */
#ifndef JSON_H
#define JSON_H


#include "string_array.h"


/* declaration */
struct Entry{
   String key;
   String value;
}typedef Entry;

struct Json{
        Entry *entry;
            int size;
                int capacity;
}typedef Json;


Entry createEntry();
Json createJson();
void addJson(Json *, String);
void incrJsonCapacity(Json *);
String getVal(Json, String);


#endif /* TEST_H */
