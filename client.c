#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>

#define LEN 256

#define DEFAULT printf("%c[%dm", 0x1B, 0)
#define RED printf("\x1b[31m")
#define BLUE printf("\x1b[36m")
#define GREEN printf("\x1b[32m")
#define YELLOW printf("\x1b[33m")

extern int connect_to_server(char *);
void talk_with_server(int);
void getLine();
void alarmHandler();

pthread_t tid;
char ans[LEN];

int main(int ac, char *av[])
{
    signal(SIGALRM, alarmHandler);
    int fd;
    fd = connect_to_server(av[1]);
    if (fd == -1)
        exit(1);
    YELLOW;
    printf("Welcome to Quiz show!!!\n\n");
    talk_with_server(fd);
    close(fd);
}

void talk_with_server(int fd)
{
    char question[LEN];
    char cheak[LEN];
    char id[LEN];
    char types[LEN];
    int n, count = 0;

    char accessTime[LEN];
    time_t serverAccessTime;

quiz:    
    strcpy(accessTime, " ");
    serverAccessTime = time(NULL);
    struct tm *t=localtime(&serverAccessTime); 

    BLUE;
    printf("id : ");
    fgets(id, LEN, stdin);
    write(fd, id, strlen(id));
    DEFAULT;

    sprintf(accessTime, "%d:%d:%d", t->tm_hour, t->tm_min, t->tm_sec);
    
    sleep(0.7);
    write(fd, accessTime, strlen(accessTime));
    printf("\n");

    while(1) {
        printf("1: 넌센스 및 속담, 2: 상식, 3: OX\nquestion type : ");
        fgets(types, LEN, stdin);
        if(atoi(types) > 0 && atoi(types) <= 3)
            break;
        else
            printf("범위 안에서 선택해주세요!\n\n");
    }
    write(fd, types, strlen(types));

    while(count < 5) {
        n = read(fd, question, LEN);
        question[n] = '\0';
        DEFAULT;
        fputs(question, stdout);
        printf("\n");
        fflush(stdout);
        memset(question, 0, sizeof(question));

        GREEN;
        printf("answer : ");
        DEFAULT;
        if(pthread_create(&tid, NULL, (void *)getLine, NULL) < 0) {
            perror("thread");
            exit(1);
        }
        pthread_join(tid, NULL);
        write(fd, ans, strlen(ans));

        if(strstr(ans, "hint") == NULL && strstr(ans, "HINT") == NULL) {
            count++;
        }
        else{;}

        n = read(fd, cheak, LEN);
        cheak[n] = '\0';
        RED;
        fputs(cheak, stdout);
        DEFAULT;
        printf("\n");
        fflush(stdout);
        memset(cheak, 0, sizeof(cheak));
        printf("\n");
    }

    n = read(fd, cheak, LEN);       //정답률, 점수 읽기
    YELLOW;
    fputs(cheak, stdout);
    fflush(stdout);
    printf("\n");
    DEFAULT;
    
    RED;
    n = read(fd, cheak, LEN);       //1. AGAIN 2. RANK 3. QUIT
    fputs(cheak, stdout);
    DEFAULT;
    printf("\nanswer : ");
    int num;
    while(1) {
        fgets(ans, LEN, stdin);
        ans[strlen(ans)-1] = '\0';
        num = atoi(ans);
        if(num == 1 || num == 2 || num == 3)
            break;
        else
            printf("다시 입력해주세요(1, 2, 3) : ");
    }

    printf("\n");
    write(fd, ans, strlen(ans));
    

    FILE *fs;
    int ii, ik, linenum = 0, ucount = 0;
    char staticStr[BUFSIZ];
    char c;
    char* strcut;
    int ss, grade;

    fs = fopen("clientStatic.txt","r");
    while((c = fgetc(fs))!= EOF)
        if(c == '\n') linenum++;


    rewind(fs);//파일포인터를 다시 앞으로
    char staticGetBuf[linenum][BUFSIZ];

    while(!feof(fs)){
        fgets(staticGetBuf[ucount],LEN,fs);
        ucount++;
    }
    
    for(ii = 0 ; ii < linenum - 1 ; ii++){
        for(ik = 0 ; ik < linenum-1-ii ; ik++){
            if(strncmp(staticGetBuf[ik], staticGetBuf[ik+1], 2) < 0){
                strcpy(staticStr, staticGetBuf[ik]);
                strcpy(staticGetBuf[ik], staticGetBuf[ik+1]);
                strcpy(staticGetBuf[ik+1], staticStr);
            }
        }
    }
    char del[] = " ";
    
    switch(num) {
        case 1:
            count = 0;
            goto quiz;
            
        case 2:
            printf("-------------<RANK>-------------\n");
            ss = 0, grade = 1;
            for(ii = 0 ; ii < ucount-1 ; ii++){
               if(ii == 0){
                    printf(" ( ");
                    BLUE;
                    printf("점수");
                    DEFAULT;
                    printf("/정답률/");
                    BLUE;
                    printf("id");
                    DEFAULT;
                    printf("/최근접속시간 )\n");
                }

                strcut = strtok(staticGetBuf[ii], del);
                while(strcut != NULL){
                    printf("%d등 ", grade);
                    switch(ss){
                        case 0:
                            BLUE;
                            printf("%s ",strcut);
                            strcut = strtok(NULL," ");
                            DEFAULT;
                        case 1:
                            printf("%s ",strcut);
                            strcut = strtok(NULL," ");
                        case 2:
                            BLUE;
                            printf("%s ",strcut);
                            strcut = strtok(NULL," ");
                            DEFAULT;
                        default:
                            printf("%s \n",strcut);
                            strcut = strtok(NULL," ");
                    }
                    ss++;
                }
                ss = 0;
                grade++;
            }
            break;

        case 3:
            n = read(fd, cheak, LEN);
            cheak[n] = '\0';
            BLUE;
            fputs(cheak, stdout);
            DEFAULT;
            exit(1);
    }
}

void getLine() {
    alarm(5);
    fgets(ans, LEN, stdin);
    alarm(0);
}

void alarmHandler() {
    strcpy(ans, " ");
    pthread_cancel(tid);
}
