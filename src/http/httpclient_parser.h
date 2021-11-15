/**
 * @file httpclient_parser.h
 * @brief  客户端 http 协议解析 
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-14
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __HTTPCLIENT_PARSER_H__
#define __HTTPCLIENT_PARSER_H__

#include "http11_common.h"

typedef struct httpclient_parser { 
  int cs;
  size_t body_start;
  int content_len;
  int status;
  int chunked;
  int chunks_done;
  int close;
  size_t nread;
  size_t mark;
  size_t field_start;
  size_t field_len;

  void *data;

  field_cb http_field;
  element_cb reason_phrase;
  element_cb status_code;
  element_cb chunk_size;
  element_cb http_version;
  element_cb header_done;
  element_cb last_chunk;
  
  
} httpclient_parser;

int httpclient_parser_init(httpclient_parser *parser);
int httpclient_parser_finish(httpclient_parser *parser);
int httpclient_parser_execute(httpclient_parser *parser, const char *data, size_t len, size_t off);
int httpclient_parser_has_error(httpclient_parser *parser);
int httpclient_parser_is_finished(httpclient_parser *parser);

#define httpclient_parser_nread(parser) (parser)->nread 

#endif