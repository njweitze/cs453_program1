#include <stdlib.h>
#include <stdio.h>
#include "lwp.h"

lwp_context lwp_ptable[LWP_PROC_LIMIT];
int lwp_procs = 0;
int lwp_running = -1;

// static ptr_int_t *main_sp = NULL;

int new_lwp(lwpfun fun, void *arg, size_t stacksize) {
    if (lwp_procs >= LWP_PROC_LIMIT) return -1;

    lwp_context *proc = &lwp_ptable[lwp_procs];
    ptr_int_t *stack = malloc(stacksize * sizeof(ptr_int_t));
    if (!stack) return -1;

    ptr_int_t *sp = stack + stacksize;

    *(--sp) = (ptr_int_t)arg;        // argument to pass
    *(--sp) = (ptr_int_t)lwp_exit;   // return address after thread finishes
    *(--sp) = (ptr_int_t)fun;        // "fake" return to thread function

    int i;
    for (i = 0; i < 6; i++) *(--sp) = 0;  // dummy edi, esi, edx, ecx, ebx, eax

    sp--;
    *sp = (ptr_int_t)(sp + 1);  // fake ebp


    proc->pid = lwp_procs;
    proc->stack = stack;
    proc->stacksize = stacksize;
    proc->sp = sp;

    printf("[DEBUG] Created LWP %lu, stack base = %p, sp = %p\n", (unsigned long)proc->pid, (void*)stack, (void*)sp);

    return lwp_procs++;
}

void lwp_yield() {
    // stub: no context switching yet
}

void lwp_exit() {
    // stub: doesn't do anything yet
}

void lwp_set_scheduler(schedfun sched) {
    // stub: ignore custom scheduler for now
}


