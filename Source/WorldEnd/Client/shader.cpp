#include "shader.h"

Shader::Shader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature) : Shader{}
{
	g_mutex.lock();
	g_loadingText = L"Shader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/standard.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_STANDARD_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/standard.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_STANDARD_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

}

void Shader::Update(FLOAT timeElapsed)
{
	if (m_player) m_player->Update(timeElapsed);

	for (const auto& elm : m_gameObjects)
		elm->Update(timeElapsed);

	for (const auto& elm : m_multiPlayers)
		elm.second->UpdateAnimation(timeElapsed);

	for (const auto& elm : m_arrows)
		elm.second->Update(timeElapsed);

	for (const auto& elm : m_monsters) 
		elm.second->UpdateAnimation(timeElapsed);
	
}

void Shader::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	Shader::UpdateShaderVariable(commandList);

	if (m_player) m_player->Render(commandList);

	for (const auto& elm : m_gameObjects)
		if (elm) elm->Render(commandList);

	for (const auto& elm : m_multiPlayers)
		if (elm.second) elm.second->Render(commandList);

	for (const auto& elm : m_arrows)
		if (elm.second) elm.second->Render(commandList);

	for (const auto& elm : m_monsters)
		if (elm.second) elm.second->Render(commandList); 
}

void Shader::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader) const
{
	shader->UpdateShaderVariable(commandList);
	
	if (m_player) m_player->Render(commandList);

	for (const auto& elm : m_gameObjects)
		if (elm) elm->Render(commandList);

	for (const auto& elm : m_multiPlayers)
		if (elm.second) elm.second->Render(commandList);

	for (const auto& elm : m_arrows)
		if (elm.second) elm.second->Render(commandList);

	for (const auto& elm : m_monsters)
		if (elm.second) elm.second->Render(commandList);
}

void Shader::Clear()
{
	m_gameObjects.clear();
	m_player.reset();
	m_camera.reset();
	m_multiPlayers.clear();
	m_monsters.clear();
}

void Shader::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	commandList->SetPipelineState(m_pipelineState.Get());
}

void Shader::SetPlayer(const shared_ptr<Player>& player)
{
	if (m_player) m_player.reset();
	m_player = player;
}

void Shader::SetCamera(const shared_ptr<Camera>& camera)
{
	if (m_camera) m_camera.reset();
	m_camera = camera;
}

void Shader::SetObject(const shared_ptr<GameObject>& object)
{
	m_gameObjects.push_back(object);
}

void Shader::SetMultiPlayer(INT ID, const shared_ptr<Player>& player)
{
	m_multiPlayers.insert({ ID, player });
}

void Shader::SetMonster(INT ID, const shared_ptr<Monster>& monster)
{
	m_monsters.insert({ ID, monster });
}

bool Shader::FindObject(const shared_ptr<GameObject>& object)
{
	if (ranges::find(m_gameObjects, object) != m_gameObjects.end())
		return true;
	return false;
}

void Shader::RemoveObject(const shared_ptr<GameObject>& object)
{
	erase(m_gameObjects, object);
}

void Shader::RemoveMultiPlayer(INT id)
{
	if(m_multiPlayers.contains(id))
		m_multiPlayers.erase(id);
}

void Shader::RemoveMonster(INT id)
{
	if (m_monsters.contains(id))
		m_monsters.erase(id);
}

StaticObjectShader::StaticObjectShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"StaticObjectShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/staticObject.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_STATICOBJECT_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/staticObject.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_STATICOBJECT_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

StaticObjectBlendShader::StaticObjectBlendShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"StaticObjectBlendShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/staticObject.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_STATICOBJECT_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/staticObject.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_STATICOBJECT_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void StaticObjectBlendShader::Update(FLOAT timeElapsed)
{
	if (m_camera) {
		sort(m_gameObjects.begin(), m_gameObjects.end(), [&](const shared_ptr<GameObject>& l, const shared_ptr<GameObject>& r) -> bool {
			return (Vector3::Length(Vector3::Sub(l->GetPosition(), m_camera->GetEye())) >
				Vector3::Length(Vector3::Sub(r->GetPosition(), m_camera->GetEye())));
		});
	}
	Shader::Update(timeElapsed);
}

AnimationShader::AnimationShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"AnimationShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/animation.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_ANIMATION_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/animation.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_ANIMATION_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 72, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

SkyboxShader::SkyboxShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"SkyboxShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/skybox.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_SKYBOX_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/skybox.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_SKYBOX_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

HorzGaugeShader::HorzGaugeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"HorzGaugeShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/gauge.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_GAUGE_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/gauge.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_GAUGE_MAIN", "gs_5_1", compileFlags, 0, &mgsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/gauge.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_HORZGAUGE_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.AlphaToCoverageEnable = TRUE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

VertGaugeShader::VertGaugeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"VertGaugeShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/gauge.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_GAUGE_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/gauge.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_GAUGE_MAIN", "gs_5_1", compileFlags, 0, &mgsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/gauge.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_VERTGAUGE_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.AlphaToCoverageEnable = TRUE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

ShadowShader::ShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"ShadowShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/Shadow.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_SHADOW_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/Shadow.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_SHADOW_MAIN", "gs_5_1", compileFlags, 0, &mgsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.DepthBias = 1000;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;

	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsByteCode.Get());
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

AnimationShadowShader::AnimationShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"AnimationShadowShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/shadow.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_ANIMATION_SHADOW_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/shadow.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_ANIMATION_SHADOW_MAIN", "gs_5_1", compileFlags, 0, &mgsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 72, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.DepthBias = 1000;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;

	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsByteCode.Get());
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

HorzBlurShader::HorzBlurShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"HorzBlurShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mcsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/blur.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS_HORZBLUR_MAIN", "cs_5_1", compileFlags, 0, &mcsByteCode, nullptr));

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.CS = CD3DX12_SHADER_BYTECODE(mcsByteCode.Get());
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

VertBlurShader::VertBlurShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"VertBlurShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mcsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/blur.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS_VERTBLUR_MAIN", "cs_5_1", compileFlags, 0, &mcsByteCode, nullptr));

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.CS = CD3DX12_SHADER_BYTECODE(mcsByteCode.Get());
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

FadeShader::FadeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"FadeShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mcsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/fade.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS_FADE_MAIN", "cs_5_1", compileFlags, 0, &mcsByteCode, nullptr));

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.CS = CD3DX12_SHADER_BYTECODE(mcsByteCode.Get());
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

SobelShader::SobelShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"SobelShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mcsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/sobel.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS_SOBEL_MAIN", "cs_5_1", compileFlags, 0, &mcsByteCode, nullptr));

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.CS = CD3DX12_SHADER_BYTECODE(mcsByteCode.Get());
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

CompositeShader::CompositeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"CompositeShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/composite.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_COMPOSITE_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/composite.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_COMPOSITE_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	// Disable depth test.
	depthStencilState.DepthEnable = false;
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void CompositeShader::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	Shader::UpdateShaderVariable(commandList);

	// Null-out IA stage since we build the vertex off the SV_VertexID in the shader.
	commandList->IASetVertexBuffers(0, 1, nullptr);
	commandList->IASetIndexBuffer(nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->DrawInstanced(6, 1, 0, 0);
}

TriggerEffectShader::TriggerEffectShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"TriggerEffectShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/circle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_CIRCLE_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/circle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_CIRCLE_MAIN", "gs_5_1", compileFlags, 0, &mgsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/circle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_CIRCLE_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.AlphaToCoverageEnable = false;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

EmitterParticleShader::EmitterParticleShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"EmitterParticleShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsdByteCode;
	ComPtr<ID3DBlob> mgssoByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_EMITTERPARTICLE_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_EMITTERPARTICLE_DRAW", "gs_5_1", compileFlags, 0, &mgsdByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_EMITTERPARTICLE_STREAMOUTPUT", "gs_5_1", compileFlags, 0, &mgssoByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_EMITTERPARTICLE_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_SO_DECLARATION_ENTRY* soDecls{ new D3D12_SO_DECLARATION_ENTRY[4] };
	soDecls[0] = { 0, "POSITION", 0, 0, 3, 0 };
	soDecls[1] = { 0, "VELOCITY", 0, 0, 3, 0 };
	soDecls[2] = { 0, "AGE", 0, 0, 1, 0 };
	soDecls[3] = { 0, "LIFETIME", 0, 0, 1, 0 };

	UINT* bufferStrides{ new UINT[1] };
	bufferStrides[0] = sizeof(EmitterParticleVertex);

	D3D12_STREAM_OUTPUT_DESC streamOutput{};
	streamOutput.NumEntries = 4;
	streamOutput.pSODeclaration = soDecls;
	streamOutput.NumStrides = 1;
	streamOutput.pBufferStrides = bufferStrides;
	streamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	CD3DX12_BLEND_DESC blendState{ D3D12_DEFAULT };
	blendState.RenderTarget[0].BlendEnable = TRUE;
	blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC streamPsoDesc{};
	streamPsoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	streamPsoDesc.pRootSignature = rootSignature.Get();
	streamPsoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	streamPsoDesc.GS = CD3DX12_SHADER_BYTECODE(mgssoByteCode.Get());
	streamPsoDesc.StreamOutput = streamOutput;
	streamPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	streamPsoDesc.DepthStencilState = depthStencilState;
	streamPsoDesc.BlendState = blendState;
	streamPsoDesc.SampleMask = UINT_MAX;
	streamPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	streamPsoDesc.NumRenderTargets = 0;
	streamPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	streamPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	streamPsoDesc.SampleDesc.Count = 1;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&streamPsoDesc, IID_PPV_ARGS(&m_streamPipelineState)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsdByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = blendState;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

PumperParticleShader::PumperParticleShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"PumperParticleShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsdByteCode;
	ComPtr<ID3DBlob> mgssoByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_PUMPERPARTICLE_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_PUMPERPARTICLE_DRAW", "gs_5_1", compileFlags, 0, &mgsdByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_PUMPERPARTICLE_STREAMOUTPUT", "gs_5_1", compileFlags, 0, &mgssoByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/particle.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_PUMPERPARTICLE_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_SO_DECLARATION_ENTRY* soDecls{ new D3D12_SO_DECLARATION_ENTRY[3] };
	soDecls[0] = { 0, "POSITION", 0, 0, 3, 0 };
	soDecls[1] = { 0, "VELOCITY", 0, 0, 3, 0 };
	soDecls[2] = { 0, "WEIGHT", 0, 0, 1, 0 };

	UINT* bufferStrides{ new UINT[1] };
	bufferStrides[0] = sizeof(PumperParticleVertex);

	D3D12_STREAM_OUTPUT_DESC streamOutput{};
	streamOutput.NumEntries = 3;
	streamOutput.pSODeclaration = soDecls;
	streamOutput.NumStrides = 1;
	streamOutput.pBufferStrides = bufferStrides;
	streamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	CD3DX12_BLEND_DESC blendState{ D3D12_DEFAULT };
	blendState.RenderTarget[0].BlendEnable = TRUE;
	blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC streamPsoDesc{};
	streamPsoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	streamPsoDesc.pRootSignature = rootSignature.Get();
	streamPsoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	streamPsoDesc.GS = CD3DX12_SHADER_BYTECODE(mgssoByteCode.Get());
	streamPsoDesc.StreamOutput = streamOutput;
	streamPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	streamPsoDesc.DepthStencilState = depthStencilState;
	streamPsoDesc.BlendState = blendState;
	streamPsoDesc.SampleMask = UINT_MAX;
	streamPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	streamPsoDesc.NumRenderTargets = 0;
	streamPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	streamPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	streamPsoDesc.SampleDesc.Count = 1;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&streamPsoDesc, IID_PPV_ARGS(&m_streamPipelineState)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsdByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = blendState;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

UIShader::UIShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"UIShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/ui.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_UI_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/ui.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_UI_MAIN", "gs_5_1", compileFlags, 0, &mgsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/ui.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_UI_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.AlphaToCoverageEnable = false;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void UIShader::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	Shader::UpdateShaderVariable(commandList);

	for (const auto& ui : m_ui) {
		ui->Render(commandList, nullptr);
	}
}

void UIShader::Clear()
{
	Shader::Clear();
	m_ui.clear();
}

DebugShader::DebugShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature)
{
	g_mutex.lock();
	g_loadingText = L"DebugShader";
	++g_loadingIndex;
	g_mutex.unlock();

	ComPtr<ID3DBlob> mvsByteCode;
	ComPtr<ID3DBlob> mgsByteCode;
	ComPtr<ID3DBlob> mpsByteCode;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/debug.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_DEBUG_MAIN", "vs_5_1", compileFlags, 0, &mvsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/debug.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS_DEBUG_MAIN", "gs_5_1", compileFlags, 0, &mgsByteCode, nullptr));
	Utiles::ThrowIfFailed(D3DCompileFromFile(TEXT("Resource/Shader/debug.hlsl"), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_DEBUG_MAIN", "ps_5_1", compileFlags, 0, &mpsByteCode, nullptr));

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mvsByteCode.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(mgsByteCode.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mpsByteCode.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.AlphaToCoverageEnable = TRUE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	Utiles::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}
