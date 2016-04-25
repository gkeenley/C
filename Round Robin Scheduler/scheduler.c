#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
char temp[56];
int finished=0; // Boolean value initialized to 0 and set to 1 when final process has finished.
int done_sum=0, t=0; // Sum of processes that are finished. Used to determine value of 'finished' ^.
int queue_1_empty=0; // Boolean value for whether queue1 is empty or full.
int queue_2_empty=0; // Boolean value for whether queue2 is empty or full.
int queue_IO_empty=0; // Boolean value for whether queueI/O is empty or full.
int queue_1_next=0; // Index of next open slot in queue 1.
int queue_2_next=0; // Index of next open slot in queue 1.
int queue_IO_next=0; // Index of next open slot in IO queue.
int queue_1_quantum_timer=0; // Timer to keep track of whether time quantum for queue 1 has expired.
int queue_2_quantum_timer=0; // Timer to keep track of whether time quantum for queue 2 has expired.
int input_line_counter; // Index for counting lines from input file.
int number_of_processes=0; // Number of Processes, initialized to 0 and updated as processes added.
int CPU=1; // Value, 1 or 2, indicating which queue has CPU.
int new_process_imminent=0; // Boolean value indicating whether there will be a new process arriving 1 ms later.
int flag_reset_time_quantum_1=0; // Flag to reset time quantum for queue 1.
int flag_reset_time_quantum_2=0; // Flag to reset time quantum for queue 2.
struct process{ // Structure for each process.
    int process_id, arrival_time, CPU_time, IO_frequency, IO_duration; // Standard parameters from input file.
    float CPU_burst_time; // Length of all BPU bursts, other than possibly the last one (in the event that it is longer).
    int number_of_CPU_bursts; // Number of CPU bursts. Equal to number of IO bursts unless CPU does not divide IO frequency evenly.
    int current_CPU_burst; // Index of which CPU burst process is currently on.
    int current_IO_burst; // Index of which IO burst process is currently on.
    int current_CPU_burst_duration; // Duration of current CPU burst, equal either to 'CPU_burst_time' or a longer value determined at runtime.
    int CPU_burst_timer; // Timer to keep track of progress of current CPU burst.
    int IO_burst_timer; // Timer to keep track of progress of current IO burst.
    int status; // 1: ready. 2: running. 3: IO. 4: done.
    int anom_CPU_burst_time; // 'anomalous' CPU burst time, for final CPU burst in the event that CPU time does not divide IO frequency evenly.
    int norm; // Boolean value, indicating if CPU time divided IO frequency evenly or not.
    int old_queue; // Value indicating which queue, 1 or 2, was old queue of given process that is now in IO.
    int preempt; // Boolean value indicating whether or not a given process was preempted.
    int IO_flag; //  Boolean value indicating that a process was JUST put in IO, so it should not run until the next time increment.
    int dont_run_yet; // Singal to not run a given process because another particular one shold have priority.
};

void clear_lead(int* queue, int size); // Function that clears leading process and moves others up.
void shift_back(int* queue, int size); // Opposite of 'clear_lead', function that moves all processes in a queue back one slot.

int main(int argc, char *argv[]) 
{
    int queue_1_time_quantum=atoi(argv[2]); // Queue 1 time quantum.
    int queue_2_time_quantum=atoi(argv[3]); // Queue 2 time quantum.
    // START Count number of processes.
    FILE *fp1 = fopen(argv[1], "r"); // Create file pointer for input file.
    while (fscanf(fp1, "%d", &input_line_counter)==1){ // While there are still characters to be read from input file...
        number_of_processes++; // Increment character count.
    }
    //number_of_processes=number_of_processes/5; // Divide character count to give process count.
    // END Count number of processes.
    // START Read input data into array.
    int input_info[number_of_processes][5]; // Create array in which to store input data.
    int i, j, k; // Loop indices.
    FILE *fp2 = fopen(argv[1], "r"); // Open input file again.
    char input[number_of_processes][6];
    for (i = 0; i < number_of_processes; i++) { // For each process/line in file...
        fscanf(fp2, "%s", temp);
        for (j=0; j<5; j++){ // For each parameter...
        input[i][j]=temp[j]; // save input data in array.
        }
    }
    // END Read input data into array.
    fclose(fp1); // Close processes counter file. 
    fclose(fp2); // Close input reader file.

struct process process[number_of_processes]; // Structure for each process.
    for (i=0; i<number_of_processes; i++){ // For each process...
        process[i].process_id=input_info[i][0]; // Process ID.
        process[i].arrival_time=input_info[i][1]; // Arrival time.
        process[i].CPU_time=input_info[i][2]; // CPU time.
        process[i].IO_frequency=input_info[i][3]; // IO frequency.
        process[i].IO_duration=input_info[i][4]; // IO duration.
        process[i].CPU_burst_time=((float)process[i].CPU_time)/((float)process[i].IO_frequency); // CPU burst time.
        if (process[i].CPU_burst_time==floorf(process[i].CPU_burst_time)){ // If IO frequency divides CPU time evenly...
            process[i].norm=1; // Indicate that process is normal.
            process[i].CPU_burst_time=process[i].CPU_burst_time; // CPU burst time stays as is.
            process[i].number_of_CPU_bursts=process[i].IO_frequency; // Same numbre of CPU bursts as IO bursts.
            process[i].anom_CPU_burst_time=0; // There is no final 'anomalous' CPU burst.
        }
        else { // If IO frequency does not divide CPU time evenly...
            process[i].norm=0; // Indicate that process is anomalous.
            process[i].CPU_burst_time=floorf(process[i].CPU_burst_time); // CPU burst time rounded down.
            process[i].number_of_CPU_bursts=process[i].IO_frequency+1; // One more CPu burst than IO bursts.
            process[i].anom_CPU_burst_time=process[i].CPU_time-(process[i].IO_frequency*process[i].CPU_burst_time); // Duration of 'anomalous' final burst.
        }
        process[i].current_CPU_burst=1; // Initialize to first CPU burst.
        process[i].current_IO_burst=0; // Initialize IO burst to zero, because we start with CPU burst.
        process[i].current_CPU_burst_duration=0; // Will initialize current CPU burst duration at runtime.
        process[i].CPU_burst_timer=0; // Initialize CPU burst timer.
        process[i].IO_burst_timer=0; // Initialize IO burst timer.
        process[i].status=1; // Initialize status to 'ready'.
        process[i].preempt=0; // Initialize preempt signal to 'false'.
        process[i].IO_flag=0; // Initialize IO flag to 'false'.
        process[i].dont_run_yet=0; // Initialize 'don't run yet' signal to 'false'.
    }
    int queue_1[number_of_processes+1]; // Array holding contents of queue 1.
    int queue_2[number_of_processes+1]; // Array holding contents of queue 2.
    int queue_IO[number_of_processes+1]; // Array holding contents of IO queue.
    int queue_done[number_of_processes+1]; // Array holding finished processes.
    int out[number_of_processes+1]; // Array holding processes that have finished, and for which we have saved result.
    int output[number_of_processes+1][2]; // Array whose contents we output to user.

// Initialize all queues to zero.
for (i=0; i<=number_of_processes; i++){
    queue_1[i]=0;
    queue_2[i]=0;
    queue_IO[i]=0;
    queue_done[i]=0;
    out[i]=0;   
}

while (finished==0){ // While there is still at least one process to run...

new_process_imminent=0; // Initially assume no new processes imminent, so that imminent process signal doesn't carry over from previous iteration.

// Add processes into queues when they arrive.
for (j=0; j<number_of_processes; j++){ // For each process...
    if (input_info[j][1]==t){ // If current time is its arrival time...
        queue_1[queue_1_next]=process[j].process_id; // Add process to queue 1.
        queue_1_next++; // Increment open slot in queue 1.
    }
    if (input_info[j][1]==t+1){ // If process' arrival time is next...
        new_process_imminent=1; // Set signal so other processes will know it's coming.
    }
}

// START Consider queue 1.
if (queue_1[0]!=0){ // If there's a process in queue 1.
    printf("ok %d\n", process[queue_1[0]-1].status);
    if (process[queue_1[0]-1].status==1){ // ready
        printf("ok\n");
        if (CPU==1){ // If the CPU has just been given to q1...
            printf("ok\n");
            // $$ START Start process running in q1
            process[queue_1[0]-1].status=2; // Set status to 'running'.
            //queue_1_quantum_timer=0; // Start time quantum timer.
            process[queue_1[0]-1].CPU_burst_timer=0; // Start CPU burst timer.
            //process[queue_1[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
            // $$ END Start process running in q1
            // $$ START set length of current CPU burst.
            if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                if (process[queue_1[0]-1].norm==1){ // If norm...
                    process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time; // Normal burst duration
                }
                else{ // If anom...
                    process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].anom_CPU_burst_time; // Anom burst duration
                }
            }
            else{ // This is not the last CPU burst
                process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time; // Normal burst duration
            }
            // $$ END set length of current CPU burst.
        }
    } // END 'ready'
    printf("ok\n");
    if (process[queue_1[0]-1].status==2){ // running
        // START Extend time quantum if necessary.
        if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
            if (process[queue_1[0]-1].norm==0){ // If anom...
                process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].anom_CPU_burst_time;
                flag_reset_time_quantum_1=1;
            }
        }
        // END Extend time quantum if necessary.
        if (process[queue_1[0]-1].CPU_burst_timer==(process[queue_1[0]-1].current_CPU_burst_duration-1)){ // If it's end of CPU burst...
            queue_1_quantum_timer=0; // Re-set quantum timer to zero.
            queue_IO[queue_1[0]-1]=queue_1[0]; // Put process in IO.
            process[queue_1[0]-1].IO_flag=1; // Signal that process has just been put in IO so it doesn't increment it this time around.
            process[queue_1[0]-1].IO_burst_timer=0;
            process[queue_1[0]-1].old_queue=1; // Remember that old q was q1.
            process[queue_1[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
            if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts+1){ // If this is the last CPU burst...
                if (process[queue_1[0]-1].norm==1){ // If norm
                    // $$ START Start process running in IO
                    process[queue_1[0]-1].status=3; // Set status to 'IO'.
                    process[queue_1[0]-1].current_IO_burst++; // Increment which IO burst process is on.
                    process[queue_1[0]-1].IO_burst_timer=0; // Intialize IO burst time to 0.
                    // $$ END Start process running in IO
                }
                else{ // If anom
                    process[queue_1[0]-1].status=4; // Set status to 'done'.
                    queue_done[queue_1[0]-1]=1;
                    if (out[queue_1[0]-1]==0){ // If we haven't already stated that process is finished...
                        output[queue_1[0]][0]=process[queue_1[0]-1].process_id;
                        output[queue_1[0]][1]=t+1;
                        out[queue_1[0]-1]=1; // So we don't declare it again.
                    }
                }
            }
            else { // If this is NOT the last CPU burst...
                // $$ START Start process running in IO
                process[queue_1[0]-1].status=3; // Set status to 'IO'.
                process[queue_1[0]-1].current_IO_burst++; // Increment which IO burst process is on.
                process[queue_1[0]-1].IO_burst_timer=0; // Intialize IO burst time to 0.
                // $$ END Start process running in IO
            }
            clear_lead(queue_1, sizeof(queue_1)/sizeof(int)); // ...remove lead
            queue_1_next--;
            // $$ START Delegate CPU...
            if (queue_1[0]!=0){ // If there's some process in q1...
                // $$ START Start process running in q1
                process[queue_1[0]-1].status=2; // Set status to 'running'.
                //queue_1_quantum_timer=0;
                process[queue_1[0]-1].CPU_burst_timer=0; // Re-set burst counter to 0.
                //process[queue_1[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
                // $$ END Start process running in q1
                // $$ START set length of current CPU burst.
                if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts+1){ // If this is the last CPU burst...
                    if (process[queue_1[0]-1].norm==1){ // If norm...
                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                    }
                    else{ // If anom...
                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                    }
                }
                else{ // This is not the last CPU burst
                    process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                }
                // $$ END set length of current CPU burst.
            }
            else if ((queue_1[0]==0)&&(queue_2[0]!=0)){ // If there's something in q2 and nothing in q1 (because q1 gets priority)...
                CPU=2; // Give CPU to q2.
                process[queue_2[0]-1].dont_run_yet=1; // Don't run q2 process during Q2 because we ran a process here in Q1.
            }
            // $$ END Delegate CPU...
        }
        
        else if (queue_1_quantum_timer==queue_1_time_quantum-1){ // Time quantum expires...
            queue_1_quantum_timer=0; // Re-set quantum timer to zero.
            process[queue_1[0]-1].CPU_burst_timer++; // Increment burst timer.
            queue_2[queue_2_next]=queue_1[0]; // ...put q1 lead at tail of q2.
            process[queue_1[0]-1].dont_run_yet=1; // Indicate it was just preempted so it doesn't get run in the Q2 section.
            if (queue_2[0]!=0){ // If there's a process in q2...
                process[queue_2[0]-1].dont_run_yet=1; // Indicate that it shouldn't run this time around (because CPU is in q1 for this ms). 
            }
            process[queue_1[0]-1].preempt=1;
            process[queue_1[0]-1].status=1; // Set status to 'ready'.
            queue_2_next++; // Increment open slot in q2.
            clear_lead(queue_1, sizeof(queue_1)/sizeof(int)); // Shift processes in q1 up.
            queue_1_next--; // Decrement open slot in q1.
            if ((queue_1[0]==0)&&(new_process_imminent==0)&&(queue_2[0]!=0)){ // If there's something in q2 and nothing in q1 (because q1 gets priority)...
                //process[queue_2[0]-1].status=2;
                CPU=2; // Give CPU to q2.
            process[queue_2[0]-1].dont_run_yet=1; // Don't run q2 process during Q2 because we ran a process here in Q1.
            }
        }
        else { // If neither CPU burst nor time quantum is finished...
            queue_1_quantum_timer++; // One ms passes for running process.
            process[queue_1[0]-1].CPU_burst_timer++;
        }
    } // END 'running'
} // END "If there's a process in queue 1."
// END Consider queue 1.


// START Reset time quantum if it had to be extended in Q1
if (flag_reset_time_quantum_1==1){
    queue_1_time_quantum=atoi(argv[2]);
    flag_reset_time_quantum_1=0;
}
// END Reset time quantum if it had to be extended in Q1

// START Consider queue 2.
if (queue_2[0]!=0){ // If there's a process in queue 2.
    if (process[queue_2[0]-1].status==1){ // ready
        if (CPU==2){ // If the CPU has just been given to q2...
            // $$ START Start process running in q2
            process[queue_2[0]-1].status=2; // Set status to 'running'.
            //queue_1_quantum_timer=0; // Start time quantum timer.
            if (process[queue_2[0]-1].preempt==0){ // If it wasn't preempted, and therefore is starting a new burst...
                process[queue_2[0]-1].CPU_burst_timer=0; // Start CPU burst timer.
                //process[queue_2[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
            }
            // $$ END Start process running in q1
            // $$ START set length of current CPU burst.
            if (process[queue_2[0]-1].current_CPU_burst==process[queue_2[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                if (process[queue_2[0]-1].norm==1){ // If norm...
                    process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time; // Normal burst duration
               }
                else{ // If anom...
                    process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].anom_CPU_burst_time; // Anom burst duration
                }
            }
            else{ // This is not the last CPU burst
                process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time; // Normal burst duration
            }
            // $$ END set length of current CPU burst.
        }
    } // END 'ready'
    if ((process[queue_2[0]-1].status==2)&&(process[queue_2[0]-1].dont_run_yet==0)){ // running and DIDN'T just arrive in Q2
        // START Extend time quantum if necessary.
        if (process[queue_2[0]-1].current_CPU_burst==process[queue_2[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
            if (process[queue_2[0]-1].norm==0){ // If anom...
                process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].anom_CPU_burst_time;
                flag_reset_time_quantum_2=1;
            }
        }
        // END Extend time quantum if necessary.
        if (process[queue_2[0]-1].CPU_burst_timer==(process[queue_2[0]-1].current_CPU_burst_duration-1)){ // If it's end of CPU burst...
            queue_2_quantum_timer=0; // Re-set quantum timer to zero.
            process[queue_2[0]-1].preempt=0; // Note that it was not preempted, so burst timer will be re-set to 0.
            process[queue_2[0]-1].CPU_burst_timer=0; // Burst timer re-set to zero.
            queue_IO[queue_2[0]-1]=queue_2[0]; // Put process in IO.
            process[queue_2[0]-1].IO_flag=1; // Signal that it's just been moved to IO
            process[queue_2[0]-1].IO_burst_timer=0;
            process[queue_2[0]-1].old_queue=2; // Remember that old q was q2.
            process[queue_2[0]-1].current_CPU_burst++; // Increment CPU burst that process is on , for next time.
            if (process[queue_2[0]-1].current_CPU_burst==process[queue_2[0]-1].number_of_CPU_bursts+1){ // If this is the last CPU burst...
                if (process[queue_2[0]-1].norm==1){ // If norm
                    // $$ START Start process running in IO
                    process[queue_2[0]-1].status=3; // Set status to 'IO'.
                    process[queue_2[0]-1].current_IO_burst++; // Increment which IO burst process is on.
                    process[queue_2[0]-1].IO_burst_timer=0; // Intialize IO burst time to 0.
                    // $$ END Start process running in IO
                }
                else{ // If anom
                    process[queue_2[0]-1].status=4; // Set status to 'done'.
                    queue_done[queue_2[0]-1]=1;
                if (out[queue_2[0]-1]==0){ // If we haven't already stated that process is finished...
                    output[queue_2[0]][0]=process[queue_2[0]-1].process_id;
                    output[queue_2[0]][1]=t+1;
                    out[queue_2[0]-1]=1; // So we don't declare it again.
                }
                }
            }
            else { // If this is NOT the last CPU burst...
                // $$ START Start process running in IO
                process[queue_2[0]-1].status=3; // Set status to 'IO'.
                process[queue_2[0]-1].current_IO_burst++; // Increment which IO burst process is on.
                process[queue_2[0]-1].IO_burst_timer=0; // Intialize IO burst time to 0.
                // $$ END Start process running in IO
            }
            clear_lead(queue_2, sizeof(queue_2)/sizeof(int)); // ...remove lead
            queue_2_next--;
            // $$ START Delegate CPU...
            if (queue_1[0]!=0){ // If there's some process in q1...
                CPU=1;
                // $$ START Start process running in q1
                process[queue_1[0]-1].status=2; // Set status to 'running'.
                //queue_1_quantum_timer=0;
                process[queue_1[0]-1].CPU_burst_timer=0; // Re-set burst counter to 0.
                //process[queue_1[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
                // $$ END Start process running in q1
                // $$ START set length of current CPU burst.
                if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                    if (process[queue_1[0]-1].norm==1){ // If norm...
                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                    }
                    else{ // If anom...
                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                    }
                }
                else{ // This is not the last CPU burst
                    process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                }
                // $$ END set length of current CPU burst.
            }
            else if ((queue_1[0]==0)&&(queue_2[0]!=0)){ // If there's something in q2 and nothing in q1 (because q1 gets priority)...
                if (new_process_imminent==1){ // If there's going to be a process arriving (we should give CPU to q1)...
                    CPU=1; // Give CPU to q1 in anticipation of new process' arriving.
                }
                else{
                    CPU=2; // Give CPU back to q2 because q1 doesn't need it.
                }
            }
            // $$ END Delegate CPU...
        }
        else if (queue_2_quantum_timer==queue_2_time_quantum-1){ // Time quantum expires...
            queue_2_quantum_timer=0; // Re-set quantum timer.
            process[queue_2[0]-1].preempt=1; // Note that process was preempted.
            process[queue_2[0]-1].CPU_burst_timer++; // Increment burst time.
            if ((queue_1[0]==0)&&(new_process_imminent==0)){ // If q1 is empty and there's no imminent process...
                if (queue_2_next==1){ // If process getting preempted is only one in q2...
                    //process[queue_2[0]-1].CPU_burst_timer++; // Increment burst time.
                }
                else{ // If there are other processes in q2...
                    queue_2[queue_2_next]=queue_2[0]; // ...put q2 lead at tail of q2.
                    clear_lead(queue_2, sizeof(queue_2)/sizeof(int)); // ...remove lead
                    // $$ START Start process running in q2
                    process[queue_2[0]-1].status=2; // Set status to 'running'.
                    // $$ END Start process running in q1
                    // $$ START set length of current CPU burst.
                    if (process[queue_2[0]-1].current_CPU_burst==process[queue_2[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                        if (process[queue_2[0]-1].norm==1){ // If norm...
                            process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time;
                        }
                        else{ // If anom...
                            process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                        }
                    }
                    else{ // This is not the last CPU burst
                        process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time;
                    }
                    // $$ END set length of current CPU burst.
                }
            }
            else{ // If q1 is not empty or there's a process imminent...
                // $$ START preempt q2
                CPU=1;
                process[queue_2[0]-1].status=1; // Set status of current q2 lead (which we're preempting) to 'ready'.
                if (queue_2_next>1){ // If there are other processes in q2...
                    queue_2[queue_2_next]=queue_2[0]; // ...put q2 lead at tail of q2.
                    clear_lead(queue_2, sizeof(queue_2)/sizeof(int)); // ...remove lead
                }
                // $$ END preempt q2
                // $$ START Start process running in q1
                if (queue_1[0]!=0){ // If there's a process in q1 (as opposed to just an imminent one)...
                    process[queue_1[0]-1].status=2; // Set status to 'running'.
                    //queue_1_quantum_timer=0;
                    process[queue_1[0]-1].CPU_burst_timer=0; // Re-set burst counter to 0.
                    //process[queue_1[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
                    // $$ END Start process running in q1
                    // $$ START set length of current CPU burst.
                    if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                        if (process[queue_1[0]-1].norm==1){ // If norm...
                            process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                        }
                        else{ // If anom...
                            process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                        }
                    }
                    else{ // This is not the last CPU burst
                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                    }
                    // $$ END set length of current CPU burst.
                }
            }

        }
        else { // If neither CPU burst nor time quantum is finished...
            queue_2_quantum_timer++; // One ms passes for running process.
            process[queue_2[0]-1].CPU_burst_timer++;
        }
    } // END "running"
    process[queue_2[0]-1].dont_run_yet=0; // Signal that now that process CAN run (next time around).
} // END "If there's a process in queue 2."
// END Consider queue 2.

// START Reset time quantum if it had to be extended in Q2
if (flag_reset_time_quantum_2==1){
    queue_2_time_quantum=atoi(argv[3]);
    flag_reset_time_quantum_2=0;
}
// END Reset time quantum if it had to be extended in Q2

// START Consider IO queue.
for (j=0; j<number_of_processes; j++){ // For each slot in the IO cue...
    if (queue_IO[j]!=0){ // If the given slot isn't empty...
        if (process[queue_IO[j]-1].IO_flag==0){ // If process wasn't JUST put into IO in this time slot...
            if (process[queue_IO[j]-1].IO_burst_timer==process[queue_IO[j]-1].IO_duration-1){ // If IO burst finished...
                if (process[queue_IO[j]-1].current_IO_burst==process[queue_IO[j]-1].IO_frequency){ // If this is the last IO burst...
                    if (process[queue_IO[j]-1].norm==1){ // If norm...
                        process[queue_IO[j]-1].status=4; // Set status to 'done'.
                        queue_done[queue_IO[j]-1]=1; // Process finished
                        if (out[queue_IO[j]-1]==0){ // If we haven't already stated that process is finished...
                        output[queue_IO[j]][0]=process[queue_IO[j]-1].process_id;
                        output[queue_IO[j]][1]=t+1;
                        out[queue_IO[j]-1]=1; // So we don't declare it again.
                        }
                    }
                    else{ // If anom...
                        if (process[queue_IO[j]-1].old_queue==1){ // If old q was q1...
                            queue_1[queue_1_next]=process[queue_IO[j]-1].process_id; // Put process back in q1.                        
                            if (queue_1_next==0){ // If it's now the leading process in q1
                                if (queue_2_next==0){ // If there's nothing in q2
                                    // Set up for next CPU burst:
                                    CPU=1; // Give CPU to q1
                                    process[queue_IO[j]-1].status=2; // Set status to 'running'
                                    //process[queue_1[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
                                    process[queue_1[0]-1].CPU_burst_timer=0;
                                    // Set length of current CPU burst time
                                    if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                                    }
                                }
                                else{ // If something in q2...
                                    process[queue_1[0]-1].status=1; // Set staus to 'ready'
                                }
                            }
                        queue_1_next++;
                        }
                        else if (process[queue_IO[j]-1].old_queue==2){ // If old q was q2...
                            queue_2[queue_2_next]=process[queue_IO[j]-1].process_id; // Put process back in q2.                        
                            if (queue_2_next==0){ // If it's now the leading process in q2
                                if ((queue_1_next==0)&&(new_process_imminent==0)){ // If there's nothing in q1 and no process imminent...
                                    // Set up for next CPU burst:
                                    CPU=2; // Give CPU to q1
                                    process[queue_2[0]-1].status=2; // Set staus to 'running'
                                    //process[queue_2[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
                                    process[queue_2[0]-1].CPU_burst_timer=0;
                                    // Set length of current CPU burst time
                                    if (process[queue_2[0]-1].current_CPU_burst==process[queue_2[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                                        if (process[queue_1[0]-1].norm==1){ // If norm...
                                        process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time;
                                        }
                                        else{ // If anom...
                                        process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                                        }
                                    }
                                    else{ // NOT last CPU burst
                                        process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time;
                                    }
                                }
                                else{ // If something in q1 or process imminent...
                                    CPU=1;
                                    process[queue_2[0]-1].status=1; // Set staus to 'ready'.
                                }
                            }
                        queue_2_next++;
                        }
                    queue_IO[j]=0; // given slot in IO queue empty.
                    }
                } // END "If this is the last IO burst..."
                else { // If this is not the last IO burst...
                    //printf("old q=%d\n", process[queue_IO[j]-1].old_queue);
                    if (process[queue_IO[j]-1].old_queue==1){ // If old q was q1...
                        queue_1[queue_1_next]=process[queue_IO[j]-1].process_id; // Put process back in q1.                        
                            if (queue_1_next==0){ // If it's now the leading process in q1
                                if (queue_2_next==0){ // If there's nothing in q2...
                                    // Set up for next CPU burst:
                                    CPU=1; // Give CPU to q1
                                    process[queue_1[0]-1].status=2; // Set staus to 'running'
                                    //process[queue_1[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
                                    process[queue_1[0]-1].CPU_burst_timer=0;
                                    // Set length of current CPU burst time
                                    if (process[queue_1[0]-1].current_CPU_burst==process[queue_1[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                                        if (process[queue_1[0]-1].norm==1){ // If norm...
                                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                                        }
                                        else{ // If anom...
                                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                                        }
                                    }
                                    else{ // NOT last CPU burst
                                        process[queue_1[0]-1].current_CPU_burst_duration=process[queue_1[0]-1].CPU_burst_time;
                                    }
                                }
                                else{ // If something in q2...
                                    process[queue_1[0]-1].status=1; // Set status to 'ready'
                                }
                            }
                            process[queue_1[queue_1_next]-1].status=1; // Set status to 'ready'.
                            queue_1_next++;
                    }
                    else if (process[queue_IO[j]-1].old_queue==2){ // If old q was q2...
                        queue_2[queue_2_next]=process[queue_IO[j]-1].process_id; // Put process back in q2.                        
                        if (queue_2_next==0){ // If it's now the leading process in q2
                            if ((queue_1_next==0)&&(new_process_imminent==0)){ // If there's nothing in q1 and no process imminent...
                                // Set up for next CPU burst:
                                CPU=2; // Give CPU to q1
                                process[queue_IO[j]-1].status=2; // Set staus to 'running'
                                //process[queue_2[0]-1].current_CPU_burst++; // Increment CPU burst that process is on.
                                process[queue_2[0]-1].CPU_burst_timer=0;
                                // Set length of current CPU burst time
                                if (process[queue_2[0]-1].current_CPU_burst==process[queue_2[0]-1].number_of_CPU_bursts){ // If this is the last CPU burst...
                                    if (process[queue_2[0]-1].norm==1){ // If norm...
                                    process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time;
                                    }
                                    else{ // If anom...
                                        process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].anom_CPU_burst_time; // CPU burst will be shorter.
                                    }
                                }
                                else{ // Not last CPU burst
                                    process[queue_2[0]-1].current_CPU_burst_duration=process[queue_2[0]-1].CPU_burst_time;
                                }
                            }
                            else{ // If something in q1 or process imminent...
                                CPU=1;
                                process[queue_2[0]-1].status=1; // Set status to 'ready'.
                            }
                        }
                        process[queue_2[queue_2_next]-1].status=1; // Set status to 'ready'.
                        queue_2_next++;
                    }
                    queue_IO[j]=0; // given slot in IO queue empty. 
                } // END "If this is not the last IO burst..."
            } // END "If IO Burst finished"
            else { // If IO burst NOT finished
                process[queue_IO[j]-1].IO_burst_timer++;
            }
        } // END "If process wasn't JUST put into IO in this time slot..."
        else{ // If process WAS just put into IO in this time slot...
            process[queue_IO[j]-1].IO_flag=0; // Process is ok to have IO updated next time around.
        }
        //printf("IO_burst_timer=%d, IO_duration=%d, old_queue=%d, queue_1_quantum_timer=%d, queue_1_time_quantum=%d\n", process[0].IO_burst_timer, process[0].IO_duration, process[0].old_queue, queue_1_quantum_timer, queue_1_time_quantum);
        //printf("it is %d and %d and %d and %d and %d\n", process[4].IO_burst_timer, process[4].IO_duration, process[4].old_queue, queue_1_quantum_timer, queue_1_time_quantum);
        if ((queue_done[j]==0)&&(process[queue_IO[j]-1].IO_burst_timer==process[queue_IO[j]-1].IO_duration-1)&&(process[queue_IO[j]-1].old_queue==1)&&(queue_1_quantum_timer==queue_1_time_quantum-1)){ // If there's about to be a process in q1 that should be getting CPU...
            CPU=1;
        }
    }
}
// END Consider IO queue.


// START If there's an imminent process and no process in q2, give CPU to q1 
if ((queue_2[0]==0)&&(new_process_imminent==1)){ // If there's a new process imminent and nothing in q2...
    CPU=1; // Give CPU to q1.
}
// END If there's an imminent process and no process in q2, give CPU to q1 

// See if all processes have completed.
done_sum=0; // Initially assume none are finished.
for (j=0; j<number_of_processes; j++){ // For each process...
    if (queue_done[j]==1){ // If proces is finished...
        done_sum++; // Increment count.
    }
}
if (done_sum==number_of_processes){ // If ALL are finished...
    finished=1; // Set flag.
}

t++; // Increment time
} // END main for loop

char *filename; // Pointer to filename provided at command line.
int length=sizeof(argv[1])/sizeof(char); // Length of filename.
filename=argv[1];
char str[length];
strcpy (str, filename); //save filename in string.
strcat (str, ".out"); // Append '.out' to filename.
FILE *output_file; // Create file pointer for output file.
output_file=fopen(str, "w"); // Create output file to write to.
printf("ok %d\n", number_of_processes);
for (i=1; i<=number_of_processes; i++){ // For each process...
    if (i==number_of_processes){ // If last process...
        fprintf(output_file, "%d : %d", output[i][0], output[i][1]); // Print data, do not start new line.
    }
    else{ // If not last process...
        fprintf(output_file, "%d : %d\n", output[i][0], output[i][1]); // Print data, start new line.
    }
}
fclose(output_file); // Close output file.

}

void clear_lead(int* queue, int size){
    int l; // Loop index.
    for (l=0; l<size-1; l++){ // For each slot in queue...
        queue[l]=queue[l+1]; // Set contents equal to contents of next slot.
    }
    queue[size-1]=0; // Empty last slot.
}

void shift_back(int* queue, int size){
    int l; // Loop index.
    for (l=0; l<size-1; l++){ // For each slot in queue...
        queue[size-1-l]=queue[size-1-(l+1)]; // Set contents equal to contents of previous slot.
    }
    queue[0]=0; // Empty first slot.
}
