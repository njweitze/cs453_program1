#include <stdlib.h>
#include <stdio.h>
#include "lwp.h"

lwp_context lwp_ptable[LWP_PROC_LIMIT];
int lwp_procs = 0;
int pid = 0;
int lwp_running = -1;

// Pointer to save the main (original) stack
ptr_int_t *main_sp = NULL;

// Optional custom scheduler
schedfun scheduler = NULL;



int new_lwp(lwpfun fun, void *arg, size_t stacksize) {
    if (lwp_procs >= LWP_PROC_LIMIT) {
        return -1;
    }
    
    ptr_int_t *sp = malloc(stacksize * 4);
    if (!sp) return -1;

    lwp_ptable[lwp_procs].stack = sp;

    sp += stacksize;
    sp--;


    *sp = (ptr_int_t)arg;        // argument to pass
    sp--;
    *sp = (ptr_int_t)lwp_exit;   // return address after thread finishes
    sp--;
    *sp = (ptr_int_t)fun;        // "fake" return to thread function
    sp--;

    *sp = (ptr_int_t)0xDEADBEEF;
    ptr_int_t bp = (ptr_int_t)sp;
    
    sp -= 7;

    *sp = bp;

    pid++;
    lwp_ptable[lwp_procs].pid = pid;
    lwp_ptable[lwp_procs].stack = sp;
    lwp_ptable[lwp_procs].stacksize = stacksize;
    lwp_procs++;

    return lwp_ptable[lwp_procs - 1].pid;
}

// ==== lwp_start(): Start the threading system ====
void lwp_start() {
    if (lwp_procs <= 0) {
        return;
    }

    SAVE_STATE();        // Save the main thread’s CPU state
    GetSP(main_sp);      // Save the main stack pointer

    // Pick the first thread to run
    lwp_running = 0;
    SetSP(lwp_ptable[lwp_running].sp);

    RESTORE_STATE();     // Jump into thread's context
}

// ==== lwp_yield(): Yield to another thread ====
void lwp_yield() {
    // Save current thread's state
    SAVE_STATE();
    GetSP(lwp_ptable[lwp_running].sp);

    // Choose next thread (scheduler or round-robin)
    // int prev = lwp_running;
    if (scheduler != NULL) {
        lwp_running = (scheduler)();
    } else {
        if (lwp_running == lwp_procs - 1) {
            lwp_running = 0;
        } else {
            lwp_running++;
        }
    }

    SetSP(lwp_ptable[lwp_running].sp);

    RESTORE_STATE(); // Jump to next thread
}

// ==== lwp_exit(): Terminate the current thread ====
void lwp_exit() {
    free(lwp_ptable[lwp_running].stack);

    lwp_procs--;

    if (lwp_procs == 0) {
        // No more threads, return to main
        SetSP(main_sp);
        RESTORE_STATE();
        return;
    } else {
        int curr = 0;
        for(curr = lwp_running; curr < lwp_procs; curr++) {
            lwp_ptable[curr] = lwp_ptable[curr + 1];
        }
    }

    

    SetSP(lwp_ptable[lwp_running].sp);

    RESTORE_STATE();
}

// ==== lwp_stop(): Pause the LWP system ====
void lwp_stop() {
    SAVE_STATE();
    GetSP(lwp_ptable[lwp_running].sp);

    lwp_running = -1;
    SetSP(main_sp);
    RESTORE_STATE();
}

// ==== lwp_getpid(): Return current thread's ID ====
int lwp_getpid() {
    if (lwp_running  == -1) {
        return 0;
    }

    return lwp_ptable[lwp_running].pid;
}

// ==== lwp_set_scheduler(): Install a custom scheduler ====
void lwp_set_scheduler(schedfun sched) {
    scheduler = sched;
}