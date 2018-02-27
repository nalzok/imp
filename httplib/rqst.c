#include <stdlib.h>
#include <string.h>

#include "httplib.h"


struct Request *parse_request(const char *raw) {
    struct Request *req = NULL;
    req = malloc(sizeof *req);
    if (!req) {
        return NULL;
    }
    memset(req, 0, sizeof *req);

    // Method
    size_t method_len = strcspn(raw, " ");
    if (strncmp(raw, S_GET, sizeof S_GET - 1) == 0) {
        req->method = GET;
    } else if (strncmp(raw, S_HEAD, sizeof S_HEAD - 1) == 0) {
        req->method = HEAD;
    } else {
        req->method = UNSUPPORTED;
    }
    raw += method_len;
    raw += strspn(raw, " "); // move past <SP>

    // Request-URI
    size_t url_len = strcspn(raw, " ");
    if ((req->url = malloc(url_len + 1)) == NULL) {
        return free_request(req);
    }
    memcpy(req->url, raw, url_len);
    req->url[url_len] = '\0';
    raw += url_len;
    raw += strspn(raw, " "); // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = malloc(ver_len + 1);
    if ((req->version = malloc(ver_len + 1)) == NULL) {
        return free_request(req);
    }
    memcpy(req->version, raw, ver_len);
    req->version[ver_len] = '\0';
    raw += ver_len;
    raw += strspn(raw, "\r\n"); // move past <CR><LF>

    struct Header *header = NULL, *last = NULL;
    while (raw[0]!='\r' || raw[1]!='\n') {
        last = header;
        if ((header = malloc(sizeof *header)) == NULL) {
            return free_request(req);
        }

        // name
        size_t name_len = strcspn(raw, ": ");
        header->name = malloc(name_len + 1);
        if ((header->name = malloc(name_len + 1)) == NULL) {
            return free_request(req);
        }
        memcpy(header->name, raw, name_len);
        header->name[name_len] = '\0';
        raw += name_len;
        raw += strspn(raw, ": "); // move past :<SP>

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = malloc(value_len + 1);
        if ((header->value = malloc(value_len + 1)) == NULL) {
            return free_request(req);
        }
        memcpy(header->value, raw, value_len);
        header->value[value_len] = '\0';
        raw += value_len;
        // move past one <CR><LF>, one <CR>, or one <LF>
        if (raw[0] == '\r') {
            raw++;
        }
        if (raw[0] == '\n') {
            raw++;
        }

        // next
        header->next = last;
    }
    req->headers = header;
    raw += strspn(raw, "\r\n"); // move past <CR><LF>

    size_t body_len = strlen(raw);
    req->body = malloc(body_len + 1);
    if ((req->body = malloc(body_len + 1)) == NULL) {
        return free_request(req);
    }
    memcpy(req->body, raw, body_len);
    req->body[body_len] = '\0';


    return req;
}


void *free_headers(struct Header *h) {
    if (h) {
        free(h->name);
        free(h->value);
        free_headers(h->next);
        free(h);
    }
    return NULL;
}


void *free_request(struct Request *req) {
    if (req) {
        free(req->url);
        free(req->version);
        free_headers(req->headers);
        free(req->body);
        free(req);
    }
    return NULL;
}