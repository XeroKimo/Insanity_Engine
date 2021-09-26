// Insanity_Engine_Application.cpp : Defines the entry point for the application.
//

#include "Application/Application.h"
#include <exception>
#include <Windows.h>

using namespace InsanityEngine;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,  _In_ int nCmdShow)
{
    try
    {
        Application::RunApplication();
    }
    catch(std::exception e)
    {
        return 1;
    }

    return 0;
}