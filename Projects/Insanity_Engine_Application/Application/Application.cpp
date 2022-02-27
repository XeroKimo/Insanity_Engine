#include "Application.h"
#include "../Rendering/Window.h"

namespace InsanityEngine::Application
{
    int Run(const Settings& settings)
    {
        Rendering::DefaultWindow window(settings.applicationName, { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED }, settings.windowResolution, SDL_WINDOW_SHOWN);

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
