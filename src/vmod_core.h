#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "cache/cache.h"
#include "vre.h"
#include "vrt.h"
#include "vcl.h"
#include "vsha256.h"
#include "vcc_if.h"

struct vmod {
	ssize_t len;
	void *priv;
};

int concat_req_body(struct req *req, void *priv, void *ptr, size_t len);
void HSH_AddBytes(const struct req *req, const struct vrt_ctx *ctx,
const void *buf, size_t len);
ssize_t http1_iter_req_body(void *priv, int flush, const void *ptr, ssize_t len);
void VRB_Blob(VRT_CTX, struct vsb *vsb);
int VRB_Buffer(struct req *req, ssize_t maxsize);
