/**
 * @file    global_defs.h
 * @brief   类和名词空间相关的宏定义
 * @author  xiangwangfeng <xiangwangfeng@gmail.com>
 * @data	2011-4-23
 * @website www.xiangwangfeng.com
 */
#pragma once

#include "GlobalDefine.h"

#define DECLARE_NON_COPYABLE(className) \
	className (const className&);\
	className& operator= (const className&);
