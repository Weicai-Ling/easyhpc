#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "easyhpc.h"

void* xmalloc (size_t size)
{
  void* ptr = malloc (size);
  /* Abort if the allocation failed.  */
  if (ptr == NULL)
    abort ();
  else
    return ptr;
}

void* xrealloc (void* ptr, size_t size)
{
  ptr = realloc (ptr, size);
  /* Abort if the allocation failed.  */
  if (ptr == NULL)
    abort ();
  else
    return ptr;
}

char* xstrdup (const char* s)
{
  char* copy = strdup (s);
  /* Abort if the allocation failed.  */
  if (copy == NULL)
    abort ();
  else
    return copy;
}

void system_error (const char* operation)
{
  /* Generate an error message for errno.  */
  error (operation, strerror (errno));
}

void error (const char* cause, const char* message)
{
  /* Print an error message to stderr.  */
  fprintf (stderr, "%s: error: (%s) %s\n", "easyhpc", cause, message);
  /* End the program.  */
  exit (1);
}


/**********************************************************************************
 * @author: Forrest Ling  
 * @brief: printf with string array *str_ary[] in format with %s
 ***********************************************************************************/
int prn_fmt_ary ( char * tar, char * fmt, char ** str_ary ) 
{
    int ret = 0;
    int off = 0;    // fmt off
    int i = 0;      // str_ary off

    while ( fmt[off] != '\0'  ) {
        if ( fmt[off] == '%' && fmt[off+1] == 's'  ) // if fmt has "%s"
        {  
            if ( str_ary[i] != NULL )  
                ret += sprintf ( tar + ret, "%s", str_ary[i++] );
            off += 2 ;
        } else {
            tar[ret++] = fmt[off++] ;
        }
    }
    tar[ret++] = '\0';
    return ret;
}


/**********************************************************************************
 * @author: Forrest Ling 
 * @brief: convert string into str array with seperator
 **********************************************************************************/
int str_to_ary ( char ** str_ary, char * str, char * sep ) 
{
    int i = 0;

    str_ary[i] = strtok (str, sep);
    while (str_ary[i] != NULL) {
        str_ary[++i] = strtok(NULL, sep);
    };
    return ++i; 
}

