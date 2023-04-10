#pragma once

#include <string>
#include <atomic>

#include "krtc/net/http.h"

namespace krtc {

    Http::HTTPERROR httpExecRequest(
        const std::string& method,
        const std::string& url,
        std::string& responsebody,
        const std::string& requestBody = "",
        const Http::ContentType& type = Http::ContentType::Json);

    std::string httpErrorString(Http::HTTPERROR errcode);

}
