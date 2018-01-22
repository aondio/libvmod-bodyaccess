#include "vmod_core.h"
#include <sys/time.h>

void
HSH_AddBytes(const struct req *req, const struct vrt_ctx *ctx,
    const void *buf, size_t len)
{
	CHECK_OBJ_NOTNULL(req, REQ_MAGIC);
	AN(ctx);

	if (buf != NULL)
		SHA256_Update(ctx->specific, buf, len);
}


static int v_matchproto_(req_body_iter_f)
IterCopyReqBody(void *priv, int flush, const void *ptr, ssize_t len)
{
	struct vsb *iter_vsb = priv;

	return (VSB_bcat(iter_vsb, ptr, len));
}

void
VRB_Blob(VRT_CTX, struct vsb *vsb)
{
	int l;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	l = VRB_Iterate(ctx->req, IterCopyReqBody, (void*)vsb);
	VSB_finish(vsb);
	if (l < 0) {
		VSB_delete(vsb);
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "Iteration on req.body didn't succeed.");
		return;
	}
}
