#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_READERS 5
#define NUM_WRITERS 2
#define ITERATIONS  6

typedef struct {
    int counter;
    sem_t mutex;
} Lightswitch;

static void lightswitch_init(Lightswitch *ls) {
    ls->counter = 0;
    sem_init(&ls->mutex, 0, 1);
}

static void lightswitch_lock(Lightswitch *ls, sem_t *sem) {
    sem_wait(&ls->mutex);
    ls->counter++;
    if (ls->counter == 1) {
        sem_wait(sem);              
    }
    sem_post(&ls->mutex);
}

static void lightswitch_unlock(Lightswitch *ls, sem_t *sem) {
    sem_wait(&ls->mutex);
    ls->counter--;
    if (ls->counter == 0) {
        sem_post(sem);              
    }
    sem_post(&ls->mutex);
}

static sem_t roomEmpty;
static sem_t turnstile;
static Lightswitch readSwitch;

static int sharedData = 0;          
static int activeReaders = 0;       
static int activeWriters = 0;
static pthread_mutex_t checkLock = PTHREAD_MUTEX_INITIALIZER;

static void *reader(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        sem_wait(&turnstile);       
        sem_post(&turnstile);

        lightswitch_lock(&readSwitch, &roomEmpty);


        pthread_mutex_lock(&checkLock);
        activeReaders++;
        if (activeWriters != 0) {
            printf("ERROR: lector junto a escritor\n");
        }
        printf("Lector %d lee %d (lectores activos: %d)\n", id, sharedData, activeReaders);
        pthread_mutex_unlock(&checkLock);
        usleep(rand() % 200000);         

        /*sleep para simuar tiempo de lectura y escritura 
        no afecta el programa se puede borrar.
        */

        pthread_mutex_lock(&checkLock);
        activeReaders--;
        pthread_mutex_unlock(&checkLock);


        lightswitch_unlock(&readSwitch, &roomEmpty);
        usleep(rand() % 200000);
    }
    return NULL;
}

static void *writer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        sem_wait(&turnstile);      
        sem_wait(&roomEmpty);      

       
        activeWriters++;
        if (activeReaders != 0 || activeWriters != 1) {
            printf("ERROR: escritor sin exclusividad\n");
        }
        sharedData++;
        printf("Escritor %d escribe %d\n", id, sharedData);
        usleep(rand() % 100000);
        activeWriters--;
        

        sem_post(&turnstile);
        sem_post(&roomEmpty);
        usleep(rand() % 200000);
    }
    return NULL;
}

int main(void) {
    pthread_t readers[NUM_READERS];
    pthread_t writers[NUM_WRITERS];
    int readerIds[NUM_READERS];
    int writerIds[NUM_WRITERS];

    srand(42);
    sem_init(&roomEmpty, 0, 1);
    sem_init(&turnstile, 0, 1);
    lightswitch_init(&readSwitch);

    for (int i = 0; i < NUM_READERS; i++) {
        readerIds[i] = i;
        pthread_create(&readers[i], NULL, reader, &readerIds[i]);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        writerIds[i] = i;
        pthread_create(&writers[i], NULL, writer, &writerIds[i]);
    }

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }

    printf("Valor final: %d (esperado %d)\n", sharedData, NUM_WRITERS * ITERATIONS);
    sem_destroy(&roomEmpty);
    sem_destroy(&turnstile);
    sem_destroy(&readSwitch.mutex);
    return 0;
}