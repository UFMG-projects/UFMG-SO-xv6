# Multi-level Feedback Queue Scheduler for XV6

### Course project of Operating Systems

## Members:
- Gabriel Oliveira Sant'Ana
- Mariana Assis Ramos

## Introduction
The xv6 is an educational operating system based on Unix Version 6 (V6), developed by MIT, designed to teach operating system principles. It uses a round-robin (RR) scheduling policy to ensure fair CPU time distribution.
This project modifies the xv6 scheduler by implementing a multilevel priority queue with four levels, each using a different policy: round-robin (RR), lottery, shortest job first (SJF), and first come first served (FCFS). Additional modifications include auxiliary functions and starvation prevention mechanisms.

## Implementation
To implement a multilevel feedback queue scheduler in xv6, we created four distinct ready queues, each implementing a different scheduling policy. These queues were managed within the ptable structure, and an array count_queue was used to keep track of the number of processes in each queue. This approach simplifies queue manipulation and allows for efficient memory usage through the use of pointers.

When a process's state changes to ready (RUNNABLE), it is added to the appropriate priority queue, and the corresponding counter in count_queue is incremented. Conversely, when a process is scheduled (moves from RUNNABLE to RUNNING), it is removed from the queue, and the counter is decremented.

## Policies

**First Come First Served** (Priority Queue 4)
- FCFS schedules processes based on their arrival time, with the first process to arrive being the first to execute. This non-preemptive approach ensures processes run to completion once started.


**Shortest Job First** (Priority Queue 3)
- Queue Structure: Each process has additional attributes: estimatedburst (estimated CPU burst time), previousburst, and bstime (current burst time).
- Burst Time Calculation: The updateBurst function updates estimatedburst using exponential averaging, factoring in the most recent burst time.
Scheduling Logic: The process with the smallest estimatedburst is selected for execution.


**Round Robin** (Priority Queue 2)
- The original Round Robin implementation in xv6 was maintained for this priority queue.


**Lottery** (Priority Queue 1)
- Queue Structure: Each process in this queue is assigned a number of tickets, represented by a new attribute tickets in the proc structure, initialized to 10.
- Scheduling Logic: A total ticket count is computed by summing the tickets of all processes in the queue. A random ticket is selected using a random number generator function, and the process holding that ticket is chosen for execution.

## Starvation Prevention and Aging
In multilevel feedback queues, lower-priority processes can suffer from starvation.
To avoid that, we introduced an aging mechanism by adding the attribute "readyTimeAging" to the proc structure. This attribute tracks how long a process remains in the RUNNABLE state.
We also created the function updateClock() that increments "readyTimeAging" for all RUNNABLE processes each tick. If a process exceeds the maximum wait time (defined by _1TO2, _2TO3, _3TO4) then it moves to a higher priority queue.


