#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define N_THREADS  6
#define ITERATIONS 20

static int room1 = 0;
static int room2 = 0;
static sem_t mutex;     
static sem_t t1;      
static sem_t t2;      

static long sharedCounter = 0;
static long entries[N_THREADS];

static void morris_lock(void) {
    sem_wait(&mutex);
    room1++;
    sem_post(&mutex);

    sem_wait(&t1);
    room2++;           
    sem_wait(&mutex);
    room1--;
    if (room1 == 0) {
        sem_post(&mutex);
        sem_post(&t2);  
    } else {
        sem_post(&mutex);
        sem_post(&t1);  
    }

    sem_wait(&t2);
    room2--;          
}

static void morris_unlock(void) {
    if (room2 == 0) {
        sem_post(&t1);  
    } else {
        sem_post(&t2);  
    }
}

static void *worker(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        morris_lock();
    
        sharedCounter++;
        entries[id]++;

        morris_unlock();
    }
    return NULL;
}

int main(void) {
    pthread_t threads[N_THREADS];
    int ids[N_THREADS];

    sem_init(&mutex, 0, 1);
    sem_init(&t1, 0, 1);
    sem_init(&t2, 0, 0);

    for (int i = 0; i < N_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < N_THREADS; i++) {
        printf("Hilo %d entró %ld veces\n", i, entries[i]);
    }
    printf("Contador: %ld (esperado %d)\n", sharedCounter, N_THREADS * ITERATIONS);
    return 0;
}