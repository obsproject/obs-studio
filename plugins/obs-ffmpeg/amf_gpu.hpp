#include "AMF/core/Context.h"
#include "AMF/core/VulkanAMF.h"

class obs_graphics // self destructuctor
{
public:
	obs_graphics() { obs_enter_graphics(); }
	~obs_graphics() { obs_leave_graphics(); }
};

class amf_gpu {
public:
	virtual AMF_RESULT init(amf::AMFContext *context, uint32_t adapter) = 0;
	virtual AMF_RESULT terminate() = 0;
	virtual AMF_RESULT copy_obs_to_amf(struct encoder_texture *texture, uint64_t lock_key, uint64_t *next_key,
					   amf::AMF_SURFACE_FORMAT format, int width, int height,
					   amf::AMFSurface **surface) = 0;
	;
	static AMF_RESULT create(amf_gpu **amf_gpu_obj);

	virtual ~amf_gpu() {}

protected:
	amf_gpu() {}
};

// need to define AMFOBS_ENCODER and AMFOBS_ENCODER_NAME
#define do_log(level, format, ...) \
	blog(level, "[%s: '%s'] " format, AMFOBS_ENCODER, AMFOBS_ENCODER_NAME, ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)
