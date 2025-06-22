#pragma once
#include <string>
#include <winhttp.h>
#include <vector>
#include <iostream>

struct ApiKeys {
    std::wstring imgur_client_id;
    std::wstring openai_api_key;
};

inline std::wstring ReadJsonString(const std::string& json, const std::string& key) {
    size_t pos = json.find(key);
    if (pos == std::string::npos) return L"";
    size_t start = json.find('"', pos + key.length());
    if (start == std::string::npos) return L"";
    start++;
    size_t end = json.find('"', start);
    if (end == std::string::npos) return L"";
    std::string value = json.substr(start, end - start);
    int len = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, NULL, 0);
    std::wstring wvalue(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, &wvalue[0], len);
    return wvalue;
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
    keys.imgur_client_id = ReadJsonString(response, "imgur_client_id");
    keys.openai_api_key = ReadJsonString(response, "openai_api_key");
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return keys;
}
