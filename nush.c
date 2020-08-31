#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>

#include "nush.h"

int handleVars(svec* sv, hashmap* map) {
    for (int i = 0; i < sv->size; i++) {
        if (sv->data[i][0] == '$') { //substitute vars
            char* var = malloc(strlen(sv->data[i]) * sizeof(char));
            for (int j = 1; j < strlen(sv->data[i]); j++) { //parse var out sv
                var[j - 1] = sv->data[i][j];
            }
            var[strlen(sv->data[i]) - 1] = '\0';

            free(sv->data[i]);
            sv->data[i] = strdup(hashmap_get(map, var)); //replace var with value
            free(var); 
        }
    }

    for (int i = 0; i < sv->size; i++) {
        if (strcmp(sv->data[i], "=") == 0) { //assignment
            hashmap_put(map, sv->data[i - 1], sv->data[i + 1]);
            return 0;
        }
    }
    return -1;
}

int handleSemicolon(svec* sv, hashmap* map, int bckgd) {
    for (int i = 0; i < sv->size; i++) {
        if (strcmp(sv->data[i], ";") == 0) { //semicolon
            svec* first = make_svec(i);
            for (int j = 0; j < i; j++) {
                svec_push_back(first, sv->data[j]);
            }
            i++;
            int status = execute(first, map, bckgd); //left side of 1st semicolon
            free_svec(first);
            if (status == -2) { //exit
                return status;
            }
            svec* second = make_svec(sv->size - i);
            for (int j = i; j < sv->size; j++) {
                svec_push_back(second, sv->data[j]);
            }
            if (second->size != 0) {
                status = execute(second, map, bckgd); //right side of 1st semicolon
                free_svec(second);
            }
            return status;
        }
    }
    return -1;
}

int handleBckgd(svec* sv, hashmap* map, int bckgd) {
    for (int i = 0; i < sv->size; i++) {
        if (strcmp(sv->data[i], "&") == 0) { //background
            svec* first = make_svec(i);
            for (int j = 0; j < i; j++) {
                svec_push_back(first, sv->data[j]);
            }
            i++;
            int status = execute(first, map, 1); //left side of &
            free_svec(first);
            if (status == -2) { //exit
                return status;
            }            
            svec* second = make_svec(sv->size - i);
            for (int j = i; j < sv->size; j++) {
                svec_push_back(second, sv->data[j]);
            }
            if (second->size != 0) {
                status = execute(second, map, bckgd); //right side of &
                free_svec(second);
            }
            return status;
        }
    }
    return -1;
}

int handleAndOr(svec* sv, hashmap* map, int bckgd) {
    for (int i = 0; i < sv->size; i++) {
        if (strcmp(sv->data[i], "&&") == 0) { //&&
            svec* first = make_svec(i);
            for (int j = 0; j < i; j++) {
                svec_push_back(first, sv->data[j]);
            }
            i++;
            int status = execute(first, map, bckgd); //left side of &&
            free_svec(first);
            if (status == -2) { //exit
                return status;
            }
            svec* second = make_svec(sv->size - i);
            for (int j = i; j < sv->size; j++) {
                svec_push_back(second, sv->data[j]);
            }
            if (status == 0) {
                status = execute(second, map, bckgd); //right side of &&
            }
            free_svec(second);

            return status;
        } else if (strcmp(sv->data[i], "||") == 0) { //||
            svec* first = make_svec(i);
            for (int j = 0; j < i; j++) {
                svec_push_back(first, sv->data[j]);
            }
            i++;
            int status = execute(first, map, bckgd); //left side of ||
            free_svec(first);
            if (status == -2) { //exit
                return status;
            }
            svec* second = make_svec(sv->size - i);
            for (int j = i; j < sv->size; j++) {
                svec_push_back(second, sv->data[j]);
            }      
            if (status != 0) {
                status = execute(second, map, bckgd); //right side of ||
            }
            free_svec(second);
            return status;
        }
    }
    return -1;
}

int handleRedirects(svec* sv, hashmap* map, int bckgd) {
    int redirhappened = 0;
    int numPipes = 0;
    for (int i = 0; i < sv->size; i++) {
        if (strcmp(sv->data[i], "|") == 0) {
            numPipes++;
        }
    }
    int stdin_fd;
    int stdout_fd;
    int pipesProcessed = 0;
    int pipes[2];
    int status_check;
    int start = 0;
    for (int i = 0; i < sv->size; i++) {
        if (strcmp(sv->data[i], "|") == 0) {
            //This pipe code works when there is only one pipe but not more.
            //When there is more than one, it seems like the shell produces
            //the correct output, but it doesn't print it to the screen and
            //hangs forever.
            redirhappened = 1;
            if (pipesProcessed == 0) {
                svec* first = make_svec(i);
                for (int j = 0; j < i; j++) {
                    svec_push_back(first, sv->data[j]);
                }
                i++;
                start = i;

                stdin_fd = dup(0);
                assert(stdin_fd != -1);
                stdout_fd = dup(1);
                assert(stdout_fd != -1);
                status_check = pipe(pipes);
                assert(status_check != -1);

                status_check = close(1);
                assert(status_check != -1);
                status_check = dup(pipes[1]);
                assert(status_check != -1);
                status_check = close(pipes[1]);
                assert(status_check != -1);

                execute(first, map, bckgd); //left side of pipe
                free_svec(first);

                status_check = close(0);
                assert(status_check != -1);
                status_check = dup(pipes[0]);
                assert(status_check != -1);
                status_check = close(pipes[0]);
                assert(status_check != -1);

                while (i < sv->size && strcmp(sv->data[i], "|") != 0) {
                   i++;
                }
            }

            if (pipesProcessed + 1 == numPipes) {
                status_check = close(1);
                assert(status_check != -1);
                status_check = dup(stdout_fd);
                assert(status_check != -1);
                status_check = close(stdout_fd);
                assert(status_check != -1);
            }
            svec* cmd = make_svec(i - start);
            for (int j = start; j < i; j++) {
                svec_push_back(cmd, sv->data[j]);
            }
            i++;
            start = i;
            execute(cmd, map, bckgd); //executes each command in pipe except the first one
            free_svec(cmd);
            if (pipesProcessed + 1 == numPipes) {
                status_check = close(0);
                assert(status_check != -1);
                status_check = dup(stdin_fd);
                assert(status_check != -1);
                status_check = close(stdin_fd);
                assert(status_check != -1);
            }
            pipesProcessed++;

            //end if
            //
            //The following is code for the base case of pipe: 1 pipe.
            /*
            if (pipesProcessed == 0) {
            svec* first = make_svec(i);
            for (int j = 0; j < i; j++) {
                svec_push_back(first, sv->data[j]);
            }
            i++;
            
            redirhappened = 1;
            stdin_fd = dup(0);
            assert(stdin_fd != -1);
            stdout_fd = dup(1);
            assert(stdout_fd != -1);
            status_check = pipe(pipes);
            assert(status_check != -1);
           
            status_check = close(1);
            assert(status_check != -1);
            status_check = dup(pipes[1]);
            assert(status_check != -1);
            status_check = close(pipes[1]);
            assert(status_check != -1);

            execute(first, map, bckgd);
            free(first);
            
            status_check = close(1);
            assert(status_check != -1);
            status_check = dup(stdout_fd);
            assert(status_check != -1);
            status_check = close(stdout_fd);
            assert(status_check != -1);

            svec* second = make_svec(sv->size - i);
            for (int j = i; j < sv->size; j++) {
                svec_push_back(second, sv->data[j]);
            }

            status_check = close(0);
            assert(status_check != -1);
            status_check = dup(pipes[0]);
            assert(status_check != -1);
            status_check = close(pipes[0]);
            assert(status_check != -1);

            execute(second, map, bckgd);
            free(second);

            status_check = close(0);
            assert(status_check != -1);
            status_check = dup(stdin_fd);
            assert(status_check != -1);
            status_check = close(stdin_fd);
            assert(status_check != -1);
            */

        } else if (strcmp(sv->data[i], "<") == 0) { //<
            redirhappened = 1;
            svec* first = make_svec(i);
            for (int j = 0; j < i; j++) {
                svec_push_back(first, sv->data[j]);
            }
            i++;

            int fd = open(sv->data[i], O_RDONLY, 0644);
            assert(fd != -1);
            stdin_fd = dup(1);
            assert(stdin_fd != -1);

            status_check = close(0);
            assert(status_check != -1);
            status_check = dup(fd);
            assert(status_check != -1);
            status_check = close(fd);
            assert(status_check != -1);

            execute(first, map, bckgd);
            free_svec(first);

            status_check = close(0);
            assert(status_check != -1);
            status_check = dup(stdin_fd);
            assert(status_check != -1);
            status_check = close(stdin_fd);
            assert(status_check != -1);
        } else if (strcmp(sv->data[i], ">") == 0) { //>
            redirhappened = 1;
            svec* first = make_svec(i);
            for (int j = 0; j < i; j++) {
                svec_push_back(first, sv->data[j]);
            }
            i++;

            int fd = open(sv->data[i], O_CREAT | O_WRONLY, 0644);
            assert(fd != -1);
            stdout_fd = dup(1);
            assert(stdout_fd != -1);

            status_check = close(1);
            assert(status_check != -1);
            status_check = dup(fd);
            assert(status_check != -1);
            status_check = close(fd);
            assert(status_check != -1);

            execute(first, map, bckgd);
            free_svec(first);
            
            status_check = close(1);
            assert(status_check != -1);
            status_check = dup(stdout_fd);
            assert(status_check != -1);
            status_check = close(stdout_fd);
            assert(status_check != -1);
        }
    }
    if (redirhappened == 1) {
        return 0;
    } else {
        return -1;
    }
}

int handleSubshell(svec* sv, hashmap* map, int bckgd) {
    int start = 0;
    int depth = 0;
    int status = -1;
    for (int i = 0; i < sv->size; i++) {
        if (strcmp(sv->data[i], "(") == 0) {
            svec* subsh = make_svec(3); //subshell argv
            svec_push_back(subsh, "./nush");
            svec_push_back(subsh, "sub");
            char* tmp = malloc(strlen(sv->data[i + 1]) * sizeof(char) + 1);
            strcpy(tmp, sv->data[i + 1]);
            tmp[strlen(tmp)] = '\n';
            tmp[strlen(tmp) + 1] = '\0';
            svec_push_back(subsh, tmp); //subshell cmd
            status = execute(subsh, map, bckgd); //run subshell
            free(tmp);
            free(subsh);
        }
    }
    return status;
}

int execute(svec* sv, hashmap* map, int bckgd) {
    if (sv->size == 0) { //no cmd in shell
        return 0;
    }

    if (strcmp(sv->data[0], "exit") == 0) {
        return -2;
    }
    
    int status = handleVars(sv, map);
    if (status != -1) {
        return status;
    }

    status = handleSemicolon(sv, map, bckgd);
    if (status != -1) {
        return status;
    }

    status = handleBckgd(sv, map, bckgd);
    if (status != -1) {
        return status;
    }

    status = handleAndOr(sv, map, bckgd);
    if (status != -1) {
        return status;
    }

    status = handleRedirects(sv, map, bckgd);
    if (status != -1) {
        return status;
    }
    
    status = handleSubshell(sv, map, bckgd);
    if (status != -1) {
        return status;
    }

    if (strcmp(sv->data[0], "cd") == 0) {
        return chdir(sv->data[1]);
    }

    char** args = calloc(sv->size + 1, sizeof(char*));;
    for (int i = 0; i < sv->size; i++) {
        args[i] = strdup(sv->data[i]);
    }
    args[sv->size] = 0; //add null byte to argv

    int cpid;
    if ((cpid = fork())) { //parent
        if (bckgd == 0) {
            int status_check = waitpid(cpid, &status, 0);
            assert(status_check != -1);
        } else {
            status = 0;
        }
    }
    else { //child
        execvp(args[0], args);
        assert(0);
    }
    for (int i = 0; i <= sv->size; i++) {
        free(args[i]);
    }
    free(args);
    return status;
}

int is_operator(char c) {
    return c == '&' || c == '|' || c == '<' || c == '>' || c == ';' || c == '=' || c == '(' || c == ')';
}

void tokenize(svec* sv, char* cmd) {
    int start = 0;
    bool openQuote = false;
    for (int i = 0; i < strlen(cmd); i++) {
        if ((is_operator(cmd[i]) || cmd[i] == '\0' || cmd[i] == '"' || isspace(cmd[i]))) {
            if (!is_operator(cmd[start]) && cmd[start] != '"' && !isspace(cmd[start])) {
                //not a special symbol; ie cmds and args
                char* slice = strndup(cmd + start, i - start + 1);
                slice[i - start] = '\0';
                svec_push_back(sv, slice);
                free(slice);
            }
            start = i + 1;

            if (cmd[i] == '\0') {
                break;
            } else if (cmd[i] == '&' && cmd[i + 1] == '&') {
                svec_push_back(sv, "&&");
                start++;
                i++;
            } else if (cmd[i] == '|' && cmd[i + 1] == '|') {
                svec_push_back(sv, "||");
                start++;
                i++;
            } else if (cmd[i] == '"') {
                i++;
                start = i;
                while (cmd[i] != '"') {
                  i++;
                }
                char* slice = strndup(cmd + start, i - start);
                svec_push_back(sv, slice);
                start = i + 1;
                free(slice);
            } else if (cmd[i] == '(') {
                int depth = 1;
                svec_push_back(sv, "(");
                while (depth != 0) {
                    i++;
                    if (cmd[i] == '"') {
                        openQuote = !openQuote;
                    }
                    if (!openQuote) {
                    if (cmd[i] == '(') {
                        depth++;
                    } else if (cmd[i] == ')') {
                        depth--;
                    }
                    }
                }
                char* slice = strndup(cmd + start, i - start);
                svec_push_back(sv, slice);
                free(slice);
                svec_push_back(sv, ")");
                start = i + 1;
            } else if (isspace(cmd[i])) {
                //do nothing
            } else { //&, |, <, >, =
                char separator[2];
                separator[0] = cmd[i];
                separator[1] = '\0';
                svec_push_back(sv, separator);
            }
        }
    }
}

int
main(int argc, char* argv[])
{
    char cmd[256];
    char tmp[128];
    FILE* file;
    if (argc == 3) { //subshell
        strncpy(cmd, argv[2], 256);
    } else if (argc == 2) { //file
       file = fopen(argv[1], "r");
    } else { //interactive
        file = stdin;
    }
    hashmap* map = make_hashmap();
    bool done = 0;
    int status = 0;
    char* ret = 0;
    while (!done) {
        if (argc == 1) {
            printf("nush$ ");
            fflush(stdout);
        }
        svec* sv = make_svec(2);
        if (argc != 3) {
            ret = fgets(cmd, 256, file);
        }
        do {
            if (argc == 3) { //handle backslash for subshell
                for (int i = 0; i < strlen(cmd); i++) {
                    if (cmd[i] == '\\' && cmd[i + 1] == '\n') {
                        cmd[i] = ' ';
                        cmd[i + 1] = ' ';
                    }
                }
            } else if (!ret) {
                done = true;
            } else { //handle backslash for interactive
                if (cmd[strlen(cmd) - 2] == '\\') {
                    cmd[strlen(cmd) - 2] = ' ';
                    cmd[strlen(cmd) - 1] = ' ';
                    fgets(tmp, 128, file);
                    strcat(cmd, tmp);
                }
            }
        } while (!done && cmd[strlen(cmd) - 1] == '\\');
        tokenize(sv, cmd);
        if (!done) {
            status = execute(sv, map, 0);
        }
        free_svec(sv);
        if (argc == 3) {
            done = true;
        }
        if (status == -2) { //exit
            done = true;
            status = 0;
        }
    }
    if (argc == 2) {
        fclose(file);
    }
    free_hashmap(map);
    return status;
}
