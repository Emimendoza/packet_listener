#pragma once
#include "packet_socket.h"
#include <memory>
#include <linux/if_packet.h>
#include <unordered_map>
#include <netinet/in.h>
#include <ctime>
#include <netinet/udp.h>
#include <fstream>
#include <atomic>
#include <thread>
#include <mutex>

// TODO: Support IPv6
typedef in_addr_t addr_t;

struct udp_usage_t {
	addr_t addr;
	std::time_t last_seen;
};

class packet_listener {
public:
	explicit packet_listener(const std::string& event_log = "event_log.txt");
	~packet_listener() = default;
	packet_listener(const packet_listener&) = delete;
	packet_listener(packet_listener&&) = delete;

	void listen();
	void console(std::istream& in, std::ostream& out);
	void stop();
private:
	void _listen();
	void process_raw();
	void process_ipv4();
	void process_ipv6();

	void process_udp(const udphdr& header);

	std::ofstream m_event_log;
	std::atomic_flag m_stop_flag = ATOMIC_FLAG_INIT;
	std::thread m_thread;
	std::unique_ptr<packet_socket> m_socket;
	std::mutex m_usage_mutex;
	std::unordered_map<addr_t, size_t> m_usage_recv{};
	std::unordered_map<addr_t, size_t> m_usage_sent{};
	// Used for detecting UDP hole punching
	std::unordered_map<uint16_t, udp_usage_t> m_udp_usage{};
	ssize_t m_bytes_received{};
	addr_t m_remote{};
	bool m_outgoing{};
	sockaddr_ll m_addr{};
	std::array<unsigned char, 16384> m_buffer{};

	// Number of seconds between UDP connections for it to be considered a hole punch
	static constexpr std::time_t UDP_HOLE_PUNCH_TIMEOUT = 5;
};

