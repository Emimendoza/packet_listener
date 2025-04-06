#include "packet_listener.h"
#include "human_readable_bytes.h"
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <vector>


packet_listener::packet_listener(const std::string& event_log) : m_event_log(event_log, std::ios::app) {
	if (!m_event_log.is_open()) {
		std::cerr << "Error opening event log file: " << event_log << std::endl;
		throw std::runtime_error("Error opening event log file");
	}
}

void packet_listener::listen() {
	m_thread = std::thread(&packet_listener::_listen, this);
}

void packet_listener::stop() {
	m_stop_flag.test_and_set();
	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void packet_listener::_listen() {
	static socklen_t addr_len = sizeof(m_addr);
	m_socket = std::make_unique<packet_socket>(SOCK_DGRAM, ETH_P_ALL);

	auto now = std::time(nullptr);
	m_event_log << "Packet listener started at " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << '\n';
	while (!m_stop_flag.test()) {
		m_bytes_received = recvfrom(**m_socket, m_buffer.data(), m_buffer.size(), MSG_TRUNC, reinterpret_cast<sockaddr*>(&m_addr), &addr_len);
		if (m_bytes_received < 0) {
			std::cerr << "Error receiving packet: " << std::strerror(errno) << std::endl;
			continue;
		}
		process_raw();
	}
	now = std::time(nullptr);
	m_event_log << "Packet listener stopped at " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << '\n';
}

void packet_listener::process_raw() {
	switch(m_addr.sll_pkttype) {
	case PACKET_OUTGOING:
		m_outgoing = true;
		break;
	case PACKET_HOST:
	case PACKET_OTHERHOST:
		m_outgoing = false;
		break;
	default:
		return;
	}
	switch(ntohs(m_addr.sll_protocol)){
	case ETH_P_IP:
		process_ipv4();
		break;
	case ETH_P_IPV6:
		process_ipv6();
		break;
	default:
		break;
	}
}
static const in_addr_t localhost_v4 = {htonl(INADDR_LOOPBACK)};
static constexpr in6_addr localhost_v6 = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};

void packet_listener::process_ipv4() {
	ip& ip_header = *reinterpret_cast<ip*>(m_buffer.data());
	if (ip_header.ip_dst.s_addr == localhost_v4) {
		return; // Filter out localhost packets
	}
	if (m_outgoing) {
		m_remote = ip_header.ip_dst.s_addr;
	} else {
		m_remote = ip_header.ip_src.s_addr;
	}
	{
		std::lock_guard<std::mutex> lock(m_usage_mutex);
		if (m_outgoing) {
			m_usage_sent[m_remote] += m_bytes_received;
		} else {
			m_usage_recv[m_remote] += m_bytes_received;
		}
	}
	// If UDP, process UDP header
	if (ip_header.ip_p == IPPROTO_UDP) {
		process_udp(*reinterpret_cast<udphdr*>(m_buffer.data() + sizeof(ip)));
	}
}

void packet_listener::process_udp(const udphdr &header) {
	uint16_t local_port;
	if (m_outgoing) {
		local_port = header.uh_sport;
	} else {
		local_port = header.uh_dport;
	}
	if (m_udp_usage.find(local_port) == m_udp_usage.end()) [[unlikely]] {
		m_udp_usage[local_port] = {m_remote, std::time(nullptr)};
	}
	else {
		auto& udp_usage = m_udp_usage[local_port];
		if (udp_usage.addr == m_remote) {
			udp_usage.last_seen = std::time(nullptr);
		} else {
			std::time_t now = std::time(nullptr);
			if ((now - udp_usage.last_seen) < UDP_HOLE_PUNCH_TIMEOUT) {
				char first_addr_str[INET_ADDRSTRLEN];
				char second_addr_str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &udp_usage.addr, first_addr_str, sizeof(first_addr_str));
				inet_ntop(AF_INET, &m_remote, second_addr_str, sizeof(second_addr_str));
				m_event_log << "UDP hole punch detected on port " << htons(local_port) << ", first " << first_addr_str << ", second " << second_addr_str << std::endl;
			}
			udp_usage.addr = m_remote;
			udp_usage.last_seen = now;
		}
	}
}

void packet_listener::process_ipv6() {
	// TODO: Implement IPv6 packet processing
	return;
	ip6_hdr& ipv6_header = *reinterpret_cast<ip6_hdr*>(m_buffer.data());
	if (memcmp(&ipv6_header.ip6_dst, &localhost_v6, sizeof(localhost_v6)) == 0) {
		return; // Filter out localhost packets
	}
}

void packet_listener::console(std::istream &in, std::ostream &out) {
	typeof(m_usage_recv) usage_copy_recv;
	typeof(m_usage_sent) usage_copy_sent;
	typeof(m_usage_recv) usage_bandwidth;
	std::vector<std::pair<addr_t, size_t>> sorted_usage;

	auto copy_usage = [this, &usage_copy_recv, &usage_copy_sent](){
		std::lock_guard<std::mutex> lock(m_usage_mutex);
		usage_copy_recv = m_usage_recv;
		usage_copy_sent = m_usage_sent;
	};

	out << "Packet listener started. Type 'exit' to stop, 'h' for help" << std::endl;
	while (true) {
		out << "> " << std::flush;
		std::string command;
		in >> command;
		if (command == "exit") {
			stop();
			return;
		}
		if (command == "h") {
			out << " Available commands:\n";
			out << "  exit - Stop the packet listener\n";
			out << "  stats [n] - Show the top n IP addresses by bytes received/sent (default 10)\n";
			out << "  h - Show this help message" << std::endl;
			continue;
		}
		if (command == "stats") {
			size_t n = 10;
			if (in.peek() != '\n') {
				if (!(in >> n)) {
					in.clear();
					in >> command;
					out << "Invalid number: " << command << std::endl;
					continue;
				}
			}
			copy_usage();
			usage_bandwidth = usage_copy_recv;
			for (auto& [addr, bytes] : usage_copy_recv) {
				if (usage_copy_sent.find(addr) != usage_copy_sent.end()) {
					usage_bandwidth[addr] += usage_copy_sent[addr];
					continue;
				}
				usage_bandwidth[addr] = bytes;
			}
			sorted_usage.clear();
			for (const auto& [addr, bytes] : usage_bandwidth) {
				sorted_usage.emplace_back(addr, bytes);
			}
			std::sort(sorted_usage.begin(), sorted_usage.end(), [](const auto& a, const auto& b) {
				return a.second > b.second;
			});
			out << "Top " << n << " IP addresses by bytes received/sent:\n";
			for (size_t i = 0; i < n && i < sorted_usage.size(); ++i) {
				char addr_str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &sorted_usage[i].first, addr_str, sizeof(addr_str));
				out << "  " << addr_str << ": " << human_bytes(sorted_usage[i].second) << " Total, ";
				if (usage_copy_recv.find(sorted_usage[i].first) != usage_copy_recv.end()) {
					out << human_bytes(usage_copy_recv[sorted_usage[i].first]) << " Received, ";
				} else {
					out << "0 Received, ";
				}
				if (usage_copy_sent.find(sorted_usage[i].first) != usage_copy_sent.end()) {
					out << human_bytes(usage_copy_sent[sorted_usage[i].first]) << " Sent";
				} else {
					out << "0 Sent";
				}
				out << '\n';
			}
			out.flush();
			continue;
		}
		out << "Unknown command: " << command << std::endl;
	}
}
