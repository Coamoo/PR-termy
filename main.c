#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#define MSG_CR_REQUEST 100
#define MSG_RESPONSE 150
#define MSG_SIZE 4
#define NUMTHREADS 2
#define ID 0
#define TL 1
#define CR 2
#define SEX 3
#define K 10 //Klienci
#define M 2  //Szafki

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
    for (int i = 0; i < size; i++)
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
    for (int i = 0; i < size; i++)
    {
        if (Queue[i].cloakroom == Queue[rank].cloakroom && Queue[i].TLast < TLast && i != rank)
        {
            numberOfPeople++;
        }
    }
    return numberOfPeople;
}

int queueSize()
{
    int numberOfAcceptances = 0;
    for (int i = 0; i < size; i++)
    {
        if (Queue[i].cloakroom != -1)
        {
            numberOfAcceptances++;
        }
    }
    return numberOfAcceptances;
}

int isOtherSexInCloakroom()
{
    for (int i = 0; i < size; i++)
    {
        if (Queue[i].cloakroom == Queue[rank].cloakroom  && Queue[i].TLast < TLast && i != rank && Queue[i].type != Queue[rank].type)
        {
            return 1;
        }pthread_mutex_lock
    }
    return 0;
}

void receiveAllAcceptances()
{
    while(1)
    {
        MPI_Recv(msg, MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, MSG_RESPONSE, MPI_COMM_WORLD, &status);
        int id_j = msg[ID];
        Queue[id_j].TLast = msg[TL];
        Queue[id_j].type = msg[SEX];
        Queue[id_j].cloakroom = msg[CR];
        if(!(isOtherSexInCloakroom()) && countPeopleInCloakroom() < M && countPeopleInCloakroom() == size))
        {
            break;
        }
    }
}

void sendInformationToEveryone()
{
    msg[ID] = rank;
    msg[TL] = T;
    msg[CR] = -2;
    msg[SEX] = Queue[rank].type;
    for (int i = 0; i < size; i++)
    {
        if (i == rank)
        {
            continue;
        }
        MPI_Send(msg, MSG_SIZE, MPI_INT, i, MSG_RESPONSE, MPI_COMM_WORLD);
    }
}

void *receiver(void *arg)
{
    while (1)
    {
        MPI_Recv(msg, MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, MSG_CR_REQUEST, MPI_COMM_WORLD, &status);
        pthread_mutex_lock( &mutex );
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
        pthread_mutex_unlock( &mutex );
        MPI_Send(msg, MSG_SIZE, MPI_INT, id_j, MSG_RESPONSE, MPI_COMM_WORLD);
    }
}

void *sender(void *arg);
{
    while (1)
    {
        entryTheCloakroom();
        receiveAllAcceptances();
        goChangeAndSwim();
        sendInformationToEveryone();
        emptyTheQueue();
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

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size >= 3 * M)
    {
        Queue[rank].type = chooseGender();
        emptyTheQueue();
        MPI_Get_processor_name(processor_name, &namelen);
        printf("Jestem %d z %d na %s\n", rank, size, processor_name);
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