#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#define MSG_CR_REQUEST 100
#define MSG_RESPONSE 150
#define MSG_SIZE 4
#define NUMTHREADS 2
#define MINIUM_PRIORITY 2147483647
#define ID 0
#define TL 1
#define CR 2
#define SEX 3
#define K 10
#define M 2

typedef struct _LOCAL_QUEUE
{
    int type;
    int cloakroom;
    int TLast;
} LOCAL_QUEUE, *PLOCAL_QUEUE;

int rank, size, T = 0, TLast = 0, recivedMessages = 0;

LOCAL_QUEUE Queue[K];
int msg[MSG_SIZE];
MPI_Status status;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void goChangeAndSwim(int rank)
{
    int randNumber = rand() % 10 + 1;
    printf("%d: Idę się kąpać na %d\n", rank, randNumber);
    sleep(randNumber);
}

int chooseGender()
{
    return rand() % 2;
}

int chooseCloakroom()
{
    return rand() % 3;
}

void emptyTheQueue()
{
    for (int i = 0; i < size; i++)
    {
        Queue[i].cloakroom = -1;
    }
}

int isQueueEmpty()
{
    return (Queue[rank].cloakroom + 1) ? 1 : 0;
}

void entryTheCloakroom()
{
    TLast = T;
    Queue[rank].TLast = T;
    Queue[rank].cloakroom = chooseCloakroom();
    msg[ID] = rank;
    msg[TL] = TLast;
    msg[CR] = Queue[rank].cloakroom;
    msg[SEX] = Queue[rank].type;
    for (int i = 0; i < rank; i++)
    {
        if (i == rank)
        {
            continue;
        }
        MPI_Send(msg, MSG_SIZE, MPI_INT, i, MSG_CR_REQUEST, MPI_COMM_WORLD);
    }
}

int countPeopleInCloakroom()
{
    int numberOfPeople = 0;
    for (int i = 0; i < K; i++)
    {
        if (Queue[i].TLast < TLast && Queue[i].cloakroom == Queue[rank].cloakroom)
        {
            numberOfPeople++;
        }
    }
    return numberOfPeople;
}

int countAcceptances()
{
    int numberOfAcceptances = 0;
    for (int i = 0; i < K; i++)
    {
        if (Queue[i].cloakroom != -1)
        {
            numberOfAcceptances++;
        }
    }
    return numberOfAcceptances;
}

//TODO coś do odbierania akceptacji?

void *receiver(void *arg)
{
    while (1)
    {
        MPI_Recv(msg, MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        //TODO: mutex?
        T = max(T, msg[TL]) + 1;
        int id_j = msg[ID];
        if (isQueueEmpty())
        {
            msg[ID] = rank;
            msg[TL] = T;
            msg[CR] = Queue[rank].cloakroom;
            msg[SEX] = Queue[rank].type;
        }
        else
        {
            msg[ID] = rank;
            msg[TL] = TLast;
            msg[CR] = Queue[rank].cloakroom;
            msg[SEX] = Queue[rank].type;
        }
        MPI_Send(msg, MSG_SIZE, MPI_INT, id_j, MSG_RESPONSE, MPI_COMM_WORLD);
    }
}

void *sender(MPI_INT, (rank + 1) % size, MSG_HELLO, MPI_COMM_WORLD);
{
    while (1)
    {
        if (msg[0] == 10)
        {
            msg[1] = 1;
            MPI_Send(msg, MSG_SIZE, MPI_INT, (rank + 1) % size, MSG_HELLO, MPI_COMM_WORLD);
            break;
        }
        pthread_mutex_lock(&mutex);
        while (visited != 1)
        {
            printf("%d: Blokowanie\n", rank);
            pthread_cond_wait(&cond, &mutex);
        }
        printf("%d: Sekcja krytyczna\n", rank);
        visited = 0;
        msg[0]++;
        doSmth(rank);
        MPI_Send(msg, MSG_SIZE, MPI_INT, (rank + 1) % size, MSG_HELLO, MPI_COMM_WORLD);
    }
}

int main(int argc, char **argv)
{
    srandom(2213);
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int namelen;
    int i[1];
    i[0] = 1;
    pthread_t threads[NUMTHREADS];

    msg[0] = 2;
    msg[1] = 0;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size >= 3 * M)
    {
        Queue[rank].type = chooseGender();
        Queue[rank].cloakroom = -1;
        MPI_Get_processor_name(processor_name, &namelen);
        printf("Jestem %d z %d na %s\n", rank, size, processor_name);
        if (rank == 0)
        {
            MPI_Send(msg, MSG_SIZE, MPI_INT, (rank + 1) % size, MSG_HELLO, MPI_COMM_WORLD);
        }
        pthread_create(&threads[0], NULL, sender, (void *)i);
        pthread_create(&threads[1], NULL, receiver, (void *)i);
        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);
    }
    else
    {
        printf("Do term przyszło za mało klientów, %d", size);
    }

    MPI_Finalize();
}