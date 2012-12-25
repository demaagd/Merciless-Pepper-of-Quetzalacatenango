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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

char *fmmap(char *base, char *file) {
	char *tf=NULL;
	char *ret=NULL;
	struct stat ts;

	if(file!=NULL) {
		tf=calloc(strlen(base)+strlen(file)+3,sizeof(char));
		sprintf(tf,"%s/%s",base,file);
	} else {
		tf=base;
	}

	LOG_DEBUG(vlevel,"Looking for file %s\n", tf);
	if(stat(tf,&ts)==0) {
		if(S_ISREG(ts.st_mode) && (S_IRUSR & ts.st_mode)) {
			ret=mmap(0, ts.st_size, PROT_READ, MAP_SHARED, open(tf, O_RDONLY), 0);
			if(ret==MAP_FAILED) {
				LOG_ERROR(vlevel, "Unable to mmap() %s: %i %s\n", tf, errno, strerror(errno));
				ret=NULL;
			}
		} else {
			LOG_ERROR(vlevel, "Not a file or not readable: %s\n", tf);
			ret=NULL;
		}
	} else {
		LOG_ERROR(vlevel,"Unable to stat %s: %i %s\n",tf, errno, strerror(errno));
		ret=NULL;
	}
	if(file!=NULL) {
		free(tf);
	}
	return ret;
}
// url_decode() is ripped out of mongoose.c, was too useful to bother rewriting, didn't want to mess with the .h...  Maybe I should address that
int url_decode(const char *src, size_t src_len, char *dst, size_t dst_len, int is_form_url_encoded) {
  size_t i, j;
  int a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

  for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
    if (src[i] == '%' &&
        isxdigit(* (const char *) (src + i + 1)) &&
        isxdigit(* (const char *) (src + i + 2))) {
      a = tolower(* (const char *) (src + i + 1));
      b = tolower(* (const char *) (src + i + 2));
      dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
      i += 2;
    } else if (is_form_url_encoded && src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }

  dst[j] = '\0'; // Null-terminate the destination

  return (int)j;
}


// replaces all instances of sstr (no {}) with dstr in instr
char *strreplace(const char* instr, char *sstr, char *dstr) {
	char *ret=NULL;
	char* rsstr=calloc(strlen(sstr)+3, sizeof(char));
	int *ridx=calloc(1, sizeof(int));
	int ct=0;
	int n=0;
	int rsstrlen=0;
	int dstrlen=strlen(dstr);
	int instrlen=strlen(instr);
	int instridx=0;
	int dstridx=0;
	int ridxidx=0;

	ridx[0]=-1;

	sprintf(rsstr,"{%s}",sstr);
	rsstrlen=strlen(rsstr);

	if(instr && sstr) { 
		while(n<instrlen) {
			if(strncmp(instr+n, rsstr, rsstrlen)==0) { 
				ridx=realloc(ridx,sizeof(int)*(ct+1));
				ridx[ct]=n;
				ct++;
			}
			n++;
		}
		if(ridx && ct) {
			ret=calloc(instrlen+(ct*(dstrlen-rsstrlen))+1,sizeof(char));
			
			instridx=0;
			dstridx=0;
			ridxidx=0;
			while(instridx<instrlen-1) {
				if(instridx==ridx[ridxidx]) {
					memcpy(ret+dstridx,dstr,dstrlen);
					instridx+=rsstrlen;
					dstridx+=dstrlen;
					ridxidx++;
				} else {
					ret[dstridx]=instr[instridx];
					dstridx++;
					instridx++;
				}
			}
		}
	}
	free(rsstr);
	free(ridx);
	return ret;
}

void jsondeslash(char **jstr) {
	int i=0,j=0;
	int jl=strlen(*jstr);
	while(i<=jl) {
		if((*jstr)[i]=='\\') {
			i++;	
		} else {
			(*jstr)[j]=(*jstr)[i];
			i++;
			j++;
		}
	}
}
