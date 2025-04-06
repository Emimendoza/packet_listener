#include "packet_socket.h"
#include <memory>

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>
#include <iomanip>
#include <arpa/inet.h>

int main () {
	auto socket = std::make_unique<packet_socket>(SOCK_DGRAM, ETH_P_IP);
	const in_addr_t localhost = {htonl(INADDR_LOOPBACK)};
	iphdr

	while (true) {
		std::array<unsigned char, 4096> buffer{};
		ip& ip_header = *reinterpret_cast<ip*>(buffer.data());
		ssize_t bytes_received = recv(**socket, buffer.data(), buffer.size(), MSG_TRUNC);
		if (ip_header.ip_dst.s_addr == localhost){
			continue;
		}
		std::cout << "Received " << bytes_received << " bytes, from: " << inet_ntoa(ip_header.ip_src) << " to: " << inet_ntoa(ip_header.ip_dst) << std::endl;
	}
}