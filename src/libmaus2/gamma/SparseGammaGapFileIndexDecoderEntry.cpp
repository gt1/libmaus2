#include <libmaus2/gamma/SparseGammaGapFileIndexDecoderEntry.hpp>

std::ostream & libmaus2::gamma::operator<<(std::ostream & out, libmaus2::gamma::SparseGammaGapFileIndexDecoderEntry const & S)
{
	out << "SparseGammaGapFileIndexDecoderEntry(" << S.ikey << "," << S.ibitoff << ")";
	return out;
}
