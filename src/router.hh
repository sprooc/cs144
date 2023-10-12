#pragma once

#include <iostream>
#include <optional>
#include <queue>

#include "network_interface.hh"
uint8_t match_length(uint32_t add1, uint32_t add_2);

struct TrieNode {
  TrieNode* zero{};
  TrieNode* one{};
  uint32_t route_prefix = 0;
  uint8_t prefix_length = 0;
  std::optional<Address> next_hop{};
  size_t interface_num = -1ul;

  TrieNode() {}

  TrieNode(uint32_t p, uint8_t l, std::optional<Address> nh, size_t n)
      : route_prefix(p), prefix_length(l), next_hop(nh), interface_num(n) {}

  TrieNode(const TrieNode& other)
      : zero(other.zero),
        one(other.one),
        route_prefix(other.route_prefix),
        prefix_length(other.prefix_length),
        next_hop(other.next_hop),
        interface_num(other.interface_num) {}

  TrieNode& operator=(const TrieNode&) { return *this; }

  ~TrieNode() {
    if (zero) delete zero;
    zero = nullptr;
    if (one) delete one;
    one = nullptr;
  }

  TrieNode* add(TrieNode* new_node, int level, int min_level) {
    if (!empty()) {
      TrieNode* old_node = new TrieNode(*this);
      clean();
      uint32_t m_len = match_length(route_prefix, new_node->route_prefix);
      min_level = m_len - level - 1;
      // std::cout << m_len << " " << level << std::endl;
      add(old_node, level, m_len - level);
    }
    TrieNode*& node =
        ((new_node->route_prefix >> (30 - level)) & 1) ? one : zero;
    if (!node && min_level <= 0) {
      node = new_node;
      new_node->one = nullptr;
      new_node->zero = nullptr;
    } else if (node) {
      node->add(new_node, level + 1, min_level - 1);
    } else {
      node = new TrieNode();
      node->add(new_node, level + 1, min_level - 1);
    }
    return this;
  }

  TrieNode* find(uint32_t addr, int level) {
    if (!empty()) {
      std::cout << "in " << std::endl;
      return this;
    }
    TrieNode* node = (addr & 1 << (30 - level)) ? one : zero;
    std::cout << "find " << Address::from_ipv4_numeric(addr).to_string() << " "
              << (addr & 1 << (30 - level));
    if (!node) {
      std::cout << " null" << std::endl;
      return nullptr;
    }

    return node->find(addr, level + 1);
  }
  bool empty() { return interface_num == -1ul; }
  void clean() { interface_num = -1ul; }
};

class Trie {
 private:
  TrieNode* root = new TrieNode();

 public:
  ~Trie() { delete root; }

  void add(uint32_t route_prefix, uint8_t prefix_length,
           std::optional<Address> next_hop, size_t interface_num) {
    root->add(
        new TrieNode(route_prefix, prefix_length, next_hop, interface_num), -1,
        3);
  }

  TrieNode* find_match(uint32_t dst) const {
    TrieNode* node = root->find(dst, -1);
    if (!node) {
      return nullptr;
    }
    std::cout << "find"
              << Address::from_ipv4_numeric(node->route_prefix).to_string()
              << std::endl;
    uint8_t m_len = match_length(dst, node->route_prefix);
    if (m_len >= node->prefix_length) {
      return node;
    } else {
      return nullptr;
    }
  }
};
// A wrapper for NetworkInterface that makes the host-side
// interface asynchronous: instead of returning received datagrams
// immediately (from the `recv_frame` method), it stores them for
// later retrieval. Otherwise, behaves identically to the underlying
// implementation of NetworkInterface.
class AsyncNetworkInterface : public NetworkInterface {
  std::queue<InternetDatagram> datagrams_in_{};

 public:
  using NetworkInterface::NetworkInterface;

  // Construct from a NetworkInterface
  explicit AsyncNetworkInterface(NetworkInterface&& interface)
      : NetworkInterface(interface) {}

  // \brief Receives and Ethernet frame and responds appropriately.

  // - If type is IPv4, pushes to the `datagrams_out` queue for later retrieval
  // by the owner.
  // - If type is ARP request, learn a mapping from the "sender" fields, and
  // send an ARP reply.
  // - If type is ARP reply, learn a mapping from the "target" fields.
  //
  // \param[in] frame the incoming Ethernet frame
  void recv_frame(const EthernetFrame& frame) {
    auto optional_dgram = NetworkInterface::recv_frame(frame);
    if (optional_dgram.has_value()) {
      datagrams_in_.push(std::move(optional_dgram.value()));
    }
  };

  // Access queue of Internet datagrams that have been received
  std::optional<InternetDatagram> maybe_receive() {
    if (datagrams_in_.empty()) {
      return {};
    }

    InternetDatagram datagram = std::move(datagrams_in_.front());
    datagrams_in_.pop();
    return datagram;
  }
};

// A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router {
  // The router's collection of network interfaces
  std::vector<AsyncNetworkInterface> interfaces_{};
  Trie route_table{};

 public:
  // Add an interface to the router
  // interface: an already-constructed network interface
  // returns the index of the interface after it has been added to the router
  size_t add_interface(AsyncNetworkInterface&& interface) {
    interfaces_.push_back(std::move(interface));
    return interfaces_.size() - 1;
  }

  // Access an interface by index
  AsyncNetworkInterface& interface(size_t N) { return interfaces_.at(N); }

  // Add a route (a forwarding rule)
  void add_route(uint32_t route_prefix, uint8_t prefix_length,
                 std::optional<Address> next_hop, size_t interface_num);

  // Route packets between the interfaces. For each interface, use the
  // maybe_receive() method to consume every incoming datagram and
  // send it on one of interfaces to the correct next hop. The router
  // chooses the outbound interface and next-hop as specified by the
  // route with the longest prefix_length that matches the datagram's
  // destination address.
  void route();
};
