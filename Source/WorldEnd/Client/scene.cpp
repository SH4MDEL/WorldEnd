#include "scene.h"

unordered_map<string, shared_ptr<Shader>>		Scene::m_shaders;
unordered_map<string, shared_ptr<Mesh>>			Scene::m_meshs;
unordered_map<string, shared_ptr<Texture>>		Scene::m_textures;
unordered_map<string, shared_ptr<Materials>>	Scene::m_materials;
unordered_map<string, shared_ptr<AnimationSet>>	Scene::m_animationSets;
unordered_map<INT, shared_ptr<Player>>	       	Scene::m_multiPlayers;
unordered_map<INT, INT>						    Scene::m_idSet;

void Scene::RecvPacket()
{
	char buf[BUF_SIZE] = { 0 };
	WSABUF wsabuf{ BUF_SIZE, buf };
	DWORD recv_byte{ 0 }, recv_flag{ 0 };

	int retval = WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr);
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
		ErrorDisplay("RecvSizeType");
	}

	if (recv_byte > 0) {
		PacketReassembly(wsabuf.buf, recv_byte);
	}
}

void Scene::PacketReassembly(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t make_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];
	ZeroMemory(packet_buffer, BUF_SIZE);

	while (io_byte != 0) {
		if (make_packet_size == 0)
			make_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= make_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, make_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += make_packet_size - saved_packet_size;
			io_byte -= make_packet_size - saved_packet_size;
			//cout << "io byte - " << io_byte << endl;
			make_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}
