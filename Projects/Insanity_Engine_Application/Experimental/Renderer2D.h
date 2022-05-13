#pragma once
#include "../Rendering/d3d12/Backend.h"
#include <TypedD3D.h>
#include <gsl/gsl>

namespace InsanityEngine::Experimental::Rendering
{
    using namespace InsanityEngine::Rendering;

    class Texture
    {

    };

    class Sprite
    {
        friend class SpriteRenderer;
        
    private:
        D3D12_GPU_VIRTUAL_ADDRESS m_objectConstantBuffer;
        Texture* m_texture;

    public:
        Math::Types::Vector3f position;
        Math::Types::Vector3f scale = Math::Types::Scalar(1);
    };

    class SpriteRenderer
    {
        template<class Ty>
        friend struct Deleter;

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_spriteRootSignature;
        TypedD3D::D3D12::PipelineState::Graphics m_spritePipeline;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_spriteMesh;
        TypedD3D::D3D12::DescriptorHeap::Sampler m_spriteSampler;

        std::vector<std::unique_ptr<Sprite>> m_sprites;

    private:
        template<class Ty>
        struct Deleter
        {
            SpriteRenderer* renderer;

            void operator()(Ty* obj) const { renderer->Destroy(obj); }
        };

    public:
        template<class Ty>
        class Handle : private std::unique_ptr<Ty, Deleter<Ty>>
        {
        private:
            using Base = std::unique_ptr<Ty, Deleter<Ty>>;

        public:
            using Base::Base;
        };

        class HandleSprite : private std::unique_ptr<Sprite, Deleter<Sprite>>
        {
        private:
            using Base = std::unique_ptr<Sprite, Deleter<Sprite>>;

        public:
            using Base::unique_ptr;
            using Base::element_type;
            using Base::deleter_type;
            using Base::pointer;
            using Base::get;
            using Base::operator bool;
            using Base::operator*;
            using Base::operator->;
            using Base::operator=;
            using Base::release;
            using Base::reset;
            using Base::swap;
        };

    public:
        Handle<Sprite> Create(Texture& texture, Math::Types::Vector3f position = {}, Math::Types::Vector3f scale = Math::Types::Scalar(1))
        {
            m_sprites.push_back(std::make_unique<Sprite>());
            m_sprites.back()->m_texture = &texture;
            m_sprites.back()->position = position;
            m_sprites.back()->scale = scale;

            return Handle<Sprite>(m_sprites.back().get(), Deleter<Sprite>{ .renderer = this });
        }

    private:
        void Destroy(Sprite* sprite)
        {
            m_sprites.erase(std::find_if(m_sprites.begin(), m_sprites.end(), [=](const std::unique_ptr<Sprite>& comp) { return comp.get() == sprite; }));
        }

    public:

    };

    class Renderer2D
    {
        gsl::strict_not_null<D3D12::Backend*> m_backend;
        TypedD3D::D3D12::CommandList::Direct5 m_commandList;
    };
}