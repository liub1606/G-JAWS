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

// Replace with your keys
std::wstring IMGUR_CLIENT_ID = L"";
std::wstring OPENAI_API_KEY = L"";

// Helper: Initialize GDI+
ULONG_PTR gdiplusToken;
void InitGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}
void ShutdownGDIPlus() {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

// Helper: Save HBITMAP as JPEG
bool SaveBitmapToJpeg(HBITMAP hBitmap, const wchar_t* filename) {
    Gdiplus::Bitmap bmp(hBitmap, NULL);
    CLSID clsid;
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    std::vector<BYTE> buffer(size);
    Gdiplus::GetImageEncoders(num, size, (Gdiplus::ImageCodecInfo*)buffer.data());
    for (UINT i = 0; i < num; ++i) {
        Gdiplus::ImageCodecInfo* p = (Gdiplus::ImageCodecInfo*)&buffer[0] + i;
        if (wcscmp(p->MimeType, L"image/jpegf") == 0) {
            clsid = p->Clsid;
            break;
        }
    }
    return bmp.Save(filename, &clsid, NULL) == Gdiplus::Ok;
}

// Capture screen and save as JPEG
bool CaptureScreen(const wchar_t* filename) {
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);
    bool ok = SaveBitmapToJpeg(hBitmap, filename);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    return ok;
}

// Read file to memory
std::vector<BYTE> ReadFileBytes(const wchar_t* filename) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<BYTE> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return data;
}

// Upload image to Imgur
std::wstring UploadToImgur(const wchar_t* filename) {
    std::vector<BYTE> imgData = ReadFileBytes(filename);
    HINTERNET hSession = WinHttpOpen(L"ScreenReader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, L"api.imgur.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/3/image", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    std::wstring headers = L"Authorization: Client-ID " + IMGUR_CLIENT_ID + L"\r\nContent-Type: application/octet-stream\r\n";
    WinHttpAddRequestHeaders(hRequest, headers.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, imgData.data(), (DWORD)imgData.size(), (DWORD)imgData.size(), 0);
    if (!bResults) return L"";
    WinHttpReceiveResponse(hRequest, NULL);
    DWORD dwSize = 0;
    WinHttpQueryDataAvailable(hRequest, &dwSize);
    std::vector<char> buffer(dwSize + 1);
    DWORD dwDownloaded = 0;
    WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
    buffer[dwDownloaded] = '\0';
    std::string response(buffer.data());
    size_t linkPos = response.find("link\":\"");
    if (linkPos == std::string::npos) return L"";
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

// Send image URL + prompt to ChatGPT Vision (GPT-4o)
std::wstring AskChatGPT(const std::wstring& imageUrl, const std::wstring& prompt) {
    std::wstring apiUrl = L"/v1/chat/completions";
    std::wstring host = L"api.openai.com";
    std::string body =
        "{\"model\":\"gpt-4o\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"";
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
    if (!bResults) return L"";
    WinHttpReceiveResponse(hRequest, NULL);
    DWORD dwSize = 0;
    WinHttpQueryDataAvailable(hRequest, &dwSize);
    std::vector<char> buffer(dwSize + 1);
    DWORD dwDownloaded = 0;
    WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
    buffer[dwDownloaded] = '\0';
    std::string response(buffer.data());
    size_t pos = response.find("content\":\"");
    if (pos == std::string::npos) return L"";
    size_t start = pos + 10;
    size_t end = response.find('"', start);
    std::string answer = response.substr(start, end - start);
    int len = MultiByteToWideChar(CP_UTF8, 0, answer.c_str(), -1, NULL, 0);
    std::wstring wanswer(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, answer.c_str(), -1, &wanswer[0], len);
    return wanswer;
}

// Speak text using SAPI
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

int wmain(int argc, wchar_t* argv[]) {
    InitGDIPlus();
    ApiKeys keys = FetchApiKeys();
    if (keys.imgur_client_id.empty() || keys.openai_api_key.empty()) {
        std::wcout << L"Failed to fetch API keys from backend.\n";
        return 1;
    }
    const wchar_t* filename = L"screen.jpg";
    std::wstring prompt = L"Describe what is happening in this screenshot.";
    if (argc > 1) prompt = argv[1];
    std::wcout << L"Capturing screen...\n";
    if (!CaptureScreen(filename)) {
        std::wcout << L"Screen capture failed.\n";
        return 1;
    }
    std::wcout << L"Uploading to Imgur...\n";
    // Use keys.imgur_client_id instead of global const
    IMGUR_CLIENT_ID = keys.imgur_client_id;
    std::wstring imgUrl = UploadToImgur(filename);
    if (imgUrl.empty()) {
        std::wcout << L"Imgur upload failed.\n";
        return 1;
    }
    std::wcout << L"Image URL: " << imgUrl << std::endl;
    std::wcout << L"Asking ChatGPT Vision...\n";
    // Use keys.openai_api_key instead of global const
    OPENAI_API_KEY = keys.openai_api_key;
    std::wstring answer = AskChatGPT(imgUrl, prompt);
    if (answer.empty()) {
        std::wcout << L"ChatGPT Vision failed.\n";
        return 1;
    }
    std::wcout << L"Response: " << answer << std::endl;
    std::wcout << L"Speaking...\n";
    Speak(answer);
    ShutdownGDIPlus();
    return 0;
}
