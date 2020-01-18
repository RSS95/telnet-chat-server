#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "./../include/string_array.h"

/*
#define INIT_CAPACITY 10
#define INCREMENT_CAPACITY 10


struct String{
    char *string;
    int length;
    int capacity;
}typedef String;


String getString();
void nextLine();
int strLen(char*);
void printS(String*);
void appendStr(String*, char*);
void incrCapacity(String *, int);
void duplicateStr(char*, char*);
void printC(char*);
void destroyStr(String*);
*/
/*
int main()
{
    char* s = "";
    int c = strLen(s);
    printf("Test_1 :: count : %d", c);
    nextLine();

    String str = getString();
    appendStr(&str, "Ritesh");

    printf("Test_2 :: struct string : %s", str.string);
    nextLine();
    
    String g = getString();
    appendStr(&g, "Ritesh");

    int k = compareStr(str, g);
    printf("Compare :: %d", k);
    nextLine();

    c = strLen(str.string);
    printf("Test_3 :: struct string length calculated : %d", c);
    nextLine();

    printf("Test_4 :: struct string lenght : %d", str.length);
    nextLine();

    printf("Test_5 :: struct string print using printS fn : ");
    printS(&str);
    nextLine();

    appendStr(&str, " is the Name, Salian is his Surname and he lives in Mumbai");
    printf("Test_6 :: new append : ");
    printS(&str);
    nextLine();

    String splitTest = getStringFrom("Ritesh:Sadhu:Harinakshi:Salian");
    SArray sarray = split(splitTest, ':');
    printf("Split Array Length : %d", sarray.length);
    nextLine();

    for(int i = 0; i < sarray.length; i++)
    {
        printS(&(sarray.sarray[i]));
        nextLine();
    }

    return 0;
}
*/

SArray createSArray()
{
    SArray sarray;
    sarray.sarray = malloc(INIT_CAPACITY * sizeof(String));
    sarray.length = 0;
    sarray.capacity = INIT_CAPACITY;
    return sarray;
}

void addString(SArray *sarray, String *str)
{
    if((sarray->length + 5) > sarray->capacity)
    {
        incrSArrayCapacity(sarray);
    }

    sarray->sarray[sarray->length++] = *str;
}

void incrSArrayCapacity(SArray *sarr)
{
    String *oldString = sarr->sarray;
    String *newString = malloc((sarr->capacity + 10) * sizeof(String));

    for(int i = 0; i < sarr->length; i++)
    {
        newString[i] = oldString[i];
    }

    sarr->sarray = newString;
    sarr->capacity = sarr->capacity + 10;
}

String getString()
{
    String string;
    string.string = malloc(INIT_CAPACITY * sizeof(char));
    string.string[0] = '\0';
    string.length = 1;
    string.capacity = INIT_CAPACITY;
    return string;
}

String getStringFrom(char *arr)
{
    String string = getString();
    appendStr(&string, arr);
    return string;
}

void nextLine()
{
    putchar('\n');
}

int strLen(char *str)
{
    int c = 0;
    while(str[c++] != '\0');
    return c;
}

void printS(String *string)
{
    for(int i = 0; i < string->length - 1; i++)
    {
        putchar(string->string[i]);
    }
}

void printC(char *array)
{
    int i = 0;
    char c;
    while((c = array[i]) != '\0')
    {
        putchar(c);
        i++;
    }
}

void appendStr(String *string, char *append)
{
    printS(string);
    //nextLine();
    printf(" : + : ");
    printC(append);
    nextLine();
    int appendLen = strLen(append);
    if(string->length + appendLen > string->capacity)
    {
        incrCapacity(string, appendLen);
        printS(string);
        nextLine();
        appendStr(string, append);
    }
    else
    {
        int i,c;
        for(i = string->length - 1, c = 0; i < string->length + appendLen - 2; i++, c++)
        {
            string->string[i] = append[c];
        }

        string->string[i] = append[c];
        string->length = string->length + appendLen - 1;
    }
}

void incrCapacity(String *str, int incr)
{
    if(incr == 0)
    {
        incr = INCREMENT_CAPACITY;
    }

    char *newstr = malloc((str->capacity + incr) * sizeof(char));
    duplicateStr(str->string, newstr);
    
    char *oldStr = str->string;
    str->string = newstr;
    str->capacity = str->capacity + incr;
    free(oldStr);
}

void duplicateStr(char *source, char *target)
{
    int srclen = strLen(source);
    for(int i = 0; i < srclen; i++)
    {
        target[i] = source[i];
    }
}

void destroyStr(String *str)
{
    free(str->string);
}

int compareStr(String str1, String str2)
{
    if(str1.length != str2.length)
    {
        return 1;
    }

    return memcmp(str1.string, str2.string, str1.length - 1);
}

int findPos(String str, char c)
{
    char *array = str.string;
    int len = str.length - 1;

    for(int i = 0; i < len; i++)
    {
        if(array[i] == c)
        {
            return i;
        }
    }
    
    return -1;
}

SArray split(String str, char c)
{
    SArray sarr = createSArray();
    int bcount = -1;
    for(int i = 0; i < str.length - 1; i++)
    {
        String s = getString();

        int len = strLen(str.string);
        printf("sl  : %d", len);
        nextLine();

        for(int j = 0; j < str.length - 1; j++)
        {

            printf("Base Count : %d", bcount);
            nextLine();

            if(bcount++ >= str.length - 1)
            {
                return sarr;
            }

            if((str.string[bcount] == c) || (bcount >= str.length - 1))
            {
                appendStr(&s, "\0");
                addString(&sarr, &s);
                break;
            }
            else
            {
                char *f = malloc(sizeof(char));
                f[0] = str.string[bcount];
                appendStr(&s, f);
            }
        }
    }
    
    return sarr;
}
