#pragma once
#include "stdafx.h"
#include "texture.h"
#include "material.h"

#define SKINNED_MESH 1
#define STANDARD_MESH 2
#define SKINNED_BONES 128

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

struct SkinnedVertex
{
	SkinnedVertex() : position{ XMFLOAT3{0.f, 0.f, 0.f} }, normal{ XMFLOAT3{0.f, 0.f, 0.f} },
		tangent{ XMFLOAT3{0.f, 0.f, 0.f} }, biTangent{ XMFLOAT3{0.f, 0.f, 0.f} }, uv0{ XMFLOAT2{0.f, 0.f} },
		boneIndex{ XMINT4(0, 0,0,0) }, boneWeight{ XMFLOAT4(0.f, 0.f, 0.f, 0.f) } {}
	SkinnedVertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT3& t, const XMFLOAT3& bt,
		const XMFLOAT2& uv0, const XMINT4& bi, const XMFLOAT4& bw)
		: position{ p }, normal{ n }, tangent{ t }, biTangent{ bt }, uv0{ uv0 }, boneIndex{ bi }, boneWeight{ bw } { }
	~SkinnedVertex() = default;

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
	Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const vector<TextureVertex>& vertices, const vector<UINT>& indices);
	virtual ~Mesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, UINT subMeshIndex) const;
	virtual void ReleaseUploadBuffer();

	BoundingOrientedBox GetBoundingBox() { return m_boundingBox; }

	UINT GetMeshType() const { return m_meshType; }
	void SetMeshType(UINT type) { m_meshType = type; }

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

	UINT						m_meshType;
};

class MeshFromFile : public Mesh
{
public:
	MeshFromFile();
	virtual ~MeshFromFile() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, UINT subMeshIndex) const override;

	void ReleaseUploadBuffer() override;

	void LoadFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const wstring& fileName);
	void LoadFileMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);
	void LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);

	string GetMeshName() const { return m_meshName; }


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


class SkinnedMesh : public MeshFromFile
{
public: 
	SkinnedMesh();
	virtual ~SkinnedMesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex) const override;

	void UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	void ReleaseUploadBuffer() override;

	void LoadSkinnedMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);

	ComPtr<ID3D12Resource>& GetSkinningBoneTransform() { return m_skinningBoneTransformBuffers; }
	XMFLOAT4X4*& GetMappedSkinningBoneTransform() { return m_mappedSkinningBoneTransforms; }
	vector<string>& GetSkinningBoneNames() { return m_skinningBoneNames; }
	vector<shared_ptr<GameObject>>& GetSkinningBoneFrames() { return m_skinningBoneFrames; }

	UINT GetSkinningBoneNum() const { return m_skinningBoneFrames.size(); }
	string GetSkinnedMeshName() const { return m_skinnedMeshName; }
private:
	string								m_skinnedMeshName;

	UINT								m_nBonesPerVertex;

	vector<string>						m_skinningBoneNames;
	vector<shared_ptr<GameObject>>		m_skinningBoneFrames;	// 해당 뼈를 바로 찾아가기 위해 저장하는 벡터

	vector<XMFLOAT4X4>					m_bindPoseBoneOffsets;	// 바인드 포즈에서의 뼈 오프셋 행렬들

	ComPtr<ID3D12Resource>				m_bindPoseBoneOffsetBuffers;
	XMFLOAT4X4*							m_mappedBindPoseBoneOffsets;

	ComPtr<ID3D12Resource>				m_skinningBoneTransformBuffers;
	XMFLOAT4X4*							m_mappedSkinningBoneTransforms;
};

class FieldMapImage
{
public:
	FieldMapImage(INT width, INT length, INT height, XMFLOAT3 scale);
	~FieldMapImage() = default;

	FLOAT GetHeight(FLOAT x, FLOAT z) const;	// (x, z) 위치의 픽셀 값에 기반한 지형 높이 반환
	XMFLOAT3 GetNormal(INT x, INT z) const;		// (x, z) 위치의 법선 벡터 반환

	XMFLOAT3 GetScale() { return m_scale; }
	BYTE* GetPixels() { return m_pixels.get(); }
	INT GetWidth() { return m_width; }
	INT GetLength() { return m_length; }
private:
	unique_ptr<BYTE[]>			m_pixels;	// 높이 맵 이미지 픽셀(8-비트)들의 이차원 배열이다. 각 픽셀은 0~255의 값을 갖는다.
	INT							m_width;	// 높이 맵 이미지의 가로와 세로 크기이다
	INT							m_length;
	XMFLOAT3					m_scale;	// 높이 맵 이미지를 실제로 몇 배 확대하여 사용할 것인가를 나타내는 스케일 벡터이다
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
	//격자의 크기(가로: x-방향, 세로: z-방향)이다. 
	INT m_width;
	INT m_length;

	//격자의 스케일(가로: x-방향, 세로: z-방향, 높이: y-방향) 벡터이다. 
	//실제 격자 메쉬의 각 정점의 x-좌표, y-좌표, z-좌표는 스케일 벡터의 x-좌표, y-좌표, z-좌표로 곱한 값을 갖는다. 
	//즉, 실제 격자의 x-축 방향의 간격은 1이 아니라 스케일 벡터의 x-좌표가 된다. 
	//이렇게 하면 작은 격자(적은 정점)를 사용하더라도 큰 크기의 격자(지형)를 생성할 수 있다.
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