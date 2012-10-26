#include "util.h"

#define TMPL_INDEX_DFL _("<HTML><HEAD><TITLE>urlshortd shortener thingy</TITLE></HEAD><BODY><FORM ACTION='/n/' METHOD='GET'> URI: <INPUT TYPE='TEXT'> <INPUT TYPE='SUBMIT' VALUE='Submit'></FORM></BODY></HTML>")
#define TMPL_STATUS_DFL "{STATUS}"
#define TMPL_NEW_DFL _("<HTML><HEAD><TITLE>urlshortd shortener thingy</TITLE></HEAD><BODY>New link submitted: {ULINK} -> {RLINK}</BODY></HTML> ")
#define TMPL_LIST_DFL _("<HTML><HEAD><TITLE>urlshortd shortener thingy</TITLE></HEAD><BODY>{LIST}</BODY></HTML>")
#define TMPL_ERROR_DFL _("<HTML><HEAD><TITLE>urlshortd shortener thingy</TITLE></HEAD><BODY>Something has gone terribly wrong, we're all gonna die!{MESSAGE}: {CODE}</BODY></HTML>")
