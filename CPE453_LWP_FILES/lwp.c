#include <stdlib.h>
#include <stdio.h>
#include "lwp.h"

lwp_context lwp_ptable[LWP_PROC_LIMIT];
int lwp_procs = 0;
int lwp_running = -1;

static ptr_int_t *main_sp = NULL;

int new_lwp(lwpfun fun, void *arg, size_t stacksize) {
    if (lwp_procs >= LWP_PROC_LIMIT) return -1;

    lwp_context *proc = &lwp_ptable[lwp_procs];
    ptr_int_t *stack = malloc(stacksize * sizeof(ptr_int_t));
    if (!stack) return -1;

    ptr_int_t *sp = stack + stacksize;  // top of stack (stack grows down)

    *(--sp) = (ptr_int_t)arg;               // push argument to indentnum
    *(--sp) = (ptr_int_t)lwp_exit;          // fake return addr to call lwp_exit when done
    *(--sp) = (ptr_int_t)fun;               // this acts like a "return address" to jump to

    // Push dummy register values (eax, ebx, ecx...) in reverse order of RESTORE_STATE
    for (int i = 0; i < 6; i++) *(--sp) = 0;  // edi, esi, edx, ecx, ebx, eax
    *(--sp) = (ptr_int_t)(sp + 1);           // fake ebp

    proc->pid = lwp_procs;
    proc->stack = stack;
    proc->stacksize = stacksize;
    proc->sp = sp;

    printf("[DEBUG] Created LWP %d, stack base = %p, sp = %p\n",
        proc->pid, (void*)stack, (void*)sp);

    return lwp_procs++;
}
