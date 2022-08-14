#ifndef LEPTJSON_H
#define LEPTJSON_H

#include <cstddef>

namespace lept {
    typedef enum {
        NUL, // 区别于 NULL
        FALSE,
        TRUE,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT
    } var;

    enum {
        PARSE_OK = 0,
        PARSE_EXPECT_VALUE,
        PARSE_INVALID_VALUE,
        PARSE_ROOT_NOT_SINGULAR,
        PARSE_NUMBER_TOO_BIG,
        PARSE_MISS_QUOTATION_MARK,
        PARSE_INVALID_STRING_ESCAPE,
        PARSE_INVALID_UNICODE_HEX,
        PARSE_INVALID_UNICODE_SURROGATE,
        PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
        PARSE_MISS_KEY,
        PARSE_MISS_COLON,
        PARSE_MISS_COMMA_OR_CURLY_BRACKET,
    };

    struct value;   // forward declare
    struct member;

    struct value {
        union {
            struct {
               member *m;
               size_t size;
            } o;
            struct {
                value *e;
                size_t size;
            } a; // array

            struct {
                char *s;
                size_t len;
            } s;
            double n;
        } u = {};

        var type = NUL;
    };

    struct member {
        char *k = nullptr;
        size_t kLen = 0;
        value v;
    };

    int parse(value *v, const char *json);
    void fre(value *v); // different from free

    const char* get_string(const value *v);
    size_t get_string_length(const value *v);
    void set_string(value *v, const char *s, size_t len);

    size_t get_array_size(const value *v);
    value* get_array_element(const value *v, size_t index);

    size_t get_object_size(const value* v);
    const char* get_object_key(const value* v, size_t index);
    size_t get_object_key_length(const value* v, size_t index);
    value* get_object_value(const value* v, size_t index);

    var get_type(const value *v);
    double get_number(const value *v);
}

#endif // LEPTJSON_H
