#include "scene.h"

unordered_map<string, shared_ptr<Mesh>>			Scene::m_meshs;
unordered_map<string, shared_ptr<Texture>>		Scene::m_textures;
unordered_map<string, shared_ptr<Shader>>		Scene::m_shaders;