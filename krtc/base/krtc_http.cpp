#include "krtc/base/krtc_http.h"

#include <iostream>

using HttpClient = Http::HttpClient;

namespace krtc {

    Http::HTTPERROR httpExecRequest(const std::string& method,
        const std::string& url,
        std::string& responsebody,
        const std::string& requestBody,
        const Http::ContentType& type) 
    {
        Http::HTTPERROR errcode = Http::HTTPERROR_SUCCESS;
        Http::HttpResponse response;
        HttpClient client;

        Http::HttpRequest request(method, url);
        std::string contentType = "application/json";
        if (type == Http::ContentType::Xml) {
            request.addHeaderField("Content-Type", "application/xml");
        }
        request.setBody(requestBody);

        if (!client.execute(&request, &response)) {
            errcode = client.getErrorCode();
            client.killSelf();
            return errcode;
        }

        responsebody = response.getBody();

        client.killSelf();

        return Http::HTTPERROR_SUCCESS;
    }

    std::string httpErrorString(Http::HTTPERROR errcode) {
        switch (errcode) {
        case Http::HTTPERROR_SUCCESS:
            return "success";
            break;
        case Http::HTTPERROR_INVALID:
            return "http invalid";
            break;
        case Http::HTTPERROR_CONNECT:
            return "connect error";
            break;
        case Http::HTTPERROR_TRANSPORT:
            return "transport error";
            break;
        case Http::HTTPERROR_IO:
            return "IO error";
            break;
        case Http::HTTPERROR_PARAMETER:
            break;
            return "param error";
        default:
            return "";
            break;
        }
        return "";
    }

}