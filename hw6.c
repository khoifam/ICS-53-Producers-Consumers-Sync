// Mehmet Nadi 56102231
// Khoi Pham 91404433
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

typedef struct {
    int *buf; /* Buffer array */
    int n; /* Maximum number of slots */
    int front; /* buf[(front+1)%n] is first item */
    int rear; /* buf[rear%n] is last item */
    sem_t mutex; /* Protects accesses to buf */
    sem_t slots; /* Counts available slots */
    sem_t items; /* Counts available items */
} sbuf_t;

/* Create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = calloc(n, sizeof(int));
    sp->n = n; /* Buffer holds max of n items */
    sp->front = sp->rear = 0; /* Empty buffer iff front == rear */
    sem_init(&sp->mutex, 0, 1); /* Binary semaphore for locking */
    sem_init(&sp->slots, 0, n); /* Initially, buf has n empty slots */
    sem_init(&sp->items, 0, 0); /* Initially, buf has zero data items */
}

/* Clean up buffer sp */
void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}

/* Insert item onto the rear of shared buffer sp */
void sbuf_insert(sbuf_t *sp, int item)
{
    sem_wait(&sp->slots); /* Wait for available slot */
    sem_wait(&sp->mutex); /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item; /* Insert the item */
    sem_post(&sp->mutex); /* Unlock the buffer */
    sem_post(&sp->items); /* Announce available item */
}

/* Remove and return the first item from buffer sp */
int sbuf_remove(sbuf_t *sp)
{
    int item;
    sem_wait(&sp->items); /* Wait for available item */
    sem_wait(&sp->mutex); /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)]; /* Remove the item */
    sem_post(&sp->mutex); /* Unlock the buffer */
    sem_post(&sp->slots); /* Announce available slot */
    return item;
}

sbuf_t shared_buffer;

int main(int argc, char *argv[])
{
    int producer_count = 0;
    int consumer_count = 0;
    int producer_item_count = 0;
    int buffer_size = 0;
    int delay_for_producers = 0; // 1 means 0.5s delay for producer, 0 means 0.5s delay for consumers 
    if (argc >= 6)
    {
        producer_count = atoi(argv[1]);
        consumer_count = atoi(argv[2]);
        producer_item_count = atoi(argv[3]);
        buffer_size = atoi(argv[4]);
        delay_for_producers = atoi(argv[5]);
    }
    else
    {
        printf("ERROR: Not enough arguments\n");
        exit(0);
    }

    sbuf_init(&shared_buffer, buffer_size);
    
}