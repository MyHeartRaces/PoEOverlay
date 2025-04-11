/**
 * @file input_handler.h
 * @brief Defines the InputHandler class for managing hotkeys and input
 * 
 * This header provides the InputHandler class that handles hotkeys, 
 * keyboard shortcuts, and general input management for the overlay.
 */

 #pragma once

 #include <Windows.h>
 #include <vector>
 #include <functional>
 #include <string>
 #include <unordered_map>
 #include <memory>
 #include "core/Application.h"
 
 namespace poe {
 
 // Forward declarations
 class Application;
 
 /**
  * @class InputHandler
  * @brief Manages hotkeys and input for the overlay
  */
 class InputHandler {
 public:
     /**
      * @struct Hotkey
      * @brief Represents a keyboard hotkey
      */
     struct Hotkey {
         int id;                 ///< Unique identifier for the hotkey
         UINT modifiers;         ///< Modifier keys (MOD_ALT, MOD_CONTROL, MOD_SHIFT, MOD_WIN)
         UINT virtualKey;        ///< Virtual key code
         std::string description; ///< Human-readable description
         bool global;            ///< Whether the hotkey is global (works outside the app)
     };
 
     /**
      * @brief Construct a new Input Handler object
      * 
      * @param app Reference to the main application
      * @param hwnd Window handle to associate with the input handler
      */
     InputHandler(Application& app, HWND hwnd);
 
     /**
      * @brief Destroy the Input Handler object
      */
     ~InputHandler();
 
     // Non-copyable
     InputHandler(const InputHandler&) = delete;
     InputHandler& operator=(const InputHandler&) = delete;
 
     // Movable
     InputHandler(InputHandler&&) noexcept;
     InputHandler& operator=(InputHandler&&) noexcept;
 
     /**
      * @brief Register a hotkey
      * 
      * @param modifiers Modifier keys (MOD_ALT, MOD_CONTROL, MOD_SHIFT, MOD_WIN)
      * @param virtualKey Virtual key code
      * @param description Human-readable description
      * @param global Whether the hotkey is global (works outside the app)
      * @param callback Function to call when the hotkey is pressed
      * @return int Hotkey ID, or -1 if registration failed
      */
     int RegisterHotkey(
         UINT modifiers,
         UINT virtualKey,
         const std::string& description,
         bool global,
         std::function<void()> callback
     );
 
     /**
      * @brief Unregister a hotkey
      * 
      * @param id Hotkey ID to unregister
      * @return true If unregistration succeeded
      * @return false If unregistration failed
      */
     bool UnregisterHotkey(int id);
 
     /**
      * @brief Handle window messages related to input
      * 
      * @param hwnd Window handle
      * @param uMsg Message type
      * @param wParam First message parameter
      * @param lParam Second message parameter
      * @return true If the message was handled
      * @return false If the message was not handled
      */
     bool HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
 
     /**
      * @brief Get a list of all registered hotkeys
      * 
      * @return std::vector<Hotkey> List of hotkeys
      */
     std::vector<Hotkey> GetHotkeys() const;
 
     /**
      * @brief Check if a key is currently pressed
      * 
      * @param virtualKey Virtual key code
      * @return true If the key is pressed
      * @return false If the key is not pressed
      */
     static bool IsKeyPressed(UINT virtualKey);
 
     /**
      * @brief Convert a hotkey to a human-readable string
      * 
      * @param modifiers Modifier keys
      * @param virtualKey Virtual key code
      * @return std::string Human-readable representation
      */
     static std::string HotkeyToString(UINT modifiers, UINT virtualKey);
 
 private:
     /**
      * @brief Generate a unique ID for a new hotkey
      * 
      * @return int Unique ID
      */
     int GenerateHotkeyId() const;
 
     Application& m_app;           ///< Reference to the main application
     HWND m_hwnd;                 ///< Window handle
     int m_nextHotkeyId;          ///< Next available hotkey ID
     
     struct HotkeyData {
         Hotkey hotkey;
         std::function<void()> callback;
     };
     
     std::unordered_map<int, HotkeyData> m_hotkeys;  ///< Registered hotkeys
 };
 
 } // namespace poe