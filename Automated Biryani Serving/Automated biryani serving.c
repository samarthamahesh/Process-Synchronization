// Including required header files
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

int m, n, k;    // Number of chefs, NUmber of tables, Number of students
int *biryani, *table; // Biryani array and table array

pthread_t *chefs, *tables, *students;   // Threads for chefs, tables and students
pthread_mutex_t biryanimutex, tablemutex;  // Mutex lock
pthread_mutex_t *cheflock, *tablelock;

// Struct for chef with chef ID
typedef struct chefstruct {
    int id;
} chefstruct;

// Struct for table with table ID
typedef struct tablestruct {
    int id;
} tablestruct;

// Struct for student with student ID and arrival time
typedef struct studentstruct {
    int id;
    int arrivalTime;
} studentstruct;

// Struct for biryani
typedef struct biryaninode {
    int chefid;
    int vessels;
    int capacity;
    struct biryaninode* next;
} biryaninode;

// Struct for table for implementation of linked list
typedef struct tablenode {
    int tableid;
    int slots;
    struct tablenode* next;
} tablenode;

// Linked lists for biryani and table
struct biryaninode* biryanihead = NULL;
struct tablenode* tablehead = NULL;
struct biryaninode* biryanitail = NULL;
struct tablenode* tabletail = NULL;

void biryani_ready(int id) {
    // Check if all biryano vessels of chef with id=id are emptied and if yes, return
    while(1) {
        // Lock resource to restrict other threads from accessing it
        pthread_mutex_lock(&cheflock[id]);
        if(biryani[id] == 0) {
            // Unlock resource before returning
            pthread_mutex_unlock(&cheflock[id]);

            printf("All vessels prepared by chef %d are emptied. Resuming cooking now\n", id);
            return;
        }

        // Unlock resource after use
        pthread_mutex_unlock(&cheflock[id]);
    }
    return;
}

void *prepare_biryani(void *arg) {
    chefstruct *chefpara = (chefstruct*) arg;
    int chefid = chefpara->id;

    // Chef can prepare biryanis for any number of rounds
    while(1) {
        int r = (rand() % 10) + 1;
        int p = (rand() % 26) + 25;
        printf("Chef %d is preparing %d vessels of biryani\n", chefid, r);
        int w = (rand() % 4) + 2;

        // Sleep for preparation time of biryani
        sleep(w);

        // Update number of biryanis
        pthread_mutex_lock(&cheflock[chefid]);
        biryani[chefid] = r;
        pthread_mutex_lock(&biryanimutex);

        // Add this chef into linked list to load biryanis later
        biryaninode* node = (biryaninode*) malloc (sizeof(biryaninode));
        node->chefid = chefid;
        node->vessels = r;
        node->capacity = p;
        node->next = NULL;
        if(biryanihead == NULL) {
            biryanihead = node;
            biryanitail = node;
        } else {
            biryanitail->next = node;
            biryanitail = node;
        }
        pthread_mutex_unlock(&biryanimutex);
        pthread_mutex_unlock(&cheflock[chefid]);

        // Call biryani_ready function after biryanis are ready
        printf("Chef %d has prepared %d vessels of biryanis with %d portions each. Waiting for all vessels to be emptied to resume cooking\n", chefid, r, p);
        biryani_ready(chefid);  
    }
    return NULL;
}

void ready_to_serve_table(int id) {
    // Check if all slots are finished in table
    while(1) {
        // Lock resource
        pthread_mutex_lock(&tablelock[id]);
        if(table[id] == 0) {
            printf("All slots in table %d are finished\n", id);

            // Unlock resource before returning
            pthread_mutex_unlock(&tablelock[id]);
            return;
        }

        // Unlock resource
        pthread_mutex_unlock(&tablelock[id]);
    }
    return;
}

void *wait_to_get_full(void *arg) {
    // Decrypt arguments
    tablestruct *tablepara = (tablestruct*) arg;
    int tableid = tablepara->id;
    int remainingbiryaniflag = 0;
    int remainingbiryaniportions;
    int capacity;
    int printflag = 1;

    // Each table can assign slots for any number of rounds
    while(1) {
        int number_of_slots = (rand() % 10) + 1;

        // If table container is empty
        if(remainingbiryaniflag == 0) {
            if(printflag == 1) {
                printf("Serving container of table %d is empty, waiting to refill\n", tableid);
                printflag = 0;
            }

            // check for any chef if biryani is ready or not
            pthread_mutex_lock(&biryanimutex);
            if(biryanihead == NULL) {
                pthread_mutex_unlock(&biryanimutex);
                continue;
            }
            pthread_mutex_unlock(&biryanimutex);

            // Load biryani into tablr container
            printflag = 1;
            remainingbiryaniflag = 1;
            pthread_mutex_lock(&biryanimutex);
            biryani[biryanihead->chefid]--;
            biryanihead->vessels--;
            remainingbiryaniportions = biryanihead->capacity;
            printf("Chef %d is refilling serving container of serving table %d\n", biryanihead->chefid, tableid);
            printf("Serving container of table %d is refilled by chef %d. Table %d is resuming to serve now\n", tableid, biryanihead->chefid, tableid);
            pthread_mutex_unlock(&biryanimutex);
        }

        // Slots of table will be minimum of random numner and portions remaining in table container
        capacity = remainingbiryaniportions > number_of_slots ? number_of_slots : remainingbiryaniportions;
        
        // Updating slots available in table array
        pthread_mutex_lock(&tablelock[tableid]);
        table[tableid] = capacity;
        pthread_mutex_unlock(&tablelock[tableid]);

        // Adding tablr into linked list
        struct tablenode* node = (struct tablenode*) malloc (sizeof(struct tablenode));
        node->tableid = tableid;
        node->slots = capacity;
        node->next = NULL;

        // Lock resource
        pthread_mutex_lock(&tablemutex);
        if(tablehead == NULL) {
            tablehead = node;
            tabletail = node;
        } else {
            tabletail->next = node;
            tabletail = node;
        }
        // Unlock resource
        pthread_mutex_unlock(&tablemutex);
        printf("Serving table %d is ready to serve with %d slots\n", tableid, capacity);
        printf("Serving table %d is entering serving phase\n", tableid);

        // Call ready_to_serve function after slots are ready
        ready_to_serve_table(tableid);

        // Subtract slots from container portions to get remaining portions left
        remainingbiryaniportions -= capacity;
        if(remainingbiryaniportions == 0) {
            remainingbiryaniflag = 0;
        }

        // Update in linked list also
        pthread_mutex_lock(&biryanimutex);
        biryanihead->capacity -= capacity;
        if(biryanihead->vessels == 0) {
            if(biryanihead == biryanitail) {
                biryanitail = NULL;
            }
            biryanihead = biryanihead->next;
        }
        pthread_mutex_unlock(&biryanimutex);
    }
}

void student_in_slot(void) {
    // Wait for student to finish eating
    int eattime = (rand() % 6) + 5;
    sleep(eattime);
}

void *wait_for_slot(void *arg) {
    studentstruct *studentpara = (studentstruct*) arg;
    int studentid = studentpara->id;

    // Sleep for arrival time of student
    sleep(studentpara->arrivalTime);
    printf("Student %d has arrived and is waiting to be allocated a slot on a serving table\n", studentid);
    
    // Check if any table is free
    while(1) {
        int flag = 0;
        
        // Lock resource
        pthread_mutex_lock(&tablemutex);
        if(tablehead != NULL) {
            flag = 1;
        }
        // Unlock resource
        pthread_mutex_unlock(&tablemutex);
        if(flag == 1) {
            break;
        }
    }

    // Update the slots in that table 
    pthread_mutex_lock(&tablemutex);
    pthread_mutex_lock(&tablelock[tablehead->tableid]);
    table[tablehead->tableid]--;
    pthread_mutex_unlock(&tablelock[tablehead->tableid]);
    tablehead->slots--;
    printf("Student %d is assigned a slot on serving table %d and is waiting to be served\n", studentid, tablehead->tableid);
    int curtable = tablehead->tableid;

    // Remove tablehead if all slots of that table are finished
    if(tablehead->slots == 0) {
        if(tablehead == tabletail) {
            tabletail = NULL;
        }
        tablehead = tablehead->next;
    }
    pthread_mutex_unlock(&tablemutex);

    // Cal studet_in_slot function
    student_in_slot();

    printf("Student %d on serving table %d has been served\n", studentid, curtable);
}

int main(void) {
    // Initializing mutex locks
    pthread_mutex_init(&biryanimutex, NULL);
    pthread_mutex_init(&tablemutex, NULL);

    // Scanning inputs
    scanf("%d %d %d", &m, &n, &k);
    int arrival[k];
    for(int i=0;i<k;i++) {
        // Arrival times can be random or taken from user
        // scanf("%d", &arrival[i]);
        arrival[i] = (rand() % 30) + 1;
    }

    // Allocating dynamic memory
    biryani = (int*) malloc (sizeof(int)*(m+1));
    table = (int*) malloc (sizeof(int)*(n+1));

    // Allocating dynamic memory for threads
    chefs = (pthread_t *) malloc (sizeof(pthread_t)*m);
    tables = (pthread_t *) malloc (sizeof(pthread_t)*n);
    students = (pthread_t *) malloc (sizeof(pthread_t)*k);
    cheflock = (pthread_mutex_t *) malloc (sizeof(pthread_mutex_t)*(m+1));
    tablelock = (pthread_mutex_t *) malloc (sizeof(pthread_mutex_t)*(n+1));

    // Initializing mutex locks
    for(int i=0;i<=m;i++) {
        pthread_mutex_init(&cheflock[i], NULL);
    }
    for(int i=0;i<=n;i++) {
        pthread_mutex_init(&tablelock[i], NULL);
    }

    // Creating threads for chefs
    for(int i=0;i<m;i++) {
        chefstruct *chefpara = (chefstruct*) malloc (sizeof(chefstruct));
        chefpara->id = i+1;
        pthread_create(&chefs[i], NULL, prepare_biryani, (void*)chefpara);
    }

    // Creating threads for tables
    for(int i=0;i<n;i++) {
        tablestruct *tablepara = (tablestruct*) malloc (sizeof(tablestruct));
        tablepara->id = i+1;
        pthread_create(&tables[i], NULL, wait_to_get_full, (void*)tablepara);
    }

    // Creating threads for students
    for(int i=0;i<k;i++) {
        studentstruct *studentpara = (studentstruct*) malloc (sizeof(studentstruct));
        studentpara->id = i+1;
        studentpara->arrivalTime = arrival[i];
        pthread_create(&students[i], NULL, wait_for_slot, (void*)studentpara);
    }

    // Join for all students
    for(int i=0;i<k;i++) {
        pthread_join(students[i], NULL);
    }

    // Simulation over
    printf("Served all students successfully\n");
    printf("Simulation over\n");

    return 0;
}