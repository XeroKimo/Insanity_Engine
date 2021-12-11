#pragma once
#include "../CommonInclude.h"
#include "Wrappers/ComponentWrapper.h"
#include <memory>

namespace InsanityEngine::DX11
{


    template<class ComponentTy, class RendererTy>
    struct RendererComponentDeleter
    {
        RendererTy* renderer = nullptr;


        void operator()(Component<ComponentTy>* object) { renderer->Destroy(object); }
    };


    template<class ComponentTy, class RendererTy>
    class RendererComponentHandle
    {
    private:
        std::unique_ptr<Component<ComponentTy>, RendererComponentDeleter<ComponentTy, RendererTy>> m_component;

    public:
        RendererComponentHandle() = default;
        RendererComponentHandle(std::nullptr_t) {};
        RendererComponentHandle(Component<ComponentTy>* component, RendererTy* renderer) :
            m_component(component, RendererComponentDeleter<ComponentTy, RendererTy>{ renderer })
        {

        }

        RendererComponentHandle(const RendererComponentHandle& other) = delete;
        RendererComponentHandle(RendererComponentHandle&& other) noexcept = default;

        RendererComponentHandle& operator=(std::nullptr_t) { m_component = nullptr; }
        RendererComponentHandle& operator=(const RendererComponentHandle& other) = delete;
        RendererComponentHandle& operator=(RendererComponentHandle&& other) noexcept = default;

        bool operator==(std::nullptr_t) const { return m_component == nullptr; }
        bool operator!=(std::nullptr_t) const { return m_component != nullptr; }


    protected:
        Component<ComponentTy>& GetComponent() { return *m_component; }
        const Component<ComponentTy>& GetComponent() const { return *m_component; }

        RendererTy& GetRenderer() { return *m_component.get_deleter().renderer; }
        const RendererTy& GetRenderer() const { return *m_component.get_deleter().renderer; }
    };

    template<class ComponentTy, class HandleTy = ComponentTy>
    class RendererComponentInterface;
}