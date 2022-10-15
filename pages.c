#define _GNU_SOURCE /* See feature_test_macros(7) */
#include "easyhpc.h"
#include <string.h>
#include <stdio.h>

static enum MHD_Result
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

