#include "YoutubeApiWrappers.hpp"

#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>
#include <algorithm>
#include <utility>

#include "moc_YoutubeApiWrappers.cpp"

using namespace json11;

bool IsYouTubeService(const std::string &service)
{
	auto it = std::find_if(youtubeServices.begin(), youtubeServices.end(),
			       [&service](const Auth::Def &yt) { return service == yt.service; });
	return it != youtubeServices.end();
}
bool IsUserSignedIntoYT()
{
	Auth *auth = OBSBasic::Get()->GetAuth();
	if (auth) {
		YoutubeApiWrappers *apiYouTube(dynamic_cast<YoutubeApiWrappers *>(auth));
		if (apiYouTube) {
			return true;
		}
	}
	return false;
}

bool YoutubeApiWrappers::GetTranslatedError(QString &error_message)
{
	return apiClient.GetTranslatedError(error_message);
}

QString YoutubeApiWrappers::GetLastError() const
{
	return apiClient.GetLastError();
}

YoutubeApiWrappers::YoutubeApiWrappers(const Def &d)
	: YoutubeAuth(d),
	  apiClient({
		  [this]() { return token; },
		  [this]() { return refresh_token; },
		  [this](std::string newToken) { token = std::move(newToken); },
	  })
{
}

bool YoutubeApiWrappers::GetChannelDescription(ChannelDescription &channel_description)
{
	return apiClient.GetChannelDescription(channel_description);
}

bool YoutubeApiWrappers::InsertBroadcast(BroadcastDescription &broadcast)
{
	return apiClient.InsertBroadcast(broadcast);
}

bool YoutubeApiWrappers::InsertStream(StreamDescription &stream)
{
	return apiClient.InsertStream(stream);
}

bool YoutubeApiWrappers::BindStream(const QString broadcast_id, const QString stream_id)
{
	return apiClient.BindStream(broadcast_id, stream_id);
}

bool YoutubeApiWrappers::GetBroadcastsList(Json &json_out, const QString &page, const QString &status)
{
	return apiClient.GetBroadcastsList(json_out, page, status);
}

bool YoutubeApiWrappers::GetVideoCategoriesList(QVector<CategoryDescription> &category_list_out)
{
	return apiClient.GetVideoCategoriesList(category_list_out);
}

bool YoutubeApiWrappers::SetVideoCategory(const QString &video_id, const QString &video_title,
					  const QString &video_description, const QString &categorie_id)
{
	return apiClient.SetVideoCategory(video_id, video_title, video_description, categorie_id);
}

bool YoutubeApiWrappers::SetVideoThumbnail(const QString &video_id, const QString &thumbnail_file)
{
	return apiClient.SetVideoThumbnail(video_id, thumbnail_file);
}

bool YoutubeApiWrappers::StartBroadcast(const QString &broadcast_id)
{
	return apiClient.StartBroadcast(broadcast_id);
}

bool YoutubeApiWrappers::StartLatestBroadcast()
{
	return apiClient.StartLatestBroadcast();
}

bool YoutubeApiWrappers::StopBroadcast(const QString &broadcast_id)
{
	return apiClient.StopBroadcast(broadcast_id);
}

bool YoutubeApiWrappers::StopLatestBroadcast()
{
	return apiClient.StopLatestBroadcast();
}

void YoutubeApiWrappers::SetBroadcastId(const QString &broadcast_id)
{
	apiClient.SetBroadcastId(broadcast_id);
}

QString YoutubeApiWrappers::GetBroadcastId()
{
	return apiClient.GetBroadcastId();
}

bool YoutubeApiWrappers::ResetBroadcast(const QString &broadcast_id, json11::Json &json_out)
{
	return apiClient.ResetBroadcast(broadcast_id, json_out);
}

bool YoutubeApiWrappers::FindBroadcast(const QString &id, json11::Json &json_out)
{
	return apiClient.FindBroadcast(id, json_out);
}

bool YoutubeApiWrappers::FindStream(const QString &id, json11::Json &json_out)
{
	return apiClient.FindStream(id, json_out);
}
