#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

//PTBR：page-table base register, record the start index of process page table
#define C_CHAR_OFFSET 65 //ASCII 'A'
#define PTBR_TO_PROCESS_INDEX (PTBR/virPage)
#define PTBR_TO_PROCESS_NAME ((PTBR_TO_INDEX) + 65)

typedef enum
{
    LRU,
    RANDOM
} TLBPolicy;

typedef enum
{
    FIFO,
    CLOCK
} PagePolicy;

typedef enum
{
    LOCAL,
    GLOBAL
} AllocPolicy;

typedef struct
{
    bool valid;
    uint16_t vpn;
    uint16_t pfn;
} TLBEntry;
TLBEntry tlbEntry[32]; //fixed size of 32

typedef struct
{
    union
    {
        uint8_t byte; //fast initialization for bitField
        struct
        {
            uint8_t reference : 1; //take 1 bit in 8 bits, at the lowest bit(bit 0)
            uint8_t present : 1; //at the 2nd bit(bit 1)
        } bits;
    } bitField;
    int16_t pfn_dbi; //physical frame number/disk block number; When a frame is moved to disk block K, the pfn_dbi will be set as K
} PageTableEntry;

typedef struct ReplaceListType
{
    uint8_t proc;
    uint16_t vpn;
    struct ReplaceListType *prev;
    struct ReplaceListType *next;
} ReplaceListType;
ReplaceListType *curReplaceNode;

typedef struct
{
    struct ReplaceListType *head;
} ReplaceListHeadType;
ReplaceListHeadType *curLocalReplaceNode; //store the ReplaceList of each process

typedef struct
{
    bool frameValid;
} PhyMem;
PhyMem *phyMem;
PhyMem *swapSpace; //store the swapped out frame

typedef struct
{
    float tlbHitCnt;
    float refTlbCnt;
    float pageFaultCnt;
    float refPageCnt;
} StatsType; //for analysis
StatsType *stats;

PageTableEntry *pageTable;
int *PerProcPTBR; //store the start index of page table for each process
//system info
AllocPolicy allocPolicy;
TLBPolicy tlbPolicy;
PagePolicy pagePolicy;
int phyPage, virPage, numProc;
uint32_t tlbCounter=0;
uint32_t tlbLRU[32]; //record the used order of tlbEntry (by tlbCounter)

bool getSysInfo(TLBPolicy *tlbPolicy, PagePolicy *pagePolicy, AllocPolicy *allocPolicy,
                int *numProc, int *virPage, int *phyPage);
void analysis();
