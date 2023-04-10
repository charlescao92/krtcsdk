#include "krtc/base/krtc_utils.h"

#include <rtc_base/string_encode.h>
#include <rtc_base/logging.h>

namespace krtc {

    bool ParseUrl(const std::string& url,
        std::string& protocol,
        std::string& host,
        std::string& action,
        std::map<std::string, std::string>& request_params)
    {
        // krtc://www.charlescao92.cn/push?uid=xxx&streamName=xxx
        std::string rest;
        if (!rtc::tokenize_first(url, ':', &protocol, &rest)) {
            RTC_LOG(LS_WARNING) << "parse protocol failed, url: " << url;
            return false;
        }

        rest = rest.substr(2);
        if (!rtc::tokenize_first(rest, '/', &host, &rest)) {
            RTC_LOG(LS_WARNING) << "parse host failed, url: " << url;
            return false;
        }

        if (!rtc::tokenize_first(rest, '?', &action, &rest)) {
            RTC_LOG(LS_WARNING) << "parse action failed, url: " << url;
            return false;
        }

        std::vector<std::string> fields;
        rtc::tokenize(rest, '&', &fields);
        for (auto field : fields) {
            // k=v
            std::vector<std::string> items;
            rtc::tokenize(field, '=', &items);
            if (items.size() != 2) {
                continue;
            }

            request_params[items[0]] = items[1];
        }

        return true;
    }

} // namespace krtc