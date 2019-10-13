// Including required header files
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<time.h>
#include<pthread.h>

// Struct argument for parameter in threaded quick sort
typedef struct argument {
    int start;
    int end;
    int* structarr;
} argument;

// Concurrent quick sort
void concurrentquicksort(int arr[], int start, int end) {
    // If size of subarray is less than 5, do insertion sort
    if(start>=end) {
        return;
    } else if(end-start <= 4) {
        int i, j, p;
        for(int i=start+1;i<=end;i++) {
            p = arr[i];
            j = i-1;
            while(j>=start && arr[j]>p) {
                arr[j+1] = arr[j];
                j--;
            }
            arr[j+1] = p;
        }
        return;
    }
    // Seeding for the rand() function which is used for pivot selection
    srand(rand());
    int pivot = (rand() % (end-start+1)) + start;

    // Swapping pivot with end element of the subarray
    int temp = arr[pivot];
    arr[pivot] = arr[end];
    arr[end] = temp;
    int i=start;
    for(int j=start;j<end;j++) {
        if(arr[j]<=arr[end]) {
            int temp = arr[j];
            arr[j] = arr[i];
            arr[i++] = temp;
        }
    }

    // Swapping the pivot to it's actual place
    temp = arr[end];
    arr[end] = arr[i];
    arr[i] = temp;

    // Forking two times for left and right subparts separately
    pid_t pid1 = fork();
    if(pid1 == 0) {
        concurrentquicksort(arr, start, i-1);
        exit(1);
    } else if(pid1 > 0) {
        pid_t pid2 = fork();
        if(pid2 == 0) {
            concurrentquicksort(arr, i+1, end);
            exit(1);
        } else {
            int status;
            // Wait for left and right subparts to complete
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
        }
    }
    return;
}

// Threaded quick sort
void* threadedquicksort(void* arg) {
    // Decrypting argument to separate values
    argument* strarr = (argument*) arg;
    int start = strarr->start;
    int end = strarr->end;
    int* arr = strarr->structarr;

    // If size of subarray is less than 5, do insertion sort
    if(start>=end) {
        return NULL;
    } else if(end-start <= 4) {
        int i, j, p;
        for(int i=start+1;i<=end;i++) {
            p = arr[i];
            j = i-1;
            while(j>=start && arr[j]>p) {
                arr[j+1] = arr[j];
                j--;
            }
            arr[j+1] = p;
        }
        return NULL;
    }

    // Seeding for the rand() function which is used for pivot selection
    srand(rand());
    int pivot = (rand() % (end-start+1)) + start;

    // Swapping pivot with end element of the subarray
    int temp = arr[pivot];
    arr[pivot] = arr[end];
    arr[end] = temp;
    pivot = arr[end]; // Making end element of the subarray as the pivot
    
    // Quick sort partitioning
    int i=start, j=start;
    for(int j=start;j<end;j++) {
        if(arr[j]<=pivot) {
            int temp = arr[j];
            arr[j] = arr[i];
            arr[i++] = temp;
        }
    }

    // Swapping the pivot to it's actual place
    temp = pivot;
    arr[end] = arr[i];
    arr[i] = temp;
    
    // Declaring pthread_t variables for left and right subpart
    pthread_t ptid1, ptid2;

    // Declaring and assigning arguments for left and right subparts
    argument* arg1 = (argument*) malloc (sizeof(argument));
    argument* arg2 = (argument*) malloc (sizeof(argument));
    arg1->start = start;
    arg1->end = i-1;
    arg1->structarr = arr;
    arg2->start = i+1;
    arg2->end = end;
    arg2->structarr = arr;

    // Creating two threads separately for left and right subparts
    pthread_create(&ptid1, NULL, threadedquicksort, (void*)arg1);
    pthread_create(&ptid2, NULL, threadedquicksort, (void*)arg2);

    // Wait for left and right subpart to complete
    pthread_join(ptid1, NULL);
    pthread_join(ptid2, NULL);
    return NULL;
}

// Normal quick sort
void normalquicksort(int arr[], int start, int end) {
    // If size of subarray is less than 5, do insertion sort
    if(start>=end) {
        return;
    } else if(end-start <= 4) {
        int i, j, p;
        for(int i=start+1;i<=end;i++) {
            p = arr[i];
            j = i-1;
            while(j>=start && arr[j]>p) {
                arr[j+1] = arr[j];
                j--;
            }
            arr[j+1] = p;
        }
        return;
    }
    // Seeding for the rand() function which is used for pivot selection
    srand(rand());
    int pivot = (rand() % (end-start+1)) + start;

    // Swapping pivot with end element of the subarray
    int temp = arr[pivot];
    arr[pivot] = arr[end];
    arr[end] = temp;
    pivot = arr[end];
    int i=start;
    for(int j=start;j<end;j++) {
        if(arr[j]<=pivot) {
            int temp = arr[j];
            arr[j] = arr[i];
            arr[i++] = temp;
        }
    }

    // Swapping the pivot to it's actual place
    temp = pivot;
    arr[end] = arr[i];
    arr[i] = temp;

    // Calling left and right subparts separately
    normalquicksort(arr, start, i-1);
    normalquicksort(arr, i+1, end);
    return;
}

int main(void) {
    int inputlength;    // Input length of the array
    scanf("%d", &inputlength);

    // Creating arrays for threaded and normal quick sort
    int normalarr[inputlength], threadedarr[inputlength];

    // Allocating shared memory for concurrent quick sort
    key_t key = IPC_PRIVATE;
    int shmid = shmget(key, inputlength*4, 0644 | IPC_CREAT);
    int *concurrentarr = (int*) shmat (shmid, (void*)0, 0);

    // Input of the array
    for(int i=0;i<inputlength;i++) {
        scanf("%d", &normalarr[i]);
        threadedarr[i] = normalarr[i];
        concurrentarr[i] = normalarr[i];
    }

    clock_t time;   // Declaration of clock_t variable for further usage;

    // Normal quick sort
    time = clock();
    normalquicksort(normalarr, 0, inputlength-1);
    time = clock() - time;

    // Printing the final array after normal quick sort
    for(int i=0;i<inputlength;i++) {
        printf("%d ", normalarr[i]);
    }
    printf("\n");

    // Printing time taken by concurrent quick sort
    double normaltimetaken = ((double)time)/CLOCKS_PER_SEC;
    printf("Normal quick sort took %f secs to complete\n", normaltimetaken);

    // Concurrent quick sort
    time = clock();
    concurrentquicksort(concurrentarr, 0, inputlength-1);
    time = clock() - time;

    // Printing the final array after concurrent quick sort
    for(int i=0;i<inputlength;i++) {
        printf("%d ", concurrentarr[i]);
    }
    printf("\n");

    // Printing time taken by concurrent quick sort
    double concurrenttimetaken = ((double)time)/CLOCKS_PER_SEC;
    printf("Concurrent quick sort took %f secs to complete\n", concurrenttimetaken);

    // argument is struct used as parameter for thread function
    argument* arg = (argument*) malloc (sizeof(argument));
    arg->start = 0;
    arg->end = inputlength-1;
    arg->structarr = threadedarr;
    
    pthread_t ptid; // Declaring pthread_t variable

    // Threaded quick sort
    time = clock();
    pthread_create(&ptid, NULL, threadedquicksort, (void*)arg);
    pthread_join(ptid, NULL);
    time = clock() - time;

    // Printing the final array after threaded quick sort
    for(int i=0;i<inputlength;i++) {
        printf("%d ", threadedarr[i]);
    }
    printf("\n");

    // Printing time taken by threaded quick sort
    double threadedtimetaken = ((double)time)/CLOCKS_PER_SEC;
    printf("Threaded quick sort took %f secs to complete\n", threadedtimetaken);

    //Comparing concurrent quick sort and threaded quick sort
    printf("Concurrent quick sort is [%f] times faster than normal quick sort\n", normaltimetaken/concurrenttimetaken);
    printf("Threaded quick sort is [%f] times faster than normal quick sort\n", normaltimetaken/threadedtimetaken);
    printf("Concurrent quick sort is [%f] times faster than threaded quick sort\n", threadedtimetaken/concurrenttimetaken);

    // Deallocating shared memory
    shmdt(concurrentarr);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}