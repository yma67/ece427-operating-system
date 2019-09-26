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

// Specify whether print prompt or error code at compile time!
// gcc tshell.c -o -DPRINT_PROMPT=1 to enable print of prompt
// gcc tshell.c -o -DPRINT_RET=1 to enable error code printing
// gcc tshell.c -o -DPRINT_PROMPT=1 -DPRINT_RET=1 to enable both

#define TRUE            1
#define FALSE           0
#define INPUT_LEN       201
#define HISTORY_LIMIT   100
#define ARGS_LEN        30
#define PWD_LEN         80
#define PROMPT_LEN      500
#define PROMPT          "\033[1;32mott-ads-148\033[0m:\033[1;34m"
#define PROMPT_SUF      "\033[0m$ "
#define PIPE_WARN       "\033[0;33m[Warning]:To enable pipe, a FIFO should be "\
                        "passed as the second argument. Only the first command"\
                        " will be executed.\033[0m\n"
#define CHDIR_ERR       "\033[1;31mPlease specify directory to change\033[0m\n"
#define SETLIM_ERR      "\033[1;31mset limit failed\033[0m\n"
#define SETLIM_NAN      "\033[1;31mInput a number, svp!\033[0m\n"
#define PIPEL_ERR       "\033[1;31mleft child exited with = %d\033[0m\n"
#define PIPER_ERR       "\033[1;31mright child exited with = %d\033[0m\n"
#define CHILD_ERR       "\033[1;31mchild exited with = %d\033[0m\n"
#define EXEC_FAIL       "\033[1;31mExecution Failed\033[0m\n"
#define QUIT_PROMPT     "\n\033[0;33mWould you like to quit [y/N]:\033[0m "
#define GET_LIM         "\033[0;33mold mem limit %lu bytes\033[0m\n"
#define SET_LIM         "\033[0;33mnew mem limit %lu bytes\033[0m\n"

#define GET_IPAST(_i, _chist) (_chist - _i + HISTORY_LIMIT) % HISTORY_LIMIT
#define CATCH_COMMAND(_line, _cmd) !strncmp(_line, _cmd, strlen(_cmd))

#ifdef PRINT_PROMPT
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
#else
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
    fflush(stdout);                                                            \
}
#endif

#define POSIX_SIGNAL(_sigint_handler, _sigtstp_handler) {                      \
    struct sigaction sgint, sgtstp;                                            \
    sgint.sa_flags = 1;                                                        \
    sgint.sa_handler = _sigint_handler;                                        \
    sgtstp.sa_flags = 1;                                                       \
    sgtstp.sa_handler = _sigtstp_handler;                                      \
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
    else fprintf(stdout, CHDIR_ERR);                                           \
    _rc;                                                                       \
})

#define LIMIT(_argc, _argv) ({                                                 \
    int _rc = -1;                                                              \
    if (_argc > 1 && IS_NUMBER(_argv[1])) {                                    \
        struct rlimit _lim;                                                    \
        memset(&_lim, 0, sizeof(struct rlimit));                               \
        getrlimit(RLIMIT_AS, &_lim);                                           \
        _lim.rlim_cur = atoi(_argv[1]);                                        \
        _rc = setrlimit(RLIMIT_AS, &_lim);                                     \
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

#define EXEC(_argc, _argv, _chist, _nhist) {                                   \
    if (CATCH_COMMAND(_argv[0], "history")) {                                  \
        PRINT_HISTORY(_chist, _nhist);                                         \
        exit(EXIT_SUCCESS);                                                    \
    } else {                                                                   \
        if (execvp(_argv[0], _argv) == -1)                                     \
            exit(EXIT_SUCCESS);                                                \
    }                                                                          \
}

#define ARGPARSE(_unparsed, _argc, _args) {                                    \
    int _unparsed_ptr = 0, len = strlen(_unparsed);                            \
    while (_unparsed[_unparsed_ptr] == ' ')                                    \
        _unparsed_ptr += 1;                                                    \
    _argc = 0;                                                                 \
    _args[_argc++] = _unparsed + _unparsed_ptr;                                \
    for (int i = _unparsed_ptr; i < len - 1; i++) {                            \
        if (_unparsed[i] == ' ' || _unparsed[i] == '\n') {                     \
            _unparsed[i] = '\0';                                               \
            _args[_argc++] = _unparsed + i + 1;                                \
        }                                                                      \
    }                                                                          \
    if (_unparsed[len - 1] == ' ' ||                                           \
        _unparsed[len - 1] == '\n')                                            \
        _unparsed[len - 1] = '\0';                                             \
}

#ifdef PRINT_RET
#define SYSTEM(_argc, _argv, _sys_argc, _sys_argv,                             \
               _pipe_ptr, _pipe_argc, _pipe_argv,                              \
               _curr_history, _num_history) ({                                 \
    int _rc = 0;                                                               \
    if (_sys_argc > 1 && _pipe_ptr != NULL) {                                  \
        int _pipe_left, _pipel_stat, _fdp[2];                                  \
        pipe(_fdp);                                                            \
        _pipe_left = fork();                                                   \
        if (_pipe_left < 0) {                                                  \
            _rc = -1;                                                          \
        } else if (_pipe_left == 0) {                                          \
            int _pipe_right, _pipe_stat, _fd;                                  \
            _pipe_right = fork();                                              \
            if (_pipe_right < 0) {                                             \
                _rc = -1;                                                      \
            } else if (_pipe_right == 0) {                                     \
                _fd = open(_sys_argv[1], O_RDONLY);                            \
                dup2(_fd, fileno(stdin));                                      \
                EXEC(_pipe_argc, _pipe_argv,                                   \
                           _curr_history, _num_history);                       \
            } else {                                                           \
                close(_fdp[0]);                                                \
                FILE *_fdpp = fdopen(_fdp[1], "w");                            \
                fprintf(_fdpp, "%d", _pipe_right);                             \
                fflush(_fdpp);                                                 \
                _fd = open(_sys_argv[1], O_WRONLY);                            \
                dup2(_fd, fileno(stdout));                                     \
                EXEC(_argc, _argv, _curr_history, _num_history);               \
            }                                                                  \
        } else {                                                               \
            int _gcpid, _gcstat;                                               \
            close(_fdp[1]);                                                    \
            FILE *_fdpp = fdopen(_fdp[0], "r");                                \
            fscanf(_fdpp, "%d", &_gcpid);                                      \
            waitpid(_gcpid, &_gcstat, WUNTRACED);                              \
            waitpid(_pipe_left, &_pipel_stat, WUNTRACED);                      \
            if (!WIFEXITED(_gcstat))                                           \
                printf(PIPEL_ERR, WEXITSTATUS(_gcstat));                       \
            if (!WIFEXITED(_pipel_stat)) {                                     \
                printf(PIPER_ERR, WEXITSTATUS(_pipel_stat));                   \
                _rc = -1;                                                      \
            }                                                                  \
        }                                                                      \
    } else {                                                                   \
        int _child_pid, _child_status;                                         \
        _child_pid = fork();                                                   \
        if (_child_pid < 0) {                                                  \
            _rc = -1;                                                          \
        } else if (_child_pid == 0) {                                          \
            EXEC(_argc, _argv, _curr_history, _num_history);                   \
        } else {                                                               \
            waitpid(_child_pid, &_child_status, WUNTRACED);                    \
            if (!WIFEXITED(_child_status)) {                                   \
                printf(CHILD_ERR, WEXITSTATUS(_child_status));                 \
                _rc = -1;                                                      \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    _rc;                                                                       \
})
#else
#define SYSTEM(_argc, _argv, _sys_argc, _sys_argv,                             \
               _pipe_ptr, _pipe_argc, _pipe_argv,                              \
               _curr_history, _num_history) ({                                 \
    int _rc = 0;                                                               \
    if (_sys_argc > 1 && _pipe_ptr != NULL) {                                  \
        int _pipe_left, _pipel_stat, _fdp[2];                                  \
        pipe(_fdp);                                                            \
        _pipe_left = fork();                                                   \
        if (_pipe_left < 0) {                                                  \
            _rc = -1;                                                          \
        } else if (_pipe_left == 0) {                                          \
            int _pipe_right, _pipe_stat, _fd;                                  \
            _pipe_right = fork();                                              \
            if (_pipe_right < 0) {                                             \
                _rc = -1;                                                      \
            } else if (_pipe_right == 0) {                                     \
                _fd = open(_sys_argv[1], O_RDONLY);                            \
                dup2(_fd, fileno(stdin));                                      \
                EXEC(_pipe_argc, _pipe_argv,                                   \
                           _curr_history, _num_history);                       \
            } else {                                                           \
                close(_fdp[0]);                                                \
                FILE *_fdpp = fdopen(_fdp[1], "w");                            \
                fprintf(_fdpp, "%d", _pipe_right);                             \
                fflush(_fdpp);                                                 \
                _fd = open(_sys_argv[1], O_WRONLY);                            \
                dup2(_fd, fileno(stdout));                                     \
                EXEC(_argc, _argv, _curr_history, _num_history);               \
            }                                                                  \
        } else {                                                               \
            int _gcpid, _gcstat;                                               \
            close(_fdp[1]);                                                    \
            FILE *_fdpp = fdopen(_fdp[0], "r");                                \
            fscanf(_fdpp, "%d", &_gcpid);                                      \
            waitpid(_gcpid, &_gcstat, WUNTRACED);                              \
            waitpid(_pipe_left, &_pipel_stat, WUNTRACED);                      \
        }                                                                      \
    } else {                                                                   \
        int _child_pid, _child_status;                                         \
        _child_pid = fork();                                                   \
        if (_child_pid < 0) {                                                  \
            _rc = -1;                                                          \
        } else if (_child_pid == 0) {                                          \
            EXEC(_argc, _argv, _curr_history, _num_history);                   \
        } else {                                                               \
            waitpid(_child_pid, &_child_status, WUNTRACED);                    \
        }                                                                      \
    }                                                                          \
    _rc;                                                                       \
})
#endif

sigjmp_buf keep_running;
int curr_history = 0;
int num_history = 0;
int ts_argc = 0;
int pipe_argc = 0;
int has_child = FALSE;
char pwd[PWD_LEN];
char history[HISTORY_LIMIT][INPUT_LEN];
char line[INPUT_LEN];
char prompt[PROMPT_LEN];
char *pipe_ptr = NULL;
char *ts_argv[ARGS_LEN];
char *pipe_argv[ARGS_LEN];

void sigint_handler(int sig) {
    if (has_child)
        return;
    printf(QUIT_PROMPT);
    fflush(stdout);
    char ans[INPUT_LEN] = {0};
    fgets(ans, INPUT_LEN, stdin);
    if (ans[0] == 'y' || ans[0] == 'Y')
        exit(EXIT_SUCCESS);
    siglongjmp(keep_running, 1);
}

void sigtstp_handler(int sig) {
    siglongjmp(keep_running, 1);
}

int main(int argc, char *argv[]) {
    POSIX_SIGNAL(sigint_handler, sigtstp_handler);
    while (TRUE) {
        // get a line
        FRESH_CMD(ts_argv, ts_argc, line, history, curr_history, 
                  prompt, pwd, pipe_ptr, pipe_argv, pipe_argc);
        if (sigsetjmp(keep_running, 1)) {
            FRESH_CMD(ts_argv, ts_argc, line, history, curr_history, 
                      prompt, pwd, pipe_ptr, pipe_argv, pipe_argc);
	    }
	    fgets(line, PROMPT_LEN, stdin);
        // if len < 1 then exit
        if (strlen(line) < 2) 
            exit(EXIT_SUCCESS);
        INSERT_HISTORY(history, curr_history, num_history, line);
        DETECT_PIPE(line, pipe_ptr);
        if (pipe_ptr != NULL && argc < 2)
            printf(PIPE_WARN);
        else if (pipe_ptr != NULL)
            ARGPARSE(pipe_ptr, pipe_argc, pipe_argv);
        ARGPARSE(line, ts_argc, ts_argv);
        // exec
        int rc;
        has_child = TRUE;
        if (CATCH_COMMAND(ts_argv[0], "exit")) {
            exit(EXIT_SUCCESS);
        } else if (CATCH_COMMAND(ts_argv[0], "chdir") ||
                   CATCH_COMMAND(ts_argv[0], "cd")) {
            rc = CHDIR(ts_argc, ts_argv);
        } else if (argc > 1 && pipe_ptr != NULL && 
                   CATCH_COMMAND(pipe_argv[0], "chdir")) {
            rc = CHDIR(pipe_argc, pipe_argv);
        } else if (CATCH_COMMAND(ts_argv[0], "limit")) {
            rc = LIMIT(ts_argc, ts_argv);
        } else if (argc > 1 && pipe_ptr != NULL && 
                   CATCH_COMMAND(pipe_argv[0], "limit")) {
            rc = LIMIT(pipe_argc, pipe_argv);
        } else {
            rc = SYSTEM(ts_argc, ts_argv, argc, argv, 
                        pipe_ptr, pipe_argc, pipe_argv, 
                        curr_history, num_history);
        }
        has_child = FALSE;
        // error handling
        if (rc == -1)
            printf(EXEC_FAIL);
    }
    printf("\n");
    return EXIT_SUCCESS;
}
