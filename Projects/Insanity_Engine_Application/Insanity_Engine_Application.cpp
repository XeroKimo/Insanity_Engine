// Insanity_Engine_Application.cpp : Defines the entry point for the application.
//

#include "Application/Application.h"
#include "SDL_config_windows.h"
#include "SDL.h"
#include <exception>
#include <Windows.h>

using namespace InsanityEngine;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,  _In_ int nCmdShow)
{
    SDL_Window* window;

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Hello world", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WindowFlags::SDL_WINDOW_SHOWN);

    SDL_Event event;
    while(true)
    {
        if(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EventType::SDL_QUIT)
            {
                break;
            }
        }
        else
        {

        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    //try
    //{
    //    Application::RunApplication();
    //}
    //catch(std::exception e)
    //{
    //    return 1;
    //}

    return 0;
}