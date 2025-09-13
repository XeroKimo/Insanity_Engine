module;
#include <cstdint>
#include <array>
export module InsanityEngine:Controller;

namespace InsanityEngine
{
    export enum class Key : std::uint8_t
    {
        // Alphanumeric keys
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        // Function keys
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,

        // Modifier keys
        ShiftLeft, ShiftRight,
        CtrlLeft, CtrlRight,
        AltLeft, AltRight,
        MetaLeft, MetaRight, // Windows / Command key

        // Navigation keys
        ArrowUp, ArrowDown, ArrowLeft, ArrowRight,
        Home, End, PageUp, PageDown,

        // Control keys
        Escape, Tab, CapsLock, Enter, Space,
        Backspace, Insert, Delete,

        // Symbols and punctuation
        Tilde, Minus, Equal, LeftBracket, RightBracket,
        Backslash, Semicolon, Apostrophe, Comma, Period, Slash,

        // Numpad keys
        NumPad0, NumPad1, NumPad2, NumPad3, NumPad4,
        NumPad5, NumPad6, NumPad7, NumPad8, NumPad9,
        NumPadAdd, NumPadSubtract, NumPadMultiply, NumPadDivide,
        NumPadDecimal, NumPadEnter,

        // Lock keys
        NumLock, ScrollLock,

        // Media keys (optional)
        VolumeUp, VolumeDown, Mute,
        PlayPause, Stop, NextTrack, PrevTrack,

        // System keys
        PrintScreen, Pause, Menu,

        // Undefined or custom
        Unknown
    };

    namespace Controller
    {
        std::array<std::uint64_t, 4> previousKeyState = {};
        std::array<std::uint64_t, 4> currentKeyState = {};

        void Set(Key key)
        {
            const int index = static_cast<std::uint8_t>(key) / 64;
            const int offset = static_cast<std::uint64_t>(1) << (static_cast<std::uint8_t>(key) % 64);
            const uint64_t flag = std::uint64_t(1) << offset;
            currentKeyState[index] |= flag;
        }

        void Reset(Key key)
        {
            const int index = static_cast<std::uint8_t>(key) / 64;
            const int offset = static_cast<std::uint64_t>(1) << (static_cast<std::uint8_t>(key) % 64);
            const uint64_t flag = std::uint64_t(1) << offset;
            currentKeyState[index] &= ~flag;
        }

        export void ClearBuffer()
        {
            previousKeyState = currentKeyState;
        }

        export bool Pressed(Key key)
        {
            const int index = static_cast<std::uint8_t>(key) / 64;
            const int offset = static_cast<std::uint64_t>(1) << (static_cast<std::uint8_t>(key) % 64);
            const uint64_t flag = std::uint64_t(1) << offset;
            return currentKeyState[index] & flag && ~previousKeyState[index] & flag;
        }

        export bool Released(Key key)
        {
            const int index = static_cast<std::uint8_t>(key) / 64;
            const int offset = static_cast<std::uint64_t>(1) << (static_cast<std::uint8_t>(key) % 64);
            const uint64_t flag = std::uint64_t(1) << offset;
            return ~currentKeyState[index] & flag && previousKeyState[index] & flag;
        }

        export bool Held(Key key)
        {
            const int index = static_cast<std::uint8_t>(key) / 64;
            const int offset = static_cast<std::uint64_t>(1) << (static_cast<std::uint8_t>(key) % 64);
            const uint64_t flag = std::uint64_t(1) << offset;
            return currentKeyState[index] & previousKeyState[index] & flag;
        }

        export bool Relaxed(Key key)
        {
            const int index = static_cast<std::uint8_t>(key) / 64;
            const int offset = static_cast<std::uint64_t>(1) << (static_cast<std::uint8_t>(key) % 64);
            const uint64_t flag = std::uint64_t(1) << offset;
            return ~currentKeyState[index] & ~previousKeyState[index] & flag;
        }
    };
}