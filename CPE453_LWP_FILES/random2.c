#include <stdio.h>

#include <stdlib.h>

#include "lwp.h"





/* MACROS:

        SAVE_STATE()

        RESTORE_STATE()

        SetSP()

        GetSP()

*/



// Global Pointer that gets stack pointers

lwp_context lwp_ptable[LWP_PROC_LIMIT];     // Process Table

int current_lwp_processes = 0;              // Current Number of LWP's

int curr_process = -1;                      // Current Process

unsigned long curr_pid_process = 0;     

ptr_int_t *global_sp = NULL; 

schedfun scheduler = NULL; 



/* lwp functions */

extern int new_lwp(lwpfun function ,void *arg, size_t stacksize); 

extern int lwp_getpid();

extern void lwp_yield();

extern void lwp_exit();

extern void lwp_start();

extern void lwp_stop(); // 

extern void lwp_set_scheduler(schedfun sched);



/* helper functions*/

void ribbed_robin(){

    if(curr_process ==  current_lwp_processes - 1){

        curr_process = 0; 

    } else{

        curr_process++; 

    } 

}



void based_scheduler(){

    if(scheduler == NULL){ // Default Scheduler

        ribbed_robin(); 

    }else{ // Other Schedulers

        curr_process = (scheduler)(); // 

    }

}





// This works

int new_lwp(lwpfun function, void *arg, size_t stacksize){

    

    if(current_lwp_processes >= LWP_PROC_LIMIT){ // Checking the number of LWP's active 

        // printf("Error: Reached LWP process limit\n"); // -- Debug Statement

        return -1; 

    }



    ptr_int_t *stack_pointer = malloc(stacksize * 4); 



    lwp_ptable[current_lwp_processes].stack = stack_pointer;



    stack_pointer += stacksize; 

    stack_pointer -= 1; 



    // Arguments First 

    *stack_pointer = (ptr_int_t) arg; 

    stack_pointer -= 1; 



    // 

    *stack_pointer = (ptr_int_t) lwp_exit; //Why?

    stack_pointer -= 1; 



    // Function 

    *stack_pointer = (ptr_int_t) function;

    stack_pointer -= 1; 



    // Default base pointer address

    *stack_pointer = (ptr_int_t) 0xBEEFBEEF;

    

    ptr_int_t base_pointer = (ptr_int_t) stack_pointer;

    stack_pointer -= 7; 

    *stack_pointer = base_pointer; 



    // Copy Stuff 

    curr_pid_process++; 

    lwp_ptable[current_lwp_processes].pid = curr_pid_process; 

    lwp_ptable[current_lwp_processes].sp = stack_pointer; 

    lwp_ptable[current_lwp_processes].stacksize = stacksize; 

    current_lwp_processes++; 



    return lwp_ptable[current_lwp_processes - 1].pid;

}



int  lwp_getpid(){

    if (curr_process == -1) {

        return 0; // No LWP is currently running

    }

    

    return lwp_ptable[curr_process].pid;

}



void lwp_yield(){ // Soft-Bullies LWP's

    SAVE_STATE(); 

    GetSP(lwp_ptable[curr_process].sp);

    

    based_scheduler(); 

    

    SetSP(lwp_ptable[curr_process].sp); 

    RESTORE_STATE(); 

}



void lwp_exit(){

    // printf("LWP end\n"); // -- Debug Statement

    free(lwp_ptable[curr_process].stack);

    current_lwp_processes--; 



    if(current_lwp_processes > 0 ){

        int curr = 0; 

        for(curr = curr_process; curr < current_lwp_processes; curr++){

            lwp_ptable[curr] = lwp_ptable[curr + 1]; 

        }

    } else{ // current_lwp_processes == 0

        SetSP(global_sp);  

        RESTORE_STATE();

        return; 

    }



    SetSP(lwp_ptable[curr_process].sp); 

    RESTORE_STATE(); 

}





// When lwp_start() is called:

void lwp_start(){

    // Check if there are processes to run

    if (current_lwp_processes <= 0) {

        return;

    }

    

    // Save the main thread's context

    SAVE_STATE();

    

    // Get the main thread's stack pointer

    GetSP(global_sp);

    

    // Set the current process to the first one

    curr_process = 0;

    

    // Switch to the first process

    SetSP(lwp_ptable[curr_process].sp);

    

    // Restore the new context (this will start running the first LWP)

    RESTORE_STATE();

}





void lwp_stop(){

    SAVE_STATE(); 

    GetSP(lwp_ptable[curr_process].sp);

    SetSP(global_sp); 

    RESTORE_STATE(); 

}



void lwp_set_scheduler(schedfun sched){

    scheduler = sched;

}