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
#include <glib.h>
#include <json/json.h> 
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
#include <zlib.h>

#include "config.h"
#include "util.h"
#include "mongoose.h"

int done=0;
int vlevel=0;

typedef struct {
	char **active;
	char **failed;
} bucket;

GThreadPool *senderpool;
struct bucket *bucketlist;

void usage(char *err, int ec) {
  if(err!=NULL) {
    fprintf(stderr,_("Error: %s\n"),err);
    fprintf(stderr,"\n");
  }
  
  fprintf(stderr,_("Usage (v%i.%i.%i):\n"),cskvs_VERSION_MAJOR,cskvs_VERSION_MINOR,cskvs_VERSION_REV);
  fprintf(stderr,_(" -p port spec           -- Port nuber to listen on, passed directly to mongoose HTTP library\n"));
  fprintf(stderr,_(" -a /path/to/accessfile -- Access log file, must be writable (if it exists) or in a writable dir (if it does not exist, it will be created)\n"));
  fprintf(stderr,_(" -t N                   -- Number of HTTP threads\n"));
  fprintf(stderr,_(" -T N                   -- Number of storage threads\n"));
  fprintf(stderr,_(" -s storage map         -- Storage mapping\n"));
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

static void storagesender(void *data, void *user_data) {
	LOG_TRACE(vlevel,_("Pool worker starting...\n"));
}

static void *mghandle(enum mg_event event, struct mg_connection *conn) {
  const struct mg_request_info *request_info = mg_get_request_info(conn);
  if (event == MG_NEW_REQUEST) {
    char *req=calloc(URL_STRING_MAX+1,sizeof(char));
    struct in_addr saddr;
    
    strncpy(req,request_info->uri, URL_STRING_MAX);
    saddr.s_addr = ntohl(request_info->remote_ip);
    
    LOG_DEBUG(vlevel, _("Connection from: %s, request: %s\n"), inet_ntoa(saddr), req);
    if(strncmp(req, "/status\0", 8) == 0) { // status
      // XXX how to get more status 
      mg_printf(conn,
								"HTTP/1.1 200 OK\r\n"
								"Content-Type: text/plain\r\n"
								"Content-Length: 4\r\n"
								"\r\n"
								"OK\r\n");
    } else if(strncmp(req, "/meta/", 6) == 0) { 

    } else if(strncmp(req, "/set/", 5) == 0) { 

    } else if(strncmp(req, "/get/", 5) == 0) {

    } else if(strncmp(req, "/mset/\0", 7) == 0) {

    } else if(strncmp(req, "/mget/\0", 7) == 0) {

    } else { // other
      LOG_ERROR(vlevel,_("Unknown/unhandled request\n"));
			mg_printf(conn,
								"HTTP/1.1 500 ERROR\r\n"
								"Content-Type: text/plain\r\n"
								"Content-Length: 9\r\n"
								"\r\n"
								"UNKNOWN\r\n");
    }
    free(req);
  } else {
    return NULL;
  }
}

int main(int argc, char **argv) {
  int goopt;
  int listenport=8079;
  int numhttpthreads=10;
  int numstoragethreads=10;
  int tf;
  
  char *lpstr=NULL;
  char *ntstr=NULL;
  char *alfile=NULL;
	char *bucketmapstr=NULL;
	char *ts;

  struct mg_context *ctx= NULL; 
  char **mgoptions;

  signal(SIGINT,handlesig);
  signal(SIGTERM,handlesig);
  
  // command line parsing
  while ((goopt=getopt (argc, argv, "p:a:t:s:vh")) != -1) {
    switch (goopt) {
    case 'a': // access log, passed to mongoose
      alfile=calloc(strlen((char*)optarg)+1,sizeof(char));
      strncpy(alfile,(char*)optarg,strlen((char*)optarg));
      break;
    case 'p': // port
      listenport=atoi(optarg);
      break;
    case 't': // number of HTTP threads
      numhttpthreads=atoi(optarg);
			break;
    case 'T': // number of storage threads
      numstoragethreads=atoi(optarg);
			break;
    case 's': // bucket/storage map
      bucketmapstr=calloc(strlen((char*)optarg)+1,sizeof(char));
      strncpy(bucketmapstr,(char*)optarg,strlen((char*)optarg));
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
  
  if(numhttpthreads<0 || numhttpthreads>1024) {
    LOG_FATAL(vlevel, _("Given HTTP threads out of bounds: %i\n"),numhttpthreads);
    exit(EXIT_FAILURE);
  }

  if(numstoragethreads<0 || numstoragethreads>1024) {
    LOG_FATAL(vlevel, _("Given storage threads out of bounds: %i\n"),numstoragethreads);
    exit(EXIT_FAILURE);
  }

	// 
	if(bucketmapstr!=NULL) {
		bucketlist=calloc(BUCKETS,sizeof(bucket));
		if(strstr((const char*)bucketmapstr, ",")) { // is a csv
			char *cptr;
			char *tres;
			tres=strtok_r(bucketmapstr, ",", &cptr);
			while(tres!=NULL) {
				int tlen=strlen(tres);
				int spos=-1;
				int cpos=-1;
				int n=tlen-1;

				LOG_TRACE(vlevel,_("Mapping token: %s\n"),tres);

				while(n>0) {
					if(tres[n]=='/') {
						spos=n;
						n--;
						break;
					}
					n--;
				}
				while(n>0) {
					if(tres[n]==':') {
						cpos=n;
						n--;
						break;
					}
					n--;
				}
				if(cpos>-1 && spos>-1) {
					LOG_TRACE(vlevel,_("Mapping token: %s colon: %i slash %i\n"),tres, cpos, spos);
				} else {
					LOG_FATAL(vlevel, _("Malformed bucket mapping: %s\n"),tres);
					exit(EXIT_FAILURE);
				}
				
				tres=strtok_r(NULL, ",", &cptr);
			}

		} else { // single element
			if(strstr((const char*)bucketmapstr, ":") && strstr((const char*)bucketmapstr, "/")) { 

				
			} else {
				LOG_FATAL(vlevel, _("Malformed bucket mapping: %s\n"),bucketmapstr);
				exit(EXIT_FAILURE);
			}
		}
	} else {
    LOG_FATAL(vlevel, _("No bucket mapping given\n"));
    exit(EXIT_FAILURE);
	}
	exit(1);

  // set mgoptions - XXX this needs to be handled better
  lpstr=calloc(7,sizeof(char));
  snprintf(lpstr,6,"%i",listenport);
  
  ntstr=calloc(5,sizeof(char));
  snprintf(ntstr,4,"%i",numhttpthreads);
  
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

	LOG_INFO(vlevel, _("Creating sender pool\n"));
	senderpool=g_thread_pool_new(storagesender,NULL,numstoragethreads,1,NULL);

	g_thread_pool_push(senderpool,"foo",NULL);
  
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



  LOG_TRACE(vlevel, _("Cleaning up\n"));
  free(lpstr);
  free(ntstr);
  free(mgoptions);
  free(bucketmapstr);
  
  return EXIT_SUCCESS;
}
