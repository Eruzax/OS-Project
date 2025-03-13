#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

typedef enum {ARRIVE, READY, RUNNING, WAITING, TERMINATED} State;

typedef struct {
    char* pid;
    int arrivalTime;
    int endTime;
    int numBursts;
    int burstsLeft;
    int* cpuBursts;
    int* ioBursts;
    int* remainingBursts;
    State state;
    int tau;
} Process;

typedef struct {
    Process* process;
    int time;
    State state;
} Event;

// Create a new event
Event* createEvent(Process* p, int time, State s){
    Event* newEvent = malloc(sizeof(Event*));
    newEvent->process = p;
    newEvent->time = time;
    newEvent->state = s;
    return newEvent;
}

typedef struct {
    Event** events;
    int size;
    int capacity;
} EventQueue;

// Initialize the event queue
void initEventQueue(EventQueue* q, int capacity) {
    q->events = calloc(capacity, sizeof(Event));
    q->size = 0;
    q->capacity = capacity;
}

// Insert an event into the event queue
void insertEvent(EventQueue* q, Event* event) {
    if (q->size >= q->capacity) {
        fprintf(stderr, "ERROR: Event queue is full, cannot insert event\n");
        return;
    }

    // Find the correct position
    int i = q->size - 1;
    while (i >= 0 && q->events[i]->time > event->time) {
        q->events[i + 1] = q->events[i];
        i--;
    }

    // Insert the event at the found position
    q->events[i + 1] = event;
    q->size++;
}

// Remove the element at the top of event queue
Event* popEvent(EventQueue* q) {
    if (q->size == 0) return NULL;
    Event* event = q->events[0];
    q->size--;
    for (int i = 0; i < q->size; i++){
        q->events[0] = q->events[i+1];
    }
    return event;
}

// Queue DS
typedef struct {
    Process **procs;
    int size;
    int capacity;
} Queue;

// Initialize queue
void initQueue(Queue *q, int capacity) {
    q->procs = calloc(capacity + 1, sizeof(Process *));
    if (q->procs == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed for queue\n");
        return;
    }
    q->size = 0;
    q->capacity = capacity;
}

// Add to queue
void enqueue(Queue *q, Process *p) {
    if (q->size < q->capacity) {
        *(q->procs + q->size) = p;
        q->size++;
    } else {
        fprintf(stderr, "ERROR: Queue is full, cannot enqueue %s\n", p->pid);
        return;
    }
}

Process* popQueue(Queue *q) {
    if (q->size == 0) return NULL;
    Process *p = *(q->procs);
    // Shift elements left
    Process **cur = q->procs;
    for (int i = 0; i < q->size - 1; i++) {
        *(cur + i) = *(cur + i + 1);
    }
    q->size--;
    return p;
}

void printQueue(Queue *q) {
    if (q->size == 0) {
        printf(" empty");
    } else {
        Process **cur = q->procs;
        for (int i = 0; i < q->size; i++) {
            printf(" %s", (*(cur + i))->pid);
        }
    }
}

// Doesnt Work
// void FCFS(Process** processes, int n, int tcs) {
//     // Set all processes to NEW and initialize burstsLeft
//     for (int i = 0; i < n; i++) {
//         (*(processes+i))->state = NEW;
//         (*(processes+i))->burstsLeft = (*(processes+i))->numBursts;
//     }

//     // Start Simultaion
//     Queue q;
//     initQueue(&q, n);
//     EventQueue eq;
//     initEventQueue(&eq, n);
//     printf("time 0ms: Simulator started for FCFS [Q empty]\n");

//     // Arrivals
//     for (int i = 0; i < n; i++){
//         Event* newEvent = createEvent(*(processes+i), (*(processes+i))->arrivalTime, NEW);
//         insertEvent(&eq, newEvent);
//     }

//     // Simulation
//     int cpuIdle = -1;
//     int timeFree = 0;
//     int time = 0;
//     int terminatedCount = 0;
//     while (terminatedCount < n) {
//         // Handle Events
//         Event* e = popEvent(&eq);
//         // Arrival
//         if (e->state == NEW){
//             enqueue(&q, e->process);
//             printf("time %dms: Process %s arrived; added to ready queue [Q", e->process->arrivalTime, e->process->pid);
//             printQueue(&q);
//             printf("]\n");
//             // No process running
//             if (cpuIdle == -1){
//                 Event* startCpu = createEvent(e->process, e->process->arrivalTime + tcs/2, START_CPU);
//                 insertEvent(&eq, startCpu);
//             } else {
//                 Event* startCpu = createEvent(e->process, timeFree + tcs, START_CPU);
//                 insertEvent(&eq, startCpu);
//             }
//             free(e);
//         }
//         // Start CPU Burst
//         else if (e->state == START_CPU && cpuIdle == 1){
//             cpuIdle = 0;
//             dequeue(&q);
//             int burstTime = *(e->process->cpuBursts+(e->process->numBursts - e->process->burstsLeft));
//             printf("time %dms: Process %s started using the CPU for %dms burst [Q", e->time, e->process->pid, burstTime);
//             printQueue(&q);
//             printf("]\n");
//             e->process->burstsLeft--;
//             // Last CPU burst
//             if (e->process->burstsLeft == 0){
//                 Event* endCpu = createEvent(e->process, e->time + burstTime, TERMINATED);
//                 insertEvent(&eq, endCpu);
//             } 
//             // Add finish CPU burst to event queue
//             else{
//                 Event* endCpu = createEvent(e->process, e->time + burstTime, FINISH_CPU);
//                 insertEvent(&eq, endCpu);
//             }
//             timeFree = e->time + burstTime;
//             free(e);
//             terminatedCount = n;
//         }
//         // CPU End
//         else if(e->state == FINISH_CPU){
//             cpuIdle = -1;
//             printf("time %dms: Process %s completed a CPU burst; %d bursts to go [Q", e->time, e->process->pid, e->process->burstsLeft);
//             printQueue(&q);
//             printf("]\n");

//             // IO Burst
//             int ioCompTime = e->time + *(e->process->ioBursts+(e->process->numBursts - e->process->burstsLeft - 1)) + tcs/2;
//             printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms [Q", e->time, e->process->pid, ioCompTime);
//             printQueue(&q);
//             printf("]\n");
//             Event* ioBurst = createEvent(e->process, ioCompTime, FINISH_IO);
//             insertEvent(&eq, ioBurst);
//             free(e);
//             terminatedCount = n;
//         }
//         // IO End
//         else if (e->state == FINISH_IO){
//             enqueue(&q, e->process);
//             printf("time %dms: Process %s completed I/O; added to ready queue [Q", e->time, e->process->pid);
//             printQueue(&q);
//             printf("]\n");
//             int burstTime = *(e->process->cpuBursts+(e->process->numBursts - e->process->burstsLeft));
//             Event* cpuBurst = createEvent(e->process, burstTime, FINISH_IO);
//             insertEvent(&eq, cpuBurst);
//             free(e);
//             terminatedCount = n;
//         }
//         // Termination
//         else if (e->state == TERMINATED){
//             printf("time %dms: Process %s terminated [Q ", e->time, e->process->pid);
//             printQueue(&q);
//             printf("]\n");
//             terminatedCount++;
//             free(e);
//             time = e->time;
//         }
//     }
//     printf("time %dms: Simulator ended for FCFS [Q empty]\n", time);

//     free(q.procs);
// }

void SJF(){
}

void SRT(){

}

void RR(Process** processes, int n, int tcs, int tslice){
// Set all processes to ARRIVE and initialize burstsLeft
    for (int i = 0; i < n; i++) {
        (*(processes+i))->state = ARRIVE;
        (*(processes+i))->burstsLeft = (*(processes+i))->numBursts;
        (*(processes+i))->remainingBursts = (*(processes+i))->cpuBursts;
    }

    // Start Simultaion
    Queue q;
    initQueue(&q, n);
    EventQueue eq;
    initEventQueue(&eq, n);
    printf("time 0ms: Simulator started for RR [Q empty]\n");

    // Arrivals
    for (int i = 0; i < n; i++){
        Event* newEvent = createEvent(*(processes+i), (*(processes+i))->arrivalTime, ARRIVE);
        insertEvent(&eq, newEvent);
    }
    int time = 0;
    int terminatedCount = 0;
    int cpuFreeAt = -1;
    int cpuIdle = -1;
    // int csCount = 0;
    while (terminatedCount < n) {
        Event *e = popEvent(&eq);
        time = e->time;
        // Arrival
        if (e->state == ARRIVE){
            // Print and add to queue
            enqueue(&q, e->process);
            printf("time %dms: Process %s arrived; added to ready queue [Q", time, e->process->pid);
            printQueue(&q);
            printf("]\n");
            // Create Event for CPU Burst 
            if (q.size == 1 && cpuIdle == -1 && time >= cpuFreeAt){
                popQueue(&q);
                Event* newEvent = (e->process, time + tcs/2, RUNNING);
                cpuIdle = 0;
            }
        } else if (e->state = RUNNING) {

        } 
        else if (e->state == TERMINATED){
            terminatedCount++;
        }
    }
}

double nextExp(double lambda, double upperBound){
    double r = drand48();
    double x = -log(r) / lambda;
    while (x > upperBound){
        r = drand48();
        x = -log(r) / lambda;
    }
    return x;
}

int main(int argc, char** argv){
    if (argc < 9){
        perror("ERROR: Invalid argument(s)");
        return EXIT_FAILURE;
    }

    // Get parameters
    int n = atoi(*(argv+1));
    int ncpu = atoi(*(argv+2));
    int seed = atoi(*(argv+3));
    double lambda = atof(*(argv+4));
    if (lambda <=0){
        perror("ERROR: Lambda <= 0");
        return EXIT_FAILURE;
    }
    int upperBound = atoi(*(argv+5));
    int tcs = atoi(*(argv+6));
    if (tcs < 0){
        perror("ERROR: Negative context switch");
        return EXIT_FAILURE;
    }
    double alpha = atof(*(argv+7));
    if (alpha > 1 || alpha < 0){
        perror("ERROR: Alpha is not in the range 0 to 1");
        return EXIT_FAILURE;
    }
    int tslice = atoi(*(argv+8));
    if (tslice < 0){
        perror("ERROR: Negative timeslice");
        return EXIT_FAILURE;
    }

    printf("<<< -- process set (n=%d) with %d CPU-bound process\n", n, ncpu);
    printf("<<< -- seed=%d; lambda=%.6f; bound=%d\n\n", seed,lambda, upperBound);

    // Simulation Calcs
    srand48(seed);
    Process **processes = calloc(n, sizeof(Process*));
    char *letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int letterCount = -1;
    for (int i = 0; i < n; i++) {
        // Create processes
        *(processes+i) = calloc(1, sizeof(Process));
        if (i % 10 == 0){
            letterCount++;
        }
        (*(processes+i))->pid = calloc(4, sizeof(char));
        sprintf((*(processes + i))->pid, "%c%d", *(letters+letterCount), (i % 10));

        // Get arrival times
        double arrivalExp = nextExp(lambda, upperBound);
        int arrivalTime = (int)floor(arrivalExp);
        int numBursts = (int)ceil(drand48() * 32);
        (*(processes+i))->arrivalTime = arrivalExp;
        (*(processes+i))->endTime = INT_MAX;
        (*(processes+i))->numBursts = numBursts;
        (*(processes+i))->cpuBursts = calloc(numBursts + 1, sizeof(int));
        (*(processes+i))->ioBursts = calloc(numBursts, sizeof(int));
        // For SJF/SRT
        (*(processes+i))->tau = (int)ceil(1.0 / lambda);

        if (i < ncpu){
            printf("CPU-bound process %s: arrival time %dms; %d CPU bursts:\n", (*(processes+i))->pid, arrivalTime, numBursts);
        } else{
            printf("I/O-bound process %s: arrival time %dms; %d CPU bursts:\n",(*(processes+i))->pid, arrivalTime, numBursts);
        }
        
        // Simulate CPU Bursts
        for (int j = 0; j < numBursts; j++) {
            int cpuBurst = (int)ceil(nextExp(lambda, upperBound));
            if (j < numBursts - 1) {
                int ioBurst = (int)ceil(nextExp(lambda, upperBound));
                // Check for CPU-bound Process
                if (i < ncpu){
                    cpuBurst *= 4;
                } else{
                    ioBurst *= 8;
                }
                *((*(processes+i))->ioBursts+j) = ioBurst;
                printf("==> CPU burst %dms ==> I/O burst %dms\n", cpuBurst, ioBurst);
            } else {
                if (i < ncpu){
                    cpuBurst *= 4;
                }
                printf("==> CPU burst %dms\n\n", cpuBurst);
            }
            *((*(processes+i))->cpuBursts+j) = cpuBurst;
        }
    }

    printf("<<< PROJECT SIMULATIONS\n");
    printf("<<< -- t_cs=%dms; alpha=%.2f; t_slice=%dms\n", tcs, alpha, tslice);
    // FCFS
    // FCFS(processes, n, tcs);
    // SJF

    // SRT

    // RR
    RR(processes, n, tcs, tslice);
    // Clean up
    for (int i = 0; i < n; i++){
        free((*(processes+i))->pid);
        free(*(processes+i));
    }
    free(processes);

}