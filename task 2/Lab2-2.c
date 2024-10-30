#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct
{
    int value;
} item;

item *buffer;
int *in, *out;
sem_t *empty, *full, *mutex, *sync_sem;

void producer(int speed, int buffer_size);
void consumer(int speed, int buffer_size);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("ERROR! Enter four parameter: <file_path> <buffer_size> <producer_speed> <consumer_speed>\n");
        exit(1);
    }

    int buffer_size = atoi(argv[1]) + 1; // 需預留一個空間作為判斷緩衝區滿或空的條件
    int producer_speed = atoi(argv[2]);
    int consumer_speed = atoi(argv[3]);

    // 配置共享記憶體
    buffer = mmap(NULL, buffer_size * sizeof(item), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    in = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    out = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    empty = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    full = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    // mutex可以確保在同一時間只有一個進程可以訪問共享資源
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sync_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (buffer == MAP_FAILED || in == MAP_FAILED || out == MAP_FAILED || empty == MAP_FAILED || full == MAP_FAILED || mutex == MAP_FAILED)
    {
        perror("Allocate shared memory failed");
        exit(1);
    }

    *in = 0;
    *out = 0;
    sem_init(empty, 1, buffer_size - 1);
    sem_init(full, 1, 0);
    sem_init(mutex, 1, 1);
    sem_init(sync_sem, 1, 0);

    pid_t producer_pid = fork();
    if (producer_pid < 0)
    {
        perror("producer process error");
        exit(1);
    }
    else if (producer_pid == 0)
    {
        // 等待主程序釋放同步信號量
        sem_wait(sync_sem);
        // 生產者子程序
        producer(producer_speed, buffer_size);
        exit(0);
    }

    pid_t consumer_pid = fork();
    if (consumer_pid < 0)
    {
        perror("consumer process error");
        exit(1);
    }
    else if (consumer_pid == 0)
    {
        // 等待主程序釋放同步信號量
        sem_wait(sync_sem);
        // 消費者子程序
        consumer(consumer_speed, buffer_size);
        exit(0);
    }

    printf("生產者程序ID: %d\n", producer_pid);
    printf("消費者程序ID: %d\n\n", consumer_pid);

    sem_post(sync_sem);
    sem_post(sync_sem);

    waitpid(producer_pid, NULL, 0);
    waitpid(consumer_pid, NULL, 0);

    // Clean up
    munmap(buffer, buffer_size * sizeof(item));
    munmap(in, sizeof(int));
    munmap(out, sizeof(int));
    munmap(empty, sizeof(sem_t));
    munmap(full, sizeof(sem_t));
    munmap(mutex, sizeof(sem_t));
    munmap(sync_sem, sizeof(sem_t));

    return 0;
}

void producer(int speed, int buffer_size)
{
    item next_produced;
    int produced_count = 0;
    while (1)
    {
        // Produce an item
        next_produced.value = rand() % 100; // Example production logic

        sem_wait(empty); // Wait if buffer is full
        sem_wait(mutex); // Enter critical section

        // while (((*in + 1) % buffer_size) == *out)
        //     ; // Wait if buffer is full

        buffer[*in] = next_produced;
        *in = (*in + 1) % buffer_size;
        produced_count++;

        int empty_slots = buffer_size - (*in - *out + buffer_size) % buffer_size - 1;

        printf("生產者: 已生產 item %d，緩衝區中還有 %d 個空位。\n", produced_count, empty_slots);

        sem_post(mutex);
        sem_post(full);

        sleep(speed); // Simulate time taken to produce an item
    }
}

void consumer(int speed, int buffer_size)
{
    item next_consumed;
    int consumed_count = 0;
    while (1)
    {
        sem_wait(full);  // Wait if buffer is empty
        sem_wait(mutex); // Enter critical section

        // while (*in == *out)
        //     ; // Wait if buffer is empty

        next_consumed = buffer[*out];
        *out = (*out + 1) % buffer_size;
        consumed_count++;

        int empty_slots = buffer_size - (*in - *out + buffer_size) % buffer_size - 1;

        printf("消費者: 已消費 item %d，緩衝區中還有 %d 個空位。\n", consumed_count, empty_slots);

        sem_post(mutex);
        sem_post(empty);

        // Consume the item
        sleep(speed); // Simulate time taken to consume an item
    }
}