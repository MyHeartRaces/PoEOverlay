/**
 * @file input_handler.cpp
 * @brief Implementation of the InputHandler class
 */

 #include "window/input_handler.h"
 #include <sstream>
 #include "core/Logger.h"
 #include "core/ErrorHandler.h"
 
 namespace poe {
 
 InputHandler::InputHandler(Application& app, HWND hwnd)
     : m_app(app), m_hwnd(hwnd), m_nextHotkeyId(1) {
     m_app.GetLogger().Info("InputHandler initialized");
 }
 
 InputHandler::~InputHandler() {
     // Unregister all global hotkeys
     for (const auto& pair : m_hotkeys) {
         if (pair.second.hotkey.global) {
             UnregisterHotKey(m_hwnd, pair.first);
         }
     }
 }
 
 InputHandler::InputHandler(InputHandler&& other) noexcept
     : m_hwnd(other.m_hwnd),
       m_nextHotkeyId(other.m_nextHotkeyId),
       m_hotkeys(std::move(other.m_hotkeys)) {
     
     other.m_hwnd = nullptr;
     other.m_nextHotkeyId = 1;
 }
 
 InputHandler& InputHandler::operator=(InputHandler&& other) noexcept {
     if (this != &other) {
         // Clean up existing hotkeys
         for (const auto& pair : m_hotkeys) {
             if (pair.second.hotkey.global) {
                 UnregisterHotKey(m_hwnd, pair.first);
             }
         }
         
         m_hwnd = other.m_hwnd;
         m_nextHotkeyId = other.m_nextHotkeyId;
         m_hotkeys = std::move(other.m_hotkeys);
         
         other.m_hwnd = nullptr;
         other.m_nextHotkeyId = 1;
     }
     return *this;
 }
 
 int InputHandler::RegisterHotkey(
     UINT modifiers,
     UINT virtualKey,
     const std::string& description,
     bool global,
     std::function<void()> callback) {
     
     try {
         // Generate a new ID for the hotkey
         int id = GenerateHotkeyId();
         
         // Create the hotkey data
         HotkeyData data;
         data.hotkey.id = id;
         data.hotkey.modifiers = modifiers;
         data.hotkey.virtualKey = virtualKey;
         data.hotkey.description = description;
         data.hotkey.global = global;
         data.callback = std::move(callback);
         
         // Register the hotkey with the system if it's global
         if (global) {
             if (!RegisterHotKey(m_hwnd, id, modifiers, virtualKey)) {
                 DWORD error = GetLastError();
                 m_app.GetLogger().Error("Failed to register global hotkey: {} (Error code: {})", 
                     HotkeyToString(modifiers, virtualKey), error);
                 
                 // Check for common errors
                 if (error == ERROR_HOTKEY_ALREADY_REGISTERED) {
                     m_app.GetLogger().Warning("Hotkey already registered by another application");
                 }
                 
                 return -1;
             }
         }
         
         // Add the hotkey to our map
         m_hotkeys[id] = std::move(data);
         
         m_app.GetLogger().Info("Registered hotkey: {} (ID: {}, Global: {})", 
             HotkeyToString(modifiers, virtualKey), id, global ? "Yes" : "No");
         
         return id;
     }
     catch (const std::exception& ex) {
         m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "InputHandler");
         return -1;
     }
 }
 
 bool InputHandler::UnregisterHotkey(int id) {
     auto it = m_hotkeys.find(id);
     if (it == m_hotkeys.end()) {
         return false;
     }
     
     // Unregister the hotkey with the system if it's global
     if (it->second.hotkey.global) {
         if (!UnregisterHotKey(m_hwnd, id)) {
             return false;
         }
     }
     
     // Remove the hotkey from our map
     m_hotkeys.erase(it);
     
     return true;
 }
 
 bool InputHandler::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
     try {
         switch (uMsg) {
             case WM_HOTKEY: {
                 // Handle global hotkey
                 int id = static_cast<int>(wParam);
                 auto it = m_hotkeys.find(id);
                 if (it != m_hotkeys.end()) {
                     m_app.GetLogger().Debug("Global hotkey triggered: {} (ID: {})", 
                         HotkeyToString(it->second.hotkey.modifiers, it->second.hotkey.virtualKey), id);
                     
                     // Call the associated callback
                     if (it->second.callback) {
                         it->second.callback();
                     }
                     return true;
                 }
                 break;
             }
             
         case WM_KEYDOWN:
         case WM_SYSKEYDOWN: {
             // Handle non-global hotkeys
             UINT virtualKey = static_cast<UINT>(wParam);
             
             // Check for modifier keys
             UINT modifiers = 0;
             if (IsKeyPressed(VK_CONTROL)) modifiers |= MOD_CONTROL;
             if (IsKeyPressed(VK_SHIFT)) modifiers |= MOD_SHIFT;
             if (IsKeyPressed(VK_MENU)) modifiers |= MOD_ALT;
             if (IsKeyPressed(VK_LWIN) || IsKeyPressed(VK_RWIN)) modifiers |= MOD_WIN;
             
             // Check if any of our non-global hotkeys match
             for (const auto& pair : m_hotkeys) {
                 const HotkeyData& data = pair.second;
                 if (!data.hotkey.global &&
                     data.hotkey.virtualKey == virtualKey &&
                     data.hotkey.modifiers == modifiers) {
                     
                     // Call the associated callback
                     if (data.callback) {
                         data.callback();
                     }
                     return true;
                 }
             }
             break;
         }
     }
     
     return false;
 }
 
 std::vector<InputHandler::Hotkey> InputHandler::GetHotkeys() const {
     std::vector<Hotkey> hotkeys;
     hotkeys.reserve(m_hotkeys.size());
     
     for (const auto& pair : m_hotkeys) {
         hotkeys.push_back(pair.second.hotkey);
     }
     
     return hotkeys;
 }
 
 bool InputHandler::IsKeyPressed(UINT virtualKey) {
     return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
 }
 
 std::string InputHandler::HotkeyToString(UINT modifiers, UINT virtualKey) {
     std::stringstream ss;
     
     // Add modifiers
     if (modifiers & MOD_CONTROL) ss << "Ctrl+";
     if (modifiers & MOD_SHIFT) ss << "Shift+";
     if (modifiers & MOD_ALT) ss << "Alt+";
     if (modifiers & MOD_WIN) ss << "Win+";
     
     // Add the virtual key name
     char keyName[256] = {0};
     UINT scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
     
     // Handle special keys
     switch (virtualKey) {
         case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
         case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
         case VK_INSERT: case VK_DELETE:
         case VK_DIVIDE:
         case VK_NUMLOCK:
             scanCode |= 0x100;  // Set extended bit
             break;
     }
     
     GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName));
     ss << keyName;
     
     return ss.str();
 }
 
 int InputHandler::GenerateHotkeyId() const {
     // Find the next available ID
     int id = m_nextHotkeyId;
     while (m_hotkeys.find(id) != m_hotkeys.end()) {
         id++;
     }
     return id;
 }
 
 } // namespace poe