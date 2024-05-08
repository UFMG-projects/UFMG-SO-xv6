//TP: TESTES
#include "types.h"
#include "user.h"

#define NUM_PROCESSOS 20 // --------------------------------------------ALTERAR AQUI PARA TESTES

int main(int argc, char *argv[]){

    int pid;
    int prioridades[NUM_PROCESSOS] = {2,2,2,2,1,4,2,2,1,2,2,4,1,1,3,3,1,1,1,4}; // --------------------------------------------ALTERAR AS PRIORIDADES PARA TESTES
    //criar os processos
    for(int i = 0; i < NUM_PROCESSOS; i++) {
		pid = fork();
		if (pid == 0) {//child

            change_prio(prioridades[i]); //mudar prioridade manualmente

            for (int a = 0; a < 100; a++) {
                sleep(1);
            }
			exit(); //children exit here
		}
		continue; // pai continua a criar outros child
	}

    //estatisticas de cada processo
    int retime, rutime, stime;
    for(int i = 0; i < NUM_PROCESSOS; i++){
        wait2(&retime, &rutime, &stime);

        printf(1, "pid: %d, READY time: %d, RUNNING time: %d, SLEEPING time: %d\n", pid, retime, rutime, stime);
    }

    exit();
}