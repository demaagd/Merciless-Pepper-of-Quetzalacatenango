// Copyright (c) 2012 Dave DeMaagd
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef __UTIL_H__
#define __UTIL_H__ 

#include <time.h>
#include <libintl.h>
#include <locale.h>

#ifndef _
#define _(STRING)    gettext(STRING)
#endif

extern int vlevel;

char *fmmap(char *base, char *file);
char *strreplace(const char* instr, char *sstr, char *dstr);
int url_decode(const char *src, size_t src_len, char *dst, size_t dst_len, int is_form_url_encoded);
void jsondeslash(char **jstr);

#define SHORT_STRING_MAX 512 
#define URL_STRING_MAX 8192
#define POST_DATA_STRING_MAX 16384

#define LOG_LVL_TRACE 4
#define LOG_LVL_DEBUG 3
#define LOG_LVL_INFO 2
#define LOG_LVL_WARN 1
#define LOG_LVL_ERROR 0
#define LOG_LVL_FATAL -1

#define LOG_TRACE(vlevel, fmt,...) do { if(vlevel >= LOG_LVL_TRACE) { \
      time_t t=time(NULL);                                    \
      char *tstr=calloc(SHORT_STRING_MAX,sizeof(char));                 \
      strftime(tstr,SHORT_STRING_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
      printf("[T:%s] TRACE ",tstr);                                    \
      free(tstr);                                                       \
      printf(fmt,##__VA_ARGS__);                                        \
    } } while(0)

#define LOG_DEBUG(vlevel, fmt,...) do { if(vlevel >= LOG_LVL_DEBUG) { \
      time_t t=time(NULL);                                    \
      char *tstr=calloc(SHORT_STRING_MAX,sizeof(char));                 \
      strftime(tstr,SHORT_STRING_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
      printf("[T:%s] DEBUG ",tstr);                                    \
      free(tstr);                                                       \
      printf(fmt,##__VA_ARGS__);                                        \
    } } while(0)

#define LOG_INFO(vlevel, fmt,...) do { if(vlevel >= LOG_LVL_INFO) { \
      time_t t=time(NULL);                                  \
      char *tstr=calloc(SHORT_STRING_MAX,sizeof(char));                 \
      strftime(tstr,SHORT_STRING_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
      printf("[T:%s] INFO ",tstr);                                     \
      free(tstr);                                                       \
      printf(fmt,##__VA_ARGS__);                                        \
    } } while(0)

#define LOG_WARN(vlevel, fmt,...) do { if(vlevel >= LOG_LVL_WARN) { \
      time_t t=time(NULL);                                  \
      char *tstr=calloc(SHORT_STRING_MAX,sizeof(char));                 \
      strftime(tstr,SHORT_STRING_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
      printf("[T:%s] WARN ",tstr);                                     \
      free(tstr);                                                       \
      printf(fmt,##__VA_ARGS__);                                        \
    } } while(0)

#define LOG_ERROR(vlevel, fmt,...) do { if(vlevel >= LOG_LVL_ERROR) { \
      time_t t=time(NULL);                                    \
      char *tstr=calloc(SHORT_STRING_MAX,sizeof(char));                 \
      strftime(tstr,SHORT_STRING_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
      printf("[T:%s] ERROR ",tstr);                                    \
      free(tstr);                                                       \
      printf(fmt,##__VA_ARGS__);                                        \
    } } while(0)

#define LOG_FATAL(vlevel, fmt,...) do { if(vlevel >= LOG_LVL_FATAL) { \
      time_t t=time(NULL);                                    \
      char *tstr=calloc(SHORT_STRING_MAX,sizeof(char));                 \
      strftime(tstr,SHORT_STRING_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
      printf("[T:%s] FATAL ",tstr);                                    \
      free(tstr);                                                       \
      printf(fmt,##__VA_ARGS__);                                        \
    } } while(0)

#define LOG_ALWAYS(vlevel, fmt,...) do {			      \
    time_t t=time(NULL);						\
    char *tstr=calloc(SHORT_STRING_MAX,sizeof(char));			\
    strftime(tstr,SHORT_STRING_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
    printf("[T:%s] ALWAYS ",tstr);					\
    free(tstr);								\
    printf(fmt,##__VA_ARGS__);						\
  } while(0)

#endif

