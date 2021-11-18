#pragma once
#include <utility>
#include <memory>

namespace InsanityEngine::DX11
{
    //class Renderer;


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
    class Handle
    {
    private:
        std::unique_ptr<ObjectT, ManagedHandleDeleter<ObjectT, RendererT>> m_object;

    public:
        Handle() = default;
        Handle(std::nullptr_t) {}
        Handle(RendererT& renderer, ObjectT& object) :
            m_object(&object, ManagedHandleDeleter<ObjectT, RendererT>(renderer))
        {
        }

        Handle& operator=(std::nullptr_t) { m_object = nullptr; }

        bool operator==(std::nullptr_t) const { return m_object == nullptr; }
        bool operator!=(std::nullptr_t) const { return !((*this) == nullptr); }

        operator bool() const { return ((*this) != nullptr); }

        void swap(Handle& other)
        {
            m_object.swap(other);
        }

    protected:
        ObjectT& Object() const { return *m_object; }
        RendererT& Renderer() const { return *m_object.get_deleter().renderer; }
    };
}