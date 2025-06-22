// ScreenReader.cpp
// Simple C++ program to capture the screen, save as JPEG, upload to Imgur, send to ChatGPT Vision, and speak response
// Requires: Gdiplus.lib, Ole32.lib, Winhttp.lib, SAPI.lib
// Add your Imgur Client-ID and OpenAI API key below

#include <windows.h>
#include <gdiplus.h>
#include <sapi.h>
#include <winhttp.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "ApiKeys.h"
#pragma comment (lib, "Gdiplus.lib")
#pragma comment (lib, "Ole32.lib")
#pragma comment (lib, "Winhttp.lib")
#pragma comment (lib, "SAPI.lib")
std::string base64_encode(const std::vector<BYTE>& data);
std::wstring IMGUR_CLIENT_ID = L"";
std::wstring OPENAI_API_KEY = L"";

ULONG_PTR gdiplusToken;
void InitGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}
void ShutdownGDIPlus() {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

bool SaveBitmapToJpeg(HBITMAP hBitmap, const wchar_t* filename) {
    Gdiplus::Bitmap bmp(hBitmap, NULL);
    CLSID clsid;
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) {
        MessageBoxW(NULL, L"GetImageEncodersSize returned size 0", L"SaveBitmapToJpeg Error", MB_OK | MB_ICONERROR);
        return false;
    }
    std::vector<BYTE> buffer(size);
    Gdiplus::GetImageEncoders(num, size, (Gdiplus::ImageCodecInfo*)buffer.data());
    bool found = false;
    for (UINT i = 0; i < num; ++i) {
        Gdiplus::ImageCodecInfo* p = (Gdiplus::ImageCodecInfo*)buffer.data() + i;
        if (wcscmp(p->MimeType, L"image/jpeg") == 0) {
            clsid = p->Clsid;
            found = true;
            break;
        }
    }
    if (!found) {
        MessageBoxW(NULL, L"JPEG encoder not found", L"SaveBitmapToJpeg Error", MB_OK | MB_ICONERROR);
        return false;
    }
    Gdiplus::Status status = bmp.Save(filename, &clsid, NULL);
    if (status != Gdiplus::Ok) {
        wchar_t msg[256];
        swprintf(msg, 256, L"bmp.Save failed with status %d", status);
        MessageBoxW(NULL, msg, L"SaveBitmapToJpeg Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

bool CaptureScreen(const wchar_t* filename) {
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    HDC hScreen = GetDC(NULL);
    if (!hScreen) {
        MessageBoxW(NULL, L"GetDC failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        return false;
    }
    HDC hDC = CreateCompatibleDC(hScreen);
    if (!hDC) {
        MessageBoxW(NULL, L"CreateCompatibleDC failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
    if (!hBitmap) {
        MessageBoxW(NULL, L"CreateCompatibleBitmap failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    if (!SelectObject(hDC, hBitmap)) {
        MessageBoxW(NULL, L"SelectObject failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    if (!BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY)) {
        MessageBoxW(NULL, L"BitBlt failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    bool ok = SaveBitmapToJpeg(hBitmap, filename);
    if (!ok) {
        MessageBoxW(NULL, L"SaveBitmapToJpeg failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
    }
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    return ok;
}

std::vector<BYTE> ReadFileBytes(const wchar_t* filename) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<BYTE> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return data;
}

std::wstring UploadToImgur(const wchar_t* filename) {
    std::vector<BYTE> imgData = ReadFileBytes(filename);
    if (imgData.empty()) {
        MessageBoxW(NULL, L"Failed to read image file for Imgur upload.", L"UploadToImgur Error", MB_OK | MB_ICONERROR);
        return L"";
    }
    HINTERNET hSession = WinHttpOpen(L"ScreenReader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) {
        MessageBoxW(NULL, L"WinHttpOpen failed", L"UploadToImgur Error", MB_OK | MB_ICONERROR);
        return L"";
    }
    HINTERNET hConnect = WinHttpConnect(hSession, L"api.imgur.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        MessageBoxW(NULL, L"WinHttpConnect failed", L"UploadToImgur Error", MB_OK | MB_ICONERROR);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/3/image", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        MessageBoxW(NULL, L"WinHttpOpenRequest failed", L"UploadToImgur Error", MB_OK | MB_ICONERROR);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    std::wstring headers = L"Authorization: Client-ID " + IMGUR_CLIENT_ID + L"\r\nContent-Type: application/octet-stream\r\n";
    WinHttpAddRequestHeaders(hRequest, headers.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, imgData.data(), (DWORD)imgData.size(), (DWORD)imgData.size(), 0);
    if (!bResults) {
        MessageBoxW(NULL, L"WinHttpSendRequest failed", L"UploadToImgur Error", MB_OK | MB_ICONERROR);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    WinHttpReceiveResponse(hRequest, NULL);
    DWORD dwSize = 0;
    WinHttpQueryDataAvailable(hRequest, &dwSize);
    if (dwSize == 0) {
        MessageBoxW(NULL, L"No data received from Imgur.", L"UploadToImgur Error", MB_OK | MB_ICONERROR);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    std::vector<char> buffer(dwSize + 1);
    DWORD dwDownloaded = 0;
    WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
    buffer[dwDownloaded] = '\0';
    std::string response(buffer.data());
    if (response.find("link\":\"") == std::string::npos) {
        std::wstring wresponse(response.begin(), response.end());
        MessageBoxW(NULL, wresponse.c_str(), L"Imgur API Response", MB_OK | MB_ICONERROR);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    size_t linkPos = response.find("link\":\"");
    size_t start = response.find("https://", linkPos);
    size_t end = response.find('"', start);
    std::string url = response.substr(start, end - start);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    int len = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, NULL, 0);
    std::wstring wurl(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wurl[0], len);
    return wurl;
}

std::wstring AskChatGPT(const std::wstring& imageUrl, const std::wstring& prompt) {
    std::wstring apiUrl = L"/v1/chat/completions";
    std::wstring host = L"api.openai.com";
    std::string body =
        "{\"model\":\"o4-mini\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"";
    body += std::string(prompt.begin(), prompt.end());
    body += "\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"";
    body += std::string(imageUrl.begin(), imageUrl.end());
    body += "\"}}]}],\"max_tokens\":256}";
    HINTERNET hSession = WinHttpOpen(L"ScreenReader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", apiUrl.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    std::wstring headers = L"Authorization: Bearer " + OPENAI_API_KEY + L"\r\nContent-Type: application/json\r\n";
    WinHttpAddRequestHeaders(hRequest, headers.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (!bResults) {
        MessageBoxW(NULL, L"WinHttpSendRequest failed", L"AskChatGPT Error", MB_OK | MB_ICONERROR);
        return L"";
    }
    WinHttpReceiveResponse(hRequest, NULL);
    DWORD dwSize = 0;
    WinHttpQueryDataAvailable(hRequest, &dwSize);
    std::vector<char> buffer(dwSize + 1);
    DWORD dwDownloaded = 0;
    WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
    buffer[dwDownloaded] = '\0';
    std::string response(buffer.data());
    size_t pos = response.find("content\":\"");
    if (pos == std::string::npos) {
        std::wstring wresponse(response.begin(), response.end());
        MessageBoxW(NULL, wresponse.c_str(), L"OpenAI API Response", MB_OK | MB_ICONERROR);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    size_t start = pos + 10;
    size_t end = response.find('"', start);
    std::string answer = response.substr(start, end - start);
    int len = MultiByteToWideChar(CP_UTF8, 0, answer.c_str(), -1, NULL, 0);
    std::wstring wanswer(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, answer.c_str(), -1, &wanswer[0], len);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return wanswer;
}

void Speak(const std::wstring& text) {
    ISpVoice* pVoice = NULL;
    if (FAILED(::CoInitialize(NULL))) return;
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if (SUCCEEDED(hr)) {
        pVoice->Speak(text.c_str(), SPF_IS_XML, NULL);
        pVoice->Release();
    }
    ::CoUninitialize();
}

void ShowStatusWindow(const std::wstring& status) {
    MessageBoxW(NULL, status.c_str(), L"ScreenReader Status", MB_OK | MB_ICONINFORMATION);
}

int wmain(int argc, wchar_t* argv[]) {
    InitGDIPlus();
    ApiKeys keys = FetchApiKeys();
    if (keys.imgur_client_id.empty() || keys.openai_api_key.empty()) {
        ShowStatusWindow(L"Failed to fetch API keys from backend.");
        return 1;
    }
    const wchar_t* filename = L"screen.jpg";
    std::wstring prompt = L"Describe what is happening in this screenshot in the context of the game, give playable actions.";
    if (argc > 1) prompt = argv[1];
    ShowStatusWindow(L"Capturing screen...");
    if (!CaptureScreen(filename)) {
        ShowStatusWindow(L"Screen capture failed.");
        return 1;
    }
    ShowStatusWindow(L"Uploading to Imgur...");
    extern std::wstring IMGUR_CLIENT_ID;
    IMGUR_CLIENT_ID = keys.imgur_client_id;
    std::wstring imgUrl = UploadToImgur(filename);
    if (imgUrl.empty()) {
        ShowStatusWindow(L"Imgur upload failed. Using base64 data URL.");
        std::vector<BYTE> imgData = ReadFileBytes(filename);
        std::string b64 = base64_encode(imgData);
        std::wstring dataUrl = L"data:image/jpeg;base64,";
        dataUrl += std::wstring(b64.begin(), b64.end());
        imgUrl = dataUrl;
    }
    ShowStatusWindow(L"Image uploaded. Asking ChatGPT Vision...");
    extern std::wstring OPENAI_API_KEY;
    OPENAI_API_KEY = keys.openai_api_key;
    std::wstring answer = AskChatGPT(imgUrl, prompt);
    if (answer.empty()) {
        ShowStatusWindow(L"ChatGPT Vision failed.");
        return 1;
    }
    ShowStatusWindow(L"Response received. Speaking...");
    Speak(answer);
    ShutdownGDIPlus();
    ShowStatusWindow(L"Done!");
    return 0;
}
// Base64 encoding table
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64_encode(const std::vector<BYTE>& data) {
    std::string out;
    int val = 0, valb = -6;
    for (BYTE c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(b64_table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}
