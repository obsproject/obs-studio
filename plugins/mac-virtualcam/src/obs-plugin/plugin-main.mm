#include <obs-module.h>
#include <obs.hpp>
#include <pthread.h>
#include <QMainWindow.h>
#include <QAction.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <util/config-file.h>
#include <util/platform.h>
#include <CoreFoundation/CoreFoundation.h>
#include <AppKit/AppKit.h>
#include "OBSDALMachServer.h"
#include "Defines.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mac-virtualcam", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "macOS virtual webcam output";
}

obs_output_t *outputRef;
obs_video_info videoInfo;
static OBSDALMachServer *sMachServer;

static BOOL update_placeholder();

static bool check_dal_plugin()
{
	NSFileManager *fileManager = [NSFileManager defaultManager];

	NSString *dalPluginDestinationPath =
		@"/Library/CoreMediaIO/Plug-Ins/DAL/";
	NSString *dalPluginFileName = [dalPluginDestinationPath
		stringByAppendingString:@"obs-mac-virtualcam.plugin"];

	BOOL dalPluginInstalled =
		[fileManager fileExistsAtPath:dalPluginFileName];
	BOOL dalPluginUpdateNeeded = NO;

	if (dalPluginInstalled) {
		NSString *dalPluginPlistPath = [dalPluginFileName
			stringByAppendingString:@"/Contents/Info.plist"];
		NSDictionary *dalPluginInfoPlist = [NSDictionary
			dictionaryWithContentsOfURL:
				[NSURL fileURLWithPath:dalPluginPlistPath]
					      error:nil];
		NSString *dalPluginVersion = [dalPluginInfoPlist
			valueForKey:@"CFBundleShortVersionString"];
		const char *obsVersion = obs_get_version_string();

		if (![dalPluginVersion isEqualToString:@(obsVersion)]) {
			dalPluginUpdateNeeded = YES;
		}
		if (!dalPluginUpdateNeeded)
			dalPluginUpdateNeeded = update_placeholder();
	} else {
		dalPluginUpdateNeeded = YES;
	}

	if (dalPluginUpdateNeeded) {
		NSString *dalPluginSourcePath;
		NSRunningApplication *app =
			[NSRunningApplication currentApplication];

		if ([app bundleIdentifier] != nil) {
			NSURL *bundleURL = [app bundleURL];
			NSString *pluginPath =
				@"Contents/Resources/data/obs-mac-virtualcam.plugin";

			NSURL *pluginUrl = [bundleURL
				URLByAppendingPathComponent:pluginPath];
			dalPluginSourcePath = [pluginUrl path];
		} else {
			dalPluginSourcePath = [[[[app executableURL]
				URLByAppendingPathComponent:
					@"../data/obs-mac-virtualcam.plugin"]
				path]
				stringByReplacingOccurrencesOfString:@"obs/"
							  withString:@""];
		}

		if ([fileManager fileExistsAtPath:dalPluginSourcePath]) {
			NSString *copyCmd = [NSString
				stringWithFormat:
					@"do shell script \"cp -R '%@' '%@'\" with administrator privileges",
					dalPluginSourcePath,
					dalPluginDestinationPath];

			NSDictionary *errorDict;
			NSAppleEventDescriptor *returnDescriptor = NULL;
			NSAppleScript *scriptObject =
				[[NSAppleScript alloc] initWithSource:copyCmd];
			returnDescriptor =
				[scriptObject executeAndReturnError:&errorDict];
			if (errorDict != nil) {
				const char *errorMessage = [[errorDict
					objectForKey:@"NSAppleScriptErrorMessage"]
					UTF8String];
				blog(LOG_INFO,
				     "[macOS] VirtualCam DAL Plugin Installation status: %s",
				     errorMessage);
				return false;
			}
		} else {
			blog(LOG_INFO,
			     "[macOS] VirtualCam DAL Plugin not shipped with OBS");
			return false;
		}
	}
	return true;
}

static const char *virtualcam_output_get_name(void *type_data)
{
	(void)type_data;
	return obs_module_text("Plugin_Name");
}

// This is a dummy pointer so we have something to return from virtualcam_output_create
static void *data = &data;

static void *virtualcam_output_create(obs_data_t *settings,
				      obs_output_t *output)
{
	UNUSED_PARAMETER(settings);

	outputRef = output;

	blog(LOG_DEBUG, "output_create");
	sMachServer = [[OBSDALMachServer alloc] init];
	return data;
}

static void virtualcam_output_destroy(void *data)
{
	UNUSED_PARAMETER(data);
	blog(LOG_DEBUG, "output_destroy");
	sMachServer = nil;
}

static bool virtualcam_output_start(void *data)
{
	UNUSED_PARAMETER(data);

	bool hasDalPlugin = check_dal_plugin();

	if (!hasDalPlugin) {
		return false;
	}

	blog(LOG_DEBUG, "output_start");

	[sMachServer run];

	obs_get_video_info(&videoInfo);

	struct video_scale_info conversion = {};
	conversion.format = VIDEO_FORMAT_UYVY;
	conversion.width = videoInfo.output_width;
	conversion.height = videoInfo.output_height;
	obs_output_set_video_conversion(outputRef, &conversion);
	if (!obs_output_begin_data_capture(outputRef, 0)) {
		return false;
	}

	return true;
}

static void virtualcam_output_stop(void *data, uint64_t ts)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(ts);

	blog(LOG_DEBUG, "output_stop");
	obs_output_end_data_capture(outputRef);
	[sMachServer stop];
}

static void virtualcam_output_raw_video(void *data, struct video_data *frame)
{
	UNUSED_PARAMETER(data);

	uint8_t *outData = frame->data[0];
	if (frame->linesize[0] != (videoInfo.output_width * 2)) {
		blog(LOG_ERROR,
		     "unexpected frame->linesize (expected:%d actual:%d)",
		     (videoInfo.output_width * 2), frame->linesize[0]);
	}

	CGFloat width = videoInfo.output_width;
	CGFloat height = videoInfo.output_height;

	[sMachServer sendFrameWithSize:NSMakeSize(width, height)
			     timestamp:frame->timestamp
			  fpsNumerator:videoInfo.fps_num
			fpsDenominator:videoInfo.fps_den
			    frameBytes:outData];
}

struct obs_output_info virtualcam_output_info = {
	.id = "virtualcam_output",
	.flags = OBS_OUTPUT_VIDEO,
	.get_name = virtualcam_output_get_name,
	.create = virtualcam_output_create,
	.destroy = virtualcam_output_destroy,
	.start = virtualcam_output_start,
	.stop = virtualcam_output_stop,
	.raw_video = virtualcam_output_raw_video,
};

bool obs_module_load(void)
{
	blog(LOG_INFO, "version=%s", PLUGIN_VERSION);

	obs_register_output(&virtualcam_output_info);

	obs_data_t *obs_settings = obs_data_create();
	obs_data_set_bool(obs_settings, "vcamEnabled", true);
	obs_apply_private_data(obs_settings);
	obs_data_release(obs_settings);

	return true;
}

static BOOL backup_placeholder(NSFileManager *fileMgr, NSString *curpng,
			       NSString *backupStr)
{
	if (backupStr != nil && curpng != nil) {
		BOOL backupIsDir = NO;
		BOOL backupExists = [fileMgr fileExistsAtPath:backupStr
						  isDirectory:&backupIsDir];
		BOOL backupWritable = [fileMgr isWritableFileAtPath:backupStr];
		BOOL curpngExists = [fileMgr fileExistsAtPath:curpng];
		BOOL curpngReadable = [fileMgr isReadableFileAtPath:curpng];

		if (backupIsDir && backupWritable && curpngReadable) {
			NSDate *now = [NSDate date];
			NSDateFormatter *tsFormat =
				[[NSDateFormatter alloc] init];
			[tsFormat setDateFormat:@"YYYY-MM-dd-HH-mm-ss"];
			NSString *ts = [tsFormat stringFromDate:now];
			NSString *backUp = [NSString
				stringWithFormat:@"%@placeholder%@.png",
						 backupStr, ts];
			blog(LOG_INFO, "backup file (%s)", [backUp UTF8String]);
			NSString *copyCmd = [NSString
				stringWithFormat:
					@"do shell script \"cp '%@' '%@'\"",
					curpng, backUp];

			blog(LOG_INFO, "copyCmd: %s", [copyCmd UTF8String]);
			NSDictionary *errorDict;
			NSAppleEventDescriptor *returnDescriptor = NULL;
			NSAppleScript *scriptObject =
				[[NSAppleScript alloc] initWithSource:copyCmd];
			returnDescriptor =
				[scriptObject executeAndReturnError:&errorDict];
			if (errorDict != nil) {
				const char *errorMessage = [[errorDict
					objectForKey:@"NSAppleScriptErrorMessage"]
					UTF8String];
				blog(LOG_INFO,
				     "[macOS] VirtualCam backup placeholder.png status: %s",
				     errorMessage);
				return NO;
			} else {
				return YES;
			}
		}
	}
	return NO;
}

static BOOL replace_placeholderpng(NSString *curpng, NSString *newpng)
{
	NSString *copyCmd =
		[NSString stringWithFormat:@"do shell script \"cp '%@' '%@'\"",
					   newpng, curpng];

	blog(LOG_INFO, "copyCmd: %s", copyCmd);
	NSDictionary *errorDict;
	NSAppleEventDescriptor *returnDescriptor = NULL;
	NSAppleScript *scriptObject =
		[[NSAppleScript alloc] initWithSource:copyCmd];
	returnDescriptor = [scriptObject executeAndReturnError:&errorDict];
	if (errorDict != nil) {
		const char *errorMessage = [[errorDict
			objectForKey:@"NSAppleScriptErrorMessage"] UTF8String];
		blog(LOG_INFO,
		     "[macOS] VirtualCam replace placeholder.png status: %s",
		     errorMessage);
		return NO;
	} else {
		return YES;
	}
}

static BOOL replace_placeholder(const char *cur_ph, const char *new_ph,
				const char *backup)
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString *curpng = nil;
	NSString *newpng = nil;
	NSString *backupStr = nil;
	BOOL backupOK = YES;

	blog(LOG_INFO, "replace_placeholder old('%s') new (%s) backup(%s)",
	     cur_ph, new_ph, backup);

	curpng = [NSString stringWithCString:cur_ph
				    encoding:NSUTF8StringEncoding];
	newpng = [NSString stringWithCString:new_ph
				    encoding:NSUTF8StringEncoding];
	BOOL replaceNeeded = ![fileManager contentsEqualAtPath:curpng
						       andPath:newpng];

	blog(LOG_INFO, "replace PNG needed ('%d')", replaceNeeded);
	if (!replaceNeeded)
		return NO;

	if (backup == NULL) {
		backupStr = nil;
	} else {
		backupStr = [NSString stringWithCString:backup
					       encoding:NSUTF8StringEncoding];
		if ([backupStr isEqualToString:@""]) {
			backupOK = YES;
		} else {
			if (![backupStr hasSuffix:@"/"])
				backupStr = [backupStr
					stringByAppendingString:@"/"];
			backupOK = backup_placeholder(fileManager, curpng,
						      backupStr);
		}
	}

	if (backupOK)
		return replace_placeholderpng(curpng, newpng);
	else
		return NO;
}

static BOOL check_placeholder(NSString *src)
{
	char basic[MAX_PATH_LEN];
	char basicConf[MAX_PATH_LEN];
	char global[MAX_PATH_LEN];
	const char *cur_ph = [src UTF8String];
	config_t *gconf;
	config_t *bconf;
	BOOL replaced = NO;
	os_get_config_path(global, sizeof(global), "obs-studio/global.ini");
	int rc = config_open(&gconf, global, CONFIG_OPEN_EXISTING);
	if (rc == CONFIG_SUCCESS) {
		const char *profiles_path =
			config_get_string(gconf, "Basic", "ProfileDir");
		blog(LOG_INFO, "%s [%d]", global, rc);
		blog(LOG_INFO, "%s", profiles_path);
		snprintf(basic, sizeof(basic),
			 "obs-studio/basic/profiles/%s/basic.ini",
			 profiles_path);
		blog(LOG_INFO, "%s", basic);
		os_get_config_path(basicConf, sizeof(basicConf), basic);
		int brc = config_open(&bconf, basicConf, CONFIG_OPEN_EXISTING);
		if (brc == CONFIG_SUCCESS) {
			blog(LOG_INFO, "config_open('%s') success [%d]",
			     basicConf, brc);
			const char *new_ph = config_get_string(
				bconf, "PlaceHolderPNG", "PNGFile");
			const char *backup = config_get_string(
				bconf, "PlaceHolderPNG", "BackupPath");
			blog(LOG_INFO, "old('%s') new (%s) backup(%s)", cur_ph,
			     new_ph, backup);
			if (cur_ph != NULL && new_ph != NULL)
				replaced = replace_placeholder(cur_ph, new_ph,
							       backup);
		} else {
			blog(LOG_INFO, "config_open('%s') failed [%d]",
			     basicConf, brc);
		}
		config_close(bconf);
	} else {
		blog(LOG_INFO, "config_open('%s') failed [%d]", global, rc);
	}
	config_close(gconf);
	blog(LOG_INFO, "replaced [%d]", replaced);
	return replaced;
}

static BOOL update_placeholder()
{
	NSString *dalPluginSourcePath;
	NSRunningApplication *app = [NSRunningApplication currentApplication];

	if ([app bundleIdentifier] != nil) {
		NSURL *bundleURL = [app bundleURL];
		NSString *pluginPath =
			@"Contents/Resources/data/obs-mac-virtualcam.plugin";

		NSURL *pluginUrl =
			[bundleURL URLByAppendingPathComponent:pluginPath];
		dalPluginSourcePath = [pluginUrl path];
	} else {
		dalPluginSourcePath = [[[[app executableURL]
			URLByAppendingPathComponent:
				@"../data/obs-mac-virtualcam.plugin"] path]
			stringByReplacingOccurrencesOfString:@"obs/"
						  withString:@""];
	}

	NSFileManager *localFileManager = [[NSFileManager alloc] init];
	NSDirectoryEnumerator *dirEnum =
		[localFileManager enumeratorAtPath:dalPluginSourcePath];
	NSString *file;
	NSString *srcpng = nil;
	blog(LOG_INFO, "%s", [dalPluginSourcePath UTF8String]);
	while ((file = [dirEnum nextObject])) {
		if ([[file lastPathComponent]
			    isEqualToString:@"placeholder.png"]) {
			srcpng = [NSString stringWithFormat:@"%@/%@",
							    dalPluginSourcePath,
							    file];
			blog(LOG_INFO, "%s", [srcpng UTF8String]);
		}
	}

	return check_placeholder(srcpng);
}
