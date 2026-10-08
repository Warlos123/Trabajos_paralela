#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_CHAIRS     4    
#define N_CUSTOMERS  15


static const int capacity = N_CHAIRS + 1;

static int customers = 0;
static sem_t mutex;
static sem_t customer;
static sem_t barber;
static sem_t customerDone;
static sem_t barberDone;

static int closing = 0;     
static int served = 0;
static int balked = 0;

static void *barberThread(void *arg) {
    (void)arg;
    for (;;) {
        sem_wait(&customer);          
        if (closing) {
            break;
        }
        sem_post(&barber);              

        printf("        Barbero corta el pelo\n");
        usleep(80000);                 

        sem_wait(&customerDone);
        sem_post(&barberDone);
    }
    printf("Barbero cierra la barbería\n");
    return NULL;
}

static void *customerThread(void *arg) {
    int id = *(int *)arg;

    sem_wait(&mutex);
    if (customers == capacity) {
        balked++;
        sem_post(&mutex);
        printf("Cliente %2d: barbería llena, se va\n", id);
        return NULL;                   
    }
    customers++;
    printf("Cliente %2d entra (en la barbería: %d)\n", id, customers);
    sem_post(&mutex);

    sem_post(&customer);              
    sem_wait(&barber);                  

    printf("Cliente %2d recibe su corte\n", id);   

    sem_post(&customerDone);
    sem_wait(&barberDone);

    sem_wait(&mutex);
    customers--;
    served++;
    sem_post(&mutex);
    printf("Cliente %2d sale\n", id);
    return NULL;
}

int main(void) {
    pthread_t barberT;
    pthread_t customerT[N_CUSTOMERS];
    int ids[N_CUSTOMERS];

    srand(3);
    sem_init(&mutex, 0, 1);
    sem_init(&customer, 0, 0);
    sem_init(&barber, 0, 0);
    sem_init(&customerDone, 0, 0);
    sem_init(&barberDone, 0, 0);

    pthread_create(&barberT, NULL, barberThread, NULL);

    for (int i = 0; i < N_CUSTOMERS; i++) {
        ids[i] = i;
        pthread_create(&customerT[i], NULL, customerThread, &ids[i]);
        usleep(rand() % 60000);         
    }
    for (int i = 0; i < N_CUSTOMERS; i++) {
        pthread_join(customerT[i], NULL);
    }

    closing = 1;
    sem_post(&customer);               
    pthread_join(barberT, NULL);

    printf("Atendidos: %d, se fueron: %d, total: %d\n", served, balked, served + balked);
    return 0;
}