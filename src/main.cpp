#include "packet_socket.h"
#include <memory>

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>
#include <iomanip>

int main () {
	auto socket = std::make_unique<packet_socket>(SOCK_DGRAM, ETH_P_ALL);
	while (true) {
		sockaddr_ll addr{};
		socklen_t addr_len = sizeof(addr);
		std::array<unsigned char, 256> buffer{};
		ssize_t bytes_received = recvfrom(**socket, buffer.data(), buffer.size(), MSG_TRUNC, reinterpret_cast<sockaddr*>(&addr), &addr_len);
		std::cout << "Received " << bytes_received << " bytes with protocol " << std::hex << std::setw(4) << std::setfill('0') << ntohs(addr.sll_protocol) << " from interface " << static_cast<int>(addr.sll_ifindex) << std::dec << '\n';

	}
}