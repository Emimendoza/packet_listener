#pragma once

class packet_socket {
public:
	packet_socket(int socket_type, int protocol);
	~packet_socket();

	packet_socket(const packet_socket&) = delete;
	packet_socket(packet_socket&&) = delete;

	constexpr int operator* () const { return m_fd; }
private:
	int m_fd;
};