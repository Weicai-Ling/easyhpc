#ifndef EASYHPC
#define EASYHPC

#include <microhttpd.h>

/*** define pages  ***********************************************************/

#define METHOD_ERROR \
  "<html><head><title>Illegal request</title></head><body>Go away.</body></html>"
#define NOT_FOUND_ERROR \
  "<html><head><title>Not found</title></head><body>Go away.</body></html>"
#define MAIN_PAGE \
  "<html><head><title>Welcome</title></head><body><form action=\"/2\" method=\"post\">What is your name? <input type=\"text\" name=\"v1\" value=\"%s\" /><input type=\"submit\" value=\"Next\" /></body></html>"
#define SECOND_PAGE \
  "<html><head><title>Tell me more</title></head><body><a href=\"/\">previous</a> <form action=\"/S\" method=\"post\">%s, what is your job? <input type=\"text\" name=\"v2\" value=\"%s\" /><input type=\"submit\" value=\"Next\" /></body></html>"
#define SUBMIT_PAGE \
  "<html><head><title>Ready to submit?</title></head><body><form action=\"/F\" method=\"post\"><a href=\"/2\">previous </a> <input type=\"hidden\" name=\"DONE\" value=\"yes\" /><input type=\"submit\" value=\"Submit\" /></body></html>"
#define LAST_PAGE \
  "<html><head><title>Thank you</title></head><body>Thank you.</body></html>"

/*** sessions.c   *************************************************************/

typedef struct Session * SESSION_PTR;
typedef struct Session
{
  struct Session *next;
  char sid[33];
  unsigned int rc;
  time_t start;
  char value_1[64];
  char value_2[64];
} SESSION; 

extern SESSION_PTR sessions;

SESSION_PTR get_session (struct MHD_Connection *connection);
void add_session_cookie ( SESSION_PTR session, struct MHD_Response *response);
void expire_sessions ();


/*** request.c   *************************************************************/

typedef struct Request * REQUEST_PTR;
typedef struct Request
{
  SESSION_PTR session;
  struct MHD_PostProcessor *pp;
  const char *post_url;
} REQUEST;

/*** posts.c   *************************************************************/
enum MHD_Result
post_iterator (void *cls,
               enum MHD_ValueKind kind,
               const char *key,
               const char *filename,
               const char *content_type,
               const char *transfer_encoding,
               const char *data, uint64_t off, size_t size);


/*** pages.c   *************************************************************/

typedef enum MHD_Result (*PageHandler)(const void *cls,
                                       const char *mime,
                                       SESSION_PTR session,
                                       struct MHD_Connection *connection);

typedef struct Page * PAGE_PTR;
typedef struct Page
{
  const char *url;
  const char *mime;
  PageHandler handler;
  const void *handler_cls;
} PAGE;
extern PAGE pages[];

#endif /* EASYHPC */
