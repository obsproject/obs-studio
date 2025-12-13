#include "nvenc-internal.h"
#include "nvenc-helpers.h"

/*
 * NVENC implementation using CUDA context and OpenGL textures
 */

void cuda_opengl_free(struct nvenc_data *enc)
{
	if (!enc->cu_ctx)
		return;

	cu->cuCtxPushCurrent(enc->cu_ctx);
	for (size_t i = 0; i < enc->input_textures.num; i++) {
		CUgraphicsResource res_y = enc->input_textures.array[i].res_y;
		CUgraphicsResource res_uv = enc->input_textures.array[i].res_uv;
		cu->cuGraphicsUnregisterResource(res_y);
		cu->cuGraphicsUnregisterResource(res_uv);
	}
	cu->cuCtxPopCurrent(NULL);
}

/* ------------------------------------------------------------------------- */
/* Actual encoding stuff                                                     */

static inline bool get_res_for_tex_ids(struct nvenc_data *enc, GLuint tex_id_y, GLuint tex_id_uv,
				       CUgraphicsResource *tex_y, CUgraphicsResource *tex_uv)
{
	bool success = true;

	for (size_t idx = 0; idx < enc->input_textures.num; idx++) {
		struct handle_tex *ht = &enc->input_textures.array[idx];
		if (ht->tex_id != tex_id_y)
			continue;

		*tex_y = ht->res_y;
		*tex_uv = ht->res_uv;
		return success;
	}

	CU_CHECK(cu->cuGraphicsGLRegisterImage(tex_y, tex_id_y, GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY))
	CU_CHECK(cu->cuGraphicsGLRegisterImage(tex_uv, tex_id_uv, GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY))

	struct handle_tex ht = {tex_id_y, *tex_y, *tex_uv};
	da_push_back(enc->input_textures, &ht);

unmap:
	if (!success) {
		cu->cuGraphicsUnregisterResource(*tex_y);
		cu->cuGraphicsUnregisterResource(*tex_uv);
	}

	return success;
}

static inline bool copy_tex_data(struct nvenc_data *enc, const bool p010, GLuint tex[2], struct nv_cuda_surface *surf)
{
	bool success = true;
	CUgraphicsResource mapped_tex[2] = {0};
	CUarray mapped_cuda;

	if (!get_res_for_tex_ids(enc, tex[0], tex[1], &mapped_tex[0], &mapped_tex[1]))
		return false;

	CU_CHECK(cu->cuGraphicsMapResources(2, mapped_tex, 0))

	CUDA_MEMCPY2D m = {0};
	m.dstMemoryType = CU_MEMORYTYPE_ARRAY;
	m.srcMemoryType = CU_MEMORYTYPE_ARRAY;
	m.dstArray = surf->tex;
	m.WidthInBytes = p010 ? enc->cx * 2 : enc->cx;
	m.Height = enc->cy;

	// Map and copy Y texture
	CU_CHECK(cu->cuGraphicsSubResourceGetMappedArray(&mapped_cuda, mapped_tex[0], 0, 0));
	m.srcArray = mapped_cuda;
	CU_CHECK(cu->cuMemcpy2D(&m))

	// Map and copy UV texture
	CU_CHECK(cu->cuGraphicsSubResourceGetMappedArray(&mapped_cuda, mapped_tex[1], 0, 0))
	m.srcArray = mapped_cuda;
	m.dstY += enc->cy;
	m.Height = enc->cy / 2;

	CU_CHECK(cu->cuMemcpy2D(&m))

unmap:
	cu->cuGraphicsUnmapResources(2, mapped_tex, 0);

	return success;
}

bool cuda_opengl_encode(void *data, struct encoder_texture *tex, int64_t pts, uint64_t lock_key, uint64_t *next_key,
			struct encoder_packet *packet, bool *received_packet)
{
	struct nvenc_data *enc = data;
	struct nv_cuda_surface *surf;
	struct nv_bitstream *bs;
	const bool p010 = obs_encoder_video_tex_active(enc->encoder, VIDEO_FORMAT_P010);
	GLuint input_tex[2];

	if (tex == NULL || tex->tex[0] == NULL) {
		error("Encode failed: bad texture handle");
		*next_key = lock_key;
		return false;
	}

	bs = &enc->bitstreams.array[enc->next_bitstream];
	surf = &enc->surfaces.array[enc->next_bitstream];

	deque_push_back(&enc->dts_list, &pts, sizeof(pts));

	/* ------------------------------------ */
	/* copy to CUDA data                    */

	CU_FAILED(cu->cuCtxPushCurrent(enc->cu_ctx))
	obs_enter_graphics();
	input_tex[0] = *(GLuint *)gs_texture_get_obj(tex->tex[0]);
	input_tex[1] = *(GLuint *)gs_texture_get_obj(tex->tex[1]);

	bool success = copy_tex_data(enc, p010, input_tex, surf);

	obs_leave_graphics();
	CU_FAILED(cu->cuCtxPopCurrent(NULL))

	if (!success)
		return false;

	/* ------------------------------------ */
	/* map output tex so nvenc can use it   */

	NV_ENC_MAP_INPUT_RESOURCE map = {NV_ENC_MAP_INPUT_RESOURCE_VER};
	map.registeredResource = surf->res;
	map.mappedBufferFmt = p010 ? NV_ENC_BUFFER_FORMAT_YUV420_10BIT : NV_ENC_BUFFER_FORMAT_NV12;

	if (NV_FAILED(nv.nvEncMapInputResource(enc->session, &map)))
		return false;

	surf->mapped_res = map.mappedResource;

	/* ------------------------------------ */
	/* do actual encode call                */

	return nvenc_encode_base(enc, bs, surf->mapped_res, pts, packet, received_packet);
}
