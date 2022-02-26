#include "Application.h"
#include "../Rendering/Window.h"

namespace InsanityEngine::Application
{
    int Run()
    {
        Rendering::DefaultWindow window("Insanity Engine", { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED }, { 1280, 720 }, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

        SDL_Event event;
        while(true)
        {
            if(SDL_PollEvent(&event))
            {
                if(event.type == SDL_EventType::SDL_QUIT)
                    break;
            }
            else
            {

            }
        }

        return 0;
    }
}
