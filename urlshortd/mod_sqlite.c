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
 
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

typedef struct {
	sqlite3 *handle;
} dbhandle;

int vlevel=3;

int db_init(dbhandle **dbh, char *conf, int vl) {
	vlevel=vl;

	LOG_DEBUG(vlevel, "Opening SQLite database using %s as data file\n", conf);
	*dbh=calloc(sizeof(dbhandle),1);
	if(sqlite3_open(conf,&((*dbh)->handle)))  {
		LOG_FATAL(vlevel, "Failed to open database: %s: %s\n",conf, sqlite3_errmsg((*dbh)->handle));
		return 1;
	} else {
		LOG_DEBUG(vlevel, "Successfully opened database\n");
	}

	LOG_DEBUG(vlevel, "Checking schema on DB\n");	
	if(sqlite3_exec((*dbh)->handle,"CREATE TABLE IF NOT EXISTS 'urlshortd' (hash TEXT PRIMARY KEY, uri TEXT, expires INTEGER)",0,0,0)) {
		LOG_FATAL(vlevel, "Error creating table in database: %s: %s\n", conf, sqlite3_errmsg((*dbh)->handle));
		return 2;
	}

	return 0;	
}

int db_insert(dbhandle **dbh, char *key, char *val, int exp) {
	int ret=0;
	sqlite3_stmt *dbins;
	LOG_DEBUG(vlevel, "Creating prepared statement: insert\n");
	if(sqlite3_prepare((*dbh)->handle, "INSERT INTO urlshortd VALUES(?,?,?)", -1, &dbins, 0)) {
		LOG_FATAL(vlevel, "Error creating insert prepared statement: %s\n", sqlite3_errmsg((*dbh)->handle));
		exit(EXIT_FAILURE);
	}

	LOG_DEBUG(vlevel, "Insert request: %s: %s %i\n",val, key, exp);

	if(sqlite3_bind_text(dbins, 1, key, strlen(key), SQLITE_STATIC)!=SQLITE_OK) {
		LOG_ERROR(vlevel, "Error binding hash value: %s: %s\n",key, sqlite3_errmsg((*dbh)->handle));
		ret=2;
	}
	if(sqlite3_bind_text(dbins, 2, val, strlen(val), SQLITE_STATIC)) {
		LOG_ERROR(vlevel, "Error binding query string value: %s: %s\n", val, sqlite3_errmsg((*dbh)->handle));
		ret=2;
	}
	if(sqlite3_bind_int(dbins, 3, exp)) {
		LOG_ERROR(vlevel, "Error binding times tamp value: %i: %s\n",exp, sqlite3_errmsg((*dbh)->handle));
		ret=2;
	}

	if(sqlite3_step(dbins)!=SQLITE_DONE) {
		ret=2;
	}
	sqlite3_clear_bindings(dbins);
 	sqlite3_finalize(dbins);
	
	return ret;
}

int db_select(dbhandle **dbh, char *key, char **ret) {
	sqlite3_stmt *dbsel;
	int s;
	LOG_DEBUG(vlevel, "Creating prepared statement, select on key: %s\n",key);
	if(sqlite3_prepare((*dbh)->handle, "SELECT uri FROM urlshortd WHERE hash=?", -1, &dbsel, 0)) {
		LOG_FATAL(vlevel, "Error creating select prepared statement: %s\n", sqlite3_errmsg((*dbh)->handle));
		exit(EXIT_FAILURE);
	}

	sqlite3_bind_text(dbsel, 1, key, strlen(key), SQLITE_STATIC);

	// XXX error checking needed...  
	if((s=sqlite3_step(dbsel))==SQLITE_ROW) {
		char *tmp=(char*)sqlite3_column_text(dbsel, 0);
		*ret=strdup(tmp);

		LOG_DEBUG(vlevel,"Found a row: %s\n", *ret);
	} else { 
		ret=NULL;
	}
	sqlite3_clear_bindings(dbsel);
	sqlite3_finalize(dbsel);

	return 0;
}

int db_shutdown(dbhandle **dbh) {
	sqlite3_close((*dbh)->handle);
	return 0;
}
