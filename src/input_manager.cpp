#include "input_manager.h"
#include <iostream>
#include <chrono>
#include "raylib.h"
#include "debugger.h"
#include <algorithm> // For std::swap

InputManager &InputManager::Instance()
{
    static InputManager instance;
    return instance;
}

InputManager::InputManager() : nextCallbackId_(1)
{
    InitializeKeyMappings();
    mousePosition_ = Vec2(0, 0);
    previousMousePosition_ = Vec2(0, 0);
    mouseWheelDelta_ = 0.0f;
}

InputManager::~InputManager()
{
    // UnregisterAllCallbacks(); TODO: implement
}

void InputManager::Update()
{
    //  previous frame state
    previousKeys_ = currentKeys_;
    previousMouseButtons_ = currentMouseButtons_;
    previousMousePosition_ = mousePosition_;

    keysPressed_.clear();
    keysReleased_.clear();
    mouseButtonsPressed_.clear();
    mouseButtonsReleased_.clear();
    mouseWheelDelta_ = 0.0f;

    // new frame state(new current)
    ProcessKeyboardInput();
    ProcessMouseInput();
}

void InputManager::ProcessKeyboardInput()
{
    for (int key = 32; key < 350; key++)
    { // ASCII + special keys
        bool isDown = ::IsKeyDown(key);
        bool wasDown = previousKeys_.count(key) > 0;

        if (isDown && !wasDown)
        {
            // Key just pressed
            keysPressed_.insert(key);
            currentKeys_.insert(key);
            // std::cout << "Key Just Pressed: " << key << std::endl;

            // Fire key down event
            InputEvent event;
            event.type = InputEventType::KeyDown;
            event.keyCode = key;
            event.keyName = GetKeyName(key);
            event.timestamp = GetTime();
            FireCallbacks(event);
        }
        else if (!isDown && wasDown)
        {
            // Key just released
            keysReleased_.insert(key);
            currentKeys_.erase(key);

            // Fire key up event
            InputEvent event;
            event.type = InputEventType::KeyUp;
            event.keyCode = key;
            event.keyName = GetKeyName(key);
            event.timestamp = GetTime();
            FireCallbacks(event);
        }
        else if (isDown)
        {
            // Key held down
            //    std::cout << "Key  Pressed: " << key << std::endl;
            currentKeys_.insert(key);
        }
    }
}

void InputManager::ProcessMouseInput()
{

    Vector2 pos = ::GetMousePosition();
    mousePosition_ = Vec2(pos.x, pos.y);

    Vec2 delta = Vec2(
        mousePosition_.x - previousMousePosition_.x,
        mousePosition_.y - previousMousePosition_.y);

    if (delta.x != 0 || delta.y != 0)
    {
        InputEvent event;
        event.type = InputEventType::MouseMove;
        event.mousePosition = mousePosition_;
        event.mouseDelta = delta;
        event.timestamp = GetTime();
        FireCallbacks(event);
    }

    //  mouse buttons
    for (int button = 0; button <= MOUSE_BUTTON_FORWARD; button++)
    {
        bool isDown = ::IsMouseButtonDown(button);
        bool wasDown = previousMouseButtons_.count(button) > 0;

        if (isDown && !wasDown)
        {
            // Button just pressed
            mouseButtonsPressed_.insert(button);
            currentMouseButtons_.insert(button);

            InputEvent event;
            event.type = InputEventType::MouseDown;
            event.mouseButton = button;
            event.mousePosition = mousePosition_;
            event.timestamp = GetTime();
            FireCallbacks(event);
        }
        else if (!isDown && wasDown)
        {
            // Button just released
            mouseButtonsReleased_.insert(button);
            currentMouseButtons_.erase(button);

            InputEvent event;
            event.type = InputEventType::MouseUp;
            event.mouseButton = button;
            event.mousePosition = mousePosition_;
            event.timestamp = GetTime();
            FireCallbacks(event);
        }
        else if (isDown)
        {
            currentMouseButtons_.insert(button);
        }
    }
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0.0f)
    {
        mouseWheelDelta_ = wheelMove;

        InputEvent event;
        event.type = InputEventType::MouseWheel;
        event.mousePosition = mousePosition_;
        event.wheelDelta = wheelMove;
        event.timestamp = GetTime();
        FireCallbacks(event);
    }
}

void InputManager::FireCallbacks(const InputEvent &event)
{
    for (const auto &callbackData : callbacks_)
    {
        bool shouldFire = false;

        // Check if this callback should fire for this event
        switch (event.type)
        {
        case InputEventType::KeyDown:
        case InputEventType::KeyUp:
            shouldFire = (callbackData.eventType == event.type) &&
                         (callbackData.keyCode == -1 || callbackData.keyCode == event.keyCode);
            break;

        case InputEventType::MouseDown:
        case InputEventType::MouseUp:
            shouldFire = (callbackData.eventType == event.type) &&
                         (callbackData.mouseButton == -1 || callbackData.mouseButton == event.mouseButton);
            break;

        case InputEventType::MouseMove:
        case InputEventType::MouseWheel:
            shouldFire = (callbackData.eventType == event.type);
            break;
        }

        if (shouldFire)
        {
            try
            {
                callbackData.callback(event);
            }
            catch (...)
            {

                Debugger::Instance().LogError("Error in input callback");
            }
        }
    }
}

bool InputManager::IsKeyDown(int keyCode) const
{

    return currentKeys_.count(keyCode) > 0;
}

bool InputManager::IsKeyDown(const std::string &keyName) const
{
    int keyCode = GetKeyCode(keyName);
    return keyCode != -1 && IsKeyDown(keyCode);
}

bool InputManager::IsKeyPressed(int keyCode) const
{
    return keysPressed_.count(keyCode) > 0;
}

bool InputManager::IsKeyPressed(const std::string &keyName) const
{
    int keyCode = GetKeyCode(keyName);
    return keyCode != -1 && IsKeyPressed(keyCode);
}

bool InputManager::IsKeyReleased(int keyCode) const
{
    return keysReleased_.count(keyCode) > 0;
}

bool InputManager::IsKeyReleased(const std::string &keyName) const
{
    int keyCode = GetKeyCode(keyName);
    return keyCode != -1 && IsKeyReleased(keyCode);
}

Vec2 InputManager::GetMousePosition() const
{
    return mousePosition_;
}

Vec2 InputManager::GetMouseDelta() const
{
    return Vec2(
        mousePosition_.x - previousMousePosition_.x,
        mousePosition_.y - previousMousePosition_.y);
}

float InputManager::GetMouseWheelDelta() const
{
    return mouseWheelDelta_;
}

bool InputManager::IsMouseButtonDown(int btn) const
{
    return currentMouseButtons_.count(btn) > 0;
}

bool InputManager::IsMouseButtonPressed(int btn) const
{
    return mouseButtonsPressed_.count(btn) > 0;
}

bool InputManager::IsMouseButtonReleased(int btn) const
{
    return mouseButtonsReleased_.count(btn) > 0;
}

InputCallbackId InputManager::RegisterKeyCallback(int keyCode, InputEventType eventType, InputCallback callback)
{
    InputCallbackId id = GenerateCallbackId();
    CallbackData data;
    data.id = id;
    data.eventType = eventType;
    data.keyCode = keyCode;
    data.mouseButton = -1;
    data.callback = callback;

    callbacks_.push_back(data);
    return id;
}

InputCallbackId InputManager::RegisterKeyCallback(const std::string &keyName, InputEventType eventType, InputCallback callback)
{
    int keyCode = GetKeyCode(keyName);
    if (keyCode == -1)
    {
        Debugger::Instance().LogWarn("Warning: Unknown key name: " + keyName);
        return 0;
    }
    return RegisterKeyCallback(keyCode, eventType, callback);
}

InputCallbackId InputManager::RegisterMouseCallback(int mouseButton, InputEventType eventType, InputCallback callback)
{
    InputCallbackId id = GenerateCallbackId();
    CallbackData data;
    data.id = id;
    data.eventType = eventType;
    data.mouseButton = mouseButton;
    data.callback = callback;

    callbacks_.push_back(data);
    return id;
};
InputCallbackId InputManager::RegisterMouseMoveCallback(InputCallback callback){
        InputCallbackId id = GenerateCallbackId();
    CallbackData data;
    data.id = id;
    data.eventType = InputEventType::MouseMove;
    data.mouseButton = -1;
    data.callback = callback;

    callbacks_.push_back(data);
    return id;
};
InputCallbackId InputManager::RegisterMouseWheelCallback(InputCallback callback){
    InputCallbackId id = GenerateCallbackId();
    CallbackData data;
    data.id = id;
    data.eventType = InputEventType::MouseWheel;
    data.mouseButton = -1;
    data.callback = callback;

    callbacks_.push_back(data);
    return id;
};

void InputManager::UnregisterCallback(InputCallbackId callbackId)
{

    for (size_t i = 0; i < callbacks_.size(); ++i)
    {
        CallbackData cb = callbacks_[i];
        if (cb.id == callbackId)
        {
            std::swap(callbacks_[i], callbacks_.back());
            callbacks_.pop_back();
            return;
        }
    }
}

void InputManager::UnregisterAllCallbacks()
{
    callbacks_.clear();
}

void InputManager::InitializeKeyMappings()
{
    // Letters
    for (char c = 'A'; c <= 'Z'; c++)
    {
        keyNameToCode_[std::string(1, c)] = c;
        keyCodeToName_[c] = std::string(1, c);
    }

    // Numbers
    for (char c = '0'; c <= '9'; c++)
    {
        keyNameToCode_[std::string(1, c)] = c;
        keyCodeToName_[c] = std::string(1, c);
    }

    // Special keys
    keyNameToCode_["Space"] = KEY_SPACE;
    keyCodeToName_[KEY_SPACE] = "Space";

    keyNameToCode_["Enter"] = KEY_ENTER;
    keyCodeToName_[KEY_ENTER] = "Enter";

    keyNameToCode_["Escape"] = KEY_ESCAPE;
    keyCodeToName_[KEY_ESCAPE] = "Escape";

    keyNameToCode_["Backspace"] = KEY_BACKSPACE;
    keyCodeToName_[KEY_BACKSPACE] = "Backspace";

    keyNameToCode_["Tab"] = KEY_TAB;
    keyCodeToName_[KEY_TAB] = "Tab";

    keyNameToCode_["Shift"] = KEY_LEFT_SHIFT;
    keyCodeToName_[KEY_LEFT_SHIFT] = "Shift";

    keyNameToCode_["Control"] = KEY_LEFT_CONTROL;
    keyCodeToName_[KEY_LEFT_CONTROL] = "Control";

    keyNameToCode_["Alt"] = KEY_LEFT_ALT;
    keyCodeToName_[KEY_LEFT_ALT] = "Alt";

    // Arrow keys
    keyNameToCode_["ArrowUp"] = KEY_UP;
    keyCodeToName_[KEY_UP] = "ArrowUp";

    keyNameToCode_["ArrowDown"] = KEY_DOWN;
    keyCodeToName_[KEY_DOWN] = "ArrowDown";

    keyNameToCode_["ArrowLeft"] = KEY_LEFT;
    keyCodeToName_[KEY_LEFT] = "ArrowLeft";

    keyNameToCode_["ArrowRight"] = KEY_RIGHT;
    keyCodeToName_[KEY_RIGHT] = "ArrowRight";

    // Function keys
    for (int i = 1; i <= 12; i++)
    {
        std::string name = "F" + std::to_string(i);
        int keyCode = KEY_F1 + (i - 1);
        keyNameToCode_[name] = keyCode;
        keyCodeToName_[keyCode] = name;
    }

    // Punctuation keys
    keyNameToCode_[","] = KEY_COMMA;
    keyCodeToName_[KEY_COMMA] = ",";

    keyNameToCode_["."] = KEY_PERIOD;
    keyCodeToName_[KEY_PERIOD] = ".";

    keyNameToCode_["/"] = KEY_SLASH;
    keyCodeToName_[KEY_SLASH] = "/";

    keyNameToCode_[";"] = KEY_SEMICOLON;
    keyCodeToName_[KEY_SEMICOLON] = ";";

    keyNameToCode_["'"] = KEY_APOSTROPHE;
    keyCodeToName_[KEY_APOSTROPHE] = "'";

    keyNameToCode_["["] = KEY_LEFT_BRACKET;
    keyCodeToName_[KEY_LEFT_BRACKET] = "[";

    keyNameToCode_["]"] = KEY_RIGHT_BRACKET;
    keyCodeToName_[KEY_RIGHT_BRACKET] = "]";

    keyNameToCode_["\\"] = KEY_BACKSLASH;
    keyCodeToName_[KEY_BACKSLASH] = "\\";

    keyNameToCode_["-"] = KEY_MINUS;
    keyCodeToName_[KEY_MINUS] = "-";

    keyNameToCode_["="] = KEY_EQUAL;
    keyCodeToName_[KEY_EQUAL] = "=";

    keyNameToCode_["`"] = KEY_GRAVE;
    keyCodeToName_[KEY_GRAVE] = "`";

    // Additional special keys
    keyNameToCode_["Insert"] = KEY_INSERT;
    keyCodeToName_[KEY_INSERT] = "Insert";

    keyNameToCode_["Delete"] = KEY_DELETE;
    keyCodeToName_[KEY_DELETE] = "Delete";

    keyNameToCode_["Home"] = KEY_HOME;
    keyCodeToName_[KEY_HOME] = "Home";

    keyNameToCode_["End"] = KEY_END;
    keyCodeToName_[KEY_END] = "End";

    keyNameToCode_["PageUp"] = KEY_PAGE_UP;
    keyCodeToName_[KEY_PAGE_UP] = "PageUp";

    keyNameToCode_["PageDown"] = KEY_PAGE_DOWN;
    keyCodeToName_[KEY_PAGE_DOWN] = "PageDown";

    keyNameToCode_["CapsLock"] = KEY_CAPS_LOCK;
    keyCodeToName_[KEY_CAPS_LOCK] = "CapsLock";

    keyNameToCode_["ScrollLock"] = KEY_SCROLL_LOCK;
    keyCodeToName_[KEY_SCROLL_LOCK] = "ScrollLock";

    keyNameToCode_["NumLock"] = KEY_NUM_LOCK;
    keyCodeToName_[KEY_NUM_LOCK] = "NumLock";

    keyNameToCode_["PrintScreen"] = KEY_PRINT_SCREEN;
    keyCodeToName_[KEY_PRINT_SCREEN] = "PrintScreen";

    keyNameToCode_["Pause"] = KEY_PAUSE;
    keyCodeToName_[KEY_PAUSE] = "Pause";

    // Right modifier keys
    keyNameToCode_["RightShift"] = KEY_RIGHT_SHIFT;
    keyCodeToName_[KEY_RIGHT_SHIFT] = "RightShift";

    keyNameToCode_["RightControl"] = KEY_RIGHT_CONTROL;
    keyCodeToName_[KEY_RIGHT_CONTROL] = "RightControl";

    keyNameToCode_["RightAlt"] = KEY_RIGHT_ALT;
    keyCodeToName_[KEY_RIGHT_ALT] = "RightAlt";

    keyNameToCode_["RightSuper"] = KEY_RIGHT_SUPER;
    keyCodeToName_[KEY_RIGHT_SUPER] = "RightSuper";

    // Keypad keys
    for (int i = 0; i <= 9; i++)
    {
        std::string name = "Keypad" + std::to_string(i);
        int keyCode = KEY_KP_0 + i;
        keyNameToCode_[name] = keyCode;
        keyCodeToName_[keyCode] = name;
    }

    keyNameToCode_["KeypadDecimal"] = KEY_KP_DECIMAL;
    keyCodeToName_[KEY_KP_DECIMAL] = "KeypadDecimal";

    keyNameToCode_["KeypadDivide"] = KEY_KP_DIVIDE;
    keyCodeToName_[KEY_KP_DIVIDE] = "KeypadDivide";

    keyNameToCode_["KeypadMultiply"] = KEY_KP_MULTIPLY;
    keyCodeToName_[KEY_KP_MULTIPLY] = "KeypadMultiply";

    keyNameToCode_["KeypadSubtract"] = KEY_KP_SUBTRACT;
    keyCodeToName_[KEY_KP_SUBTRACT] = "KeypadSubtract";

    keyNameToCode_["KeypadAdd"] = KEY_KP_ADD;
    keyCodeToName_[KEY_KP_ADD] = "KeypadAdd";

    keyNameToCode_["KeypadEnter"] = KEY_KP_ENTER;
    keyCodeToName_[KEY_KP_ENTER] = "KeypadEnter";

    keyNameToCode_["KeypadEqual"] = KEY_KP_EQUAL;
    keyCodeToName_[KEY_KP_EQUAL] = "KeypadEqual";

    keyNameToCode_["Menu"] = KEY_KB_MENU;
    keyCodeToName_[KEY_KB_MENU] = "Menu";
}

std::string InputManager::GetKeyName(int keyCode) const
{
    auto it = keyCodeToName_.find(keyCode);
    if (it != keyCodeToName_.end())
    {
        return it->second;
    }
    return "Unknown";
}

int InputManager::GetKeyCode(const std::string &keyName) const
{
    auto it = keyNameToCode_.find(keyName);
    if (it != keyNameToCode_.end())
    {
        return it->second;
    }
    return -1;
}

InputCallbackId InputManager::GenerateCallbackId()
{
    return nextCallbackId_++;
}