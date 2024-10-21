#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define STORE_PATH "/tmp/store"
#define STORE_SIZE 100

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void init_store() {
    int fd = open(STORE_PATH, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    char buffer[STORE_SIZE] = {0};
    write(fd, buffer, STORE_SIZE);
    close(fd);
}

void producer(int semid) {
    int fd = open(STORE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    struct sembuf sem_op;
    char resource = '1';
    while (1) {
        sem_op.sem_num = 0;
        sem_op.sem_op = -1;
        sem_op.sem_flg = 0;
        semop(semid, &sem_op, 1);

        lseek(fd, 0, SEEK_SET);
        char buffer[STORE_SIZE];
        read(fd, buffer, STORE_SIZE);
        int count = 0;
        for (int i = 0; i < STORE_SIZE; i++) {
            if (buffer[i] != 0) count++;
        }
        if (count < STORE_SIZE) {
            printf("生产前: 资源数量=%d, 空位数量=%d\n", count, STORE_SIZE - count);
            buffer[count] = resource;
            lseek(fd, 0, SEEK_SET);
            write(fd, buffer, STORE_SIZE);
            printf("生产后: 资源数量=%d, 空位数量=%d, 生产资源=%c\n", count + 1, STORE_SIZE - count - 1, resource);
            resource = (resource == '9') ? '0' : resource + 1;
        }

        sem_op.sem_op = 1;
        semop(semid, &sem_op, 1);
        sleep(1);
    }
    close(fd);
}

void consumer(int semid) {
    int fd = open(STORE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    struct sembuf sem_op;
    while (1) {
        sem_op.sem_num = 0;
        sem_op.sem_op = -1;
        sem_op.sem_flg = 0;
        semop(semid, &sem_op, 1);

        lseek(fd, 0, SEEK_SET);
        char buffer[STORE_SIZE];
        read(fd, buffer, STORE_SIZE);
        int count = 0;
        for (int i = 0; i < STORE_SIZE; i++) {
            if (buffer[i] != 0) count++;
        }
        if (count > 0) {
            printf("消费前: 资源数量=%d, 空位数量=%d\n", count, STORE_SIZE - count);
            char consumed = buffer[0];
            memmove(buffer, buffer + 1, STORE_SIZE - 1);
            buffer[STORE_SIZE - 1] = 0;
            lseek(fd, 0, SEEK_SET);
            write(fd, buffer, STORE_SIZE);
            printf("消费后: 资源数量=%d, 空位数量=%d, 消费资源=%c\n", count - 1, STORE_SIZE - count + 1, consumed);
        }

        sem_op.sem_op = 1;
        semop(semid, &sem_op, 1);
        sleep(2);
    }
    close(fd);
}

int main() {
    key_t key = ftok(STORE_PATH, 'a');
    int semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget");
        exit(1);
    }
    union semun sem_union;
    sem_union.val = 1;
    if (semctl(semid, 0, SETVAL, sem_union) < 0) {
        perror("semctl");
        exit(1);
    }

    init_store();

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        producer(semid);
    } else {
        consumer(semid);
    }

    return 0;
}