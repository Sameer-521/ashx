#include "builtins.h"
#include "exec.h"
#include "parser.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1
#define BUILTIN_PID -99

static void set_error(ExecError *err, ExecErrorType type, char *cmd_name, int _errno) {
    err->had_error = true;
    err->type = type;
    err->cmd_name = cmd_name;
    err->_errno = _errno;
}

static ExecError open_dup_close_file(const char *name, int flags, int fd, int perm) {
    ExecError err = {0};
    int f, _;

    if (perm != 0) {
        f = open(name, flags, perm);
    } else
        f = open(name, flags);

    if (f == -1) {
        set_error(&err, EXEC_OPEN_FAILED, NULL, errno);
        return err;
    }

    _ = dup2(f, fd);
    if (_ == -1) {
        set_error(&err, EXEC_DUP2_FAILED, NULL, errno);
        return err;
    }

    close(f);

    return err;
}

static ExecError apply_redirects(Cmd *cmd) {
    ExecError err = {0};
    Redirect redirect;
    int flags;

    for (size_t i = 0; i < cmd->num_redirects; i++) {
        redirect = cmd->redirects[i];
        switch (redirect.type) {
        case REDIRECT_IN:
            err = open_dup_close_file(redirect.filename, O_RDONLY, STDIN_FILENO, 0);
            if (err.had_error)
                return err;
            break;

        case REDIRECT_OUT:
        case REDIRECT_OUT_APPEND:
            flags = O_CREAT | O_WRONLY;
            flags |= redirect.type == REDIRECT_OUT ? O_TRUNC : O_APPEND;
            err = open_dup_close_file(redirect.filename, flags, STDOUT_FILENO, 0644);
            if (err.had_error)
                return err;
            break;

        case REDIRECT_ERR_OUT:
        case REDIRECT_ERR_OUT_APPEND:
            flags = O_CREAT | O_WRONLY;
            flags |= redirect.type == REDIRECT_ERR_OUT ? O_TRUNC : O_APPEND;
            err = open_dup_close_file(redirect.filename, flags, STDERR_FILENO, 0644);
            if (err.had_error)
                return err;
            break;

        case REDIRECT_ERR_TO_1:
            dup2(STDOUT_FILENO, STDERR_FILENO);
            break;

        default:
            fprintf(stderr, "ashx: internal error: unsupported REDIRECTION type\n");
            // TODO: handle error later
            exit(EXIT_FAILURE);
        }
    }

    return err;
}

static void run_cmd(Cmd *cmd) {
    // expansion here!

    if (execvp(cmd->argv[0], cmd->argv) == -1) {
        perror("ashx");

        if (errno == ENOENT)
            exit(127);
        else if (errno == EACCES)
            exit(126);
        else
            exit(EXIT_FAILURE);
    }
}

static void close_fds(size_t n, int fds[n][2]) {
    for (size_t i = 0; i < n; i++) {
        close(fds[i][READ_END]);
        close(fds[i][WRITE_END]);
    }
}
typedef struct {
    bool had_run;
    ExecError err;
} RunBuiltinOut;

static RunBuiltinOut run_builtin(Cmd *cmd, pid_t *pids, size_t current) {
    RunBuiltinOut out = {0};

    int old_out = dup(STDOUT_FILENO);
    int old_in = dup(STDIN_FILENO);
    int old_err = dup(STDERR_FILENO);

    out.err = apply_redirects(cmd);
    if (out.err.had_error) {
        out.had_run = false;
        return out;
    }

    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(cmd->argv[0], builtin_str[i]) == 0) {
            (*builtin_func[i])(cmd->argv);
            pids[current] = BUILTIN_PID; // indicator that this is builtin, for now
            out.had_run = true;
            return out;
        }
    }

    // restore fds
    dup2(old_in, STDIN_FILENO);
    dup2(old_out, STDOUT_FILENO);
    dup2(old_err, STDERR_FILENO);

    close(old_in);
    close(old_out);
    close(old_err);

    out.had_run = false;
    return out;
}

static ExecError run_pipeline(Pipeline *pipeline) {
    ExecError err = {0};

    size_t ncmds = pipeline->ncmds;
    if (ncmds == 0) {
        err.had_error = true;
        err.type = EXEC_INTERNAL_ERR;
        return err;
    }

    int fds[ncmds][2]; // ncmds and not ncmds - 1 cause it can evaluate to
                       // zero and Asan will scream
    size_t i;
    for (i = 0; i < ncmds - 1; i++) { // TODO: handle errors
        if (pipe(fds[i]) == -1) {
            close_fds(i, fds);
            set_error(&err, EXEC_PIPE_FAILED, pipeline->cmds[i].argv[0], errno);
            return err;
        }
    }

    pid_t pids[ncmds];
    for (size_t i = 0; i < ncmds; i++) {
        RunBuiltinOut rb_out;
        // builtin?
        rb_out = run_builtin(&pipeline->cmds[i], pids, i);
        if (rb_out.err.had_error) {
            err = rb_out.err;
            break;
        }

        if (rb_out.had_run)
            continue;

        if ((pids[i] = fork()) == 0) {
            // child
            if (i > 0)
                dup2(fds[i - 1][READ_END], STDIN_FILENO); // read from

            if (i < ncmds - 1)
                dup2(fds[i][WRITE_END], STDOUT_FILENO); // write to

            close_fds(ncmds - 1, fds);

            // TODO: report potential errors to parent
            ExecError err = apply_redirects(&(pipeline->cmds[i]));
            if (err.had_error) {
                exit(EXIT_FAILURE);
            }

            // should not return
            run_cmd(&(pipeline->cmds[i]));
        } else if (pids[i] < 0) {
            // error forking
            close_fds(ncmds - 1, fds);
            for (size_t j = 0; j < i; j++)
                if (pids[j] != BUILTIN_PID)
                    kill(pids[j], SIGTERM);

            set_error(&err, EXEC_FORK_FAILED, pipeline->cmds[i].argv[0], errno);
            return err;
        }
    }

    close_fds(ncmds - 1, fds);

    if (err.had_error) {
        return err;
    }

    int status = 0;
    int exit_status[ncmds];
    pid_t wpid;

    for (size_t i = 0; i < ncmds; i++) {
        if (pids[i] == BUILTIN_PID) {
            continue;
        }

        wpid = waitpid(pids[i], &status, WUNTRACED);

        if (WIFEXITED(status)) {
            exit_status[i] = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status))
            exit_status[i] = 128 + WTERMSIG(status);
    }

    return err;
}

ExecError ashx_run_cmdlist(CmdList *list) {
    ExecError err = {0};

    for (size_t i = 0; i < list->npls; i++) {
        err = run_pipeline(&(list->pipelines[i]));
        if (err.had_error) {
            err.current_pl_index = i;
            break;
        }
    }

    return err;
}

static const char *exec_error_type_to_string(ExecErrorType type) {
    static const char *const error_strings[] = {
        [EXEC_DUP2_FAILED] = "dup2() failed",
        [EXEC_OPEN_FAILED] = "open() failed",
        [EXEC_CLOSE_FAILED] = "close() failed",
        [EXEC_FORK_FAILED] = "fork() failed",
        [EXEC_PIPE_FAILED] = "pipe() failed",
        [EXEC_INVALID_REDIRECT_TYPE] = "Invalid redirect type",
        [EXEC_INTERNAL_ERR] = "Internal error",
    };
    size_t arrlen = (sizeof(error_strings) / sizeof(error_strings[0]));

    if (type < 0 || type >= arrlen || !error_strings[type]) {
        return "Unknown error";
    }

    return error_strings[type];
}

void print_exec_error(const ExecError *err) {
    if (!err || !err->had_error) {
        printf("ExecError: No error recorded.\n");
        return;
    }

    printf("=== Exec Error ===\n");
    printf("  Type:     %s\n", exec_error_type_to_string(err->type));
    printf("  Cmd:      %s\n", err->cmd_name ? err->cmd_name : "(null)");
    printf("  Status:   %zu\n", err->status);
    printf("  Pipeline: %zu\n", err->current_pl_index);
    printf("  Errno:    %d", err->_errno);
    if (err->_errno != 0) {
        printf(" (%s)", strerror(err->_errno));
    }
    printf("\n");
    printf("==================\n");
}
