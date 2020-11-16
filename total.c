#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#define MAX_BUF 255

int getTokken(char* input, char** argv) { //토큰 분석 함수
    int off_s = 0;
    int off_e = 0;
    int argc = 0;
    char temp;

    while (input[off_e] != '\n') {
        temp = input[off_e];
        if (temp == ' ') {
            strncpy(argv[argc], input + off_s, off_e - off_s);
            argc++;
            off_s = off_e + 1;
        }
        off_e++;
    }
    strncpy(argv[argc], input + off_s, off_e - off_s);
    argc++;
    argv[argc] = (char*)0;
    return argc;
}

int inner_command(char** argv) { // 내부 명령어 처리 함수
    if (strncmp("exit", argv[0], 4) == 0) {
        printf("Good Bye!!\n");
        exit(1);
    }
    if (strncmp("cd", argv[0], 2) == 0) {
        chdir(argv[1]);
        return 1;
    }
    return 0;
}

void do_command(char** argv) { // 일반 외부 명령어 처리 함수
    pid_t pid;
    int exit_status;
    if ((pid = fork()) == -1) {
        printf("fork() 에러\n");
    }
    else if (pid == 0) {
        if (execvp(argv[0], argv) == -1)if (strlen(argv[0]) != 0)printf("명령을 찾을 수 없습니다. %s\n", argv[0]);
        exit(1);
    }
    else {
        wait(&exit_status);
    }
}

void do_pipe_command(int pp, char** argv) { // 파이프 명령어 처리 함수
    char** fw_argv;
    char** bw_argv;
    int i, j = 0;
    pid_t pid;
    int fd[2];

    fw_argv = (char**)malloc(32 * sizeof(char*));
    for (i = 0; i < 32; i++)fw_argv[i] = (char*)malloc(64 * sizeof(char));
    bw_argv = (char**)malloc(32 * sizeof(char*));
    for (i = 0; i < 32; i++)bw_argv[i] = (char*)malloc(64 * sizeof(char));

    for (i = 0; i < pp; i++) {
        strcpy(fw_argv[i], argv[i]);
    }
    fw_argv[pp] = (char*)0;
    for (i = pp + 1; argv[i] != '\0'; i++) {
        strcpy(bw_argv[j], argv[i]);
        j++;
    }
    bw_argv[j] = (char*)0;

    // program1의 표준출력을 program2의 표준입력으로 입력 받음
    pipe(fd);
    if ((pid = fork()) == -1) { // 프로세스 호출과 -1이면 비정상 호출로 에러 발생
        printf("fork() 에러\n");
    }
    else if (pid == 0) {
        if (fork() == 0) {
            close(fd[1]);
            dup2(fd[0], 0); // 복사본 생성
            close(fd[0]);
            if (execvp(bw_argv[0], bw_argv) == -1)if (strlen(bw_argv[0]) != 0)printf("명령을 찾을 수 없습니다. %s\n", bw_argv[0]);
            exit(1);
        }
        else {
            close(fd[0]);
            dup2(fd[1], 1); // 복사
            close(fd[1]);
            if (execvp(fw_argv[0], fw_argv) == -1)if (strlen(fw_argv[0]) != 0)printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
            exit(1);
        }
    }
    else {
        wait(1);
    }
}

void do_redirect_command(int flag, int dp, char** argv) { // 리다이렉션 명령어 처리 함수
    char** fw_argv;
    char** bw_argv;
    int i, j = 0;
    int fd;
    int exit_status;
    pid_t pid; // 프로세스를 호출하고 제거하여 원활한 사용을 위한 pid 변수

    fw_argv = (char**)malloc(32 * sizeof(char*));
    for (i = 0; i < 32; i++)fw_argv[i] = (char*)malloc(64 * sizeof(char));
    bw_argv = (char**)malloc(32 * sizeof(char*));
    for (i = 0; i < 32; i++)bw_argv[i] = (char*)malloc(64 * sizeof(char));

    for (i = 0; i < dp; i++) {
        strcpy(fw_argv[i], argv[i]);
    }
    fw_argv[dp] = (char*)0;
    strcpy(bw_argv[0], argv[dp + 1]);
    bw_argv[dp + 2] = (char*)0;


    switch (flag) {
    case 1: // " > " 표준 출력을 파일로 재지향 합니다. 파일이 없으면 새로 만들고, 파일이 있으면 덮어씁니다.
        if ((pid = fork()) == -1) { printf("fork() 에러"); exit(1); }
        else if (pid == 0) {
            close(fd);
            fd = open(bw_argv[0], O_RDWR | O_CREAT | O_TRUNC, 0644); //
            dup2(fd, STDOUT_FILENO); // 값을 STDOUT_FILENO로 지정
            write(fd, STDOUT_FILENO, sizeof(STDOUT_FILENO));
            close(fd);
            if (execvp(fw_argv[0], fw_argv) == -1)if (strlen(fw_argv[0]) != 0)printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
            exit(1);
        }
        else {
            wait(&exit_status);
        }
        break;
    case 2: // ">>" 표준 출력을 파일로 재지향 합니다. 파일이 없으면 새로 만들고, 파일이 있으면 파일의 끝에 덧붙입니다.
        if ((pid = fork()) == -1) { printf("fork() 에러"); exit(1); }
        else if (pid == 0) {
            close(fd);
            fd = open(bw_argv[0], O_RDWR | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO); // 값을 STDOUT_FILENO로 지정
            write(fd, STDOUT_FILENO, sizeof(STDOUT_FILENO));
            close(fd);
            if (execvp(fw_argv[0], fw_argv) == -1)if (strlen(fw_argv[0]) != 0)printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
            exit(1);
        }
        else {
            wait(&exit_status);
        }
        break;
    case 3: // "<" 파일로부터 표준 입력을 받도록 재지향합니다.
        if ((pid = fork()) == -1) { printf("fork() 에러"); exit(1); }
        else if (pid == 0) {
            fd = open(bw_argv[0], O_RDONLY);
            dup2(fd, STDIN_FILENO); // 값을 STDIN_FILENO로 지정
            close(fd);
            if (execvp(fw_argv[0], fw_argv) == -1)if (strlen(fw_argv[0]) != 0)printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
            exit(1);
        }
        else {
            wait(&exit_status);
        }
        break;
    default:
        break;
    }

}
void do_bg_command(int bp, char** argv) { // 백그라운드 명령어 처리 함수
    pid_t pid;
    int exit_status, i;
    char** fw_argv;

    fw_argv = (char**)malloc(32 * sizeof(char*));
    for (i = 0; i < 32; i++)fw_argv[i] = (char*)malloc(64 * sizeof(char));

    for (i = 0; i < bp; i++) {
        strcpy(fw_argv[i], argv[i]);
    }

    fw_argv[bp] = (char*)0;
    if ((pid = fork()) == -1) {
        printf("fork() 에러\n");
    }
    else if (pid == 0) {
        if (fork() == 0) {
            printf("pid : %d", getpid());
            if (execvp(fw_argv[0], fw_argv) == -1)if (strlen(fw_argv[0]) != 0)printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
            exit(1);
        }
        else
        {
            exit(1);
        }
    }
    else {
        int status;
        // 자식프로세스의 종료 혹은 STOP를 기다린다. 
        wait(&exit_status);
    }
}

int check_command(int argc, char** argv) { // 파이프/입출력재지정/백그라운드 체크 함수   
    int i = 0;
    int find[2] = { 0, };
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "&") == 0) {
            find[0] = 1;
            find[1] = i;
            break;
        }
        else if (strcmp(argv[i], ">") == 0) {
            find[0] = 2;
            find[1] = i;
            break;
        }
        else if (strcmp(argv[i], ">>") == 0) {
            find[0] = 3;
            find[1] = i;
            break;
        }
        else if (strcmp(argv[i], "<") == 0) {
            find[0] = 4;
            find[1] = i;
            break;
        }
        else if (strcmp(argv[i], "|") == 0) {
            find[0] = 5;
            find[1] = i;
            break;
        }
        else {
            find[0] = 0;
            find[1] = i;
        }
    }

    switch (find[0]) {
    case 0:
        do_command(argv);
        break;
    case 1:
        do_bg_command(find[1], argv);
        break;
    case 2:
        do_redirect_command(find[0], find[1], argv);
        break;
    case 3:
        do_redirect_command(find[0], find[1], argv);
        break;
    case 4:
        do_redirect_command(find[0], find[1], argv);
        break;
    case 5:
        do_pipe_command(find[1], argv);
        break;
    default:
        break;
    }
}

int main()
{
    char prompt[] = "[HG's_Shell ";
    char* dir;
    int inner_flag;
    int i;
    // 전처리: 쉘 설명 출력
    printf("--------------------------------------\n");
    printf(" HG's Shell을 시작합니다!\n");
    printf(" [ 기능 ]\n");
    printf(" 쉘 명령어 : cd, exit\n");
    printf(" 외부 명령어 : pipe(|), redirection(|), background(&)\n");
    printf(" -> 외부 명령어 사용에는 약간의 제약이 있습니다.\n");
    printf("--------------------------------------\n");

    while (1) {
        char command[MAX_BUF] = { '\0', }; //명령어
        char** argv; //인자
        int argc = 0;
        dir = (char*)malloc(64 * sizeof(char)); //동적사이즈
        getcwd(dir, MAX_BUF); //동적으로 받음
        // 포인터 변수 메모리 동적 할당
        argv = (char**)malloc(32 * sizeof(char*));
        for (i = 0; i < 32; i++)argv[i] = (char*)malloc(64 * sizeof(char));

        //      printf("HG Shell %s > ", strrchr(dir,'/') );
              //쉘 프롬프트 출력. 그리고 표준 입력에서 명령어 라인 읽기
        write(STDOUT_FILENO, prompt, sizeof(prompt));
        write(STDOUT_FILENO, strrchr(dir, '/') + 1, strlen(strrchr(dir, '/')));
        write(STDOUT_FILENO, "> ", 3);
        read(STDIN_FILENO, command, MAX_BUF);

        //명령을 토큰 별로 구분
        argc = getTokken(command, argv);
        //      for(i=0;i<argc;i++)printf("argv[%d] : %s(%d)\n", i, argv[i],strlen(argv[i]));

              //토큰이 내부 명령일 경우 처리
        inner_flag = inner_command(argv);

        // 토큰에서 파이프, 입출력 재지정, 백 그라운드, 다중 명령 처리
        // 쉘 스크립트 처리
        // 외부 명령 처리
        if (inner_flag == 0)check_command(argc, argv);
    }
    return 0;
}
