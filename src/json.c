#include<stdio.h>
#include<stdlib.h>
#include "./../include/string_array.h"


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


//int main()
//{
//    Json j = createJson();
//    String kv = getStringFrom("Ritesh:Salian");
//    addJson(&j, kv);
//    String k = getStringFrom("Ritesh");
//    String v = getVal(j, k);
//    nextLine();
//    printf("Value for Ritesh : ");
//    printS(&v);
//
//    return 0;
//}

Entry createEntry(String str)
{
    Entry e;
    SArray sarr = split(str, ':');

    //String g = getString(sarr[0]);
    //String h = getString(sarr[1]);
    e.key = sarr.sarray[0];
    e.value = sarr.sarray[1];
    return e;
}

Json createJson()
{
    Json j;
    j.entry = malloc(10 * sizeof(Entry));
    j.size = 0;
    j.capacity = 10;
    return j;
}

void addJson(Json *json, String str)
{
    Entry e = createEntry(str);

    if(json->size + 5 > json->capacity)
    {
        incrJsonCapacity(json);
    }

    json->entry[json->size++] = e;
}

void incrJsonCapacity(Json *json)
{
    Entry *newEntry = malloc((json->capacity + 10) * sizeof(Entry));
    Entry *oldEntry = json->entry;
    for(int i = 0; i < json->size; i++)
    {
        newEntry[i] = oldEntry[i];
    }

    json->entry = newEntry;
    json->capacity = json->capacity + 10;
}

String getVal(Json json, String key)
{
    String s;
    Entry *e = json.entry;

    for(int i = 0; i < json.size; i++)
    {
        if(compareStr(key, e[i].key) == 0)
        {
            return e[i].value;
        }
    }
}
