module;

#include <memory>
#include <d3d11_4.h>
#include <list>

export module InsanityEditor.EditorContext;
import TypedD3D11;

namespace InsanityEditor
{
	export struct EditorContext;

	struct ContextDeleter
	{
		void operator()(EditorContext* context);
	};

	export using EditorContextHandle = std::unique_ptr<EditorContext, ContextDeleter>;

	export struct EditorContext
	{
		friend ContextDeleter;

	private:
		inline static EditorContext* activeContext;
		inline static std::list<EditorContext> contexts;

	public:
		TypedD3D::Wrapper<ID3D11Device> device;
		TypedD3D::Wrapper<ID3D11DeviceContext> deviceContext;

	private:
		EditorContext() = default;

	public:
		static EditorContextHandle New(TypedD3D::Wrapper<ID3D11Device> device, TypedD3D::Wrapper<ID3D11DeviceContext> deviceContext)
		{
			contexts.push_back({});
			if (!activeContext)
				activeContext = &contexts.back();

			EditorContext& context = contexts.back();
			context.device = device;
			context.deviceContext = deviceContext;

			return EditorContextHandle{ &context };
		}

		static void Delete(EditorContext* context)
		{
			std::erase_if(contexts, [=](const EditorContext& c) { return &c == context; });
			if (!contexts.empty())
				activeContext = &contexts.front();
		}

		static EditorContext& GetActiveContext()
		{
			if (!activeContext)
				throw std::exception("No active editor context");
			return *activeContext;
		}

		static void SetActiveContext(EditorContext* context)
		{
			activeContext = context || contexts.empty() ? context : &contexts.front();
		}
	};

	void ContextDeleter::operator()(EditorContext* context)
	{
		EditorContext::Delete(context);
	}
}