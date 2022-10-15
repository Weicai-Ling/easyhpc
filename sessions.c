#include "easyhpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define COOKIE_NAME "session"

SESSION_PTR sessions;

SESSION_PTR
get_session (struct MHD_Connection *connection)
{
  SESSION_PTR ret;
  const char *cookie;

  cookie = MHD_lookup_connection_value (connection,
                                        MHD_COOKIE_KIND,
                                        COOKIE_NAME);
  if (cookie != NULL)
  {
    /* find existing session */
    ret = sessions;
    while (NULL != ret)
    {
      if (0 == strcmp (cookie, ret->sid))
        break;
      ret = ret->next;
    }
    if (NULL != ret)
    {
      ret->rc++;
      return ret;
    }
  }
  /* create fresh session */
  ret = calloc (1, sizeof ( SESSION ));
  if (NULL == ret)
  {
    fprintf (stderr, "calloc error: %s\n", strerror (errno));
    return NULL;
  }
  /* not a super-secure way to generate a random session ID,
     but should do for a simple example... */
  snprintf (ret->sid,
            sizeof (ret->sid),
            "%X%X%X%X",
            (unsigned int) rand (),
            (unsigned int) rand (),
            (unsigned int) rand (),
            (unsigned int) rand ());
  ret->rc++;
  ret->start = time (NULL);
  ret->next = sessions;
  sessions = ret;
  return ret;
}

void
add_session_cookie ( SESSION_PTR session,
                    struct MHD_Response *response)
{
  char cstr[256];
  snprintf (cstr,
            sizeof (cstr),
            "%s=%s",
            COOKIE_NAME,
            session->sid);
  if (MHD_NO ==
      MHD_add_response_header (response,
                               MHD_HTTP_HEADER_SET_COOKIE,
                               cstr))
  {
    fprintf (stderr,
             "Failed to set session cookie header!\n");
  }
}

void
expire_sessions ()
{
  SESSION_PTR pos;
  SESSION_PTR prev;
  SESSION_PTR next;
  time_t now;

  now = time (NULL);
  prev = NULL;
  pos = sessions;
  while (NULL != pos)
  {
    next = pos->next;
    if (now - pos->start > 60 * 60)
    {
      /* expire sessions after 1h */
      if (NULL == prev)
        sessions = pos->next;
      else
        prev->next = next;
      free (pos);
    }
    else
      prev = pos;
    pos = next;
  }
}
