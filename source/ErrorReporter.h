#pragma once
#include <string>
#include <functional>

namespace YGsoftware
{
    // 启动时设置一次
    void SetErrorHandler(std::function<void(const std::string&)> handler);

    // 全局调用
    void ReportError(const std::string& msg);
}
