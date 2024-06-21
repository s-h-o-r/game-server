#pragma once

#include <boost/json.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include "app.h"
#include "http_server.h"
#include "logger.h"
#include "model.h"
#include "player.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace http_handler {

namespace fs = std::filesystem;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

using Strand = net::strand<net::io_context::executor_type>;

enum class Extention {
    empty, unkown,
    htm, html, css, txt, js, json, xml,
    png, jpg, jpe, jpeg, gif, bmp, ico,
    tiff, tif, svg, svgz, mp3
};

class Uri {
public:
    explicit Uri(std::string_view uri, const fs::path& base);

    const fs::path& GetCanonicalUri() const;

    Extention GetFileExtention() const;

    bool IsSubPath(const fs::path& base) const;

private:
    fs::path uri_;
    fs::path canonical_uri_;

    std::string EncodeUri(std::string_view uri) const;
};

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view APP_JSON = "application/json";
    constexpr static std::string_view APP_XML = "application/xml";
    constexpr static std::string_view APP_BINARY = "application/octet-stream";

    constexpr static std::string_view TXT_HTML = "text/html";
    constexpr static std::string_view TXT_CSS = "text/css";
    constexpr static std::string_view TXT_PLAIN = "text/plain";
    constexpr static std::string_view TXT_JS = "text/javascript";

    constexpr static std::string_view IMG_PNG = "image/png";
    constexpr static std::string_view IMG_JPEG = "image/jpeg";
    constexpr static std::string_view IMG_GIF = "image/gif";
    constexpr static std::string_view IMG_BMP = "image/bmp";
    constexpr static std::string_view IMG_ICON = "image/vnd.microsoft.icon";
    constexpr static std::string_view IMG_TIFF = "image/tiff";
    constexpr static std::string_view IMG_SVG_XML = "image/svg+xml";

    constexpr static std::string_view AUDIO_MPEG = "audio/mpeg";
};

template <typename Request, typename Response>
void FillBasicInfo(Request& req, Response& response) {
    response.version(req.version());
    response.keep_alive(req.keep_alive());
}

std::string_view GetMimeType(Extention extention);
std::string ParseMapToJson(const model::Map* map);
std::unordered_map<std::string, std::string> ParseQuery(std::string_view query);

using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;

class ApiRequestHandler : public std::enable_shared_from_this<ApiRequestHandler> {
public:

    explicit ApiRequestHandler(app::Application& app, bool manual_update)
        : app_(app)
        , manual_update_(manual_update) {
    }

    ApiRequestHandler(const ApiRequestHandler&) = delete;
    ApiRequestHandler& operator=(const ApiRequestHandler&) = delete;

    template <typename Request, typename Send>
    void operator() (Request&& req, Send&& send) {
        SendApiResponse(std::forward<Request>(req), std::forward<Send>(send), req.target());
    }

private:
    app::Application& app_;
    bool manual_update_;

    template <typename Request, typename Send>
    void SendApiResponse(Request&& req, Send&& send, std::string_view target) {
        using namespace std::literals;

        StringResponse response;
        FillBasicInfo(req, response);
        response.set(http::field::cache_control, "no-cache");
        try {

            if (target.substr(0, 12) == "/api/v1/maps"sv) {
                switch (req.method()) {
                    case http::verb::get:
                    case http::verb::head:
                        ProcessApiMaps(response, target);
                        break;
                    default:
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_get_head,
                                             "Invalid method"sv);
                        break;
                }
            } else if (target.substr(0, 20) == "/api/v1/game/players"sv) {
                switch (req.method()) {
                    case http::verb::get:
                    case http::verb::head:
                        ProcessApiPlayers(req, response);
                        break;
                    default:
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_get_head,
                                             "Invalid method"sv);
                        break;
                }
            } else if (target.substr(0, 17) == "/api/v1/game/join"sv) {
                switch (req.method()) {
                    case http::verb::post:
                        ProcessApiJoin(req, response);
                        break;
                    default:
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_post,
                                             "Only POST method is expected"sv);
                        break;
                }
            } else if (target.substr(0, 18) == "/api/v1/game/state"sv) {
                switch (req.method()) {
                    case http::verb::get:
                    case http::verb::head:
                        ProcessApiGameState(req, response);
                        break;
                    default:
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_get_head,
                                             "Invalid method"sv);
                        break;
                }
            } else if (target.substr(0, 26) == "/api/v1/game/player/action"sv) {
                switch (req.method()) {
                    case http::verb::post:
                        ProcessApiAction(req, response);
                        break;

                    default:
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_post,
                                             "Invalid method"sv);
                        break;
                }
            } else if (target.substr(0, 17) == "/api/v1/game/tick"sv) {
                switch (req.method()) {
                    case http::verb::post:
                        if (manual_update_) {
                            ProcessApiTick(req, response);
                        } else {
                            MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::bad_request,
                                                 "Invalid endpoint"sv);
                        }
                        break;

                    default:
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_post,
                                             "Invalid method"sv);
                        break;
                }
            } else if (target.substr(0, 20) == "/api/v1/game/records"sv) {
                switch (req.method()) {
                    case http::verb::get:
                    case http::verb::head:
                        ProcessGetRecords(req, response);
                        break;
                    default:
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_get_head,
                                             "Invalid method"sv);
                }
            } else {
                MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::bad_request,
                                     "Bad request"sv);
            }
        } catch (...) {
            MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::bad_request,
                                 "Uknown error"sv);
            throw;
        }

        send(response);
    }

    void ProcessApiMaps(StringResponse& response, std::string_view target) const;

    template <typename Request>
    void ProcessApiPlayers(Request& request, StringResponse& response) const {

        ExecuteAuthorized(request, response, [self = shared_from_this(), &response](std::string_view token) {
            const auto& players = self->app_.ListPlayers(token);
            json::object players_on_map_json;
            for (const auto& [id, player] : players) {
                players_on_map_json[std::to_string(*id)] = {{"name", player->GetName()}};
            }

            response.body() = json::serialize(json::value(std::move(players_on_map_json)));

            response.set(http::field::content_type, ContentType::APP_JSON);
            response.content_length(response.body().size());
            response.result(http::status::ok);
        });
    }

    template <typename Request>
    void ProcessApiJoin(Request& request, StringResponse& response) {
        using namespace std::literals;

        boost::system::error_code ec;
        json::value request_body = json::parse(request.body(), ec);
        if (ec || !(request_body.if_object() && request_body.as_object().count("userName") 
                    && request_body.as_object().count("mapId"))) {
            MakeErrorApiResponse(response, ErrorCode::invalid_argument, "Join game request parse error"sv);
            return;
        }

        try {
            std::string user_name = std::string(request_body.as_object().at("userName").as_string());
            std::string map_id = std::string(request_body.as_object().at("mapId").as_string());
            
            auto join_result = app_.JoinGame(user_name, map_id);

            json::value jv = {
                {"authToken", *join_result.token},
                {"playerId", *join_result.player_id}
            };

            response.body() = json::serialize(jv);

        } catch (const app::JoinGameError& error) {
            switch (error.reason) {
                case app::JoinGameErrorReason::invalidMap:
                    MakeErrorApiResponse(response, ErrorCode::map_not_found, error.what());
                    return;
                case app::JoinGameErrorReason::invalidName:
                    MakeErrorApiResponse(response, ErrorCode::invalid_argument, error.what());
                    return;
            }
        }

        response.set(http::field::content_type, ContentType::APP_JSON);
        response.content_length(response.body().size());
        response.result(http::status::ok);
    }

    template <typename Request>
    void ProcessApiGameState(Request& request, StringResponse& response) {

        ExecuteAuthorized(request, response, [self = shared_from_this(), &response](std::string_view token) {
            const auto& dogs = self->app_.ListPlayers(token);

            json::object game_state_json;
            game_state_json["players"].emplace_object();

            for (const auto& [id, dog] : dogs) {
                const geom::Point2D& pos = dog->GetPosition();
                const geom::Vec2D& speed = dog->GetSpeed();

                game_state_json["players"].as_object().insert_or_assign(std::to_string(*id), json::object{
                    {"pos", {pos.x, pos.y}},
                    {"speed", {speed.x, speed.y}},
                    {"dir", model::DirectionToString(dog->GetDirection())},
                    {"score", dog->GetScore()}
                });

                auto& player_obj = game_state_json["players"].as_object()[std::to_string(*id)].as_object();
                auto& bag = player_obj["bag"].emplace_array();
                for (const auto& loot : dog->GetBag()->GetAllLoot()) {
                    bag.emplace_back(json::object{{"id", *loot.id},
                        {"type", loot.type}});
                }
            }

            game_state_json["lostObjects"].emplace_object();
            for (const auto& [id, loot_ptr] : self->app_.GetPlayerGameSession(token)->GetAllLoot()) {
                game_state_json["lostObjects"].as_object().insert_or_assign(std::to_string(*id), json::object{
                    {"type", loot_ptr->type},
                    {"pos", {loot_ptr->point.x, loot_ptr->point.y}}
                });
            }

            response.body() = json::serialize(json::value(std::move(game_state_json)));

            response.set(http::field::content_type, ContentType::APP_JSON);
            response.content_length(response.body().size());
            response.result(http::status::ok);
        });
    }

    template <typename Request>
    void ProcessApiAction(Request& request, StringResponse& response) {
        using namespace std::literals;

        ExecuteAuthorized(request, response,
                          [self = shared_from_this(), &request, &response] (std::string_view token) {

            if (!request.count(http::field::content_type)) {
                self->MakeErrorApiResponse(response, ErrorCode::invalid_argument,
                                           "Invalid content type"sv);
                return;
            }

            boost::system::error_code ec;

            json::value request_body = json::parse(request.body(), ec);
            if (ec || !(request_body.if_object() && request_body.as_object().count("move"))
                   || !self->app_.MoveDog(token, request_body.as_object().at("move").as_string())) {
                self->MakeErrorApiResponse(response, ErrorCode::invalid_argument, "Failed to parse action"sv);
                return;
            }

            response.body() = "{}"sv;

            response.set(http::field::content_type, ContentType::APP_JSON);
            response.content_length(response.body().size());
            response.result(http::status::ok);
        });
    }

    template <typename Request>
    void ProcessApiTick(Request& request, StringResponse& response) {
        using namespace std::literals;

        if (!request.count(http::field::content_type)) {
            MakeErrorApiResponse(response, ErrorCode::invalid_argument,
                                 "Invalid content type"sv);
            return;
        }

        boost::system::error_code ec;

        json::value request_body = json::parse(request.body(), ec);
        if (ec || !(request_body.if_object() && request_body.as_object().count("timeDelta"))
               || !request_body.as_object().at("timeDelta").is_int64()) {
            MakeErrorApiResponse(response, ErrorCode::invalid_argument,
                                       "Failed to parse tick request JSON"sv);
            return;
        }

        try {
            app_.ProcessTick(request_body.as_object().at("timeDelta").as_int64());
        } catch (const std::exception& e) {
            response.body() = "{\"message\":\""s + e.what() + "\"}"s;
        }

        response.body() = "{}"sv;

        response.set(http::field::content_type, ContentType::APP_JSON);
        response.content_length(response.body().size());
        response.result(http::status::ok);
    }

    template <typename Request>
    void ProcessGetRecords(Request& request, StringResponse& response) {
        using namespace std::literals;

        auto target = request.target();
        size_t delim_params = target.find('?');
        auto params = ParseQuery(target.substr(delim_params + 1));

        int start = 0;
        if (params.contains("start"s)) {
            start = std::stoi(params.at("start"s));
        }

        int max_items = 100;
        if (params.contains("maxItems"s)) {
            max_items = std::stoi(params.at("maxItems"s));
        }

        if (max_items > 100) {
            MakeErrorApiResponse(response, ErrorCode::bad_request, "Imposible to show more then 100 players on 1 list"sv);
        }

        json::array body;

        try {
            auto leaderboard = app_.GetLeaders(start, max_items);
            double second_multiplier = 1000.;
            for (const auto& player_info : leaderboard) {
                body.push_back(json::object{
                    {"name", player_info.GetName()},
                    {"score", player_info.GetScore()},
                    {"playTime", static_cast<double>(player_info.GetPlayTimeInMs()/second_multiplier)}
                });
            }
        } catch (const std::exception& e) {
            body.push_back({"message", e.what()});
        }

        response.body() = json::serialize(json::value(body));

        response.set(http::field::content_type, ContentType::APP_JSON);
        response.content_length(response.body().size());
        response.result(http::status::ok);
    }

    enum class ErrorCode {
        map_not_found, invalid_method_get_head, invalid_method_post,
        invalid_argument, bad_request, invalid_token, unknown_token
    };

    template <typename Request, typename Executor>
    void ExecuteAuthorized(Request& request, StringResponse& response, Executor&& executor) const {
        using namespace std::literals;

        try {
            std::string_view token = GetRawTokenValue(request);
            if (!app_.IsTokenValid(token)) {
                MakeErrorApiResponse(response, ErrorCode::unknown_token, "Player token has not been found"sv);
                return;
            }
            executor(token);
        } catch (const ErrorCode ec) {
            switch (ec) {
                case ErrorCode::invalid_token:
                    MakeErrorApiResponse(response, ErrorCode::invalid_token, "Authorization header is required"sv);
                    break;
                default:
                    MakeErrorApiResponse(response, ErrorCode::bad_request, "Unknown error code while getting raw token"sv);
                    break;
            }
            return;
        }
    }

    template <typename Request>
    std::string_view GetRawTokenValue(Request& request) const {
        if (!request.base().count("Authorization")) {
            throw ErrorCode::invalid_token;
        }

        auto auth_content = request.base().at("Authorization");
        size_t auth_header_size = 39;
        if (auth_content.size() < auth_header_size || auth_content.size() > auth_header_size) { // size of Bearer + ' ' + token.size()
            throw ErrorCode::invalid_token;
        }

        size_t prefix_size = 7;
        std::string_view token_prefix = auth_content.substr(0, prefix_size);
        std::string_view token = auth_content.substr(prefix_size); // Extract token (delete Bearer before token)

        if (std::size_t token_size = 32; token_prefix != "Bearer " || token.size() != token_size) {
            throw ErrorCode::invalid_token;
        }
        return token;
    }

    void MakeErrorApiResponse(StringResponse& response, ApiRequestHandler::ErrorCode code,
                              std::string_view message) const;
};

class StaticRequestHandler {
public:
    explicit StaticRequestHandler(std::filesystem::path&& static_files_path)
    : static_files_path_(std::move(static_files_path)) {
    }

    StaticRequestHandler(const StaticRequestHandler&) = delete;
    StaticRequestHandler& operator=(const StaticRequestHandler&) = delete;

    template <typename Request, typename Send>
    void operator() (Request&& req, Send&& send) {
        try {
            std::string_view target = req.target();
            SendStaticFile(std::forward<Request>(req), std::forward<Send>(send), target);
        } catch (...) {
            send(MakeErrorStaticFileResponse(http::status::unknown));
        }
    }

private:
    std::filesystem::path static_files_path_;

    template <typename Request, typename Send>
    void SendStaticFile(Request&& req, Send&& send, std::string_view target) const {
        FileResponse response;
        FillBasicInfo(req, response);

        http::status status{};

        switch (req.method()) {
            case http::verb::get:
            case http::verb::head:
                status = ProcessStaticFileTarget(response, target);
                break;

            default:
                auto error_response = MakeErrorStaticFileResponse(http::status::method_not_allowed);
                FillBasicInfo(req, error_response);
                send(error_response);
                return;
        }

        if (status != http::status::ok) {
            auto error_response = MakeErrorStaticFileResponse(status);
            FillBasicInfo(req, error_response);
            send(error_response);
        } else {
            send(response);
        }
    }

    http::status ProcessStaticFileTarget(FileResponse& response, std::string_view target) const;
    StringResponse MakeErrorStaticFileResponse(http::status status) const;
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    explicit RequestHandler(app::Application& app, net::io_context& ioc, Strand& api_strand,
                            std::filesystem::path&& static_files_path, bool manual_update)
        : ioc_(ioc)
        , api_strand_(api_strand)
        , api_handler_(std::make_shared<ApiRequestHandler>(app, manual_update))
        , static_handler_(std::move(fs::canonical(static_files_path))) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        using namespace std::literals;

        std::string_view target = req.target();
        if (target.size() >= 4 && target.substr(0, 5) == "/api/"sv) {
            net::dispatch(api_strand_, [self = shared_from_this(),
                                       req = std::forward<decltype(req)>(req),
                                       send = std::forward<Send>(send)]() {
                (*self->api_handler_)(req, send);
            });
        } else {
            static_handler_(std::forward<decltype(req)>(req), std::forward<Send>(send));
        }
    }

private:
    net::io_context& ioc_;
    Strand& api_strand_;
    std::shared_ptr<ApiRequestHandler> api_handler_;
    StaticRequestHandler static_handler_;
};

}  // namespace http_handler
