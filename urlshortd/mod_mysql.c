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

#include <json/json.h> 
#include <mysql.h>
#include <my_global.h>
#include <my_sys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

int vlevel=3;

typedef struct {
	MYSQL *handle;
} dbhandle;

int db_init(dbhandle **dbh, char *conf, int vl) {
	char *cfgjson=fmmap(conf,NULL);
	struct json_object *cfgjsonobj;

	char *t;
	
	char *hostname;
	int port;
	char *database;
	char *username;
	char *password;
	
	vlevel=vl;

	LOG_DEBUG(vlevel,"Loading MySQL connection info from %s\n",conf);
	cfgjsonobj=json_tokener_parse(cfgjson);
	t=(char*)json_object_to_json_string(json_object_object_get(cfgjsonobj, "hostname"));
	hostname=calloc(strlen(t),sizeof(char));
	snprintf(hostname,strlen(t)-1,"%s",t+1);
	LOG_DEBUG(vlevel, "MySQL hostname: %s\n", hostname);

	port=json_object_get_int(json_object_object_get(cfgjsonobj, "port"));
	LOG_DEBUG(vlevel, "MySQL port: %i\n", port);

	t=(char*)json_object_to_json_string(json_object_object_get(cfgjsonobj, "database"));
	database=calloc(strlen(t),sizeof(char));
	snprintf(database,strlen(t)-1,"%s",t+1);
	LOG_DEBUG(vlevel, "MySQL database: %s\n", database);

	t=(char*)json_object_to_json_string(json_object_object_get(cfgjsonobj, "username"));
	username=calloc(strlen(t),sizeof(char));
	snprintf(username,strlen(t)-1,"%s",t+1);
	LOG_DEBUG(vlevel, "MySQL username: %s\n", username);

	t=(char*)json_object_to_json_string(json_object_object_get(cfgjsonobj, "password"));
	password=calloc(strlen(t),sizeof(char));
	snprintf(password,strlen(t)-1,"%s",t+1);
	LOG_DEBUG(vlevel, "MySQL password: %s\n", password);

	*dbh=calloc(sizeof(dbhandle),1);	
	(*dbh)->handle = mysql_init(NULL);	
	
	if((*dbh)->handle == NULL) {
		LOG_DEBUG(vlevel, "Init failed: %i %s\n", mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return mysql_errno((*dbh)->handle);
	}

	if(!mysql_real_connect((*dbh)->handle, hostname, username, password, database, port, NULL, 0)) {
		LOG_DEBUG(vlevel, "Connect failed: %i %s\n", mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return mysql_errno((*dbh)->handle);
	}

	return 0;	
}

int db_insert(dbhandle **dbh, char *key, char *val) {
	MYSQL_STMT *sth=mysql_stmt_init((*dbh)->handle);
	MYSQL_BIND param[2];

	unsigned long kl = strlen(key);
	unsigned long vl = strlen(val);

	if(sth == NULL) {
		LOG_FATAL(vlevel, "Unable to create statement handle %i %s\n", mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return 1;
	}
	if(mysql_stmt_prepare(sth, "INSERT INTO urlshortd VALUES(?,?)", 33) != 0) {
		LOG_FATAL(vlevel, "Unable to create prepared statement %i %s\n", mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return 1;
	}

	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_STRING;
	param[0].buffer = key;
	param[0].is_unsigned = 0;
	param[0].is_null = 0;
	param[0].length = &kl;

	param[1].buffer_type = MYSQL_TYPE_STRING;
	param[1].buffer = val;
	param[1].is_unsigned = 0;
	param[1].is_null = 0;
	param[1].length = &vl;

	if(mysql_stmt_bind_param(sth, param) != 0) {
		LOG_FATAL(vlevel, "Unable to bind parameters %i %s\n", mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return 1;
	}

	LOG_DEBUG(vlevel, "MySQL running statement\n");
	if(mysql_stmt_execute(sth) != 0) {
		LOG_FATAL(vlevel, "Unable to execute statement %i %s\n", mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return 1;
	}

	mysql_stmt_free_result(sth);
	mysql_stmt_close(sth);

	return 0;
}

int db_select(dbhandle **dbh, char *key, char **ret) {
	MYSQL_RES *result;
  MYSQL_ROW row;
	char *q=calloc(72,sizeof(char));

	sprintf(q,"SELECT uri FROM urlshortd where hash='%s'", key);
	mysql_query((*dbh)->handle, q);
  result = mysql_store_result((*dbh)->handle);
	if((row = mysql_fetch_row(result))) {
		LOG_DEBUG(vlevel, "MySQL found row: %s\n", row[0]);
		*ret=calloc(strlen(row[0])+1,sizeof(char));
		strcpy(*ret, row[0]);
	} else {
		LOG_DEBUG(vlevel, "MySQL failed for query: '%s': %i %s\n", q, mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
	}

	mysql_free_result(result);

	return 0;
}

int db_shutdown(dbhandle **dbh) {
  mysql_close((*dbh)->handle);

	return 0;
}
