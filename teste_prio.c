//MUDANÇAS TP
//TP: TESTES
#include "types.h"
#include "user.h"

#define NUM_PROCESSOS 20

int main(int argc, char *argv[]){

    int pid;
    //criar os processos CPU-BOUND
    for(int i = 0; i < NUM_PROCESSOS; i++) {
		pid = fork();
		if (pid == 0) {//child
            //CPU-bound process (CPU)
            for (int a = 0; a < 100; a++) {
                for (int b = 0; b < 1000000; b++) {}
            }
			exit(); //children exit here
		}
		continue; // pai continua a criar outros child
	}

    //estatisticas de cada processo
    int retime, rutime, stime;
    int cpu_retime = 0, cpu_rutime = 0, cpu_stime = 0, cpu_n = 0;
    for(int i = 0; i < NUM_PROCESSOS; i++){
        int wait2_ok = wait2(&retime, &rutime, &stime);
        if(wait2_ok != 0){
            printf(1, "Erro na Wait2, ao retornar estatisticas do processo\n");
		    exit();
        }

        printf(1, "pid: %d, CPU-Bound, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid, retime, rutime, stime);
        cpu_retime+=retime;
        cpu_rutime+=rutime;
        cpu_stime+=stime;
        cpu_n++;

    }

    //estatisticas agregadas
    printf(1, "\n\nCPU-Bound médias:\nREADY time: %d\nRUNNING time: %d\nSLEEPING time: %d\nTURNAROUND time: %d\n\n\n",
           cpu_retime / cpu_n, cpu_rutime / cpu_n, cpu_stime / cpu_n, (cpu_retime + cpu_rutime + cpu_stime) / cpu_n);

    exit();
}