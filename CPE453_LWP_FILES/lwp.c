#include <stdlib.h>
#include <stdio.h>
#include "lwp.h"

// globals
lwp_context lwp_ptable[LWP_PROC_LIMIT];
int lwp_procs = 0;
int pid = 0;
int lwp_running = -1;
ptr_int_t *main_sp = NULL; // main thread's stack pointer
schedfun scheduler = NULL; // optional scheduler override

int new_lwp(lwpfun fun, void *arg, size_t stacksize) {
    if (lwp_procs >= LWP_PROC_LIMIT) {
        return -1; // max processes hit
    }

    // allocate stack memory
    ptr_int_t *sp = malloc(stacksize * 4);

    lwp_ptable[lwp_procs].stack = sp; // save original base

    // move sp to top of stack
    sp += stacksize;
    sp--;

    // setup initial stack frame
    *sp = (ptr_int_t)arg;       // push thread argument
    sp--;
    *sp = (ptr_int_t)lwp_exit;  // push fake return to lwp_exit
    sp--;
    *sp = (ptr_int_t)fun;       // push thread function
    sp--;

    *sp = (ptr_int_t)0xDEADBEEF; // dummy BP

    ptr_int_t bp = (ptr_int_t)sp; // save fake BP
    sp -= 7;
    *sp = bp;                    // push base pointer

    // setup process table entry
    pid++;
    lwp_ptable[lwp_procs].pid = pid;
    lwp_ptable[lwp_procs].sp = sp;
    lwp_ptable[lwp_procs].stacksize = stacksize;
    lwp_procs++;

    return lwp_ptable[lwp_procs - 1].pid; // return new pid
}

void lwp_start() {
    if (lwp_procs <= 0) {
        return; // no processes to run
    }

    SAVE_STATE();
    GetSP(main_sp);

    lwp_running = 0; // start with first thread
    SetSP(lwp_ptable[lwp_running].sp);

    RESTORE_STATE();
}

void lwp_yield() {
    SAVE_STATE();
    GetSP(lwp_ptable[lwp_running].sp);

    // pick next thread
    if (scheduler != NULL) {
        lwp_running = scheduler();
    } else {
        if (lwp_running == lwp_procs - 1) {
            lwp_running = 0; // wrap around
        } else {
            lwp_running = (lwp_running + 1) % lwp_procs;
        }
    }

    SetSP(lwp_ptable[lwp_running].sp);
    RESTORE_STATE();
}

// kill the current thread
void lwp_exit() {
    free(lwp_ptable[lwp_running].stack); // free stack

    lwp_procs--;

    if (lwp_procs == 0) {
        // last thread exited, return to main
        SetSP(main_sp);
        RESTORE_STATE();
        return;
    } else {
        // shift remaining threads up in the table
        int curr;
        for (curr = lwp_running; curr < lwp_procs; curr++) {
            lwp_ptable[curr] = lwp_ptable[curr + 1];
        }
    }

    SetSP(lwp_ptable[lwp_running].sp);
    RESTORE_STATE();
}

void lwp_stop() {
    SAVE_STATE();
    GetSP(lwp_ptable[lwp_running].sp);

    SetSP(main_sp);
    RESTORE_STATE();
}

int lwp_getpid() {
    if (lwp_running == -1) {
        return 0; // not inside LWP system
    }
    return lwp_ptable[lwp_running].pid;
}

void lwp_set_scheduler(schedfun sched) {
    scheduler = sched;
}
