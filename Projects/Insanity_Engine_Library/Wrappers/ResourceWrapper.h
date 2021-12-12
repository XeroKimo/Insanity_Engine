#pragma once
#include <memory>

namespace InsanityEngine
{
    template<class T>
    struct Resource;

    template<class T>
    class ResourceHandle;

    template<class T>
    class SharedResourceHandle
    {
        template<class Derived>
        friend class SharedResourceHandle;

    private:
        std::shared_ptr<Resource<T>> m_resource;

    public:
        SharedResourceHandle() = default;
        SharedResourceHandle(std::nullptr_t) {}
        SharedResourceHandle(std::shared_ptr<Resource<T>> resource) : m_resource(resource) {}
        SharedResourceHandle(const SharedResourceHandle& other) : m_resource(other.m_resource) {};
        SharedResourceHandle(SharedResourceHandle&& other) noexcept : m_resource(std::move(other.m_resource)) {};

        template<class Derived>
        SharedResourceHandle(std::shared_ptr<Resource<Derived>> resource) : m_resource(resource) {}

        template<class Derived>
        SharedResourceHandle(const ResourceHandle<Derived>& other) : m_resource(other.m_resource) {}

        template<class Derived>
        SharedResourceHandle(ResourceHandle<Derived>&& other) noexcept : m_resource(std::move(other.resource)) {}

    public:
        SharedResourceHandle& operator=(std::nullptr_t) { m_resource = nullptr; }

        SharedResourceHandle& operator=(const SharedResourceHandle& other)
        {
            m_resource = other.m_resource;
            return *this;
        }
        SharedResourceHandle& operator=(SharedResourceHandle&& other) noexcept
        {
            m_resource = std::move(other.m_resource);
            return *this;
        }
        bool operator==(std::nullptr_t) const { return m_resource == nullptr; }
        bool operator!=(std::nullptr_t) const { return m_resource != nullptr; }


        template<class Derived>
        bool operator==(const ResourceHandle<Derived>& other) const { return m_resource == other.m_resource; }

        template<class Derived>
        bool operator!=(const ResourceHandle<Derived>& other) const { return m_resource != other.m_resource; }
    
        operator bool() const { return m_resource != nullptr; }

        //Resource<T>* operator->() { return m_resource.get(); }
        //Resource<T>& operator*() { return *m_resource; }
    public:
        std::shared_ptr<Resource<T>> GetUnderlyingResource() const { return m_resource; }
        Resource<T>& Get() { return *m_resource; }
        const Resource<T>& Get() const { return *m_resource; }
    };
}