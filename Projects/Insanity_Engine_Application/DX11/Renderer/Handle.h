#pragma once
#include <utility>
#include <memory>

namespace InsanityEngine::DX11
{
    //class Renderer;

    template<class T>
    class Handle
    {
    };

    template<class ObjectT, class RendererT>
    struct ManagedHandleDeleter
    {
        RendererT* renderer = nullptr;

        ManagedHandleDeleter() = default;
        ManagedHandleDeleter(RendererT& renderer) :
            renderer(&renderer)
        {

        }

        void operator()(ObjectT* object) { renderer->Destroy(object); }
    };

    template<class ObjectT, class RendererT>
    class ManagedHandle : Handle<ObjectT>
    {
    private:
        std::unique_ptr<ObjectT, ManagedHandleDeleter<ObjectT, RendererT>> m_object;

    public:
        ManagedHandle() = default;
        ManagedHandle(std::nullptr_t) {}
        ManagedHandle(RendererT& renderer, ObjectT& object) :
            m_object(&object, ManagedHandleDeleter<ObjectT, RendererT>(renderer))
        {
        }

        ManagedHandle& operator=(std::nullptr_t) { m_object = nullptr; }

        bool operator==(std::nullptr_t) const { return m_object == nullptr; }
        bool operator!=(std::nullptr_t) const { return !((*this) == nullptr); }

        operator bool() const { return ((*this) != nullptr); }

        void swap(ManagedHandle& other)
        {
            m_object.swap(other);
        }

    protected:
        ObjectT& Object() const { return *m_object; }
    };
}