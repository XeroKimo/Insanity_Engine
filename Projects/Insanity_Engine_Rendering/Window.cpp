#include "Window.h"

namespace InsanityEngine::Rendering
{
    HWND WindowBase::GetRawHandle() const
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        SDL_GetWindowWMInfo(m_windowHandle.get(), &info);
        return info.info.win.window;
    }
}