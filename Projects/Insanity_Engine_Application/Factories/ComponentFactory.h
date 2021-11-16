#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>

#include <concepts>
#include <any>
#include <optional>



namespace InsanityEngine
{

    template<class T>
    struct ComponentInitializer;

    template<class T>
    class Component
    {
    };

    template<class T>
    struct ComponentInitializer
    {

    };

    class UnknownComponentCreationCallback
    {
    public:
        void operator()(const void* initializer, void* obj) const
        {
            return ForewardCreation(initializer, obj);
        }

    private:
        virtual void ForewardCreation(const void*, void* obj) const = 0;
    };


    template <class ComponentType>
    using ComponentCreationFunction = std::function<Component<ComponentType>(const ComponentInitializer<ComponentType>& initializer)>;


    template <class ComponentType>
    class ComponentCreationCallback : public UnknownComponentCreationCallback
    {
        using CallbackType = ComponentCreationFunction<ComponentType>;

    private:
        CallbackType m_callback;

    public:
        ComponentCreationCallback(CallbackType callback) :
            m_callback(callback)
        {

        }

    private:
        void ForewardCreation(const void* initializer, void* obj) const override
        {
            std::optional<Component<ComponentType>>& opt = *reinterpret_cast<std::optional<Component<ComponentType>>*>(obj);
            const ComponentInitializer<ComponentType>& init = *reinterpret_cast<const ComponentInitializer<ComponentType>*>(initializer);
            opt = m_callback(init);
        }
    };



    class ComponentFactory
    {
        std::unordered_map<std::type_index, std::unique_ptr<UnknownComponentCreationCallback>> m_componentCreationCallbacks;

    public:
        template<class ComponentType>
        void RegisterComponentCreationCallback(ComponentCreationFunction<ComponentType> callback)
        {
            m_componentCreationCallbacks.insert({ typeid(ComponentType), std::make_unique<ComponentCreationCallback<ComponentType>>(callback) });
        }

        template<class ComponentType>
        void UnregisteryComponentCreationCallback()
        {
            m_componentCreationCallbacks.erase(typeid(ComponentType));
        }

        template<class ComponentType>
        Component<ComponentType> CreateComponent(const ComponentInitializer<ComponentType>& initializer)
        {
            std::optional<Component<ComponentType>> obj;
            (*m_componentCreationCallbacks[typeid(ComponentType)])(&initializer, &obj);
            return std::move(obj.value());
        }

    };
}