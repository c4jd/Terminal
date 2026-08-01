// mysh - a simple Unix shell

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>


int main(void) {
    char line[1024];
    char *args[64];

    while (1) {
        printf("mysh> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';


        int arg_count = 0;
        args[arg_count] = strtok(line, " ");
        while (args[arg_count] != NULL) {
            arg_count++;
            args[arg_count] = strtok(NULL, " ");
        }

        if (arg_count == 0) continue;


        if (strcmp(args[0], "exit") == 0) {
            break;
        }


        if (strcmp(args[0], "cd") == 0) {
            char *dir = args[1];
            if (dir == NULL) dir = getenv("HOME");

            if (chdir(dir) == -1) {
                perror("cd failed");
            }
            continue;
        }

        int pipe_index = -1;
        for (int i = 0; i < arg_count; i++) {
            if (strcmp(args[i], "|") == 0) {
                pipe_index = i;
                break;
            }
        }

        if (pipe_index != -1) {
            args[pipe_index] = NULL;

            char **left = args;
            char **right = &args[pipe_index + 1];

            int pipe_fdis[2];
            if (pipe(pipe_fdis) == -1) {
                perror("pipe failed");
                continue;
            }

            pid_t p1 = fork();
            if (p1 == 0) {
                close(pipe_fdis[0]);
                dup2(pipe_fdis[1], STDOUT_FILENO);
                close(pipe_fdis[1]);

                execvp(left[0], left);
                perror("execvp failed");
                exit(1);
            }

            pid_t p2 = fork();
            if (p2 == 0) {
                close(pipe_fdis[1]);
                dup2(pipe_fdis[0], STDIN_FILENO);
                close(pipe_fdis[0]);

                execvp(right[0], right);
                perror("execvp failed");
                exit(1);
            }

            close(pipe_fdis[0]);
            close(pipe_fdis[1]);

            waitpid(p1, NULL, 0);
            waitpid(p2, NULL, 0);

            continue;
        }

        char *in_file = NULL;
        char *out_file = NULL;

        for (int i = 0; i < arg_count; i++) {
            if (strcmp(args[i], "<") == 0) {
                in_file = args[i + 1];
                args[i] = NULL;
            }
            else if (strcmp(args[i], ">") == 0) {
                out_file = args[i + 1];
                args[i] = NULL;
            }
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
        }
        else if (pid == 0) {
            if (in_file != NULL) {
                int fd = open(in_file, O_RDONLY);
                if (fd == -1) {
                    perror("open failed");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (out_file != NULL) {
                int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) {
                    perror("open failed");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        }
        else {
            waitpid(pid, NULL, 0);
        }
    }

    return 0;
}