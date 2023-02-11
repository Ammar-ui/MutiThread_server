#ifndef __WEBPRJAPP_H__
#define __WEBPRJAPP_H__

#define VERSION    24
#define BUFSIZE  8096
#define ERROR      40
#define MSG1       41
#define MSG2       42
#define MSG3       43
#define MSG4       44
#define MSG5       45
#define MSG6       46
#define MSG7       47
#define MSG8       48
#define FORBIDDEN 403
#define NOTFOUND  404
#define SERVERROR 503
#define FIFO      0
#define SFF       1
#define SFFBS     2
#define CRF       3
#define BLCK      0
#define DRPT      1
#define DRPH      2
#define DRPR      3
#define BLCK_STR "BLCK"
#define DRPT_STR "DRPT"
#define DRPH_STR "DRPH"


/* ===========================================================================
 * Define a structure for supported file extensions
 * ===========================================================================
 */
typedef struct {
    char *ext;
    char *filetype;
} ext_t;
static ext_t extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"ico", "image/ico" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tgz", "image/tgz" },
    {"tar", "image/tar" },
    {"rar", "image/rar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"txt", "text/txt"  },
    {"pdf","application/pdf"},
    {"doc","application/doc"},
    {0,0} };

/* logs ========================================================================
 * This function is used to send error messages to the client browesr and
 * to the log file. It is also used to write all the activities of the server
 * to the same log file. The default log file is named "web-server.log" and
 * located in the root directory of the web site.
 *
 * Input Parameters:
 *  type - Message type, one of: ERROR,FORBIDDEN,NOTFOUND,SERVERROR,MSG1..MSG8.
 *  req -- Request ID about which the message is produced (an integer).
 *  s1 --- The error/log message (a string).
 *  s2 --- Additional information about the cause of the message (a string).
 *  num -- Additional number to ID entity related to the message (an integer).
 *
 * Depending on the message type, the following actions are performed:
 *  ERROR: "ERROR:", s1, s2, error# & proc.ID logged.
 *  FORBIDDEN: causes two actions:
 *      1. A "Forbidden" web page containing s1 and s2 is sent to req (clnt-fd).
 *      2. "FORBIDEN:", req, num, s1 and s2 are logged.
 *  NOTFOUND:  causes two actions:
 *      1. A "Not Found" web page containing s1 and s2 is sent to req (clnt-fd).
 *      2. "NOTFOUND:", req, num, s1 and s2 are logged.
 *  SERVERROR: causes four actions:
 *      1. Request (req - to be dropped) is rea d from the client.
 *      2. A "Server Error" web page containing s1 and s2 sent to req (clnt-fd).
 *      3. "SRVERROR:", num, s1, s2 and req are logged.
 *      4. The connection with the client at req is closed.
 * Types MSG1 to MSG8 give wide choices for where to place message parameters:
 *  MSG1:  "LOG:", s1 and s2 only are logged.
 *  MSG2:  "LOG:", s1, s2 and num only are logged.
 *  MSG3:  "LOG:", s1, num and s2 only are logged
 *  MSG4:  "LOG:", num, s1 and s2 only are logged
 *  MSG5:  "LOG:", req, num, s1 and s2 are logged.
 *  MSG6:  "LOG:", s1, req, s2 and num are logged.
 *  MSG7:  "LOG:", req, s1, num and s2 are logged.
 *  MSG8:  "LOG:", req, s1, s2 and num are logged.
 *
 * When logging is done or errors are encountered, function returns normally.
 * =============================================================================
 */
void logs(int type, int req, char *s1, char *s2, int num);


/* serve =====================================================================
 * This function does all the work needed to get and serve an HTTP request:
 *    1. Read the HTTP input string from the given client's file descriptor.
 *    2. Parse through the input string (the request from the browser).
 *    3. Check for errors in the request.
 *    4. Extract the path, file name and extension.
 *    5. Send a response header to the browser.
 *    6. Send the requested file.
 *
 * Input Parameters:
 *  sfd -- The file descriptor for the client process (the browser).
 *  hit -- The hit counter. Used as identifier for logging only.
 *
 * When errors are encountered, this function logs a FORBIDDEN or a NOTFOUND
 * message and returns.  When work is done, this function returns normally.
 * ===========================================================================
 */
void serve(int sfd, int hit);


#endif /* __WEBPRJAPP_H__ */
