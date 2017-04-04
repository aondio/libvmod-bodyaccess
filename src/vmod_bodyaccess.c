#include "vmod_core.h"

VCL_VOID
vmod_hash_req_body(VRT_CTX)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	struct vsb *vsb;

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

	vsb = VSB_new_auto();

	VRB_Blob(ctx, vsb);
	HSH_AddBytes(ctx->req, ctx, VSB_data(vsb),  VSB_len(vsb));

	VSB_delete(vsb);
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
	const char *error;
	struct vsb *vsb;
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

	vsb = VSB_new_auto();
	VRB_Blob(ctx, vsb);

	i = VRE_exec(priv_call->priv, VSB_data(vsb), VSB_len(vsb), 0, 0,
	    NULL, 0, NULL);

	VSB_delete(vsb);

	if (i > 0)
		return (1);

	if (i == VRE_ERROR_NOMATCH )
		return (0);

	VSLb(ctx->vsl, SLT_VCL_Error, "Regexp matching returned %d", i);
		return (-1);

}

VCL_STRING
vmod_req_body(VRT_CTX)
{
	struct vsb *vsb;
	char *s;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	if (ctx->req->req_body_status != REQ_BODY_CACHED) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		   "Unbuffered req.body");
		return (NULL);
	}

	if (ctx->method != VCL_MET_RECV) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "rematch_req_body can be used only in vcl_recv{}");
		return (NULL);
	}

	vsb = VSB_new_auto();
	VRB_Blob(ctx, vsb);
	s = WS_Alloc(ctx->ws, VSB_len(vsb) + 1);
	if (s == NULL) {
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "workspace can't hold size of req_body (%ld bytes)", VSB_len(vsb));
		VSB_delete(vsb);
		return (NULL);
	}
	memcpy(s, VSB_data(vsb), VSB_len(vsb));
	s[VSB_len(vsb)] = 0;
	VSB_delete(vsb);
	return (s);
}
