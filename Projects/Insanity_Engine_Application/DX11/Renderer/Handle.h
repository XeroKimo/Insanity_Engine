#pragma once
#include <utility>
#include <memory>

namespace InsanityEngine::DX11
{
    class Renderer;

    template<class T>
    class Handle
    {
    };

    template<class T>
    struct ManagedHandleDeleter
    {
        Renderer* renderer = nullptr;

        ManagedHandleDeleter() = default;
        ManagedHandleDeleter(Renderer& renderer) :
            renderer(&renderer)
        {

        }

        void operator()(T* object) { renderer->Destroy(object); }
    };

    template<class T>
    class ManagedHandle : Handle<T>
    {
    private:
        std::unique_ptr<T, ManagedHandleDeleter<T>> m_object;

    public:
        ManagedHandle() = default;
        ManagedHandle(std::nullptr_t) {}
        ManagedHandle(Renderer& renderer, T& object) :
            m_object(&object, ManagedHandleDeleter<T>(renderer))
        {
        }
        ManagedHandle(const ManagedHandle& other) = delete;
        ManagedHandle(ManagedHandle&& other) noexcept = default;

        ~ManagedHandle() = default;

        ManagedHandle& operator=(const ManagedHandle& other) = delete;
        ManagedHandle& operator=(std::nullptr_t) { m_object = nullptr; }
        ManagedHandle& operator=(ManagedHandle&& other) noexcept = default;


        void swap(ManagedHandle& other)
        {
            m_object.swap(other);
        }

    protected:
        T& Object() const { return *m_object; }
    };
}