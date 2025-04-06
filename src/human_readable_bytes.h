#pragma once

#include <iostream>
#include <iomanip>
#include <cstdint>

class human_bytes {
public:
	explicit constexpr human_bytes(size_t bytes) : bytes_(bytes) {}

	friend std::ostream& operator<<(std::ostream& os, const human_bytes& hrb) {
		static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};
		static const int unitCount = sizeof(units) / sizeof(units[0]);

		double size = static_cast<double>(hrb.bytes_);
		int unit = 0;

		while (size >= 1024.0 && unit < unitCount - 1) {
			size /= 1024.0;
			++unit;
		}

		int precision = (size < 10.0) ? 2 : (size < 100.0) ? 1 : 0;
		auto old_flags = os.flags();
		auto old_precision = os.precision();

		os << std::fixed << std::setprecision(precision) << size << ' ' << units[unit];

		os.flags(old_flags);
		os.precision(old_precision);
		return os;
	}

private:
	size_t bytes_;
};
