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
 
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>

#include "util.h"
#include "tmpldfl.h"
#include "mongoose.h"

int done=0;
int vlevel=0;

void *dbh; 
int (*db_init)(void **dbh, char *conf, int vl);
int (*db_insert)(void **dbh, char *key, char *val);
int (*db_select)(void **dbh, char *key, char **ret);
int (*db_shutdown)(void **dbh);

char **tmpldata;
#define TMPL_INDEX 0
#define TMPL_STATUS 1
#define TMPL_NEW 2
#define TMPL_REDIR 3
#define TMPL_LIST 4
#define TMPL_ERROR 5 // ERROR needs to be last, it's used for memory allocation 

void usage(char *err, int ec) {
	if(err!=NULL) {
		fprintf(stderr,_("Error: %s\n"),err);
		fprintf(stderr,"\n");
	}
	fprintf(stderr,"Usage:\n"
					" -d database definition -- Specifies database connection to use, module:/path/to/file\n"
					" -p port spec           -- Port nuber to listen on, passed directly to mongoose HTTP library\n"
					" -n N                   -- Number of HTTP serving threads (default: 10)\n"
					" -a /path/to/accessfile -- Access log file, must be writable (if it exists) or in a writable dir (if it does not exist, it will be created)\n"
					" -t /path/to/templates  -- Template directory\n"
					" -v                     -- Increases verbose level, can be specified multiple times\n"
					" -h                     -- This help listing\n"
);

	exit(ec);
}

void handlesig(int sig) {
	if(sig==SIGTERM || sig==SIGINT) {
		if(done) {
			exit(EXIT_FAILURE);
		} else {
			LOG_DEBUG(vlevel, gettext("Finishing...\n"));
			done=1; 
		}
	}
}

int ishash(char* str) {
	int n=0;
	int slen = strlen(str);

	if(slen>32) {
		return 0;
	}
	while(n<33 && n<slen) {
		if(!((str[n] >= '0' && str[n] <= '9') || (str[n] >= 'a' && str[n] <= 'f') || (str[n] >= 'A' && str[n] <= 'F'))) {
			return 0;
		}
		n++;
	}
	return 1;
}

static void *mghandle(enum mg_event event, struct mg_connection *conn) {
	const struct mg_request_info *request_info = mg_get_request_info(conn);
	if (event == MG_NEW_REQUEST) {
		char *req=calloc(URL_STRING_MAX+1,sizeof(char));
		struct in_addr saddr;

		strncpy(req,request_info->uri, URL_STRING_MAX);
 		saddr.s_addr = ntohl(request_info->remote_ip);

		LOG_DEBUG(vlevel, "Connection from: %s, request: %s\n", inet_ntoa(saddr), req);
		if(strncmp(req, "/status\0", 8) == 0) { // status
			// XXX how to get more status 
			char *status=strreplace(tmpldata[TMPL_STATUS],"STATUS","OK");
			mg_printf(conn,
								"HTTP/1.1 200 OK\r\n"
								"Content-Type: text/plain\r\n"
								"Content-Length: %d\r\n"        
								"\r\n"
								"%s",
								(int)strlen(status), status);
			free(status);
		} else if(strncmp(req, "/\0", 2) == 0) { // home page
			mg_printf(conn,
								"HTTP/1.1 200 OK\r\n"
								"Content-Type: text/HTML\r\n"
								"Content-Length: %d\r\n"        
								"\r\n"
								"%s",
								(int)strlen(tmpldata[TMPL_INDEX]), tmpldata[TMPL_INDEX]);
		} else if(strncmp(req, "/list\0", 6) == 0) { // list 
			// XXX fill in list page
		} else if(strncmp(req, "/n/\0", 4) == 0 && ((char*)(request_info->query_string))[0]=='u' && ((char*)(request_info->query_string))[1]=='=') { // new redirect
			int resplen;
			char *hash=calloc(33,sizeof(char));
			char *respurl=NULL;
			char *tu;
			char *tr;
			char *requrl;
			LOG_DEBUG(vlevel, "Looks like a new insert request: %s\n",request_info->query_string);

			mg_md5(hash, (char*)(request_info->query_string)+2, NULL);
			if(db_insert(&dbh, hash, (char*)(request_info->query_string)+2)) {
				char *errresp=strreplace(tmpldata[TMPL_ERROR],"MESSAGE","Unable to insert, maybe a duplicate?");
				mg_printf(conn,
									"HTTP/1.1 200 OK\r\n"
									"Content-Type: text/html\r\n"
									"Content-Length: %d\r\n"        
									"\r\n"
									"%s",
									(int)strlen(errresp), errresp);
				free(errresp);
			} else {
				resplen=48+strlen(mg_get_header(conn, "Host"));
				respurl=calloc(resplen, sizeof(char));
				snprintf(respurl,resplen,"http://%s/%s",mg_get_header(conn, "Host"), hash);

				requrl=calloc(strlen((char*)request_info->query_string)*2,sizeof(char));
				url_decode((char*)(request_info->query_string)+2, strlen((char*)(request_info->query_string)+2), requrl, strlen((char*)(request_info->query_string))*2, 1);
				tu=strreplace(tmpldata[TMPL_NEW],"ULINK",requrl);
				tr=strreplace(tu, "RLINK", respurl);

				mg_printf(conn,
									"HTTP/1.1 200 OK\r\n"
									"Content-Type: text/html\r\n"
									"Content-Length: %d\r\n"        
									"\r\n"
									"%s\r\n",
									(int)strlen(tr), tr);

				free(tr);
				free(tu);
				free(requrl);
				free(respurl);
			}
			free(hash);
		} else if (ishash(req+1) && strlen(req+1)==32) { // redirect
			char *uri=NULL, *uridec;
			//char *redir=NULL;
			LOG_DEBUG(vlevel, "Looks like a hash, should check DB: %s\n",req);
			
			db_select(&dbh, req+1, &uri);

			if(uri!=NULL) {
				uridec=calloc(strlen((char*)uri)*2,sizeof(char));
				url_decode(uri, strlen(uri), uridec, strlen((char*)uri)*2, 1);
		
				mg_printf(conn,
									"HTTP/1.1 301 Moved Permanently\r\n"
									"Content-Type: text/plain\r\n"
									"Content-Length: %d\r\n"
									"Location: %s\r\n"
									"\r\n"
									"Redirect to: %s\r\n",
									(int)strlen((char*)uridec)+15, (char*)uridec, (char*)uridec);
				free(uridec);
			} else {
				char *errresp=strreplace(tmpldata[TMPL_ERROR],"MESSAGE","Don't think that is a valid redirect");
				mg_printf(conn,
									"HTTP/1.1 200 OK\r\n"
									"Content-Type: text/html\r\n"
									"Content-Length: %d\r\n"        
									"\r\n"
									"%s",
									(int)strlen(errresp), errresp);
				free(errresp);
			}
			free(uri);
		} else { // other
			char *errresp=strreplace(tmpldata[TMPL_ERROR],"MESSAGE","Not sure what you meant by that...");
			mg_printf(conn,
								"HTTP/1.1 200 OK\r\n"
								"Content-Type: text/html\r\n"
								"Content-Length: %d\r\n"        
								"\r\n"
								"%s",
								(int)strlen(errresp), errresp);
			free(errresp);
		}
		free(req);
		return "";
	} else {
		return NULL;
	}
}

int main(int argc, char **argv) {
  int goopt;
	int listenport=8080;
	int numthreads=10;
	int tf;

	void *dlh;

	char *dle;
	char *dbs=NULL;
	char *lpstr=NULL;
	char *ntstr=NULL;
	char *alfile=NULL;
	char *tdir=NULL;

	struct mg_context *ctx= NULL; 
  char **mgoptions;

	signal(SIGINT,handlesig);
  signal(SIGTERM,handlesig);

  setlocale(LC_ALL, "");
  textdomain("urlshortd");

	// command line parsing
	while ((goopt=getopt (argc, argv, "d:p:n:a:t:vh")) != -1) {
		switch (goopt) {
		case 'd': // database 
			dbs=calloc(strlen((char*)optarg)+1,sizeof(char));
			strncpy(dbs,(char*)optarg,strlen((char*)optarg));
			break;
		case 't':
			tdir=calloc(strlen((char*)optarg)+1,sizeof(char));
			strncpy(tdir,(char*)optarg,strlen((char*)optarg));
			break;
		case 'a':
			alfile=calloc(strlen((char*)optarg)+1,sizeof(char));
			strncpy(alfile,(char*)optarg,strlen((char*)optarg));
			break;
		case 'p': // port
			listenport=atoi(optarg);
			break;
		case 'n': // number of threads
			numthreads=atoi(optarg);
			break;
		case 'v': // verbose
			vlevel++;
			break;
		case 'h': // help
			usage(NULL,EXIT_SUCCESS);
			break;
		default: // fallthrough
			usage(NULL,EXIT_FAILURE);
		}
	}

	// validation
	if(dbs==NULL) {
		usage("Must give a db spec\n",EXIT_FAILURE);
	} else {
		LOG_DEBUG(vlevel, "Using database spec: %s\n",dbs);
	}

	if(alfile!=NULL) {
		if((tf=open(alfile,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))==-1) {
			LOG_FATAL(vlevel,"Unable to open access log file: %s: %s\n", alfile, strerror(errno));
			exit(EXIT_FAILURE);
		}
		close(tf);
	}
	if(alfile!=NULL && access(alfile, W_OK)!=0) {
		LOG_FATAL(vlevel, "Unable to write to access log file: %s\n",alfile);
		exit(EXIT_FAILURE);
	}

	if(listenport<0 || listenport>65536) {
		LOG_FATAL(vlevel, "Given port out of bounds: %i\n",listenport);
		exit(EXIT_FAILURE);
	}

	if(numthreads<0 || numthreads>99) {
		LOG_FATAL(vlevel, "Given threads out of bounds: %i\n",numthreads);
		exit(EXIT_FAILURE);
	}

	// templates
	LOG_DEBUG(vlevel,"Checking templates\n");
	if(tdir!=NULL) {
		LOG_DEBUG(vlevel, "Loading templates from %s\n", tdir);
		tmpldata=calloc(TMPL_ERROR+1,sizeof(char*));

		LOG_DEBUG(vlevel,"Looking for index file\n");
		tmpldata[TMPL_INDEX]=fmmap(tdir,"index");

		LOG_DEBUG(vlevel,"Looking for status file\n");
		tmpldata[TMPL_STATUS]=fmmap(tdir,"status");

		LOG_DEBUG(vlevel,"Looking for new file\n");
		tmpldata[TMPL_NEW]=fmmap(tdir,"new");

		LOG_DEBUG(vlevel,"Looking for list file\n");
		tmpldata[TMPL_LIST]=fmmap(tdir,"list");

		LOG_DEBUG(vlevel,"Looking for error file\n");
		tmpldata[TMPL_ERROR]=fmmap(tdir,"error");
	}

	if(tmpldata[TMPL_INDEX]==NULL) {
		tmpldata[TMPL_INDEX]=TMPL_INDEX_DFL;
	}
	if(tmpldata[TMPL_STATUS]==NULL) {
		tmpldata[TMPL_STATUS]=TMPL_STATUS_DFL;
	}
	if(tmpldata[TMPL_NEW]==NULL) {
		tmpldata[TMPL_NEW]=TMPL_NEW_DFL;
	}
	if(tmpldata[TMPL_LIST]==NULL) {
		tmpldata[TMPL_LIST]=TMPL_LIST_DFL;
	}
	if(tmpldata[TMPL_ERROR]==NULL) {
		tmpldata[TMPL_ERROR]=TMPL_ERROR_DFL;
	}

	// db mod setup
	dlerror(); // clear out the errors
	if(strstr(dbs,":")==NULL) {
		LOG_FATAL(vlevel,"Invalid database specification: %s\n", dbs);
		exit(EXIT_FAILURE);
	} else {
		int n=0;
		char *mn; 
		char *cf; 
		char *lf;

		lf=calloc(strlen(dbs),sizeof(char));
		while(dbs[n]!=':') {
			n++;
		}
		dbs[n]=0;
		mn=dbs;
		cf=dbs+n+1;

		sprintf(lf,"./libmod_%s.so",mn);

		dlh=dlopen(lf, RTLD_LAZY);
		if((dle=dlerror())!=NULL) {
			LOG_FATAL(vlevel, "dlopen() %s: %s\n", lf, dle);
			exit(EXIT_FAILURE);
		}
		free(lf);
		*(void**)(&db_init)=dlsym(dlh, "db_init");
		if((dle=dlerror())!=NULL) {
			LOG_FATAL(vlevel, "dlsym() db_init: %s\n",dle);
			exit(EXIT_FAILURE);
		}
		*(void**)(&db_select)=dlsym(dlh, "db_select");
		if((dle=dlerror())!=NULL) {
			LOG_FATAL(vlevel, "dlsym() db_select: %s\n",dle);
			exit(EXIT_FAILURE);
		}
		*(void**)(&db_insert)=dlsym(dlh, "db_insert");
		if((dle=dlerror())!=NULL) {
			LOG_FATAL(vlevel, "dlsym() db_insert: %s\n",dle);
			exit(EXIT_FAILURE);
		}
		*(void**)(&db_shutdown)=dlsym(dlh, "db_shutdown");
		if((dle=dlerror())!=NULL) {
			LOG_FATAL(vlevel, "dlsym() db_shutdown: %s\n",dle);
			exit(EXIT_FAILURE);
		}
		dlerror();
		LOG_DEBUG(vlevel, "Initializing database using config: %s\n", cf);
		if(db_init(&dbh, cf, vlevel)) {
			LOG_FATAL(vlevel,"Error initializing database\n");
			exit(EXIT_FAILURE);
		}
	}

	// set mgoptions - XXX this needs to be handled better
	lpstr=calloc(7,sizeof(char));
	snprintf(lpstr,6,"%i",listenport);

	ntstr=calloc(4,sizeof(char));
	snprintf(ntstr,3,"%i",numthreads);

	if(alfile!=NULL) {
		mgoptions = calloc(9,sizeof(char*));
	} else {
		mgoptions = calloc(7,sizeof(char*));
	}
	mgoptions[0]="listening_ports";
	mgoptions[1]=lpstr;
	mgoptions[2]="document_root";
	mgoptions[3]="/dev/null";
	mgoptions[4]="num_threads";
	mgoptions[5]=ntstr;
	if(alfile!=NULL) {
		mgoptions[6]="access_log_file";
		mgoptions[7]=alfile;
		mgoptions[8]=NULL;
	} else {
		mgoptions[6]=NULL;
	}
	// main loop
	LOG_DEBUG(vlevel, "Starting Mongoose HTTP server loop\n");
  ctx = mg_start(&mghandle, NULL, (const char**)mgoptions);
	if(ctx!=NULL) {
		while(!done) {
			// cleaner thread here?
			sleep(1);
		}
		LOG_DEBUG(vlevel, "Ending Mongoose HTTP server loop\n");
		mg_stop(ctx);
	} else {
		LOG_FATAL(vlevel,"Error in creating Mongoose HTTP server\n");
	}
	LOG_DEBUG(vlevel, "Closing database handle\n");
	db_shutdown(&dbh);

	LOG_DEBUG(vlevel, "Cleaning up\n");
	dlclose(dlh);
	free(dbs);
	free(lpstr);
	free(ntstr);
	free(mgoptions);
	free(tdir);
	free(tmpldata);
	
	return EXIT_SUCCESS;
}
