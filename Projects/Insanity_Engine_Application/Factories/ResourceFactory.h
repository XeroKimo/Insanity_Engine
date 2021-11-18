#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>

#include <concepts>



namespace InsanityEngine
{
    //template<class FunctionSig, class OptClass>
    //struct FunctionPointerImpl;

    //template<class RetType, class... Params>
    //struct FunctionPointerImpl<RetType(Params...), void>
    //{
    //    using type = RetType(*)(Params...);
    //};

    //template<class RetType, class... Params, class Type>
    //struct FunctionPointerImpl<RetType(Params...), Type>
    //{
    //    using type = RetType(Type::*)(Params...);
    //};

    //template<class FunctionSig, class OptClass = void>
    //using FunctionPointer = typename FunctionPointerImpl<FunctionSig, OptClass>::type;



    template<class T>
    struct ResourceInitializer;

    class UnknownResource
    {
    public:
        UnknownResource(std::string_view name);

        virtual ~UnknownResource() = default;

    private:
        std::string m_name;

    public:
        std::string GetName() const { return m_name; }
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
        const T& Get() const { return m_underlyingResource; }

    };

    template<class T>
    class ResourceHandle;


    template<class T>
    class UserDefinedResourceHandle
    {
        template<class Others>
        friend class UserDefinedResourceHandle;

    private:
        template<class To, class From>
        friend ResourceHandle<To> StaticResourceCast(const UserDefinedResourceHandle<From>& handle);

        template<class To, class From>
        friend ResourceHandle<To> DynamicResourceCast(const UserDefinedResourceHandle<From>& handle);

    private:
        std::shared_ptr<Resource<T>> m_resource;

    public:
        UserDefinedResourceHandle() = default;
        UserDefinedResourceHandle(std::nullptr_t)
        {
        }

        UserDefinedResourceHandle(std::shared_ptr<Resource<T>> resource) :
            m_resource(resource)
        {

        }

        template<class Derived>
        UserDefinedResourceHandle(std::shared_ptr<Resource<Derived>> resource) :
            m_resource(resource)
        {

        }

        template<class Derived>
        UserDefinedResourceHandle(const UserDefinedResourceHandle<Derived>& other) requires (std::is_same_v<T, UnknownResource>) :
            m_resource(other.m_resource)
        {
        }

        template<class Derived>
        UserDefinedResourceHandle(const UserDefinedResourceHandle<Derived>& other) requires (std::is_convertible_v<Resource<Derived>*, Resource<T>*>):
            m_resource(other.m_resource)
        {
        }

        UserDefinedResourceHandle(const UserDefinedResourceHandle<T>& other) :
            m_resource(other.m_resource)
        {
        }

        UserDefinedResourceHandle(UserDefinedResourceHandle<T> && other) noexcept :
            m_resource(std::move(other.m_resource))
        {
        }

        ~UserDefinedResourceHandle() = default;

    public:
        UserDefinedResourceHandle& operator=(std::nullptr_t) { m_resource = nullptr; }

        UserDefinedResourceHandle& operator=(const UserDefinedResourceHandle&other)
        {
            m_resource = other.m_resource;
            return *this;
        }
        UserDefinedResourceHandle& operator=(UserDefinedResourceHandle && other) noexcept
        {
            m_resource = std::move(other.m_resource);
            return *this;
        }
        bool operator==(std::nullptr_t) const { return m_resource == nullptr; }
        bool operator!=(std::nullptr_t) const { return m_resource != nullptr; }

    public:
        bool IsValid() const { return m_resource != nullptr; }

    protected:
        Resource<T>& GetResource() { return *m_resource; }
        const Resource<T>& GetResource() const { return *m_resource; }

        std::shared_ptr<Resource<T>> GetResourcePointer() const { return m_resource; }
    };
    
    template<>
    class UserDefinedResourceHandle<UnknownResource>
    {
        template<class Others>
        friend class UserDefinedResourceHandle;

    private:
        template<class To, class From>
        friend ResourceHandle<To> StaticResourceCast(const UserDefinedResourceHandle<From>& handle);

        template<class To, class From>
        friend ResourceHandle<To> DynamicResourceCast(const UserDefinedResourceHandle<From>& handle);

    private:
        std::shared_ptr<UnknownResource> m_resource;

    public:
        UserDefinedResourceHandle() = default;
        UserDefinedResourceHandle(std::nullptr_t)
        {
        }

        UserDefinedResourceHandle(std::shared_ptr<UnknownResource> resource) :
            m_resource(resource)
        {

        }

        template<class Derived>
        UserDefinedResourceHandle(std::shared_ptr<Resource<Derived>> resource) :
            m_resource(resource)
        {

        }

        template<class Derived>
        UserDefinedResourceHandle(const UserDefinedResourceHandle<Derived>& other) requires (std::is_convertible_v<Resource<Derived>*, UnknownResource*>) :
            m_resource(other.m_resource)
        {
        }

        UserDefinedResourceHandle(const UserDefinedResourceHandle& other) :
            m_resource(other.m_resource)
        {
        }

        UserDefinedResourceHandle(UserDefinedResourceHandle&& other) noexcept :
            m_resource(std::move(other.m_resource))
        {
        }

        ~UserDefinedResourceHandle() = default;

    public:
        UserDefinedResourceHandle& operator=(std::nullptr_t) { m_resource = nullptr; }

        UserDefinedResourceHandle& operator=(const UserDefinedResourceHandle& other)
        {
            m_resource = other.m_resource;
            return *this;
        }
        UserDefinedResourceHandle& operator=(UserDefinedResourceHandle&& other) noexcept
        {
            m_resource = std::move(other.m_resource);
            return *this;
        }

        bool operator==(std::nullptr_t) const { return m_resource == nullptr; }
        bool operator!=(std::nullptr_t) const { return m_resource != nullptr; }

    public:
        bool IsValid() const { return m_resource != nullptr; }

    protected:
        UnknownResource& GetResource() { return *m_resource; }
        const UnknownResource& GetResource() const { return *m_resource; }

        std::shared_ptr<UnknownResource> GetResourcePointer() const { return m_resource; }
    };


    template<class T>
    class ResourceHandle : public UserDefinedResourceHandle<T>
    {
    public:
        using UserDefinedResourceHandle<T>::UserDefinedResourceHandle;
    };


    template<class To, class From>
    ResourceHandle<To> StaticResourceCast(const UserDefinedResourceHandle<From>& handle)
    {
        return ResourceHandle<To>(std::static_pointer_cast<Resource<To>>(handle.GetResourcePointer()));
    }

    template<class To, class From>
    ResourceHandle<To> DynamicResourceCast(const UserDefinedResourceHandle<From>& handle)
    {
        return ResourceHandle<To>(std::dynamic_pointer_cast<Resource<To>>(handle.GetResourcePointer()));
    }



    template<>
    struct ResourceInitializer<UnknownResource>
    {
        std::string_view name;
    };

    template<class T>
    struct ResourceInitializer : ResourceInitializer<UnknownResource>
    {
        T resource;
    };



    template <class ResourceType>
    using ResourceCreationFunction = std::function<std::shared_ptr<Resource<ResourceType>>(const ResourceInitializer<ResourceType>& initializer)>;
    class UnknownResourceCreationCallback
    {
    public:
        ResourceHandle<UnknownResource> operator()(const ResourceInitializer<UnknownResource>& initializer) const
        {
            return ForewardCreation(initializer);
        }

    private:
        virtual ResourceHandle<UnknownResource> ForewardCreation(const ResourceInitializer<UnknownResource>& initializer) const = 0;
    };


    template <class ResourceType>
    class ResourceCreationCallback : public UnknownResourceCreationCallback
    {
        using CallbackType = ResourceCreationFunction<ResourceType>;

    private:
        CallbackType m_callback;

    public:
        ResourceCreationCallback(CallbackType callback) :
            m_callback(callback)
        {

        }

    private:
        ResourceHandle<UnknownResource> ForewardCreation(const ResourceInitializer<UnknownResource>& initializer) const override
        {
            return m_callback(static_cast<const ResourceInitializer<ResourceType>&>(initializer));
        }
    };



    template <class ResourceType>
    using ResourceGetterFunction = std::function<std::shared_ptr<Resource<ResourceType>>(std::string_view name)>;

    class UnknownResourceGetterCallback
    {
    public:
        ResourceHandle<UnknownResource> operator()(std::string_view name) const
        {
            return ForewardGetter(name);
        }

    private:
        virtual ResourceHandle<UnknownResource> ForewardGetter(std::string_view name) const = 0;
    };



    template <class ResourceType, class CreatorType>
    class ResourceGetterCallback : public UnknownResourceGetterCallback
    {
        using CallbackType = ResourceGetterFunction<ResourceType>;
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
        ResourceHandle<UnknownResource> ForewardGetter(std::string_view name) const override
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
        ResourceHandle<ResourceType> CreateResource(const ResourceInitializer<ResourceType>& initializer)
        {
            return StaticResourceCast<ResourceType>((*m_resourceCreationCallbacks[typeid(ResourceType)])(initializer));
        }

        template<class ResourceType>
        ResourceHandle<ResourceType> GetResource(std::string_view name)
        {
            return StaticResourceCast<ResourceType>((*m_resourceGetterCallbacks[typeid(ResourceType)])(name));
        }
    };

}