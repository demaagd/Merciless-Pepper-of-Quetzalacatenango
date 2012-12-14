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
#include <leveldb/c.h>
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
#include "mongoose.h"

int done=0;
int vlevel=0;

leveldb_t *dbh;
leveldb_readoptions_t *ropt;
leveldb_writeoptions_t *wopt;
char *errptr;	  

void usage(char *err, int ec) {
  if(err!=NULL) {
    fprintf(stderr,_("Error: %s\n\n"),err);
  }
  
  fprintf(stderr,_("Usage (cosd v%i.%i.%i):\n"),cosd_VERSION_MAJOR,cosd_VERSION_MINOR,cosd_VERSION_REV);
  fprintf(stderr,_(" -d database dir        -- Specifies database connection to use, module:/path/to/file\n"));
  fprintf(stderr,_(" -p port spec           -- Port nuber to listen on, passed directly to mongoose HTTP library\n"));
  fprintf(stderr,_(" -n N                   -- Number of HTTP serving threads (default: 10)\n"));
  fprintf(stderr,_(" -a /path/to/accessfile -- Access log file, must be writable (if it exists) or in a writable dir (if it does not exist, it will be created)\n"));
  fprintf(stderr,_(" -v                     -- Increases verbose level, can be specified multiple times\n"));
  fprintf(stderr,_(" -h                     -- This help listing\n"));
	
  exit(ec);
}

void handlesig(int sig) {
	if(sig==SIGTERM || sig==SIGINT) {
		if(done) {
			exit(EXIT_FAILURE);
		} else {
			LOG_DEBUG(vlevel, _("Finishing...\n"));
			done=1; 
		}
	}
}

static void *mghandle(enum mg_event event, struct mg_connection *conn) {
  const struct mg_request_info *request_info = mg_get_request_info(conn);
  if (event == MG_NEW_REQUEST) {
    char *req=calloc(URL_STRING_MAX+1,sizeof(char));
    struct in_addr saddr;
    
    strncpy(req,request_info->uri, URL_STRING_MAX);
    saddr.s_addr = ntohl(request_info->remote_ip);
    
    LOG_DEBUG(vlevel, _("Connection from: %s, request: %s\n"), inet_ntoa(saddr), req);
    if(strncmp(req, "/status\0", 8) == 0) { 
      // XXX how to get more status 
      mg_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %d\r\n"        
		"\r\n"
		"OK\r\n",
		4);
    } else if(strncmp(req, "/set/", 5) == 0) { 
      int n=strlen(req);
      while(n) {
	if(req[n]==':') {
	  leveldb_put(dbh, wopt, req+5, n-5, req+n+1, strlen(req)-n-1, &errptr);
	  if(errptr!=NULL) {
	    LOG_ERROR(vlevel,_("leveldb_put(): %s\n"),errptr);
	    mg_printf(conn,
		      "HTTP/1.1 500 OK\r\n"
		      "Content-Type: text/plain\r\n"
		      "Content-Length: %d\r\n"        
		      "\r\n"
		      "ERROR: %s\r\n",
		      9+strlen(errptr), errptr);
	  } else {
	    mg_printf(conn,
		      "HTTP/1.1 200 OK\r\n"
		      "Content-Type: text/plain\r\n"
		      "Content-Length: %d\r\n"        
		      "\r\n"
		      "OK\r\n",
		      4);
	  }
	  break;
	}
	n--;
      }
      if(n==0) {
	LOG_ERROR(vlevel,_("Malformed request\n"));
	mg_printf(conn,
		  "HTTP/1.1 500 OK\r\n"
		  "Content-Type: text/plain\r\n"
		  "Content-Length: %d\r\n"        
		  "\r\n"
		  "MALFORMED\r\n",
		  11);
      }
    } else if(strncmp(req, "/get/", 5) == 0) { 
      size_t rlen=-1;
      char *tmp=leveldb_get(dbh, ropt, req+5, strlen(req)-5, &rlen, &errptr);
      if(rlen) {
	char *val=calloc(rlen+2,sizeof(char));
	snprintf(val,rlen+1,"%s",tmp);
	LOG_DEBUG(vlevel, _("Found: %s for %s\n"),val,req+5);	      
	mg_printf(conn,
		  "HTTP/1.1 200 OK\r\n"
		  "Content-Type: text/plain\r\n"
		  "Content-Length: %d\r\n"        
		  "\r\n"
		  "%s\r\n",
		  strlen(val)+2, val);
	free(val);
      } else {
	LOG_DEBUG(vlevel, _("Nothing found for %s\n"),req+5);
	mg_printf(conn,
		  "HTTP/1.1 500 OK\r\n"
		  "Content-Type: text/plain\r\n"
		  "Content-Length: %d\r\n"        
		  "\r\n"
		  "NOTFOUND\r\n",
		  10);
      } 
      free(tmp);
    } else if(strncmp(req, "/pset/", 6) == 0) { 
      // XXX
    } else if(strncmp(req, "/pget/", 6) == 0) { 
      // XXX
    } else { 
      // XXX
      LOG_ERROR(vlevel,_("Unknown/unhandled request\n"));
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
  
  char *dbd=NULL;
  char *lpstr=NULL;
  char *ntstr=NULL;
  char *alfile=NULL;

  leveldb_options_t *dbopt;

  struct mg_context *ctx= NULL; 
  char **mgoptions;
  
  signal(SIGINT,handlesig);
  signal(SIGTERM,handlesig);
  
  // command line parsing
  while ((goopt=getopt (argc, argv, "d:p:n:a:t:vh")) != -1) {
    switch (goopt) {
    case 'd': // database 
      dbd=calloc(strlen((char*)optarg)+1,sizeof(char));
      strncpy(dbd,(char*)optarg,strlen((char*)optarg));
      break;
    case 'a': // access log, passed to mongoose
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
  
  LOG_TRACE(vlevel, _("Validation\n"),dbd);
  if(dbd==NULL) {
    usage("Must give a database dir\n",EXIT_FAILURE);
  } else {
    LOG_TRACE(vlevel, _("Using database dir: %s\n"),dbd);
  }
  
  if(alfile!=NULL) {
    if((tf=open(alfile,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))==-1) {
      LOG_FATAL(vlevel,_("Unable to open access log file: %s: %s\n"), alfile, strerror(errno));
      exit(EXIT_FAILURE);
    }
    close(tf);
  }
  if(alfile!=NULL && access(alfile, W_OK)!=0) {
    LOG_FATAL(vlevel, _("Unable to write to access log file: %s\n"),alfile);
    exit(EXIT_FAILURE);
  }
  
  if(listenport<0 || listenport>65536) {
    LOG_FATAL(vlevel, _("Given port out of bounds: %i\n"),listenport);
    exit(EXIT_FAILURE);
  }
  
  if(numthreads<0 || numthreads>1024) {
    LOG_FATAL(vlevel, _("Given threads out of bounds: %i\n"),numthreads);
    exit(EXIT_FAILURE);
  }

  LOG_TRACE(vlevel, _("Setting up leveldb store in %s\n"),dbd);
  dbopt=leveldb_options_create();
  leveldb_options_set_create_if_missing(dbopt, 1);
  leveldb_options_set_write_buffer_size(dbopt, 8388608);
  leveldb_options_set_compression(dbopt,leveldb_no_compression);
  dbh=leveldb_open(dbopt,dbd,&errptr);

  LOG_TRACE(vlevel, _("Setting leveldb read options\n"));
  ropt = leveldb_readoptions_create();
  leveldb_readoptions_set_verify_checksums(ropt, 1);
  leveldb_readoptions_set_fill_cache(ropt, 0);

  LOG_TRACE(vlevel, _("Setting leveldb write options\n"));
  wopt = leveldb_writeoptions_create();
  leveldb_writeoptions_set_sync(wopt, 0);

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
  LOG_INFO(vlevel, _("Starting Mongoose HTTP server loop\n"));
  ctx = mg_start(&mghandle, NULL, (const char**)mgoptions);
  if(ctx!=NULL) {
    while(!done) {
      // cleaner thread here?
      sleep(1);
    }
    LOG_INFO(vlevel, _("Ending Mongoose HTTP server loop\n"));
    mg_stop(ctx);
  } else {
    LOG_FATAL(vlevel,_("Error in creating Mongoose HTTP server\n"));
  }
  LOG_TRACE(vlevel, _("Closing database handle\n"));

  // close leveldb handle
  LOG_TRACE(vlevel, _("Cleaning up leveldb\n"));
  leveldb_compact_range(dbh, NULL, 0, NULL, 0);

  leveldb_options_destroy(dbopt);
  leveldb_readoptions_destroy(ropt);
  leveldb_writeoptions_destroy(wopt);
  leveldb_close(dbh);

  LOG_TRACE(vlevel, _("Cleaning up\n"));
  free(dbd);
  free(lpstr);
  free(ntstr);
  free(mgoptions);
  
  return EXIT_SUCCESS;
}
