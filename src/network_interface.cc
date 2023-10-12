#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
	auto ad_map = address_table.find(next_hop.ipv4_numeric());
	if(ad_map != address_table.end()) {
		EthernetFrame frame;
		frame.header.dst = ad_map->second.first;
		frame.header.src = ethernet_address_;
		frame.header.type = EthernetHeader::TYPE_IPv4;
	  frame.payload = serialize(dgram);
	  ready_frame.push(frame);
	} else {
		if(sending_arp.find(next_hop.ipv4_numeric()) == sending_arp.end()) { 
			EthernetFrame arp_frame;
		  ARPMessage arp_msg;
		  arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
	    arp_msg.sender_ethernet_address = ethernet_address_;
	    arp_msg.sender_ip_address = ip_address_.ipv4_numeric();
	    arp_msg.target_ip_address = next_hop.ipv4_numeric();
	    arp_frame.header.dst = ETHERNET_BROADCAST;
	    arp_frame.header.src = ethernet_address_;
	    arp_frame.header.type = EthernetHeader::TYPE_ARP;
	    arp_frame.payload = serialize(arp_msg);
	    ready_frame.push(arp_frame);
			sending_arp[next_hop.ipv4_numeric()] = timer;
		}
	  EthernetFrame frame;
		frame.header.src = ethernet_address_;
		frame.header.type = EthernetHeader::TYPE_IPv4;
		frame.payload = serialize(dgram);
		auto wait_frames = wait_arp_frame.find(next_hop.ipv4_numeric());
		if(wait_frames == wait_arp_frame.end()) {
			list<EthernetFrame> frames;
			frames.push_back(frame);
			wait_arp_frame[next_hop.ipv4_numeric()] = frames;
		} else {
			wait_frames->second.push_back(frame);
		}
	}
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
	if(frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_) {
		return nullopt;
	}
	if(frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram datagram;
		parse(datagram, frame.payload);
		return datagram;
	} else if(frame.header.type == EthernetHeader::TYPE_ARP) {
		ARPMessage msg;
		parse(msg, frame.payload);
		if(msg.opcode == ARPMessage::OPCODE_REQUEST) {
			address_table[msg.sender_ip_address] = pair<EthernetAddress, size_t>(msg.sender_ethernet_address, timer);
			msg.opcode = ARPMessage::OPCODE_REPLY;
			bool has_find = false;
			if(msg.target_ip_address == ip_address_.ipv4_numeric()) {
				msg.target_ethernet_address = ethernet_address_;
				has_find = true;
			} else {
				for(auto iter: address_table) {
					if(iter.first == msg.target_ip_address) {
						msg.target_ethernet_address = iter.second.first;
						has_find  = true;
						break;
					}
				}
			}
			if(has_find) {
				uint32_t ip_t = msg.target_ip_address;
				msg.target_ip_address = msg.sender_ip_address;
				msg.sender_ip_address = ip_t;
				EthernetAddress ad_t = msg.target_ethernet_address;
				msg.target_ethernet_address = msg.sender_ethernet_address;
				msg.sender_ethernet_address = ad_t;
				EthernetFrame send_frame;
				send_frame.header.dst = msg.target_ethernet_address;
				send_frame.header.src = ethernet_address_;
				send_frame.header.type = EthernetHeader::TYPE_ARP;
				send_frame.payload = serialize(msg);
				ready_frame.push(send_frame);
			}
		} else {
			EthernetAddress new_ethernet_address = msg.sender_ethernet_address;
      uint32_t new_ip_address = msg.sender_ip_address;
			address_table[new_ip_address] = pair<EthernetAddress, size_t>(new_ethernet_address, timer);
			sending_arp.erase(new_ip_address);
			auto frames = wait_arp_frame.find(new_ip_address);
			if(frames != wait_arp_frame.end()) {
				for(auto iter: frames->second) {
					iter.header.dst = new_ethernet_address;
					ready_frame.push(iter);
				}
				wait_arp_frame.erase(frames);
			}
		}
	}
	return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
	timer += ms_since_last_tick;;
	//pop sending_arp 5s
	//pop address_table 30s
	for(auto iter = sending_arp.begin(); iter != sending_arp.end();) {
		if(timer - iter->second >= 5000) {
			sending_arp.erase(iter++);
		} else {
			iter++;
		}
	}
  for(auto iter = address_table.begin(); iter != address_table.end();) {
		if(timer - iter->second.second >= 30000) {
			address_table.erase(iter++);
		} else {
			iter++;
		}
	}

}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
	if(ready_frame.empty()) {
		return nullopt;
	} else {
		EthernetFrame frame = ready_frame.front();
		ready_frame.pop();
		return frame;
	}
}
