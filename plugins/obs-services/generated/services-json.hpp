//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     ServicesJson data = nlohmann::json::parse(jsonString);

#pragma once

#include <optional>
#include <nlohmann/json.hpp>

#ifndef NLOHMANN_OPT_HELPER
#define NLOHMANN_OPT_HELPER
namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::shared_ptr<T>> {
        static void to_json(json & j, const std::shared_ptr<T> & opt) {
            if (!opt) j = nullptr; else j = *opt;
        }

        static std::shared_ptr<T> from_json(const json & j) {
            if (j.is_null()) return std::make_shared<T>(); else return std::make_shared<T>(j.get<T>());
        }
    };
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json & j, const std::optional<T> & opt) {
            if (!opt) j = nullptr; else j = *opt;
        }

        static std::optional<T> from_json(const json & j) {
            if (j.is_null()) return std::make_optional<T>(); else return std::make_optional<T>(j.get<T>());
        }
    };
}
#endif

namespace OBSServices {
    using nlohmann::json;

    #ifndef NLOHMANN_UNTYPED_OBSServices_HELPER
    #define NLOHMANN_UNTYPED_OBSServices_HELPER
    inline json get_untyped(const json & j, const char * property) {
        if (j.find(property) != j.end()) {
            return j.at(property).get<json>();
        }
        return json();
    }

    inline json get_untyped(const json & j, std::string property) {
        return get_untyped(j, property.data());
    }
    #endif

    #ifndef NLOHMANN_OPTIONAL_OBSServices_HELPER
    #define NLOHMANN_OPTIONAL_OBSServices_HELPER
    template <typename T>
    inline std::shared_ptr<T> get_heap_optional(const json & j, const char * property) {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null()) {
            return j.at(property).get<std::shared_ptr<T>>();
        }
        return std::shared_ptr<T>();
    }

    template <typename T>
    inline std::shared_ptr<T> get_heap_optional(const json & j, std::string property) {
        return get_heap_optional<T>(j, property.data());
    }
    template <typename T>
    inline std::optional<T> get_stack_optional(const json & j, const char * property) {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null()) {
            return j.at(property).get<std::optional<T>>();
        }
        return std::optional<T>();
    }

    template <typename T>
    inline std::optional<T> get_stack_optional(const json & j, std::string property) {
        return get_stack_optional<T>(j, property.data());
    }
    #endif

    /**
     * Maximum values allowed by the service
     */
    struct Maximums {
        /**
         * Maximum audio bitrate per codec
         */
        std::optional<std::map<std::string, int64_t>> audioBitrate;
        std::optional<int64_t> framerate;
        /**
         * Maximum video bitrate per codec
         */
        std::optional<std::map<std::string, int64_t>> videoBitrate;
        /**
         * Maximum video bitrate per supported resolution with framerate and per codec
         */
        std::optional<std::map<std::string, std::map<std::string, int64_t>>> videoBitrateMatrix;
    };

    enum class H264Profile : int { BASELINE, HIGH, MAIN };

    /**
     * Properties related to the H264 codec
     */
    struct H264Settings {
        std::optional<int64_t> bframes;
        std::optional<int64_t> keyint;
        std::optional<H264Profile> profile;
    };

    /**
     * Settings that are applied only if the user wants to do so.
     */
    struct RecommendedSettings {
        /**
         * Properties related to the H264 codec
         */
        std::optional<H264Settings> h264;
        /**
         * Options meant for the x264 encoder implementation with the id 'obs_x264'
         */
        std::optional<std::string> obsX264;
    };

    /**
     * Properties related to the RIST protocol
     */
    struct RistProperties {
        bool encryptPassphrase;
        bool srpUsernamePassword;
    };

    enum class ServerProtocol : int { HLS, RIST, RTMP, RTMPS, SRT, WHIP };

    struct Server {
        /**
         * Name of the server (e.g. location, primary/backup), displayed in the Server dropdown.
         */
        std::string name;
        /**
         * Protocol used by the server. Required if the URI scheme can't help identify the streaming
         * protocol (e.g. HLS).
         */
        ServerProtocol protocol;
        /**
         * URL of the ingest server.
         */
        std::string url;
    };

    /**
     * Properties related to the SRT protocol
     */
    struct SrtProperties {
        bool encryptPassphrase;
        bool streamId;
    };

    /**
     * Audio codecs that are supported by the service on this protocol.
     */
    enum class ProtocolSupportedAudioCodec : int { AAC, OPUS };

    /**
     * Video codecs that are supported by the service on this protocol.
     */
    enum class ProtocolSupportedVideoCodec : int { AV1, H264, HEVC };

    /**
     * Codecs that are supported by the service.
     */
    struct SupportedCodecs {
        /**
         * Audio codecs that are supported by the service.
         */
        std::optional<std::map<std::string, std::vector<ProtocolSupportedAudioCodec>>> audio;
        /**
         * Video codecs that are supported by the service.
         */
        std::optional<std::map<std::string, std::vector<ProtocolSupportedVideoCodec>>> video;
    };

    struct Service {
        /**
         * Whether or not the service is shown in the list before it is expanded to all services by
         * the user.
         */
        std::optional<bool> common;
        /**
         * Human readable identifier used to register the service in OBS
         * Making it human readable is meant to allow users to use it through scripts and plugins
         */
        std::string id;
        /**
         * Name of the streaming service. Will be displayed in the Service dropdown.
         */
        std::string name;
        /**
         * Maximum values allowed by the service
         */
        std::optional<Maximums> maximums;
        /**
         * Link that provides additional info about the service, presented in the UI as a button
         * next to the services dropdown.
         */
        std::optional<std::string> moreInfoLink;
        /**
         * Settings that are applied only if the user wants to do so.
         */
        std::optional<RecommendedSettings> recommended;
        /**
         * Array of server objects
         */
        std::vector<Server> servers;
        /**
         * Link where a logged-in user can find the 'stream key', presented as a button alongside
         * the stream key field.
         */
        std::optional<std::string> streamKeyLink;
        /**
         * Codecs that are supported by the service.
         */
        std::optional<SupportedCodecs> supportedCodecs;
        /**
         * Resolution supported by the service. All with or all withtout the framerate.
         */
        std::optional<std::vector<std::string>> supportedResolutions;
        /**
         * Properties related to the RIST protocol
         */
        std::optional<RistProperties> rist;
        /**
         * Properties related to the SRT protocol
         */
        std::optional<SrtProperties> srt;
    };

    struct ServicesJson {
        int64_t formatVersion;
        std::vector<Service> services;
    };
}

namespace OBSServices {
    void from_json(const json & j, Maximums & x);
    void to_json(json & j, const Maximums & x);

    void from_json(const json & j, H264Settings & x);
    void to_json(json & j, const H264Settings & x);

    void from_json(const json & j, RecommendedSettings & x);
    void to_json(json & j, const RecommendedSettings & x);

    void from_json(const json & j, RistProperties & x);
    void to_json(json & j, const RistProperties & x);

    void from_json(const json & j, Server & x);
    void to_json(json & j, const Server & x);

    void from_json(const json & j, SrtProperties & x);
    void to_json(json & j, const SrtProperties & x);

    void from_json(const json & j, SupportedCodecs & x);
    void to_json(json & j, const SupportedCodecs & x);

    void from_json(const json & j, Service & x);
    void to_json(json & j, const Service & x);

    void from_json(const json & j, ServicesJson & x);
    void to_json(json & j, const ServicesJson & x);

    void from_json(const json & j, H264Profile & x);
    void to_json(json & j, const H264Profile & x);

    void from_json(const json & j, ServerProtocol & x);
    void to_json(json & j, const ServerProtocol & x);

    void from_json(const json & j, ProtocolSupportedAudioCodec & x);
    void to_json(json & j, const ProtocolSupportedAudioCodec & x);

    void from_json(const json & j, ProtocolSupportedVideoCodec & x);
    void to_json(json & j, const ProtocolSupportedVideoCodec & x);

    inline void from_json(const json & j, Maximums& x) {
        x.audioBitrate = get_stack_optional<std::map<std::string, int64_t>>(j, "audio_bitrate");
        x.framerate = get_stack_optional<int64_t>(j, "framerate");
        x.videoBitrate = get_stack_optional<std::map<std::string, int64_t>>(j, "video_bitrate");
        x.videoBitrateMatrix = get_stack_optional<std::map<std::string, std::map<std::string, int64_t>>>(j, "video_bitrate_matrix");
    }

    inline void to_json(json & j, const Maximums & x) {
        j = json::object();
        j["audio_bitrate"] = x.audioBitrate;
        j["framerate"] = x.framerate;
        j["video_bitrate"] = x.videoBitrate;
        j["video_bitrate_matrix"] = x.videoBitrateMatrix;
    }

    inline void from_json(const json & j, H264Settings& x) {
        x.bframes = get_stack_optional<int64_t>(j, "bframes");
        x.keyint = get_stack_optional<int64_t>(j, "keyint");
        x.profile = get_stack_optional<H264Profile>(j, "profile");
    }

    inline void to_json(json & j, const H264Settings & x) {
        j = json::object();
        j["bframes"] = x.bframes;
        j["keyint"] = x.keyint;
        j["profile"] = x.profile;
    }

    inline void from_json(const json & j, RecommendedSettings& x) {
        x.h264 = get_stack_optional<H264Settings>(j, "h264");
        x.obsX264 = get_stack_optional<std::string>(j, "obs_x264");
    }

    inline void to_json(json & j, const RecommendedSettings & x) {
        j = json::object();
        j["h264"] = x.h264;
        j["obs_x264"] = x.obsX264;
    }

    inline void from_json(const json & j, RistProperties& x) {
        x.encryptPassphrase = j.at("encrypt_passphrase").get<bool>();
        x.srpUsernamePassword = j.at("srp_username_password").get<bool>();
    }

    inline void to_json(json & j, const RistProperties & x) {
        j = json::object();
        j["encrypt_passphrase"] = x.encryptPassphrase;
        j["srp_username_password"] = x.srpUsernamePassword;
    }

    inline void from_json(const json & j, Server& x) {
        x.name = j.at("name").get<std::string>();
        x.protocol = j.at("protocol").get<ServerProtocol>();
        x.url = j.at("url").get<std::string>();
    }

    inline void to_json(json & j, const Server & x) {
        j = json::object();
        j["name"] = x.name;
        j["protocol"] = x.protocol;
        j["url"] = x.url;
    }

    inline void from_json(const json & j, SrtProperties& x) {
        x.encryptPassphrase = j.at("encrypt_passphrase").get<bool>();
        x.streamId = j.at("stream_id").get<bool>();
    }

    inline void to_json(json & j, const SrtProperties & x) {
        j = json::object();
        j["encrypt_passphrase"] = x.encryptPassphrase;
        j["stream_id"] = x.streamId;
    }

    inline void from_json(const json & j, SupportedCodecs& x) {
        x.audio = get_stack_optional<std::map<std::string, std::vector<ProtocolSupportedAudioCodec>>>(j, "audio");
        x.video = get_stack_optional<std::map<std::string, std::vector<ProtocolSupportedVideoCodec>>>(j, "video");
    }

    inline void to_json(json & j, const SupportedCodecs & x) {
        j = json::object();
        j["audio"] = x.audio;
        j["video"] = x.video;
    }

    inline void from_json(const json & j, Service& x) {
        x.common = get_stack_optional<bool>(j, "common");
        x.id = j.at("id").get<std::string>();
        x.name = j.at("name").get<std::string>();
        x.maximums = get_stack_optional<Maximums>(j, "maximums");
        x.moreInfoLink = get_stack_optional<std::string>(j, "more_info_link");
        x.recommended = get_stack_optional<RecommendedSettings>(j, "recommended");
        x.servers = j.at("servers").get<std::vector<Server>>();
        x.streamKeyLink = get_stack_optional<std::string>(j, "stream_key_link");
        x.supportedCodecs = get_stack_optional<SupportedCodecs>(j, "supported_codecs");
        x.supportedResolutions = get_stack_optional<std::vector<std::string>>(j, "supported_resolutions");
        x.rist = get_stack_optional<RistProperties>(j, "RIST");
        x.srt = get_stack_optional<SrtProperties>(j, "SRT");
    }

    inline void to_json(json & j, const Service & x) {
        j = json::object();
        j["common"] = x.common;
        j["id"] = x.id;
        j["name"] = x.name;
        j["maximums"] = x.maximums;
        j["more_info_link"] = x.moreInfoLink;
        j["recommended"] = x.recommended;
        j["servers"] = x.servers;
        j["stream_key_link"] = x.streamKeyLink;
        j["supported_codecs"] = x.supportedCodecs;
        j["supported_resolutions"] = x.supportedResolutions;
        j["RIST"] = x.rist;
        j["SRT"] = x.srt;
    }

    inline void from_json(const json & j, ServicesJson& x) {
        x.formatVersion = j.at("format_version").get<int64_t>();
        x.services = j.at("services").get<std::vector<Service>>();
    }

    inline void to_json(json & j, const ServicesJson & x) {
        j = json::object();
        j["format_version"] = x.formatVersion;
        j["services"] = x.services;
    }

    inline void from_json(const json & j, H264Profile & x) {
        if (j == "baseline") x = H264Profile::BASELINE;
        else if (j == "high") x = H264Profile::HIGH;
        else if (j == "main") x = H264Profile::MAIN;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const H264Profile & x) {
        switch (x) {
            case H264Profile::BASELINE: j = "baseline"; break;
            case H264Profile::HIGH: j = "high"; break;
            case H264Profile::MAIN: j = "main"; break;
            default: throw std::runtime_error("This should not happen");
        }
    }

    inline void from_json(const json & j, ServerProtocol & x) {
        if (j == "HLS") x = ServerProtocol::HLS;
        else if (j == "RIST") x = ServerProtocol::RIST;
        else if (j == "RTMP") x = ServerProtocol::RTMP;
        else if (j == "RTMPS") x = ServerProtocol::RTMPS;
        else if (j == "SRT") x = ServerProtocol::SRT;
        else if (j == "WHIP") x = ServerProtocol::WHIP;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const ServerProtocol & x) {
        switch (x) {
            case ServerProtocol::HLS: j = "HLS"; break;
            case ServerProtocol::RIST: j = "RIST"; break;
            case ServerProtocol::RTMP: j = "RTMP"; break;
            case ServerProtocol::RTMPS: j = "RTMPS"; break;
            case ServerProtocol::SRT: j = "SRT"; break;
            case ServerProtocol::WHIP: j = "WHIP"; break;
            default: throw std::runtime_error("This should not happen");
        }
    }

    inline void from_json(const json & j, ProtocolSupportedAudioCodec & x) {
        if (j == "aac") x = ProtocolSupportedAudioCodec::AAC;
        else if (j == "opus") x = ProtocolSupportedAudioCodec::OPUS;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const ProtocolSupportedAudioCodec & x) {
        switch (x) {
            case ProtocolSupportedAudioCodec::AAC: j = "aac"; break;
            case ProtocolSupportedAudioCodec::OPUS: j = "opus"; break;
            default: throw std::runtime_error("This should not happen");
        }
    }

    inline void from_json(const json & j, ProtocolSupportedVideoCodec & x) {
        if (j == "av1") x = ProtocolSupportedVideoCodec::AV1;
        else if (j == "h264") x = ProtocolSupportedVideoCodec::H264;
        else if (j == "hevc") x = ProtocolSupportedVideoCodec::HEVC;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const ProtocolSupportedVideoCodec & x) {
        switch (x) {
            case ProtocolSupportedVideoCodec::AV1: j = "av1"; break;
            case ProtocolSupportedVideoCodec::H264: j = "h264"; break;
            case ProtocolSupportedVideoCodec::HEVC: j = "hevc"; break;
            default: throw std::runtime_error("This should not happen");
        }
    }
}
