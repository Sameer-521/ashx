#ifndef LINE_H
#define LINE_H

#define ASHX_LINE_BUFSIZE 1024

char *ashx_readline(void);
char **ashx_split_line(char *line);

#endif
