#include <fstream> 
#include <unordered_map>
#include "map.h"

vector<shared_ptr<GameObject>> Dungeon::m_structures;

void Dungeon::LoadMap()
{
	unordered_map<string, BoundingOrientedBox> bounding_box_data;

	ifstream in{ "./Resource/DungeonObject.bin", std::ios::binary };

	BYTE strLength{};
	string objectName;
	BoundingOrientedBox bounding_box{};

	while (in.read((char*)(&strLength), sizeof(BYTE))) {
		objectName.resize(strLength, '\0');
		in.read((char*)(&objectName[0]), strLength);

		in.read((char*)(&bounding_box.Extents), sizeof(XMFLOAT3));

		bounding_box_data.insert({ objectName, bounding_box });
	}
	in.close();

	
	in.open("./Resource/DungeonMap.bin", std::ios::binary);

	XMFLOAT3 position{};
	FLOAT yaw{};

	while (in.read((char*)(&strLength), sizeof(BYTE))) {
		objectName.resize(strLength, '\0');
		in.read(&objectName[0], strLength);

		in.read((char*)(&position), sizeof(XMFLOAT3));
		in.read((char*)(&yaw), sizeof(FLOAT));
		// 라디안 값

		if (!bounding_box_data.contains(objectName))
			continue;

		auto object = make_shared<GameObject>();
		object->SetPosition(position);
		object->SetRotation(yaw);

		XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, yaw, 0.f);
		XMFLOAT4 q{};
		XMStoreFloat4(&q, vec);

		bounding_box = bounding_box_data[objectName];
		bounding_box.Center = position;
		bounding_box.Orientation = XMFLOAT4{ q.x, q.y, q.x, q.w };
		
		object->SetBoundingBox(bounding_box);

		m_structures.push_back(object);
	}

}
