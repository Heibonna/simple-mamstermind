/* Mateusz Murawski 2019*/
/* Simple Mastermind game implementation*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#define MAXLINE 1024
int msqid;

struct my_msgbuf {
    long mtype;
    char mtext[MAXLINE];
};


void close_device(int msqid){
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }
    printf("Closing connection...\n");
}


char password[4];

void check_re(char* passwd, char* re){
    if(passwd[0] == re[0])
        re[0] = '2';
    else if((re[0] == passwd[1] || re[0] == passwd[2] ) || re[0] == passwd[3])
        re[0] = '1';
    else
        re[0] = '0';

    if(passwd[1] == re[1])
        re[1] = '2';
    else if((re[1] == passwd[0] || re[1] == passwd[2]) || re[1] == passwd[3])
        re[1] = '1';
    else
        re[1] = '0';

    if(passwd[2] == re[2])
        re[2] = '2';
    else if((re[2] == passwd[1] || re[2] == passwd[0]) || re[2] == passwd[3])
        re[2] = '1';
    else
        re[2] = '0';

    if(passwd[3] == re[3])
        re[3] = '2';
    else if((re[3] == passwd[1] || re[3] == passwd[2]) || re[3] == passwd[0])
        re[3] = '1';
    else
        re[3] = '0';

    re[4]='\0';

    if(re[0] == '2' && re[1] == '2' && re[2] == '2' && re[3] == '2')
        strcpy(re, "\n~~~~~~~~~~~~\n  You won!\n~~~~~~~~~~~~\n");

}


void run_server(key_t key){
    int msqid, licznik = 0, temp;
    struct my_msgbuf buf;
    char quit = 'q';
    char response[MAXLINE];
    char tmp[MAXLINE];
    int player;


    if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Set password (4 digits betweeen 1 and 6): ");

    buf.mtype = 1;

    if(fgets(password, 5, stdin) != NULL ) {
        printf("Waiting for players...\n");
        for(;;) {
            if ((temp = msgrcv(msqid,(struct msgbuf *)&buf, sizeof(buf), 1, 0)) == -1) {
                perror("msgrcv");
                exit(1);
            }
            if(quit == buf.mtext[0]){
                if(msgctl(msqid, IPC_RMID, NULL) == -1){
                    perror("msgctl");
                    exit(1);
                }
                putchar('\n');
                exit(0);
            }



            if(buf.mtext[0] == '#'){

                printf("New player! ID: %s\n", buf.mtext);
                strncpy(tmp, buf.mtext + 1, strlen(buf.mtext)-1);

                player = atoi(tmp);

                if(fork() == 0){
                    licznik = 12;
                    strcpy(buf.mtext, "Connected. Let's play!\n!Press 'q' to exit or 'c' to remove\n");
                    buf.mtype = player;
                    if (msgsnd(msqid, (struct msgbuf *)&buf, sizeof(buf), 0) == -1){
                        perror("msgsnd");
                        exit(1);
                    }

                    while(1){
                        if ((temp = msgrcv(msqid,(struct msgbuf *)&buf, sizeof(buf), player, 0)) == -1) {
                            perror("msgrcv");
                            exit(1);
                        }
                        buf.mtype = player;

                        /*check answer*/
                        if(buf.mtext[0] == 'q'){
                            strcpy(buf.mtext, "Exiting...");
                            if (msgsnd(msqid, (struct msgbuf *)&buf, sizeof(buf), 0) == -1){
                                perror("msgsnd");
                                exit(1);
                             }
                            exit(0);
                        }
                        else if(buf.mtext[0] == 'c'){
                            strcpy(buf.mtext, "Preclosing...");
                            if (msgsnd(msqid, (struct msgbuf *)&buf, sizeof(buf), 0) == -1){
                                perror("msgsnd");
                                exit(1);
                             }
                            exit(0);
                        }
                        else if(strlen(buf.mtext) != 5 || !(buf.mtext[0] > '0' && buf.mtext[0] < '7')|| !(buf.mtext[1] > '0' && buf.mtext[1] < '7')|| !(buf.mtext[2] > '0' && buf.mtext[2] < '7')|| !(buf.mtext[3] > '0' && buf.mtext[3] < '7') ){
                            strcpy(buf.mtext, "Use 4 digits in range 1-6");
                        }
                        else{
                            check_re(password, buf.mtext);
                            licznik--;
                            if(licznik == 0){
                                strcpy(buf.mtext, "You lost! :(\n");

                                if (msgsnd(msqid, (struct msgbuf *)&buf, sizeof(buf), 0) == -1){
                                        perror("msgsnd");
                                        exit(1);
                                }
                                exit(0);
                            }
                            else{
                                snprintf(response, MAXLINE, "%d", licznik);
                                strcat(buf.mtext, " ");
                                strcat(buf.mtext, response);
                                strcat(buf.mtext, " left");
                            }
                        }

                        if (msgsnd(msqid, (struct msgbuf *)&buf, sizeof(buf), 0) == -1){
                            perror("msgsnd");
                            exit(1);
                        }
                    }
                    return;
                }

            }

        }
    }


    return;
}


int main(int argc, char** argv){
    struct my_msgbuf buf;
    key_t key;

    if(argc != 3 || (!strcmp(argv[2], "1"))){
    	printf("Use:'client GameId PlayerId (> 1)'.\n");
	    exit(1);
    }

    key = strtol(argv[1], NULL, 16);

    if ((msqid = msgget(key, 0664|IPC_EXCL)) == -1) {    /*Try open*/
        if ((key = ftok("client.c", atoi(argv[1]))) == -1) {  /*Create new key*/
            perror("ftok");
            exit(1);
        }

        printf("Object doesn't exist. New key:\n%x\n", key);

        run_server(key);
        return 0;
    }

    strcpy(buf.mtext, "#");
    strcat(buf.mtext, argv[2]);
    buf.mtype = 1;

 /* printf("Content: '%s'\nType: %ld\n", buf.mtext, buf.mtype);*/

    if (msgsnd(msqid, (struct msgbuf *)&buf, sizeof(buf), 0) == -1){
        perror("msgsnd");
        exit(1);
    }
        /*    printf("sent: %s\n",buf.mtext);*/


    for(;;) {
        if (msgrcv(msqid, (struct msgbuf *)&buf, sizeof(buf), atoi(argv[2]), 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        if(buf.mtext[0] == '\n'){
            printf("%s\n",buf.mtext);
            exit(0);
        }
        else
            printf("!%s\n",buf.mtext);

        if(!strcmp(buf.mtext, "Exiting...")){
            return 0;
        }
        else if(!strcmp(buf.mtext, "Preclosing...")){
            close_device(msqid);
            return 0;
        }
        else if(!strcmp(buf.mtext, "You lost! :(\n")){
            return 0;
        }



        if(fgets(buf.mtext, MAXLINE, stdin) != NULL ) {

            buf.mtype = atoi(argv[2]);

            if (msgsnd(msqid, (struct msgbuf *)&buf, sizeof(buf), 0) == -1){
                perror("msgsnd");
                exit(1);
            }
        }
    }


    return 0;
}
