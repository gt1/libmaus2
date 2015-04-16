#include <libmaus2/util/SuccinctBorderArray.hpp>

std::ostream & operator<<(std::ostream & out, libmaus2::util::SuccinctBorderArray const & S)
{
	for ( uint64_t i = 0; i < S.size(); ++i )
		out << i << "\t" << S[i] << std::endl;
	
	return out;
}
