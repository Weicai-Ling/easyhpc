#include "easyhpc.h"
#include <stdio.h>
#include <string.h>

enum MHD_Result
post_iterator (void *cls,
               enum MHD_ValueKind kind,
               const char *key,
               const char *filename,
               const char *content_type,
               const char *transfer_encoding,
               const char *data, uint64_t off, size_t size)
{
  REQUEST_PTR request = cls;
  SESSION_PTR session = request->session;
  (void) kind;               /* Unused. Silent compiler warning. */
  (void) filename;           /* Unused. Silent compiler warning. */
  (void) content_type;       /* Unused. Silent compiler warning. */
  (void) transfer_encoding;  /* Unused. Silent compiler warning. */

  if (0 == strcmp ("DONE", key))
  {
    fprintf (stdout,
             "Session `%s' submitted `%s', `%s'\n",
             session->sid,
             session->value_1,
             session->value_2);
    return MHD_YES;
  }
  if (0 == strcmp ("v1", key))
  {
    if (size + off > sizeof(session->value_1))
      size = sizeof (session->value_1) - off;
    memcpy (&session->value_1[off],
            data,
            size);
    if (size + off < sizeof (session->value_1))
      session->value_1[size + off] = '\0';
    return MHD_YES;
  }
  if (0 == strcmp ("v2", key))
  {
    if (size + off > sizeof(session->value_2))
      size = sizeof (session->value_2) - off;
    memcpy (&session->value_2[off],
            data,
            size);
    if (size + off < sizeof (session->value_2))
      session->value_2[size + off] = '\0';
    return MHD_YES;
  }
  fprintf (stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}

