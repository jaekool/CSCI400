
#ifndef GETOPT_H
#define GETOPT_H

#ifdef  __cplusplus
extern "C" {
#endif

#define _next_char(string)  (char)(*(string+1))

extern char * optarg; 
extern int    optind; 

extern int getopt(int, char**, char*);

#ifdef  __cplusplus
}
#endif

#endif /* GETOPT_H */

