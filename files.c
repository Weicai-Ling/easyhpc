#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "easyhpc.h"
#include "files.h"


struct Table menu_tbl;

/*** read file into buff ****/
char* read_file (const char* filename, size_t* length)
{
    int fd;
    struct stat file_info;
    char* buffer;
    fd = open (filename, O_RDONLY);
    fstat (fd, &file_info);

    *length = file_info.st_size + 1;
    if (!S_ISREG (file_info.st_mode)) {
        close (fd);
        return NULL;
    }
    buffer = (char*) malloc (*length);
    read (fd, buffer, *length - 1 );
    *(buffer + *length) = '\0';
    close (fd);
    return buffer;
}


/*** write file ***/
void write_file( char *filename, char * buffer )
{   
    int fd = open (filename, O_WRONLY | O_CREAT , 0666);
    write (fd, buffer, strlen(buffer) );
    close (fd);   
    return;
}

/**read_json_array**/
int jsonstr_to_tbl ( struct Table *tbl, char * str ) {

    char **h = calloc ( sizeof (char *), TBL_INDEX_MAX); 
    char *token;
    tbl->col = 0;
    tbl->row = 0;
    size_t len;
    int i = 0;

    token = strtok ( str , ","); 
    while ( token != NULL ) { 
        if ( strstr(token,"]" ) != NULL) 
            tbl->row++;
        // trim space, '[' and ']'         
        len = strlen(token); 
        // remove ']' and space from tail
        while( token[len - 1] == ']' || isspace(token[len - 1]) ) token[--len] = 0 ;
        // remove '[' amd space from head
        while(  *token == '[' || isspace(*token) ) ++token, --len;
        // remove " 
        if ( *token == '"' && *(token+1) == '"' ) 
            token = NULL; 
        else if ( *token =='"' && *(token+ len - 1 ) == '"' ) {
            token[len-1] = 0;
            ++token;
        }

        h[i++] = token;
        token = strtok (NULL, ",");
    }

    tbl->col = i / tbl->row;
    tbl->head = (char ** ) realloc(h, sizeof (char *) * (i+1) );

    return i;
}

void gen_menu_tbl ( char * json_ary_file ){
    size_t len;
    char * str = read_file ( json_ary_file, &len) ;
    int i = jsonstr_to_tbl ( &menu_tbl,  str ); 
}

/*** read page cfg file *******/
/* void read_page_cfg_file ( struct cfg_s * cfg, char * filename ) 
{
        struct cfg_s * ret;
        FILE *stream;
        char *line, *k,*v;
        size_t len;
        ssize_t nread;
        int i;

        if ( ( stream = fopen(filename, "r") ) == NULL ) {
                exit(EXIT_FAILURE);
                perror("fopen");
        }

        while ((nread = getline(&line, &len, stream)) != -1) { 
                // str ": " to seperate key/value;
                if ( line[0] == '#' || ( v = strstr (line, ": ")) == NULL )  
                        continue;
                *v = '\0';
                // strimString malloc memory with strdup(); 
                k = malloc_trimString ( line );
                // value position start two chars after ": "
                v = malloc_trim_rmdqoute ( v+2 ); 
                i = 0;
                while ( cfg[i].key != NULL ) {
                        if ( strcmp ( cfg[i].key, k ) == 0 ) { 
                                cfg[i].val = v;
                                break;
                        }
                        i++;
                };
                if (k) free(k);
        };

        free(line);
        fclose(stream);

} */

/******start position *******/
int start_line_pos ( char *str, int start_line, char sep) 
{
    int off = 0, row = 1;
    char s = (sep)?sep:'\n';

    while ( str[off] != '\0' && row < start_line ) {
        if ( str[off++] == s ) {
            row++;
        };
    };
    return off;
}

/***gen_data_ary**/
char ** malloc_data_ary ( char * str, int rows, int cols, char * col_sep )
{
    char **head;
    int i = 0;

    head = (char**) calloc (sizeof (char*), rows* cols + 1 ); 

    head[i] = strtok( str, col_sep );
    while ( head[i] != NULL && i < rows * cols ) {
                head[++i] = strtok( NULL, col_sep );
    };

    return head; 
}

/***malloc for two points to char ** and strdup **/
char ** malloc_strary ( char * str, char * sep )
{
#define ARY_MAX_ELE 64
    int i = 0;
    char **ret = (char**) calloc (sizeof (char*), ARY_MAX_ELE  );

    ret[i] = strtok( str, sep );
    while ( ret[i] != NULL && i < ARY_MAX_ELE - 1 ) 
        ret[++i] = strtok( NULL, sep );
    
    return (char **) realloc ((char ** ) ret, sizeof (char **) * ++i ); 
}

/*** conv_yaml_array*/ 
int snprint_json_ary ( char *json_ary, size_t size, char ** head, int rows, int cols )
{
    int off = 0;
    int row,col,i = 0; 

    off = snprintf( json_ary + off, size, "[\n" );
    for ( row = 0; ( row < rows ) && head[i] != NULL ; row++ ) { 
            off += snprintf( json_ary + off, size - off, " [" );
            for ( col = 0; col < cols; col++ ) {
                off += SNPRNT_JSONSTR(json_ary + off, size - off, head[i++]);
                off += snprintf( json_ary + off , size - off, "," );
            };
            off--;
            off += snprintf( json_ary + off , size - off, " ],\n" );
    };
    off -=2;
    off += snprintf( json_ary + off, size - off, "\n]\n" );    
    return off;
}


/***exec_cmd**
 * sep = 0 : text format 
 * sep = '\n' : one string ;
 * sep = ' ' : string array;
 * ***/
int snprint_exec_cmd ( char *out, size_t out_size, char ** cmd, char sep ) 
{
    char *cmd_black_list[] = { "rm","cp", "mkdir", NULL };
    int pipefd[2];
    pipe(pipefd);
    ssize_t nr;
    unsigned char buf;
    int off = 0, i = 0;
    int state;

    while ( cmd_black_list[i] != NULL ) {
        if ( strstr ( cmd[0], cmd_black_list[i++] ) != NULL ) {
            return 0;
        } 
    }; 

    // process cmd
    if (fork() == 0)
    {
        close(pipefd[0]);    // close reading end in the child

        dup2(pipefd[1], 1);  // send stdout to the pipe
        dup2(pipefd[1], 2);  // send stderr to the pipe

        close(pipefd[1]);    // this descriptor is no longer needed

        execv (cmd[0], cmd );  
        exit(0);
    } else {

        close(pipefd[1]);  // close the write end of the pipe in the parent

        switch ( sep ) {
            // sep = 0, to generate text format
            case '\0' : 
                while ( read (pipefd[0], &buf, 1 ) > 0 )  
                    out[off++] = buf;
                break;

            case '\n' :
                while ( read(pipefd[0], &buf, 1 ) > 0 ) { 
                    if ( buf == sep ) {
                        out[off++] = '\\'; 
                        out[off++] = 'n';
                    } else                 
                        out[off++] = buf;
                }
                break;

            // sep = ' ', blank to generate json string format
            case ' ' : 
                #define IN 0
                #define OUT 1
                state = OUT;
                out[off++] = '[', out[off++] = '\n', out[off++] = '[';
                while ( read(pipefd[0], &buf, 1 ) > 0 ) { 
                    if ( buf == '\n' ) {
                        if ( state == OUT ) { 
                            if ( out[off-1] == ',' ) off--;
                            out[off++] = ']'; 
                            out[off++] = ','; out[off++] = '\n'; 
                            out[off++] = '['; 
                        } else { 
                            state = OUT; 
                            out[off++] = '"', out[off++] = ']'; out[off++] = ',';out[off++] = '\n'; 
                            out[off++] = '[';
                        }

                    } else if ( buf == sep || buf == '\t' ) {
                        if ( state == IN ) {
                            state = OUT;
                            out[off++] = '"'; 
                            out[off++] = ',';
                        } 
                    } else {
                        if ( state == OUT) { 
                            state = IN;
                            out[off++] = '"'; 
                            out[off++] = buf;
                        } else     
                            out[off++] = buf;
                    }                
                }
/*                 while ( out[off--] != ',') ;
                off++; out[off++] = ']', out[off++] = '\n', out[off++] = ']';
 */             
                // last line , remove ",.*[", and adding ']' 
                if (out[off-1] == '[') off--;
                while ( out[off--] != ',') ;
                off++; 
                out[off++] = '\n', out[off++] = ']';
                break;
        }
        out[off] = '\0';
        return off;
    }
}

char *malloc_trimString(char *str)
{
    size_t len = strlen(str);

    while(isspace(str[len - 1])) --len;
    while(*str && isspace(*str)) ++str, --len;
        
    return strndup(str, len);
}

char *malloc_trim_rmdqoute (char *str)
{
    size_t len = strlen(str);

    while(isspace(str[len - 1])) --len;
    while(*str && isspace(*str)) ++str, --len;

    if ( *str == '"'  && str[len-1] == '"'  ) ++str, len -=2;     
    return strndup(str, len);
}

/*** read file in memory as a char** in table ****/
struct table_s * malloc_tbl ( char * filename, char ** thead, char * col_sep )
{
        struct table_s * tbl = malloc ( sizeof (struct table_s) );
        int cols = 0; 
        int rows;

        // calculate cols numbers
        while ( thead[cols++] != NULL ) ; 
        cols--;

        char ** lines = (char **) calloc ( sizeof (char *), MAXLINES + 1  );
        char ** tbody = (char **) calloc ( sizeof (char *),  (MAXLINES + 1 )* cols  );

        FILE *stream;
        size_t len = 0;
        ssize_t nread;
        int i,j;

        if ( ( stream = fopen( filename, "r") ) == NULL ) {
                exit(EXIT_FAILURE);
                perror("fopen");
        }
        
        // generate tbody refer to lines, which are required to free 
        i = 0;
        while ((nread = getline( &lines[i], &len, stream)) != -1 && i < MAXLINES ) { 
                if ( lines[i][0] == '#' ) 
                        continue;

                j = 0;
                tbody[i*cols + j] = strtok ( lines[i], col_sep); 
                while ( j < cols )  { 
                        tbody[i*cols + ++j] = strtok(NULL, col_sep );
                };

                i++;
                len = 0;
        };  
        fclose(stream);

        // trim space before/end of tbody strings
        rows = i;
        i = 0;
        while ( i < cols * rows ) {
                len = strlen(tbody[i]);
                while(isspace(tbody[i][len - 1])) tbody[i][--len] = '\0';
                while(*tbody[i] && isspace(*tbody[i])) ++(tbody[i]);
                i++;
        };

        tbl->lines = (char **) realloc ( lines, (rows + 1 ) * sizeof (char*) );
        tbl->tbody = (char **) realloc ( tbody, (rows + 1 ) * cols * sizeof (char *) ); 
        tbl->thead = thead;
        tbl->col_nr = cols;
        tbl->row_nr = rows; 

        return tbl;
}

void free_tbl ( struct table_s *tbl )
{
        int i = 0;
        if ( tbl ) { 
                while ( i < tbl->col_nr ) {
                        if (tbl->lines[i]) free ( tbl->lines[i] );
                        i++;
                }
                if (tbl->tbody) free(tbl->tbody);
                free (tbl);
        };
}