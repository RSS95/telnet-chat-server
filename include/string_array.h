/* string_array.h */
#ifndef STRING_H
#define STRING_H


#define INIT_CAPACITY 10
#define INCREMENT_CAPACITY 10


struct String{
    char *string;
    int length;
    int capacity;
}typedef String;

struct SArray{
    String *sarray;
    int length;
    int capacity;
}typedef SArray;


String getString();
String getStringFrom(char *);
void nextLine();
int strLen(char*);
void printS(String*);
void appendStr(String*, char*);
void incrCapacity(String *, int);
void duplicateStr(char*, char*);
void printC(char*);
void destroyStr(String*);
int compareStr(String, String);
int findPos(String, char);
SArray createSArray();
void addString(SArray *, String *);
SArray split(String, char);
void addString(SArray *, String *);
void incrSArrayCapacity(SArray *);


#endif /* STRING_H */
