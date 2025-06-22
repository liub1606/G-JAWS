# ScreenReader: Gemini Vision-Powered Screen Narrator

## Overview
ScreenReader is a C++ application for Windows that captures the screen, encodes it as a JPEG, and sends it to Google Gemini Vision for context-aware narration. The response is spoken aloud using Windows Text-to-Speech (TTS). The program is designed for accessibility and game narration, but can be used for any visual context.

## Features
- **Screen Capture:** Captures the entire desktop as a JPEG image.
- **Gemini Vision Integration:** Sends the screenshot to Gemini Vision (Google's multimodal LLM) for analysis.
- **Context-Aware Narration:** Receives a concise, context-aware description and playable actions from Gemini, then speaks the answer aloud.
- **Secure API Key Handling:** Fetches Gemini API key from a local Python Flask backend (`backend/server.py`).
- **Keyboard Shortcut:** Press the tilde (`~`) key to trigger screen reading and narration.
- **Robust Error Handling:** The app remains running even if errors occur, and will speak out any unexpected responses.

## Project Structure
- `ScreenReader.cpp` — Main C++ application.
- `ApiKeys.h` — Fetches Gemini API key from backend.
- `extract_gemini_answer.h` — Robustly extracts the answer from Gemini's JSON response.
- `backend/server.py` — Flask backend serving API keys.
- `gemini_debug.json` — (Ignored by git) For debugging Gemini's raw output.
- `.gitignore` — Excludes sensitive and build files from version control.

## Usage
1. **Start the Flask backend:**
   ```
   cd backend
   python server.py
   ```
2. **Build the C++ app:**
   ```
   cl ScreenReader.cpp /EHsc /link Gdiplus.lib Ole32.lib Winhttp.lib SAPI.lib Gdi32.lib User32.lib Shell32.lib
   ```
3. **Run `ScreenReader.exe`.**
4. **Press the tilde (`~`) key** to capture the screen and hear the narration.

## Security & Privacy
- API keys are never hardcoded; they are fetched from the backend.
- Sensitive files like `ApiKeys.h` and `gemini_debug.json` are gitignored.

## Requirements
- Windows (with GDI+, WinHTTP, SAPI, etc.)
- Visual Studio or MSVC build tools
- Python 3 (for backend)

## Customization
- Change the prompt in `ScreenReader.cpp` for different narration styles.
- Adjust `extract_gemini_answer.h` for more advanced response parsing.

## License
MIT License (or specify your own)

---
For questions or contributions, open an issue or pull request.
