#include <sstream>
#include <windows.h>
#include <gdiplus.h>
#include <sapi.h>
#include <winhttp.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "ApiKeys.h"
#include "extract_gemini_answer.h"
#include <thread>
#include <atomic>
#include <mmsystem.h>
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Winhttp.lib")
#pragma comment(lib, "SAPI.lib")
#pragma comment(lib, "winmm.lib")
std::string wstring_to_utf8(const std::wstring &wstr);
std::string base64_encode(const std::vector<BYTE> &data);
std::wstring GEMINI_API_KEY = L"";

ULONG_PTR gdiplusToken;
void InitGDIPlus()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}
void ShutdownGDIPlus()
{
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

bool SaveBitmapToJpeg(HBITMAP hBitmap, const wchar_t *filename)
{
    Gdiplus::Bitmap bmp(hBitmap, NULL);
    CLSID clsid;
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
    {
        MessageBoxW(NULL, L"GetImageEncodersSize returned size 0", L"SaveBitmapToJpeg Error", MB_OK | MB_ICONERROR);
        return false;
    }
    std::vector<BYTE> buffer(size);
    Gdiplus::GetImageEncoders(num, size, (Gdiplus::ImageCodecInfo *)buffer.data());
    bool found = false;
    for (UINT i = 0; i < num; ++i)
    {
        Gdiplus::ImageCodecInfo *p = (Gdiplus::ImageCodecInfo *)buffer.data() + i;
        if (wcscmp(p->MimeType, L"image/jpeg") == 0)
        {
            clsid = p->Clsid;
            found = true;
            break;
        }
    }
    if (!found)
    {
        MessageBoxW(NULL, L"JPEG encoder not found", L"SaveBitmapToJpeg Error", MB_OK | MB_ICONERROR);
        return false;
    }
    Gdiplus::Status status = bmp.Save(filename, &clsid, NULL);
    if (status != Gdiplus::Ok)
    {
        wchar_t msg[256];
        swprintf(msg, 256, L"bmp.Save failed with status %d", status);
        MessageBoxW(NULL, msg, L"SaveBitmapToJpeg Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

bool CaptureScreen(const wchar_t *filename)
{
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    HDC hScreen = GetDC(NULL);
    if (!hScreen)
    {
        MessageBoxW(NULL, L"GetDC failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        return false;
    }
    HDC hDC = CreateCompatibleDC(hScreen);
    if (!hDC)
    {
        MessageBoxW(NULL, L"CreateCompatibleDC failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
    if (!hBitmap)
    {
        MessageBoxW(NULL, L"CreateCompatibleBitmap failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    if (!SelectObject(hDC, hBitmap))
    {
        MessageBoxW(NULL, L"SelectObject failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    if (!BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY))
    {
        MessageBoxW(NULL, L"BitBlt failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }
    bool ok = SaveBitmapToJpeg(hBitmap, filename);
    if (!ok)
    {
        MessageBoxW(NULL, L"SaveBitmapToJpeg failed", L"CaptureScreen Error", MB_OK | MB_ICONERROR);
    }
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    return ok;
}

std::vector<BYTE> ReadFileBytes(const wchar_t *filename)
{
    std::ifstream file(filename, std::ios::binary);
    std::vector<BYTE> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return data;
}

std::wstring AskGemini(const std::wstring &imageUrl, const std::wstring &prompt, std::string* raw_json_out = nullptr)
{
    std::string prompt_utf8 = wstring_to_utf8(prompt);
    std::string imageUrl_utf8 = wstring_to_utf8(imageUrl);
    std::string api_key_utf8 = wstring_to_utf8(GEMINI_API_KEY);
    std::string endpoint = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + api_key_utf8;

    std::string body =
        "{\"contents\":[{\"parts\":[{\"text\":\"" + prompt_utf8 + "\"},{\"inline_data\":{\"mime_type\":\"image/jpeg\",\"data\":\"" + imageUrl_utf8 + "\"}}]}]}";

    HINTERNET hSession = WinHttpOpen(L"ScreenReader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    std::wstring endpointW(endpoint.begin(), endpoint.end());
    wchar_t hostName[256] = {0};
    wchar_t urlPath[512] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 512;
    WinHttpCrackUrl(endpointW.c_str(), (DWORD)endpointW.length(), 0, &urlComp);
    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    std::wstring headers = L"Content-Type: application/json\r\n";
    WinHttpAddRequestHeaders(hRequest, headers.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (!bResults)
    {
        MessageBoxW(NULL, L"WinHttpSendRequest failed", L"AskGemini Error", MB_OK | MB_ICONERROR);
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
    if (raw_json_out) *raw_json_out = response;
    std::string answer = extract_gemini_answer(response);
    std::wstring wanswer;
    if (!answer.empty()) {
        int len = MultiByteToWideChar(CP_UTF8, 0, answer.c_str(), -1, NULL, 0);
        wanswer.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, answer.c_str(), -1, &wanswer[0], len);
    } else {
        // If extraction fails, just speak the raw response
        int len = MultiByteToWideChar(CP_UTF8, 0, response.c_str(), -1, NULL, 0);
        wanswer.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, response.c_str(), -1, &wanswer[0], len);
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return wanswer;
}

void Speak(const std::wstring &text)
{
    ISpVoice *pVoice = NULL;
    if (FAILED(::CoInitialize(NULL)))
        return;
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if (SUCCEEDED(hr))
    {
        pVoice->Speak(text.c_str(), SPF_IS_XML, NULL);
        pVoice->Release();
    }
    ::CoUninitialize();
}

HHOOK g_hHook = NULL;
std::atomic<bool> g_triggered = false;
std::atomic<bool> g_debug = false;
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void InstallHook();
void UninstallHook();

void InstallHook() {
    g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
}
void UninstallHook() {
    if (g_hHook) UnhookWindowsHookEx(g_hHook);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        // VK_OEM_3 is the tilde (`~`) key on US keyboards
        if (wParam == WM_KEYDOWN && p->vkCode == VK_OEM_3) {
            PlaySoundW(L"SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC);
            g_triggered = true;
        }
        // VK_OEM_2 is the forward slash (/) key on US keyboards
        if (wParam == WM_KEYDOWN && p->vkCode == VK_OEM_2) {
            g_debug = !g_debug;
            PlaySoundW(L"SystemExclamation", NULL, SND_ALIAS | SND_ASYNC);
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

int wmain(int argc, wchar_t *argv[])
{
    InitGDIPlus();  
    ApiKeys keys = FetchApiKeys();
    GEMINI_API_KEY = keys.gemini_api_key;
    if (keys.gemini_api_key.empty())
        return 1;
    InstallHook();
    Speak(L"Screen Reader is running. Press the tilda button to read the screen.");
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (g_triggered) {
            g_triggered = false;
            const wchar_t *filename = L"screen.jpg";
            if (!CaptureScreen(filename))
                continue;
            std::vector<BYTE> imgData = ReadFileBytes(filename);
            std::string b64 = base64_encode(imgData);
            std::wstring b64w(b64.begin(), b64.end());
            std::wstring prompt = L"Describe what is happening in this screenshot in the context of the game, give playable actions in a concise manner. Return in plain text no rich text";
            std::string raw_json;
            std::wstring answer = AskGemini(b64w, prompt, &raw_json);
            if (!answer.empty())
                Speak(answer);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    UninstallHook();
    ShutdownGDIPlus();
    return 0;
}
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64_encode(const std::vector<BYTE> &data)
{
    std::string out;
    int val = 0, valb = -6;
    for (BYTE c : data)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(b64_table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4)
        out.push_back('=');
    return out;
}

std::string wstring_to_utf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}