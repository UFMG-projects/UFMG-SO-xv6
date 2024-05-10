//TP: TESTES
#include "types.h"
#include "user.h"

#define NUM_PROCESSOS 200 // --------------------------------------------ALTERAR AQUI PARA TESTES

int main(int argc, char *argv[]){

    int pid;
    int prioridades[NUM_PROCESSOS]; // --------------------------------------------ALTERAR AS PRIORIDADES PARA TESTES
    
    for (int i = 0; i < NUM_PROCESSOS; i++)
    {
        prioridades[i] = i%3;
    }
    
    
    //criar os processos
    for(int i = 0; i < NUM_PROCESSOS; i++) {
		pid = fork();
		if (pid == 0) {//child
 
            change_prio(prioridades[i]); //mudar prioridade manualmente

            int i = 0;
            for (int a = 0; a < 100; a++) {
                i = a + a;
                if(i == -1)
                    continue;
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