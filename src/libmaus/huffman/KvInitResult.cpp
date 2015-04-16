#include <libmaus/huffman/KvInitResult.hpp>

::libmaus::huffman::KvInitResult::KvInitResult()
: koffset(0), voffset(0), kvoffset(0), kvtarget(0)
{
}

std::ostream & operator<<(std::ostream & out, ::libmaus::huffman::KvInitResult const & KIR)
{
	out << "KvInitResult("
		<< "koffset=" << KIR.koffset
		<< ",voffset=" << KIR.voffset
		<< ",kvoffset=" << KIR.kvoffset
		<< ",kvtarget=" << KIR.kvtarget
		<< ")";
	return out;
}

