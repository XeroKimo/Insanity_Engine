#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <utility>
#include <optional>
#include <tuple>
#include <filesystem>
#include <wrl/client.h>

module InsanityEngine.Renderer;
import TypedD3D11;
import xk.Math;
import InsanityEngine.Container.StableVector;

namespace InsanityEngine::Renderer
{
	struct SpritePipeline
	{
		TypedD3D11::Wrapper<ID3D11Buffer> cameraBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> vertexBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> instanceBuffer;
		TypedD3D11::Wrapper<ID3D11RasterizerState> rasterizerState;

		TypedD3D11::Wrapper<ID3D11InputLayout> layout;
		TypedD3D11::Wrapper<ID3D11VertexShader> vertexShader;
		TypedD3D11::Wrapper<ID3D11PixelShader> pixelShader;
		TypedD3D11::Wrapper<ID3D11ShaderResourceView> defaultTexture;
		TypedD3D11::Wrapper<ID3D11DepthStencilState> depthState;
		TypedD3D11::Wrapper<ID3D11SamplerState> pointSampler;
		TypedD3D11::Wrapper<ID3D11BlendState> blendState;

	public:
		static constexpr UINT VSPerFrameCBufferSlot = 0;
		static constexpr UINT VSPerCameraCBufferSlot = 1;
		static constexpr UINT VSPerMaterialCBufferSlot = 2;
		static constexpr UINT VSPerObjectCBufferSlot = 3;
	};

	struct Sprite
	{
		StableVector<xk::Math::Matrix<float, 4, 4>> transforms;
		StableVector<TypedD3D11::Wrapper<ID3D11ShaderResourceView>> texture;
	};

	void DrawSprites();

	struct Vertex
	{
		xk::Math::Vector<float, 3> pos;
		xk::Math::Vector<float, 2> uv;
	};
	static TypedD3D::Wrapper<ID3D11Device> device;
	static TypedD3D::Wrapper<ID3D11DeviceContext> deviceContext;
	static Microsoft::WRL::ComPtr<ID3D11Debug> debugDevice;
	static SpritePipeline spritePipeline;
	static Sprite sprites;

	std::filesystem::path relativeEngineAssetPath = std::filesystem::path{ "../Insanity_Engine/Insanity_Framework/" };


	Camera::Camera(xk::Math::Vector<float, 3> position, xk::Math::Degree<float> angle, xk::Math::Matrix<float, 4, 4> perspective) :
		viewPerspectiveTransform{ perspective * xk::Math::TransformMatrix(-position) * xk::Math::RotationZMatrix(angle) }
	{

	}

	Lifetime::~Lifetime()
	{
		if (engaged)
			Shutdown();
	}

	Lifetime Initialize(bool enableDebug)
	{
		std::tie(device, deviceContext) = TypedD3D11::CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, enableDebug ? D3D11_CREATE_DEVICE_DEBUG : 0, D3D_FEATURE_LEVEL_11_1, D3D11_SDK_VERSION);
		if (enableDebug)
			debugDevice = TypedD3D::Cast<ID3D11Debug>(device.AsComPtr());
		
		auto CompileShader = [](std::filesystem::path path, LPCSTR target)
		{
			Microsoft::WRL::ComPtr<ID3DBlob> blob;
			TypedD3D::ThrowIfFailed(D3DCompileFromFile((relativeEngineAssetPath / path).c_str(), nullptr, nullptr, "main", target, 0, 0, &blob, nullptr));
			return blob;
		};

		{
			auto blob = CompileShader("Assets/Engine/SpriteVS.hlsl", "vs_5_0");
			spritePipeline.vertexShader = GetDevice()->CreateVertexShader(*blob.Get(), nullptr);
			std::array inputElement
			{
				D3D11_INPUT_ELEMENT_DESC{
					.SemanticName = "POSITION",
					.SemanticIndex = 0,
					.Format = DXGI_FORMAT_R32G32B32_FLOAT,
					.InputSlot = 0,
					.AlignedByteOffset = 0,
					.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
					.InstanceDataStepRate = 0,
				},
				D3D11_INPUT_ELEMENT_DESC{
					.SemanticName = "TEXCOORD",
					.SemanticIndex = 0,
					.Format = DXGI_FORMAT_R32G32_FLOAT,
					.InputSlot = 0,
					.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
					.InstanceDataStepRate = 0,
				},
				D3D11_INPUT_ELEMENT_DESC{
					.SemanticName = "OBJTRANSFORM",
					.SemanticIndex = 0,
					.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
					.InputSlot = 1,
					.AlignedByteOffset = 0,
					.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
					.InstanceDataStepRate = 1,
				},
				D3D11_INPUT_ELEMENT_DESC{
					.SemanticName = "ObjTransform",
					.SemanticIndex = 1,
					.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
					.InputSlot = 1,
					.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
					.InstanceDataStepRate = 1,
				},
				D3D11_INPUT_ELEMENT_DESC{
					.SemanticName = "ObjTransform",
					.SemanticIndex = 2,
					.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
					.InputSlot = 1,
					.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
					.InstanceDataStepRate = 1,
				},
				D3D11_INPUT_ELEMENT_DESC{
					.SemanticName = "ObjTransform",
					.SemanticIndex = 3,
					.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
					.InputSlot = 1,
					.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA,
					.InstanceDataStepRate = 1,
				},
			};

			spritePipeline.layout = GetDevice()->CreateInputLayout(inputElement, *blob.Get());
		}
		spritePipeline.pixelShader = GetDevice()->CreatePixelShader(*CompileShader("Assets/Engine/SpritePS.hlsl", "ps_5_0").Get(), nullptr);

		{
			constexpr Vertex bl{ { -0.5f, -0.5f}, { 0, 1 - 0 } };
			constexpr Vertex tl{ { -0.5f, 0.5f}, { 0, 1 - 1 } };
			constexpr Vertex tr{ { 0.5f, 0.5f}, { 1, 1 - 1 } };
			constexpr Vertex br{ { 0.5f, -0.5f}, { 1, 1 - 0 } };

			constexpr std::array<Vertex, 6> vertexData
			{
				bl, tl, tr,
				tr, br, bl
			};

			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(vertexData),
				.Usage = D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11_BIND_VERTEX_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};


			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = vertexData.data();
			spritePipeline.vertexBuffer = GetDevice()->CreateBuffer(bufferDesc, &data);
		}
		{
			static constexpr UINT instanceBufferElementMaxCount = 256;
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(xk::Math::Aliases::Matrix4x4) * instanceBufferElementMaxCount,
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_VERTEX_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};

			spritePipeline.instanceBuffer = GetDevice()->CreateBuffer(bufferDesc, nullptr);
		}

		{
			D3D11_RASTERIZER_DESC desc
			{
				.FillMode = D3D11_FILL_SOLID,
				.CullMode = D3D11_CULL_BACK,
				.FrontCounterClockwise = false,
				.DepthBias = 0,
				.DepthBiasClamp = 0,
				.SlopeScaledDepthBias = 0,
				.DepthClipEnable = true,
				.ScissorEnable = true,
				.MultisampleEnable = false,
				.AntialiasedLineEnable = true
			};
			spritePipeline.rasterizerState = GetDevice()->CreateRasterizerState(desc);
		}

		{
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(xk::Math::Aliases::Matrix4x4),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};
			spritePipeline.cameraBuffer = GetDevice()->CreateBuffer(bufferDesc);
		}

		{
			D3D11_TEXTURE2D_DESC bufferDesc
			{
				.Width = 1,
				.Height = 1,
				.MipLevels = 0,
				.ArraySize = 1,
				.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc = { 1, 0 },
				.Usage = D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11_BIND_SHADER_RESOURCE,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
			};

			xk::Math::Color color{ { 255, 255, 255, 255 } };
			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = &color;
			data.SysMemPitch = 4;
			auto tempBuffer = GetDevice()->CreateTexture2D(bufferDesc, &data);

			spritePipeline.defaultTexture = GetDevice()->CreateShaderResourceView(tempBuffer);
		}
		{
			D3D11_DEPTH_STENCIL_DESC desc
			{
				.DepthEnable = true,
				.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D11_COMPARISON_LESS,
				.StencilEnable = false,
				.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
				.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
				.FrontFace = {},
				.BackFace = {}
			};
			spritePipeline.depthState = GetDevice()->CreateDepthStencilState(desc);
		}
		{
			D3D11_BLEND_DESC desc
			{
				.AlphaToCoverageEnable = false,
				.IndependentBlendEnable = false,
				.RenderTarget = {
					{
						.BlendEnable = true,
						.SrcBlend = D3D11_BLEND_SRC_ALPHA,
						.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
						.BlendOp = D3D11_BLEND_OP_ADD,
						.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
						.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
						.BlendOpAlpha = D3D11_BLEND_OP_ADD,
						.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
					}}
			};
			spritePipeline.blendState = GetDevice()->CreateBlendState(desc);
		}
		{
			D3D11_SAMPLER_DESC desc
			{
				.Filter = D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
				.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
				.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
				.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
				.MipLODBias = 0,
				.MaxAnisotropy = 16,
				.ComparisonFunc = D3D11_COMPARISON_NEVER,
				.BorderColor = { 0, 0, 0, 1 },
				.MinLOD = std::numeric_limits<float>::lowest(),
				.MaxLOD = (std::numeric_limits<float>::max)()
			};
			spritePipeline.pointSampler = GetDevice()->CreateSamplerState(desc);
		}

		return { true };
	}

	void Shutdown()
	{
		auto tempDevice = std::move(device);
		auto tempDeviceContext = std::move(deviceContext);
		auto tempDebugDevice = std::move(debugDevice);

		{
			auto tempSpritePipeline = std::move(spritePipeline);
			auto tempSprites = std::move(sprites);
		}

		if(tempDebugDevice)
			tempDebugDevice->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_SUMMARY | D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
	}

	template<class Func>
	void UpdateConstantBuffer(TypedD3D::Wrapper<ID3D11Resource> resource, Func func)
	{
		D3D11_MAPPED_SUBRESOURCE data = GetDeviceContext()->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0);
		func(data);
		GetDeviceContext()->Unmap(resource, 0);
	}

	void DrawScene(TypedD3D::Wrapper<ID3D11RenderTargetView> target, const Camera& camera)
	{
		GetDeviceContext()->OMSetRenderTargets(target, nullptr);
		D3D11_TEXTURE2D_DESC desc = TypedD3D::Cast<ID3D11Texture2D>(target->GetResource())->GetDesc();
		D3D11_VIEWPORT viewports;
		viewports.TopLeftX = 0;
		viewports.TopLeftY = 0;
		viewports.MinDepth = 0;
		viewports.MaxDepth = 1;
		viewports.Width = static_cast<FLOAT>(desc.Width);
		viewports.Height = static_cast<FLOAT>(desc.Height);
		deviceContext->RSSetViewports(viewports);
		UpdateConstantBuffer(spritePipeline.cameraBuffer, [&camera](D3D11_MAPPED_SUBRESOURCE data)
		{
			std::memcpy(data.pData, &camera.viewPerspectiveTransform, sizeof(camera.viewPerspectiveTransform));
		});
		deviceContext->VSSetConstantBuffers(SpritePipeline::VSPerCameraCBufferSlot, spritePipeline.cameraBuffer);

		DrawSprites();
	}

	void DrawSprites()
	{
		for (std::size_t i = 0; i < sprites.transforms.Size() - 1; i++)
		{
			for (std::size_t j = i + 1; j < sprites.transforms.Size(); j++)
			{
				if (sprites.transforms.At(i).At(2, 3) < sprites.transforms.At(j).At(2, 3))
				{
					sprites.transforms.SwapElements(i, j);
					sprites.texture.SwapElements(i, j);
				}
			}
		}
		GetDeviceContext()->IASetInputLayout(spritePipeline.layout);
		GetDeviceContext()->VSSetShader(spritePipeline.vertexShader, {});
		GetDeviceContext()->PSSetShader(spritePipeline.pixelShader, {});
		GetDeviceContext()->PSSetSamplers(0, spritePipeline.pointSampler);
		GetDeviceContext()->OMSetBlendState(spritePipeline.blendState, std::nullopt, D3D11_DEFAULT_SAMPLE_MASK);
		GetDeviceContext()->OMSetDepthStencilState(spritePipeline.depthState, D3D11_DEFAULT_SAMPLE_MASK);
		GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (std::size_t i = 0; i < sprites.transforms.Size(); i++)
		{
			UpdateConstantBuffer(spritePipeline.instanceBuffer, [=](D3D11_MAPPED_SUBRESOURCE data)
			{
				std::memcpy(data.pData, &sprites.transforms.At(i), sizeof(xk::Math::Matrix<float, 4, 4>));
			});
			TypedD3D::Array<TypedD3D::Wrapper<ID3D11Buffer>, 2> buffers{ spritePipeline.vertexBuffer, spritePipeline.instanceBuffer };
			std::array<UINT, 2> strides{ sizeof(Vertex), sizeof(xk::Math::Aliases::Matrix4x4) };
			std::array<UINT, 2> offsets{ 0, 0 };
			deviceContext->IASetVertexBuffers(0, { TypedD3D::Span{buffers}, std::span{strides}, std::span{offsets} });
			deviceContext->PSSetShaderResources(0, sprites.texture.At(i));
			deviceContext->DrawInstanced(6, 1, 0, 0);
		}
	}

	TypedD3D::Wrapper<ID3D11Device> GetDevice()
	{
		return device;
	}

	TypedD3D::Wrapper<ID3D11DeviceContext> GetDeviceContext()
	{
		return deviceContext;
	}

	constexpr float priorityBias = 500;

	UniqueSpriteHandle NewSprite(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture)
	{
		GenerationHandle handle = sprites.texture.PushBack(texture ? texture : spritePipeline.defaultTexture);
		sprites.transforms.PushBack({});

		sprites.transforms.Back().At(2, 3) = priorityBias;
		return SpriteHandle{ handle };
	}


	UniqueSpriteHandle::~UniqueSpriteHandle()
	{
		sprites.transforms.EraseHandle(handle.Get());
		sprites.texture.EraseHandle(handle.Get());
	}

	void SpriteHandle::SetTransform(const xk::Math::Matrix<float, 4, 4>& transform)
	{
		auto priority = sprites.transforms.HandleAt(handle)->At(2, 3);
		*sprites.transforms.HandleAt(handle) = transform;
		sprites.transforms.HandleAt(handle)->At(2, 3) = priority;
	}
	void SpriteHandle::SetTransform(xk::Math::Vector<float, 2> position, xk::Math::Degree<float> rotation, xk::Math::Vector<float, 2> scale)
	{
		SetTransform(xk::Math::RotationZMatrix(rotation) * xk::Math::TransformMatrix<float>(position) * xk::Math::ScaleMatrix<float>({ scale, 1 }));
	}
	void SpriteHandle::SetTexture(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture)
	{
		*sprites.texture.HandleAt(handle) = texture ? texture : spritePipeline.defaultTexture;
	}
	void SpriteHandle::SetPriority(int priority)
	{
		sprites.transforms.HandleAt(handle)->At(2, 3) = priorityBias - static_cast<float>(priority);
	}

	TypedD3D11::Wrapper<ID3D11ShaderResourceView> SpriteHandle::GetTexture() const
	{
		return *sprites.texture.HandleAt(handle);
	}
}