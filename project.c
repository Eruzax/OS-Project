#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

typedef enum {ARRIVE, READY, RUNNING, PREEMPTION, WAITING, TERMINATED} State;

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

// Process: Process associated with the event
// Time: timestamp when a process finishes its state
// State: state of the process
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

// Insert an event into the event queue for FCFS
void insertEventFCFS(EventQueue* q, Event* event) {
    if (q->size >= q->capacity) {
        fprintf(stderr, "ERROR: EventQueue is full. Cannot insert event.\n");
        return;
    }
    
    int insertIndex = q->size;  // Default to inserting at the end

    // Find the correct position to insert while keeping order
    for (int i = 0; i < q->size; i++) {
        if (q->events[i]->time > event->time) {
            insertIndex = i;
            break;
        }
    }

    // Shift all events **after insertIndex** one position to the right
    for (int j = q->size; j > insertIndex; j--) {
        q->events[j] = q->events[j - 1];
    }

    // Insert the new event at the correct position
    q->events[insertIndex] = event;
    q->size++;
}


// Remove the element at the top of event queue
Event* popEvent(EventQueue* q) {
    if (q->size == 0) return NULL;
    Event* event = q->events[0];
    q->size--;
    for (int i = 0; i < q->size; i++) {
        q->events[i] = q->events[i + 1];
    }
    return event;
}

const char* stateToString(State s) {
    switch (s) {
        case ARRIVE:      return "ARRIVE";
        case READY:       return "READY";
        case RUNNING:     return "RUNNING";
        case PREEMPTION:  return "PREEMPTION";
        case WAITING:     return "WAITING";
        case TERMINATED:  return "TERMINATED";
        default:          return "UNKNOWN";
    }
}

void printEventQueue(EventQueue* q) {
    if (q->size == 0) {
        printf("[Q empty]\n");
        return;
    }
    
    printf("Event Queue: ");
    for (int i = 0; i < q->size; i++) {
        Event* e = q->events[i];
        printf("[Time: %d, Process: %s, State: %s]", e->time, e->process->pid, stateToString(e->state));
    }
    printf("\n");
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

Process* dequeue(Queue *q) {
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

// First Come First Serve
void FCFS(Process** processes, int n, int tcs) {
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
    printf("time 0ms: Simulator started for FCFS [Q empty]\n");

    // Arrivals
    for (int i = 0; i < n; i++){
        Event* newEvent = createEvent(*(processes+i), (*(processes+i))->arrivalTime, ARRIVE);
        insertEventFCFS(&eq, newEvent);
    }
    int time = 0;
    int terminatedCount = 0;
    int cpuFreeAt = 0;
    int cpuIdle = -1;
    while (terminatedCount < n) {
        // Handle Events
        Event* e = popEvent(&eq);
        time = e->time;
        // Arrival
        if (e->state == ARRIVE){
            // Print and add to queue
            enqueue(&q, e->process);
            printf("time %dms: Process %s arrived; added to ready queue [Q", time, e->process->pid);
            printQueue(&q);
            printf("]\n");
            // Create a CPU burst event
            // CPU is free
            if (cpuIdle == -1 && time >= cpuFreeAt){
                Event* newEvent = createEvent(e->process, time + tcs/2, READY);
                insertEventFCFS(&eq, newEvent);
                cpuFreeAt = time + e->process->cpuBursts[e->process->numBursts - e->process->burstsLeft] + tcs/2;
                dequeue(&q);
            }
            // CPU is not free
            else{
                Event* newEvent = createEvent(e->process, cpuFreeAt + tcs, READY);
                insertEventFCFS(&eq, newEvent);
                cpuFreeAt += e->process->cpuBursts[e->process->numBursts - e->process->burstsLeft] + tcs;
            }
        }
        // Start CPU Burst
        else if (e->state == READY){
            cpuIdle = 0;
            if (e->process->pid == q.procs[0]->pid){
                dequeue(&q);
            }
            int burstTime = *(e->process->cpuBursts + (e->process->numBursts - e->process->burstsLeft));
            printf("time %dms: Process %s started using the CPU for %dms burst [Q", time, e->process->pid, burstTime);
            printQueue(&q);
            printf("]\n");
            e->process->burstsLeft--;
            if (e->process->burstsLeft == 0){
                Event* endCpu = createEvent(e->process, time + burstTime, TERMINATED);
                insertEventFCFS(&eq, endCpu);
            } else{
                Event* endCpu = createEvent(e->process, time + burstTime, RUNNING);
                insertEventFCFS(&eq, endCpu);
            }
            // updated the time when the CPU is free
            if (cpuFreeAt < time + burstTime){
                cpuFreeAt = time + burstTime;
            }
        }
        // CPU Burst complete
        else if(e->state == RUNNING){
            // CPU Burst complete
            cpuIdle = -1;
            if (e->process->burstsLeft == 1){
                printf("time %dms: Process %s completed a CPU burst; %d burst to go [Q", time, e->process->pid, e->process->burstsLeft);
            } else{
                printf("time %dms: Process %s completed a CPU burst; %d bursts to go [Q", time, e->process->pid, e->process->burstsLeft);
            }
            printQueue(&q);
            printf("]\n");

            // IO Burst start
            int ioCompTime = time + *(e->process->ioBursts+(e->process->numBursts - e->process->burstsLeft - 1)) + tcs/2;
            printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms [Q", time, e->process->pid, ioCompTime);
            printQueue(&q);
            printf("]\n");
            Event* ioBurst = createEvent(e->process, ioCompTime, WAITING);
            insertEventFCFS(&eq, ioBurst);
        }
        // IO End
        else if (e->state == WAITING){
            if (cpuIdle == -1){
                dequeue(&q);
            }
            enqueue(&q, e->process);
            printf("time %dms: Process %s completed I/O; added to ready queue [Q", time, e->process->pid);
            printQueue(&q);
            printf("]\n");
            // Get the burst time of the last process ready to run
            if (q.size == 1 && cpuIdle == -1){
                cpuFreeAt = time;
                Event* cpuBurst = createEvent(e->process, cpuFreeAt + tcs/2, READY);
                insertEventFCFS(&eq, cpuBurst);
            } else if (q.size > 1){
                int lastProcBurst = time;
                for (int i = eq.size; i > 0; i--){
                    if (eq.events[i]->state == READY){
                        lastProcBurst = eq.events[i]->time + eq.events[i]->process->cpuBursts[eq.events[i]->process->numBursts - eq.events[i]->process->burstsLeft];
                        break;
                    }
                }
                
                // Creates an event while considering CPU Bursts times in the queue
                Event* cpuBurst = createEvent(e->process, lastProcBurst + tcs, READY);
                insertEventFCFS(&eq, cpuBurst);
                int burstTime = e->process->cpuBursts[e->process->numBursts - e->process->burstsLeft];
                cpuFreeAt = lastProcBurst + burstTime + tcs;
            } else {
                Event* cpuBurst = createEvent(e->process, cpuFreeAt + tcs, READY);
                insertEventFCFS(&eq, cpuBurst);
            }
        }
        // Termination
        else if (e->state == TERMINATED){
            cpuIdle = -1;
            printf("time %dms: Process %s terminated [Q", time, e->process->pid);
            printQueue(&q);
            printf("]\n");
            terminatedCount++;
        }
        free(e);
    }
    printf("time %dms: Simulator ended for FCFS [Q empty]\n", time + tcs/2);

    free(q.procs);
}

void SJF(){
}

void SRT(){

}

// void RR(Process** processes, int n, int tcs, int tslice){
// // Set all processes to ARRIVE and initialize burstsLeft
//     for (int i = 0; i < n; i++) {
//         (*(processes+i))->state = ARRIVE;
//         (*(processes+i))->burstsLeft = (*(processes+i))->numBursts;
//         (*(processes+i))->remainingBursts = (*(processes+i))->cpuBursts;
//     }

//     // Start Simultaion
//     Queue q;
//     initQueue(&q, n);
//     EventQueue eq;
//     initEventQueue(&eq, n);
//     printf("time 0ms: Simulator started for RR [Q empty]\n");

//     // Arrivals
//     for (int i = 0; i < n; i++){
//         Event* newEvent = createEvent(*(processes+i), (*(processes+i))->arrivalTime, ARRIVE);
//         insertEvent(&eq, newEvent);
//     }
//     int time = 0;
//     int terminatedCount = 0;
//     int cpuFreeAt = -1;
//     int cpuIdle = -1;
//     // int csCount = 0;
//     while (terminatedCount < n) {
//         Event *e = popEvent(&eq);
//         time = e->time;
//         // Arrival
//         if (e->state == ARRIVE){
//             // Print and add to queue
//             enqueue(&q, e->process);
//             printf("time %dms: Process %s arrived; added to ready queue [Q", time, e->process->pid);
//             printQueue(&q);
//             printf("]\n");
//             // Create a CPU burst event
//             // CPU is free
//             if (cpuIdle == -1 && time >= cpuFreeAt){
//                 Event* newEvent = createEvent(e->process, time + tcs/2, READY);
//                 insertEvent(&eq, newEvent);
//                 cpuIdle = 0;
//             }
//             // CPU is not free
//             else{
//                 // Update variables
//                 cpuFreeAt = time + e->process->cpuBursts[e->process->numBursts - e->process->burstsLeft] + tcs;
//                 Event* newEvent = createEvent(e->process, cpuFreeAt, READY);
//                 insertEvent(&eq, newEvent);
//                 cpuIdle = 0;
//             }
//             free(e);
//         }
//         // CPU Burst
//         else if (e->state == READY) {
//             dequeue(&q);
//         } 
//         // CPU Burst finished and Begin I/O Burst
//         else if (e->state == RUNNING) {

//         } 
//         // Kick out running process if the process ran out of time
//         else if (e->state == PREEMPTION) {

//         } 
//         // IO Burst finished and add to queue if there are more bursts
//         else if (e->state == WAITING) {

//         } 
//         // Process completed all bursts
//         else if (e->state == TERMINATED){

//             terminatedCount++;
//         }
//     }
// }

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
    if (ncpu == 1){
        printf("<<< -- process set (n=%d) with %d CPU-bound process\n", n, ncpu);
    } else {
        printf("<<< -- process set (n=%d) with %d CPU-bound processes\n", n, ncpu);
    }
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

        // Print Process Info
        if (i < ncpu){
            if (numBursts == 1){
                printf("CPU-bound process %s: arrival time %dms; %d CPU burst:\n", (*(processes+i))->pid, arrivalTime, numBursts);
            } else {
                printf("CPU-bound process %s: arrival time %dms; %d CPU bursts:\n", (*(processes+i))->pid, arrivalTime, numBursts);
            }
        } else{
            if (numBursts == 1){
                printf("I/O-bound process %s: arrival time %dms; %d CPU burst:\n",(*(processes+i))->pid, arrivalTime, numBursts);
            } else {
                printf("I/O-bound process %s: arrival time %dms; %d CPU bursts:\n",(*(processes+i))->pid, arrivalTime, numBursts);
            }
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
    FCFS(processes, n, tcs);
    // SJF

    // SRT

    // RR
    // RR(processes, n, tcs, tslice);
    // Clean up
    for (int i = 0; i < n; i++){
        free((*(processes+i))->pid);
        free(*(processes+i));
    }
    free(processes);

}