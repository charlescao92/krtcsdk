#ifndef LOG_STREAM_H_
#define LOG_STREAM_H_

#include <string>
#include <memory>

#include <rtc_base/log_sinks.h>

namespace utils {

class LogStreamFile {
 public:
  LogStreamFile(const std::string& logPreffix = "");
  virtual ~LogStreamFile();

  void Init();

 private:
  std::unique_ptr<rtc::FileRotatingLogSink> fileLogSink_ = NULL;
};

}

#endif
