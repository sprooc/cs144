#include "wrapping_integers.hh"
#include <iostream>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  (void)n;
  (void)zero_point;
  return zero_point + (uint32_t)n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
	uint32_t zero = zero_point.raw_value_;
	uint32_t abs_val = raw_value_ >= zero ? raw_value_ - zero : ~0 - zero + raw_value_ + 1;
  int t = checkpoint / (1ul << 32);
	uint64_t uw1 = t * (1ul << 32) + abs_val;
	uint64_t diff1 = uw1 > checkpoint ? uw1 - checkpoint : checkpoint - uw1;
	uint64_t uw2 = (t + 1) * (1ul << 32) + abs_val;
	uint64_t diff2 = uw2 - checkpoint;
	uint64_t uw3 = (t - 1) * (1ul << 32) + abs_val;
	uint64_t diff3 = checkpoint - uw3;
	if(t == 0) { diff3 = diff1;}
	if(t == 1 << 31){ diff2 = diff1; }
	if(diff2 < diff1) {diff1 = diff2; uw1 = uw2; }
	if(diff3 < diff1) {diff1 = diff3; uw1 = uw3; }
	return uw1;
  (void)zero_point;
  (void)checkpoint;
}
