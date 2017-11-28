#include <stdio.h>     // perror; printf
#include <unistd.h>    // fork
#include <sys/wait.h>  // wait()
#include <sys/mman.h>  // mmap, munmap
#include <semaphore.h> // sem_open(), sem_destroy(), sem_wait(), sem_post(), sem_t
#include <fcntl.h>     // O_CREAT, O_EXEC
#include <stdlib.h>    /* srand, rand */
#include <time.h>      /* time */
#include <string.h>

#define N 5 // num of philosophers

typedef struct
{
    sem_t *chopsticks[N];
    int choosing_priority[N];
    int priority[N];
} table;

/* GLOBALS */
table *T;

char sem_names[N][3];

int fill_sem_names();
void close_sem();
void starving_philosopher(int);
void philosopher(int);
int MEALS = 2;


int main(int args, char *argv[])
{
    srand(time(NULL));
    fill_sem_names();
    int starving = 0;
    if(args > 1 && !strcmp(argv[1], "starving") ) starving = 1;
    if(args > 2) MEALS = atoi(argv[2]);
    
    if(starving)
        printf("A philosopher may starve to death!\n");
    else 
        printf("Every philosopher will eat fill - no starvation.\n");

    // init table
    T = mmap(NULL, sizeof(table), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    for (int i = 0; i < N; ++i)
    {
        T->chopsticks[i] = sem_open(sem_names[i], O_CREAT | O_EXCL, 0644, 1);
        T->priority[i] = 0;
        T->choosing_priority[i] = 0;
    }

    pid_t pid;

    for (int i = 0; i < N; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            // child process
            if( starving) starving_philosopher(i);
            else philosopher(i);
            break;
        }
        else if (pid < 0)
        {
            // check for error in forking process
            close_sem();
            printf("ERROR: Fork failed");
        }
        //sleep(1);
    }

    if (pid > 0)
    {
        for (int i = 0; i < N; i++)
            printf("Waited for: %d\n", wait(NULL));

        close_sem();
    }

    return 0;
}

int fill_sem_names()
{
    for (int i = 0; i < N; ++i)
    {
        sem_names[i][0] = 'S';
        sem_names[i][1] = i + '0';
        sem_names[i][2] = '\0';
    }
}

void close_sem()
{
    for (int i = 0; i < N; ++i)
    {
        sem_unlink(sem_names[i]);
        sem_close(T->chopsticks[i]);
    }
}

void _swap(int *a, int *b)
{
    *a = *a ^ *b;
    *b = *a ^ *b;
    *a = *a ^ *b;
}
int max(int a, int b)
{
    return a > b ? a : b;
}

void starving_philosopher(int num)
{
    for (int i = 0; i < MEALS; ++i)
    {
        int l = num, r = (num + 1) % N;
        if (num % 2)
            _swap(&l, &r);

        sem_wait(T->chopsticks[l]);
        sem_wait(T->chopsticks[r]);

        int eating_time = 2; //rand() % 3 + 1;
        printf("Philosopher %d will be eating for %d seconds.\n", num, eating_time);
        //printf("Philosopher %d uses chopsticks: %d and %d.\n", num, num, (num + 1) % 5);
        sleep(eating_time);

        sem_post(T->chopsticks[l]);
        sem_post(T->chopsticks[r]);

        int thinking_time = 1;//rand() % 3;
        printf("Philosopher %d will be thinking for %d seconds.\n", num, thinking_time);
        sleep(thinking_time);
    }
}

void get_priority(int num)
{
    int res = T->priority[0];
    for (int i = 1; i < N; ++i)
        res = max(res, T->priority[i]);
    T->priority[num] = 1 + res;
}
int cmp(int x, int y)
{
    if (T->priority[x] < T->priority[y])
        return 1;
    if (T->priority[x] == T->priority[y] && x < y)
        return 1;
    return 0;
}

void philosopher(int num)
{
    for (int i = 0; i < MEALS; ++i)
    {
        // bakery algorithm - assuring no starvation
        T->choosing_priority[num] = 1;
        get_priority(num);
        T->choosing_priority[num] = 0;

        for (int j = 0; j < N; ++j)
        {
            while (T->choosing_priority[j]);
            while (T->priority[j] != 0 && cmp(j, num));
        }

        int l = num, r = (num + 1) % N;
        // assuring no deadlock
        if (num % 2)
            _swap(&l, &r);

        sem_wait(T->chopsticks[l]);
        sem_wait(T->chopsticks[r]);

        int eating_time = 2;//rand() % 3 + 1;
        printf("Philosopher %d will be eating for %d seconds.\n", num, eating_time);
        //printf("Philosopher %d uses chopsticks: %d and %d.\n", num, num, (num + 1) % 5);
        sleep(eating_time);

        sem_post(T->chopsticks[l]);
        sem_post(T->chopsticks[r]);

        // bakery algorithm
        T->priority[num] = 0;

        int thinking_time = 1;//rand() % 3;
        printf("Philosopher %d will be thinking for %d seconds.\n", num, thinking_time);
        sleep(thinking_time);
    }
}