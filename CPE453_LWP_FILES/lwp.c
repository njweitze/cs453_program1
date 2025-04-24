#include <stdlib.h>
#include <stdio.h>
#include "lwp.h"

lwp_context lwp_ptable[LWP_PROC_LIMIT];
int lwp_procs = 0;
int lwp_running = -1;

// Pointer to save the main (original) stack
static ptr_int_t *main_sp = NULL;

// Optional custom scheduler
static schedfun scheduler = NULL;


int new_lwp(lwpfun fun, void *arg, size_t stacksize) {
    if (lwp_procs >= LWP_PROC_LIMIT) return -1;

    lwp_context *proc = &lwp_ptable[lwp_procs];
    ptr_int_t *stack = malloc(stacksize * sizeof(ptr_int_t));
    if (!stack) return -1;

    ptr_int_t *sp = stack + stacksize;

    // 👇 Stack grows down — push in reverse order of use

    // Push argument for the function (this goes into the function's stack frame)
    *(--sp) = (ptr_int_t)arg;

    // Push fake return address — after 'fun' returns, it "returns" to lwp_exit
    *(--sp) = (ptr_int_t)lwp_exit;

    // Push dummy base pointer (so 'leave' works on return)
    *(--sp) = 0;

    // Push dummy registers to match what RESTORE_STATE expects
    *(--sp) = 0; // edi
    *(--sp) = 0; // esi
    *(--sp) = 0; // edx
    *(--sp) = 0; // ecx
    *(--sp) = 0; // ebx
    *(--sp) = 0; // eax

    // Push the function pointer — becomes the "return address" after RESTORE_STATE
    *(--sp) = (ptr_int_t)fun;

    proc->pid = lwp_procs;
    proc->stack = stack;
    proc->stacksize = stacksize;
    proc->sp = sp;

    return lwp_procs++;
}

// ==== lwp_start(): Start the threading system ====
void lwp_start() {
    if (lwp_procs == 0) return;

    SAVE_STATE();        // Save the main thread’s CPU state
    GetSP(main_sp);      // Save the main stack pointer

    // Pick the first thread to run
    lwp_running = scheduler ? scheduler() : 0;
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
    if (scheduler) {
        lwp_running = scheduler();
    } else {
        lwp_running = (lwp_running + 1) % lwp_procs;
    }

    SetSP(lwp_ptable[lwp_running].sp);
    RESTORE_STATE(); // Jump to next thread
}

// ==== lwp_exit(): Terminate the current thread ====
void lwp_exit() {
    free(lwp_ptable[lwp_running].stack);

    // Shift remaining threads in the table up
    for (int i = lwp_running; i < lwp_procs - 1; i++) {
        lwp_ptable[i] = lwp_ptable[i + 1];
    }

    lwp_procs--;

    if (lwp_procs == 0) {
        // No more threads, return to main
        SetSP(main_sp);
        RESTORE_STATE();
    }

    // Otherwise, pick a new thread to run
    lwp_running %= lwp_procs;
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
    return (lwp_running >= 0) ? lwp_ptable[lwp_running].pid : -1;
}

// ==== lwp_set_scheduler(): Install a custom scheduler ====
void lwp_set_scheduler(schedfun sched) {
    scheduler = sched;
}