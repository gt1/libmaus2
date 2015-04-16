#include <libmaus/gamma/SparseGammaGapFileIndexDecoderEntry.hpp>

std::ostream & libmaus::gamma::operator<<(std::ostream & out, libmaus::gamma::SparseGammaGapFileIndexDecoderEntry const & S)
{
	out << "SparseGammaGapFileIndexDecoderEntry(" << S.ikey << "," << S.ibitoff << ")";
	return out;
}
