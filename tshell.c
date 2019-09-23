#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>

#define TRUE          1
#define FALSE         0
#define INPUT_LEN     201
#define HISTORY_LIMIT 4
#define ARGS_LEN      30
#define PWD_LEN       80
#define PROMPT_LEN    500
#define PROMPT        "ott-ads-148:"
#define PROMPT_SUF    " > "
#define CMD_DELIM     " \n"

#define GET_IPAST(_i, _chist) (_chist - _i + HISTORY_LIMIT) % HISTORY_LIMIT
#define CATCH_COMMAND(_line, _cmd) !strncmp(_line, _cmd, strlen(_cmd))

#define FRESH_CMD(_argv, _argc, _line, _history, _history_count, _prompt,      \
                  _pwd, _pipe_ptr, _pipe_argv, _pipe_argc) {                   \
    _argc = 0;                                                                 \
    _pipe_argc = 0;                                                            \
    _pipe_ptr = NULL;                                                          \
    memset(_line, '\0', sizeof(char) * INPUT_LEN);                             \
    memset(_pwd, '\0', sizeof(char) * PWD_LEN);                                \
    memset(_pipe_argv, 0, sizeof(char *) * ARGS_LEN);                          \
    memset(_argv, 0, sizeof(char *) * ARGS_LEN);                               \
    memset(_history[_history_count], '\0', sizeof(char) * INPUT_LEN);          \
    memset(_prompt, '\0', sizeof(char) * PROMPT_LEN);                          \
    getcwd(_pwd, PWD_LEN);                                                     \
    strcpy(_prompt, PROMPT);                                                   \
    strcat(_prompt, _pwd);                                                     \
    strcat(_prompt, PROMPT_SUF);                                               \
    printf("%s", _prompt);                                                     \
    fflush(stdout);                                                            \
}

#define POSIX_SIGNAL {                                                         \
    struct sigaction sgint, sgtstp;                                            \
    sgint.sa_flags = 1;                                                        \
    sgint.sa_handler = sigint_handler;                                         \
    sgtstp.sa_flags = 1;                                                       \
    sgtstp.sa_handler = sigtstp_handler;                                       \
    sigaction(SIGINT, &sgint, NULL);                                           \
    sigaction(SIGTSTP, &sgtstp, NULL);                                         \
}

#define INSERT_HISTORY(_hist, _chist, _num_hist, _line) {                      \
    strcpy(_hist[_chist], _line);                                              \
    _hist[_chist][strlen(_line) - 1] = '\0';                                   \
    _chist = (_chist + 1) % HISTORY_LIMIT;                                     \
    _num_hist += 1;                                                            \
}

#define PRINT_HISTORY(_chist, _num_hist) {                                     \
    for (int i = 1; i < HISTORY_LIMIT + 1; i++) {                              \
        if (i > _num_hist)                                                     \
            break;                                                             \
        fprintf(stdout, "%d\t%s\n", i, history[GET_IPAST(i, _chist)]);         \
    }                                                                          \
}

#define IS_NUMBER(_str) ({                                                     \
    int _is_num = 1;                                                           \
    for (int i = 0; i < strlen(_str); i++) {                                   \
        if (!isdigit(_str[i])) {                                               \
            _is_num = 0;                                                       \
            break;                                                             \
        }                                                                      \
    }                                                                          \
    _is_num;                                                                   \
})

#define CHDIR(_argc, _argv) ({                                                 \
    int _rc = 0;                                                               \
    if (_argc > 1) _rc = chdir(_argv[1]);                                      \
    else fprintf(stdout, "Please specify directory to change.\n");             \
    _rc;                                                                       \
})

#define LIMIT(_argc, _argv) ({                                                 \
    int _rc;                                                                   \
    if (_argc > 1 && IS_NUMBER(_argv[1])) {                                    \
        struct rlimit old_lim, new_lim;                                        \
        memset(&old_lim, 0, sizeof(struct rlimit));                            \
        getrlimit(RLIMIT_AS, &old_lim);                                        \
        fprintf(stdout, "old mem limit %ju bytes\n", old_lim.rlim_cur);        \
        memcpy(&new_lim, &old_lim, sizeof(struct rlimit));                     \
        new_lim.rlim_cur = atoi(_argv[1]);                                     \
        if (setrlimit(RLIMIT_AS, &new_lim) != -1) {                            \
            getrlimit(RLIMIT_AS, &new_lim);                                    \
            fprintf(stdout, "new mem limit %ju bytes\n", new_lim.rlim_cur);    \
        } else {                                                               \
            fprintf(stdout, "set limit failed\n");                             \
            _rc = -1;                                                          \
        }                                                                      \
    } else {                                                                   \
        fprintf(stdout, "Input a number, svp!\n");                             \
        _rc = -1;                                                              \
    }                                                                          \
    _rc;                                                                       \
})

#define DETECT_PIPE(_line, _pipe_ptr) {                                        \
    for (int i = 0; i < strlen(_line); i++) {                                  \
        if (_line[i] == '|') {                                                 \
            _line[i] = '\0';                                                   \
            _pipe_ptr = _line + i + 1;                                         \
            break;                                                             \
        }                                                                      \
    }                                                                          \
}

#define EXEC(_argc, _argv, _chist, _nhist) ({                                  \
    int _rc;                                                                   \
    if (CATCH_COMMAND(_argv[0], "history")) {                                  \
        PRINT_HISTORY(_chist, _nhist);                                         \
        exit(EXIT_SUCCESS);                                                    \
    } else {                                                                   \
        _rc = execvp(_argv[0], _argv);                                         \
    }                                                                          \
    _rc;                                                                       \
})

sigjmp_buf keep_running;
int curr_history = 0;
int num_history = 0;
int my_argc = 0;
int pipe_argc = 0;
char pwd[PWD_LEN];
char history[HISTORY_LIMIT][INPUT_LEN];
char line[INPUT_LEN];
char prompt[PROMPT_LEN];
char *pipe_ptr = NULL;
char *my_argv[ARGS_LEN];
char *pipe_argv[ARGS_LEN];

void sigint_handler(int sig);
void sigtstp_handler(int sig);
void argparse(char unparsed[], int *argv, char *args[]); 
void my_system(char *my_argv[], int sys_argc, char *sys_argv[], 
               char *pipe_ptr, int pipe_argc, char *pipe_argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please specify location of FIFO pipe.\n");
        exit(EXIT_FAILURE);
    }
    POSIX_SIGNAL;
    while (TRUE) {
        FRESH_CMD(my_argv, my_argc, line, history, curr_history, prompt, 
                  pwd, pipe_ptr, pipe_argv, pipe_argc);
        fgets(line, INPUT_LEN, stdin);
        if (sigsetjmp(keep_running, 1)) 
            continue;
        if (strlen(line) <= 1)
            break;
        INSERT_HISTORY(history, curr_history, num_history, line);
        DETECT_PIPE(line, pipe_ptr);
        if (pipe_ptr != NULL)
            argparse(pipe_ptr, &pipe_argc, pipe_argv);
        argparse(line, &my_argc, my_argv);
        if (CATCH_COMMAND(my_argv[0], "chdir")) {
            CHDIR(my_argc, my_argv);
        } else if (pipe_ptr != NULL && CATCH_COMMAND(pipe_argv[0], "chdir")) {
            CHDIR(pipe_argc, pipe_argv);
        } else if (CATCH_COMMAND(my_argv[0], "limit")) {
            LIMIT(my_argc, my_argv);
        } else if (pipe_ptr != NULL && CATCH_COMMAND(pipe_argv[0], "limit")) {
            LIMIT(pipe_argc, pipe_argv);
        } else {
            my_system(my_argv, argc, argv, pipe_ptr, pipe_argc, pipe_argv);
        }
    }
    printf("\n");
    return EXIT_SUCCESS;
}

void sigint_handler(int sig) {
    POSIX_SIGNAL;
    printf("\nWould you like to quit [y/N]: ");
    fflush(stdout);
    char ans[INPUT_LEN] = {0};
    fgets(ans, INPUT_LEN, stdin);
    if (ans[0] == 'y' || ans[0] == 'Y') 
        exit(EXIT_SUCCESS);
    siglongjmp(keep_running, 1);
}

void sigtstp_handler(int sig) {
    POSIX_SIGNAL;
    siglongjmp(keep_running, 1);
}

void argparse(char unparsed[], int *argv, char *args[]) {
    int i = 0;
    char *carg = strtok(unparsed, CMD_DELIM);
    while (carg != NULL) {
        args[i] = carg;
        i = i + 1;
        carg = strtok(NULL, CMD_DELIM);
        if (carg != NULL)
            *(carg - 1) = '\0';
    }
    *argv = i;
}

void my_system(char *my_argv[], int sys_argc, char *sys_argv[], 
               char *pipe_ptr, int pipe_argc, char *pipe_argv[]) {
    int rc;
    if (pipe_ptr != NULL) {
        int pipe_left, pipel_stat, fdp[2];
        char buff[200];
        memset(buff, 0, sizeof(char) * 200);
        pipe(fdp);
        pipe_left = fork();
        if (pipe_left < 0) {
            rc = -1;
        } else if (pipe_left == 0) {
            int pipe_right, pipe_stat, fd;
            pipe_right = fork();
            if (pipe_right < 0) {
                rc = -1;
            } else if (pipe_right == 0) {
                fd = open(sys_argv[1], O_RDONLY);
                dup2(fd, fileno(stdin));
                rc = EXEC(pipe_argc, pipe_argv, curr_history, num_history);
            } else {
                close(fdp[0]);
                FILE *fdpp = fdopen(fdp[1], "w");
                fprintf(fdpp, "%d", pipe_right);
                fflush(fdpp);
                fd = open(sys_argv[1], O_WRONLY);
                dup2(fd, fileno(stdout));
                rc = EXEC(my_argc, my_argv, curr_history, num_history);
            }
        } else {
            int gcpid, gcstat;
            close(fdp[1]);
            FILE *fdpp = fdopen(fdp[0], "r");
            fscanf(fdpp, "%d", &gcpid);
            waitpid(gcpid, &gcstat, WUNTRACED);
            waitpid(pipe_left, &pipel_stat, WUNTRACED);
        }
    } else {
        int child_pid, child_status;
        child_pid = fork();
        if (child_pid < 0) {
            rc = -1;
        } else if (child_pid == 0) {
            rc = EXEC(my_argc, my_argv, curr_history, num_history);
        } else {
            rc = waitpid(child_pid, &child_status, WUNTRACED);
        }
    }
    if (rc < 0) 
        exit(EXIT_FAILURE);
}


