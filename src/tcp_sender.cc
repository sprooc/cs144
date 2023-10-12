#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{
	RTO = initial_RTO_ms_;
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return sequence_numbers;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmission_time;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
	if(ready_segments.empty()) { return nullopt; }
	TCPSenderMessage msg = ready_segments.front();
	ready_segments.pop_front();
	if(retransmission_time == 0 && (receiver_window_size > 0 || (receiver_window_size == 0 && outstanding_segments.size() == 0))) { outstanding_segments.push_back(msg); }
	timer_on = true;
  return msg;	
}

void TCPSender::push( Reader& outbound_stream )
{
	if(has_fin) { return;}
  while((receiver_window_size == 0 && !has_extra_send) || ((outbound_stream.is_finished() || sending_number == 0 || outbound_stream.bytes_buffered()) && receiver_window_size > sequence_numbers)) {
	 uint32_t max_len = TCPConfig::MAX_PAYLOAD_SIZE < (receiver_window_size - sequence_numbers) ? TCPConfig::MAX_PAYLOAD_SIZE : (receiver_window_size - sequence_numbers);
		if(receiver_window_size == 0) { max_len = 1; has_extra_send = true; }
		TCPSenderMessage msg;
		string str;
		read(outbound_stream, max_len,str);
		msg.payload = Buffer(str);
		msg.SYN = sending_number == 0;
	  if(msg.sequence_length() < max_len || max_len == TCPConfig::MAX_PAYLOAD_SIZE) {	msg.FIN = outbound_stream.is_finished(); has_fin = msg.FIN;}
		msg.seqno = Wrap32::wrap(sending_number, isn_);
		sending_number += msg.sequence_length();
		ready_segments.push_back(msg);
		sequence_numbers += msg.sequence_length();
		if(sending_number == 0 || msg.FIN || !receiver_window_size) { return; }
	}
}

TCPSenderMessage TCPSender::send_empty_message() const
{
	TCPSenderMessage msg {};
	msg.seqno = Wrap32::wrap(sending_number, isn_);
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
	cout << " in " << endl;
	if(msg.ackno.has_value() && msg.ackno.value().unwrap(isn_, sending_number) > sending_number) { return; }
	receiver_window_size = msg.window_size;
	has_extra_send = false;
	if(!msg.ackno.has_value()) { return; }
	if(msg.ackno.value().unwrap(isn_, sending_number) > outstanding_number) {
		RTO = initial_RTO_ms_;
		retransmission_time = 0;
		timer = 0;
		timer_on = !outstanding_segments.empty();
	}
	cout << outstanding_segments.empty() << " " << outstanding_number << endl;
	while(!outstanding_segments.empty() && outstanding_number + outstanding_segments.front().sequence_length() <= msg.ackno.value().unwrap(isn_, sending_number)) {
		int pop_len = outstanding_segments.front().sequence_length();
	  outstanding_number += pop_len;
	  sequence_numbers -= pop_len;
	cout << "out" << pop_len << outstanding_segments.size() <<  endl;
	  outstanding_segments.pop_front();
		}
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
	if(timer_on) {
		timer += ms_since_last_tick;
		if(timer >= RTO) {
			timer = 0;
			if(receiver_window_size > 0) {
			  retransmission_time += 1;
			  RTO *= 2;
			}
			if(!outstanding_segments.empty()) {
				ready_segments.push_front(outstanding_segments.front());
			}
		}
	}
}
