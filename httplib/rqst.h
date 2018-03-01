#ifndef IMP_HTTPLIB_H
#define IMP_HTTPLIB_H


typedef enum Method {UNSUPPORTED, GET, HEAD} Method;

#define S_GET "GET"
#define S_HEAD "HEAD"


#define S_CT_CC     "Cache-Control"
#define S_CT_EP     "Expect"
#define S_CT_HO     "Host"
#define S_CT_MF     "Max-Forwards"
#define S_CT_PG     "Pragma"
#define S_CT_RG     "Range"
#define S_CT_TE     "TE"
#define S_CD_IM     "If-Match"
#define S_CD_NM     "If-None-Match"
#define S_CD_MS     "If-Modified-Since"
#define S_CD_US     "If-Unmodified-Since"
#define S_CD_IR     "If-Range"
#define S_CN_AA     "Accept"
#define S_CN_AC     "Accept-Charset"
#define S_CN_AE     "Accept-Encoding"
#define S_CN_AL     "Accept-Language"
#define S_AC_AU     "Authorization"
#define S_AC_PA     "Proxy-Authorization"
#define S_RC_FM     "From"
#define S_RC_RF     "Referer"
#define S_RC_UA     "User-Agent"


typedef struct F_CT {
    /* Fields: Controls */

    char *cc;       /* Cache-Control */
    char *ep;       /* Expect */
    char *ho;       /* Host */
    char *mf;       /* Max-Forwards */
    char *pg;       /* Pragma */
    char *rg;       /* Range */
    char *te;       /* TE */
} F_CT;


typedef struct F_CD {
    /* Fields: Conditionals */

    char *im;       /* If-Match */
    char *nm;       /* If-None-Match */
    char *ms;       /* If-Modified-Since */
    char *us;       /* If-Unmodified-Since */
    char *ir;       /* If-Range */
} F_CD;


typedef struct F_CN {
    /* Fields: Content Negotiation */

    char *aa;       /* Accept */
    char *ac;       /* Accept-Charset */
    char *ae;       /* Accept-Encoding */
    char *al;       /* Accept-Language */
} F_CN;


typedef struct F_AC {
    /* Fields: Authentication Credentials */

    char *au;       /* Authorization */
    char *pa;       /* Proxy-Authorization */
} F_AC;


typedef struct F_RC {
    /* Fields: Request Context */

    char *fm;       /* From */
    char *rf;       /* Referer */
    char *ua;       /* User-Agent */
} F_RC;


typedef struct F_OT {
    /* Fields: Other */

    char *name;
    char *value;
    struct F_OT *next;
} F_OT;


typedef struct Headers {
    struct F_CT ct;
    struct F_CD cd;
    struct F_CN cn;
    struct F_AC ac;
    struct F_RC rc;
    struct F_OT *ot;
} Headers;


typedef struct Request {
    enum Method method;
    char *url;
    char *http_ver;
    struct Headers *headers;
    char *body;
} Request;


struct Request *parse_request(const char *raw);
struct Headers *parse_headers(const char **raw_ptr);
void free_headers_ct(struct F_CT *ct);
void free_headers_cd(struct F_CD *cd);
void free_headers_cn(struct F_CN *cn);
void free_headers_ac(struct F_AC *ac);
void free_headers_rc(struct F_RC *rc);
void free_headers_ot(struct F_OT *ot);
void *free_headers(struct Headers *hds);
void *free_request(struct Request *req);


#endif //IMP_HTTPLIB_H