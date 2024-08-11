//
//  plugin-main.h
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

#import <obs-module.h>
#import <pthread.h>
#import "OBSAVCapture.h"
#import "plugin-properties.h"

/// Get  localized text for module string identifier
/// - Parameters:
///   - text_id: Localization string identifier
/// - Returns: Pointer to localized text variant
extern const char *av_capture_get_text(const char *text_id);

/// Create a macOS ``OBSAVCapture`` source  using asynchronous video frames
/// - Parameters:
///   - settings: Pointer to ``libobs`` data struct with possible user settings read from configuration file
///   - source: Pointer to ``libobs`` source struct
/// - Returns: Pointer to created ``OBSAVCaptureInfo`` struct
static void *av_capture_create(obs_data_t *settings, obs_source_t *source);

/// Create a macOS ``OBSAVCapture`` source   rendering video frames directly.
/// - Parameters:
///   - settings: Pointer to ``libobs`` data struct with possible user settings read from configuration file
///   - source: Pointer to ``libobs`` source struct
/// - Returns: Pointer to created ``OBSAVCaptureInfo`` struct
static void *av_fast_capture_create(obs_data_t *settings, obs_source_t *source);

/// Get localized ``OBSAVCapture`` source  name.
///
/// The value returned by this function will be used to identify the ``OBSAVCapture`` source throughout the OBS Studio user interface.
///
/// - Parameters:
///   - data: Pointer to ``OBSAVCaptureInfo`` struct as returned by ``av_capture_create``
/// - Returns: Pointer to localized source type name
static const char *av_capture_get_name(void *capture_info);

/// Get localized source name of the fast capture variant of the ``OBSAVCapture`` source.
///
/// The value returned by this function will be used to identify the ``OBSAVCapture`` source throughout the OBS Studio user interface.
///
/// - Parameters:
///   - data: Pointer to ``OBSAVCaptureInfo`` struct as returned by ``av_capture_create``
/// - Returns: Pointer to localized source type name
static const char *av_fast_capture_get_name(void *capture_info);

/// Set default values used by the ``OBSAVCapture`` source
///
/// While this function sets default values for specific properties in the user settings, the function is also called to _get_ defaults by ``libobs``
///
/// - Parameters:
///   - settings: Pointer to obs settings struct
static void av_capture_set_defaults(obs_data_t *settings);

/// Set default values used by the fast capture variant of the ``OBSAVCapture`` source
///
/// While this function sets default values for specific properties in the user settings, the function is also called to _get_ defaults by ``libobs``
///
/// - Parameters:
///   - settings: Pointer to obs settings struct
static void av_fast_capture_set_defaults(obs_data_t *settings);

/// Creates a new properties struct with all the properties used by the ``OBSAVCapture`` source.
///
/// This function is commonly used to set up all the properties that a source type has and can be used by the internal API to discover which properties exist, but is also commonly used to set up the properties used for the OBS Studio user interface.
///
/// If the source type was created successfully, the associated ``OBSAVCaptureInfo`` struct is passed to this function as well, which allows setting up property visibility and availability depending on the source state.
///
/// - Parameters:
///   - capture_info: Pointer to ``OBSAVCaptureInfo`` struct
/// - Returns: Pointer to created OBS properties struct
static obs_properties_t *av_capture_properties(void *capture_info);

/// Update ``OBSAVCapture`` source
/// - Parameters:
///   - capture_info: Pointer to ``OBSAVCaptureInfo`` struct
///   - settings: Pointer to settings struct
static void av_capture_update(void *capture_info, obs_data_t *settings);

/// Handle ``tick`` of ``libobs`` compositing engine.
///
/// ``libobs`` sends a tick at an internal refresh rate to allow sources to prepare data for output. This function works in conjunction with the ``render`` which function which is called at a later point at the frame rate specified in the `Output` configuration of OBS Studio.
///
/// ``OBSAVCapture`` keeps a reference to the ``IOSurface`` of the last two frames provided by ``CoreMediaIO`` and converts a valid ``IOSurface`` into a ``libobs``-recognized texture in this function.
///
/// - Parameters:
///   - capture_info: Pointer to ``OBSAVCaptureInfo`` struct
///   - seconds: Delta since the last call to this function
static void av_fast_capture_tick(void *capture_info, float seconds);

/// Handle ``render`` of ``libobs`` compositing engine.
///
/// ``libobs`` sends a render call at the frame rate specified in the `Output` configuration of OBS Studio, which requires the source to do the actual rendering work using the ``libobs`` graphics engine.Draw
///
/// - Parameters:
///   - capture_info: Pointer to ``OBSAVCaptureInfo`` struct
///   - effect: Draw function used by ``libobs``
static void av_fast_capture_render(void *capture_info, gs_effect_t *effect);

/// Get width of texture currently managed by the ``OBSAVCapture`` source
/// - Parameters:
///   - capture_info: Pointer to ``OBSAVCaptureInfo`` struct
static UInt32 av_fast_capture_get_width(void *capture_info);

/// Get height of texture currently managed by the ``OBSAVCapture`` source
/// - Parameters:
///   - capture_info: Pointer to ``OBSAVCaptureInfo`` struct
static UInt32 av_fast_capture_get_height(void *capture_info);

/// Tear down ``OBSAVCapture`` source.
///
/// This function implements all the necessary cleanup functions to ensure a clean exit of the program. Any resources or references held by the source need to be deleted or destroyed to avoid memory leaks. Any shutdown or cleanup functions required by resources used by the source also need to be called to ensure a clean shutdown.
/// - Parameters:
///   - capture_info: Pointer to ``OBSAVCaptureInfo`` struct
static void av_capture_destroy(void *capture_info);
