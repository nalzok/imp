#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>

#define PORT "8080"
#define BACKLOG 10
#define MAX_REQUEST_LEN 8192
#define MAX_RESPONSE_LEN 65536

#define add_to_resp(__STR) do {strcat(response, __STR);} while(0)


ssize_t sendall(int socket, const void *buffer, size_t length, int flags);
int handle_get(int sockfd, char *request, ssize_t len, char *base_dir);
void urldecode(char *dst, const char *src);


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: imp <directory_prefix>\n");
        exit(2);
    }

    char *base_dir;
    if (strcmp(argv[1], ".") == 0) {
        base_dir = getcwd(NULL, 0);
        // infinite loop below, cannot free()
    } else {
        base_dir = argv[1];
    }

    int rv;
    struct addrinfo hints, *res, *res0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    rv = getaddrinfo(NULL, PORT, &hints, &res0);
    if (rv == -1) {
        fprintf(stderr, "imp: getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    int listener = -1;
    for (res = res0; res; res = res->ai_next) {
        listener = socket(res->ai_family, res->ai_socktype,
                          res->ai_protocol);
        if (listener < 0) {
            perror("imp: socket");
            continue;
        }

        int yes = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

        rv = bind(listener, res->ai_addr, res->ai_addrlen);
        if (rv == -1) {
            perror("imp: bind");
            close(listener);
            continue;
        }

        break;
    }

    if (res == NULL) {
        fprintf(stderr, "imp: failed to bind\n");
        exit(1);
    }

    rv = listen(listener, BACKLOG);
    if (rv) {
        perror("imp: listen");
        exit(1);
    }

    char server_ip[INET6_ADDRSTRLEN];
    void *server_addr_ptr;
    if (res->ai_family == AF_INET) {
        struct sockaddr_in *ip_v4
                = (struct sockaddr_in *) res->ai_addr;
        server_addr_ptr = &(ip_v4->sin_addr);
    } else {
        struct sockaddr_in6 *ip_v6
                = (struct sockaddr_in6 *) res->ai_addr;
        server_addr_ptr = &(ip_v6->sin6_addr);
    }
    inet_ntop(res->ai_family, server_addr_ptr, server_ip,
              sizeof server_ip);
    printf("selectserver: listening on %s port %s\n",
           server_ip, PORT);

    freeaddrinfo(res0);

    struct fd_set master, mirror;
    FD_ZERO(&master);
    FD_SET(listener, &master);
    int maxfd = listener;

    for(;;) {
        mirror = master;
        rv = select(maxfd+1, &mirror, NULL, NULL, NULL);
        if (rv == -1) {
            perror("imp: select");
            exit(1);
        } else {
            int i;
            for (i=0; i<=maxfd; i++) {
                if (FD_ISSET(i, &mirror)) {
                    if (i==listener) {
                        struct sockaddr_storage client_addr;
                        socklen_t ca_len = sizeof client_addr;
                        int client_sock;
                        client_sock = accept(listener, (struct sockaddr *)
                                &client_addr, &ca_len);
                        if (client_sock == -1) {
                            perror("imp: accept");
                            continue;
                        } else {
                            FD_SET(client_sock, &master);
                            maxfd = (client_sock > maxfd) ? client_sock : maxfd;

                            char client_ip[INET6_ADDRSTRLEN];
                            unsigned short client_port;
                            void *client_addr_ptr;
                            if (client_addr.ss_family == AF_INET) {
                                struct sockaddr_in *ip_v4
                                        = (struct sockaddr_in *) &client_addr;
                                client_addr_ptr = &(ip_v4->sin_addr);
                                client_port = ntohs(ip_v4->sin_port);
                            } else {
                                struct sockaddr_in6 *ip_v6
                                        = (struct sockaddr_in6 *) &client_addr;
                                client_addr_ptr = &(ip_v6->sin6_addr);
                                client_port = ntohs(ip_v6->sin6_port);
                            }
                            inet_ntop(client_addr.ss_family, client_addr_ptr, client_ip,
                                      sizeof client_ip);
                            printf("selectserver: got connection from %s port %u\n",
                                   client_ip, client_port);
                        }
                    } else {
                        char *request = malloc(MAX_REQUEST_LEN);
                        ssize_t request_len = recv(i, request, MAX_REQUEST_LEN, 0);
                        if (request_len <= 0) {
                            if (request_len == 0) {
                                printf("imp: connection dropped by client %d\n", i);
                            } else {
                                perror("imp: recv");
                            }
                            close(i);
                            FD_CLR(i, &master);
                            continue;
                        } else {
                            rv = handle_get(i, request, request_len, base_dir);
                            if (rv == -1) {
                                perror("imp: handle_get");
                                continue;
                            }
                        }
                        free(request);
                    }
                }
            }
        }
    }
}


ssize_t sendall(int socket, const void *buffer, size_t length, int flags) {
    ssize_t partial = 0;
    while (partial < length) {
        ssize_t chunk = send(socket, buffer+partial, length-partial, flags);
        if (chunk == -1) {
            return -1;
        }
        partial += chunk;
    }
    return 0;
}


int handle_get(int sockfd, char *request, ssize_t len, char *base_dir) {
    char *response = malloc(MAX_RESPONSE_LEN);
    if (!response) goto handle_error;

    response[0] = '\0';

    size_t base_len = strlen(base_dir);
    if (base_dir[base_len-1] == '/') {
        base_dir[base_len-1] = '\0';
        base_len--;
    }
    size_t rel_len = strcspn(request+4, " ");
    if (request[4+rel_len-1] == '/') {
        rel_len--;
    }

    char *rel_dir_url = malloc(rel_len+1);
    memcpy(rel_dir_url, request+4, rel_len);
    rel_dir_url[rel_len] = '\0';
    char *rel_dir = malloc(rel_len+1);
    urldecode(rel_dir, rel_dir_url);

    char *full_dir = malloc(base_len+rel_len+1);
    memcpy(full_dir, base_dir, base_len);
    memcpy(full_dir+base_len, rel_dir, rel_len);
    full_dir[base_len+rel_len] = '\0';

    add_to_resp("<html>\r\n");

    add_to_resp("<head>\r\n");
    add_to_resp("<meta charset=\"utf-8\">\r\n");
    add_to_resp("<meta http-equiv=\"x-ua-compatible\" content=\"ie=edge\">\r\n");
    add_to_resp("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">\r\n");
    add_to_resp("<title>[imp] ");
    add_to_resp(rel_dir);
    add_to_resp("/</title>\r\n");
    add_to_resp("<link href=\"https://cdn.bootcss.com/mini.css/2.3.7/mini-default.css\" rel=\"stylesheet\">\r\n");
    add_to_resp("<style>ul {list-style-type: none;}</style>\r\n");
    add_to_resp("</head>\r\n");

    add_to_resp("<body>\r\n");

    add_to_resp("<div class=\"container\">\r\n");
    add_to_resp("<h1 style=\"text-align: center\">");
    add_to_resp(full_dir);
    add_to_resp("</h1>\r\n");

    add_to_resp("<div class=\"col-sm-4 col-sm-offset-4\">\r\n");

    puts(rel_dir);
    puts(full_dir);
    DIR *dir = opendir(full_dir);
    if (dir == NULL) goto handle_error;

    add_to_resp("<ul>");
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        add_to_resp("<li><a href=\"");
        add_to_resp(rel_dir);
        if (strcmp(rel_dir, "/") != 0) {
            add_to_resp("/");
        }
        add_to_resp(ent->d_name);
        add_to_resp("\">");
        add_to_resp(ent->d_name);
        add_to_resp("</a></li>\r\n");
    }
    closedir(dir);

    free(rel_dir_url);
    free(rel_dir);
    free(full_dir);
    add_to_resp("</ul>");

    add_to_resp("</div>\r\n");
    add_to_resp("</div>\r\n");
    add_to_resp("</body>\r\n");
    add_to_resp("</html>\r\n");

    ssize_t send_status = 0;

    const char http_200[] = "HTTP/1.1 200 OK\r\n";
    send_status = sendall(sockfd, http_200,
                           strlen(http_200), 0);
    if (send_status) goto handle_error;

    const char content_length_fmt[] = "Content-Length: %zu\r\n";
    char content_length[strlen(content_length_fmt) + 8];
    size_t msg_length = strlen(response);
    sprintf(content_length, content_length_fmt, msg_length);
    send_status = sendall(sockfd, content_length,
                          strlen(content_length), 0);
    if (send_status) goto handle_error;

    const char content_type[] = "Content-Type: text/html\r\n";
    send_status = sendall(sockfd, content_type,
                          strlen(content_type), 0);
    if (send_status) goto handle_error;

    const char end_of_header[] = "\r\n";
    send_status = sendall(sockfd, end_of_header,
                          strlen(end_of_header), 0);
    if (send_status) goto handle_error;

    send_status = sendall(sockfd, response,
                          strlen(response), 0);
    if (send_status) goto handle_error;

    return 0;

    handle_error:
    free(response);
    return -1;
}


void urldecode(char *dst, const char *src)
{
    /*
     * Courtesy of stackoverflow.com/a/14530993
     */
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            a = (char)toupper(a);
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            b = (char)toupper(b);
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = (char)16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}
