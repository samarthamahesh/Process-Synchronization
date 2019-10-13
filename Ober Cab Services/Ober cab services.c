// Including required header files
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<time.h>
#include<sys/sem.h>

// defining cab types
#define UNDEFINED 0
#define POOLRIDE 1
#define PREMIERRIDE 2

// defining states of cab/driver
#define waitState 0
#define onRidePoolOne 1
#define onRidePoolFull 2
#define onRidePremier 4

// Number of drivers, rider, payment servers and driverstate and payment progress
int n, m, k;
int *driverstate, *paymentprogress;

// mutex locks
pthread_mutex_t paymentmutex, progressmutex, driverstatemutex, queuemutex;

// Rider and payment server threads
pthread_t *riderthreads, *serverthreads;

// Semaphores
sem_t waitcabsem, poolonecabsem, poolfullcabsem, premiercabsem, paymentserversem;

// Details of rider
typedef struct riderstruct {
    int id;
    int arrivalTime;
    int cabType;
    int maxWaitTime;
    int rideTime;
} riderstruct;

// drivernode for linked list to check poolone state
typedef struct drivernode {
    int id;
    struct drivernode* next;
} drivernode;

// Payment server details
typedef struct serverstruct {
    int id;
} serverstruct;

// paymentnode for linked list for sending request to payment server
typedef struct paymentnode {
    int riderid;
    int cabType;
    struct paymentnode *next;
} paymentnode;

// Linked list for poolone state
struct drivernode* poolhead;
struct drivernode* pooltail;

// Linked list for payment
struct paymentnode* paymenthead;
struct paymentnode* paymenttail;

// Initialize functions
void* driverthread(void* arg);
void makePayment(riderstruct *arg);

// Function called by rider thread
void* bookCab(void* arg) {
    // Decrypting the argument
    riderstruct *rider = (riderstruct*) arg;
    int riderid = rider->id;
    int arrivalTime = rider->arrivalTime;
    int cabType = rider->cabType;
    int maxWaitTime = rider->maxWaitTime;
    int rideTime = rider->rideTime;

    // Wait for arrival time of rider
    sleep(arrivalTime);

    if(cabType == POOLRIDE) {
        printf("Rider %d has booked POOL ride and is waiting to get accepted by a driver\n", riderid);

        // We need to check whether any driver with onRidePoolOne state are there
        pthread_mutex_lock(&queuemutex);
        if(poolhead == NULL) {
            pthread_mutex_unlock(&queuemutex);

            // Rider exits the system after max wait time if he is not assigned a cab
            // If he is assigned, decrement waiting cabs by 1 (sem_wait/sem_timedwait)
            struct timespec tm;
            clock_gettime(CLOCK_REALTIME, &tm);
            tm.tv_sec += maxWaitTime;
            if(sem_timedwait(&waitcabsem, &tm) == -1) {
                printf("Rider %d cancelled his ride due to longer wait time\n", riderid);
                return NULL;
            }

            // Increment poolone cabs by 1 (sem_post)
            sem_post(&poolonecabsem);

            // Assign a driver with suitable state
            int driverforthis;
            drivernode *driver = (drivernode*) malloc (sizeof(drivernode));
            for(int i=1;i<=n;i++) {
                pthread_mutex_lock(&driverstatemutex);
                if(driverstate[i] == waitState) {
                    driver->id = i;
                    driverstate[i] = onRidePoolOne;
                    driverforthis = i;
                    pthread_mutex_unlock(&driverstatemutex);
                    break;
                }
                pthread_mutex_unlock(&driverstatemutex);
            }

            // Add this cab into linked list
            pthread_mutex_lock(&queuemutex);
            if(poolhead == NULL) {
                poolhead = driver;
                pooltail = driver;
            } else {
                pooltail->next = driver;
                pooltail = driver;
            }
            pthread_mutex_unlock(&queuemutex);

            printf("POOL ride confirmed for rider %d\n", riderid);
            printf("Driver %d picked up rider %d. Ride in progress\n", driverforthis, riderid);
            
            // Sleep for ride time of rider
            sleep(rideTime);
            printf("POOL ride completed for rider %d\n", riderid);

            // Update driverstate array and semaphores accordingly
            pthread_mutex_lock(&driverstatemutex);
            if(driverstate[driverforthis] == onRidePoolOne) {
                driverstate[driverforthis] = waitState;
                sem_post(&waitcabsem);
                sem_wait(&poolonecabsem);
            } else if(driverstate[driverforthis] = onRidePoolFull) {
                driverstate[driverforthis] = onRidePoolOne;
                sem_post(&poolonecabsem);
                sem_wait(&poolfullcabsem);
            }
            pthread_mutex_unlock(&driverstatemutex);
        } else {
            // If there is any driver with state onRidePoolOne
            pthread_mutex_unlock(&queuemutex);

            // Rider exits the system after max wait time if he is not assigned a cab
            // If he is assigned, decrement poolone cabs by 1 (sem_wait/sem_timedwait)
            struct timespec tm;
            clock_gettime(CLOCK_REALTIME, &tm);
            tm.tv_sec += maxWaitTime;
            if(sem_timedwait(&poolonecabsem, &tm) == -1) {
                printf("Rider %d cancelled his ride due to longer wait time\n", riderid);
                return NULL;
            }
            sem_post(&poolfullcabsem);

            // Assign a driver
            int driverforthis;
            for(int i=1;i<=n;i++) {
                pthread_mutex_lock(&driverstatemutex);
                if(driverstate[i] == onRidePoolOne) {
                    driverforthis = i;
                    driverstate[i] = onRidePoolFull;
                    pthread_mutex_unlock(&driverstatemutex);
                    break;
                }
                pthread_mutex_unlock(&driverstatemutex);
            }

            printf("POOL ride confirmed for rider %d\n", riderid);
            printf("Driver %d picked up rider %d. Ride in progress\n", driverforthis, riderid);
            
            // Sleep for ride time of rider
            sleep(rideTime);

            printf("POOL ride completed for rider %d\n", riderid);

            // Update the state of driver/cab in driverstate array using locks
            pthread_mutex_lock(&driverstatemutex);
            if(driverstate[driverforthis] == onRidePoolOne) {
                driverstate[driverforthis] = waitState;
                sem_post(&waitcabsem);
                sem_wait(&poolonecabsem);
            } else if(driverstate[driverforthis] == onRidePoolFull) {
                driverstate[driverforthis] = onRidePoolOne;
                sem_post(&poolonecabsem);
                sem_wait(&poolfullcabsem);
            }
            pthread_mutex_unlock(&driverstatemutex);
        }
    } else if(cabType == PREMIERRIDE) {
        printf("Rider %d has booked PREMIER ride and is waiting to get accepted by a driver\n", riderid);
        
        // Rider exits the system after max wait time if he is not assigned a cab
        // If he is assigned, change semaphores accordingly
        struct timespec tm;
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += maxWaitTime;
        if(sem_timedwait(&waitcabsem, &tm) == -1) {
            printf("Rider %d cancelled his ride due to longer wait time\n", riderid);
            return NULL;
        }
        sem_post(&premiercabsem);

        // Assign driver
        int driverforthis;
        for(int i=1;i<=n;i++) {
            pthread_mutex_lock(&driverstatemutex);
            if(driverstate[i] == waitState) {
                driverstate[i] = onRidePremier;
                driverforthis = i;
                pthread_mutex_unlock(&driverstatemutex);
                break;
            }
            pthread_mutex_unlock(&driverstatemutex);
        }

        printf("PREMIER ride confirmed for rider %d.\n", riderid);
        printf("Driver %d picked up rider %d. Ride in progress\n", driverforthis, riderid);

        // Sleep for ride time of rider
        sleep(rideTime);

        // Update driverstate array
        pthread_mutex_lock(&driverstatemutex);
        driverstate[driverforthis] = waitState;
        pthread_mutex_unlock(&driverstatemutex);

        // Change semaphores accordingly
        sem_post(&waitcabsem);
        sem_wait(&premiercabsem);
        printf("PREMIER ride completed for rider %d\n", riderid);
    }

    // Proceed for payment
    makePayment(rider);

    // Exit from thread after payment is done
    pthread_exit(NULL);
}

void makePayment(riderstruct *arg) {
    // Creating a payment node to send a request to pament server to process
    paymentnode *node = (paymentnode*) malloc (sizeof(paymentnode));
    node->riderid = arg->id;
    node->cabType = arg->cabType;
    node->next = NULL;

    // Locking the linked list for access
    pthread_mutex_lock(&paymentmutex);
    if(paymenthead == NULL) {
        paymenthead = node;
        paymenttail = node;
    } else {
        paymenttail->next = node;
        paymenttail = node;
    }

    // Unlocking linked list
    pthread_mutex_unlock(&paymentmutex);
    
    // Waiting for payment to finish by checking acknowledgement in paymentprogress array by locking it
    while(1) {
        pthread_mutex_lock(&progressmutex);
        if(paymentprogress[arg->id] == 1) {
            pthread_mutex_unlock(&progressmutex);
            printf("Payment is successfully completed for rider %d\n",node->riderid);
            break;
        }
        pthread_mutex_unlock(&progressmutex);
    }
    return;
}

void* acceptPayment(void* arg) {
    while(1) {
        // Locking paymentmutex
        pthread_mutex_lock(&paymentmutex);

        // Checking whether there is any rider whose payment has to be done
        if(paymenthead == NULL) {
            pthread_mutex_unlock(&paymentmutex);
            continue;
        }

        // Copying the rider details to process payment and remove that rider from linked list
        paymentnode *node = paymenthead;
        if(paymenthead == paymenttail) {
            paymenttail = NULL;
        }
        paymenthead = paymenthead->next;

        // Unloking paymentmutex
        pthread_mutex_unlock(&paymentmutex);

        // Payment in process for 2 secs
        printf("Payment in process for rider %d in payment server %d\n", node->riderid, ((serverstruct*)arg)->id);
        sleep(2);

        // Updating paymentprogress array by locking it
        pthread_mutex_lock(&progressmutex);
        paymentprogress[node->riderid] = 1;
        pthread_mutex_unlock(&progressmutex);
        sleep(0.5);
    }
}

int main(void) {
    // Initializing muex locks
    pthread_mutex_init(&paymentmutex, NULL);
    pthread_mutex_init(&progressmutex, NULL);
    pthread_mutex_init(&driverstatemutex, NULL);
    pthread_mutex_init(&queuemutex, NULL);

    // Scanning number of drivers, rider and payment servers
    printf("Number of drivers, Number of riders, Number of payment servers\n");
    scanf("%d %d %d", &n, &m, &k);

    // Scanning booking details of each rider
    int riderinfo[m][4];
    printf("For each rider, enter\nArrival time, Cab type(1->POOL, 2->PREMIER), Max wait time, Ride time\n");
    for(int i=0;i<m;i++) {
        // Arrival times, cab type, max wait time, ride time can be taken from input or they can be random
        // scanf("%d %d %d %d", &riderinfo[i][0], &riderinfo[i][1], &riderinfo[i][2], &riderinfo[i][3]);
        riderinfo[i][0] = (rand() % 10) + 1;
        riderinfo[i][1] = (rand() % 2) + 1;
        riderinfo[i][2] = (rand() % 4) + 3;
        riderinfo[i][3] = (rand() % 9) + 3;
    }

    // Allocating dynamic memory for driverstate(state of a driver in cab) and paymentprogress(payment status)
    driverstate = (int*) malloc (sizeof(int)*(n+1));
    paymentprogress = (int*) malloc (sizeof(int)*(m+1));

    // Allocating dynamic memory for threads
    riderthreads = (pthread_t*) malloc (sizeof(pthread_t)*m);
    serverthreads = (pthread_t*) malloc (sizeof(pthread_t)*k);

    // Initializing semaphores
    sem_init(&waitcabsem, 0, n);
    sem_init(&poolonecabsem, 0, 0);
    sem_init(&poolfullcabsem, 0, 0);
    sem_init(&premiercabsem, 0, 0);
    sem_init(&paymentserversem, 0, k);

    // Creating payment server threads
    for(int i=0;i<k;i++) {
        serverstruct *server = (serverstruct*) malloc (sizeof(serverstruct));
        server->id = i+1;
        pthread_create(&serverthreads[i], NULL, acceptPayment, (void*)server);
    }

    // Creating rider threads with respective details as arguments
    for(int i=0;i<m;i++) {
        riderstruct *rider = (riderstruct*) malloc (sizeof(riderstruct));
        rider->id = i+1;
        rider->arrivalTime = riderinfo[i][0];
        rider->cabType = riderinfo[i][1];
        rider->maxWaitTime = riderinfo[i][2];
        rider->rideTime = riderinfo[i][3];
        pthread_create(&riderthreads[i], NULL, bookCab, (void*)rider);
    }

    // Joining M rider threads
    for(int i=0;i<m;i++) {
        pthread_join(riderthreads[i], NULL);
    }

    printf("Simulation over\n");
    return 0;
}