#pragma once
#include "fbxsdk.h"
#include "stdafx.h"

class AnimationManager
{
public:
	AnimationManager();
	~AnimationManager();

	void Initialize();

	void ProcessNode(FbxNode* fbxNode);

private:
	FbxManager*		m_sdkManager;
	FbxScene*		m_scene;
	
	/*unique_ptr<FbxManager>		m_sdkManager;
	unique_ptr<FbxScene>		m_scene;*/
};

