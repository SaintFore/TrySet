#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>

#define STORE_PATH "/tmp/store"
#define MAX_CAPACITY 100

sem_t empty;
sem_t full;
pthread_mutex_t mutex;

void* producer(void* arg) {
    char resource[] = "1234567890";
    int index = 0;

    while (1) {
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);

        FILE* f = fopen(STORE_PATH, "a+");
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (size < MAX_CAPACITY) {
            fputc(resource[index], f);
            index = (index + 1) % strlen(resource);
            fseek(f, 0, SEEK_END);
            long new_size = ftell(f);
            printf("生产前: %ld bytes, 空位: %ld bytes\n", size, MAX_CAPACITY - size);
            printf("生产后: %ld bytes, 空位: %ld bytes\n", new_size, MAX_CAPACITY - new_size);
            printf("生产资源: %c\n", resource[index]);
        }
        fclose(f);

        pthread_mutex_unlock(&mutex);
        sem_post(&full);
        sleep(1);
    }
    return NULL;
}

void* consumer(void* arg) {
    while (1) {
        sem_wait(&full);
        pthread_mutex_lock(&mutex);

        FILE* f = fopen(STORE_PATH, "r+");
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (size > 0) {
            fseek(f, 0, SEEK_SET);
            char consumed = fgetc(f);
            char buffer[MAX_CAPACITY];
            fread(buffer, 1, size - 1, f);
            freopen(STORE_PATH, "w", f);
            fwrite(buffer, 1, size - 1, f);
            fseek(f, 0, SEEK_END);
            long new_size = ftell(f);
            printf("消费前: %ld bytes, 空位: %ld bytes\n", size, MAX_CAPACITY - size);
            printf("消费后: %ld bytes, 空位: %ld bytes\n", new_size, MAX_CAPACITY - new_size);
            printf("消费资源: %c\n", consumed);
        }
        fclose(f);

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        sleep(2);
    }
    return NULL;
}

int main() {
    // 创建或清空文件
    FILE* f = fopen(STORE_PATH, "w");
    fclose(f);

    // 初始化信号量和互斥锁
    sem_init(&empty, 0, MAX_CAPACITY);
    sem_init(&full, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    // 启动生产者和消费者线程
    pthread_t producer_thread, consumer_thread;
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // 销毁信号量和互斥锁
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);

    return 0;
}