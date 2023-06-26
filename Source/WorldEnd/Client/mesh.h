#pragma once
#include "stdafx.h"
#include "texture.h"
#include "material.h"

class GameObject;

namespace AnimationSetting {
	constexpr int MAX_BONE = 100;

	constexpr int STANDARD_MESH = 1;
	constexpr int ANIMATION_MESH = 2;
}

struct Vertex
{
	Vertex(const XMFLOAT3& p) : position{ p } { }
	~Vertex() = default;

	XMFLOAT3 position;
};

struct NormalVertex
{
	NormalVertex() : position{ XMFLOAT3{0.f, 0.f, 0.f} }, normal{ XMFLOAT3{0.f, 0.f, 0.f} } {}
	NormalVertex(const XMFLOAT3& p, const XMFLOAT3& n) : position{ p }, normal{ n } { }
	~NormalVertex() = default;

	XMFLOAT3 position;
	XMFLOAT3 normal;
};

struct TextureVertex
{
	TextureVertex(const XMFLOAT3& position, const XMFLOAT2& uv) : position{ position }, uv{ uv } { }
	~TextureVertex() = default;

	XMFLOAT3 position;
	XMFLOAT2 uv;
};

struct DetailVertex
{
	DetailVertex(const XMFLOAT3& p, const XMFLOAT2& uv0, const XMFLOAT2& uv1) : position{ p }, uv0{ uv0 }, uv1{ uv1 } { }
	~DetailVertex() = default;

	XMFLOAT3 position;
	XMFLOAT2 uv0;
	XMFLOAT2 uv1;
};

struct SkyboxVertex
{
	SkyboxVertex(const XMFLOAT3& position) : position{ position } { }
	~SkyboxVertex() = default;

	XMFLOAT3 position;
};

struct TextureHierarchyVertex
{
	TextureHierarchyVertex() : position{ XMFLOAT3{0.f, 0.f, 0.f} }, normal{ XMFLOAT3{0.f, 0.f, 0.f} },
		tangent{ XMFLOAT3{0.f, 0.f, 0.f} }, biTangent{ XMFLOAT3{0.f, 0.f, 0.f} }, uv0{ XMFLOAT2{0.f, 0.f} }{}
	TextureHierarchyVertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT3& t, const XMFLOAT3& bt, const XMFLOAT2& uv0) : 
		position{ p }, normal{ n }, tangent{ t }, biTangent{ bt }, uv0{ uv0 } { }
	~TextureHierarchyVertex() = default;

	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
	XMFLOAT2 uv0;
};

struct AnimationVertex
{
	AnimationVertex() : position{ XMFLOAT3{0.f, 0.f, 0.f} }, normal{ XMFLOAT3{0.f, 0.f, 0.f} },
		tangent{ XMFLOAT3{0.f, 0.f, 0.f} }, biTangent{ XMFLOAT3{0.f, 0.f, 0.f} }, uv0{ XMFLOAT2{0.f, 0.f} },
		boneIndex{ XMINT4(0, 0,0,0) }, boneWeight{ XMFLOAT4(0.f, 0.f, 0.f, 0.f) } {}
	AnimationVertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT3& t, const XMFLOAT3& bt,
		const XMFLOAT2& uv0, const XMINT4& bi, const XMFLOAT4& bw)
		: position{ p }, normal{ n }, tangent{ t }, biTangent{ bt }, uv0{ uv0 }, boneIndex{ bi }, boneWeight{ bw } { }
	~AnimationVertex() = default;

	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
	XMFLOAT2 uv0;
	XMINT4 boneIndex;
	XMFLOAT4 boneWeight;
};

class Mesh
{
public:
	Mesh() = default;
	Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const vector<Vertex>& vertices, const vector<UINT>& indices);
	virtual ~Mesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, GameObject* rootObject, const GameObject* object) {}
	virtual void ReleaseUploadBuffer();

	BoundingOrientedBox GetBoundingBox() { return m_boundingBox; }
	
	virtual void CreateShaderVariables(GameObject* rootObject) {}

protected:
	UINT						m_nVertices;
	ComPtr<ID3D12Resource>		m_vertexBuffer;
	ComPtr<ID3D12Resource>		m_vertexUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW	m_vertexBufferView;

	UINT						m_nIndices;
	ComPtr<ID3D12Resource>		m_indexBuffer;
	ComPtr<ID3D12Resource>		m_indexUploadBuffer;
	D3D12_INDEX_BUFFER_VIEW		m_indexBufferView;

	D3D12_PRIMITIVE_TOPOLOGY	m_primitiveTopology;
	BoundingOrientedBox			m_boundingBox;
};

class MeshFromFile : public Mesh
{
public:
	MeshFromFile();
	virtual ~MeshFromFile() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex) const override;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const override;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, GameObject* rootObject, const GameObject* object) override {}

	void ReleaseUploadBuffer() override;

	void LoadFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const wstring& fileName);
	void LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);

	string GetMeshName() const { return m_meshName; }

	virtual void CreateShaderVariables(GameObject* rootObject) override {}

protected:
	string								m_meshName;

	UINT								m_nSubMeshes;
	vector<UINT>						m_vSubsetIndices;
	vector<vector<UINT>>				m_vvSubsetIndices;

	vector<ComPtr<ID3D12Resource>>		m_subsetIndexBuffers;
	vector<ComPtr<ID3D12Resource>>		m_subsetIndexUploadBuffers;
	vector<D3D12_INDEX_BUFFER_VIEW>		m_subsetIndexBufferViews;
};

// ------------------------------------------

class GameObject;

class AnimationMesh : public MeshFromFile
{
public: 
	AnimationMesh();
	virtual ~AnimationMesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, GameObject* rootObject, const GameObject* object) override;

	virtual void CreateShaderVariables(GameObject* rootObject) override;

	void UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* rootObject, const GameObject* object);

	void ReleaseUploadBuffer() override;

	void LoadAnimationMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);

	virtual string GetAnimationMeshName() const { return m_animationMeshName; }

	UINT GetType() const { return m_meshType; }
	void SetType(int type) { m_meshType = type; }
private:
	string								m_animationMeshName;

	vector<string>						m_skinningBoneNames;

	vector<XMFLOAT4X4>					m_bindPoseBoneOffsets;	// 바인드 포즈에서의 뼈 오프셋 행렬들

	ComPtr<ID3D12Resource>				m_bindPoseBoneOffsetBuffers;
	XMFLOAT4X4*							m_bindPoseBoneOffsetBuffersPointer;

	unordered_map<GameObject*, pair<ComPtr<ID3D12Resource>, XMFLOAT4X4*>>	 m_animationTransformBuffers;

	int									m_meshType;
};

class SkyboxMesh : public Mesh
{
public:
	SkyboxMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
		FLOAT width = 20.0f, FLOAT height = 20.0f, FLOAT depth = 20.0f);
	~SkyboxMesh() = default;
};

// 기하 셰이더를 사용해야 함.
class PlaneMesh : public Mesh
{
public:
	PlaneMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		XMFLOAT3 position, XMFLOAT2 size);
	~PlaneMesh() = default;
};