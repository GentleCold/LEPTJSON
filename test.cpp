#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "leptjson.h"
#include <cstdio>
#include <cstring>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)\
    do {\
        test_count++;\
        if (equality) {\
            test_pass++;\
        } else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%lf")
#define EXPECT_EQ_STRING(expect, actual, len) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == (len) && memcmp(expect, actual, len) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

#define TEST_ERROR(error, json)\
    do {\
        lept::value v;\
        v.type = lept::FALSE;\
        EXPECT_EQ_INT(error, lept::parse(&v, json));\
        EXPECT_EQ_INT(lept::NUL, lept::get_type(&v));\
    } while(0)

#define TEST_NUMBER(expect, json)\
    do {\
        lept::value v;\
        EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, json));\
        EXPECT_EQ_INT(lept::NUMBER, lept::get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept::get_number(&v));\
    } while(0)

#define TEST_STRING(expect, json)\
    do {\
        lept::value v;\
        EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, json));\
        EXPECT_EQ_INT(lept::STRING, lept::get_type(&v));\
        EXPECT_EQ_STRING(expect, lept::get_string(&v), lept::get_string_length(&v));\
        lept::fre(&v);\
    } while(0)

static void test_parse_null() {
    lept::value v;
    v.type = lept::TRUE;
    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, "null"));
    EXPECT_EQ_INT(lept::NUL, lept::get_type(&v));
}

static void test_parse_true() {
    lept::value v;
    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, "true"));
    EXPECT_EQ_INT(lept::TRUE, lept::get_type(&v));
}

static void test_parse_false() {
    lept::value v;
    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, "false"));
    EXPECT_EQ_INT(lept::FALSE, lept::get_type(&v));
}

static void test_parse_expect_value() {
    TEST_ERROR(lept::PARSE_EXPECT_VALUE, "");
    TEST_ERROR(lept::PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(lept::PARSE_INVALID_VALUE, " ?");

    // invalid number
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(lept::PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(lept::PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(lept::PARSE_ROOT_NOT_SINGULAR, "null x");

    // invalid number
    TEST_ERROR(lept::PARSE_ROOT_NOT_SINGULAR, "0122"); /* after zero should be '.' or nothing */
    TEST_ERROR(lept::PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(lept::PARSE_ROOT_NOT_SINGULAR, "1x");
}

static void test_parse_number_too_big() {
    TEST_ERROR(lept::PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(lept::PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_string() {
    TEST_STRING("\\", "\"\\\\\"");
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_array() {
    lept::value v;

    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, "[ ]"));
    EXPECT_EQ_INT(lept::ARRAY, lept::get_type(&v));
    EXPECT_EQ_INT(0, lept::get_array_size(&v));
    lept::fre(&v);

    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(lept::ARRAY, lept::get_type(&v));
    EXPECT_EQ_INT(5, lept::get_array_size(&v));
    EXPECT_EQ_INT(lept::NUL,   lept::get_type(lept::get_array_element(&v, 0)));
    EXPECT_EQ_INT(lept::FALSE,  lept::get_type(lept::get_array_element(&v, 1)));
    EXPECT_EQ_INT(lept::TRUE,   lept::get_type(lept::get_array_element(&v, 2)));
    EXPECT_EQ_INT(lept::NUMBER, lept::get_type(lept::get_array_element(&v, 3)));
    EXPECT_EQ_INT(lept::STRING, lept::get_type(lept::get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, lept::get_number(lept::get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", lept::get_string(lept::get_array_element(&v, 4)), lept::get_string_length(lept::get_array_element(&v, 4)));
    lept::fre(&v);

    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(lept::ARRAY, lept::get_type(&v));
    EXPECT_EQ_INT(4, lept::get_array_size(&v));
    for (size_t i = 0; i < 4; i++) {
        lept::value* a = lept::get_array_element(&v, i);
        EXPECT_EQ_INT(lept::ARRAY, lept::get_type(a));
        EXPECT_EQ_INT(i, lept::get_array_size(a));
        for (size_t j = 0; j < i; j++) {
            lept::value* e = lept::get_array_element(a, j);
            EXPECT_EQ_INT(lept::NUMBER, lept::get_type(e));
            EXPECT_EQ_DOUBLE((double)j, lept::get_number(e));
        }
    }
    lept::fre(&v);
}

static void test_access_string() {
    lept::value v;
    lept::set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept::get_string(&v), lept::get_string_length(&v));
    lept::set_string(&v, "HELLO", 5);
    EXPECT_EQ_STRING("HELLO", lept::get_string(&v), lept::get_string_length(&v));
    lept::fre(&v);
}

static void test_parse_object() {
    lept::value v;
    size_t i;

    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v, " { } "));
    EXPECT_EQ_INT(lept::OBJECT, lept::get_type(&v));
    EXPECT_EQ_INT(0, lept::get_object_size(&v));
    lept::fre(&v);

    EXPECT_EQ_INT(lept::PARSE_OK, lept::parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(lept::OBJECT, lept::get_type(&v));
    EXPECT_EQ_INT(7, lept::get_object_size(&v));
    EXPECT_EQ_STRING("n", lept::get_object_key(&v, 0), lept::get_object_key_length(&v, 0));
    EXPECT_EQ_INT(lept::NUL,   lept::get_type(lept::get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", lept::get_object_key(&v, 1), lept::get_object_key_length(&v, 1));
    EXPECT_EQ_INT(lept::FALSE,  lept::get_type(lept::get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", lept::get_object_key(&v, 2), lept::get_object_key_length(&v, 2));
    EXPECT_EQ_INT(lept::TRUE,   lept::get_type(lept::get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", lept::get_object_key(&v, 3), lept::get_object_key_length(&v, 3));
    EXPECT_EQ_INT(lept::NUMBER, lept::get_type(lept::get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, lept::get_number(lept::get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", lept::get_object_key(&v, 4), lept::get_object_key_length(&v, 4));
    EXPECT_EQ_INT(lept::STRING, lept::get_type(lept::get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", lept::get_string(lept::get_object_value(&v, 4)), lept::get_string_length(lept::get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", lept::get_object_key(&v, 5), lept::get_object_key_length(&v, 5));
    EXPECT_EQ_INT(lept::ARRAY, lept::get_type(lept::get_object_value(&v, 5)));
    EXPECT_EQ_INT(3, lept::get_array_size(lept::get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        lept::value* e = lept::get_array_element(lept::get_object_value(&v, 5), i);
        EXPECT_EQ_INT(lept::NUMBER, lept::get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, lept::get_number(e));
    }
    EXPECT_EQ_STRING("o", lept::get_object_key(&v, 6), lept::get_object_key_length(&v, 6));
    {
        lept::value* o = lept::get_object_value(&v, 6);
        EXPECT_EQ_INT(lept::OBJECT, lept::get_type(o));
        for (i = 0; i < 3; i++) {
            lept::value* ov = lept::get_object_value(o, i);
            EXPECT_TRUE('1' + i == lept::get_object_key(o, i)[0]);
            EXPECT_EQ_INT(1, lept::get_object_key_length(o, i));
            EXPECT_EQ_INT(lept::NUMBER, lept::get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, lept::get_number(ov));
        }
    }
    lept::fre(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();

    test_access_string();
}

int main() {
    // 检测是否内存泄露
    #ifdef _WINDOWS
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}