#include <obs-module.h>
#include <libavutil/log.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ffmpeg", "en-US")

extern struct obs_source_info  ffmpeg_source;
extern struct obs_output_info  ffmpeg_output;
extern struct obs_output_info  ffmpeg_muxer;
extern struct obs_encoder_info aac_encoder_info;

static void ffmpeg_log_callback(void* context, int level, const char* format,
	va_list args)
{
	if (format == NULL)
		return;

	static char str[4096] = {0};
	static int print_prefix = 1;

	av_log_format_line(context, level, format, args, str + strlen(str),
			sizeof(str) - strlen(str), &print_prefix);

	int obs_level;
	switch (level) {
	case AV_LOG_PANIC:
	case AV_LOG_FATAL:
		obs_level = LOG_ERROR;
		break;
	case AV_LOG_ERROR:
	case AV_LOG_WARNING:
		obs_level = LOG_WARNING;
		break;
	case AV_LOG_INFO:
	case AV_LOG_VERBOSE:
		obs_level = LOG_INFO;
		break;
	case AV_LOG_DEBUG:
	default:
		obs_level = LOG_DEBUG;
	}

	if (!print_prefix)
		return;

	char *str_end = str + strlen(str) - 1;
	while(str < str_end) {
		if (*str_end != '\n')
			break;
		*str_end-- = '\0';
	}

	if (str_end <= str)
		return;

	blog(obs_level, "[ffmpeg] %s", str);
	str[0] = 0;
}

bool obs_module_load(void)
{
	av_log_set_callback(ffmpeg_log_callback);

	obs_register_source(&ffmpeg_source);
	obs_register_output(&ffmpeg_output);
	obs_register_output(&ffmpeg_muxer);
	obs_register_encoder(&aac_encoder_info);
	return true;
}
