#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if(closed) return;
  data.erase(data.begin() + available_capacity(), data.end());
  buffer.push(data);
  buffer_size += data.size();
  push_bytes += date.size();
  (void)data;
  return;
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
  string_view str_v(buffer.front(), 1);
  return str_v;
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
    string& bf = buffer.front();
    uint64_t fs_len = bf.size();
    if(fs_len <= len) {
      buffer.pop();
      len -= fs_len;
      buffer_size -= fs_len;
      pop_size += fs_len;
    } else {
      bf.erase(bf.begin(), bf.begin() + len);
      buffer_size -= len;
      pop_size += len;
      len = 0;
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
