#include "vmod_core.h"

VCL_VOID
vmod_hash_req_body(VRT_CTX)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	struct vmod_priv priv_top = { 0 };

	if (ctx->req->req_body_status != REQ_BODY_CACHED) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		   "Unbuffered req.body");
		return;
	}

	if (ctx->method != VCL_MET_HASH) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "hash_req_body can only be used in vcl_hash{}");
		return;
	}

	VRB_Blob(ctx, &priv_top);
	HSH_AddBytes(ctx->req, ctx, VSB_data(priv_top.priv),  priv_top.len);
	VSB_delete(priv_top.priv);
}

VCL_INT
vmod_len_req_body(VRT_CTX)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	if (ctx->req->req_body_status != REQ_BODY_CACHED) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		   "Unbuffered req.body");
		return (-1);
	}

	if (ctx->method != VCL_MET_RECV) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "len_req_body can only be used in vcl_recv{}");
		return (-1);
	}

	return (ctx->req->req_bodybytes);
}

VCL_INT
vmod_rematch_req_body(VRT_CTX, struct vmod_priv *priv_call, VCL_STRING re)
{
	struct vmod_priv priv_top = { 0 };
	const char *error;
	int erroroffset;
	vre_t *t = NULL;
	int i;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (ctx->req->req_body_status != REQ_BODY_CACHED) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		   "Unbuffered req.body");
		return(-1);
	}

	if (ctx->method != VCL_MET_RECV) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "rematch_req_body can be used only in vcl_recv{}");
		return (-1);
	}

	AN(re);

	if(priv_call->priv == NULL) {
		t =  VRE_compile(re, 0, &error, &erroroffset);
		if (t == NULL) {
			VSLb(ctx->vsl, SLT_VCL_Error,
			    "Regular expression not valid");
			return (-1);
		}
		priv_call->priv = t;
		priv_call->free = free;

	}

	VRB_Blob(ctx, &priv_top);

	i = VRE_exec(priv_call->priv, VSB_data(priv_top.priv), priv_top.len, 0, 0,
	    NULL, 0, NULL);

	VSB_delete(priv_top.priv);

	if (i > 0)
		return (1);

	if (i == VRE_ERROR_NOMATCH )
		return (0);

	VSLb(ctx->vsl, SLT_VCL_Error, "Regexp matching returned %d", i);
		return (-1);

}

struct log_req_body {
	const char *prefix;
	size_t len;
};

static int __match_proto__(req_body_iter_f)
IterLogReqBody(struct req *req, void *priv, void *ptr, size_t len)
{
	struct log_req_body *lrb;
	txt txtbody;
	const char *str;
	char *buf;
	size_t size, prefix_len;

	CHECK_OBJ_NOTNULL(req, REQ_MAGIC);

	lrb = priv;
	str = ptr;

	if (lrb->len > 0)
		size = lrb->len;
	else
		size = len;
	prefix_len = strlen(lrb->prefix);
	size += prefix_len;

	buf = malloc(size);
	AN(buf);

	while (len > 0) {
		if (len > lrb->len && lrb->len > 0)
			size = lrb->len;
		else
			size = len;

		memcpy(buf, lrb->prefix, prefix_len);
		memcpy(buf + prefix_len, str, size);

		txtbody.b = buf;
		txtbody.e = buf + prefix_len + size;

		VSLbt(req->vsl, SLT_Debug, txtbody);

		len -= size;
		str += size;
	}

	free(buf);

	return (0);
}

VCL_VOID
vmod_log_req_body(VRT_CTX, VCL_STRING prefix, VCL_INT length)
{
	struct log_req_body lrb;
	int ret;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	if (!prefix)
		prefix = "";

	lrb.prefix = prefix;
	lrb.len = length;

	if (ctx->req->req_body_status != REQ_BODY_CACHED) {
		VSLb(ctx->vsl, SLT_VCL_Error, "Unbuffered req.body");
		return;
	}

	ret = VRB_Iterate(ctx->req, IterLogReqBody, &lrb);

	if (ret < 0) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "Iteration on req.body didn't succeed.");
		return;
	}
}
