#include <obs-module.h>
#include <util/darray.h>

#include "vaapi-common.h"
#include "vaapi-caps.h"
#include "vaapi-internal.h"

static const vaapi_profile_def_t vaapi_profile_defs[] = {
	{ "constrained baseline", VAProfileH264ConstrainedBaseline,
	                          VAAPI_PROFILE_BASELINE_CONSTRAINED },
	{ "baseline",             VAProfileH264Baseline,
	                          VAAPI_PROFILE_BASELINE             },
	{ "main",                 VAProfileH264Main,
	                          VAAPI_PROFILE_MAIN                 },
	{ "high",                 VAProfileH264High,
	                          VAAPI_PROFILE_HIGH                 }
};
static const size_t va_profiles_cnt =
		sizeof(vaapi_profile_defs) / sizeof(vaapi_profile_defs[0]);

static VAConfigAttrib vaapi_config_attribs[] = {
	{ VAConfigAttribRTFormat,         VA_RT_FORMAT_YUV420           |
	                                  VA_RT_FORMAT_YUV444             },
	{ VAConfigAttribRateControl,      VA_RC_CBR                     |
	                                  VA_RC_VBR                     |
	                                  VA_RC_VCM                     |
					  VA_RC_CQP                     |
	                                  VA_RC_VBR_CONSTRAINED           },
	{ VAConfigAttribEncPackedHeaders, VA_ENC_PACKED_HEADER_SEQUENCE |
	                                  VA_ENC_PACKED_HEADER_PICTURE    }
};
static size_t vaapi_config_attribs_cnt =
		  sizeof(vaapi_config_attribs)
		/ sizeof(vaapi_config_attribs[0]);

bool vaapi_caps_is_format_supported(vaapi_profile_caps_t *caps,
		enum video_format format)
{
	int va_format = VAAPI_FORMAT_NONE;
	switch(format)
	{
	case VIDEO_FORMAT_I420:
		va_format = VA_RT_FORMAT_YUV420;
		break;
	case VAAPI_FORMAT_YUV444:
		va_format = VA_RT_FORMAT_YUV444;
		break;
	default:
		break;
	}

	return (va_format & caps->format) > 0;
}

static void vaapi_profile_caps_dump(struct vaapi_profile_caps *p)
{


	VA_LOG(LOG_DEBUG, "    name:    \t%s:", p->def.name);
	VA_LOG(LOG_DEBUG, "    max_dim: \tw=%d, h=%d", p->max_width,
			p->max_height);

#define FMT_SUPP(f) \
	(((p->format & VA_RT_FORMAT_ ## f) > 0) ? "yes" : "no")
	VA_LOG(LOG_DEBUG, "    formats: \tYUV420: %s", FMT_SUPP(YUV420));
	VA_LOG(LOG_DEBUG, "             \tYUV444: %s", FMT_SUPP(YUV444));
#undef FMT_SUPP

	for(size_t i = 0; i < p->attribs_cnt; i++) {
		switch(p->attribs[i].type)
		{
		case VAConfigAttribRateControl:
#define RC_SUPP(rc) (((p->attribs[i].value & VA_RC_ ## rc) > 0) \
	? #rc ": yes" : #rc": no" )

			VA_LOG(LOG_DEBUG, "    rate: \t%s", RC_SUPP(NONE));
			VA_LOG(LOG_DEBUG, "    rate: \t%s", RC_SUPP(CBR));
			VA_LOG(LOG_DEBUG, "    rate: \t%s", RC_SUPP(VBR));
			VA_LOG(LOG_DEBUG, "    rate: \t%s", RC_SUPP(VCM));
			VA_LOG(LOG_DEBUG, "    rate: \t%s", RC_SUPP(CQP));
			VA_LOG(LOG_DEBUG, "    rate: \t%s",
					RC_SUPP(VBR_CONSTRAINED));
#undef RC_SUPP
			break;
		default:
			break;
		}
	}
}

static bool apply_encoder_dimensions(VADisplay display, VAProfile profile,
		VAEntrypoint entrypoint,
		struct vaapi_profile_caps *profile_caps)
{
	VAConfigID config_id = 0;
	VAStatus status;
	DARRAY(VASurfaceAttrib) attribs = {0};
	unsigned int attrib_cnt;

	CHECK_STATUS_FAIL(vaCreateConfig(display, profile, entrypoint,
			vaapi_config_attribs, vaapi_config_attribs_cnt,
			&config_id));

	if (config_id == VA_INVALID_ID) {
		VA_LOG(LOG_WARNING, "failed to create config_id for %s profile",
				profile_caps->def.name);
		goto fail;
	}


	CHECK_STATUS_FALSE(vaQuerySurfaceAttributes(display, config_id,
			NULL, &attrib_cnt));

	if (attrib_cnt == 0) {
		VA_LOG(LOG_WARNING, "failed query of surface attributes for "
				"%s profile",
				profile_caps->def.name);
		goto fail;
	}

	da_reserve(attribs, attrib_cnt);
	CHECK_STATUS_FALSE(vaQuerySurfaceAttributes(display, config_id,
			attribs.array, &attrib_cnt));
	attribs.num = attrib_cnt;

	for(size_t i = 0; i < attribs.num; i++) {
		if (attribs.array[i].type == VASurfaceAttribMaxWidth)
			profile_caps->max_width =
					attribs.array[i].value.value.i;
		else if (attribs.array[i].type == VASurfaceAttribMaxHeight)
			profile_caps->max_height =
					attribs.array[i].value.value.i;
	}

	if (profile_caps->max_width == 0 ||
	    profile_caps->max_height == 0) {
		profile_caps->max_width = 1920;
		profile_caps->max_height = 1080;
	}

	da_free(attribs);
	return true;

fail:
	if (config_id != VA_INVALID_ID)
		vaDestroyConfig(display, config_id);
	da_free(attribs);

	return false;
}

static bool apply_encoder_attrib(VADisplay display, VAProfile profile,
		VAEntrypoint entrypoint,
		struct vaapi_profile_caps *supported_profile)
{
	VAStatus status;
	DARRAY(VAConfigAttrib) attribs = {0};
	da_resize(attribs, 0);

	da_push_back_array(attribs, vaapi_config_attribs,
			vaapi_config_attribs_cnt);

	for(size_t i = 0; i < attribs.num; i++)
		attribs.array[i].value = 0;

	CHECK_STATUS_FAIL(vaGetConfigAttributes(display, profile, entrypoint,
			attribs.array, attribs.num));

#define ATTRIB_PRESENT \
	((attribs.array[i].value & vaapi_config_attribs[i].value) == \
			vaapi_config_attribs[i].value)
#define ATTRIB_ANY \
	((attribs.array[i].value & vaapi_config_attribs[i].value) > 0)

	for(size_t i = 0; i < vaapi_config_attribs_cnt; i++) {
		switch(attribs.array[i].type)
		{
		case VAConfigAttribRTFormat:
			if (!ATTRIB_ANY)
				return false;
			supported_profile->format &= attribs.array[i].value;
			continue;
		case VAConfigAttribRateControl:
			if (!ATTRIB_ANY)
				return false;
			supported_profile->rc &= attribs.array[i].value;
			continue;
		case VAConfigAttribEncPackedHeaders:
			if (!ATTRIB_PRESENT)
				return false;
			continue;
		default:
			continue;
		}
	}

#undef ATTRIB_PRESENT
#undef ATTRIB_ANY

	supported_profile->attribs = vaapi_config_attribs;
	supported_profile->attribs_cnt = vaapi_config_attribs_cnt;
	da_free(attribs);

	return apply_encoder_dimensions(display, profile, entrypoint,
			supported_profile);

fail:
	da_free(attribs);
	return false;
}

struct vaapi_profile_caps *vaapi_caps_from_profile(vaapi_display_t *display,
		vaapi_profile_t profile)
{
	VAProfile p;

	switch(profile)
	{
	case VAAPI_PROFILE_BASELINE_CONSTRAINED:
		p = VAProfileH264ConstrainedBaseline; break;
	case VAAPI_PROFILE_BASELINE:
		p = VAProfileH264Baseline; break;
	case VAAPI_PROFILE_MAIN:
		p = VAProfileH264Main; break;
	case VAAPI_PROFILE_HIGH:
		p = VAProfileH264High; break;
	default:
		p = VAProfileNone;
	}

	vaapi_profile_t best_match = VAAPI_PROFILE_NONE;
	vaapi_profile_caps_t *best_match_caps = NULL;

	for(size_t i = 0; i < display->caps.num; i++) {
		if (display->caps.array[i].def.va == p) {
			return &display->caps.array[i];
		} else if (display->caps.array[i].def.vaapi > best_match) {
			best_match = display->caps.array[i].def.va;
			best_match_caps = &display->caps.array[i];
		}
	}

	if (best_match != VAAPI_PROFILE_NONE && best_match_caps != NULL)
		VA_LOG(LOG_WARNING, "found no exact match for profile, "
				"defaulting to best match '%s'",
				best_match_caps->def.name);
	else
		VA_LOG(LOG_ERROR, "no matching profile found in supported "
				"profiles");

	return best_match_caps;
}

bool vaapi_profile_caps_init(vaapi_display_t *display)
{
	VADisplay va_display = display->display;
	da_resize(display->caps, 0);
	int ep_max = vaMaxNumEntrypoints(va_display);

	VAEntrypoint *eps = bzalloc(sizeof(VAEntrypoint) * ep_max);
	int eps_cnt;

	for (size_t i = 0; i < va_profiles_cnt; i++) {
		struct vaapi_profile_caps p = {0};
		VAStatus status = 0;
		bool valid_encoder;

		status = vaQueryConfigEntrypoints(va_display,
				vaapi_profile_defs[i].va, eps,
				&eps_cnt);
		if (status == VA_STATUS_ERROR_UNSUPPORTED_PROFILE)
			continue;
		else if (status != VA_STATUS_SUCCESS)
			goto fail;

		p.def = vaapi_profile_defs[i];

		for(int j = 0; j < eps_cnt; j++) {
			if (eps[j] == VAEntrypointEncSlice) {
				if (apply_encoder_attrib(va_display,
						vaapi_profile_defs[i].va,
						eps[j], &p)) {
					valid_encoder = true;
					break;
				}
			}
		}

		p.entrypoint = VAEntrypointEncSlice;

		if (!valid_encoder)
			continue;

		da_push_back(display->caps, &p);
	}

	bfree(eps);

	return true;

fail:
	bfree(eps);

	return false;
}
