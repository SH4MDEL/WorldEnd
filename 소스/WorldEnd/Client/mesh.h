#pragma once
#include "stdafx.h"
#include "texture.h"
#include "material.h"

struct Vertex
{
	Vertex(const XMFLOAT3& p, const XMFLOAT4& c) : position{ p }, color{ c } { }
	~Vertex() = default;

	XMFLOAT3 position;
	XMFLOAT4 color;
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

class Mesh
{
public:
	Mesh() = default;
	Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const vector<TextureVertex>& vertices, const vector<UINT>& indices);
	virtual ~Mesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const;
	virtual void ReleaseUploadBuffer();

	BoundingOrientedBox GetBoundingBox() { return m_boundingBox; }

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
	~MeshFromFile() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList) const;

	void ReleaseUploadBuffer() override;

	void LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);
	void LoadMaterial(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);
	void SetMaterials(const shared_ptr<vector<Material>> materials) { m_materials = materials; }

private:
	string								m_meshName;

	UINT								m_nSubMeshes;
	vector<UINT>						m_vSubsetIndices;
	vector<vector<UINT>>				m_vvSubsetIndices;

	vector<ComPtr<ID3D12Resource>>		m_subsetIndexBuffers;
	vector<ComPtr<ID3D12Resource>>		m_subsetIndexUploadBuffers;
	vector<D3D12_INDEX_BUFFER_VIEW>		m_subsetIndexBufferViews;

	shared_ptr<vector<Material>>		m_materials;
};

// ------------------------------------------
class GameObject;

class SkinnedMesh : public Mesh
{
public: 
	SkinnedMesh();
	~SkinnedMesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList) const;

	void ReleaseUploadBuffer() override;

	void LoadSkinnedMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);

private:
	string								m_skinnedMeshName;

	UINT								m_nBonesPerVertex;

	vector<XMUINT4>						m_boneIndices;
	vector<XMFLOAT4>					m_boneWeights;

	vector<ComPtr<ID3D12Resource>>		m_boneIndicesBuffers;
	vector<ComPtr<ID3D12Resource>>		m_boneIndicesUploadBuffers;
	vector<D3D12_INDEX_BUFFER_VIEW>		m_boneIndicesBufferViews;

	vector<ComPtr<ID3D12Resource>>		m_boneWeightsBuffers;
	vector<ComPtr<ID3D12Resource>>		m_boneWeightsUploadBuffers;
	vector<D3D12_INDEX_BUFFER_VIEW>		m_boneWeightsBufferViews;
	

	UINT								m_nSkinningBones;

	vector<string>						m_skinningBoneNames;
	vector<GameObject>					m_skinningBoneFrame;	// �ش� ���� �ٷ� ã�ư��� ���� �����ϴ� ����

	vector<XMFLOAT4X4>					m_bindPoseBoneOffsets;	// ���ε� ������� �� ������ ��ĵ�

	vector<ComPtr<ID3D12Resource>>		m_bindPoseBoneOffsetBuffers;
	vector<XMFLOAT4X4>					m_mappedBindPoseBoneOffsets;

	vector<ComPtr<ID3D12Resource>>		m_skinningBoneTransformBuffers;
	vector<XMFLOAT4X4>					m_mappedSkinningBoneTransforms;
};

class FieldMapImage
{
public:
	FieldMapImage(INT width, INT length, INT height, XMFLOAT3 scale);
	~FieldMapImage() = default;

	FLOAT GetHeight(FLOAT x, FLOAT z) const;	// (x, z) ��ġ�� �ȼ� ���� ����� ���� ���� ��ȯ
	XMFLOAT3 GetNormal(INT x, INT z) const;		// (x, z) ��ġ�� ���� ���� ��ȯ

	XMFLOAT3 GetScale() { return m_scale; }
	BYTE* GetPixels() { return m_pixels.get(); }
	INT GetWidth() { return m_width; }
	INT GetLength() { return m_length; }
private:
	unique_ptr<BYTE[]>			m_pixels;	// ���� �� �̹��� �ȼ�(8-��Ʈ)���� ������ �迭�̴�. �� �ȼ��� 0~255�� ���� ���´�.
	INT							m_width;	// ���� �� �̹����� ���ο� ���� ũ���̴�
	INT							m_length;
	XMFLOAT3					m_scale;	// ���� �� �̹����� ������ �� �� Ȯ���Ͽ� ����� ���ΰ��� ��Ÿ���� ������ �����̴�
};

class FieldMapGridMesh : public Mesh
{
public:
	FieldMapGridMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		INT xStart, INT zStart, INT width, INT length, XMFLOAT3 scale, FieldMapImage* heightMapImage);
	virtual ~FieldMapGridMesh() = default;

	//virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet);

	XMFLOAT3 GetScale() { return(m_scale); }
	INT GetWidth() { return(m_width); }
	INT GetLength() { return(m_length); }

protected:
	//������ ũ��(����: x-����, ����: z-����)�̴�. 
	INT m_width;
	INT m_length;

	//������ ������(����: x-����, ����: z-����, ����: y-����) �����̴�. 
	//���� ���� �޽��� �� ������ x-��ǥ, y-��ǥ, z-��ǥ�� ������ ������ x-��ǥ, y-��ǥ, z-��ǥ�� ���� ���� ���´�. 
	//��, ���� ������ x-�� ������ ������ 1�� �ƴ϶� ������ ������ x-��ǥ�� �ȴ�. 
	//�̷��� �ϸ� ���� ����(���� ����)�� ����ϴ��� ū ũ���� ����(����)�� ������ �� �ִ�.
	XMFLOAT3 m_scale;
};

class TextureRectMesh : public Mesh
{
public:
	TextureRectMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		XMFLOAT3 position, FLOAT width=20.0f, FLOAT height=20.0f, FLOAT depth=20.0f);
	~TextureRectMesh() = default;
};

class SkyboxMesh : public Mesh
{
public:
	SkyboxMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
		FLOAT width = 20.0f, FLOAT height = 20.0f, FLOAT depth = 20.0f);
	~SkyboxMesh() = default;
};