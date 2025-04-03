#include "packet_socket.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>


#include <iostream>
#include <array>
#include <memory>
#include <cstring>


packet_socket::packet_socket(int socket_type, int protocol) {
	m_fd = socket(AF_PACKET, socket_type, htons(protocol));
	if (m_fd < 0) {
		throw std::runtime_error("Error creating socket: " + std::string(strerror(errno)));
	}
}

packet_socket::~packet_socket() {
	if (m_fd >= 0) {
		close(m_fd);
	}
}