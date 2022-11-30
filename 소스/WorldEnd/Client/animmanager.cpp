#include "animmanager.h"

AnimationManager::AnimationManager()
{
}

AnimationManager::~AnimationManager()
{
}

void AnimationManager::Initialize()
{
	m_sdkManager = FbxManager::Create();

	// 생성 실패
	if (!m_sdkManager) {
		printf("FBX Create Error\n");
	}

	FbxIOSettings* ioSetting = FbxIOSettings::Create(m_sdkManager, IOSROOT);
	m_sdkManager->SetIOSettings(ioSetting);

	FbxImporter* importer = FbxImporter::Create(m_sdkManager, "");
	bool retval = importer->Initialize("./Resource/Model/lpMale_casual_G.FBX", -1,
		m_sdkManager->GetIOSettings());

	if (!retval) {
		printf("import fail\n");
	}

	m_scene = FbxScene::Create(m_sdkManager, "My Scene");
	if (!m_scene) {
		printf("Scene Fail\n");
	}
	importer->Import(m_scene);
	importer->Destroy();

	FbxNode* fbxRootNode = m_scene->GetRootNode();
	
	if (fbxRootNode) {
		for (int i = 0; i < fbxRootNode->GetChildCount(); ++i)
			ProcessNode(fbxRootNode->GetChild(i));
	}



}

int gnTabs = 0;
void PrintTabs()
{
	for (int i = 0; i < gnTabs; ++i)
		printf("\t");
}


FbxString GetAttributeTypeName(FbxNodeAttribute::EType type)
{
	if (type == FbxNodeAttribute::eMesh)	 return("Mesh");
	if (type == FbxNodeAttribute::eCamera) return("Camera");
}

void PrintAttribute(FbxNodeAttribute* fbxAttribute)
{
	FbxString type = GetAttributeTypeName(fbxAttribute->GetAttributeType());
	FbxString name = fbxAttribute->GetName();
	PrintTabs();
	printf("<Attribute type = %s  name = %s>\n", type, name);
}
void AnimationManager::ProcessNode(FbxNode* fbxNode)
{
	if (fbxNode) {
		PrintTabs();
		const char* nodeName = fbxNode->GetName();
		FbxDouble3 translation = fbxNode->LclTranslation.Get();
		FbxDouble3 rotation = fbxNode->LclRotation.Get();
		FbxDouble3 scaling = fbxNode->LclScaling.Get();

		printf("<Node name = %s Translation = (%f, %f, %f) Rotation = (%f, %f %f) Scailing = (%f, %f, %f)>\n",
			translation[0], translation[1], translation[2],
			rotation[0], rotation[1], rotation[2],
			scaling[0], scaling[1], scaling[2]);
		gnTabs++;
		for (int i = 0; i < fbxNode->GetNodeAttributeCount(); ++i) {
			PrintAttribute(fbxNode->GetNodeAttributeByIndex(i));
		}
		for (int i = 0; i < fbxNode->GetChildCount(); ++i)
			ProcessNode(fbxNode->GetChild(i));
		gnTabs--;

		printf("</Node>\n");
	}
}

