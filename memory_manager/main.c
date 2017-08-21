#include <stdio.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define BLOCK_HEAD_SIZE 3
#define SPACE_HEAD_SIZE 4
#define MIN_BLOCK_SIZE 4

char *space;

typedef struct header
{
    unsigned int size : 23;
    unsigned int is_free : 1;
}HEADER;

/**
* Spojí susedné voľné bloky
*/
void tryJointNext(char* block)
{
    const char *LAST_PTR = space + *((unsigned int *)space);
    HEADER *first = (HEADER*)block;
    HEADER *second = (HEADER*)(block + BLOCK_HEAD_SIZE + first->size);
    if((char*)second<LAST_PTR && second->is_free==TRUE)
    {
        first->size += second->size + BLOCK_HEAD_SIZE;
        second->size = 0;
        second->is_free = FALSE;
    }
}
/**
* Alokácia bloku pamäte
*/
void *memory_alloc(unsigned int size)
{
    const unsigned int NEED_SIZE = ((size > MIN_BLOCK_SIZE)? size: MIN_BLOCK_SIZE);
    const unsigned int SPACE_SIZE = *((unsigned int *)space);
    int rest = 0, offset = SPACE_HEAD_SIZE;
    HEADER *block = NULL;
    HEADER *next = NULL;

    while(offset<SPACE_SIZE)
    {
        block = (HEADER*)(space + offset);
                                                                                                        //printf("size = %d \n",block->size);
        if(block->is_free == TRUE)// blok je prazdny
        {
            tryJointNext((char*)block);
            if(block->size >= NEED_SIZE)// blok je dost velky
            {
                rest = block->size - NEED_SIZE - BLOCK_HEAD_SIZE;
                if(rest >= MIN_BLOCK_SIZE) // blok je prilis velky
                {
                    next = (HEADER*)(space + offset + NEED_SIZE + BLOCK_HEAD_SIZE);
                    next->size = rest;
                    block->size = NEED_SIZE;
                    next->is_free = TRUE;
                }
                block->is_free = FALSE;
                return (void*)(space + offset + BLOCK_HEAD_SIZE);
            }
        }
        offset += block->size + BLOCK_HEAD_SIZE;
    }
    return NULL;
}
/**
* Uvoľnenie bloku pamäte
*/
int memory_free(void *valid_ptr)
{
    char *p = (char*)valid_ptr;
    HEADER *block = (HEADER *) (p - BLOCK_HEAD_SIZE);
    block->is_free = TRUE;
    tryJointNext((char*)block);
    return 0;
}
/**
* Inicializácia premenných
*/
void memory_init(void *ptr, unsigned int size)
{
    //memset(ptr,0,size);
    space = ptr;
    unsigned int *space_size = (unsigned int *)space;
    *space_size = size;
    HEADER *first = (HEADER*)(space + SPACE_HEAD_SIZE);    //prvy blok (prazdny)
    first->size = (size - BLOCK_HEAD_SIZE - SPACE_HEAD_SIZE);
    first->is_free = TRUE;
}
/**
* Kontrola
*/
int memory_check(void *ptr)
{
    const unsigned int SPACE_SIZE = *((unsigned int *)space);
    int offset = SPACE_HEAD_SIZE;
    HEADER *block = NULL;

    while(offset<SPACE_SIZE)
    {
        block = (HEADER*)(space + offset);
        if((void*)(space + offset + BLOCK_HEAD_SIZE) == ptr)//ak pointer ukazuje na blok pamati
        {
            if(block->is_free == TRUE) return 0;
            else return 1;
        }
        offset += block->size + BLOCK_HEAD_SIZE;
    }
    return 0;
}
/**
* Prideľovanie nerovnakých blokov rôznych veľkostí, konrola obsahu a uvoľnovanie
*/
int test_3()
{
    const int COUNT=9;                              //COUNT pointerov
    char *p[COUNT];                                 //pole pointerov
    int size[]={1024,32,8,128,16,64,2048,512,256};  //pole dlzok pamate
    int before,i,j;
    int dimension=4088+(COUNT*BLOCK_HEAD_SIZE)+SPACE_HEAD_SIZE;//"max"velkost priestoru
    char memory[dimension];

    memory_init(memory,dimension);
    before=*((unsigned int*)space);       //velkost volnej pamate na zaciatku

    for(i = 0; i < COUNT; i++)
    {
        if((p[i] = (char*)memory_alloc(size[i]))==NULL)
        {
            return 1;
        }
        memset(p[i],i,size[i]);                 //zapis
    }
    for(i = 0; i < COUNT; i++)
    {
        for(j=0; j<size[i]; j++)
        {
            if(*((unsigned char*)p[i]+j)!=i)    //kontrola zapisu
            {
                return 1;
            }
        }
        if(memory_free(p[i]))
        {
            return 1;
        }
    }
    if(before!=*((unsigned int*)space))   //porovnanie velkosti volnej pamate na zaciatku a na konci
   {
        return 1;
   }
    return 0;
}
/**
* Prideľovanie rovnakých blokov veľkosti 8B, postupne uvolnovanie a znova pridelovanie
*/
int test_2()
{
    const int COUNT=10;         //COUNT pointerov
    char *p[COUNT];             //pole pointerov
    int i,j;
    int dimension = COUNT*(8+BLOCK_HEAD_SIZE)+SPACE_HEAD_SIZE;  //"max"velkost priestoru
    char memory[dimension];

    memory_init(memory,dimension);
    //alokacia 10 blokov
    for(i = 0; i < COUNT; i++)
    {
        if((p[i] = (char*)memory_alloc(8))==NULL)
            return 1;
        if(memory_check(p[i])==0)
            return 1;
    }
    //n=(1-10)
    for(i = 1; i <= COUNT; i++)
    {
        for(j = 0; j < i; j++)                          //uvolnenie n-blokov
        {
            if(memory_free(p[j]))
                return 1;
        }
        for(j = 0; j < i; j++)
        {
            if((p[j] = (char*)memory_alloc(8))==NULL)   //alokacia n-blokov
                return 1;
            if(memory_check(p[j])==0)
                return 1;
        }
    }

    return 0;
}
/**
* Alokacia 3 blokov a uvolnenie 1.+3.+2., alokacia 3 blokov a uvolnenie 3.+2.+1.
*/
int test_1()
{
    const int COUNT=4, SIMPLE_BLOCK_SIZE = 256;
    int i,dimension=3*(SIMPLE_BLOCK_SIZE+BLOCK_HEAD_SIZE)+SPACE_HEAD_SIZE;   //"max"velkost priestoru
    const int MULTI_BLOCK_SIZE = dimension-BLOCK_HEAD_SIZE-SPACE_HEAD_SIZE;
    char *p[COUNT];                 //pole pointerov na bloky pamate
    char memory[dimension];

    memory_init(memory,dimension);
//1-alokacia jedneho bloku celej velkosti
    if((p[0] = (char*)memory_alloc(MULTI_BLOCK_SIZE))==NULL)     //alloc 774
        return 1;
    if(memory_free(p[0]))                           //free  774
            return 1;
//2-alokacia troch blokov
    for(i=1; i<COUNT; i++)                          //3 krat
    {
        if((p[i] = (char*)memory_alloc(SIMPLE_BLOCK_SIZE))==NULL) //alloc 256
            return 1;
    }
    if(memory_free(p[1]) || memory_free(p[3]) || memory_free(p[2]))  //free  256
        return 1;
//3-alokacia troch blokov
    for(i=1; i<COUNT; i++)                          //3 krat
    {
        if((p[i] = (char*)memory_alloc(SIMPLE_BLOCK_SIZE))==NULL) //alloc 256
            return 1;
    }
    if(memory_free(p[3]) || memory_free(p[1]) || memory_free(p[2]))  //free  256
        return 1;

//4-alokacia jedneho bloku celej velkosti
    if((p[0] = (char*)memory_alloc(MULTI_BLOCK_SIZE))==NULL)     //alloc 774
        return 1;
    if(memory_free(p[0]))                           //free  774
            return 1;

    return 0;
}
/**
* Vrati 0 ak testy prebehli OK, inak vrati 1
*/
int memory_test()
{
    const int COUNT = 3;                            //COUNT testovacich funkcii
    int i;
    int (*test[])(void) = {test_1,test_2,test_3};   //pole pointerov na testovacie funkcie

    for(i=0;i<COUNT;++i)
    {
        //printf("%d.: %d\n",i, (*test[i])() );
        if( (*test[i])() )
        {
            return 1;
        }
    }
    return 0;
}
/**
* testovanie casu behu porogramu
*/
void timeTester(int COUNT)
{
    const int SIZE_BLOCK=8;
    int i,size = ( (SIZE_BLOCK+BLOCK_HEAD_SIZE) * COUNT ) + BLOCK_HEAD_SIZE;
    char *p[COUNT];
    char memory[size];
    printf("%.3lf\n",(double)clock()/ 1000.0);            //zaciatok init
    memory_init(memory,size);
    printf("%.3lf\n",(double)clock()/ 1000.0);            //zaciatok alloc
    for(i=0; i<COUNT; i++)
        if((p[i] = (char*)memory_alloc(SIZE_BLOCK))==NULL)
            printf("chyba %d. alokacie",i);
    printf("%.3lf\n",(double)clock()/ 1000.0);             //zaciatok free
    for(i=COUNT-1; i>=0; i--)
        if(memory_free(p[i]))
            printf("chyba %d. uvolnenia",i);
    printf("%.3lf\n",(double)clock()/ 1000.0);             //koniec testu

}
/**
* Hlavá metóda
*/
int main()
{
   if (memory_test())
        printf("Implementacia je chybna\n");
    else
        printf("Implementacia je OK\n");

    /*int i;
    for(i=1000; i<=81000; i+=20000)
        timeTester(i);*/

  return 0;
}
