#pragma once
#include <stdexcept>
#include <sstream>
#include <Windows.h>

namespace InsanityEngine::Debug::Exceptions
{
    class HRESULTException : public std::runtime_error
    {
    public:
        explicit HRESULTException(const std::string& message, HRESULT result); 

    private:
        std::string Message(const std::string& message, HRESULT result);

    };
}