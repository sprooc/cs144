#include "tcp_receiver.hh"
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
	if(message.SYN) {
		zero_point = message.seqno;
		message.seqno = message.seqno + 1u;
		has_syn = true;
	}
	reassembler.insert(message.seqno.unwrap(zero_point, inbound_stream.bytes_pushed()) - 1, string(message.payload), message.FIN, inbound_stream);	
  (void)message;
  (void)reassembler;
  (void)inbound_stream;
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
	TCPReceiverMessage message;
	if(has_syn) {
	message.ackno = Wrap32::wrap(inbound_stream.bytes_pushed() + 1u + inbound_stream.is_closed(), zero_point);
}
  uint64_t capa = inbound_stream.available_capacity();
	message.window_size = capa < UINT16_MAX ? capa : UINT16_MAX;	
  return message;	
  (void)inbound_stream;
  return {};
}
