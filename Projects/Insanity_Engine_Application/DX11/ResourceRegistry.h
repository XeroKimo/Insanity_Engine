#pragma once
#include "CommonInclude.h"
#include <typeindex>
#include <unordered_map>
#include <memory>

namespace InsanityEngine::DX11
{

    enum class RegisterStatus
    {
        Succeeded,
        Name_Conflict
    };


    class ResourceRegistry
    {
    private:
        std::unordered_map<std::string, ComPtr<IUnknown>> m_resources;

    public:
        RegisterStatus Register(std::string_view name, ComPtr<IUnknown> resource)
        {
            std::string nameCopy{ name };
            if(m_resources.contains(nameCopy))
                return RegisterStatus::Name_Conflict;

            m_resources[nameCopy] = resource;

            return RegisterStatus::Succeeded;
        }


        void Unregister(std::string_view name)
        {
            auto it = m_resources.find(std::string(name));

            if(it == m_resources.end())
                return;

            m_resources.erase(it);
        }

        ComPtr<IUnknown> Get(std::string_view name)
        {
            return Get<IUnknown>(name);
        }

        template<class T>
        ComPtr<T> Get(std::string_view name)
        {
            auto it = m_resources.find(std::string(name));

            if(it == m_resources.end())
                return nullptr;

            if constexpr(std::is_same_v<T, IUnknown>)
                return it->second;
            else
            {
                ComPtr<T> derived;
                it->second.As(&derived);

                return derived;
            }
        }
    };
}