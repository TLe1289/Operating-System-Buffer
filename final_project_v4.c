/*
Name:       Youjian Tang, Travis Tran, Tommy Le
UID:        U30756616, U27438581, U52555883
Email:      {tang24, travist, tle568} @usf.edu
    COP4600 Final Project - Problem 1 - Baboon crossing the rope
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h> 
#include <unistd.h>

#define MAX_BABOONS 3
#define BUFFER 256


/// @brief struct contains all baboons' direction and their number
typedef struct
{
    char direction[BUFFER];
    int count;
} BABOON;


/// @brief enum for the direction
typedef enum 
{ 
    LEFT = 'L',
    RIGHT = 'R' 
} DIRECTION;


/// @brief struct contains the properties of the lope
typedef struct 
{
    int capacity;       // maximum capacity of baboons on rope
    int travel_time;    // time that baboons to travel through the rope
    int left_count;     // indiciate the current baboons travelling from the left
    int right_count;    // indiciate the current baboons travelling from the right
    sem_t sem_left;     // semaphore used to prevent baboons from the left entering the rope
    sem_t sem_right;    // semaphore used to prevent baboons from the right entering the rope
    sem_t sem_counter;  // semaphore used to limit the capacity of the baboons traveling the rope
    sem_t sem_announce; // semaphore used to declare the direction of the rope
} ROPE;


/// @brief struct contains the parameters passing to thread runner process
typedef struct
{
    ROPE* rope;
    DIRECTION direction;
} ThreadArgs;


int BABOON_open_file(const char *FILE_NAME, BABOON *baboons);
void BABOON_print_seqeunce(BABOON baboom);


void ROPE_init(ROPE* rope, int capacity, int travel_time);
void ROPE_destroy(ROPE* rope);
void ROPE_announce(ROPE* rope, DIRECTION direction);
void ROPE_mount(ROPE* rope, DIRECTION direction);
void ROPE_dismount(ROPE* rope, DIRECTION direction);
void *ROPE_cross(void *args);


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Proper command line usage is: \n<program_name> <filename> <travel_time>\n");
        return 1;
    }

    pthread_attr_t attr;
    int scope;
    pthread_attr_init(&attr);
    if (pthread_attr_getscope(&attr, &scope) != 0)
    {
        fprintf(stderr, "Unable to get scheduling scope\n");
    }
    else
    {
        if (scope == PTHREAD_SCOPE_PROCESS)
            printf("PTHREAD_SCOPE_PROCESS\n\n\n");
        else if (scope == PTHREAD_SCOPE_SYSTEM)
            printf("PTHREAD_SCOPE_SYSTEM\n\n\n");
        else
            fprintf(stderr, "Illegal scope value.\n");
    }

    printf("Start of Simulation\n\n");

    // Define baboons and get all baboons' directions from the file (argv[1])
    BABOON baboons;
    BABOON_open_file(argv[1], &baboons);
    BABOON_print_seqeunce(baboons);

    // Obtain the travel time from argv[2]
    int travel_time = atoi(argv[2]);

    // Define the rope and initiate all semaphores
    ROPE* rope = (ROPE*)malloc(sizeof(ROPE));
    ROPE_init(rope, MAX_BABOONS, travel_time);

    printf("Baboons are crossing:\n");

    // Define the size of maximum threads to be created at one batch
    pthread_t threads[rope->capacity];
    ThreadArgs* threadArgs[rope->capacity];

    // Define the index to the baboon sequence
    int baboon_index = 0;

    // Define the number of baboons remaining for crossing
    int baboons_remain = baboons.count;

    // Threads are creating in batch with maximum size of rope's capacity
    // This allows the program to take a large input since all threads are
    // not creating in the very start
    while (baboons_remain > 0)
    {
        // Define tha batch size
        int batch_count = 1;

        // Announce the direction for the current batch
        char baboon_direction = baboons.direction[baboon_index];

        // Find the current batch size.
        // All baboons in the batch must contain the same direction
        // The batch size cannot be greater than the rope capacity
        for (int index = 1 + baboon_index; index < rope->capacity + baboon_index; index++)
            if (baboon_direction == baboons.direction[index])
                batch_count++;
            else
                break;

        // Allow the current batch to cross the rope
        for (int index = 0; index < batch_count; index++)
        {
            // Define the thread parameters
            threadArgs[index] = (ThreadArgs*)malloc(sizeof(ThreadArgs));
            threadArgs[index]->rope = rope;
            threadArgs[index]->direction = (DIRECTION)baboons.direction[baboon_index];

            // Create threads for each baboon in the batch
            pthread_create(&threads[index], &attr, ROPE_cross, threadArgs[index]);

            // Increment the actual baboon index
            baboon_index++;
        }

        // Release resources for the current batch
        for (int index = 0; index < batch_count; index++)
        {
            // free thread
            pthread_join(threads[index], NULL);

            // free heap
            free(threadArgs[index]);
        }

        // Update the count of remaining baboons
        baboons_remain -= batch_count;
    }

    // free rope resources
    ROPE_destroy(rope);
    free(rope);

    printf("\n\nEnd of Termination\n\n");

    return 0;
}

/// @brief Read the baboon sequence into an array
/// @param FILE_NAME name to the text files
/// @param baboons object used to store the sequence
/// @return status; 0 means successful read
int BABOON_open_file(const char *FILE_NAME, BABOON *baboons)
{
    FILE *file;
    char temp[BUFFER];
    int index = 0;
    int c;

    if (!(file = fopen(FILE_NAME, "r")))
    {
        printf("Unable to open input file.\n");
        return -1;
    }

    // read the sequence into a temp var
    while ((c = getc(file)) != EOF)
    {
        if (c == LEFT || c == RIGHT)
            temp[index++] = c;
    }

    fclose(file);

    // copy the sequence into the baboon object
    if (index > 0)
    {
        strncpy(baboons->direction, temp, sizeof(temp));
        baboons->count = index;
        return 0;
    }
    else
    {
        return -2;
    }
}

/// @brief print the baboon sequence
/// @param baboon object containing the sequence
void BABOON_print_seqeunce(BABOON baboon)
{
    printf("Original Sequence: \n");
    for (int index = 0; index < baboon.count; index++)
    {
        printf("%c ", baboon.direction[index]);
    }
    printf("\n\n");
}

/// @brief Initiate the rope variables and semaphores
/// @param rope the target object for initiation
/// @param capacity the maximum capacity that the rope can hold
/// @param travel_time the time to travle the rope
void ROPE_init(ROPE* rope, int capacity, int travel_time)
{
    rope->capacity = capacity;
    rope->travel_time = travel_time;
    rope->left_count = 0;
    rope->right_count = 0;
    sem_init(&rope->sem_left, 0, 1);
    sem_init(&rope->sem_right, 0, 1);
    sem_init(&rope->sem_counter, 0, capacity);
    sem_init(&rope->sem_announce, 0, 1);
}

/// @brief Release all semaphores created for the thread control
/// @param rope the target object for destory
void ROPE_destroy(ROPE* rope)
{
    sem_destroy(&rope->sem_left);
    sem_destroy(&rope->sem_right);
    sem_destroy(&rope->sem_counter);
    sem_destroy(&rope->sem_announce);
}

/// @brief Declare the current bacth's direction and access the rope
/// @param rope the shared resource
/// @param direction baboon's direction
void ROPE_announce(ROPE *rope, DIRECTION direction)
{
    // only one baboon can access the rope at one time
    sem_wait(&rope->sem_announce);

    // left side access the rope
    if (direction == LEFT)
    {
        sem_wait(&rope->sem_left);
        rope->left_count++;
        if (rope->left_count == 1)
            sem_wait(&rope->sem_right);
        sem_post(&rope->sem_left);
    }
    // right side access the rope
    else if (direction == RIGHT)
    {
        sem_wait(&rope->sem_right);
        rope->right_count++;
        if (rope->right_count == 1)
            sem_wait(&rope->sem_left);
        sem_post(&rope->sem_right);
    }

    sem_post(&rope->sem_announce);
}

/// @brief baboon mount and crossing the rope
/// @param rope the shared resource
/// @param direction baboon's direction
void ROPE_mount(ROPE *rope, DIRECTION direction)
{
    // limit the number of baboons that can cross the rope
    sem_wait(&rope->sem_counter);
    sleep(rope->travel_time);
    printf("%c ", direction);
    fflush(stdout);
    sem_post(&rope->sem_counter);
}

/// @brief baboon dismount from the rope after crossing
/// @param rope the shared resource
/// @param direction baboon's direction
void ROPE_dismount(ROPE *rope, DIRECTION direction)
{
    // Release the rope if the current count = 0 on the rope
    if (direction == LEFT)
    {
        sem_wait(&rope->sem_left);
        rope->left_count--;
        if (rope->left_count == 0)
            sem_post(&rope->sem_right);
        sem_post(&rope->sem_left);

        
    }
    else if (direction == RIGHT)
    {
        sem_wait(&rope->sem_right);
        rope->right_count--;
        if (rope->right_count == 0)
            sem_post(&rope->sem_left);
        sem_post(&rope->sem_right);
    }
}

/// @brief thread runner function for baboon crossing
/// @param args thread arguments
/// @return NULL
void *ROPE_cross(void *args)
{
    ThreadArgs *a = (ThreadArgs*)args;

    ROPE_announce(a->rope, a->direction);
    ROPE_mount(a->rope, a->direction);
    ROPE_dismount(a->rope, a->direction);

    return NULL;
}
