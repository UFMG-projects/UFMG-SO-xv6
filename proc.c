#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  //TP: PRIORIDADE
  struct proc* queue_ready[NUMFILAS][NPROC];
  int count_queue[NUMFILAS]; //numero de processos em cada fila de prioridade
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  //TP: PRIORIDADE
  ptable.count_queue[0] = 0; 
  ptable.count_queue[1] = 0; //init começa nela
  ptable.count_queue[2] = 0; 
  ptable.count_queue[3] = 0;
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  //TP: TESTES
  p->ctime = ticks;
  p->stime = 0;
  p->retime = 0;
  p->rutime = 0;

  //TP:LOTERIA
  p->tickets = 10;

  //TP: SJF
  p->estimatedburst = 2;
  p->previousburst = 0;
  p->bstime = 0;

  //TP: AGING
  p->readyTimeAging = 0;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  //TP: TESTES
  p->ctime = ticks;
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  //TP: PRIORIDADE
  p->priority = PRIO;
  //adicionando o processo na fila de prioridade correta
  ptable.queue_ready[PRIO][ptable.count_queue[PRIO]] = p;
  ptable.count_queue[PRIO]++;
  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  //TP: PRIORIDADE
  np->priority = PRIO;
  //adicionando o processo na fila de prioridade correta
  ptable.queue_ready[PRIO][ptable.count_queue[PRIO]] = np; //adicionando na fila
  ptable.count_queue[PRIO]++; //aumentar num proc
  
  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        //TP: TESTES
        p->ctime = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
/*
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }

    release(&ptable.lock);

  }
}
*/

//calcula o burst esperado para a próxima execução do processo
void updateBurst(struct proc *p){
  p->estimatedburst = (0.5 * p->estimatedburst) + (0.5 * p->previousburst);
}


//percorre todos os processos da fila e retorna a posição do processo com menor burst esperado
int shortestBurst(){

  struct proc *p;
  int shortest = 10000; // inicializa com qualquer numero grande pra comparar com o tempo dos processos
  int positionShortest = 0; // posição do processo com menor burst

  for(int i = 0; i < ptable.count_queue[2]; i++){
    p = ptable.queue_ready[2][i];
    // compara e atualiza o processo com menor burst
    if(p->estimatedburst < shortest){
      shortest = p->estimatedburst;
      positionShortest = i;
    }
  }
return positionShortest;
}

int Random (){ 
  static int next = 3251 ; 
  next = ((next * next) / 100 ) % 10000 ; 
  return next ; 
} 
 
int RandomInRange (int min, int max ){ 
  return Random() % (max+1-min) + min ;  
} 

int loteria_Total(void){
  struct proc *p;
  int tickets_totais=0;

  //percorre os processos da fila e soma a quantidade de tickets de cada processo
  for(int i = 0; i < ptable.count_queue[0]; i++){  
    p = ptable.queue_ready[0][i];
    if(p->state != RUNNABLE)
        continue;

    tickets_totais += p->tickets; 
  } 
  return tickets_totais;          // retorna total de tickets com os processos da fila 
}

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  // TP: LOTERIA
  int count = 0;
  long golden_ticket = 0;
  int total_tickets = 0;
  int aux = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();
    
    acquire(&ptable.lock);

    if(ptable.count_queue[3] > 0){ //--------------- FIRST-COME-FIRST-SERVED [FCFS]
      p = ptable.queue_ready[3][0]; 
      
      //TP: INTERV
      p->clock  = 0;
      //TP: AGING
      p->readyTimeAging = 0;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);

      //TP: PRIORIDADE
      for(int i = 0; i < ptable.count_queue[3] - 1; i++){
        ptable.queue_ready[3][i] = ptable.queue_ready[3][i+1]; //andar com a fila
      }
      ptable.count_queue[3]--; //diminuir num proc

      p->state = RUNNING;
      // cprintf("FCFS: %d\n",p->pid);  

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    else if(ptable.count_queue[2] > 0){ //--------------- SJF

      int menorBurstPosicao = shortestBurst(); //  retorna a posição do processo com menor tempo de burst esperado

      p = ptable.queue_ready[2][menorBurstPosicao]; //pega o respectivo processo

      //TP: INTERV
      p->clock  = 0;
      //TP: AGING
      p->readyTimeAging = 0;
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);

      //TP: PRIORIDADE
      for(int i = menorBurstPosicao; i < ptable.count_queue[2] - 1; i++){
        ptable.queue_ready[2][i] = ptable.queue_ready[2][i+1]; //andar com a fila
      }
      ptable.count_queue[2]--; //diminuir num proc

      p->state = RUNNING;
      // cprintf("SJF: %d\n",p->pid);  

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
      
    }
    else if(ptable.count_queue[1] > 0){ //--------------- ROUND ROBIN

      p = ptable.queue_ready[1][0]; 
      
      //TP: INTERV
      p->clock  = 0;
      //TP: AGING
      p->readyTimeAging = 0;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);

      //TP: PRIORIDADE
      for(int i = 0; i < ptable.count_queue[1] - 1; i++){
        ptable.queue_ready[1][i] = ptable.queue_ready[1][i+1]; //andar com a fila
      }
      ptable.count_queue[1]--; //diminuir num proc

      p->state = RUNNING;
      // cprintf("RR: %d\n",p->pid);  

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    else if(ptable.count_queue[0] > 0){ //--------------- LOTERIA
      golden_ticket = 0;
      count = 0;
      aux = 0;
      total_tickets = 0;

      total_tickets = loteria_Total();                    //conta os tickets totais
      golden_ticket = RandomInRange(0,total_tickets);     //escolhe um ticket aleatório
      
 
      for(aux = 0; aux < ptable.count_queue[0]; aux++){
        
        p = ptable.queue_ready[0][aux];  

        //encontra o processo com o ticket sorteado
        if ((count + p->tickets) < golden_ticket){
          count += p->tickets;
          continue;
        }
        break; 
      } 

      // executa o processo

      p->clock  = 0;
      //TP: AGING
      p->readyTimeAging = 0;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);

      //TP: PRIORIDADE
      for(int j = aux; j < ptable.count_queue[0] - 1; j++){          
        ptable.queue_ready[0][j] = ptable.queue_ready[0][j+1]; //andar com a fila
      }
      ptable.count_queue[2]--; //diminuir num proc

      p->state = RUNNING;
      // cprintf("LOTERIA: %d\n",p->pid);  

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock

  //TP: PRIORIDADE
  int prioridade = myproc()->priority; //não muda a prioridade
  //adicionando o processo na fila de prioridade correta
  ptable.queue_ready[prioridade][ptable.count_queue[prioridade]] = myproc(); //adicionando na fila
  ptable.count_queue[prioridade]++; //aumentar num proc

  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  
  // TP - SJF
  p->previousburst = p->bstime;
  updateBurst(p);

  p->chan = chan;
  p->state = SLEEPING;
  p->bstime = 0;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == SLEEPING && p->chan == chan){

      //TP: PRIORIDADE
      int prioridade = p->priority; //não muda a prioridade
      //adicionando o processo na fila de prioridade correta
      ptable.queue_ready[prioridade][ptable.count_queue[prioridade]] = p; //adicionando na fila
      ptable.count_queue[prioridade]++; //aumentar num proc
      
      p->state = RUNNABLE;
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING){

        //TP: PRIORIDADE
        int prioridade = p->priority; //não muda a prioridade
        //adicionando o processo na fila de prioridade correta
        ptable.queue_ready[prioridade][ptable.count_queue[prioridade]] = p; //adicionando na fila
        ptable.count_queue[prioridade]++; //aumentar num proc

        p->state = RUNNABLE; 
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

//TP: TESTES
/*
 * Função wait2 espera que um processo filho termine e retorna algumas estatísticas sobre esse processo.
 *
 * Entradas:
 *   - retime: Ponteiro para um inteiro que armazenará o tempo de espera (READY) do processo filho.
 *   - rutime: Ponteiro para um inteiro que armazenará o tempo de execução na CPU (RUNNING) do processo filho.
 *   - stime: Ponteiro para um inteiro que armazenará o tempo de dormir (SLEEP) do processo filho.
 *
 * Saída:
 *   - Retorna 0 em caso de sucesso ou -1 se nenhum processo filho estiver disponível para esperar.
 */
// OBS: SYSTEM CALL
int wait2(int* retime, int* rutime, int* stime){
  struct proc *p;
  int havekids;

  acquire(&ptable.lock);
  for(;;){
    //procurando zombie child -> processo filho que terminou
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      
      if(p->parent != myproc()) //child
        continue;
      havekids = 1;

      if(p->state == ZOMBIE){ //child e zombie
        //atualizar tempo do encontrado
        int pid = p->pid;
        *retime = p->retime;
        *rutime = p->rutime;
        *stime = p->stime;

        //liberar espaço
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->ctime = 0;
        p->retime = 0;
        p->rutime = 0;
        p->stime = 0;
        
        release(&ptable.lock);
        return pid; //retorna PID se deu certo
      }
    }

    //não tem nenhuma child ou foi finalizado
    if(!havekids || myproc()->killed){
      release(&ptable.lock);
      return -1; //retorna -1 se NÃO deu certo
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(myproc(), &ptable.lock);  //DOC: wait-sleep
  }
}

//TP: TESTES
/*
 * Função updateClock atualiza o relógio do sistema e as estatísticas de tempo de execução de cada processo.
 */
void updateClock() {
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == SLEEPING){
      p->stime++;
      p->clock = 0;
    }
    else if(p->state == RUNNABLE){
      p->retime++;
      p->readyTimeAging++;
      p->clock = 0;
    }
    else if(p->state == RUNNING){
      p->bstime++;
      p->rutime++;
      p->clock++;
    }
  }
  release(&ptable.lock);
}

void auxTrocarFilaPriority(struct proc *p, int prioridade_nova){
  int prioridade_antiga = prioridade_nova-1;
  int posicao_do_p = 0;
  //achar processo na fila antiga
  for(int i = 0; i < ptable.count_queue[prioridade_antiga]; i++){
    if(p->pid == ptable.queue_ready[prioridade_antiga][i]->pid)
      posicao_do_p = i;
  }
  
  //tirar processo e andar com fila antiga
  for(int i = posicao_do_p; i < ptable.count_queue[prioridade_antiga] - 1; i++){
    ptable.queue_ready[prioridade_antiga][i] = ptable.queue_ready[prioridade_antiga][i+1]; 
  }
  ptable.count_queue[prioridade_antiga]--; //diminuir num proc

  //colocar processo na nova fila
  ptable.queue_ready[prioridade_nova][ptable.count_queue[prioridade_nova]] = p; //adicionando na fila
  ptable.count_queue[prioridade_nova]++; //aumentar num proc
  
  //zerou o tempo de espera
  p->readyTimeAging = 0;;
}

//TP: AGING
/*
 * Esta função é responsável por prevenir a inanição (starvation) ao implementar um mecanismo de envelhecimento (aging). 
 */
void upgradePriority_Aging(){
  struct proc *p;
  
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    //checar qual fila
    if(p->state == 3 && p->priority == 0 && p->readyTimeAging >= _1TO2){ //[0] -> [1]
      p->priority++;
      auxTrocarFilaPriority(p,1);
    }
    else if(p->state == 3 && p->priority == 1 && p->readyTimeAging >= _2TO3){ //[1] -> [2]
      p->priority++;
      auxTrocarFilaPriority(p,2);
    }
    else if(p->state == 3 && p->priority == 2 && p->readyTimeAging >= _3TO4){ //[2] -> [3]
      p->priority++;
      auxTrocarFilaPriority(p,3);
    }
  }
  release(&ptable.lock);
}

//TP: PRIORIDADE
/*
* Função change_prio muda a prioridade do processo que fez a chamada, caso esteja em runnable reajusta a fila de prontos adicionando o processo
*/
int change_prio(int priority){
  //erro prevenir do usuário
  if(priority < 1 || priority > 4){
    cprintf("\nWarning: prioridade não alterada(change_prio)\nPrioridades aceitas: [1,2,3,4]. Prioridade requisitada para troca foi: %d.\n\n", priority);
    return -1;
  }

  //mudar a prioridade
  myproc()->priority = priority-1;
  yield();
  
  // se RUNNABLE, arrumar a fila de pronto
  if(myproc()->state == 3){
    auxTrocarFilaPriority(myproc(),priority-1);
  }

  return 0;
}