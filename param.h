#define NPROC        640 //64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks

//TP: INTERV
#define INTERV        1
//TP: PRIORIDADE
#define NUMFILAS 4 //uma só pro init
#define PRIO 1 //prioridade padrão = 2 ou seja 1 -> [0,(1),2,3]
//TP: AGING
#define _1TO2 10 
#define _2TO3 10
#define _3TO4 20

/*
    Algoritmos atuais
    0 -> RR
    1 -> RR
    2 -> LOTERIA
    3 -> FCFS

*/