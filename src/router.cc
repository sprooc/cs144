#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's
// destination address against prefix_length: For this route to be applicable,
// how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the
//    datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is
// directly attached to the router (in
//    which case, the next hop address should be the datagram's final
//    destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix, const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
  cerr << "DEBUG: adding route "
       << Address::from_ipv4_numeric(route_prefix).ip() << "/"
       << static_cast<int>(prefix_length) << " => "
       << (next_hop.has_value() ? next_hop->ip() : "(direct)")
       << " on interface " << interface_num << "\n";

  route_table.add(route_prefix, prefix_length, next_hop, interface_num);
}

void Router::route() {
  for (AsyncNetworkInterface& _interface : interfaces_) {
    optional<InternetDatagram> datagram;
    while ((datagram = _interface.maybe_receive()).has_value()) {
      if (datagram.value().header.ttl-- <= 1) {
        return;
      }
      datagram.value().header.compute_checksum();
      TrieNode* route_node =
          route_table.find_match(datagram.value().header.dst);
      if (route_node) {
        interface(route_node->interface_num)
            .send_datagram(
                datagram.value(),
                route_node->next_hop.has_value()
                    ? route_node->next_hop.value()
                    : Address::from_ipv4_numeric(datagram.value().header.dst));
      }
    }
  }
}

uint8_t match_length(uint32_t add1, uint32_t add2) {
  uint32_t and_result = add1 ^ add2;
  uint8_t length = 0;
  while (!(and_result & ((uint32_t)1 << 31))) {
    length++;
    and_result <<= 1;
  }
  cout << "lenth : " << (uint32_t)length << endl;
  return length;
}
