/*
 * student.c
 * Multithreaded OS Simulation for ECE 3056
 * Peter Nguyen pnguyen77
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os-sim.h"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;

static pcb_t *front_ready_queue; // front of the ready queue, start of the linked list
static pthread_mutex_t ready_mutex;

static pthread_cond_t not_idle; //idle function

static int time_slice; //based on os-sim, for context_switch, -1 for FIFO LRTF
static int longest_rem_time;
static int round_robin;
static int cpu_count;

static void queue_ready(pcb_t *pcbt){
  pcb_t *current_node; //current node of the linked list
  pthread_mutex_lock(&ready_mutex);
  current_node = front_ready_queue;
  if(current_node == NULL){
    front_ready_queue = pcbt; //sets the front of the queue to pcbt
  }
  else{
    while(current_node -> next != NULL){
      current_node = current_node -> next; // goes to the next current_node
    }
    current_node -> next = pcbt; //sets the last process to the end
  }
  pcbt -> next = NULL;
  pthread_cond_signal(&not_idle);
  pthread_mutex_unlock(&ready_mutex);

}


static pcb_t* dequeue_ready(){ //removes the head of the queue
  pcb_t *node; //head of the queue
  pthread_mutex_lock(&ready_mutex);
  node = front_ready_queue; //sets the head to be dequeued
  if(node != NULL){
    front_ready_queue = front_ready_queue -> next;
  } //if the removed node isnt NULL, set the front of the queue to the next node.
  pthread_mutex_unlock(&ready_mutex);
  return node; //returns the removed head of the queue
}

static pcb_t* dequeue_longest(){ //removes the longest time node
  pcb_t * current_node;
  pcb_t * longest_node;
  unsigned int time_longest = 0;
  current_node = front_ready_queue;

  if(current_node == NULL){
    return NULL;
  }
  else if (current_node -> next == NULL){
    front_ready_queue = NULL;
    return current_node;
  }
  else{
    while(current_node != NULL){
      if(current_node -> time_remaining > time_longest){
        time_longest = current_node -> time_remaining;
        longest_node = current_node;
      }
      current_node = current_node -> next;
    }
// finds the longest node, now remove it and link back together
  current_node = front_ready_queue; // restarts the looking through the list
  if(longest_node == current_node){
    front_ready_queue = front_ready_queue -> next;
    return longest_node;
  }
  else{
    while(current_node -> next != longest_node){
      current_node = current_node -> next;
    }//iterates through the list til we find the longest_node
    current_node->next = longest_node -> next;

  }
    return longest_node;
  }
}








/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id.
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */
    pcb_t *pcbt; //the process being ran
    if(longest_rem_time){
      pcbt = dequeue_longest();
    }
    else{
      pcbt = dequeue_ready(); // the process being ran is the front of the queue
    }
    pthread_mutex_lock(&current_mutex);
    if(pcbt != NULL){
      pcbt -> state = PROCESS_RUNNING;
    }
    current[cpu_id] = pcbt; //sets a processor to the process
    pthread_mutex_unlock(&current_mutex);
    context_switch(cpu_id,pcbt,time_slice);//calls upon context switch
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&ready_mutex);
    while(front_ready_queue == NULL){
      pthread_cond_wait(&not_idle,&ready_mutex);
    }
    pthread_mutex_unlock(&ready_mutex);
    schedule(cpu_id);

}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
  pcb_t* pcbt;
  pthread_mutex_lock(&current_mutex);
  pcbt = current[cpu_id];
  pcbt -> state = PROCESS_READY;
  pthread_mutex_unlock(&current_mutex);
  queue_ready(pcbt);
  schedule(cpu_id);
    /* FIX ME */
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pcb_t* pcbt;
    pcbt = current[cpu_id];
    pcbt -> state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);

}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pcb_t* pcbt;
    pcbt = current[cpu_id];
    pcbt -> state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is LRTF, wake_up() may need
 *      to preempt the CPU with lower remaining time left to allow it to
 *      execute the process which just woke up with higher reimaing time.
 * 	However, if any CPU is currently running idle,
* 	or all of the CPUs are running processes
 *      with a higher remaining time left than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    /* FIX ME */
    int id;
    unsigned int lowest_time;
    process -> state = PROCESS_READY;
    queue_ready(process);
    if(longest_rem_time){
      pthread_mutex_lock(&current_mutex);
      id = -1;
      lowest_time = 10;
      for(int i = 0; i < cpu_count; i++){
        if(current[i] == NULL){
          id = -1;
          break;
        }
        if(current[i] -> time_remaining < lowest_time){
          lowest_time = current[i] -> time_remaining;
          id = i;
        }
      }
      pthread_mutex_unlock(&current_mutex);
      if(id != -1 && lowest_time < process -> time_remaining){
        id = (unsigned int) id;
        force_preempt(id);
      }
    }
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -l and -r command-line parameters.
 */
int main(int argc, char *argv[])
{
    unsigned int cpu_count;
    time_slice = -1;//FIFO
    longest_rem_time = 0;
    round_robin = 0;

    /* Parse command-line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "ECE 3056 OS Sim -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -l | -r <time slice> ]\n"
            "    Default : FIFO Scheduler\n"
	    "         -l : Longest Remaining Time First Scheduler\n"
            "         -r : Round-Robin Scheduler\n\n");
        return -1;
    }
    cpu_count = strtoul(argv[1], NULL, 0);

    /* FIX ME - Add support for -l and -r parameters*/
      printf("%d", longest_rem_time);
      if (argc >= 3) {
      if(strcmp(argv[2],"-l") == 0){
        longest_rem_time = 1;
      }
      if(strcmp(argv[2],"-r") == 0){
        round_robin = 1;
        time_slice = strtoul(argv[3], NULL, 0);
      }
    }







    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}
