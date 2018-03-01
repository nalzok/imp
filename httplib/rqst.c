#include <stdlib.h>
#include <string.h>

#include "rqst.h"


struct Request *parse_request(const char *raw) {
    struct Request *req = NULL;
    req = malloc(sizeof *req);
    if (!req) {
        return NULL;
    }
    memset(req, 0, sizeof *req);

    // Method
    size_t method_len = strcspn(raw, " ");
    if (strncmp(raw, S_GET, sizeof S_GET - 1) == 0) {
        req->method = GET;
    } else if (strncmp(raw, S_HEAD, sizeof S_HEAD - 1) == 0) {
        req->method = HEAD;
    } else {
        req->method = UNSUPPORTED;
    }
    raw += method_len;
    raw += strspn(raw, " "); // move past <SP>

    // Request-URI
    size_t url_len = strcspn(raw, " ");
    if ((req->url = malloc(url_len + 1)) == NULL) {
        return free_request(req);
    }
    memcpy(req->url, raw, url_len);
    req->url[url_len] = '\0';
    raw += url_len;
    raw += strspn(raw, " "); // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    if ((req->http_ver = malloc(ver_len + 1)) == NULL) {
        return free_request(req);
    }
    memcpy(req->http_ver, raw, ver_len);
    req->http_ver[ver_len] = '\0';
    raw += ver_len;
    raw += strspn(raw, "\r\n"); // move past <CR><LF>

    // Headers
    req->headers = parse_headers(&raw);
    raw += strspn(raw, "\r\n"); // move past <CR><LF>

    // Body
    size_t body_len = strlen(raw);
    if ((req->body = malloc(body_len + 1)) == NULL) {
        return free_request(req);
    }
    memcpy(req->body, raw, body_len);
    req->body[body_len] = '\0';


    return req;
}


struct Headers *parse_headers(const char **raw_ptr) {
    const char *raw = *raw_ptr;
    struct Headers *h;

    if ((h = malloc(sizeof *h)) == NULL) {
        return free_headers(h);
    }
    memset(h, 0, sizeof *h);

    struct F_OT *last_ot = NULL;
    while (raw[0]!='\r' || raw[1]!='\n') {

        /*
         * name
         */
        size_t name_len = strcspn(raw, ": ");
        char *fname;
        if ((fname = malloc(name_len + 1)) == NULL) {
            return free_headers(h);
        }
        memcpy(fname, raw, name_len);
        fname[name_len] = '\0';

        char **fvalue;
        if      (strcmp(fname, S_CT_CC) == 0) fvalue = &h->ct.cc;
        else if (strcmp(fname, S_CT_EP) == 0) fvalue = &h->ct.ep;
        else if (strcmp(fname, S_CT_HO) == 0) fvalue = &h->ct.ho;
        else if (strcmp(fname, S_CT_MF) == 0) fvalue = &h->ct.mf;
        else if (strcmp(fname, S_CT_PG) == 0) fvalue = &h->ct.pg;
        else if (strcmp(fname, S_CT_RG) == 0) fvalue = &h->ct.rg;
        else if (strcmp(fname, S_CT_TE) == 0) fvalue = &h->ct.te;
        else if (strcmp(fname, S_CD_IM) == 0) fvalue = &h->cd.im;
        else if (strcmp(fname, S_CD_NM) == 0) fvalue = &h->cd.nm;
        else if (strcmp(fname, S_CD_MS) == 0) fvalue = &h->cd.ms;
        else if (strcmp(fname, S_CD_US) == 0) fvalue = &h->cd.us;
        else if (strcmp(fname, S_CD_IR) == 0) fvalue = &h->cd.ir;
        else if (strcmp(fname, S_CN_AA) == 0) fvalue = &h->cn.aa;
        else if (strcmp(fname, S_CN_AC) == 0) fvalue = &h->cn.ac;
        else if (strcmp(fname, S_CN_AE) == 0) fvalue = &h->cn.ae;
        else if (strcmp(fname, S_CN_AL) == 0) fvalue = &h->cn.al;
        else if (strcmp(fname, S_AC_AU) == 0) fvalue = &h->ac.au;
        else if (strcmp(fname, S_AC_PA) == 0) fvalue = &h->ac.pa;
        else if (strcmp(fname, S_RC_FM) == 0) fvalue = &h->rc.fm;
        else if (strcmp(fname, S_RC_RF) == 0) fvalue = &h->rc.rf;
        else if (strcmp(fname, S_RC_UA) == 0) fvalue = &h->rc.ua;
        else {
            last_ot = h->ot;
            if ((h->ot = malloc(sizeof *h->ot)) == NULL ||
                (h->ot->name = malloc(name_len + 1)) == NULL) {
                return free_headers(h);
            }
            strcpy(h->ot->name, fname);
            h->ot->next = last_ot;
            fvalue = &h->ot->value;
        }
        raw += name_len;
        raw += strspn(raw, ": "); // move past :<SP>


        /*
         * value
         */
        size_t value_len = strcspn(raw, "\r\n");
        if ((*fvalue = malloc(value_len + 1)) == NULL) {
            return free_headers(h);
        }
        memcpy(*fvalue, raw, value_len);
        (*fvalue)[value_len] = '\0';
        raw += value_len;

        // move past one <CR><LF>, one <CR>, or one <LF>
        if (raw[0] == '\r') raw++;
        if (raw[0] == '\n') raw++;
    }

    *raw_ptr = raw;

    return h;
}


void free_headers_ct(struct F_CT *ct) {
    free(ct->cc); free(ct->ep); free(ct->ho); free(ct->mf);
    free(ct->pg); free(ct->rg); free(ct->te);
}

void free_headers_cd(struct F_CD *cd) {
    free(cd->im); free(cd->nm); free(cd->ms); free(cd->us);
    free(cd->ir);
}

void free_headers_cn(struct F_CN *cn) {
    free(cn->aa); free(cn->ac); free(cn->ae); free(cn->al);
}

void free_headers_ac(struct F_AC *ac) {
    free(ac->au); free(ac->pa);
}

void free_headers_rc(struct F_RC *rc) {
    free(rc->fm); free(rc->rf); free(rc->ua);
}

void free_headers_ot(struct F_OT *ot) {
    if (ot) {
        free(ot->name);
        free(ot->value);
        free_headers_ot(ot->next);
        free(ot);
    }
}

void *free_headers(struct Headers *hds) {
    if (hds) {
        free_headers_ct(&hds->ct);
        free_headers_cd(&hds->cd);
        free_headers_cn(&hds->cn);
        free_headers_ac(&hds->ac);
        free_headers_rc(&hds->rc);
        free_headers_ot(hds->ot);
        free(hds);
    }
    return NULL;
}


void *free_request(struct Request *req) {
    if (req) {
        free(req->url);
        free(req->http_ver);
        free_headers(req->headers);
        free(req->body);
        free(req);
    }
    return NULL;
}