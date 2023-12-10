// Mehmet Nadi 56102231
// Khoi Pham 91404433
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct
{
    int *buf;    /* Buffer array */
    int n;       /* Maximum number of slots */
    int front;   /* buf[(front+1)%n] is first item */
    int rear;    /* buf[rear%n] is last item */
    sem_t mutex; /* Protects accesses to buf */
    sem_t slots; /* Counts available slots */
    sem_t items; /* Counts available items */
    int item_count_per_producer;
    int delay_for_producers;
} sbuf_t;

sbuf_t shared_buffer;

/* Create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n, int item_count_per_producer, int delay_for_producers)
{
    sp->buf = calloc(n, sizeof(int));
    sp->n = n;                  /* Buffer holds max of n items */
    sp->front = sp->rear = 0;   /* Empty buffer iff front == rear */
    sem_init(&sp->mutex, 0, 1); /* Binary semaphore for locking */
    sem_init(&sp->slots, 0, n); /* Initially, buf has n empty slots */
    sem_init(&sp->items, 0, 0); /* Initially, buf has zero data items */
    sp->item_count_per_producer = item_count_per_producer;
    sp->delay_for_producers = delay_for_producers;
}

/* Clean up buffer sp */
void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}

/* Insert item onto the rear of shared buffer sp */
void sbuf_insert(sbuf_t *sp, int item, int id)
{
    sem_wait(&sp->slots);                   /* Wait for available slot */
    sem_wait(&sp->mutex);                   /* Lock the buffer */
    sp->buf[(++sp->rear) % (sp->n)] = item; /* Insert the item */
    printf("producer_%d produced item %d\n", id, item);
    sem_post(&sp->mutex); /* Unlock the buffer */
    sem_post(&sp->items); /* Announce available item */
}

/* Remove and return the first item from buffer sp */
int sbuf_remove(sbuf_t *sp, int id)
{
    int item;
    sem_wait(&sp->items);                    /* Wait for available item */
    sem_wait(&sp->mutex);                    /* Lock the buffer */
    item = sp->buf[(++sp->front) % (sp->n)]; /* Remove the item */
    printf("consumer_%d consumed item %d\n", id, item);
    sem_post(&sp->mutex); /* Unlock the buffer */
    sem_post(&sp->slots); /* Announce available slot */
    return item;
}

void *producer(void *vargp)
{
    int myid = *((int *)vargp);
    free(vargp);
    // printf("I'm producer %d\n", myid);

    int start_item_num = myid * shared_buffer.item_count_per_producer;
    int end_item_num = start_item_num + shared_buffer.item_count_per_producer - 1;
    for (int item_num = start_item_num; item_num <= end_item_num; item_num++)
    {
        sbuf_insert(&shared_buffer, item_num, myid);
        if (shared_buffer.delay_for_producers == 1)
        {
            usleep(500000);
        }
    }
    return NULL;
}

void *consumer(void *vargp)
{
    int myid = *((int *)vargp);
    free(vargp);
    // printf("I'm consumer %d\n", myid);

    while (1)
    {
        sbuf_remove(&shared_buffer, myid);
        if (shared_buffer.delay_for_producers == 0)
        {
            usleep(500000);
        }
    }
}

int main(int argc, char *argv[])
{
    int producer_count = 0;
    int consumer_count = 0;
    int item_count_per_producer = 0;
    int buffer_size = 0;
    int delay_for_producers = 0; // 1 means 0.5s delay for producer, 0 means 0.5s delay for consumers
    if (argc >= 6)
    {
        producer_count = atoi(argv[1]);
        consumer_count = atoi(argv[2]);
        item_count_per_producer = atoi(argv[3]);
        buffer_size = atoi(argv[4]);
        delay_for_producers = atoi(argv[5]);
    }
    else
    {
        printf("ERROR: Not enough arguments\n");
        exit(0);
    }

    if (producer_count > 16 || consumer_count > 16)
    {
        printf("ERROR: Maximum number of producers/consumers is 16 each\n");
        exit(1);
    }
    if (consumer_count >= producer_count * item_count_per_producer)
    {
        printf("ERROR: Number of consumers must be less than total items produced\n");
        exit(1);
    }

    sbuf_init(&shared_buffer, buffer_size, item_count_per_producer, delay_for_producers);

    // Create consumer threads
    pthread_t consumers[consumer_count];
    for (int i = 0; i < consumer_count; i++)
    {
        int *consumer_number = malloc(sizeof(int));
        *consumer_number = i;
        pthread_create(&consumers[i], NULL, consumer, consumer_number);
    }

    // Create producer threads
    pthread_t producers[producer_count];
    for (int i = 0; i < producer_count; i++)
    {
        int *producer_number = malloc(sizeof(int));
        *producer_number = i;
        pthread_create(&producers[i], NULL, producer, producer_number);
    }

    // Reap producer threads
    for (int i = 0; i < producer_count; i++)
    {
        pthread_join(producers[i], NULL);
    }

    // Wait until the buffer is empty
    int curr_empty_slots;
    do
    {
        if (sem_getvalue(&shared_buffer.slots, &curr_empty_slots) == -1)
        {
            printf("ERROR: sem_getvalue\n");
            exit(0);
        }
    } while (curr_empty_slots != shared_buffer.n);

    // Since the producer has finished producing and the buffer is empty,
    // the consumers have finished consuming everything, thus ready to be killed
    for (int i = 0; i < consumer_count; i++)
    {
        pthread_cancel(consumers[i]);
    }
    exit(0);
}

/*
TESTED WITH:
$ ./executable 2 4 10 8 0
$ ./executable 2 4 10 8 1
$ ./executable 3 4 5 8 1
$ ./executable 10 4 5 8 0
$ ./executable 16 16 10 8 2
$ ./executable 16 16 10 8
$ ./executable 17 10 10 8 2
$ ./executable 10 17 10 8 2
*/