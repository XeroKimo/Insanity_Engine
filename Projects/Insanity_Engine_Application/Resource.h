#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>

#include <concepts>



namespace InsanityEngine
{
    template<class FunctionSig, class OptClass>
    struct FunctionPointerImpl;

    template<class RetType, class... Params>
    struct FunctionPointerImpl<RetType(Params...), void>
    {
        using type = RetType(*)(Params...);
    };

    template<class RetType, class... Params, class Type>
    struct FunctionPointerImpl<RetType(Params...), Type>
    {
        using type = RetType(Type::*)(Params...);
    };

    template<class FunctionSig, class OptClass = void>
    using FunctionPointer = typename FunctionPointerImpl<FunctionSig, OptClass>::type;




    template<class T>
    struct ResourceInitializer;

    class UnknownResource : protected std::enable_shared_from_this<UnknownResource>
    {
    public:
        UnknownResource(std::string_view name);

    private:
        std::string m_name;

    public:
        std::string GetName() const { return m_name; }
    };

    template<>
    struct ResourceInitializer<UnknownResource>
    {
        std::string_view name;

    };

    template<class T>
    struct ResourceInitializer : ResourceInitializer<UnknownResource>
    {

    };

    template<class T>
    class Resource : public UnknownResource
    {
    private:
        T m_underlyingResource;

    public:
        Resource(std::string_view name, T m_resource) :
            UnknownResource(name),
            m_underlyingResource(std::move(m_resource))
        {

        }

    public:
        T& Get() { return m_underlyingResource; }

    };


    class UnknownResourceCreationCallback
    {
    public:
        std::shared_ptr<UnknownResource> operator()(const ResourceInitializer<UnknownResource>& initializer) const
        {
            return ForewardCreation(initializer);
        }

    private:
        virtual std::shared_ptr<UnknownResource> ForewardCreation(const ResourceInitializer<UnknownResource>& initializer) const = 0;
    };


    template <class ResourceType>
    using ResourceCreationFunction = std::function<std::shared_ptr<Resource<ResourceType>>(const ResourceInitializer<ResourceType>& initializer)>;


    template <class ResourceType>
    class ResourceCreationCallback : public UnknownResourceCreationCallback
    {
        using CallbackType = std::function<std::shared_ptr<Resource<ResourceType>>(const ResourceInitializer<ResourceType>& initializer)>;

    private:
        CallbackType m_callback;

    public:
        ResourceCreationCallback(CallbackType callback) :
            m_callback(callback)
        {

        }

    private:
        std::shared_ptr<UnknownResource> ForewardCreation(const ResourceInitializer<UnknownResource>& initializer) const override
        {
            return m_callback(static_cast<const ResourceInitializer<ResourceType>&>(initializer));
        }
    };


    class UnknownResourceGetterCallback
    {
    public:
        std::shared_ptr<UnknownResource> operator()(std::string_view name) const
        {
            return ForewardGetter(name);
        }

    private:
        virtual std::shared_ptr<UnknownResource> ForewardGetter(std::string_view name) const = 0;
    };



    template <class ResourceType, class CreatorType>
    class ResourceGetterCallback : public UnknownResourceGetterCallback
    {
        using CallbackType = FunctionPointer<std::shared_ptr<Resource<ResourceType>>(std::string_view name), CreatorType>;
    private:
        CreatorType* m_creator = nullptr;
        CallbackType m_callback = nullptr;

    public:
        ResourceGetterCallback(CreatorType& creator, CallbackType callback) :
            m_creator(&creator),
            m_callback(callback)
        {

        }

    private:
        std::shared_ptr<UnknownResource> ForewardGetter(std::string_view name) const override
        {
            return std::invoke(m_callback, m_creator, name);
        }
    };


    class ResourceFactory
    {
        std::unordered_map<std::type_index, std::unique_ptr<UnknownResourceCreationCallback>> m_resourceCreationCallbacks;
        std::unordered_map<std::type_index, std::unique_ptr<UnknownResourceGetterCallback>> m_resourceGetterCallbacks;

    public:
        template<class ResourceType>
        void AddResourceCreationCallback(ResourceCreationFunction<ResourceType> callback)
        {
            m_resourceCreationCallbacks.insert({ typeid(ResourceType), std::make_unique<ResourceCreationCallback<ResourceType>>(callback) });
        }

        template<class ResourceType>
        void RemoveResourceCreationCallback()
        {
            m_resourceCreationCallbacks.erase(typeid(ResourceType));
        }

        template<class ResourceType>
        void AddResourceGetterCallback(const UnknownResourceGetterCallback& callback)
        {
            m_resourceCreationCallbacks[typeid(ResourceType)] = &callback;
        }

        template<class ResourceType>
        void RemoveResourceGetterCallback()
        {
            m_resourceCreationCallbacks.erase(typeid(ResourceType));
        }

        template<class ResourceType>
        std::shared_ptr<Resource<ResourceType>> CreateResource(const ResourceInitializer<ResourceType>& initializer)
        {
            return std::static_pointer_cast<Resource<ResourceType>>((*m_resourceCreationCallbacks[typeid(ResourceType)])(initializer));
        }

        template<class ResourceType>
        std::shared_ptr<Resource<ResourceType>> GetResource(std::string_view name)
        {
            return std::static_pointer_cast<Resource<ResourceType>>((*m_resourceGetterCallbacks[typeid(ResourceType)])(name));
        }
    };
}