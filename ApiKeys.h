#pragma once
#include <string>
#include <winhttp.h>
#include <vector>
#include <iostream>
#include <regex>

struct ApiKeys {
    std::wstring gemini_api_key;
};

inline std::wstring ReadJsonString(const std::string& json, const std::string& key) {
    std::regex rgx("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(json, match, rgx) && match.size() > 1) {
        std::string value = match[1].str();
        int len = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, NULL, 0);
        std::wstring wvalue(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, &wvalue[0], len);
        return wvalue;
    }
    return L"";
}

inline ApiKeys FetchApiKeys() {
    ApiKeys keys;
    HINTERNET hSession = WinHttpOpen(L"ScreenReader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 5000, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/keys", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!bResults) return keys;
    WinHttpReceiveResponse(hRequest, NULL);
    DWORD dwSize = 0;
    WinHttpQueryDataAvailable(hRequest, &dwSize);
    std::vector<char> buffer(dwSize + 1);
    DWORD dwDownloaded = 0;
    WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
    buffer[dwDownloaded] = '\0';
    std::string response(buffer.data());
    keys.gemini_api_key = ReadJsonString(response, "gemini_api_key");
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return keys;
}
