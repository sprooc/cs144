#pragma once
#include <deque>
#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
using std::deque;
class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
private:
	bool has_extra_send = false;
	bool has_fin = false;
  deque<TCPSenderMessage> outstanding_segments {};	
	deque<TCPSenderMessage> ready_segments {};
	uint64_t timer = 0;
	bool timer_on = false;
  uint64_t outstanding_number = 0;
	uint16_t receiver_window_size = 1;
	uint64_t RTO = 0;
  uint64_t sequence_numbers = 0;
	uint64_t retransmission_time = 0;
	Reader reader {ByteStream{0}};
	uint64_t sending_number = 0;
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
