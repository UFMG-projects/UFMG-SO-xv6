//TP: TESTES
#include "types.h"
#include "user.h"

#define NUM_PROCESSOS 3

int main(int argc, char *argv[]){
    int prioridades[50] = {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1}; // --------------------------------------------ALTERAR AS PRIORIDADES PARA TESTES

    int n = atoi(argv[1]);
    int j = 0;
    int pid, tipo_processo;
    //criar os 3*n processos
    for(int i = 0; i < NUM_PROCESSOS * n; i++) {
		pid = fork();
		if (pid == 0) {//child
            tipo_processo = getpid() % NUM_PROCESSOS; //calculo para atribuir um tipo ao processo

            change_prio(prioridades[j]); //mudar prioridade manualmente
            if(tipo_processo == 0){ //CPU
                for (int i = 0; i < 100; i++){
                    for (volatile int j = 0; j < 1000000; j++)
                        ;
                }
            }
            else if(tipo_processo == 1){ //S-CPU
                for (int i = 0; i < 20; i++){
                    for (volatile int j = 0; j < 1000000; j++)
                        ;
                    yield();
                }
            }
            else if(tipo_processo == 2){ //IO
                for (int i = 0; i < 100; i++){
                    sleep(1);
                }
            }
            
			exit(); //children exit here
		}
        if(i%3 == 0 && 1 != 0)
            j++;
		continue; // pai continua a criar outros child
	}

    //estatisticas de cada processo
    int retime, rutime, stime;
    int cpu_retime = 0, cpu_rutime = 0, cpu_stime = 0, cpu_n = 0;
    int s_retime = 0, s_rutime = 0, s_stime = 0, s_n = 0;
    int io_retime = 0, io_rutime = 0, io_stime = 0, io_n = 0;
    int pid_child = 0;
    for(int i = 0; i < NUM_PROCESSOS * n; i++){
        pid_child = wait2(&retime, &rutime, &stime);
        tipo_processo = pid_child % NUM_PROCESSOS; //calculo para atribuir um tipo ao processo
        if(tipo_processo == 0){ //CPU
            printf(1, "pid: %d, CPU-Bound, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid_child, retime, rutime, stime);
            cpu_retime+=retime;
            cpu_rutime+=rutime;
            cpu_stime+=stime;
            cpu_n++;
        }
        else if(tipo_processo == 1){ //S-CPU
            printf(1, "pid: %d, S-Bound, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid_child, retime, rutime, stime);
            s_retime+=retime;
            s_rutime+=rutime;
            s_stime+=stime;
            s_n++;
        }
        else if(tipo_processo == 2){ //IO
            printf(1, "pid: %d, IO-Bound, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid_child, retime, rutime, stime);
            io_retime+=retime;
            io_rutime+=rutime;
            io_stime+=stime;
            io_n++;
        }

    }

    //estatisticas agregadas
    printf(1, "\n\nCPU-Bound medias:\nREADY time: %d\nRUNNING time: %d\nSLEEPING time: %d\nTURNAROUND time: %d\n\n\n",
           cpu_retime / cpu_n, cpu_rutime / cpu_n, cpu_stime / cpu_n, (cpu_retime + cpu_rutime + cpu_stime) / cpu_n);
    printf(1, "S-Bound medias:\nREADY time: %d\nRUNNING time: %d\nSLEEPING  time: %d\nTURNAROUND time: %d\n\n\n",
           s_retime / s_n, s_rutime / s_n, s_stime / s_n, (s_retime + s_rutime + s_stime) / s_n);
    printf(1, "I/O-Bound medias:\nREADY time: %d\nRUNNING time: %d\nSLEEPING time: %d\nTURNAROUND time: %d\n\n\n",
           io_retime / io_n, io_rutime / io_n, io_stime / io_n, (io_retime + io_rutime + io_stime) / io_n);

    exit();
}