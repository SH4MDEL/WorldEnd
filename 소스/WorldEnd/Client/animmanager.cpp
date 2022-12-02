#include "animmanager.h"
#include "mesh.h"

AnimationManager::AnimationManager()
{
}

AnimationManager::~AnimationManager()
{
	if (m_mesh)
		m_mesh->Destroy();
	if (m_scene)
		m_scene->Destroy();
	if (m_sdkManager)
		m_sdkManager->Destroy();
	
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
	bool retval = importer->Initialize("./Resource/Model/Remy.FBX", -1,
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

	ProcessNode(fbxRootNode);

	//ProcessMesh(fbxRootNode->GetChild(0));



	//if (fbxRootNode) {
	//	for (int i = 0; i < fbxRootNode->GetChildCount(); ++i)
	//		;//ProcessNode(fbxRootNode);
	//}

	FbxAxisSystem sceneAxisSystem = m_scene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem axis(FbxAxisSystem::DirectX);
	if (sceneAxisSystem != axis)
		axis.ConvertScene(m_scene);


}

int gnTabs = 0;
void PrintTabs()
{
	for (int i = 0; i < gnTabs; ++i)
		cout << "\t";
}


FbxString GetAttributeTypeName(FbxNodeAttribute::EType type)
{
	if (type == FbxNodeAttribute::eMesh)	 return("Mesh");
	if (type == FbxNodeAttribute::eCamera) return("Camera");
	if (type == FbxNodeAttribute::eUnknown) return("Unknown");
	if (type == FbxNodeAttribute::eNull) return("Null");
	if (type == FbxNodeAttribute::eNurbs) return("Nurbs");
	if (type == FbxNodeAttribute::eMarker) return("Marker");
	if (type == FbxNodeAttribute::ePatch) return("Patch");
	if (type == FbxNodeAttribute::eCamera) return("Camera");
	if (type == FbxNodeAttribute::eCameraStereo) return("CameraStereo");
	if (type == FbxNodeAttribute::eCameraSwitcher) return("CameraSwitcher");
	if (type == FbxNodeAttribute::eLight) return("Light");
	if (type == FbxNodeAttribute::eOpticalReference) return("OpticalReference");
	if (type == FbxNodeAttribute::eOpticalMarker) return("OpticalMarker");
	if (type == FbxNodeAttribute::eNurbsCurve) return("NurbsCurve");
	if (type == FbxNodeAttribute::eTrimNurbsSurface) return("TrimNurbsSurface");
	if (type == FbxNodeAttribute::eBoundary) return("Boundary");
	if (type == FbxNodeAttribute::eNurbsSurface) return("NurbsSurface");
	if (type == FbxNodeAttribute::eShape) return("Shape");
	if (type == FbxNodeAttribute::eLODGroup) return("LODGroup");
	if (type == FbxNodeAttribute::eSubDiv) return("SubDiv");
	if (type == FbxNodeAttribute::eCachedEffect) return("CachedEffect");
	if (type == FbxNodeAttribute::eLine) return("Line");
	if (type == FbxNodeAttribute::eSkeleton) return("Skeleton");

}

void PrintAttribute(FbxNodeAttribute* fbxAttribute)
{
	if (fbxAttribute) {
		FbxString type = GetAttributeTypeName(fbxAttribute->GetAttributeType());
		FbxString name = fbxAttribute->GetName();
		//PrintTabs();
		if (type && name) {
			printf("<Attribute type = %s, name = %s>\n", type.Buffer(), name.Buffer());
		}
		else
			printf("<Atrribute type = None>\n");
	}
}

//void AnimationManager::ProcessNode(FbxNode* fbxNode)
//{
//	if (fbxNode) {
//		//PrintTabs();
//		const char* nodeName = fbxNode->GetName();
//		FbxDouble3 translation = fbxNode->LclTranslation.Get();
//		FbxDouble3 rotation = fbxNode->LclRotation.Get();
//		FbxDouble3 scaling = fbxNode->LclScaling.Get();
//
//		/*cout << printf("<Node name = %s Translation = (%f, %f, %f) Rotation = (%f, %f, %f) Scailing = (%f, %f, %f)>\n",
//			nodeName, translation[0], translation[1], translation[2],
//			rotation[0], rotation[1], rotation[2],
//			scaling[0], scaling[1], scaling[2]);*/
//
//		int nVertices{};
//		
//
//		gnTabs++;
//		for (int i = 0; i < fbxNode->GetNodeAttributeCount(); ++i) {
//			//PrintAttribute(fbxNode->GetNodeAttributeByIndex(i));
//			
//			FbxNodeAttribute* attribute = fbxNode->GetNodeAttributeByIndex(i);
//			if (attribute) {
//				// attribute 타입이 메쉬인 경우, 해당 FbxNode는 FbxMesh를 가지고 있음
//				if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh && !m_mesh) {
//					FbxMesh* mesh = fbxNode->GetMesh();
//					cout << "정점의 개수 : " << mesh->GetControlPointsCount() << endl;
//					cout << "폴리곤 갯수 : " << mesh->GetPolygonCount() << endl;
//
//					for (int j = 0; j < mesh->GetPolygonCount(); ++j) {
//						for (int k = 0; k < 3; ++k) {
//							//cout << mesh->GetPolygonVertex(j, k) << endl;
//						}
//					}
//					
//
//					// 정점 갯수, 정점 인덱스 갯수만큼 크기 재설정
//					//m_vertices.resize(mesh->GetControlPointsCount());
//					//m_indices.resize(mesh->GetPolygonVertexCount());
//
//					// normal, bitangent, tangent, uv 를 포인터 형태로 받음
//					FbxGeometryElementNormal* normal = mesh->GetElementNormal();
//					FbxGeometryElementBinormal* binormal = mesh->GetElementBinormal();
//					FbxGeometryElementTangent* tangent = mesh->GetElementTangent();
//					FbxGeometryElementUV* uv = mesh->GetElementUV();
//
//					// 컨트롤 포인터에 의해 매핑되도록 설정
//					if(normal)
//						normal->SetMappingMode(FbxGeometryElement::eByControlPoint);
//					if(binormal)
//						binormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
//					if(tangent)
//						tangent->SetMappingMode(FbxGeometryElement::eByControlPoint);
//					if(uv)
//						uv->SetMappingMode(FbxGeometryElement::eByControlPoint);
//					
//					// 정점마다 position, normal, binormal, tangent, uv를 출력하도록 함
//					for (int j = 0; j < mesh->GetControlPointsCount(); ++j) {
//						FbxDouble3 vec3 = mesh->GetControlPointAt(j);
//						//printf("position : % f, % f, % f\n", vec3[0], vec3[1], vec3[2]);
//						//m_vertices[j].position = XMFLOAT3(vec3[0], vec3[1], vec3[2]);
//
//						FbxVector2 uv2 = uv->GetDirectArray().GetAt(j);
//						//printf("uv : %f, %f\n", uv[0], uv[1]);
//						//m_vertices[j].uv = XMFLOAT2(uv2[0], uv2[1]);
//
//						m_vertices.emplace_back(XMFLOAT3(vec3[0], vec3[1], vec3[2]), XMFLOAT2(uv2[0], uv2[1]));
//
//						//FbxDouble3 nor = normal->GetDirectArray().GetAt(j);
//						//printf("normal : %f, %f, %f\n", nor[0], nor[1], nor[2]);
//						//m_vertices[j].normal = XMFLOAT3(nor[0], nor[1], nor[2]);
//						//FbxDouble3 bi = binormal->GetDirectArray().GetAt(j);
//						//printf("binormal : %f, %f, %f\n", bi[0], bi[1], bi[2]);
//						//m_vertices[j].biTangent = XMFLOAT3(bi[0], bi[1], bi[2]);
//						//FbxDouble3 tan = tangent->GetDirectArray().GetAt(j);
//						//printf("tangent : %f, %f, %f\n", tan[0], tan[1], tan[2]);
//						//m_vertices[j].tangent = XMFLOAT3(tan[0], tan[1], tan[2]);
//					}
//
//					// 폴리곤마다 인덱스를 구함
//					// 지금 fbx 파일은 인덱스가 폴리곤마다 거의 4개씩 있고 3개있는경우 있음
//					/*for (int j = 0; j < mesh->GetPolygonCount(); ++j) {
//						for (int k = 0; k < 3; ++k) {
//							mesh->GetPolygonVertex(j, k);
//						}
//					}*/
//
//				}
//			}
//
//			
//
//		}
//		
//		/*for (int i = 0; i < fbxNode->GetChildCount(); ++i)
//			ProcessNode(fbxNode->GetChild(i));
//		gnTabs--;*/
//
//		//cout << "</Node>\n";
//	}
//
//	/*if (fbxNode) {
//		if (fbxNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
//			FbxMesh* fbxMesh = fbxNode->GetMesh();
//			if (fbxMesh) {
//				cout << fbxMesh->GetControlPointsCount() << endl;
//			}
//		}
//
//		for (int i = 0; i < fbxNode->GetChildCount(); ++i) {
//			ProcessNode(fbxNode->GetChild(i));
//		}
//	}*/
//}

void PrintMatrix(const FbxAMatrix& mat)
{
	// 행렬은 열 우선
	// 00 : 0열 0행, 10 : 1열 0행
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			printf("%f ", mat[i][j]);
		}
		printf("\n");
	}
}

FbxAMatrix GetNodeOffset(FbxNode* fbxNode)
{
	FbxVector4 T = fbxNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	FbxVector4 R = fbxNode->GetGeometricRotation(FbxNode::eSourcePivot);
	FbxVector4 S = fbxNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return(FbxAMatrix(T, R, S));
}

void LoadTexture(FbxMesh* mesh)
{


}

void AnimationManager::ProcessNode(FbxNode* fbxNode)
{
	if (fbxNode) {

		

		//PrintAttribute(fbxNode->GetNodeAttribute());

		FbxAMatrix& globalTransform = fbxNode->EvaluateGlobalTransform();
		printf("global transform\n");
		PrintMatrix(globalTransform);

		FbxAMatrix& localTransform = fbxNode->EvaluateLocalTransform();
		printf("local transform\n");
		PrintMatrix(localTransform);

		FbxAMatrix offsetMatrix = GetNodeOffset(fbxNode);
		printf("offset Matrix\n");
		PrintMatrix(offsetMatrix);
		 
		// 월드변환 = 부모 월드변환 * 이동행렬 * 오프셋 회전행렬 * 피봇 회전행렬 *
		//			   회전 후 행렬의 역행렬 * 회전 피벗 행렬의 역행렬 *
		//			   크기변환 오프셋 행렬 * 크기변환 피벗행렬 * 크기변환 행렬 *
		//				크기변환 피벗의 역행렬

		if (fbxNode->GetMesh())
			ProcessMesh(fbxNode);
		for (int i = 0; i < fbxNode->GetChildCount(); ++i) {
			ProcessNode(fbxNode->GetChild(i));
		}

	}
}

void AnimationManager::ProcessMesh(FbxNode* fbxNode)
{
	FbxMesh* mesh = fbxNode->GetMesh();
	int vertexCount = mesh->GetControlPointsCount();
	int polygonCount = mesh->GetPolygonCount();
	FbxGeometryElementUV* uv = mesh->GetElementUV();
	if (uv)
		uv->SetMappingMode(FbxGeometryElement::eByControlPoint);

	

	FbxLayerElementArray arr = uv->GetDirectArray();
	//cout << arr.GetCount() << "uv array" << endl;

	// uv array 파일로 출력해보기


	int index[4]{};
	
	//FbxArray<FbxLayerElement::EType> arr= mesh->GetAllChannelUV(0);


	FbxAMatrix mat = fbxNode->EvaluateGlobalTransform();

	// 폴리곤 개수만큼 반복
	for (int i = 0; i < polygonCount; ++i) {
		
		// 폴리곤마다 정점의 개수
		int verticeCount = mesh->GetPolygonSize(i);

		// 폴리곤의 정점이 4개 -> 삼각형 2개로 나누기
		if (4 == verticeCount) {
			index[0] = mesh->GetPolygonVertex(i, 0);
			index[1] = mesh->GetPolygonVertex(i, 1);
			index[2] = mesh->GetPolygonVertex(i, 2);
			index[3] = mesh->GetPolygonVertex(i, 3);

			FbxVector4 pos = mesh->GetControlPointAt(index[0]);
			FbxDouble2 uv2 = uv->GetDirectArray().GetAt(index[0]);
			pos = mat.MultT(pos);
			m_vertices.emplace_back(XMFLOAT3(pos[0], pos[1], pos[2]), XMFLOAT2(uv2[0], uv2[1]));

			pos = mesh->GetControlPointAt(index[1]);
			uv2 = uv->GetDirectArray().GetAt(index[1]);
			pos = mat.MultT(pos);
			m_vertices.emplace_back(XMFLOAT3(pos[0], pos[1], pos[2]), XMFLOAT2(uv2[0], uv2[1]));

			pos = mesh->GetControlPointAt(index[2]);
			uv2 = uv->GetDirectArray().GetAt(index[2]);
			pos = mat.MultT(pos);
			m_vertices.emplace_back(XMFLOAT3(pos[0], pos[1], pos[2]), XMFLOAT2(uv2[0], uv2[1]));


			pos = mesh->GetControlPointAt(index[2]);
			uv2 = uv->GetDirectArray().GetAt(index[2]);
			pos = mat.MultT(pos);
			m_vertices.emplace_back(XMFLOAT3(pos[0], pos[1], pos[2]), XMFLOAT2(uv2[0], uv2[1]));

			pos = mesh->GetControlPointAt(index[3]);
			uv2 = uv->GetDirectArray().GetAt(index[3]);
			pos = mat.MultT(pos);
			m_vertices.emplace_back(XMFLOAT3(pos[0], pos[1], pos[2]), XMFLOAT2(uv2[0], uv2[1]));

			pos = mesh->GetControlPointAt(index[0]);
			uv2 = uv->GetDirectArray().GetAt(index[0]);
			pos = mat.MultT(pos);
			m_vertices.emplace_back(XMFLOAT3(pos[0], pos[1], pos[2]), XMFLOAT2(uv2[0], uv2[1]));
			
		}
		// 3개 -> 삼각형 1개
		else if (3 == verticeCount) {
			for (int j = 0; j < 3; ++j) {
				FbxVector4 pos = mesh->GetControlPointAt(mesh->GetPolygonVertex(i, j));
				FbxDouble2 uv2 = uv->GetDirectArray().GetAt(mesh->GetPolygonVertex(i, j));
				pos = mat.MultT(pos);
				m_vertices.emplace_back(XMFLOAT3(pos[0], pos[1], pos[2]), XMFLOAT2(uv2[0], uv2[1]));
			}
		}
	}
	
	//int* indices = mesh->GetPolygonVertices();
	//for (int i = 0; i < mesh->GetPolygonVertexCount(); ++i)
	//	cout << indices[i] << endl;

}

 