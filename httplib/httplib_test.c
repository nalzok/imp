#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "httplib.h"


static void null_test_success(void **state) {
    (void) state; /* unused */
}


static void test_free_headers(void **state) {
    (void) state; /* unused */

    struct Header *h = malloc(sizeof *h);
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
            cmocka_unit_test(test_free_headers),
            cmocka_unit_test(test_free_request),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
