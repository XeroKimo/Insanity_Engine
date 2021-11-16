#include "ResourceFactory.h"

namespace InsanityEngine
{
    UnknownResource::UnknownResource(std::string_view name) :
        m_name(name)
    {
    }

    //InsanityEngine::ResourceHandle<UnknownResource>::ResourceHandle(std::nullptr_t)
    //{
    //}
    //ResourceHandle<UnknownResource>& ResourceHandle<UnknownResource>::operator=(std::nullptr_t)
    //{
    //    m_resource = nullptr;
    //    return *this;
    //}
}