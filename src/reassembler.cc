#include "reassembler.hh"
#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
	if(window.empty()) {
		window.resize(output.size(), 0); 
		flags.resize(output.size(), false);
		capacity_ = output.size();
	}
	uint64_t capacity = output.available_capacity();
	if(first_index >= window_first_index + capacity) { return; }
	if(!(data.empty() && is_last_substring) && first_index + data.size() <= window_first_index) { return; }
	if(is_last_substring && window_first_index + capacity >= first_index + data.size()) { last_in = true;}
	uint64_t begin_index = first_index;
	uint64_t end_index = first_index + data.size();
	if(begin_index < window_first_index) { begin_index = window_first_index; }
	if(end_index > window_first_index + capacity) { end_index = window_first_index + capacity; }
	for(uint64_t i = begin_index;i < end_index;i++) {
		window[(begin + i - window_first_index)%capacity_] = data[i - first_index];
		if(!flags[(begin + i - window_first_index)%capacity_]) { window_size ++;}
		flags[(begin + i - window_first_index)%capacity_] = true;
	}
	string push_data = "";
	while(flags[begin]) {
	 push_data += window[begin]; 
	//	output.push(string(1,window[begin]));
		flags[begin] = false;
		begin = (begin + 1) % capacity_;
	//	window_first_index++;
	//	window_size--;
	}
	window_first_index += push_data.size();
	window_size -= push_data.size();
	if(!push_data.empty()) {
		output.push(push_data);
	}
	if(last_in && window_size == 0) { output.close(); }
  (void)first_index;
  (void)data;
  (void)is_last_substring;
  (void)output;
}

uint64_t Reassembler::bytes_pending() const
{
  return window_size;
	
}
