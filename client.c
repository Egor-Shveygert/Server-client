#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

#define GSEM "/myglobalsem"

int fd[2], fd2[2];
int sock_desc = -1;
sem_t *global;
pid_t pid; // pid je Process ID unikatny identifikator processu

void mojafunkcia(int); //Handler na SIGINT
void sigusr2(int); //Handler na SIGUSR2
int pripojit(int); //Funkcia na pripojenie na server vracia socket descriptor na komunikaciu
timer_t vytvorCasovac(int); //Funkcia na vytvorenie casovacu s navratovou hodnotou casovacu
void spustiCasovac(timer_t, int);

int main(){
    signal(SIGINT, mojafunkcia);
    sleep(1); // Sleep so the server has time to start up

    int bytes, score; // fd is the file descriptor for our pipe fd[0] is for reading and fd[1] is for writing
    char grade;
    global = sem_open(GSEM, O_CREAT, 0600, 0);

    pipe(fd); // We open the pipe here
//CLIENT 2 ---------------------------------------------------------------------------------------------------------------
    if((pid = fork()) == 0){
        usleep(500000); // We use sleep here so that Client 1 can start and connect to the server first
        // When pid == 0 means we are in the child process
        // use getpid() inside the child to get the child pid
        // Client 2 - check input
        close(fd[0]); //Close reading end of pipe 1 because client 2 sends
        sock_desc = pripojit(6226); // Connect to server
        if(sock_desc < 0){
            close(fd[1]);
            exit(0);
        }

        bytes = recv(sock_desc, &score, sizeof(score), 0); //recv data from server
        // printf("Bytes received from server %d and %d\n", bytes, score);
        if(bytes < 0 || bytes == 0){
            if(bytes < 0) perror("Receiving from server");
            else perror("Server closed connection");
            close(sock_desc);
            close(fd[1]);
            exit(0);
        }
        //check rozsahu
        if(score < 0 || score > 100){
            perror("Invalid input");
            close(sock_desc);
            close(fd[1]);
            kill(getppid(), SIGINT);
            exit(0);
        }

        bytes = write(fd[1], &score, sizeof(score));

        if(bytes < 0 || bytes == 0){
            if(bytes < 0) perror("Sending to pipe 1");
            else perror("Pipe 1 connection closed");

            close(fd[1]);
            exit(0);
        }

        close(fd[1]);
        exit(0);
    }
    else if(pid < 0) {
        //Error forking (maybe not enough memory, or OS failed to give the process a process ID aka pid)
        perror("Chyba fork()");
        exit(EXIT_FAILURE);
    }

    int fd2[2];
    pipe(fd2);
//CLIENT 3 ----------------------------------------------------------------------------------------------------------
    if((pid = fork()) == 0){
        // Client 3 - Convert input
        close(fd[1]); // Close writing end of pipe 1 because we only read from pipe 1
        close(fd2[0]); // Close reading end of pipe 2 because we only write to pipe 2

        bytes = read(fd[0], &score, sizeof(score)); // read() returns number of bytes received

        if(bytes < 0 || bytes == 0){
            if(bytes < 0) perror("Reading from pipe 1");
            else perror("Pipe 1 connection closed");

            close(fd[0]);
            close(fd2[1]);
            exit(0);
        }
        //vyhodnotime score
        if(score >= 90 && score <= 100) //Acko
            grade = 'A';
        else if(score >= 80) //Bcko
            grade = 'B';
        else if(score >= 70) //Ccko
            grade = 'C';
        else if(score >= 60) //Dcko
            grade = 'D';
        else if(score >= 50) //Ecko
            grade = 'E';
        else //FX
            grade = 'F';
        //vyhodnotene data zapiseme do pipe
        bytes = write(fd2[1], &grade, sizeof(grade)); // write returns number of bytes received

        if(bytes < 0 || bytes == 0){
            if(bytes < 0) perror("Writing to pipe 2");
            else perror("Pipe 2 connection closed");

            close(fd[0]);
            close(fd2[1]);
            printf("Exiting...\n");
            exit(0);
        }
        //zatvaranie pipe
        close(fd[0]); //Closing pipe when done
        close(fd2[1]); //Closing pipe when done
        exit(0);
    }
    else if(pid < 0) {
        //Error forking (maybe not enough memory, or OS failed to give the process a process ID aka pid)
        perror("Chyba fork()");
        exit(EXIT_FAILURE);
    }
//CLIENT 4 --------------------------------------------------------------------------------------------------------------
    if((pid = fork()) == 0){
        // Client 4 - print grade
        close(fd[0]); //We dont use these pipe
        close(fd[1]); //We dont use these pipe
        close(fd2[1]); //We close the input of pipe 2 because we are only reading the output of pipe 2


        bytes = read(fd2[0], &grade, sizeof(grade));

        if(bytes < 0 || bytes == 0){
            if(bytes < 0) perror("Reading from pipe 2");
            else perror("Pipe 2 connection closed");
            close(fd2[0]);
            exit(0);
        }

        close(fd2[0]); // After successful reading close the pipe
        printf("Your grade based on the entered score is \"%c\"\n", grade);
        exit(0);
    }
    else if(pid < 0) {
        //Error forking (maybe not enough memory, or OS failed to give the process a process ID aka pid)
        perror("Chyba fork()");
        exit(EXIT_FAILURE);
    }
//CLIENT 1 (main je cl1)--------------------------------------------------------------------------------------------------
    // Parent lebo pid > 0
    // fork() nam v parent processe vratil pid child processu;
    // Client 1 - get input
    signal(SIGUSR2, sigusr2); // Mapujeme signal pre casovac
    sock_desc = pripojit(6226); //Connect to server
    if(sock_desc < 0) {
        close(fd[0]);
        close(fd[1]);
        close(fd[0]);
        close(fd2[1]);
        exit(0);
    }

    timer_t casovac = vytvorCasovac(SIGUSR2);
    spustiCasovac(casovac, 6);

    //user input
    sleep(1);
    printf("Enter score(0-100)\n");
    scanf("%d", &score);

    timer_delete(casovac); // Vymazanie casovacu

    bytes = send(sock_desc, &score, sizeof(score), 0);

    if(bytes < 0 || bytes == 0){
        if(bytes < 0) perror("Writing to server");
        else perror("Server closed connection");
        close(sock_desc);
        exit(0);
    }

    close(sock_desc);
    //sem_post(global);
    waitpid(pid, NULL, 0); // Cakanie na skoncenie processu podla PID, cl1 wait for cl4
    sem_post(global); // after end of cl4, turn off server
    exit(0);
}

void mojafunkcia(int sig){
    sem_post(global);
    sem_close(global);
    sem_destroy(global);
    close(fd[0]); //Close Pipe 1
    close(fd[1]); //Close Pipe 1
    close(fd2[0]); //Close Pipe 2
    close(fd2[1]); //Close Pipe 2
    if(sock_desc >= 0) // Check if we have socket created
        close(sock_desc); // Close socket
    sem_unlink(GSEM);
    kill(pid, SIGINT); // Vyslanie signalu Interrupt process podla PID
    waitpid(pid, NULL, 0); // Cakanie na skoncenie processu podla PID
    exit(0);
}

void sigusr2(int sig){
    printf("Enter score(0-100)\n");
}

int pripojit(int port){
    printf("Creating socket\n");

    int sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_desc == -1){
        printf("Couldn't create socket\n");
        return -1;
    }

    struct sockaddr_in client;
    memset(&client, 0, sizeof(client)); //Clear memory of garbage
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost
    client.sin_port = htons(port);

    printf("Connecting to the server...\n");
    if(connect(sock_desc, (struct sockaddr*)&client, sizeof(client)) != 0){
        printf("Can't connect to server\n");
        close(sock_desc);
        return -1;
    }

    return sock_desc;
}

timer_t vytvorCasovac(int signal)
{
  struct sigevent kam;
  kam.sigev_notify=SIGEV_SIGNAL; // Casovac po ubehnuti posle signal keby sme pouzili SIGEV_THREAD tak miesto poslania signalu spusti funkciu vo threade
  kam.sigev_signo=signal;

  timer_t casovac;
  timer_create(CLOCK_REALTIME, &kam, &casovac);
  return(casovac);
}

void spustiCasovac(timer_t casovac, int sekundy)
{
  struct itimerspec casik;
  casik.it_value.tv_sec=sekundy;
  casik.it_value.tv_nsec=0;
  casik.it_interval.tv_sec=sekundy; // ak interval > 0 casovac pobezi do nekonecna a signal bude posielat po intervaloch ktory nastavime ak chceme aby zbehol iba raz interval = 0
  casik.it_interval.tv_nsec=0;
  timer_settime(casovac,CLOCK_REALTIME,&casik,NULL);
}
