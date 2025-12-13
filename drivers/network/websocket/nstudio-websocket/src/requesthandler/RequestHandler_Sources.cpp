/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <QBuffer>
#include <QImageWriter>
#include <QFileInfo>
#include <QImage>
#include <QDir>

#include "RequestHandler.h"

QImage TakeSourceScreenshot(obs_source_t *source, bool &success, uint32_t requestedWidth = 0, uint32_t requestedHeight = 0)
{
	// Get info about the requested source
	const uint32_t sourceWidth = obs_source_get_width(source);
	const uint32_t sourceHeight = obs_source_get_height(source);
	const double sourceAspectRatio = ((double)sourceWidth / (double)sourceHeight);

	uint32_t imgWidth = sourceWidth;
	uint32_t imgHeight = sourceHeight;

	// Determine suitable image width
	if (requestedWidth) {
		imgWidth = requestedWidth;

		if (!requestedHeight)
			imgHeight = ((double)imgWidth / sourceAspectRatio);
	}

	// Determine suitable image height
	if (requestedHeight) {
		imgHeight = requestedHeight;

		if (!requestedWidth)
			imgWidth = ((double)imgHeight * sourceAspectRatio);
	}

	// Create final image texture
	QImage ret(imgWidth, imgHeight, QImage::Format::Format_RGBA8888);
	ret.fill(0);

	// Video image buffer
	uint8_t *videoData = nullptr;
	uint32_t videoLinesize = 0;

	// Enter graphics context
	obs_enter_graphics();

	gs_texrender_t *texRender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	gs_stagesurf_t *stageSurface = gs_stagesurface_create(imgWidth, imgHeight, GS_RGBA);

	success = false;
	gs_texrender_reset(texRender);
	if (gs_texrender_begin(texRender, imgWidth, imgHeight)) {
		vec4 background;
		vec4_zero(&background);

		gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
		gs_ortho(0.0f, (float)sourceWidth, 0.0f, (float)sourceHeight, -100.0f, 100.0f);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		obs_source_inc_showing(source);
		obs_source_video_render(source);
		obs_source_dec_showing(source);

		gs_blend_state_pop();
		gs_texrender_end(texRender);

		gs_stage_texture(stageSurface, gs_texrender_get_texture(texRender));
		if (gs_stagesurface_map(stageSurface, &videoData, &videoLinesize)) {
			int lineSize = ret.bytesPerLine();
			for (uint y = 0; y < imgHeight; y++) {
				memcpy(ret.scanLine(y), videoData + (y * videoLinesize), lineSize);
			}
			gs_stagesurface_unmap(stageSurface);
			success = true;
		}
	}

	gs_stagesurface_destroy(stageSurface);
	gs_texrender_destroy(texRender);

	obs_leave_graphics();

	return ret;
}

bool IsImageFormatValid(std::string format)
{
	QByteArrayList supportedFormats = QImageWriter::supportedImageFormats();
	return supportedFormats.contains(format.c_str());
}

/**
 * Gets the active and show state of a source.
 *
 * **Compatible with inputs and scenes.**
 *
 * @requestField ?sourceName | String | Name of the source to get the active state of
 * @requestField ?sourceUuid | String | UUID of the source to get the active state of
 *
 * @responseField videoActive  | Boolean | Whether the source is showing in Program
 * @responseField videoShowing | Boolean | Whether the source is showing in the UI (Preview, Projector, Properties)
 *
 * @requestType GetSourceActive
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category sources
 */
RequestResult RequestHandler::GetSourceActive(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!source)
		return RequestResult::Error(statusCode, comment);

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT && obs_source_get_type(source) != OBS_SOURCE_TYPE_SCENE)
		return RequestResult::Error(RequestStatus::InvalidResourceType, "The specified source is not an input or a scene.");

	json responseData;
	responseData["videoActive"] = obs_source_active(source);
	responseData["videoShowing"] = obs_source_showing(source);
	return RequestResult::Success(responseData);
}

/**
 * Gets a Base64-encoded screenshot of a source.
 *
 * The `imageWidth` and `imageHeight` parameters are treated as "scale to inner", meaning the smallest ratio will be used and the aspect ratio of the original resolution is kept.
 * If `imageWidth` and `imageHeight` are not specified, the compressed image will use the full resolution of the source.
 *
 * **Compatible with inputs and scenes.**
 *
 * @requestField ?sourceName              | String | Name of the source to take a screenshot of
 * @requestField ?sourceUuid              | String | UUID of the source to take a screenshot of
 * @requestField imageFormat              | String | Image compression format to use. Use `GetVersion` to get compatible image formats
 * @requestField ?imageWidth              | Number | Width to scale the screenshot to                                                                                         | >= 8, <= 4096 | Source value is used
 * @requestField ?imageHeight             | Number | Height to scale the screenshot to                                                                                        | >= 8, <= 4096 | Source value is used
 * @requestField ?imageCompressionQuality | Number | Compression quality to use. 0 for high compression, 100 for uncompressed. -1 to use "default" (whatever that means, idk) | >= -1, <= 100 | -1
 *
 * @responseField imageData | String | Base64-encoded screenshot
 *
 * @requestType GetSourceScreenshot
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category sources
 */
RequestResult RequestHandler::GetSourceScreenshot(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!(source && request.ValidateString("imageFormat", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT && obs_source_get_type(source) != OBS_SOURCE_TYPE_SCENE)
		return RequestResult::Error(RequestStatus::InvalidResourceType, "The specified source is not an input or a scene.");

	std::string imageFormat = request.RequestData["imageFormat"];

	if (!IsImageFormatValid(imageFormat))
		return RequestResult::Error(RequestStatus::InvalidRequestField,
					    "Your specified image format is invalid or not supported by this system.");

	uint32_t requestedWidth{0};
	uint32_t requestedHeight{0};
	int compressionQuality{-1};

	if (request.Contains("imageWidth")) {
		if (!request.ValidateOptionalNumber("imageWidth", statusCode, comment, 8, 4096))
			return RequestResult::Error(statusCode, comment);

		requestedWidth = request.RequestData["imageWidth"];
	}

	if (request.Contains("imageHeight")) {
		if (!request.ValidateOptionalNumber("imageHeight", statusCode, comment, 8, 4096))
			return RequestResult::Error(statusCode, comment);

		requestedHeight = request.RequestData["imageHeight"];
	}

	if (request.Contains("imageCompressionQuality")) {
		if (!request.ValidateOptionalNumber("imageCompressionQuality", statusCode, comment, -1, 100))
			return RequestResult::Error(statusCode, comment);

		compressionQuality = request.RequestData["imageCompressionQuality"];
	}

	bool success;
	QImage renderedImage = TakeSourceScreenshot(source, success, requestedWidth, requestedHeight);

	if (!success)
		return RequestResult::Error(RequestStatus::RequestProcessingFailed, "Failed to render screenshot.");

	QByteArray encodedImgBytes;
	QBuffer buffer(&encodedImgBytes);
	buffer.open(QBuffer::WriteOnly);

	if (!renderedImage.save(&buffer, imageFormat.c_str(), compressionQuality))
		return RequestResult::Error(RequestStatus::RequestProcessingFailed, "Failed to encode screenshot.");

	buffer.close();

	QString encodedPicture = QString("data:image/%1;base64,").arg(imageFormat.c_str()).append(encodedImgBytes.toBase64());

	json responseData;
	responseData["imageData"] = encodedPicture.toStdString();
	return RequestResult::Success(responseData);
}

/**
 * Saves a screenshot of a source to the filesystem.
 *
 * The `imageWidth` and `imageHeight` parameters are treated as "scale to inner", meaning the smallest ratio will be used and the aspect ratio of the original resolution is kept.
 * If `imageWidth` and `imageHeight` are not specified, the compressed image will use the full resolution of the source.
 *
 * **Compatible with inputs and scenes.**
 *
 * @requestField ?sourceName              | String | Name of the source to take a screenshot of
 * @requestField ?sourceUuid              | String | UUID of the source to take a screenshot of
 * @requestField imageFormat              | String | Image compression format to use. Use `GetVersion` to get compatible image formats
 * @requestField imageFilePath            | String | Path to save the screenshot file to. Eg. `C:\Users\user\Desktop\screenshot.png`
 * @requestField ?imageWidth              | Number | Width to scale the screenshot to                                                                                         | >= 8, <= 4096 | Source value is used
 * @requestField ?imageHeight             | Number | Height to scale the screenshot to                                                                                        | >= 8, <= 4096 | Source value is used
 * @requestField ?imageCompressionQuality | Number | Compression quality to use. 0 for high compression, 100 for uncompressed. -1 to use "default" (whatever that means, idk) | >= -1, <= 100 | -1
 *
 * @requestType SaveSourceScreenshot
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category sources
 */
RequestResult RequestHandler::SaveSourceScreenshot(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!(source && request.ValidateString("imageFormat", statusCode, comment) &&
	      request.ValidateString("imageFilePath", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT && obs_source_get_type(source) != OBS_SOURCE_TYPE_SCENE)
		return RequestResult::Error(RequestStatus::InvalidResourceType, "The specified source is not an input or a scene.");

	std::string imageFormat = request.RequestData["imageFormat"];
	std::string imageFilePath = request.RequestData["imageFilePath"];

	if (!IsImageFormatValid(imageFormat))
		return RequestResult::Error(RequestStatus::InvalidRequestField,
					    "Your specified image format is invalid or not supported by this system.");

	QFileInfo filePathInfo(QString::fromStdString(imageFilePath));
	if (!filePathInfo.absoluteDir().exists())
		return RequestResult::Error(RequestStatus::ResourceNotFound, "The directory for your file path does not exist.");

	uint32_t requestedWidth{0};
	uint32_t requestedHeight{0};
	int compressionQuality{-1};

	if (request.Contains("imageWidth")) {
		if (!request.ValidateOptionalNumber("imageWidth", statusCode, comment, 8, 4096))
			return RequestResult::Error(statusCode, comment);

		requestedWidth = request.RequestData["imageWidth"];
	}

	if (request.Contains("imageHeight")) {
		if (!request.ValidateOptionalNumber("imageHeight", statusCode, comment, 8, 4096))
			return RequestResult::Error(statusCode, comment);

		requestedHeight = request.RequestData["imageHeight"];
	}

	if (request.Contains("imageCompressionQuality")) {
		if (!request.ValidateOptionalNumber("imageCompressionQuality", statusCode, comment, -1, 100))
			return RequestResult::Error(statusCode, comment);

		compressionQuality = request.RequestData["imageCompressionQuality"];
	}

	bool success;
	QImage renderedImage = TakeSourceScreenshot(source, success, requestedWidth, requestedHeight);

	if (!success)
		return RequestResult::Error(RequestStatus::RequestProcessingFailed, "Failed to render screenshot.");

	QString absoluteFilePath = filePathInfo.absoluteFilePath();

	if (!renderedImage.save(absoluteFilePath, imageFormat.c_str(), compressionQuality))
		return RequestResult::Error(RequestStatus::RequestProcessingFailed, "Failed to save screenshot.");

	return RequestResult::Success();
}

// Intentionally undocumented
RequestResult RequestHandler::GetSourcePrivateSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!source)
		return RequestResult::Error(statusCode, comment);

	OBSDataAutoRelease privateSettings = obs_source_get_private_settings(source);

	json responseData;
	responseData["sourceSettings"] = Utils::Json::ObsDataToJson(privateSettings);

	return RequestResult::Success(responseData);
}

// Intentionally undocumented
RequestResult RequestHandler::SetSourcePrivateSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!source || !request.ValidateObject("sourceSettings", statusCode, comment, true))
		return RequestResult::Error(statusCode, comment);

	OBSDataAutoRelease privateSettings = obs_source_get_private_settings(source);

	OBSDataAutoRelease newSettings = Utils::Json::JsonToObsData(request.RequestData["sourceSettings"]);

	// Always overlays to prevent destroying internal source data unintentionally
	obs_data_apply(privateSettings, newSettings);

	return RequestResult::Success();
}
