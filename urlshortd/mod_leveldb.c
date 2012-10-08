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
 
#include <leveldb/c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

int vlevel=3;

typedef struct {
	leveldb_t *dbh;
  leveldb_options_t *dbopt;
  leveldb_readoptions_t* ropt;
  leveldb_writeoptions_t* wopt;
	char *errptr;
} dbhandle;

int db_init(dbhandle **dbh, char *conf, int vl) {
	
	vlevel=vl;
	*dbh=calloc(sizeof(dbhandle),1);

	LOG_ERROR(vlevel, "Setting up leveldb store in %s\n",conf);
  (*dbh)->dbopt=leveldb_options_create();
  leveldb_options_set_create_if_missing((*dbh)->dbopt, 1);
  leveldb_options_set_write_buffer_size((*dbh)->dbopt, 8388608);
  (*dbh)->dbh=leveldb_open((*dbh)->dbopt,conf,&((*dbh)->errptr));
  
	LOG_ERROR(vlevel, "Setting leveldb read options\n");
  (*dbh)->ropt = leveldb_readoptions_create();
  leveldb_readoptions_set_verify_checksums((*dbh)->ropt, 1);
  leveldb_readoptions_set_fill_cache((*dbh)->ropt, 0);

	LOG_ERROR(vlevel, "Setting leveldb write options\n");
  (*dbh)->wopt = leveldb_writeoptions_create();
  leveldb_writeoptions_set_sync((*dbh)->wopt, 1);

	return 0;	
}

int db_insert(dbhandle **dbh, char *key, char *val, int exp) {

	leveldb_put((*dbh)->dbh, (*dbh)->wopt, key, strlen(key), val, strlen(val), &((*dbh)->errptr));
	if((*dbh)->errptr!=NULL) {
		LOG_ERROR(vlevel, "leveldb_put() error: '%s', '%s': %s\n",key,val,(*dbh)->errptr);
		return 1;
	}

	return 0;
}

int db_select(dbhandle **dbh, char *key, char **ret) {
	size_t klen=strlen(key);

	char *tmp=leveldb_get((*dbh)->dbh, (*dbh)->ropt, key, strlen(key), &klen, &((*dbh)->errptr));
	if((*dbh)->errptr!=NULL) {
		LOG_ERROR("leveldb_get() error: '%s': %s\n",key,(*dbh)->errptr);
		return 1; 
	}
	*ret=calloc(klen+2,sizeof(char));
	snprintf(*ret,klen+1,"%s",tmp);

	return 0;
}

int db_shutdown(dbhandle **dbh) {
  LOG_DEBUG(vlevel, "Cleaning up leveldb\n");
  leveldb_options_destroy((*dbh)->dbopt);
  leveldb_readoptions_destroy((*dbh)->ropt);
  leveldb_writeoptions_destroy((*dbh)->wopt);
  leveldb_compact_range((*dbh)->dbh, NULL, 0, NULL, 0);
  leveldb_close((*dbh)->dbh);

	return 0;
}
