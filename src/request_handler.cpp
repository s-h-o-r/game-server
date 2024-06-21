#include "request_handler.h"

namespace extra_data {
void tag_invoke(json::value_from_tag, json::value& jv, const LootType& loot_type) {
    jv = {loot_type.loot_info};
}
} // namespace extra_data

namespace model {

namespace json = boost::json;

void tag_invoke(json::value_from_tag, json::value& jv, const Building& building) {
    auto bounds = building.GetBounds();
    jv = {
        {"x", bounds.position.x}, {"y", bounds.position.y},
        {"w", bounds.size.width}, {"h", bounds.size.height}
    };
}

void tag_invoke(json::value_from_tag, json::value& jv, const Office& office) {
    auto position = office.GetPosition();
    auto offset = office.GetOffset();
    jv = {
        {"id", *office.GetId()}, {"x", position.x}, {"y", position.y},
        {"offsetX", offset.dx}, {"offsetY", offset.dy}
    };
}

void tag_invoke(json::value_from_tag, json::value& jv, const model::Road& road) {
    auto start = road.GetStart();
    auto end = road.GetEnd();
    if (road.IsVertical()) {
        jv = {
            {"x0", start.x}, {"y0", start.y}, {"y1", end.y}
        };
    } else {
        jv = {
            {"x0", start.x}, {"y0", start.y}, {"x1", end.x}
        };
    }
}

void tag_invoke(json::value_from_tag, json::value& jv, const model::Map& map) {
    jv = {
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", json::value_from(map.GetRoads())},
        {"buildings", json::value_from(map.GetBuildings())},
        {"offices", json::value_from(map.GetOffices())},
        {"lootTypes", json::value_from(map.GetLootTypes())}
    };
}
} // namespace model

namespace http_handler {

using namespace std::literals;

namespace detail {

static std::string EncodeUriSpaces(std::string_view sub_uri) {
    std::string encoded_sub_uri;

    size_t pos = 0;
    while (pos != std::string_view::npos) {
        size_t space_pos = sub_uri.find_first_of('+', pos);
        if (space_pos != std::string_view::npos) {
            encoded_sub_uri += std::string{sub_uri.substr(pos, space_pos - pos)};
            encoded_sub_uri += ' ';
            pos = space_pos + 1;
        } else {
            encoded_sub_uri += std::string{sub_uri.substr(pos)};
            pos = space_pos;
        }
    }
    return encoded_sub_uri;
}

}; // namespace http_handler::detail

Uri::Uri(std::string_view uri, const fs::path& base)
    : uri_(base / ((uri == "/"sv || uri == ""sv) ? "index.html"sv : EncodeUri(uri)))
    , canonical_uri_(fs::weakly_canonical(uri_)) {
}

const fs::path& Uri::GetCanonicalUri() const {
    return canonical_uri_;
}

Extention Uri::GetFileExtention() const {
    auto extention = canonical_uri_.extension().string();
    if (extention.empty()) {
        return Extention::empty;
    }

    for (int i = 0; i < extention.length(); ++i) {
        if (std::isupper(extention[i])) {
            extention[i] = std::tolower(extention[i]);
        }
    }

    if (extention == ".htm") return Extention::htm;
    if (extention == ".html") return Extention::html;
    if (extention == ".css") return Extention::css;
    if (extention == ".txt") return Extention::txt;
    if (extention == ".js") return Extention::js;
    if (extention == ".json") return Extention::json;
    if (extention == ".xml") return Extention::xml;
    if (extention == ".png") return Extention::png;
    if (extention == ".jpg") return Extention::jpg;
    if (extention == ".jpe") return Extention::jpe;
    if (extention == ".jpeg") return Extention::jpeg;
    if (extention == ".gif") return Extention::gif;
    if (extention == ".bmp") return Extention::bmp;
    if (extention == ".ico") return Extention::ico;
    if (extention == ".tiff") return Extention::tiff;
    if (extention == ".tif") return Extention::tif;
    if (extention == ".svg") return Extention::svg;
    if (extention == ".svgz") return Extention::svgz;
    if (extention == ".mp3") return Extention::mp3;

    return Extention::unkown;
}

bool Uri::IsSubPath(const fs::path& base) const {
    for (auto b = base.begin(), p = canonical_uri_.begin(); b != base.end(); ++b, ++p) {
        if (p == canonical_uri_.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string Uri::EncodeUri(std::string_view uri) const {
    std::string encoded_uri;
    
    size_t pos = 1;
    while (pos != std::string_view::npos) {
        size_t encoding_pos = uri.find_first_of('%', pos);

        if (encoding_pos != std::string_view::npos) {
            encoded_uri += detail::EncodeUriSpaces(uri.substr(pos, encoding_pos - pos));

            char coded_symbol = std::stoi(std::string{uri.substr(encoding_pos + 1, 2)}, 0, 16);
            encoded_uri += coded_symbol;

            pos = encoding_pos + 3;
        } else {
            encoded_uri += detail::EncodeUriSpaces(uri.substr(pos));
            pos = encoding_pos;
        }
    }

    return encoded_uri;
}

std::string_view GetMimeType(Extention extention) {
    if (extention == Extention::htm
        || extention == Extention::html) return ContentType::TXT_HTML;
    if (extention == Extention::css) return ContentType::TXT_CSS;
    if (extention == Extention::txt) return ContentType::TXT_PLAIN;
    if (extention == Extention::js) return ContentType::TXT_JS;
    if (extention == Extention::json) return ContentType::APP_JSON;
    if (extention == Extention::xml) return ContentType::APP_XML;
    if (extention == Extention::png) return ContentType::IMG_PNG;
    if (extention == Extention::jpg
        || extention == Extention::jpe
        || extention == Extention::jpeg) return ContentType::IMG_JPEG;
    if (extention == Extention::gif) return ContentType::IMG_GIF;
    if (extention == Extention::bmp) return ContentType::IMG_BMP;
    if (extention == Extention::tiff
        || extention == Extention::tif) return ContentType::IMG_TIFF;
    if (extention == Extention::svg
        || extention == Extention::svgz) return ContentType::IMG_SVG_XML;
    if (extention == Extention::mp3) return ContentType::AUDIO_MPEG;

    return ContentType::APP_BINARY;
}

std::string ParseMapToJson(const model::Map* map) {
    return json::serialize(json::value_from(*map));
}

std::unordered_map<std::string, std::string> ParseQuery(std::string_view query) {
    std::unordered_map<std::string, std::string> query_map;

    size_t prev_pos = 0;
    size_t cur_pos = 0;

    while (cur_pos != std::string_view::npos) {
        cur_pos = query.find('&', prev_pos);
        size_t delim = query.find('=', prev_pos);
        query_map.emplace(std::string(query.substr(prev_pos, delim - prev_pos)),
                          std::string(query.substr(delim + 1, cur_pos - delim - 1)));

        prev_pos = cur_pos + 1;
    }

    return query_map;
}

void ApiRequestHandler::ProcessApiMaps(StringResponse& response,
                                       std::string_view target) const {
    size_t target_legth = 12;
    if (target.size() > target_legth && target[target_legth] != '/') {
        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::bad_request, "Bad request");
        return;
    }

    if (target.size() > target_legth +1) {
        std::string map_name(target.begin() + target_legth + 1,
            *(target.end() - 1) == '/' ? target.end() - 1 : target.end());

        try {
            const model::Map* map = app_.FindMap(model::Map::Id(map_name));
            response.body() = ParseMapToJson(map);
        } catch (const app::GetMapError& error) {
            switch (error.reason) {
                case app::GetMapErrorReason::mapNotFound:
                    MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::map_not_found, error.what());
                    return;
            }
        }
    } else {
        json::array maps_json;
        const model::Game::Maps& maps = app_.ListMaps();
        for (const auto& map : maps) {
            maps_json.push_back({
                {"id", *map.GetId()}, {"name", map.GetName()}
                                });
        }
        response.body() = json::serialize(json::value(std::move(maps_json)));
    }

    response.set(http::field::content_type, ContentType::APP_JSON);
    response.content_length(response.body().size());
    response.result(http::status::ok);
}



void ApiRequestHandler::MakeErrorApiResponse(StringResponse& response, ApiRequestHandler::ErrorCode code,
                                             std::string_view message) const {
    using ec = ApiRequestHandler::ErrorCode;
    response.set(http::field::content_type, ContentType::APP_JSON);

    switch (code) {
        case ec::map_not_found:
        {
            response.result(http::status::not_found);
            json::value jv = {
                {"code", "mapNotFound"},
                {"message", message}
            };
            response.body() = json::serialize(jv);
            break;
        }

        case ec::invalid_method_get_head:
        {
            response.result(http::status::method_not_allowed);
            json::value jv = {
                {"code", "invalidMethod"},
                {"message", message}
            };
            response.body() = json::serialize(jv);
            response.set(http::field::allow, "GET, HEAD");
            break;
        }

        case ec::invalid_method_post:
        {
            response.result(http::status::method_not_allowed);
            json::value jv = {
                {"code", "invalidMethod"},
                {"message", message}
            };
            response.body() = json::serialize(jv);
            response.set(http::field::cache_control, "no-cache");
            response.set(http::field::allow, "POST");
            break;
        }

        case ec::bad_request:
        {
            response.result(http::status::bad_request);
            json::value jv = {
                {"code", "badRequest"},
                {"message", message}
            };
            response.body() = json::serialize(jv);
            break;
        }

        case ec::invalid_token:
        {
            response.result(http::status::unauthorized);
            json::value jv = {
                {"code", "invalidToken"},
                {"message", message}
            };
            response.body() = json::serialize(jv);
            break;
        }

        case ec::invalid_argument:
        {
            response.result(http::status::bad_request);
            json::value jv = {
                {"code", "invalidArgument"},
                {"message", message}
            };
            response.body() = json::serialize(jv);
            break;
        }

        case ec::unknown_token:
        {
            response.result(http::status::unauthorized);
            json::value jv = {
                {"code", "unknownToken"},
                {"message", message}
            };
            response.body() = json::serialize(jv);
            break;
        }
    }
    response.content_length(response.body().size());
}

http::status StaticRequestHandler::ProcessStaticFileTarget(FileResponse& response,
                                                     std::string_view target) const {
    Uri uri(target, static_files_path_);
    if (!uri.IsSubPath(static_files_path_)) {
        return http::status::bad_request;
    }

    http::file_body::value_type file;

    if (http_server::sys::error_code ec; file.open(uri.GetCanonicalUri().string().data(), beast::file_mode::read, ec), ec) {
        return http::status::not_found;
    }

    response.set(http::field::content_type, GetMimeType(uri.GetFileExtention()));
    response.body() = std::move(file);
    response.prepare_payload();
    response.result(http::status::ok);

    return http::status::ok;
}

StringResponse StaticRequestHandler::MakeErrorStaticFileResponse(http::status status) const {
    StringResponse response;
    response.result(status);
    response.set(http::field::content_type, ContentType::TXT_PLAIN);

    switch (status) {
        case http::status::not_found:
            response.body() = "File is not found"sv;
            break;

        case http::status::bad_request:
            response.body() = "Target is out of home directory"sv;
            break;

        case http::status::method_not_allowed:
            response.body() = "Method not allowed"sv;
            response.set(http::field::allow, "GET, HEAD"sv);
            break;

        default:
            response.body() = "Unknown error"sv;
            break;
    }

    response.content_length(response.body().size());
    return response;
}

}  // namespace http_handler
