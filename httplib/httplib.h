#ifndef IMP_HTTPLIB_H
#define IMP_HTTPLIB_H


typedef enum Method {UNSUPPORTED, GET, HEAD} Method;

#define S_GET "GET"
#define S_HEAD "HEAD"

typedef struct Header {
    char *name;
    char *value;
    struct Header *next;
} Header;

typedef struct Request {
    enum Method method;
    char *url;
    char *version;
    struct Header *headers;
    char *body;
} Request;


struct Request *parse_request(const char *raw);
void *free_headers(struct Header *h);
void *free_request(struct Request *req);


#endif //IMP_HTTPLIB_H