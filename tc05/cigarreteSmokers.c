#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ROUNDS 20

static sem_t agentSem;
static sem_t tobacco;
static sem_t paper;
static sem_t match;


static sem_t mutex;
static sem_t tobaccoSem;   /* despierta al fumador que TIENE tabaco */
static sem_t paperSem;     /* despierta al fumador que TIENE papel */
static sem_t matchSem;     /* despierta al fumador que TIENE fósforos */
static int isTobacco = 0;
static int isPaper = 0;
static int isMatch = 0;

static int smokeCount[3] = {0, 0, 0};


static void *agent(void *arg) {
    (void)arg;
    for (int i = 0; i < ROUNDS; i++) {
        sem_wait(&agentSem);
        int choice = rand() % 3;
        if (choice == 0) {
            printf("Agente pone: tabaco + papel\n");
            sem_post(&tobacco);
            sem_post(&paper);
        } else if (choice == 1) {
            printf("Agente pone: papel + fósforos\n");
            sem_post(&paper);
            sem_post(&match);
        } else {
            printf("Agente pone: tabaco + fósforos\n");
            sem_post(&tobacco);
            sem_post(&match);
        }
    }
    sem_wait(&agentSem);   
    return NULL;
}


static void *pusherTobacco(void *arg) {
    (void)arg;
    for (;;) {
        sem_wait(&tobacco);
        sem_wait(&mutex);
        if (isPaper) {
            isPaper = 0;
            sem_post(&matchSem);        /* tabaco + papel -> fumador con fósforos */
        } else if (isMatch) {
            isMatch = 0;
            sem_post(&paperSem);        /* tabaco + fósforos -> fumador con papel */
        } else {
            isTobacco = 1;              /* llegó primero: se anota y espera al otro */
        }
        sem_post(&mutex);
    }
    return NULL;
}

static void *pusherPaper(void *arg) {
    (void)arg;
    for (;;) {
        sem_wait(&paper);
        sem_wait(&mutex);
        if (isTobacco) {
            isTobacco = 0;
            sem_post(&matchSem);
        } else if (isMatch) {
            isMatch = 0;
            sem_post(&tobaccoSem);
        } else {
            isPaper = 1;
        }
        sem_post(&mutex);
    }
    return NULL;
}

static void *pusherMatch(void *arg) {
    (void)arg;
    for (;;) {
        sem_wait(&match);
        sem_wait(&mutex);
        if (isTobacco) {
            isTobacco = 0;
            sem_post(&paperSem);
        } else if (isPaper) {
            isPaper = 0;
            sem_post(&tobaccoSem);
        } else {
            isMatch = 1;
        }
        sem_post(&mutex);
    }
    return NULL;
}


typedef struct {
    int id;
    const char *has;
    sem_t *wakeUp;
} Smoker;

static void *smoker(void *arg) {
    Smoker *s = (Smoker *)arg;
    for (;;) {
        sem_wait(s->wakeUp);
        printf("    Fumador con %s arma el cigarro\n", s->has);
        smokeCount[s->id]++;
        sem_post(&agentSem);            
        usleep(rand() % 50000);        
    }
    return NULL;
}

int main(void) {
    pthread_t agentThread;
    pthread_t pushers[3];
    pthread_t smokers[3];
    Smoker smokerData[3];

    srand(7);
    sem_init(&agentSem, 0, 1);
    sem_init(&tobacco, 0, 0);
    sem_init(&paper, 0, 0);
    sem_init(&match, 0, 0);
    sem_init(&mutex, 0, 1);
    sem_init(&tobaccoSem, 0, 0);
    sem_init(&paperSem, 0, 0);
    sem_init(&matchSem, 0, 0);

    smokerData[0] = (Smoker){0, "tabaco", &tobaccoSem};
    smokerData[1] = (Smoker){1, "papel", &paperSem};
    smokerData[2] = (Smoker){2, "fósforos", &matchSem};

    pthread_create(&pushers[0], NULL, pusherTobacco, NULL);
    pthread_create(&pushers[1], NULL, pusherPaper, NULL);
    pthread_create(&pushers[2], NULL, pusherMatch, NULL);
    for (int i = 0; i < 3; i++) {
        pthread_create(&smokers[i], NULL, smoker, &smokerData[i]);
    }
    pthread_create(&agentThread, NULL, agent, NULL);

    pthread_join(agentThread, NULL);

    int total = 0;
    for (int i = 0; i < 3; i++) {
        printf("Fumador con %s fumó %d veces\n", smokerData[i].has, smokeCount[i]);
        total += smokeCount[i];
    }
    printf("Total: %d (esperado %d)\n", total, ROUNDS);
    return 0;
}