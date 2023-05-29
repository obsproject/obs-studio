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
     * Properties related to the RIST protocol
     */
    struct RistProperties {
        bool encryptPassphrase;
        bool srpUsernamePassword;
    };

    enum class ServerProtocol : int { HLS, RIST, RTMP, RTMPS, SRT };

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
         * Link that provides additional info about the service, presented in the UI as a button
         * next to the services dropdown.
         */
        std::optional<std::string> moreInfoLink;
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
         * Properties related to the RIST protocol
         */
        std::optional<RistProperties> rist;
        /**
         * Properties related to the SRT protocol
         */
        std::optional<SrtProperties> srt;
    };

    struct ServicesJson {
        std::vector<Service> services;
    };
}

namespace OBSServices {
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

    void from_json(const json & j, ServerProtocol & x);
    void to_json(json & j, const ServerProtocol & x);

    void from_json(const json & j, ProtocolSupportedAudioCodec & x);
    void to_json(json & j, const ProtocolSupportedAudioCodec & x);

    void from_json(const json & j, ProtocolSupportedVideoCodec & x);
    void to_json(json & j, const ProtocolSupportedVideoCodec & x);

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
        x.moreInfoLink = get_stack_optional<std::string>(j, "more_info_link");
        x.servers = j.at("servers").get<std::vector<Server>>();
        x.streamKeyLink = get_stack_optional<std::string>(j, "stream_key_link");
        x.supportedCodecs = get_stack_optional<SupportedCodecs>(j, "supported_codecs");
        x.rist = get_stack_optional<RistProperties>(j, "RIST");
        x.srt = get_stack_optional<SrtProperties>(j, "SRT");
    }

    inline void to_json(json & j, const Service & x) {
        j = json::object();
        j["common"] = x.common;
        j["id"] = x.id;
        j["name"] = x.name;
        j["more_info_link"] = x.moreInfoLink;
        j["servers"] = x.servers;
        j["stream_key_link"] = x.streamKeyLink;
        j["supported_codecs"] = x.supportedCodecs;
        j["RIST"] = x.rist;
        j["SRT"] = x.srt;
    }

    inline void from_json(const json & j, ServicesJson& x) {
        x.services = j.at("services").get<std::vector<Service>>();
    }

    inline void to_json(json & j, const ServicesJson & x) {
        j = json::object();
        j["services"] = x.services;
    }

    inline void from_json(const json & j, ServerProtocol & x) {
        if (j == "HLS") x = ServerProtocol::HLS;
        else if (j == "RIST") x = ServerProtocol::RIST;
        else if (j == "RTMP") x = ServerProtocol::RTMP;
        else if (j == "RTMPS") x = ServerProtocol::RTMPS;
        else if (j == "SRT") x = ServerProtocol::SRT;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, const ServerProtocol & x) {
        switch (x) {
            case ServerProtocol::HLS: j = "HLS"; break;
            case ServerProtocol::RIST: j = "RIST"; break;
            case ServerProtocol::RTMP: j = "RTMP"; break;
            case ServerProtocol::RTMPS: j = "RTMPS"; break;
            case ServerProtocol::SRT: j = "SRT"; break;
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
