#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rqst.h"


static void null_test_success(void **state) {
    (void) state; /* unused */
}

static void test_parse_request(void **state) {
    (void) state; /* unused */

    // modified version of a real request sent by Safari
    char *raw_request =
            "GET / HTTP/1.1\r\n"
            "Host: localhost:8080\r\n"
            "Connection: keep-alive\r\n"
            "Upgrade-Insecure-Requests: 1\r\n"
            "Accept: text/html,application/xhtml+xml,"
                    "application/xml;q=0.9,*/*;q=0.8\r\n"
            "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_3)"
                    " AppleWebKit/604.5.6 (KHTML, like Gecko)"
                    " Version/11.0.3 Safari/604.5.6\r\n"
            "Accept-Language: en-us\r\n"
            "DNT: 1\r\n"
            "Accept-Encoding: gzip, deflate\r\n"
            "\r\n"
            "Usually GET requests don\'t have a body\r\n"
            "But I don\'t care in this case :)";
    struct Request *req = parse_request(raw_request);

    assert_non_null(req);
    assert_int_equal(req->method, GET);
    assert_string_equal(req->url, "/");
    assert_string_equal(req->http_ver, "HTTP/1.1");

    assert_string_equal(req->headers->ct.ho, "localhost:8080");
    assert_string_equal(req->headers->cn.aa,
                        "text/html,application/xhtml+xml,"
                                "application/xml;q=0.9,*/*;q=0.8");
    assert_null(req->headers->cn.ac);
    assert_string_equal(req->headers->cn.ae, "gzip, deflate");
    assert_string_equal(req->headers->cn.al, "en-us");
    assert_string_equal(req->headers->rc.ua,
                        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_3)"
                                " AppleWebKit/604.5.6 (KHTML, like Gecko)"
                                " Version/11.0.3 Safari/604.5.6");

    assert_non_null(req->headers->ot);
    assert_string_equal(req->headers->ot->name, "DNT"); // Last In, First Out
    assert_string_equal(req->headers->ot->value, "1");

    assert_non_null(req->headers->ot->next);
    assert_string_equal(req->headers->ot->next->name,
                        "Upgrade-Insecure-Requests");
    assert_string_equal(req->headers->ot->next->value, "1");

    assert_non_null(req->headers->ot->next->next);
    assert_string_equal(req->headers->ot->next->next->name, "Connection");
    assert_string_equal(req->headers->ot->next->next->value, "keep-alive");
    assert_null(req->headers->ot->next->next->next);

    assert_string_equal(req->body,
                        "Usually GET requests don\'t have a body\r\n"
                                "But I don\'t care in this case :)");
    free_request(req);
}


static void test_free_headers(void **state) {
    (void) state; /* unused */

    struct Headers *h = malloc(sizeof *h);
    memset(h, 0, sizeof *h);
    assert_null(free_headers(h));

    assert_null(free_headers(NULL));
}


static void test_free_request(void **state) {
    (void) state; /* unused */

    struct Request *req = malloc(sizeof *req);
    memset(req, 0, sizeof *req);
    assert_null(free_request(req));

    assert_null(free_headers(NULL));
}


int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(null_test_success),
            cmocka_unit_test(test_parse_request),
            cmocka_unit_test(test_free_headers),
            cmocka_unit_test(test_free_request),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
