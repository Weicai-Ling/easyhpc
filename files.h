#ifndef FILES
#define FILES

#define TBL_INDEX_MAX 4*1024
struct Table {
    char ** head;
    int col;
    int row;
}; 
extern struct Table menu_tbl;

char ** malloc_strary ( char * str, char * sep );
int snprint_exec_cmd ( char *out, size_t out_size, char ** cmd, char sep ) ;
#endif    /* FILES */
