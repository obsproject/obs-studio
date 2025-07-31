#include "AMF/core/Context.h"
#include "AMF/core/VulkanAMF.h"

class amf_gpu {
public:
	virtual AMF_RESULT init(amf::AMFContext *context, uint32_t adapter) = 0;
	virtual AMF_RESULT terminate() = 0;
	virtual AMF_RESULT copy_obs_to_amf(struct encoder_texture *texture, uint64_t lock_key, uint64_t *next_key,
					   amf::AMF_SURFACE_FORMAT format, int width, int height,
					   amf::AMFSurface **surface) = 0;
	virtual AMF_RESULT get_luid(uint64_t *luid) = 0;
	virtual AMF_RESULT get_vendor(uint32_t *vendor) = 0;

	static amf_gpu *create();
	virtual ~amf_gpu() {}

protected:
	amf_gpu() {}
};

// need to define AMFOBS_ENCODER and AMFOBS_ENCODER_NAME before
#define do_log(level, format, ...) \
	blog(level, "[%s: '%s'] " format, AMFOBS_ENCODER, AMFOBS_ENCODER_NAME, ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

#define AMFOBS_RETURN_IF_FALSE(exp, ret, format, ...) \
	{ \
		bool exp_res = (exp); \
		if(!exp_res) \
		{ \
			error(format,  ##__VA_ARGS__); \
			return ret; \
		} \
	}

#define AMFOBS_RETURN_IF_FAILED(_ret, format, ...) \
	AMFOBS_RETURN_IF_FALSE((_ret == AMF_OK), _ret, format, ##__VA_ARGS__)
