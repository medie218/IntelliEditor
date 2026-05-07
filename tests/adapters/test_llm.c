/**
 * @file test_llm.c
 * @brief Tests unitaires du module LLM — cmocka
 * @author DEV-C
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "llm.h"
#include "threads.h"
#include <string.h>
#include <stdio.h>

/* Test 1 : llm_create() sans modèle retourne NULL */
static void test_llm_create_no_model(void **state) {
    (void)state;
    LlmEngine *engine = llm_create("fichier_inexistant.gguf", 4, 2048);
    assert_null(engine);
}

/* Test 2 : llm_is_ready() sur NULL retourne false */
static void test_llm_is_ready_null(void **state) {
    (void)state;
    assert_false(llm_is_ready(NULL));
}

/* Test 3 : llm_queue_size() sur NULL retourne 0 */
static void test_llm_queue_size_null(void **state) {
    (void)state;
    assert_int_equal(llm_queue_size(NULL), 0);
}

/* Test 4 : llm_submit_request() sur NULL retourne 0 */
static void test_llm_submit_null_engine(void **state) {
    (void)state;
    LlmRequestId id = llm_submit_request(
        NULL, LLM_TASK_GRAMMAR_CHECK, "test", NULL, NULL);
    assert_int_equal(id, 0);
}

/* Test 5 : llm_cancel_request() sur NULL ne crash pas */
static void test_llm_cancel_null_safe(void **state) {
    (void)state;
    bool result = llm_cancel_request(NULL, 1);
    assert_false(result);
}

/* Test 6 : llm_destroy(NULL) ne crash pas */
static void test_llm_destroy_null_safe(void **state) {
    (void)state;
    llm_destroy(NULL);
}

int main(void) {
    printf("=== Tests LLM — DEV-C ===\n\n");
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_llm_create_no_model),
        cmocka_unit_test(test_llm_is_ready_null),
        cmocka_unit_test(test_llm_queue_size_null),
        cmocka_unit_test(test_llm_submit_null_engine),
        cmocka_unit_test(test_llm_cancel_null_safe),
        cmocka_unit_test(test_llm_destroy_null_safe),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
