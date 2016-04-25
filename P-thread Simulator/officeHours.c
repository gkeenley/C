#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Define variables associated with the mutex.
pthread_mutex_t count_mutex; // Mutex variable
pthread_cond_t count_threshold_cv; // Condition variable

// Define structure in which we will store student information, one of which will be passed to each thread.
struct List_question_counter{
  int *List_question_counter; // Array with one element for each student, value equal to question number that student is asking next.
};
struct List_office_occupancy{
  int *List_office_occupancy; // Array with one element for each student, value 1 or 0, indicating if student is in office or not, respectively.
};
struct List_left_office{
  int *List_left_office; // Array with one element for each student, value 1 or 0, indicating if student has asked all questions and left office or not, respectively.
};
struct student_data{ // Structure containing each student's index, our three student-relevant quantities entered at command line, and our three lists (arrays). 
  int student_id; // Index corresponding to each student.
  int number_of_students; // Total number of students (entered at command line).
  int questions_per_student; // Number of questions each student will ask (entered at command line).
  int office_capacity; // Total number of students allowed in office at once.
  struct List_question_counter List_question_counter; // 'Question number' array from line 11.
  struct List_office_occupancy List_office_occupancy; // 'Office occupancy' array from line 14.
  struct List_left_office List_left_office; // 'Left office' array from line 17.
};

void *individual_student_procedure(void *t); // Prototype of routine that each thread (each student) will execute.

int main (int argc, char *argv[])
{
  printf("\n"); // Skip a line after command line input.
  int number_of_students=atoi(argv[1]); // Total number of students (entered at command line).
  int office_capacity=atoi(argv[2]); // Total number of students allowed in office at once.
  int questions_per_student=atoi(argv[3]); // Number of questions each student will ask (entered at command line).
  struct student_data student_data[number_of_students]; // Create one 'student data' structure for each student.
  pthread_t threads[number_of_students]; // Create one identifier for each thread, which will be returned by each execution of the subroutine.
  pthread_attr_t attr; // Common attribute identifier for each thread.
  int j; // Loop index for main().

  if (number_of_students==0){ // If there are no students...
    printf("There are no students.\n\n"); // Terminate code.
  }

  else{ // If there ARE students///
    // Fill structure corresponding to first student with data.
    student_data[0].student_id=0; // First student's index is 0.
    student_data[0].number_of_students=number_of_students; // From command line.
    student_data[0].questions_per_student=questions_per_student; // From command line.
    student_data[0].office_capacity=office_capacity; // From command line.
    student_data[0].List_question_counter.List_question_counter=malloc(sizeof(struct student_data)+3*number_of_students*sizeof(int)); // Since each element is a pointer, we allocate memory.
    student_data[0].List_office_occupancy.List_office_occupancy=malloc(sizeof(struct student_data)+3*number_of_students*sizeof(int)); // Since each element is a pointer, we allocate memory.
    student_data[0].List_left_office.List_left_office=malloc(sizeof(struct student_data)+3*number_of_students*sizeof(int)); // Since each element is a pointer, we allocate memory.
    // Fill structures corresponding to all other students with data.
    if (number_of_students>1){ // If there are more students than just the one already considered above...
      for (j=1; j<number_of_students; j++){
        student_data[j].student_id=j;
        student_data[j].number_of_students=number_of_students;
        student_data[j].questions_per_student=questions_per_student;
        student_data[j].office_capacity=office_capacity;
        student_data[j].List_question_counter.List_question_counter=student_data[0].List_question_counter.List_question_counter;
        student_data[j].List_office_occupancy.List_office_occupancy=student_data[0].List_office_occupancy.List_office_occupancy;
        student_data[j].List_left_office.List_left_office=student_data[0].List_left_office.List_left_office;
      }
    }
    // Initialize variables associated with the mutex.
    pthread_mutex_init(&count_mutex, NULL); // Mutex variable.
    pthread_cond_init (&count_threshold_cv, NULL); // Condition variable.

    // Allow threads that we are about to create to be joinable.
    pthread_attr_init(&attr); // Initialize thread attribute variable.
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // Set thread detached attribute.

    // Create threads.
    for (j=0; j<number_of_students; j++){
      pthread_create(&threads[j], &attr, individual_student_procedure, (void *) &student_data[j]);
    }

    // Join threads.
    for (j=0; j<number_of_students; j++) {
      pthread_join(threads[j], NULL);
    }

    // Delete remaining thread variables, and free space allocated for pointers.
    pthread_attr_destroy(&attr); // Destroy thread attribute variable.
    pthread_mutex_destroy(&count_mutex); // Destroy mutex variable.
    pthread_cond_destroy(&count_threshold_cv); // Destroy condition variable.
    free(student_data[0].List_question_counter.List_question_counter);
    free(student_data[0].List_office_occupancy.List_office_occupancy);
    free(student_data[0].List_left_office.List_left_office);
  }
} // END main()


// Routine executed by each thread/student. 
void *individual_student_procedure(void *t)
{

// Set up structure that we will operate on in order to modify the corresponding structure (pointed to) in main().
struct student_data *student_data; // Create pointer to new 'student data' structure.
student_data=(struct student_data *) t; // Make our structure of pointers just created point to 'student data' structure created in main(), by casting input argument as pinter to 'student data' structure.

// Take values from structure in main pointed to by our new 'student data' structure, and set them equal to corresponding variables in this routine, in order to operate on them.
int student_id = student_data->student_id;
int number_of_students=student_data->number_of_students;
int questions_per_student=student_data->questions_per_student;
int office_capacity=student_data->office_capacity;
int *List_question_counter=student_data->List_question_counter.List_question_counter;
int *List_office_occupancy=student_data->List_office_occupancy.List_office_occupancy;
int *List_left_office=student_data->List_left_office.List_left_office;

// Other variables for this routine.
int full_or_not; // Boolean value indicating whether office is full or not.
int total_number_in_office; // Total number of students in the office.
int i; // Loop index for procedure.


// Operation that 'individual_student_procedure' routine performs.
while (List_question_counter[student_id]<=questions_per_student) // While the student still has question(s) to ask...
{
  pthread_mutex_lock(&count_mutex); // Lock mutex
  if ((List_question_counter[student_id]<=questions_per_student)&&(rand()%2==1)) // If student still has question(s) to ask, and is randomly selected for it to be his or her 'turn'.
  {
    total_number_in_office=0; // Initialize to zero for this iteration of while loop, so we re-calculate every time.
    for (i = 0; i < number_of_students; i++){ // For each student...
      total_number_in_office+=List_office_occupancy[i]; // Add student's contribution (0 or 1) to office occupancy.
    }
    if (total_number_in_office==office_capacity){ // If the number of students in the office is equal to the capacity...
      full_or_not=1; // Designate office as 'full'.
    }
    else{ // If the number of students in the office is NOT equal to the capacity (ie. lower)...
      full_or_not=0; // Designate office as 'not full'.
    }

    if (List_office_occupancy[student_id]==1){ // If given student is inside office...
      List_question_counter[student_id]++; // Increment question that given student is on.
      if (List_question_counter[student_id]>questions_per_student){ // If student has already asked the maximum number of questions... 
        if (questions_per_student==0){ // If students cannot ask any questions...
          printf("I am student %d and I am leaving the office because we cannot ask any questions.\n\n", student_id); // Student leaves office and states that it is because students cannot ask questions.
        }
        else{ // If students CAN ask questions...
          printf("I am student %d and I am leaving the office.\n\n", student_id); // Student leaves office, having had all questions answered.
        }
        List_office_occupancy[student_id]=0; // Indicate vacated space in office.
        List_left_office[student_id]=1; // Indicate that student has left office.
      }
      else if (List_question_counter[student_id]<=questions_per_student) // If student has not yet asked the maximum number of questions...
      {
        printf("I am student %ld and I am asking question %d.\n", student_id, List_question_counter[student_id]); // Student asks given question.
        printf("I am TA and this will be discussed in class.\n\n"); // TA immediately responds to every question.
      }
    }

    else if ((full_or_not==0)&&(List_office_occupancy[student_id]==0)&&(List_left_office[student_id]==0)){ // If office is not full, and student is waiting outside... 
      printf("I am student %ld and I am entering the office.\n\n",student_id); // Student enters.
      List_office_occupancy[student_id]=1; // Indicate that student is now in the office.
    }

    else if ((full_or_not==1)&&(List_office_occupancy[student_id]==0)&&(List_left_office[student_id]==0)){ // If office IS full, and student is waiting outside... 
      if (office_capacity==0){ // If there is not space for any students in the office...
        printf("I am student %ld and I am leaving because no one is allowed into the office.\n\n",student_id); // Student leaves, instead of waiting outside.
        List_question_counter[student_id]=questions_per_student+1;
      }
      else{ // If there IS space in office, student cannot get in simply because it is full, and therefore waits.
        printf("I am student %ld and I am waiting outside the office.\n\n",student_id);
      }
    }
  }
  pthread_mutex_unlock(&count_mutex); // Unlock mutex.
  sleep(1); // Perform operation that takes small amount of time, in order to allow for threads to alternate on mutex lock.
}
pthread_exit(NULL); // While loop completed, ie. given student has asked all questions and left office, so corresponding thread terminates.
} // END individual_student_procedure routine.
