#include "vmod_core.h"
#include <sys/time.h>

void
HSH_AddBytes(const struct req *req, void *ctx, const void *buf, size_t len)
{
	CHECK_OBJ_NOTNULL(req, REQ_MAGIC);
	AN(ctx);

	if (buf != NULL)
		SHA256_Update(ctx, buf, len);
}


static int __match_proto__(req_body_iter_f)
IterCopyReqBody(struct req *req, void *priv, void *ptr, size_t l)
{
	struct vsb *iter_vsb = priv;
	CHECK_OBJ_NOTNULL(req, REQ_MAGIC);

	return (VSB_bcat(iter_vsb, ptr, l));
}

void
VRB_Blob(VRT_CTX, struct vmod *vmod)
{
	struct vsb *vsb;
	int l;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	vsb = VSB_new_auto();
	l = VRB_Iterate(ctx->req, IterCopyReqBody, (void*)vsb);
	VSB_finish(vsb);
	if (l < 0) {
		VSB_delete(vsb);
		VSLb(ctx->vsl, SLT_VCL_Error,
		    "Iteration on req.body didn't succeed.");
		return;
	}

	vmod->priv = VSB_data(vsb);
	vmod->len = VSB_len(vsb);
	VSB_delete(vsb);
}
