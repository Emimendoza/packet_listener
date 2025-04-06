#pragma once

#include <linux/filter.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>


namespace packet_filters {

static constexpr sock_filter filter_localhost[]{

};
}