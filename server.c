#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define oops(msg) {perror(msg); exit(1);}

#define DEFAULT printf("%c[%dm", 0x1B, 0)
#define BLUE printf("\x1b[36m")
#define MAGENTA printf("\x1b[35m")
#define YELLOW printf("\x1b[33m")

extern int make_server_socket(int);
void process_request(int);
int getText(FILE *fp, char **buf, char *fileName);          //txt 파일 읽기
void lower(char *str);                                      //소문자로 바꾸기
void childHandler(int signal);

int main(int ac, char *av[])
{
    int sock, fd;
    YELLOW;
    printf("waiting for clients...\n\n");
    DEFAULT;
    srand(time(NULL));
    sock=make_server_socket(atoi(av[1]));
    if(sock==-1)
        exit(1);
    
    signal(SIGCHLD, (void *)childHandler);
    
    while(1){
        fd = accept(sock, NULL,NULL);
        if(fd==-1)
            break;
         
        process_request(fd);
        close(fd);
    }
}

void process_request(int fd)
{
    char *bufQ[BUFSIZ], *bufA[BUFSIZ], *bufH[BUFSIZ];
    FILE *fpQ, *fpA, *fpH;
    int qCount = 0, random = 0;
    int hCount[BUFSIZ];
    memset(hCount, 0, sizeof(hCount));
    
    qCount = getText(fpQ, bufQ, "question.txt");
    getText(fpA, bufA, "answer.txt");
    getText(fpH, bufH, "hint.txt");

    char question[BUFSIZ], answer[BUFSIZ], received_ans[BUFSIZ];
    int n, count = 1;
    int m[BUFSIZ];
    
    char id[BUFSIZ];
    int answerNum;
    int score;
    float answerRate;

    char lastAccessTime[BUFSIZ];        //클라이언트의 최근접속시간을 나타내는 버퍼
    int pid = fork();
    
    switch(pid) {
        case -1:
            return;
        case 0:
quiz:
            n = read(fd, id, BUFSIZ);
            id[n-1] = '\0';
            printf("Client ");
            BLUE;
            printf("'%s'", id);
            DEFAULT;
            n = read(fd, lastAccessTime, BUFSIZ);
            printf("가 %s에 접속하였습니다.\n", lastAccessTime);          
           
            qCount = getText(fpQ, bufQ, "question.txt");
            
            for(int i = 0; i < 5; i++)
                while(1) {
                    random = rand() % qCount;
                    for(int j = 0; j < i; j++)
                        if(m[j] == random)
                            random = -1;
                    if(random >= 0) {
                        m[i] = random;
                        break;
                    }
                }

            qCount = 0;
            answerNum = 0;
            score = 0;

            do{
                sprintf(question, "Q%d. %s", count, bufQ[m[count - 1]]);
                write(fd, question, strlen(question));

                n = read(fd, received_ans, BUFSIZ);
                lower(received_ans);

                if(strcmp(received_ans, bufA[m[count-1]]) == 0){
                    answerNum++;
                    if(hCount[count] != 0)
                        score += 10;
                    else
                        score += 15;
                    sprintf(answer, "<<<Q%d : 정답입니다.>>>", count++);
                    write(fd, answer, strlen(answer));
                    qCount++;
                }
        
                else if(strstr(received_ans, "hint") != NULL) { 
                    if(hCount[count] == 0) {
                        sprintf(answer, "\n★★★★★hint : %s", bufH[m[count-1]]);
                        write(fd, answer, strlen(answer));
                        hCount[count]++;
                    }
                    else{
                        strcpy(answer, "\n****힌트를 이미 썼습니다.");
                        write(fd, answer, strlen(answer));
                    }       
                }

                else {
                    sprintf(answer, "<<<Q%d : 오답입니다.>>>\n정답 : %s", count++, bufA[m[count-1]]);
                    write(fd, answer, strlen(answer));
                    qCount++;
                }
                sleep(1);
                memset(question, 0, sizeof(question));
                memset(answer, 0, sizeof(answer));
                memset(received_ans, 0, sizeof(received_ans));
            } while(qCount != 5);
            
            memset(hCount, 0, sizeof(hCount));
            
            char arBuf[BUFSIZ];
            answerRate = (((float)answerNum/(float)(count-1))*100.0);
            sprintf(answer, "정답률 = %.2f%%, 최종 점수 : %d점\n", answerRate, score);
            write(fd, answer, strlen(answer));
                       
            printf("Client ");
            MAGENTA;
            printf("'%s'", id);
            DEFAULT;
            printf("의 %s", answer);
            memset(answer, 0, sizeof(answer));

            char staticBuf[BUFSIZ];
            int fd2 = open("clientStatic.txt",O_WRONLY | O_CREAT | O_APPEND,0644);
            sprintf(staticBuf,"%d %.2f%% %s %s\n",score, answerRate,id,lastAccessTime);
            write(fd2, staticBuf, strlen(staticBuf));
            close(fd2);

            sleep(1);
            strcpy(answer, "선택해주세요.(번호)\n1. AGAIN?\n2. RANK\n3. QUIT\n");
            write(fd, answer, strlen(answer));
            memset(answer, 0, sizeof(answer));

            n = read(fd, received_ans, BUFSIZ);
            int num = atoi(received_ans);
            
            switch(num) {
                case 1:
                    count = 1;
                    goto quiz;
                    break;
                case 2:
                    break;
                case 3:
                    strcpy(answer, "Thank you! Good bye~\n");
                    write(fd, answer, strlen(answer));
                    printf("Client ");
                    MAGENTA;
                    printf("'%s'", id);
                    DEFAULT;
                    printf("가 나갔습니다.\n");
            }
    }
}

void childHandler(int siganl) {
    int status;
    pid_t spid;
   
    /*-1 : child process 모두 다 받는다.
    WNOHANG : child process가 종료되지 않아도 부모는 자신 할 일 한다.*/
    while((spid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Client ");
        MAGENTA;
        printf("'%d'", spid);
        DEFAULT;
        printf("(pid)가 중간에 종료되었습니다.\n");
    }
}

int getText(FILE *fp, char **buf, char *fileName){
    fp = fopen(fileName, "r");
    int count = 0;
    char temp[100];
    while(1){
        fgets(temp, 100, fp);
        if(feof(fp)) break;
        char *bufM = malloc(strlen(temp) * sizeof(char));
        strcpy(bufM, temp);
        buf[count++] = bufM;
    }
    return count;
}

void lower(char *str) {
    int i;
    char ch;
    
    for(i = 0 ; str[i] != '\0'; ++i) {
        ch = str[i];
        if(ch >= 'A' && ch <= 'Z') {
            str[i] = tolower(ch);
        }
    }
}
