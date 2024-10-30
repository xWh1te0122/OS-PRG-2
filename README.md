# OS-PRG-2
**程式碼部分以copilot為工具完成，透過一步一步了解架構去修正錯誤內容，並實作出同步機制。**

### 編譯程式方法:

1. 使用Ubuntu(WSL) Terminal

2. 使用gcc編譯程式，指令如下:

```sh
gcc -o lab2-2 lab2-2.c
```

**註:檔案請照當下路徑做對應更改**

### 運行程式方法:

```sh
./lab2-2 <buffer_size> <producer_speed> <consumer_speed>
```

**例如:**

```sh
./lab2-2 10 1 2
```

**註:參數格式若輸入錯誤會報錯**

### 程式代碼解釋

**struct、variable、semaphore的建立**

```c
typedef struct
{
    int value;
} item;

item *buffer;
int *in, *out;
sem_t *empty, *full, *mutex, *sync_sem;
```

這個部分定義了產品的資訊、緩衝區的指標、circle queue中的in和out指標(One for producer, another one for consumer.)以及之後會用到的同步信號。

**主函式的運行流程**

```c
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
```

主函式主要用於運行參數輸入與偵測、配置及清空共享記憶體、變數及信號初始化和最重要的主程序和子程序的運行。

比較需要注意的幾點如下:

1. buffer_size須預留一個空位，作為circle queue判斷full或empty的條件。

2. 若不建立同步信號，會有父程序輸出PID比子程序運行副函式慢的問題、輸出buffer_size亂掉的狀況(race condition)等，這邊採用<semaphore.h>的函式來解決。

3. parameter的偵測判斷式必須包含開頭輸入的執行檔，所以實際上argc會有4個。

**副函式的運行流程**

1. producer()

```c
void producer(int speed, int buffer_size)
{
    item next_produced;
    int produced_count = 0;
    while (1)
    {
        next_produced.value = rand() % 100;

        sem_wait(empty);
        sem_wait(mutex);

        buffer[*in] = next_produced;
        *in = (*in + 1) % buffer_size;
        produced_count++;

        int empty_slots = buffer_size - (*in - *out + buffer_size) % buffer_size - 1;

        printf("生產者: 已生產 item %d，緩衝區中還有 %d 個空位。\n", produced_count, empty_slots);

        sem_post(mutex);
        sem_post(full);

        sleep(speed);
    }
}
```

2. consumer()

```c
void consumer(int speed, int buffer_size)
{
    item next_consumed;
    int consumed_count = 0;
    while (1)
    {
        sem_wait(full);
        sem_wait(mutex);

        next_consumed = buffer[*out];
        *out = (*out + 1) % buffer_size;
        consumed_count++;

        int empty_slots = buffer_size - (*in - *out + buffer_size) % buffer_size - 1;

        printf("消費者: 已消費 item %d，緩衝區中還有 %d 個空位。\n", consumed_count, empty_slots);

        sem_post(mutex);
        sem_post(empty);

        sleep(speed);
    }
}
```

副函式需要注意的幾點如下:

1. 原採用一般circle queue的full、empty判斷方法 (1) (*in + 1) % buffer_size) == *out (2) (*in == *out)，來讓生產者或消費者進行等待，但輸出後會有順序亂掉的問題，所以後來一樣採用同步信號解決。(狀況範例於task 2資料夾內)

2. empty_slots的計算方法可拆分為2個區塊:

1. 判斷已放入商品的個數

```c
(*in - *out + buffer_size) % buffer_size
```

2. 位置總數 - 已放入商品數 - 1(預留空間) = 剩餘空位數

```c
buffer_size - (*in - *out + buffer_size) % buffer_size - 1
```
