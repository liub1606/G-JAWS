// Minimal JSON parser for extracting Gemini answer
#include <string>
#include <vector>

std::string extract_gemini_answer(const std::string& json) {
    // Look for "candidates":[{"content":{"parts":[{"text":"...
    size_t cand = json.find("\"candidates\":");
    if (cand == std::string::npos) return "";
    size_t text = json.find("\"text\":\"", cand);
    if (text == std::string::npos) return "";
    size_t start = text + 8;
    size_t end = start;
    bool in_escape = false;
    std::string result;
    while (end < json.size()) {
        char c = json[end];
        if (!in_escape && c == '\\') {
            in_escape = true;
        } else if (!in_escape && c == '"') {
            break;
        } else {
            if (in_escape) {
                if (c == 'n') result += '\n';
                else if (c == 't') result += '\t';
                else if (c == 'r') result += '\r';
                else result += c;
                in_escape = false;
            } else {
                result += c;
            }
        }
        ++end;
    }
    return result;
}
