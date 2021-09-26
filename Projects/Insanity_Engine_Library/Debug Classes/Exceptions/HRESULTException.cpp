#include "HRESULTException.h"

namespace InsanityEngine::Debug::Exceptions
{
    HRESULTException::HRESULTException(const std::string& message, HRESULT result) : std::runtime_error(Message(message, result)) 
    {
    }

    std::string HRESULTException::Message(const std::string& message, HRESULT result)
    {
        std::stringstream stream(message, std::ios_base::in | std::ios_base::out | std::ios_base::ate);
        stream << " HRESULT: " << std::hex << result;
        return stream.str();
    }
}