# LEPTJSON

轻量级 json 解析库, 根据开源项目开源项目 json-tutorial ( 项目地址 https://github.com/miloyip/json-tutorial ) 开发

使用 c++ 编写, 但由于 json-tutorial 使用 c 开发, 故没有使用 STL 与 class

所有声明和定义位于命名空间 lept 中

## 文件结构

leptjson.cpp 和 leptjson.h 被编译为静态库，链接到 test.cpp 进行测试

## 使用方法

可用函数定义:

```c++
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
```
使用:

```c++
lept::value v;
lept::parse(&v, "{ }") // json 文本
// 之后可根据上述定义函数获取相应键或值
```

## 项目总结

* cmake 构建
* 静态库和动态库
* 单元测试
* 测试驱动开发 ( tdd ) 
* 断言
* 内存泄漏检测
* utf-8 编码
* 熟悉枚举，前向声明, 静态函数, 联合等语法规则, memcpy, strtod 等函数使用
* 宏定义 do while 包裹技巧, \#ifdef 0/1 注释技巧
* 代码重构

## LICENCE

MIT