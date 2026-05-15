#pragma once

// ============================================================
//  版本号 —— 只在此处修改，其他文件全部引用这里
// ============================================================
#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   1
#define APP_VERSION_PATCH   0
#define APP_VERSION_BUILD   0


// 辅助宏：把数字拼成字符串
#define _VER_STR2(x)        #x
#define _VER_STR(x)         _VER_STR2(x)

// 普通字符串："1.0.0.0"
#define APP_VERSION_A       _VER_STR(APP_VERSION_MAJOR) "." \
                            _VER_STR(APP_VERSION_MINOR) "." \
                            _VER_STR(APP_VERSION_PATCH) "." \
                            _VER_STR(APP_VERSION_BUILD)

// 宽字符串：L"1.0.0.0"
#define APP_VERSION         L"" _VER_STR(APP_VERSION_MAJOR) "." \
                            _VER_STR(APP_VERSION_MINOR) "." \
                            _VER_STR(APP_VERSION_PATCH) "." \
                            _VER_STR(APP_VERSION_BUILD)

// ============================================================
//  产品信息 —— 与 .rc 文件中的 StringFileInfo 保持同步
// ============================================================
#define APP_COMPANY_NAME    "Qizhiwoniu"
#define APP_FILE_DESC       "MiniCAD"
#define APP_INTERNAL_NAME   "MiniCAD"
#define APP_ORIGINAL_FILE   "MiniCAD.exe"
#define APP_PRODUCT_NAME    "MiniCAD"
