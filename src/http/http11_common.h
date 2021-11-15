/**
 * @file http11_common.h
 * @brief http 协议解析
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-14
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __HTTP_COMMON_H__
#define __HTTP_COMMON_H__

#include <sys/types.h>

typedef void (*element_cb)(void *data, const char *at, size_t length);
typedef void (*field_cb)(void *data, const char *field, size_t flen, const char *value, size_t vlen);

#endif