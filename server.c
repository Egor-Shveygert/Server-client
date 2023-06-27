#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>  //sockaddr, socklen_t
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#define GSEM "/myglobalsem" // Definujem si meno globalneho semaforu

int score;
int klient[2]; // 2 klienti sa pripoja takze 2 socket descriptor
int sock_desc; // Server socket descriptor
sem_t sem; // Lokalny semafor na sync threadu tzv. binarny semafor 0-1

//Signal handler funkcia
void sigint(int sig){
    for(int i=0; i < 2; i++){
        if(klient[i] >= 0) close(klient[i]);
    }
    close(sock_desc);
    sem_unlink(GSEM);
    sem_destroy(&sem);
    exit(EXIT_FAILURE);
}

//Funkcia ktoru bude vykonavat thread
void *precitaj(){
    int bytes;

    memset(&score, 0, sizeof(score));

    bytes = recv(klient[0], &score, sizeof(score), 0);

    if(bytes < 0 || bytes == 0){
        if(bytes < 0) perror("Reading from Client 1");
        else perror("Client closed connection");
        close(klient[0]);
        close(sock_desc);
        exit(1);
    }

    sem_post(&sem);
    pthread_exit(NULL); // Ukoncujeme thread s tym ze hodnotu ziadnu nevracame preto "NULL"
}

int main(){
    signal(SIGINT, sigint); //Mapovanie signalu a handlera

    printf("Creating socket [server]\n"); //Vytvaram socket
    sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_desc == -1){
        printf("Can't create socket\n");
        exit(0);
    }

    sem_t *global = sem_open(GSEM, O_CREAT, 0600, 0); //Inicializujem globalny semafor
    //struct for serever
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(6226);

    printf("Binding to socket\n");
    if(bind(sock_desc, (struct sockaddr*)&server, sizeof(server)) != 0){
        printf("Couldn't bind socket\n");
        close(sock_desc);
        exit(0);
    }

    printf("Listening on socket\n");
    if(listen(sock_desc, 3) != 0){
        printf("Can't listen on socket\n");
        close(sock_desc);
        exit(0);
    }

    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    socklen_t len = sizeof(client);
    printf("Accepting first client\n");
    klient[0] = accept(sock_desc, (struct sockaddr*)&client, &len); //accept 1 client

    if(klient[0] == -1){
        printf("Error accepting first\n");
        close(klient[0]);
        close(sock_desc);
        exit(0);
    }

    memset(&client, 0, sizeof(client));
    printf("Accepting second client\n");
    klient[1] = accept(sock_desc, (struct sockaddr*)&client, &len); //accept 2 client

    if(klient[1] == -1){
        printf("Error accepting second\n");
        close(klient[1]);
        close(sock_desc);
        exit(0);
    }


    memset(&sem, 0, sizeof(sem));
    if(sem_init(&sem, 0, 0) != 0){
        perror("Semaphore");
        for(int i=0; i <2; i++) close(klient[i]);
        close(sock_desc);
        exit(0);
    }
    //thread initialization
    pthread_t thid;
    if(pthread_create(&thid, NULL, precitaj, NULL) < 0){ //thread do function precitaj
        printf("Pthread_create() error...\n");
        for(int i=0; i < 2; i++) close(klient[i]);
        close(sock_desc);
        sem_destroy(&sem);
        exit(EXIT_FAILURE);
    }

    sem_wait(&sem); // Cakame na lokalny semafor
    int bytes;

    bytes = send(klient[1], &score, sizeof(score), 0); //send data to 2 client
    if(bytes < 0 || bytes == 0){
        if(bytes < 0) perror("Sending to Client 2");
        else perror("Client closed connection");
        close(klient[1]);
        close(sock_desc);
        sem_destroy(&sem);
        pthread_join(thid, NULL);
        exit(0);
    }

    pthread_join(thid, NULL); // Waititng for thread to finish

    printf("Waiting to exit...\n");
    sem_wait(global);
    printf("Exiting...\n");

    for(int i=0; i < 2; i++) close(klient[i]); //Closing client sockets
    close(sock_desc); // Close server socket
    sem_close(global); // Close global semafor
    sem_destroy(global); //Free global semafor
    sem_unlink(GSEM); // Delete global semafor by name
    sem_destroy(&sem); //Free local semafor
    exit(EXIT_SUCCESS);
}
