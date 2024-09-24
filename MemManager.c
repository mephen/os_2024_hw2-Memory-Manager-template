#include "MemManager.h"

bool getSysInfo(TLBPolicy *tlbPolicy, PagePolicy *pagePolicy, AllocPolicy *allocPolicy,
                int *numProc, int *virPage, int *phyPage)
{
    FILE *fsys;
    char *contents = NULL;
    size_t len = 0;

    fsys = fopen("sys_config.txt", "r");
    if (fsys == NULL)
    {
        printf("Error: sys_config.txt not found\n");
        return false;
    }
    for (int i = 0; i < 6; i++)
    {
        getline(&contents, &len, fsys);
        if (i != 5)
            // remove '\n' at the end of the line
            contents[strlen(contents) - 1] = '\0';
        switch (i)
        {
        case 0:
            if (!strcmp(contents, "TLB Replacement Policy: LRU")) //strcmp return 0 if two strings are equal
                *tlbPolicy = LRU;
            else if (!strcmp(contents, "TLB Replacement Policy: RANDOM"))
                *tlbPolicy = RANDOM;
            else
                return false;
            break;
        case 1:
            if (!strcmp(contents, "Page Replacement Policy: FIFO"))
                *pagePolicy = FIFO;
            else if (!strcmp(contents, "Page Replacement Policy: CLOCK"))
                *pagePolicy = CLOCK;
            else
                return false;
            break;
        case 2:
            if (!strcmp(contents, "Frame Allocation Policy: LOCAL"))
                *allocPolicy = LOCAL;
            else if (!strcmp(contents, "Frame Allocation Policy: GLOBAL"))
                *allocPolicy = GLOBAL;
            else
                return false;
            break;
        case 3:
            *numProc = atoi(&contents[20]);
            if (*numProc < 1 || *numProc > 20)
                return false;
            break;
        case 4:
            *virPage = atoi(&contents[23]);
            // check if virPage is power of 2
            if (!(ceil(log2(*virPage)) == floor(log2(*virPage)))) 
                return false;
            else if (*virPage < 2 || *virPage > 2048)
                return false;
            break;
        case 5:
            *phyPage = atoi(&contents[26]);
            if (!(ceil(log2(*phyPage)) == floor(log2(*phyPage))))
                return false;
            else if (*virPage < *phyPage)
                return false;
            else if (*phyPage < 1 || *phyPage > 1024)
                return false;
            break;
        }
    }
    fclose(fsys);

    return true;
}

void initialize(){
    // create page table, and set index for per process page table
    pageTable = malloc(numProc * virPage * sizeof(PageTableEntry));
    PerProcPTBR = malloc(numProc * sizeof(int));
    for (int i = 0; i < numProc; i++)
        PerProcPTBR[i] = i * virPage;
    for (int i = 0; i < numProc * virPage; i++)
    {
        pageTable[i].bitField.byte = 0;
        pageTable[i].pfn_dbi = -1; //demand paging
    }

    // create phyMem and swapSpace
    phyMem = malloc(phyPage * sizeof(PhyMem));
    swapSpace = malloc(numProc * virPage * sizeof(PhyMem));
    for (int i=0; i<phyPage; i++)
        phyMem[i].frameValid = true;
    for (int i=0; i<numProc * virPage; i++)
        swapSpace[i].frameValid = true;
    flushTLB();

    // init tlbLRU array
    if (tlbPolicy == LRU)
    {
        for (int i=0; i<32; i++)
            tlbLRU[i] = 0;
    }

    // create replacement list
    if (allocPolicy == LOCAL)
    {
        curLocalReplaceNode = malloc(numProc * sizeof(ReplaceListHeadType));
        for (int i=0; i<numProc; i++)
        {
            curLocalReplaceNode[i].head = malloc(sizeof(ReplaceListType));
            curLocalReplaceNode[i].head->next = curLocalReplaceNode[i].head->prev = NULL;
            curLocalReplaceNode[i].head->proc = i;
        }
    }
    else if (allocPolicy == GLOBAL)
    {
        curReplaceNode = malloc(sizeof(ReplaceListType));
        curReplaceNode->next = curReplaceNode->prev = NULL;
    }
    
    // create StatsType array
    stats = malloc(numProc * sizeof(StatsType));
    for (int i=0; i<numProc; i++)
        stats[i].pageFaultCnt = stats[i].refPageCnt = stats[i].refTlbCnt = stats[i].tlbHitCnt = 0;
}

void analysis(){
    FILE *fanalysis;
    fanalysis = fopen("analysis.txt", "w+");
    
    float hitRatio, formula, pageFaultRate;
    for (int i=0; i<numProc; i++)
    {
        hitRatio = stats[i].tlbHitCnt/stats[i].refTlbCnt;
        formula = (hitRatio * 120) + (1-hitRatio) * 220;
        pageFaultRate = stats[i].pageFaultCnt/stats[i].refPageCnt;
        fprintf(fanalysis, "Process %c, Effective Access Time = %f\n", i+C_CHAR_OFFSET, formula); //C_CHAR_OFFSET is ASCII 'A'
        fprintf(fanalysis, "Page Fault Rate: %1.3f\n", pageFaultRate);
    }
    fclose(fanalysis);
}

int main(){
    FILE *ftrace, *fanalysis;
    int PTBR, proc = -1, lastProc = -1;
    char *contents = NULL;
    size_t len = 0;
    uint16_t refPage;

    //get system information
    if (!getSysInfo(&tlbPolicy, &pagePolicy, &allocPolicy, &numProc, &virPage, &phyPage))
    {
        printf("abort: sys_config.txt occurs format errors\n");
        return 0;
    }

    //initialize environment
    initialize();

    //get reference information & handle each reference
    ftrace = fopen("trace.txt", "r");
    if (ftrace == NULL) {
        perror("Error opening trace.txt");
        exit(EXIT_FAILURE);
    }
    while (getline(&contents, &len, ftrace) != -1)
    {
        if (!(contents[strlen(contents) - 1] == ')'))
            contents[strlen(contents) - 1] = '\0';
        proc = contents[10];
        refPage = atoi(&contents[12]);
        /* TODO: 
         * 1. Implement a paging based memory manager with TLB support
         *      (1). It can manage physical frames for multiple processes, and don't forget to Flush TLB and change PTBR when switching process
         *      (2). It can handle TLB hit/miss & page hit/fault.
         *      (3). It should maintain the TLB, page table, replacement list, free frame list, physical memory, and swap space.
         *      (3). It should follow: LRU (TLB replacement policy), FIFO (Page replacement policy), and GLOBAL (frame allocation policy)
         * 2. Show the reference output in 'trace_output.txt'.
         * 3. Use StatsType to calculate 'Effective Access Time' and 'Page Fault Rate',show them in 'analysis.txt'.
         */
    }
    fclose(ftrace);

    analysis();

    //Free dynamically allocated memory
    free(pageTable);
    free(PerProcPTBR);
    free(phyMem);
    free(swapSpace);
    free(stats);
    if (allocPolicy == LOCAL)
    {
        for (int i=0; i<numProc; i++)
            free(curLocalReplaceNode[i].head);
        free(curLocalReplaceNode);
    }
    else if (allocPolicy == GLOBAL)
        free(curReplaceNode);

    return 0;
}