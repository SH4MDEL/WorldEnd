#pragma once
#include <fbxsdk.h>
#include "stdafx.h"
#include "mesh.h"

class AnimationManager
{
public:
	AnimationManager();
	~AnimationManager();

	void Initialize();

	void ProcessNode(FbxNode* fbxNode);
	void ProcessMesh(FbxMesh* mesh);

	void SetVertices(vector<TextureVertex>& vertices) { vertices = m_vertices; }
	vector<TextureVertex>& GetVertices() { return m_vertices; }

private:
	FbxManager*		m_sdkManager;
	FbxScene*		m_scene;
	FbxMesh*		m_mesh;
	
	/*unique_ptr<FbxManager>		m_sdkManager;
	unique_ptr<FbxScene>		m_scene;*/

	vector<TextureVertex>	 m_vertices;
	vector<UINT>			 m_indices;
};

