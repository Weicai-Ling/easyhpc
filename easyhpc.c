#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "easyhpc.h"

static enum MHD_Result
create_response (void *cls,
                 struct MHD_Connection *connection,
                 const char *url,
                 const char *method,
                 const char *version,
                 const char *upload_data,
                 size_t *upload_data_size,
                 void **ptr)
{
  struct MHD_Response *response;
  REQUEST_PTR request;
  SESSION_PTR session;
  enum MHD_Result ret;
  unsigned int i;
  (void) cls;               /* Unused. Silent compiler warning. */
  (void) version;           /* Unused. Silent compiler warning. */

  printf ("\n>>>>>>>>>>>>>>>>>> url : %s\n",url);   

  if ( 0 == strncmp ( url, "/doc/", 5 )  || 
        0 == strncmp ( url + 1 , "index", 5 ) || 
        0 == strncmp ( url + 1 , "menu.json", 9 ) ) {
        ret = send_file ( url , connection ); 
        return ret;
  } else if ( 0 == strncmp ( url, "/api/", 3 )   ) {
        printf (" *****/api/:  %s \n", url); 
        ret = send_api_v2 ( url, connection );
        return ret;
  } else {
    printf ("\n no fetch in url \n" );   
  }

  request = *ptr;
  if (NULL == request)
  {
    request = calloc (1, sizeof (struct Request));
    if (NULL == request)
    {
      fprintf (stderr, "calloc error: %s\n", strerror (errno));
      return MHD_NO;
    }
    *ptr = request;
    if (0 == strcmp (method, MHD_HTTP_METHOD_POST))
    {
      request->pp = MHD_create_post_processor (connection, 1024,
                                               &post_iterator, request);
      if (NULL == request->pp)
      {
        fprintf (stderr, "Failed to setup post processor for `%s'\n",
                 url);
        return MHD_NO; /* internal error */
      }
    }
    return MHD_YES;
  }
  if (NULL == request->session)
  {
    request->session = get_session (connection);
    if (NULL == request->session)
    {
      fprintf (stderr, "Failed to setup session for `%s'\n",
               url);
      return MHD_NO; /* internal error */
    }
  }
  session = request->session;
  session->start = time (NULL);
  if (0 == strcmp (method, MHD_HTTP_METHOD_POST))
  {
    /* evaluate POST data */
    MHD_post_process (request->pp,
                      upload_data,
                      *upload_data_size);
    if (0 != *upload_data_size)
    {
      *upload_data_size = 0;
      return MHD_YES;
    }
    /* done with POST data, serve response */
    MHD_destroy_post_processor (request->pp);
    request->pp = NULL;
    method = MHD_HTTP_METHOD_GET;   /* fake 'GET' */
    if (NULL != request->post_url)
      url = request->post_url;
  }

  if ( (0 == strcmp (method, MHD_HTTP_METHOD_GET)) ||
       (0 == strcmp (method, MHD_HTTP_METHOD_HEAD)) )
  {
    /* find out which page to serve */
    i = 0;
    while ( (pages[i].url != NULL) &&
            (0 != strcmp (pages[i].url, url)) )
      i++;
    ret = pages[i].handler (pages[i].handler_cls,
                            pages[i].mime,
                            session, connection);
    if (ret != MHD_YES)
      fprintf (stderr, "Failed to create page for `%s'\n",
               url);
    return ret;
  }
  /* unsupported HTTP method */
  response = MHD_create_response_from_buffer (strlen (METHOD_ERROR),
                                              (void *) METHOD_ERROR,
                                              MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_NOT_ACCEPTABLE,
                            response);
  MHD_destroy_response (response);
  return ret;
}

static void
request_completed_callback (void *cls,
                            struct MHD_Connection *connection,
                            void **con_cls,
                            enum MHD_RequestTerminationCode toe)
{
  REQUEST_PTR request = *con_cls;
  (void) cls;         /* Unused. Silent compiler warning. */
  (void) connection;  /* Unused. Silent compiler warning. */
  (void) toe;         /* Unused. Silent compiler warning. */

  if (NULL == request)
    return;
  if (NULL != request->session)
    request->session->rc--;
  if (NULL != request->pp)
    MHD_destroy_post_processor (request->pp);
  free (request);
}

/**
 * Call with the port number as the only argument.
 * Never terminates (other than by signals, such as CTRL-C).
 */
int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;
  struct timeval tv;
  struct timeval *tvp;
  fd_set rs;
  fd_set ws;
  fd_set es;
  MHD_socket max;
  MHD_UNSIGNED_LONG_LONG mhd_timeout;

  if (argc != 2)
  {
    printf ("%s PORT\n", argv[0]);
    return 1;
  }

  ini_json_pages("pages.json");
 
  /* initialize PRNG */
  srand ((unsigned int) time (NULL));
  d = MHD_start_daemon (MHD_USE_ERROR_LOG,
                        atoi (argv[1]),
                        NULL, NULL,
                        &create_response, NULL,
                        MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 15,
                        MHD_OPTION_NOTIFY_COMPLETED,
                        &request_completed_callback, NULL,
                        MHD_OPTION_END);
  if (NULL == d)
    return 1;
  while (1)
  {
    expire_sessions ();
    max = 0;
    FD_ZERO (&rs);
    FD_ZERO (&ws);
    FD_ZERO (&es);
    if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
      break; /* fatal internal error */
    if (MHD_get_timeout (d, &mhd_timeout) == MHD_YES)
    {
      tv.tv_sec = mhd_timeout / 1000;
      tv.tv_usec = (mhd_timeout - (tv.tv_sec * 1000)) * 1000;
      tvp = &tv;
    }
    else
      tvp = NULL;
    if (-1 == select (max + 1, &rs, &ws, &es, tvp))
    {
      if (EINTR != errno)
        fprintf (stderr,
                 "Aborting due to error during select: %s\n",
                 strerror (errno));
      break;
    }
    MHD_run (d);
  }
  MHD_stop_daemon (d);
  return 0;
}
