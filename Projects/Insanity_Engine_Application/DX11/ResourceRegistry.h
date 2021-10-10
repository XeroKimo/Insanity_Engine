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
        class BaseRegistry
        {
        public:
            virtual RegisterStatus Register(std::string_view name, ComPtr<IUnknown> resource) = 0;
            virtual void Unregister(std::string_view name) = 0;

            virtual ComPtr<IUnknown> Get(std::string_view name) = 0;
        };

        template<class T>
        class Registry : public BaseRegistry
        {
        private:
            std::unordered_map<std::string, ComPtr<T>> registry;

        public:
            RegisterStatus Register(std::string_view name, ComPtr<IUnknown> resource) override
            {
                std::string nameCopy{ name };
                if(registry.contains(nameCopy))
                    return RegisterStatus::Name_Conflict;

                ComPtr<T> derived;
                resource.As(&derived);

                registry[nameCopy] = derived;

                return RegisterStatus::Succeeded;
            }

            void Unregister(std::string_view name) override
            {
                registry.erase(std::string(name));
            }

            ComPtr<IUnknown> Get(std::string_view name) override
            {
                auto it = registry.find(std::string(name));

                if(it == registry.end())
                    return nullptr;

                return it->second;
            }
        };

    private:
        std::unordered_map<std::type_index, std::unique_ptr<BaseRegistry>> m_registries;

    public:
        template<class T>
        RegisterStatus Register(std::string_view name, ComPtr<T> resource)
        {
            if(!m_registries.contains(std::type_index(typeid(T))))
                m_registries[std::type_index(typeid(T))] = std::make_unique<Registry<T>>();

            return m_registries[typeid(T)]->Register(name, resource);
        }

        template<class T>
        void Unregister(std::string_view name)
        {
            auto it = m_registries.find(std::type_index(typeid(T)));

            if(it == m_registries.end())
                return;

            it->second->Unregister(name);
        }

        template<class T>
        ComPtr<T> Get(std::string_view name)
        {
            auto it = m_registries.find(std::type_index(typeid(T)));

            if(it == m_registries.end())
                return nullptr;

            ComPtr<IUnknown> resource = it->second->Get(name);

            if(resource == nullptr)
                return nullptr;

            ComPtr<T> derived;
            resource.As(&derived);

            return derived;

        }
    };
}