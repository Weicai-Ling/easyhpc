#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <stdio.h>
#include <sys/wait.h> 
#include <unistd.h>
#include <errno.h> 
#include <fcntl.h>
#include "easyhpc.h"

#define CMD_PAR_MAX 64
#define URL_LEN_MAX 256
#define CMD_OUT_MAX 4*1024
#define SEND_MAX 32*1024

enum DIV_CARD {
    BEGI=0,
    HEAD=1,
    BODY=2,
    FOOT=3
};
char * tag_card[] = {
    "<div class=\"card\">",
    "<div class=\"card-header\">",
    "<div class=\"card-body\">", 
    "<div class=\"card-footer\">"
};

char * exe_cmd ( char ** cmd ) 
{
    char * ret, *ret_final;
    int pipefd[2];
    pipe(pipefd);
    off_t off = 0;
    size_t nr = 0;

    ret = (char*) calloc (CMD_OUT_MAX, sizeof (char) );
//    char* cmd[] = { "/bin/ls", "-l", NULL }; 

    if (fork() == 0)
    {
        close(pipefd[0]);    // close reading end in the child

        dup2(pipefd[1], 1);  // send stdout to the pipe
        dup2(pipefd[1], 2);  // send stderr to the pipe

        close(pipefd[1]);    // this descriptor is no longer needed

        execv (cmd[0], cmd); 
        system_error("execv");
    }
    else
    {
    // parent
        close(pipefd[1]);  // close the write end of the pipe in the parent
        nr = read(pipefd[0], ret, CMD_OUT_MAX - 1 ) ;
    }
    ret_final = (char * ) realloc ( (char*) ret, nr + 1 );
    *(ret_final + nr ) = '\0';
    return ret_final;
}

void url2cmd ( char *urlcp, char **cmd ) 
{
    int i = 0;
    cmd[i] = strtok ( urlcp , "_" ); // cmd seperator is "_"
    while ( cmd[i] != NULL && i < CMD_PAR_MAX )  {
        cmd[++i] = strtok(NULL, "_");
    };
    if ( i = CMD_PAR_MAX ) 
        cmd[i-1] = NULL; 
}

enum MHD_Result send_cmd ( const char * url, 
                    const char *mime,    SESSION_PTR session,
                    struct MHD_Connection *connection)
{      
    char *ptr;
    char * reply;
    char *cmd_output;
    off_t off = 0;
    struct MHD_Response *response;
    enum MHD_Result ret;

    char *cmd[CMD_PAR_MAX] = {NULL};
    char urlcp[URL_LEN_MAX] = {0};

    strncpy (urlcp, url, ( URL_LEN_MAX > strlen (url))? (strlen(url)):(URL_LEN_MAX -1) );
    url2cmd (urlcp, cmd ); 

    reply = (char *) xmalloc ( SEND_MAX ); 

    off += snprintf ( reply + off, SEND_MAX - off, "<div class=\"card\"><div class=\"card-header\">" );
    int i = 0;
    while ( cmd[i]!= NULL && i < CMD_PAR_MAX ) 
        off += snprintf ( reply + off, SEND_MAX - off, "%s",   cmd[i++] );

    // url format "/usr/bin/cmd_xxx_xxx" , cmd[0] is number of id, cmd array start from cmd[1]
    cmd_output = exe_cmd ( cmd );  

    off += snprintf ( reply + off, SEND_MAX - off, "</div><div class=\"card-body\"><pre>%s",  cmd_output ); 

    off += snprintf ( reply + off, SEND_MAX - off, "</pre></div></div>" ); 
    
    ptr = realloc ( reply, off + 1 ) ; 

    response = MHD_create_response_from_buffer (strlen (ptr),
                                                (void *) ptr,
                                                MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header (response,
                            MHD_HTTP_HEADER_CONTENT_ENCODING,
                            "text/html");
    ret = MHD_queue_response (connection,
                                MHD_HTTP_OK,
                                response);
    MHD_destroy_response (response);

    if ( cmd_output ) free ( cmd_output) ;

    return ret;
}

enum MHD_Result send_json ( const char * url, 
                    const char *mime,    SESSION_PTR session,
                    struct MHD_Connection *connection)
{      
    char *reply, *ptr;
    struct MHD_Response *response;
    enum MHD_Result ret;
    off_t off = 0;
    char *buf;
    size_t len = 0;

    ptr = (char *) xmalloc ( SEND_MAX ); 
/*     printf ("before read_file \n");
    buf = read_file( "doc/ajax/table_tag.html", &len ) ;
    printf ("after read_file table tag \n");
    off += snprintf ( ptr + off, SEND_MAX - off, "{ \"html\": \"%s\"," , buf);
    printf ("after sprintf table tag \n");

    if ( buf ) free (buf);
    len = 0; */
    buf = read_file( "doc/ajax/array.json", &len ) ;
    off += snprintf ( ptr + off, SEND_MAX - off, "%s" , buf); 
    if ( buf ) free (buf);
    
    reply = realloc ( ptr, off + 1 ) ; 

    printf ("\n******* json table + data : \n %s\n", reply ); 

    response = MHD_create_response_from_buffer (strlen (reply),
                                                (void *) reply,
                                                MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header (response,
                            MHD_HTTP_HEADER_CONTENT_ENCODING,
                            "application/json" );
    ret = MHD_queue_response (connection,
                                MHD_HTTP_OK,
                                response);
    MHD_destroy_response (response);

    return ret;
}
