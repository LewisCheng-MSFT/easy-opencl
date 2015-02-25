#pragma once

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include <CL/cl.hpp>

// 是否显示跟踪信息
#define TRACE

// 是否显示“跟踪：……”行。
//#define DISP_TRACE_HEADER

// 计时开启
#define SET_TIMER