#include <dxgi1_6.h>
#include <d3d11.h>
#include <utility>
#include <tuple>
#include <d3d11sdklayers.h>
#include <array>
#include <span>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <cmath>
#include <filesystem>

#include <SDL2/SDL_image.h>
#include <numbers>
#include <cassert>
module InsanityEngine.RendererDX11;

using namespace TypedD3D;

namespace InsanityEngine
{
	TypedD3D11::Wrapper<ID3D11ShaderResourceView> CreateTexture(std::filesystem::path path, TypedD3D11::Wrapper<ID3D11Device> device)
	{
		SDL_Surface* surface = IMG_Load(path.string().c_str());
		D3D11_TEXTURE2D_DESC textDesc
		{
			.Width = static_cast<UINT>(surface->w),
			.Height = static_cast<UINT>(surface->h),
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = { 1, 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};

		if(SDL_PIXELTYPE(surface->format->format) == SDL_PIXELTYPE_INDEX8)
		{
			std::vector<char> pixels;
			pixels.resize(surface->pitch * surface->h * 4);
			char* refPixels = static_cast<char*>(surface->pixels);
			surface->format->palette->colors[0].a = 0;

			for(int i = 0; i < pixels.size(); i += 4)
			{
				int paletteIndex = refPixels[i / 4];
				int x = (i / 4) % surface->w;
				int y = (i / 4) / surface->h;
				assert(paletteIndex < surface->format->palette->ncolors);
				SDL_Color color = surface->format->palette->colors[paletteIndex];
				pixels[i] = color.r;
				pixels[i + 1] = color.g;
				pixels[i + 2] = color.b;
				pixels[i + 3] = color.a;
			}

			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = pixels.data();
			data.SysMemPitch = surface->pitch * 4;
			TypedD3D11::Wrapper<ID3D11Texture2D> buffer = device->CreateTexture2D(textDesc, &data);
			D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
				.Texture2D = { 0, 1 }
			};

			return device->CreateShaderResourceView(buffer, &viewDesc);
		}
		else
		{
			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = surface->pixels;
			data.SysMemPitch = surface->pitch;
			TypedD3D11::Wrapper<ID3D11Texture2D> buffer = device->CreateTexture2D(textDesc, &data);
			D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
				.Texture2D = { 0, 1 }
			};

			return device->CreateShaderResourceView(buffer, &viewDesc);
		}
	}

	RendererDX11::RendererDX11(HWND handle)
	{
		TypedDXGI::Wrapper<IDXGIFactory2> factory = TypedDXGI::CreateFactory1<IDXGIFactory2>();
		std::tie(m_device, m_deviceContext) = TypedD3D11::CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, D3D_FEATURE_LEVEL_11_0, D3D11_SDK_VERSION);
		m_swapChain = factory->CreateSwapChainForHwnd<IDXGISwapChain1>(
			m_device,
			handle,
			DXGI_SWAP_CHAIN_DESC1
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc
				{
					.Count = 1
				},
				.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = 2,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
				.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
			},
			nullptr,
			nullptr);

		m_debugDevice = TypedD3D::Cast<ID3D11Debug>(m_device.AsComPtr());
		m_backBuffer = m_device->CreateRenderTargetView(m_swapChain->GetBuffer<ID3D11Resource>(0));

		auto desc = TypedD3D::Cast<ID3D11Texture2D>(m_swapChain->GetBuffer<ID3D11Resource>(0))->GetDesc();
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		m_backDepthBuffer = m_device->CreateDepthStencilView(m_device->CreateTexture2D(desc));
	}

	RendererDX11::~RendererDX11()
	{
		if(m_debugDevice)
			m_debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
	}

	Camera::Camera(xk::Math::Vector<float, 3> position, xk::Math::Degree<float> angle, xk::Math::Matrix<float, 4, 4> perspective) :
		viewPerspectiveTransform
	{
		perspective * xk::Math::Matrix<float, 4, 4> {
			1, 0, 0, -position.X(),
			0, 1, 0, -position.Y(),
			0, 0, 1, -position.Z(),
			0, 0, 0, 1
		} *xk::Math::Matrix<float, 4, 4>
		{
			std::cos(-xk::Math::Radian<float>(angle)._value), -std::sin(-xk::Math::Radian<float>(angle)._value), 0, 0,
			std::sin(-xk::Math::Radian<float>(angle)._value), std::cos(-xk::Math::Radian<float>(angle)._value), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		}
	}
	{
	}

	struct Vertex
	{
		xk::Math::Vector<float, 3> pos;
		xk::Math::Vector<float, 2> uv;
	};

	static constexpr UINT instanceBufferElementMaxCount = 256;

	void SpriteRenderInterfaceDX11::Draw(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, xk::Math::Aliases::Matrix4x4 transform)
	{
		UpdateConstantBuffer(m_renderer.GetDeviceContext(), m_spriteRenderer.instanceBuffer, [&transform](D3D11_MAPPED_SUBRESOURCE data)
		{
			std::memcpy(data.pData, &transform, sizeof(transform));
		});
		TypedD3D::Array<TypedD3D::Wrapper<ID3D11Buffer>, 2> buffers{ m_spriteRenderer.vertexBuffer, m_spriteRenderer.instanceBuffer };
		std::array<UINT, 2> strides{ sizeof(Vertex), sizeof(xk::Math::Aliases::Matrix4x4) };
		std::array<UINT, 2> offsets{ 0, 0 };
		m_renderer.GetDeviceContext()->IASetVertexBuffers(0, { TypedD3D::Span{buffers}, std::span{strides}, std::span{offsets} });
		m_renderer.GetDeviceContext()->PSSetShaderResources(0, texture ? texture : m_spriteRenderer.defaultTexture);
		m_renderer.GetDeviceContext()->DrawInstanced(6, 1, 0, 0);
	}

	void SpriteRenderInterfaceDX11::DrawMultiple(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, std::span<xk::Math::Aliases::Matrix4x4> transform)
	{
		m_renderer.GetDeviceContext()->PSSetShaderResources(0, texture ? texture : m_spriteRenderer.defaultTexture);

		for(size_t i = 0; i < transform.size();)
		{
			UINT amountToDraw = transform.size() > instanceBufferElementMaxCount ? instanceBufferElementMaxCount : static_cast<UINT>(transform.size());
			UpdateConstantBuffer(m_renderer.GetDeviceContext(), m_spriteRenderer.instanceBuffer, [&transform, i, amountToDraw](D3D11_MAPPED_SUBRESOURCE data)
			{
				std::memcpy(data.pData, &transform[i], sizeof(xk::Math::Aliases::Matrix4x4) * amountToDraw);
			});

			TypedD3D::Array<TypedD3D::Wrapper<ID3D11Buffer>, 2> buffers{ m_spriteRenderer.vertexBuffer, m_spriteRenderer.instanceBuffer };
			std::array<UINT, 2> strides{ sizeof(Vertex), sizeof(xk::Math::Aliases::Matrix4x4) };
			std::array<UINT, 2> offsets{ 0, 0 };
			m_renderer.GetDeviceContext()->IASetVertexBuffers(0, { TypedD3D::Span{buffers}, std::span{strides}, std::span{offsets} });

			m_renderer.GetDeviceContext()->DrawInstanced(6, amountToDraw, 0, 0);
			i += amountToDraw;
		}
	}

	SpritePipelineDX11::SpritePipelineDX11(TypedD3D11::Wrapper<ID3D11Device> device, TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext)
	{
		struct TemporaryWorkingPath
		{
			std::filesystem::path oldPath = std::filesystem::current_path();
			TemporaryWorkingPath(std::filesystem::path path)
			{
				std::filesystem::current_path(path);
			}
			~TemporaryWorkingPath()
			{
				std::filesystem::current_path(oldPath);
			}
		};
		//TODO: Figure out how to not hardcode this
		auto relative = TemporaryWorkingPath{ std::filesystem::path{ "../Insanity_Engine/Projects/Insanity_Engine/" } };
		Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(std::filesystem::path{ "Assets/Engine/SpriteVS.hlsl" }.c_str(), nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBlob, nullptr));
		vertexShader = device->CreateVertexShader(*vertexBlob.Get(), nullptr);
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

		layout = device->CreateInputLayout(inputElement, *vertexBlob.Get());

		Microsoft::WRL::ComPtr<ID3DBlob> pixelBlob;
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(std::filesystem::path{ "Assets/Engine/SpritePS.hlsl" }.c_str(), nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelBlob, nullptr));
		pixelShader = device->CreatePixelShader(*pixelBlob.Get(), nullptr);

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
			vertexBuffer = device->CreateBuffer(bufferDesc, &data);
		}
		{
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(xk::Math::Aliases::Matrix4x4) * instanceBufferElementMaxCount,
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_VERTEX_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};

			instanceBuffer = device->CreateBuffer(bufferDesc, nullptr);
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
			rasterizerState = device->CreateRasterizerState(desc);
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
			cameraBuffer = device->CreateBuffer(bufferDesc);
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
			auto tempBuffer = device->CreateTexture2D(bufferDesc, &data);

			defaultTexture = device->CreateShaderResourceView(tempBuffer);
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
			depthState = device->CreateDepthStencilState(desc);
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
			blendState = device->CreateBlendState(desc);
		}
	}

	void SpritePipelineDX11::Bind(RendererDX11& renderer)
	{
		renderer.GetDeviceContext()->IASetInputLayout(layout);
		renderer.GetDeviceContext()->VSSetShader(vertexShader, {});
		renderer.GetDeviceContext()->PSSetShader(pixelShader, {});
		renderer.GetDeviceContext()->OMSetBlendState(blendState, std::nullopt, 0xff'ff'ff'ff);
		renderer.GetDeviceContext()->OMSetDepthStencilState(depthState, 0xff'ff'ff'ff);
		D3D11_TEXTURE2D_DESC desc = TypedD3D::Cast<ID3D11Texture2D>(renderer.GetSwapChainBackBuffer()->GetResource())->GetDesc();
		D3D11_VIEWPORT viewports;
		viewports.TopLeftX = 0;
		viewports.TopLeftY = 0;
		viewports.MinDepth = 0;
		viewports.MaxDepth = 1;
		viewports.Width = static_cast<FLOAT>(desc.Width);
		viewports.Height = static_cast<FLOAT>(desc.Height);
		renderer.GetDeviceContext()->RSSetViewports(viewports);
		renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}


	void DebugRenderInterfaceDX11::DrawLine(xk::Math::Vector<float, 3> start, xk::Math::Vector<float, 3> end)
	{
		DrawLine(std::array{ start, end });
	}

	void DebugRenderInterfaceDX11::DrawLine(std::span<xk::Math::Vector<float, 3>> points)
	{
		for(size_t i = 0; i < points.size();)
		{
			size_t amountToDraw = (std::min)(points.size() - i, DebugPipelineDX11::maxPointsPerBatch);
			UpdateConstantBuffer(m_renderer.GetDeviceContext(), m_debugRenderer.vertexBuffer, [&](D3D11_MAPPED_SUBRESOURCE data)
			{
				std::memcpy(data.pData, &points[i], sizeof(xk::Math::Vector<float, 3>) * amountToDraw);
			});
			i += amountToDraw;
			m_renderer.GetDeviceContext()->IASetVertexBuffers(0, m_debugRenderer.vertexBuffer, sizeof(xk::Math::Vector<float, 3>), 0);
			m_renderer.GetDeviceContext()->Draw(amountToDraw, 0);
		}
	}

	void DebugRenderInterfaceDX11::DrawConnectedLines(std::span<xk::Math::Vector<float, 3>> points)
	{
		std::vector<xk::Math::Vector<float, 3>> connectedPoints;
		connectedPoints.resize(points.size() * 2);

		for(size_t i = 0; i < connectedPoints.size(); i += 2)
		{
			connectedPoints[i] = points[i / 2];
			connectedPoints[(i + connectedPoints.size() - 1) % connectedPoints.size()] = connectedPoints[i];
		}

		DrawLine(connectedPoints);
	}

	void DebugRenderInterfaceDX11::DrawSquare(xk::Math::Vector<float, 3> center, xk::Math::Vector<float, 3> halfSize)
	{
		const xk::Math::Vector<float, 3> bl = center - halfSize;
		const xk::Math::Vector<float, 3> tr = center + halfSize;
		const xk::Math::Vector<float, 3> tl{ bl.X(), tr.Y() };
		const xk::Math::Vector<float, 3> br{ tr.X(), bl.Y() };

		//DrawLine(std::array{ bl, tl, tl, tr, tr, br, br, bl });
		DrawConnectedLines(std::array{ bl, tl, tr, br });
	}

	void DebugRenderInterfaceDX11::DrawCircle(xk::Math::Vector<float, 3> center, float radius)
	{
		static constexpr size_t pointResolution = 64;
		std::array<xk::Math::Vector<float, 3>, pointResolution> points;

		float angleIncrements = std::numbers::pi * 2 / (pointResolution);
		for(size_t i = 0; i < points.size(); i++)
		{
			points[i] = center + xk::Math::Vector<float, 3>{ std::cos(angleIncrements* i), std::sin(angleIncrements* i), 0 } *radius;
			//points[(i + points.size() - 1) % points.size()] = points[i];
		}

		DrawConnectedLines(points);
	}

	void DebugRenderInterfaceDX11::SetColor(xk::Math::Vector<float, 4> rgba)
	{
		UpdateConstantBuffer(m_renderer.GetDeviceContext(), m_debugRenderer.batchBuffer, [rgba](D3D11_MAPPED_SUBRESOURCE data)
		{
			std::memcpy(data.pData, &rgba, sizeof(rgba));
		});
		m_renderer.GetDeviceContext()->PSSetConstantBuffers(0, m_debugRenderer.batchBuffer);
	}

	DebugPipelineDX11::DebugPipelineDX11(TypedD3D11::Wrapper<ID3D11Device> device, TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext)
	{
		struct TemporaryWorkingPath
		{
			std::filesystem::path oldPath = std::filesystem::current_path();
			TemporaryWorkingPath(std::filesystem::path path)
			{
				std::filesystem::current_path(path);
			}
			~TemporaryWorkingPath()
			{
				std::filesystem::current_path(oldPath);
			}
		};
		//TODO: Figure out how to not hardcode this
		auto relative = TemporaryWorkingPath{ std::filesystem::path{ "../Insanity_Engine/Projects/Insanity_Engine/" } };
		Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(std::filesystem::path{ "Assets/Engine/DebugVS.hlsl" }.c_str(), nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBlob, nullptr));
		vertexShader = device->CreateVertexShader(*vertexBlob.Get(), nullptr);
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
			}
		};

		layout = device->CreateInputLayout(inputElement, *vertexBlob.Get());

		Microsoft::WRL::ComPtr<ID3DBlob> pixelBlob;
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(std::filesystem::path{ "Assets/Engine/DebugPS.hlsl" }.c_str(), nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelBlob, nullptr));
		pixelShader = device->CreatePixelShader(*pixelBlob.Get(), nullptr);

		{
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(Vertex) * maxPointsPerBatch,
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_VERTEX_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};

			vertexBuffer = device->CreateBuffer(bufferDesc);
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
			cameraBuffer = device->CreateBuffer(bufferDesc);
		}


		{
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(xk::Math::Vector<float, 4>),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};
			batchBuffer = device->CreateBuffer(bufferDesc);
		}
	}

	void DebugPipelineDX11::Bind(RendererDX11& renderer)
	{
		renderer.GetDeviceContext()->IASetInputLayout(layout);
		renderer.GetDeviceContext()->VSSetShader(vertexShader, {});
		renderer.GetDeviceContext()->PSSetShader(pixelShader, {});

		D3D11_TEXTURE2D_DESC desc = TypedD3D::Cast<ID3D11Texture2D>(renderer.GetSwapChainBackBuffer()->GetResource())->GetDesc();
		D3D11_VIEWPORT viewports;
		viewports.TopLeftX = 0;
		viewports.TopLeftY = 0;
		viewports.MinDepth = 0;
		viewports.MaxDepth = 1;
		viewports.Width = static_cast<FLOAT>(desc.Width);
		viewports.Height = static_cast<FLOAT>(desc.Height);
		renderer.GetDeviceContext()->RSSetViewports(viewports);
		renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}
}