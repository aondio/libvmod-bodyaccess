#include "vmod_core.h"
#include "vsb.h"

void
RESP_Blob(const struct vrt_ctx *ctx, struct vsb *vsb) {
	struct storage *st;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	if (!ctx->req->obj->gziped) {
		VTAILQ_FOREACH(st, &ctx->req->obj->store, list) {
			CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
			CHECK_OBJ_NOTNULL(st, STORAGE_MAGIC);

			VSB_bcat(vsb, st->ptr, st->len);
		}
	} else {
		struct vgz *vg;
		const void *dp;
		size_t dl;
		vg = VGZ_NewUngzip(ctx->req->vsl, "U D -");
		VTAILQ_FOREACH(st, &ctx->req->obj->store, list) {
			CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
			CHECK_OBJ_NOTNULL(st, STORAGE_MAGIC);
			VGZ_Ibuf(vg, st->ptr, st->len);
			do {
				VGZ_Obuf(vg, VSB_data(vsb), vsb->s_size);
					VGZ_Gunzip(vg, &dp, &dl);

			} while (!VGZ_IbufEmpty(vg));
		}
		VGZ_Destroy(&vg);
	}
	return;
}

VCL_INT
vmod_len_resp_body(VRT_CTX)
{
	struct vsb *vsb;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);

	vsb = VSB_new_auto();
	RESP_Blob(ctx, vsb);
	VSB_finish(vsb);

	return VSB_len(vsb);
}

VCL_INT
vmod_rematch_resp_body(VRT_CTX, struct vmod_priv *priv_call, VCL_STRING re,
    VCL_INT limit)
{
	struct vsb *vsb;
	const char *error;
	int erroroffset;
	vre_t *t = NULL;
	int i;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(re);

	if(priv_call->priv == NULL) {
		t =  VRE_compile(re, 0, &error, &erroroffset);

		if(t == NULL) {
			VSLb(ctx->vsl, SLT_VCL_Error,
			    "Regular expression not valid");
			return (-1);
		}

		priv_call->priv = t;
		priv_call->free = free;

	}

	cache_param->vre_limits.match = limit;
	cache_param->vre_limits.match_recursion = limit;
	vsb = VSB_new_auto();
	RESP_Blob(ctx, vsb);
	VSB_finish(vsb);

	i = VRE_exec(priv_call->priv, VSB_data(vsb), VSB_len(vsb), 0, 0,
	    NULL, 0, &cache_param->vre_limits);

	if (i > 0)
		return (1);

	if (i == VRE_ERROR_NOMATCH )
		return (0);

	VSLb(ctx->vsl, SLT_VCL_Error, "Regexp matching returned %d", i);
		return (-1);

}

