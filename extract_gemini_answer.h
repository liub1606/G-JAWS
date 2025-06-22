// Improved Gemini answer extraction for plain JSON structure
#include <regex>
#include <string>
#include <algorithm>

std::string extract_gemini_answer(const std::string& json) {
    // Look for: "text":"..."
    std::regex text_re("\"text\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
    std::smatch match;
    if (std::regex_search(json, match, text_re)) {
        std::string result = match[1].str();
        // Unescape common JSON escape sequences
        std::string unescaped;
        for (size_t i = 0; i < result.size(); ++i) {
            if (result[i] == '\\' && i + 1 < result.size()) {
                char next = result[i + 1];
                if (next == 'n') { unescaped += '\n'; ++i; }
                else if (next == 't') { unescaped += '\t'; ++i; }
                else if (next == 'r') { unescaped += '\r'; ++i; }
                else if (next == '"') { unescaped += '"'; ++i; }
                else if (next == '\\') { unescaped += '\\'; ++i; }
                else { unescaped += result[i]; }
            } else {
                unescaped += result[i];
            }
        }
        return unescaped;
    }
    // Fallback: return empty string
    return "";
}
