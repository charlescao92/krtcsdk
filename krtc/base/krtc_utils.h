#ifndef KRTCSDK_KRTC_BASE_KRTC_UTILS_H_
#define KRTCSDK_KRTC_BASE_KRTC_UTILS_H_

#include <string>
#include <map>

namespace krtc {

    // krtc://www.charlescao92.cn/push?uid=xxx&streamName=xxx
    bool ParseUrl(const std::string& url,
        std::string& protocol,
        std::string& host,
        std::string& action,
        std::map<std::string, std::string>& request_params);

} // namespace krtc

#endif // KRTCSDK_KRTC_BASE_KRTC_UTILS_H_
