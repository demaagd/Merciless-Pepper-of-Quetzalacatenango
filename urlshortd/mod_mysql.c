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
	char *cfgjson;
	struct json_object *cfgjsonobj;
	struct json_object *tj;

	char *t;
	
	char *hostname;
	int port;
	char *database;
	char *username;
	char *password;
	
	vlevel=vl;

	if(access(conf,F_OK|R_OK)) {
		LOG_FATAL(vlevel, _("Config file missing or not readable: %s: %i %s\n"), conf, errno, strerror(errno));
		return 1;
	}
	cfgjson=fmmap(conf,NULL);

	LOG_DEBUG(vlevel,_("Loading MySQL connection info from %s\n"),conf);
	cfgjsonobj=json_tokener_parse(cfgjson);


	tj=json_object_object_get(cfgjsonobj, "hostname");
	t=(char*)json_object_to_json_string(tj);
	hostname=calloc(strlen(t),sizeof(char));
	snprintf(hostname,strlen(t)-1,"%s",t+1);
	json_object_put(tj);	
	LOG_TRACE(vlevel, _("MySQL hostname: %s\n"), hostname);

	tj=json_object_object_get(cfgjsonobj, "port");
	port=json_object_get_int(tj);
	json_object_put(tj);	
	LOG_TRACE(vlevel, _("MySQL port: %i\n"), port);

	tj=json_object_object_get(cfgjsonobj, "database");
	t=(char*)json_object_to_json_string(tj);
	database=calloc(strlen(t),sizeof(char));
	snprintf(database,strlen(t)-1,"%s",t+1);
	json_object_put(tj);	
	LOG_TRACE(vlevel, _("MySQL database: %s\n"), database);

	tj=json_object_object_get(cfgjsonobj, "username");
	t=(char*)json_object_to_json_string(tj);
	username=calloc(strlen(t),sizeof(char));
	snprintf(username,strlen(t)-1,"%s",t+1);
	json_object_put(tj);	
	LOG_TRACE(vlevel, _("MySQL username: %s\n"), username);

	tj=json_object_object_get(cfgjsonobj, "password");
	t=(char*)json_object_to_json_string(tj);
	password=calloc(strlen(t),sizeof(char));
	snprintf(password,strlen(t)-1,"%s",t+1);
	json_object_put(tj);	
	LOG_TRACE(vlevel, _("MySQL password: %s\n"), password);

	*dbh=calloc(sizeof(dbhandle),1);	
	(*dbh)->handle = mysql_init(NULL);	
	
	if((*dbh)->handle == NULL) {
		LOG_FATAL(vlevel, _("Init failed: %i %s\n"), mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return mysql_errno((*dbh)->handle);
	}

	if(!mysql_real_connect((*dbh)->handle, hostname, username, password, database, port, NULL, 0)) {
		LOG_FATAL(vlevel, _("Connect failed: %i %s\n"), mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return mysql_errno((*dbh)->handle);
	}

	free(hostname);
	free(database);
	free(username);
	free(password);
	//json_object_put(cfgjsonobj);
	return 0;	
}

int db_insert(dbhandle **dbh, char *key, char *val) {
	MYSQL_STMT *sth=mysql_stmt_init((*dbh)->handle);
	MYSQL_BIND param[2];

	unsigned long kl = strlen(key);
	unsigned long vl = strlen(val);

	int ret=0;

	LOG_DEBUG(vlevel, _("Inserting '%s' '%s'\n"),key, val);
	LOG_TRACE(vlevel, _("Creating prepared statement for'%s' '%s'\n"),key, val);
	if(sth == NULL) {
		LOG_FATAL(vlevel, _("Unable to create statement handle %i %s\n"), mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		return 1;
	}
	if(mysql_stmt_prepare(sth, "INSERT INTO urlshortd VALUES(?,?)", 33) != 0) {
		LOG_FATAL(vlevel, _("Unable to create prepared statement %i %s\n"), mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
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
		LOG_FATAL(vlevel, _("Unable to bind parameters %i %s\n"), mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
		ret=1;
	} else {
		LOG_DEBUG(vlevel, _("MySQL running statement\n"));
		if(mysql_stmt_execute(sth) != 0) {
			LOG_FATAL(vlevel, _("Unable to execute statement %i %s\n"), mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
			ret=1;
		}
	}
	mysql_stmt_free_result(sth);
	mysql_stmt_close(sth);

	return ret;
}

int db_select(dbhandle **dbh, char *key, char **ret) {
	MYSQL_RES *result;
  MYSQL_ROW row;
	char *q=calloc(72,sizeof(char));

	LOG_DEBUG(vlevel, _("Selecting '%s'\n"),key);

	sprintf(q,"SELECT uri FROM urlshortd where hash='%s'", key);
	mysql_query((*dbh)->handle, q);
  result = mysql_store_result((*dbh)->handle);
	if((row = mysql_fetch_row(result))) {
		LOG_DEBUG(vlevel, _("MySQL found row: %s\n"), row[0]);
		*ret=calloc(strlen(row[0])+1,sizeof(char));
		strcpy(*ret, row[0]);
	} else {
		LOG_ERROR(vlevel, _("MySQL failed for query: '%s': %i %s\n"), q, mysql_errno((*dbh)->handle), mysql_error((*dbh)->handle));
	}

	mysql_free_result(result);
	free (q);
	return 0;
}

int db_shutdown(dbhandle **dbh) {
  LOG_DEBUG(vlevel, _("Cleaning up MySQL\n"));
  mysql_close((*dbh)->handle);
	free(*dbh);
	return 0;
}
