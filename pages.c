#define _GNU_SOURCE /* See feature_test_macros(7) */
#include "easyhpc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <search.h>
#include <ctype.h>

const char * pages_json = NULL; 

/** get value of a key in json object string , ***/
char * strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

char * malloc_json_kv ( const char * json_str, char *key, int len ) 
{
    char * ptr = strnstr ( json_str, (const char *) key, (size_t) len );
    if ( ptr == NULL ) return NULL;

    char * end;
    int i = 0;
    // first '"' is end of key, the second '"' is for the start of value 
    while ( i < 2 ) {
      if (*ptr++ == '"') i++;
    }
    // the third '"' is for the end of value
    end = strchr (ptr, '"'); 
    return strndup ( ptr, end - ptr );  
}

/*** root of tsearch create url_index binary tree *****/
void *url_index = NULL;
struct urlnode {
  const char * url;
  int url_len;
  const char * bgn;
  int len;
};

static int urlcompare(const void *pa, const void *pb)
{
    const struct urlnode * r  = pa;
    const struct urlnode * l  = pb;
    if ( strncmp ( r->url, l->url, l->url_len ) < 0 )
        return -1;
    if ( strncmp ( r->url, l->url, l->url_len ) > 0 )
        return 1;
    return 0;
}

static void urlaction(const void *nodep, const VISIT which, const int depth)
 {
     struct urlnode *datap;
     switch (which) {
     case preorder:
         break;
     case postorder:
         datap = *(struct urlnode **) nodep;
         printf("%s\n ", datap->url );
         printf ("json: %s \n", strndup (datap->bgn,datap->len) );
         break;
     case endorder:
         break;
     case leaf:
         datap = *(struct urlnode **) nodep;
         printf("%s\n", datap->url);
         printf ("json: %s \n", strndup (datap->bgn,datap->len) );
         break;
     }
 }

void gen_url_index ( const char *pgjson ) 
{
    const char *p = pgjson;
    struct urlnode * node;
    void *val;
    char * url_end;

    while ( *p++ != '\0' ) {
        if ( *p == '{' ) { 
          node = malloc( sizeof (struct urlnode) );

          node->bgn = p;
          while ( *p++ != '}') ;
          node->len = p - node->bgn ;
          node->url = (const char *) malloc_json_kv ( node->bgn ,"url", node->len );
          // to find the '<' at the url of easyhpc.cn/wwsh/node/print/<hostname> 
          if (( url_end = strrchr(node->url, '$' ) ) !=NULL )
              *url_end = '\0';   

          node->url_len = strlen( node->url );

          val = tsearch((void *) node, &url_index, urlcompare);
          if (val == NULL)
              exit(EXIT_FAILURE);
          else if ((*(struct urlnode **) val) != node)
              free(node);
        } 
    }
}

void ini_json_pages( char * pagesjson ){
    size_t len;
    pages_json = read_file ( pagesjson, &len );
    gen_url_index ( pages_json );
    twalk(url_index, urlaction); 
}

enum MHD_Result send_api_v2 ( const char * url, struct MHD_Connection *connection ) 
{
    enum MHD_Result ret;
    struct MHD_Response *res; 
    char **cmd = NULL;
    char *cmd_str;
    char * ptr;
    char * buf = NULL ;
    int len = 0; 
    int id;
    void *val;
    struct urlnode pagej ;

    if ( pages_json == NULL ) {
        buf = strdup ( "{\"output\" : \"did not find the url on file of page.json !\" }" ); 
        len = strlen(buf);
        goto END;        
    };

    pagej.url = strdup(url);
    
    val = tfind( (void *) &pagej, &url_index, urlcompare ); 

    if ( val == NULL ) { 
        buf = strdup ( "{ \"output\" : [\"did not find the url page!\" ] }" ); 
        len = strlen(buf);
    } else if ( (*(struct urlnode **) val) == &pagej ) { 
        buf = strdup ( "{ \"output\" : [ \"did not find the url on the url page adding with tsearch!\" ] }" ); 
        len = strlen(buf);    
    } else {
        pagej.bgn = (*(struct urlnode **)val)->bgn;
        pagej.len = (*(struct urlnode **)val)->len;

        cmd_str = malloc_json_kv ( pagej.bgn, "cmd", pagej.len );
        if ( cmd_str == NULL ) {
            buf = strdup ("{\"output\": [ \"did not find cmd in json.\" ] } ");
            len = strlen(buf); 
        } else {
            cmd = malloc_strary ( cmd_str , " ");
            int i = 0;
            while ( cmd[i] != NULL ) {
              if ( cmd[i][0] == '$' ) {
                // get url variable, for example get id from url/resources/id
                 cmd[i] = url + strlen ( (*(struct urlnode **)val)->url ); 
              }
              i++;
            } 
            // get url variable, for example get id from url/usrs/id
            i = 0;
            printf ("\n cmd end start \n" );
            while ( cmd[i] != NULL ) printf ( " $ %s ", cmd[i] ), i++ ;
            printf ("\n cmd end \n" );
            
            #define BUF_MAX 16*1024
            char *temp = (char *) calloc ( sizeof (char), BUF_MAX);
            len = 0;
            // copy the page json file
            len += snprintf ( temp + len, pagej.len, "%s", pagej.bgn ); 
            while ( temp[len--] != '\n' ) ;
            // copy the output 
            len += snprintf ( temp + len + 1, BUF_MAX - len, ",\n\"output\" :\n" );
            len += snprint_exec_cmd ( temp + len  , BUF_MAX -len , cmd, ' ' ) ; 
            len += snprintf ( temp + len, BUF_MAX - len, "\n}" );
            free (cmd);
            free ( cmd_str );
            buf = realloc (temp, len+1 );
        };    
    };

END:
    res = MHD_create_response_from_buffer ( len,
                                                (void *) buf,
                                                MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header (res,
                           MHD_HTTP_HEADER_CONTENT_TYPE,
                             "application/json"); 
    ret = MHD_queue_response (connection, MHD_HTTP_OK, res);
    MHD_destroy_response (res);
    return ret;
}


enum MHD_Result
serve_simple_form (const void *cls,
                   const char *mime,
                   SESSION_PTR session,
                   struct MHD_Connection *connection)
{
  enum MHD_Result ret;
  const char *form = cls;
  struct MHD_Response *response;

  /* return static form */
  response = MHD_create_response_from_buffer (strlen (form),
                                              (void *) form,
                                              MHD_RESPMEM_PERSISTENT);
  add_session_cookie (session, response);
  MHD_add_response_header (response,
                           MHD_HTTP_HEADER_CONTENT_ENCODING,
                           mime);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response (response);
  return ret;
}

static enum MHD_Result
fill_v1_form (const void *cls,
              const char *mime,
              SESSION_PTR session,
              struct MHD_Connection *connection)
{
  enum MHD_Result ret;
  const char *form = cls;
  char *reply;
  struct MHD_Response *response;

  if (-1 == asprintf (&reply,
                      form,
                      session->value_1))
  {
    /* oops */
    return MHD_NO;
  }
  /* return static form */
  response = MHD_create_response_from_buffer (strlen (reply),
                                              (void *) reply,
                                              MHD_RESPMEM_MUST_FREE);
  add_session_cookie (session, response);
  MHD_add_response_header (response,
                           MHD_HTTP_HEADER_CONTENT_ENCODING,
                           mime);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response (response);
  return ret;
}

static enum MHD_Result
fill_v1_v2_form (const void *cls,
                 const char *mime,
                 SESSION_PTR session,
                 struct MHD_Connection *connection)
{
  enum MHD_Result ret;
  const char *form = cls;
  char *reply;
  struct MHD_Response *response;

  if (-1 == asprintf (&reply,
                      form,
                      session->value_1,
                      session->value_2))
  {
    /* oops */
    return MHD_NO;
  }
  /* return static form */
  response = MHD_create_response_from_buffer (strlen (reply),
                                              (void *) reply,
                                              MHD_RESPMEM_MUST_FREE);
  add_session_cookie (session, response);
  MHD_add_response_header (response,
                           MHD_HTTP_HEADER_CONTENT_ENCODING,
                           mime);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response (response);
  return ret;
}

static enum MHD_Result
not_found_page (const void *cls,
                const char *mime,
                SESSION_PTR session,
                struct MHD_Connection *connection)
{
  enum MHD_Result ret;
  struct MHD_Response *response;
  (void) cls;     /* Unused. Silent compiler warning. */
  (void) session; /* Unused. Silent compiler warning. */

  /* unsupported HTTP method */
  response = MHD_create_response_from_buffer (strlen (NOT_FOUND_ERROR),
                                              (void *) NOT_FOUND_ERROR,
                                              MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_NOT_FOUND,
                            response);
  MHD_add_response_header (response,
                           MHD_HTTP_HEADER_CONTENT_ENCODING,
                           mime);
  MHD_destroy_response (response);
  return ret;
}

PAGE pages[] = {
  { "/", "text/html",  &fill_v1_form, MAIN_PAGE },
  { "/2", "text/html", &fill_v1_v2_form, SECOND_PAGE },
  { "/S", "text/html", &serve_simple_form, SUBMIT_PAGE },
  { "/F", "text/html", &serve_simple_form, LAST_PAGE },
  { NULL, NULL, &not_found_page, NULL }   /* 404 */
};

enum MHD_Result send_file ( const char * url,  struct MHD_Connection *connection)
{ 
  enum MHD_Result ret;
  struct MHD_Response *response;
  int fd;
  struct stat buf;

  fd = open ( url + 1, O_RDONLY );
  if (-1 != fd)
  {
    if ( (0 != fstat (fd, &buf)) ||
         (! S_ISREG (buf.st_mode)) )
    {
      /* not a regular file, refuse to serve */
      if (0 != close (fd))
        abort ();
      fd = -1;
    }
  }
  if (-1 == fd)
  {
#define FILE_NO_FOUND  "<html><head><title>File not found</title></head><body>File not found</body></html>"
    response = MHD_create_response_from_buffer (strlen (FILE_NO_FOUND),
                                                (void *) FILE_NO_FOUND,
                                                MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response (response);
  }
  else
  {
    response = MHD_create_response_from_fd64 (buf.st_size, fd);
    if (NULL == response)
    {
      if (0 != close (fd))
        abort ();
      return MHD_NO;
    }
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
  }
  return ret;
}
/* 
enum MHD_Result send_api ( const char * url, struct MHD_Connection *connection ) 
{
    enum MHD_Result ret;
    struct MHD_Response *res; 
    char **cmd = NULL;
    char * buf;
    int len; 
    int id;
    char * cmd_str;

    id = atoi ( MHD_lookup_connection_value (connection, MHD_HEADER_KIND, "id"));
    cmd_str = strdup (menu_tbl.head[id * menu_tbl.col + 3]);
    cmd = malloc_strary ( cmd_str, " ");
#define BUF_MAX 16*1024
    buf = (char *) malloc (BUF_MAX);

    len = snprint_exec_cmd ( buf, BUF_MAX, cmd ) ;
    free (cmd);
    free (cmd_str);
    len += snprintf ( buf + len , BUF_MAX - len, "\n %s %d", url, id ) ;

    res = MHD_create_response_from_buffer ( len,
                                                (void *) realloc (buf, len + 1 ),
                                                MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header (res,
                           MHD_HTTP_HEADER_CONTENT_TYPE,
                           "text/html");

    ret = MHD_queue_response (connection, MHD_HTTP_OK, res);
    MHD_destroy_response (res);
    return ret;
}
 */