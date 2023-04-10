/**
 * @file    base64.h
 * @brief   BASE64编解码方法
 * @author  xiangwangfeng <xiangwangfeng@gmail.com>
 * @data	2011-4-23
 * @website www.xiangwangfeng.com
 */

#pragma once
#include <string>
#include "global_defs.h"

NAMESPACE_BEGIN(Util)

void base64Encode(const std::string& input,std::string& output);

void base64Decode(const std::string& input,std::string& output);

NAMESPACE_END(Util)
