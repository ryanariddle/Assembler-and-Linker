#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include "src/utils.h"
#include "src/tables.h"
#include "src/translate_utils.h"
#include "src/translate.h"

const char* TMP_FILE = "test_output.txt";
const int BUF_SIZE = 1024;

/****************************************
 *  Helper functions 
 ****************************************/

int do_nothing() {
    return 0;
}

int init_log_file() {
    set_log_file(TMP_FILE);
    return 0;
}

int check_lines_equal(char **arr, int num) {
    char buf[BUF_SIZE];

    FILE *f = fopen(TMP_FILE, "r");
    if (!f) {
        CU_FAIL("Could not open temporary file");
        return 0;
    }
    for (int i = 0; i < num; i++) {
        if (!fgets(buf, BUF_SIZE, f)) {
            CU_FAIL("Reached end of file");
            return 0;
        }
        CU_ASSERT(!strncmp(buf, arr[i], strlen(arr[i])));
    }
    fclose(f);
    return 0;
}

/****************************************
 *  Test cases for translate_utils.c 
 ****************************************/

void test_translate_reg() {
    CU_ASSERT_EQUAL(translate_reg("$0"), 0);
    CU_ASSERT_EQUAL(translate_reg("$at"), 1);
    CU_ASSERT_EQUAL(translate_reg("$v0"), 2);
    CU_ASSERT_EQUAL(translate_reg("$a0"), 4);
    CU_ASSERT_EQUAL(translate_reg("$a1"), 5);
    CU_ASSERT_EQUAL(translate_reg("$a2"), 6);
    CU_ASSERT_EQUAL(translate_reg("$a3"), 7);
    CU_ASSERT_EQUAL(translate_reg("$t0"), 8);
    CU_ASSERT_EQUAL(translate_reg("$t1"), 9);
    CU_ASSERT_EQUAL(translate_reg("$t2"), 10);
    CU_ASSERT_EQUAL(translate_reg("$t3"), 11);
    CU_ASSERT_EQUAL(translate_reg("$s0"), 16);
    CU_ASSERT_EQUAL(translate_reg("$s1"), 17);
    CU_ASSERT_EQUAL(translate_reg("$3"), -1);
    CU_ASSERT_EQUAL(translate_reg("asdf"), -1);
    CU_ASSERT_EQUAL(translate_reg("hey there"), -1);
}

void test_translate_num() {
    long int output;

    CU_ASSERT_EQUAL(translate_num(&output, "35", -1000, 1000), 0);
    CU_ASSERT_EQUAL(output, 35);
    CU_ASSERT_EQUAL(translate_num(&output, "145634236", 0, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 145634236);
    CU_ASSERT_EQUAL(translate_num(&output, "0xC0FFEE", -9000000000, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 12648430);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 72), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 71), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 72, 150), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 73, 150), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "35x", -100, 100), -1);
}

/****************************************
 *  Test cases for tables.c 
 ****************************************/

void test_table_1() {
    int retval;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    retval = add_to_table(tbl, "abc", 8);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "efg", 12);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 24); 
    CU_ASSERT_EQUAL(retval, -1); 
    retval = add_to_table(tbl, "bob", 14); 
    CU_ASSERT_EQUAL(retval, -1); 

    retval = get_addr_for_symbol(tbl, "abc");
    CU_ASSERT_EQUAL(retval, 8); 
    retval = get_addr_for_symbol(tbl, "q45");
    CU_ASSERT_EQUAL(retval, 16); 
    retval = get_addr_for_symbol(tbl, "ef");
    CU_ASSERT_EQUAL(retval, -1);
    
    free_table(tbl);

    char* arr[] = { "Error: name 'q45' already exists in table.",
                    "Error: address is not a multiple of 4." };
    check_lines_equal(arr, 2);

    SymbolTable* tbl2 = create_table(SYMTBL_NON_UNIQUE);
    CU_ASSERT_PTR_NOT_NULL(tbl2);

    retval = add_to_table(tbl2, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl2, "q45", 24); 
    CU_ASSERT_EQUAL(retval, 0);

    free_table(tbl2);
}

void test_table_2() {
    int retval, max = 100;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    char buf[10];
    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = add_to_table(tbl, buf, 4 * i);
        CU_ASSERT_EQUAL(retval, 0);
    }

    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = get_addr_for_symbol(tbl, buf);
        CU_ASSERT_EQUAL(retval, 4 * i);
    }

    free_table(tbl);
}


/****************************************
 *  Test cases for translate.c 
 ****************************************/


void test_translate() {
    int retval;
    FILE* file_out;
    char name[5];
    char line[256];

    /* Test addiu number 1*/
    file_out = fopen("test_addiu1.txt", "w");
    char** addiu1 = (char *[]){"$a0", "$0", "0xABC"};
    size_t num_args = 3;
    uint32_t addr;
    strcpy(name, "addiu");
    retval = translate_inst(file_out, name, addiu1, num_args, NULL, NULL, NULL);
    CU_ASSERT_EQUAL(retval, 0)
    fclose(file_out);
    file_out = fopen("test_addiu1.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    char blah[50];
    // printf("%s\n", blah);
    CU_ASSERT_EQUAL(strcmp(line, "24040abc"), 0);

    /* Test addiu number 2*/
    file_out = fopen("test_addiu2.txt", "w");
    char** addiu2 = (char *[]){"$a1", "$0", "10"};
    retval = translate_inst(file_out, name, addiu2, num_args, NULL, NULL, NULL);
    CU_ASSERT_EQUAL(retval, 0);
    fclose(file_out); 
    file_out = fopen("test_addiu2.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(strcmp(line, "2405000a"), 0);

    /* Test lui number 1*/
    file_out = fopen("test_lui1.txt", "w");
    strcpy(name, "lui");
    char** lui = (char *[]){"$at", "10"};
    num_args = 2;
    retval = translate_inst(file_out, name, lui, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_lui1.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);

    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "3c01000a"), 0);

    /* Test lui number 2 */
    file_out = fopen("test_lui2.txt", "w");
    char** lui2 = (char *[]){"$t3", "532"};
    retval = translate_inst(file_out, name, lui2, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_lui2.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "3c0b0214"), 0);

    /* Test ori number 1 */
    file_out = fopen("test_ori1.txt", "w");
    strcpy(name, "ori");
    char** ori = (char *[]){"$v0", "$at", "48350"};
    num_args = 3;
    retval = translate_inst(file_out, name, ori, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_ori1.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "3422bcde"), 0);

    /* Test ori number 2 */
    file_out = fopen("test_ori2.txt", "w");
    char** ori2 = (char *[]){"$t3", "$t2", "0x123"};
    retval = translate_inst(file_out, name, ori2, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_ori2.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "354b0123"), 0);


    /* Test addu */
    file_out = fopen("test_addu.txt", "w");
    strcpy(name, "addu");
    char** addu = (char *[]){"$v0", "$a0", "$a1"};
    num_args = 3;
    retval = translate_inst(file_out, name, addu, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_addu.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "00851021"), 0);

    /* Test or */
    file_out = fopen("test_or.txt", "w");
    strcpy(name, "or");
    char** or = (char *[]){"$a0", "$a1", "$a3"};
    num_args = 3;
    retval = translate_inst(file_out, name, or, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_or.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "00a72025"), 0);

    /* Test slt number 1 */
    file_out = fopen("test_slt1.txt", "w");
    strcpy(name, "slt");
    char** slt = (char *[]){"$a2", "$t1", "$t0"};
    num_args = 3;
    retval = translate_inst(file_out, name, slt, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_slt1.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "0128302a"), 0);

    /* Test slt number 2 */
    file_out = fopen("test_slt2.txt", "w");
    char** slt2 = (char *[]){"$at", "$t3", "$t2"};
    retval = translate_inst(file_out, name, slt2, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_slt2.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "016a082a"), 0);

    /* Test sltu */
    file_out = fopen("test_sltu.txt", "w");
    strcpy(name, "sltu");
    char** sltu = (char *[]){"$a2", "$t1", "$t0"};
    num_args = 3;
    retval = translate_inst(file_out, name, sltu, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_sltu.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "0128302b"), 0);

    /* Test sll */
    file_out = fopen("test_sll.txt", "w");
    strcpy(name, "sll");
    char** sll = (char *[]){"$t3", "$t2", "31"};
    num_args = 3;
    retval = translate_inst(file_out, name, sll, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_sll.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "000a5fc0"), 0);

    /* Test jr */
    file_out = fopen("test_jr.txt", "w");
    strcpy(name, "jr");
    char** jr = (char *[]){"$t1"};
    num_args = 1;
    retval = translate_inst(file_out, name, jr, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_jr.txt", "r");
    fgets(line, sizeof(line), file_out);
    strtok(line, "\n");
    fclose(file_out);
    CU_ASSERT_EQUAL(retval, 0);
    printf("Line: %s, should be: 01200008 \n", line);
    CU_ASSERT_EQUAL(strcmp(line, "01200008"), 0);

    /* Test lb */
    file_out = fopen("test_lb.txt", "w");
    strcpy(name, "lb");
    char** lb = (char *[]){"$t2", "0", "$t1"};
    num_args = 3;
    retval = translate_inst(file_out, name, lb, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_lb.txt", "r");
    fgets(line, sizeof(line), file_out);
    fclose(file_out);
    strtok(line, "\n");
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "812a0000"), 0);
    /*

    /* Test lbu */
    file_out = fopen("test_lbu.txt", "w");
    strcpy(name, "lbu");
    char** lbu = (char *[]){"$t3", "-3", "$s2"};
    num_args = 3;
    retval = translate_inst(file_out, name, lbu, num_args, NULL, NULL, NULL);
    fclose(file_out);
    file_out = fopen("test_lbu.txt", "r");
    fgets(line, sizeof(line), file_out);
    fclose(file_out);
    strtok(line, "\n");
    CU_ASSERT_EQUAL(retval, 0);
    CU_ASSERT_EQUAL(strcmp(line, "924bfffd"), 0);

    /*
    This website (http://www.kurtm.net/mipsasm/index.cgi) was really helpful in creating the above tests.
    Still need to test:
        - jr^
        - lb^
        - lbu
        - lw
        - sb
        - sw
        - beq
        - bne
        - j
        - jal
    */
}

/****************************************
 *  Add your test cases here
 ****************************************/

int main(int argc, char** argv) {
    CU_pSuite pSuite1 = NULL, pSuite2 = NULL, pSuite3 = NULL;

    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    /* Suite 1 */
    pSuite1 = CU_add_suite("Testing translate_utils.c", NULL, NULL);
    if (!pSuite1) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_reg", test_translate_reg)) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_num", test_translate_num)) {
        goto exit;
    }

    /* Suite 2 */
    pSuite2 = CU_add_suite("Testing tables.c", init_log_file, NULL);
    if (!pSuite2) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_1", test_table_1)) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_2", test_table_2)) {
        goto exit;
    }

    /* Suite 3 */
    pSuite3 = CU_add_suite("Testing translate.c", NULL, NULL);
    if (!pSuite3) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_translate", test_translate)) {
        goto exit;
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

exit:
    CU_cleanup_registry();
    return CU_get_error();;
}
