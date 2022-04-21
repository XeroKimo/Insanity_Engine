// Insanity_Engine_Application.cpp : Defines the entry point for the application.
//

#include "Application/Application.h"
#include <exception>
#include <Windows.h>

using namespace InsanityEngine;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,  _In_ int nCmdShow)
{
    Application::Settings settings
    {
        .applicationName = "Insanity Engine",
        .windowResolution = { 1280, 720 },
        .renderAPI = Rendering::RenderAPI::DX11
    };
    return Application::Run(settings);
}