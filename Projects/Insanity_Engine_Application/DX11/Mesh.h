#pragma once
#include "CommonInclude.h"
#include "Insanity_Math.h"
#include <array>

namespace InsanityEngine::DX11
{
    class ResourceRegistry;
}

namespace InsanityEngine::DX11::StaticMesh
{
    using namespace Math::Types;

    struct VertexData
    {
        Vector3f position;
        Vector3f normal;
        Vector2f uv;
    };

    extern std::array<D3D11_INPUT_ELEMENT_DESC, 3> GetInputElementDescription();

    class BaseResource
    {
    private:
        std::string m_name;

    public:
        BaseResource(std::string name);

    public:
        std::string_view GetName() const { return m_name; }
        virtual const std::type_info& GetType() = 0;
    };

    template<class T>
    class Resource : public BaseResource
    {
    private:
        T m_resource;

    public:
        Resource(std::string name, T resource) :
            BaseResource(name),
            m_resource(resource)
        {

        }

    public:
        T* operator->() const { return &m_resource; }
        T& Get() { return m_resource; }
        const std::type_info& GetType() override { return typeid(T); }
    };

    class Mesh
    {
    private:
        ComPtr<ID3D11Buffer> m_vertexBuffer;
        ComPtr<ID3D11Buffer> m_indexBuffer;

        UINT m_vertexCount = 0;
        UINT m_indexCount = 0;

    public:
        Mesh(ComPtr<ID3D11Buffer> vertexBuffer, UINT vertexCount, ComPtr<ID3D11Buffer> indexBuffer, UINT indexCount);

    public:
        friend bool operator==(const Mesh& lh, const Mesh& rh) = default;
        friend bool operator!=(const Mesh& lh, const Mesh& rh) = default;

    public:
        ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
        ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }

        UINT GetVertexCount() const { return m_vertexCount; }
        UINT GetIndexCount() const { return m_indexCount; }
    };



    class Texture
    {
        ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
        ComPtr<ID3D11SamplerState> m_samplerState;

    public:
        Texture(ComPtr<ID3D11ShaderResourceView> shaderResourceView, ComPtr<ID3D11SamplerState> samplerState);

    public:
        friend bool operator==(const Texture& lh, const Texture& rh) = default;
        friend bool operator!=(const Texture& lh, const Texture& rh) = default;

    public:
        void SetSamplerState(ComPtr<ID3D11SamplerState> samplerState);

        ID3D11ShaderResourceView* GetView() const { return m_shaderResourceView.Get(); }
        ID3D11SamplerState* GetSamplerState() const { return m_samplerState.Get(); }

    };



    class Shader
    {
    private:
        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader> m_pixelShader;

    public:
        Shader(ComPtr<ID3D11VertexShader> vertexShader, ComPtr<ID3D11PixelShader> pixelShader);

    public:
        ID3D11VertexShader* GetVertexShader() const { return m_vertexShader.Get(); }
        ID3D11PixelShader* GetPixelShader() const { return m_pixelShader.Get(); }

    };



    class Material
    {
    private:
        std::shared_ptr<Resource<Shader>> m_shader;
        std::shared_ptr< Resource<Texture>> m_albedo;

    public:
        Vector4f color;

    public:
        Material(std::shared_ptr<Resource<Shader>> shader, std::shared_ptr<Resource<Texture>> albedo, Vector4f color = Vector4f(Scalar(1.f)));

    public:
        void SetShader(std::shared_ptr<Resource<Shader>> shader);
        void SetAlbedo(std::shared_ptr<Resource<Texture>> texture);

    public:
        std::shared_ptr< Resource<Shader>> GetShader() const { return m_shader; }
        std::shared_ptr< Resource<Texture>> GetAlbedo() const { return m_albedo; }


    public:
        friend bool operator==(const Material& lh, const Material& rh) = default;
        friend bool operator!=(const Material& lh, const Material& rh) = default;

    };


    class MeshObject
    {
    private:
        std::shared_ptr< Resource<Mesh>> m_mesh;
        std::shared_ptr< Resource<Material>> m_material;

    public:
        Vector3f position;
        Quaternion<float> quat;

    public:
        MeshObject(std::shared_ptr<Resource<Mesh>> mesh, std::shared_ptr<Resource<Material>> material);

    public:
        void SetMesh(std::shared_ptr<Resource<Mesh>> mesh);
        void SetMaterial(std::shared_ptr<Resource<Material>> material);

    public:
        std::shared_ptr<Resource<Mesh>> GetMesh() const { return m_mesh; }
        std::shared_ptr<Resource<Material>> GetMaterial() const { return m_material; }

    public:
        Matrix4x4f GetObjectMatrix() const;
    };


    template<class T>
    using SharedResource = std::shared_ptr<Resource<T>>;

    using SharedBaseResource = std::shared_ptr<BaseResource>;

    enum class RegisterStatus
    {
        Succeeded,
        Name_Conflict,
    };

    class ResourceManager
    {
    private:
        std::unordered_map<std::string, SharedBaseResource> m_resources;

    public:
        SharedResource<Mesh> CreateMesh(std::string_view name, Mesh mesh);
        SharedResource<Material> CreateMaterial(std::string_view name, Material material);
        SharedResource<Shader> CreateShader(std::string_view name, Shader shader);
        SharedResource<Texture> CreateTexture(std::string_view name, Texture texture);

        void RegisterMesh(SharedResource<Mesh> mesh);
        void RegisterMaterial(SharedResource<Material> material);
        void RegisterShader(SharedResource<Shader> shader);
        void RegisterTexture(SharedResource<Texture> texture);

        void DestroyResource(std::string_view name);
        void DestroyResource(SharedBaseResource resource);

        SharedResource<Material> GetMaterial(std::string_view name);
        SharedResource<Shader> GetShader(std::string_view name);
        SharedResource<Texture> GetTexture(std::string_view name);
        SharedResource<Mesh> GetMesh(std::string_view name);

    private:
        template<class T>
        SharedResource<T> CreateResource(std::string_view name, T resource)
        {
            std::string nameCopy{ name };

            if(m_resources.contains(nameCopy))
                return nullptr;

            SharedResource<T> sharedResource = std::make_shared<Resource<T>>(nameCopy, resource);
            m_resources[nameCopy] = sharedResource;

            return sharedResource;
        }

        template<class T>
        void RegisterResource(SharedResource<T> resource)
        {
            std::string nameCopy{ resource->GetName() };

            if(m_resources.contains(nameCopy))
                return;

            m_resources[nameCopy] = resource;
        }

        template<class T>
        std::shared_ptr<Resource<T>> Get(std::string_view name)
        {
            auto it = m_resources.find(std::string(name));

            if(it == m_resources.end())
                return nullptr;

            if(it->second->GetType() != typeid(T))
                return nullptr;

            return std::static_pointer_cast<Resource<T>>(it->second);
        }
    };
}