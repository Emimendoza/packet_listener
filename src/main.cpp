#include "packet_listener.h"

#include <iostream>

int main () {
	packet_listener listener;
	listener.listen();
	// Attach console
	listener.console(std::cin, std::cout);
}