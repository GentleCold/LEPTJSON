//
// Created by GentleCold on 2022/8/13.
//

#include "leptjson.h"

#include <cerrno>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstring>

#define EXPECT(c, ch) do { assert(*(c -> json) == (ch)); c -> json++; } while(0)
#define ISDIGIT(ch) (ch >= '0' && ch <= '9')
#define PUTC(c, ch) (*(char *) context_push(c, sizeof(char)) = ch)

#define STRING_ERROR(error) do { c -> top = head; return error; } while(0)

#ifndef PARSE_STACK_INIT_CAPACITY
#define PARSE_STACK_INIT_CAPACITY 256
#endif

namespace lept {

    struct context {
        const char *json = "";
        char *stack = nullptr;
        size_t capacity = 0, top = 0;
    };

    static void* context_push(context *c, size_t size) {
        void *ret;
        assert(size > 0);
        if (c -> top + size >= c -> capacity) {
            if (c -> capacity == 0) c -> capacity = PARSE_STACK_INIT_CAPACITY;
            while (c -> top + size >= c -> capacity) {
                c -> capacity += c -> capacity >> 1; // * 1.5
            }
            c -> stack = (char *) realloc(c -> stack, c -> capacity);
        }

        ret = c -> stack + c -> top;
        c -> top += size;
        return ret;
    }

    static void* context_pop(context *c, size_t size) {
        assert(c -> top >= size);
        return c -> stack + (c -> top -= size);
    }

    static const char* parse_hex4(const char *p, unsigned *u) {
        *u = 0;
        for (size_t i = 0; i < 4; i++) {
            *u <<= 4;
            if (ISDIGIT(p[i]))
                *u |= (p[i] - '0');
            else if (p[i] >= 'a' && p[i] <= 'f')
                *u |= (p[i] - 'a' + 10);
            else if (p[i] >= 'A' && p[i] <= 'F')
                *u |= (p[i] - 'A' + 10);
            else return nullptr;
        }

        p += 4;
        return p;
    }

    static void encode_utf8(context *c, unsigned u) {
        if (u <= 0x7F)
        PUTC(c, u & 0xFF);
        else if (u <= 0x7FF) {
            PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
            PUTC(c, 0x80 | ( u       & 0x3F));
        }
        else if (u <= 0xFFFF) {
            PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
            PUTC(c, 0x80 | ((u >>  6) & 0x3F));
            PUTC(c, 0x80 | ( u        & 0x3F));
        }
        else {
            assert(u <= 0x10FFFF);
            PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
            PUTC(c, 0x80 | ((u >> 12) & 0x3F));
            PUTC(c, 0x80 | ((u >>  6) & 0x3F));
            PUTC(c, 0x80 | ( u        & 0x3F));
        }
    }

    static void parse_whitespace(context *c) {
        const char *p = c -> json;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
            p++;
        c -> json = p;
    }

    static int parse_literal(context *c, value *v, const char *literal, var type) {
        EXPECT(c, literal[0]);
        size_t i;
        for (i = 0; literal[i + 1]; i++) {
            if (c -> json[i] != literal[i + 1])
                return PARSE_INVALID_VALUE;
        }

        v -> type = type;
        c -> json += i;

        return PARSE_OK;
    }

    static int parse_number(context *c, value *v) {
        const char *p = c -> json;

        if (*p == '-') p++;

        if (*p == '0') p++;
        else {
            if (!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
            while (ISDIGIT(*p)) p++;
        }

        if (*p == '.') {
            p++;
            if (!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
            while (ISDIGIT(*p)) p++;
        }

        if (*p == 'e' || *p == 'E') {
            p++;
            if (*p == '-' || *p == '+') p++;
            if (!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
            while (ISDIGIT(*p)) p++;
        }

        errno = 0;
        v -> u.n = strtod(c -> json, nullptr); // str to double

        if (errno == ERANGE && (v -> u.n == HUGE_VAL || v -> u.n == -HUGE_VAL))
            return PARSE_NUMBER_TOO_BIG;

        c -> json = p;
        v -> type = NUMBER;
        return PARSE_OK;
    }

    static int parse_string_raw(context *c, char **s, size_t *len) {
        size_t head = c -> top;
        unsigned u = 0, u2 = 0;
        const char *p;
        EXPECT(c, '\"');
        p = c -> json;

        while (true) {
            char ch = *p++;
            switch (ch) {
                case '\"':
                    *len = c -> top - head;
                    *s = (char *)context_pop(c, *len);
                    c -> json = p;
                    return PARSE_OK;
                case '\0':
                    STRING_ERROR(PARSE_MISS_QUOTATION_MARK);
                case '\\':
                    switch (*p++) {
                        case '\"': PUTC(c, '\"'); break;
                        case '\\': PUTC(c, '\\'); break;
                        case '/': PUTC(c, '/' ); break;
                        case 'b': PUTC(c, '\b'); break;
                        case 'f': PUTC(c, '\f'); break;
                        case 'n': PUTC(c, '\n'); break;
                        case 'r': PUTC(c, '\r'); break;
                        case 't': PUTC(c, '\t'); break;
                        case 'u':
                            if (!(p = parse_hex4(p, &u)))
                                STRING_ERROR(PARSE_INVALID_UNICODE_HEX);

                            if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                                if (*p++ != '\\')
                                    STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                                if (*p++ != 'u')
                                    STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                                if (!(p = parse_hex4(p, &u2)))
                                    STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
                                if (u2 < 0xDC00 || u2 > 0xDFFF)
                                    STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                                u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                            }

                            encode_utf8(c, u);
                            break;
                        default:
                            STRING_ERROR(PARSE_INVALID_STRING_ESCAPE);
                    }
                    break;
                default:
                    PUTC(c, ch);
            }
        }
    }

    static int parse_string(context *c, value *v) {
        int ret;
        char *s;
        size_t len;
        if ((ret = parse_string_raw(c, &s, &len)) == PARSE_OK)
            set_string(v, s, len);

        return ret;
    }

    static int parse_value(context *c, value *v);

    static int parse_array(context *c, value *v) {
        size_t size = 0;
        int ret;
        EXPECT(c, '[');
        parse_whitespace(c);
        if (*c -> json == ']') {
            c -> json++;
            v -> type = ARRAY;
            v -> u.a.size = size;
            v -> u.a.e = nullptr;
            return PARSE_OK;
        }

        while (true) {
            parse_whitespace(c);
            value e;
            if ((ret = parse_value(c, &e)) != PARSE_OK)
                break;

            memcpy(context_push(c, sizeof(value)), &e, sizeof(value));
            size++;
            parse_whitespace(c);
            if (*c -> json == ',') {
                c -> json++;
            } else if (*c -> json == ']') {
                c -> json++;
                v -> type = ARRAY;
                v -> u.a.size = size;
                memcpy(v -> u.a.e = new value[size], context_pop(c, size * sizeof(value)), size * sizeof(value));
                return  PARSE_OK;
            } else {
                ret = PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
                break;
            }
        }

        for (size_t i = 0; i < size; i++)
            fre((value*)context_pop(c, sizeof(value)));
        return ret;
    }

    static int parse_object(context *c, value *v) {
        size_t size = 0;
        member m;
        int ret;
        EXPECT(c, '{');
        parse_whitespace(c);
        if (*c -> json == '}') {
            c -> json++;
            v -> type = OBJECT;
            v -> u.o.size = size;
            v -> u.o.m = nullptr;
            return PARSE_OK;
        }

        while (true) {
            char *s;
            if ( *c -> json != '"') {
                ret = PARSE_MISS_KEY;
                break;
            }

            if ((ret = parse_string_raw(c, &s, &m.kLen) != PARSE_OK))
                break;

            memcpy(m.k = (char*)malloc(m.kLen + 1), s, m.kLen);
            m.k[m.kLen] = '\0';

            parse_whitespace(c);

            if (*c -> json != ':') {
                ret = PARSE_MISS_COLON;
                break;
            }

            c -> json++;

            parse_whitespace(c);

            if ((ret = parse_value(c, &m.v)) != PARSE_OK)
                break;

            memcpy(context_push(c, sizeof(member)), &m, sizeof(member));
            size++;
            m.k = nullptr; // has transferred to stack

            parse_whitespace(c);

            if (*c -> json == ',') {
                c -> json++;
                parse_whitespace(c);
            } else if (*c -> json == '}') {
                size_t l = sizeof(member) * size;
                c -> json++;
                v -> type = OBJECT;
                v -> u.o.size = size;
                memcpy(v -> u.o.m = new member[size], context_pop(c, l), l);
                return PARSE_OK;
            } else {
                ret = PARSE_MISS_COMMA_OR_CURLY_BRACKET;
                break;
            }
        }

        free(m.k);
        for (size_t i = 0; i < size; i++) {
            member* m = (member*)context_pop(c,sizeof(member));
            free(m -> k);
            fre(&m -> v);
        }

        v -> type = NUL;
        return ret;
    }

    static int parse_value(context *c, value *v) {
        switch (*c -> json) {
            case 't': return parse_literal(c, v, "true", TRUE);
            case 'f': return parse_literal(c, v, "false", FALSE);
            case 'n': return parse_literal(c, v, "null", NUL);
            case '[': return parse_array(c, v);
            case '{': return parse_object(c, v);
            case '\"': return parse_string(c, v);
            case '\0': return PARSE_EXPECT_VALUE;
            default: return parse_number(c, v);
        }
    }

    int parse(value *v, const char *json) {
        context c;
        int ret;

        assert(v != nullptr);

        c.json = json;
        v -> type = NUL;

        parse_whitespace(&c);

        if ((ret = parse_value(&c, v)) == PARSE_OK) {
            parse_whitespace(&c);
            if (*c.json != '\0') {
                v -> type = NUL;
                ret = PARSE_ROOT_NOT_SINGULAR;
            }
        }
        assert(c.top == 0);
        delete c.stack;

        return ret;
    }

    void fre(value *v) {
        assert(v != nullptr);
        if (v -> type == STRING) {
            free(v -> u.s.s);
        } else if (v -> type == ARRAY) {
            for (size_t i = 0; i < v -> u.a.size; i++) {
                fre(&v -> u.a.e[i]);
            }

            free(v -> u.a.e);
        } else if (v -> type == OBJECT) {
            for (size_t i = 0; i < v -> u.o.size; i++) {
                free(v -> u.o.m[i].k);
                fre(&v -> u.o.m[i].v);
            }
            free(v -> u.o.m);
        }
        v -> type = NUL;
    }

    const char* get_string(const value* v) {
        assert(v != nullptr && v -> type == STRING);
        return v -> u.s.s;
    }

    size_t get_string_length(const value* v) {
        assert(v != nullptr && v -> type == STRING);
        return v -> u.s.len;
    }

    void set_string(value *v, const char *s, size_t len) {
        assert(v != nullptr && (s != nullptr || len == 0));
        fre(v);
        v -> u.s.s = new char[len + 1];
        memcpy(v -> u.s.s, s, len);
        v -> u.s.s[len] = '\0';
        v -> u.s.len = len;
        v -> type = STRING;
    }

    size_t get_array_size(const value *v) {
        assert(v != nullptr && v -> type == ARRAY);
        return v -> u.a.size;
    }

    value* get_array_element(const value *v, size_t index) {
        assert(v != nullptr && v -> type == ARRAY);
        assert(index < v -> u.a.size);
        return &v -> u.a.e[index];
    }

    size_t get_object_size(const value* v) {
        assert(v != nullptr && v -> type == OBJECT);
        return v -> u.o.size;
    }

    const char* get_object_key(const value* v, size_t index) {
        assert(v != nullptr && v -> type == OBJECT);
        assert(index < v -> u.o.size);
        return v -> u.o.m[index].k;
    }

    size_t get_object_key_length(const value* v, size_t index) {
        assert(v != nullptr && v -> type == OBJECT);
        assert(index < v -> u.o.size);
        return v -> u.o.m[index].kLen;
    }
    value* get_object_value(const value* v, size_t index) {
        assert(v != nullptr && v -> type == OBJECT);
        assert(index < v -> u.o.size);
        return &v -> u.o.m[index].v;
    }

    var get_type(const value *v) {
        assert(v != nullptr);
        return  v -> type;
    }

    double get_number(const value *v) {
        assert(v != nullptr && v -> type == NUMBER);
        return v -> u.n;
    }
}

