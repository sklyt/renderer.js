#pragma once 


#include "raylib.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <string>
#include "renderer.h" // vec2




enum class InputEventType {
    KeyDown,
    KeyUp,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseWheel
};


struct InputEvent {
    InputEventType type;
    
 
    int keyCode;
    std::string keyName;
    

    int mouseButton;
    Vec2 mousePosition;
    Vec2 mouseDelta;
    float wheelDelta;
    
    // Timing
    double timestamp;
    
    InputEvent() : type(InputEventType::KeyDown), keyCode(0), mouseButton(0), 
                  mousePosition{0,0}, mouseDelta{0,0}, wheelDelta(0), timestamp(0) {}
};


using InputCallback = std::function<void(const InputEvent&)>;
using InputCallbackId = uint32_t;


class InputManager {
public:
    static InputManager& Instance();
    
    InputManager();
    ~InputManager();
    
    void Update(); // once per frame to capture input
    void Clear(); 
    
    // Polling: no callback, boolean is light(a number) to return
    bool IsKeyDown(int keyCode) const;
    bool IsKeyDown(const std::string& keyName) const;
    bool IsKeyPressed(int keyCode) const; // Just pressed this frame
    bool IsKeyPressed(const std::string& keyName) const;
    bool IsKeyReleased(int keyCode) const; // Just released this frame
    bool IsKeyReleased(const std::string& keyName) const;
    
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonReleased(int button) const;
    
    Vec2 GetMousePosition() const;
    Vec2 GetMouseDelta() const;
    float GetMouseWheelDelta() const;
    
    // Event interface (good for discrete actions like menu clicks)
    InputCallbackId RegisterKeyCallback(int keyCode, InputEventType eventType, InputCallback callback);
    InputCallbackId RegisterKeyCallback(const std::string& keyName, InputEventType eventType, InputCallback callback);
    InputCallbackId RegisterMouseCallback(int mouseButton, InputEventType eventType, InputCallback callback);
    InputCallbackId RegisterMouseMoveCallback(InputCallback callback);
    InputCallbackId RegisterMouseWheelCallback(InputCallback callback);
    
    void UnregisterCallback(InputCallbackId callbackId);
    void UnregisterAllCallbacks();
    

    std::string GetKeyName(int keyCode) const;
    int GetKeyCode(const std::string& keyName) const;
    
private:
    void ProcessKeyboardInput();
    void ProcessMouseInput();
    void FireCallbacks(const InputEvent& event);
    
    InputCallbackId GenerateCallbackId();
    
    // Current frame state
    std::unordered_set<int> currentKeys_;
    std::unordered_set<int> currentMouseButtons_;
    
    // Previous frame state (for detecting pressed/released)
    std::unordered_set<int> previousKeys_;
    std::unordered_set<int> previousMouseButtons_;
    
    // Frame-specific events
    std::unordered_set<int> keysPressed_;
    std::unordered_set<int> keysReleased_;
    std::unordered_set<int> mouseButtonsPressed_;
    std::unordered_set<int> mouseButtonsReleased_;
    
    Vec2 mousePosition_;
    Vec2 previousMousePosition_;
    float mouseWheelDelta_;
    
    // Event system
    struct CallbackData {
        InputCallbackId id;
        InputEventType eventType;
        int keyCode; // -1 for any key
        int mouseButton; // -1 for any button
        InputCallback callback;
    };
    
    std::vector<CallbackData> callbacks_;
    InputCallbackId nextCallbackId_;
    
    // Key name mapping
    std::unordered_map<std::string, int> keyNameToCode_;
    std::unordered_map<int, std::string> keyCodeToName_;
    void InitializeKeyMappings();
};


