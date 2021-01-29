#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define N_THREADS 4
#define RED 0

int **current, **next;
int size_x, size_y;

pthread_cond_t cond;
pthread_mutex_t mutex;


enum {DEAD=0, ALIVE, WALL_V, WALL_H};


typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count, color, n_threads;
} pth_barrier_t;

pth_barrier_t barrier;


void pth_barrier_init(pth_barrier_t *barrier, int n_threads)
{
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->cond, NULL);
    barrier->count = barrier->n_threads = n_threads;
    barrier->color = RED;
}


void pth_barrier(pth_barrier_t *barrier)
{
    pthread_mutex_lock(&barrier->mutex);
    int kolor = barrier->color;

    if ((--(barrier->count))) {
        while (kolor == barrier->color) pthread_cond_wait(&barrier->cond, &barrier->mutex);
    }
    else {
        barrier->count = barrier->n_threads;
        barrier->color = !barrier->color;
        pthread_cond_broadcast(&barrier->cond);
    }
    pthread_mutex_unlock(&barrier->mutex);
}



int **init_stage(int size_x, int size_y)
{
    int **ptr = NULL;
    int x, y;

    srand((unsigned int)time(NULL));

    ptr = malloc(sizeof(int *) * size_x);
    if (!ptr) goto fail;

    for (x = 0; x < size_x; ++x) {
        ptr[x] = malloc(sizeof(int) * size_y);
        if (!ptr[x]) goto fail;
    }

    for (y = 0; y < size_y; ++y) {
        for (x = 0; x < size_x; ++x)
            ptr[x][y] = rand()%2;
    }

    for (y = 0; y < size_y; ++y)
        ptr[0][y] = ptr[size_x - 1][y] = WALL_V;

    for (x = 0 ; x < size_x; ++x)
        ptr[x][0] = ptr[x][size_y - 1] = WALL_H;

    return ptr;

  fail:
    perror("malloc");
    exit(1);
}



void display(void)
{
    int x, y;
    for(y = 0; y < size_y; ++y){
        for(x = 0; x < size_x; ++x){
            switch (current[x][y]) {
            case WALL_H: printf("="); break;
            case WALL_V: printf("|"); break;
            case ALIVE : printf("\e[32mo\e[0m"); break;
            case DEAD  : printf(" "); break;
            }
        }
        printf("\n");
    }
}


void* next_cells(void *arg)
{
    int x, y, count;
    int tnum = *(int *)arg;

    while (1) {
        for (x = tnum+1; x < size_x - 1; x += N_THREADS) {
            for (y = 1; y < size_y - 1; ++y) {
                count = 0;

                if (current[x-1][y-1] == ALIVE) ++count;
                if (current[x][y-1]   == ALIVE) ++count;
                if (current[x+1][y-1] == ALIVE) ++count;

                if (current[x-1][y] == ALIVE) ++count;
                if (current[x+1][y] == ALIVE) ++count;


                if (current[x-1][y+1] == ALIVE) ++count;
                if (current[x][y+1]   == ALIVE) ++count;
                if (current[x+1][y+1] == ALIVE) ++count;

                if (count <= 1 || count >= 4)
                    next[x][y] = DEAD;
                else if (current[x][y] == DEAD && count == 3)
                    next[x][y] = ALIVE;
                else if (current[x][y] == ALIVE && (count == 2 || count == 3))
                    next[x][y] = ALIVE;

            }
        }
        pth_barrier(&barrier);
        for (x = tnum; x < size_x; x += N_THREADS)
            memcpy(current[x], next[x], sizeof(int)*size_y);

        pth_barrier(&barrier);
        if (tnum == 0) {
            pthread_cond_signal(&cond);
            usleep(50000);
        }
        pth_barrier(&barrier);

    }



    return NULL;
}


void* free_stage(int **stage)
{
    for (int x = 0; x < size_x; ++x) {
        free(stage[x]);
    }

    free(stage);

    return NULL;
}


int main(void)
{
    struct winsize size;
    pthread_t *handle;

    if (ioctl(1, TIOCGWINSZ, &size) == -1)
        return 1;

    size_x = size.ws_col;
    size_y = size.ws_row - 2;

    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    pth_barrier_init(&barrier, N_THREADS);

    current = init_stage(size_x, size_y);
    next    = init_stage(size_x, size_y);
    handle = malloc(sizeof(pthread_t) * N_THREADS);


    for (int i = 0; i < N_THREADS; ++i) {
        int *tnum = malloc(sizeof(int));
        *tnum = i;
        pthread_create(&handle[i], NULL, next_cells, (void *)tnum);
    }

    while (1) {
        pthread_cond_wait(&cond, &mutex);
        system("clear");
        display();
    }

    current = free_stage(current);
    next    = free_stage(next);

    return 0;
}
