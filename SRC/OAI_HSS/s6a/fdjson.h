#ifndef __FDJSON_H
#define __FDJSON_H

#ifdef __cplusplus
extern "C" {
#endif

int fdJsonAddAvps( const char *json, struct msg *msg, void (*errfunc)(const char *) );

#define FDJSON_SUCCESS             0
#define FDJSON_JSON_PARSING_ERROR  1
#define FDJSON_EXCEPTION           2

#ifdef __cplusplus
};
#endif

#endif /* #define __FDJSON_H */
