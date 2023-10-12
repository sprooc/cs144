#include "byte_stream.hh"
#include <iostream>
#include <stdexcept>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if(closed) { return; }
	uint64_t size = data.size() < available_capacity() ? data.size() : available_capacity();
	if(size == 0) { return; }
	buffer.push(data);
	buffer_view.push(string_view(buffer.back().begin(), buffer.back().begin() + size));
  buffer_size += size;
  push_bytes += size;
  (void)data;
}

void Writer::close()
{
  closed = true;

}

void Writer::set_error()
{
  error = true;
}

bool Writer::is_closed() const
{
  return closed;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_size;;
}

uint64_t Writer::bytes_pushed() const
{
  return push_bytes;
}

string_view Reader::peek() const
{
  return buffer_view.front();
}

bool Reader::is_finished() const
{
  return closed && push_bytes == pop_bytes;
}

bool Reader::has_error() const
{
  return error;
}

void Reader::pop( uint64_t len )
{
  while(len > 0) {
    string_view& bvf = buffer_view.front();
    const uint64_t fs_len = bvf.size();
    if(fs_len <= len) {
      buffer.pop();
			buffer_view.pop();
      len -= fs_len;
      buffer_size -= fs_len;
      pop_bytes += fs_len;
    } else {
			bvf.remove_prefix(len);
      buffer_size -= len;
      pop_bytes += len;
      len = 0;
			if(bvf.empty()) {
			 	buffer.pop();
				buffer_view.pop();
			}
    }
  }
 

  (void)len;
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_size;
}

uint64_t Reader::bytes_popped() const
{
  return pop_bytes;
}
