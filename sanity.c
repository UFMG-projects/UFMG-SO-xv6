//TP: TESTES
#include "types.h"
#include "user.h"

#define NUM_PROCESSOS 3

void executarCargaTipoProcesso(int tipo_processo){
    switch (tipo_processo) {
        case 0: //CPU-bound process (CPU)
            for (int a = 0; a < 100; a++) {
                for (int b = 0; b < 1000000; b++) {}
            }
            break;
        case 1: //S-CPU
            for (int a = 0; a < 20; a++) {
                for (int b = 0; b < 1000000; b++) {}
                yield();
            }
            break;
        case 2: //IO-bound process (IO)
            for (int a = 0; a < 100; a++) {
                sleep(1);
            }
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf(1, "Sanity recebe somente um parametro(número de processos)\n");
		exit();
    }

    int n = atoi(argv[1]);
    int pid, tipo_processo;
    //criar os 3*n processos
    for(int i = 0; i < NUM_PROCESSOS * n; i++) {
		pid = fork();
		if (pid == 0) {//child
            tipo_processo = getpid() % NUM_PROCESSOS; //calculo para atribuir um tipo ao processo
            executarCargaTipoProcesso(tipo_processo);
			exit(); //children exit here
		}
		continue; // pai continua a criar outros child
	}

    //estatisticas de cada processo
    int retime, rutime, stime;
    int cpu_retime = 0, cpu_rutime = 0, cpu_stime = 0, cpu_n = 0;
    int s_retime = 0, s_rutime = 0, s_stime = 0, s_n = 0;
    int io_retime = 0, io_rutime = 0, io_stime = 0, io_n = 0;
    int wait2_ok = 0;
    for(int i = 0; i < NUM_PROCESSOS * n; i++){
        wait2_ok = wait2(&retime, &rutime, &stime);
        if(wait2_ok != 0){
            printf(1, "Erro na Wait2, ao retornar estatisticas do processo\n");
		    exit();
        }
        pid = getpid();
        tipo_processo = pid % NUM_PROCESSOS; //calculo para atribuir um tipo ao processo
        switch(tipo_processo)
        {
            case 0: //CPU-bound process (CPU)
                printf(1, "pid: %d, CPU-Bound, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid, retime, rutime, stime);
                cpu_retime+=retime;
                cpu_rutime+=rutime;
                cpu_stime+=stime;
                cpu_n++;
                break;
            case 1: //S-CPU
                printf(1, "pid: %d, S-Bound, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid, retime, rutime, stime);
                s_retime+=retime;
                s_rutime+=rutime;
                s_stime+=stime;
                s_n++;
                break;  
            case 2: //IO-bound process (IO)
                printf(1, "pid: %d, IO-Bound, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid, retime, rutime, stime);
                io_retime+=retime;
                io_rutime+=rutime;
                io_stime+=stime;
                io_n++;
                break;
            default:
                break;
        }

    }

    //estatisticas agregadas
    printf(1, "\n\nCPU-Bound médias:\nREADY time: %d\nRUNNING time: %d\nSLEEPING time: %d\nTURNAROUND time: %d\n\n\n",
           cpu_retime / cpu_n, cpu_rutime / cpu_n, cpu_stime / cpu_n, (cpu_retime + cpu_rutime + cpu_stime) / cpu_n);
    printf(1, "S-Bound médias:\nREADY time: %d\nRUNNING time: %d\nSLEEPING  time: %d\nTURNAROUND time: %d\n\n\n",
           s_retime / s_n, s_rutime / s_n, s_stime / s_n, (s_retime + s_rutime + s_stime) / s_n);
    printf(1, "I/O-Bound médias:\nREADY time: %d\nRUNNING time: %d\nSLEEPING time: %d\nTURNAROUND time: %d\n\n\n",
           io_retime / io_n, io_rutime / io_n, io_stime / io_n, (io_retime + io_rutime + io_stime) / io_n);

    exit();
}