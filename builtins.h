#ifndef BUILTINS_H
#define BUILTINS_H

int ashx_cd(char **args);
int ashx_help(char **args);
int ashx_exit(char **args);

extern char *builtin_str[];
extern int (*builtin_func[]) (char **);

int num_builtins(void);

#endif
