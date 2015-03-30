#include "vmod_core.h"
#include <sys/time.h>

double
VTIM_real(void)
{
#ifdef HAVE_CLOCK_GETTIME
        struct timespec ts;

        AZ(clock_gettime(CLOCK_REALTIME, &ts));
        return (ts.tv_sec + 1e-9 * ts.tv_nsec);
#else
        struct timeval tv;

        AZ(gettimeofday(&tv, NULL));
        return (tv.tv_sec + 1e-6 * tv.tv_usec);
#endif
}

ssize_t
http1_iter_req_body(struct req *req, enum req_body_state_e bs,
    void *buf, ssize_t len)
{
	const char *err;

	CHECK_OBJ_NOTNULL(req, REQ_MAGIC);

	AN(len);
	AN(buf);
	if (bs == REQ_BODY_PRESENT) {
		AN(req->req_bodybytes);
		if (len > req->req_bodybytes - req->h1.bytes_done)
			len = req->req_bodybytes - req->h1.bytes_done;
		if (len == 0) {
			req->req_body_status = REQ_BODY_DONE;
			return (0);
		}
		len = HTTP1_Read(req->htc, buf, len);
		if (len <= 0) {
			req->req_body_status = REQ_BODY_FAIL;
			return (-1);
		}
		req->h1.bytes_done += len;
		req->h1.bytes_yet = req->req_bodybytes - req->h1.bytes_done;
		req->acct.req_bodybytes += len;
		return (len);
	} else if (bs == REQ_BODY_CHUNKED) {
		switch (HTTP1_Chunked(req->htc, &req->chunk_ctr, &err,
		    &req->acct.req_bodybytes, buf, &len)) {
		case H1CR_ERROR:
			VSLb(req->vsl, SLT_Debug, "CHUNKERR: %s", err);
			return (-1);
		case H1CR_MORE:
			return (len);
		case H1CR_END:
			return (0);
		default:
			WRONG("invalid HTTP1_Chunked return");
		}
	} else
		WRONG("Illegal req_bodystatus");
}

int
VRB_Buffer(struct req *req, ssize_t maxsize)
{
	struct storage *st;
	ssize_t l, l2;

	CHECK_OBJ_NOTNULL(req, REQ_MAGIC);

	assert (req->req_step == R_STP_RECV);
	switch(req->req_body_status) {
	case REQ_BODY_CACHED:
	case REQ_BODY_FAIL:
		return (-1);
	case REQ_BODY_NONE:
		return (0);
	case REQ_BODY_CHUNKED:
	case REQ_BODY_PRESENT:
		break;
	default:
		WRONG("Wrong req_body_status");
	}

	if (req->req_bodybytes > maxsize) {
		req->req_body_status = REQ_BODY_FAIL;
		req->doclose = SC_RX_BODY;
		return (-1);
	}

	l2 = 0;
	st = NULL;
	do {
		if (st == NULL) {
			st = STV_alloc_transient(
			    req->h1.bytes_yet ?
			    req->h1.bytes_yet : cache_param->fetch_chunksize);
			if (st == NULL) {
				req->req_body_status = REQ_BODY_FAIL;
				l = -1;
				break;
			} else {
				VTAILQ_INSERT_TAIL(&req->body, st, list);
			}
		}

		l = st->space - st->len;
		l = http1_iter_req_body(req, req->req_body_status,
		    st->ptr + st->len, l);

		if (req->req_body_status == REQ_BODY_CHUNKED)
			req->req_bodybytes += l;

		if (l < 0) {
			req->doclose = SC_RX_BODY;
			break;
		}
		if (req->req_bodybytes > maxsize) {
			req->req_body_status = REQ_BODY_FAIL;
			req->doclose = SC_RX_BODY;
			STV_free(st);
			STV_close();
			l = -1;
			break;
		}
		if (l > 0) {
			l2 += l;
			st->len += l;
			if (st->space == st->len)
				st = NULL;
		}
	} while (l > 0);
	if (l == 0) {
		req->req_bodybytes = l2;
		http_Unset(req->http0, H_Content_Length);
		http_Unset(req->http0, H_Transfer_Encoding);
		http_PrintfHeader(req->http0, "Content-Length: %ju",
		    (uintmax_t)req->req_bodybytes);

		http_Unset(req->http, H_Content_Length);
		http_Unset(req->http, H_Transfer_Encoding);
		http_PrintfHeader(req->http, "Content-Length: %ju",
		    (uintmax_t)req->req_bodybytes);

		req->req_body_status = REQ_BODY_CACHED;
	}
	VSLb_ts_req(req, "ReqBody", VTIM_real());
	return (l);
}


void
HSH_AddBytes(const struct req *req, const void *buf, size_t len)
{
	CHECK_OBJ_NOTNULL(req, REQ_MAGIC);
	AN(req->sha256ctx);

	if (buf != NULL)
		SHA256_Update(req->sha256ctx, buf, len);
}

int
concat_req_body(struct req *req, void *priv, void *ptr, size_t len)
{
	struct ws *ws = priv;
	(void)req;

	return (!WS_Copy(ws, ptr, len));
}

void
VRB_Blob(VRT_CTX, struct vmod *vmod)
{
	struct vmod *p;
	char *ws_f;
	ssize_t l;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	if (vmod->priv) {
		return;
	}

	if (ctx->req->req_body_status != REQ_BODY_CACHED){
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "Uncached req.body");
		return;
	}

	assert(ctx->req->req_body_status == REQ_BODY_CACHED);
	p =(void*)WS_Alloc(ctx->ws, sizeof *p);
	AN(p);
	vmod->priv = p;

	ws_f = WS_Snapshot(ctx->ws);
	AN(ws_f);
	l = HTTP1_IterateReqBody(ctx->req, concat_req_body, ctx->ws);

	if (l < 0 || WS_Copy(ctx->ws, "\0", 1) == NULL) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "Iteration on req.body didn't succeed.");
		WS_Reset(ctx->ws, ws_f);
		memset(p, 0, sizeof *p);
		vmod->len = -1;
		return;
	}

	vmod->len = ctx->req->req_bodybytes;
	vmod->priv = ws_f;
	return;
}
