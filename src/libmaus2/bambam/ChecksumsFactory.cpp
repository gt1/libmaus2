/*
    libmaus2
    Copyright (C) 2014-2014 David K. Jackson
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <libmaus2/bambam/ChecksumsFactory.hpp>
#include <libmaus2/bambam/SeqChksumUpdateContext.hpp>
#include <libmaus2/bambam/SeqChksumPrimeNumbers.hpp>

#if defined(LIBMAUS2_HAVE_GMP)
#include <gmp.h>
#endif

template<typename _context_type>
struct SeqChksumsSimpleSums
{
	typedef _context_type context_type;
	typedef SeqChksumsSimpleSums<context_type> this_type;

	private:
	uint64_t count;
	libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> b_seq;
	libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> name_b_seq;
	libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> b_seq_qual;
	libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> b_seq_tags;

	public:
	void push(context_type const & context)
	{
		count += 1;
		b_seq += context.flags_seq_digest;
		name_b_seq += context.name_flags_seq_digest;
		b_seq_qual += context.flags_seq_qual_digest;
		b_seq_tags += context.flags_seq_tags_digest;
	}

	void push (this_type const & subsetproducts)
	{
		count += subsetproducts.count;
		b_seq += subsetproducts.b_seq;
		name_b_seq += subsetproducts.name_b_seq;
		b_seq_qual += subsetproducts.b_seq_qual;
		b_seq_tags += subsetproducts.b_seq_tags;
	};

	bool operator== (this_type const & other) const
	{
		return count==other.count &&
			b_seq==other.b_seq && name_b_seq==other.name_b_seq &&
			b_seq_qual==other.b_seq_qual && b_seq_tags==other.b_seq_tags;
	};

	void get_b_seq(std::vector<uint8_t> & H) const
	{
		b_seq.getHexVector(H);
	}

	void get_name_b_seq(std::vector<uint8_t> & H) const
	{
		name_b_seq.getHexVector(H);
	}

	void get_b_seq_qual(std::vector<uint8_t> & H) const
	{
		b_seq_qual.getHexVector(H);
	}

	void get_b_seq_tags(std::vector<uint8_t> & H) const
	{
		b_seq_tags.getHexVector(H);
	}

	std::string get_b_seq() const
	{
		std::ostringstream ostr;
		ostr << std::hex << b_seq;
		return ostr.str();
	}

	std::string get_name_b_seq() const
	{
		std::ostringstream ostr;
		ostr << std::hex << name_b_seq;
		return ostr.str();
	}

	std::string get_b_seq_qual() const
	{
		std::ostringstream ostr;
		ostr << std::hex << b_seq_qual;
		return ostr.str();
	}

	std::string get_b_seq_tags() const
	{
		std::ostringstream ostr;
		ostr << std::hex << b_seq_tags;
		return ostr.str();
	}

	uint64_t get_count() const
	{
		return count;
	}
};

template<size_t a, size_t b>
struct TemplateMax
{
	static size_t const value = a > b ? a : b;
};

template<typename _context_type, size_t _primeWidth>
struct PrimeProduct
{
	typedef _context_type context_type;
	typedef PrimeProduct<context_type,_primeWidth> this_type;

	// width of the prime in bits
	static size_t const primeWidth = _primeWidth;
	// width of the products in bits
	static size_t const productWidth = 2 * TemplateMax<primeWidth/32, context_type::digest_type::digestlength/4>::value;
	// folding prime
	static libmaus2::math::UnsignedInteger<productWidth> const foldprime;
	// prime
	static libmaus2::math::UnsignedInteger<productWidth> const prime;

	private:
	uint64_t count;

	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_t gmpprime;
	mpz_t gmpfoldprime;
	mpz_t gmpb_seq;
	mpz_t gmpname_b_seq;
	mpz_t gmpb_seq_qual;
	mpz_t gmpb_seq_tags;
	mpz_t gmpcrc;
	mpz_t gmpfoldcrc;
	mpz_t gmpprod;
	#else
	libmaus2::math::UnsignedInteger<productWidth> b_seq;
	libmaus2::math::UnsignedInteger<productWidth> name_b_seq;
	libmaus2::math::UnsignedInteger<productWidth> b_seq_qual;
	libmaus2::math::UnsignedInteger<productWidth> b_seq_tags;
	#endif

	template<size_t k>
	void updateProduct(libmaus2::math::UnsignedInteger<productWidth> & product, libmaus2::math::UnsignedInteger<k> const & rcrc)
	{
		// convert crc to different size
		libmaus2::math::UnsignedInteger<productWidth> crc(rcrc);
		// break down crc to range < prime
		crc = crc % foldprime;
		// make sure it is not null
		if ( crc.isNull() )
			crc = libmaus2::math::UnsignedInteger<productWidth>(1);
		product = product * crc;
		product = product % prime;
	}

	#if defined(LIBMAUS2_HAVE_GMP)
	template<size_t k>
	void updateProduct(mpz_t & gmpproduct, libmaus2::math::UnsignedInteger<k> const & rcrc)
	{
		mpz_import(gmpcrc,k,-1 /* least sign first */,sizeof(uint32_t),0 /* native endianess */,0 /* don't skip bits */, rcrc.getWords());
		mpz_mod(gmpfoldcrc,gmpcrc,gmpfoldprime); // foldcrc = crc % foldprime
		if ( mpz_cmp_ui(gmpfoldcrc,0) == 0 )
			mpz_set_ui(gmpfoldcrc,1);
		mpz_mul(gmpprod,gmpproduct,gmpfoldcrc); // prod = product * foldcrc
		mpz_mod(gmpproduct,gmpprod,gmpprime); // product = prod % prime
	}
	#endif

	#if defined(LIBMAUS2_HAVE_GMP)
	static libmaus2::math::UnsignedInteger<productWidth> convertNumber(mpz_t const & gmpnum)
	{
		size_t const numbitsperel = 8 * sizeof(uint32_t);
		size_t const numwords = (mpz_sizeinbase(gmpnum,2) + numbitsperel - 1) / numbitsperel;
		libmaus2::autoarray::AutoArray<uint32_t> A(numwords,false);
		size_t countp = numwords;
		mpz_export(A.begin(),&countp,-1,sizeof(uint32_t),0,0,gmpnum);
		libmaus2::math::UnsignedInteger<productWidth> U;
		for ( size_t i = 0; i < std::min(countp,static_cast<size_t>(productWidth)); ++i )
			U[i] = A[i];
		return U;
	}
	#endif

	public:
	PrimeProduct()
	: count(0)
		#if !defined(LIBMAUS2_HAVE_GMP)
		, b_seq(1), name_b_seq(1), b_seq_qual(1), b_seq_tags(1)
		#endif
	{
		#if defined(LIBMAUS2_HAVE_GMP)
		mpz_init(gmpprime);
		mpz_import(gmpprime,productWidth,-1 /* least sign first */,sizeof(uint32_t),0 /* native endianess */,0 /* don't skip bits */, prime.getWords());
		mpz_init(gmpfoldprime);
		mpz_import(gmpfoldprime,productWidth,-1 /* least sign first */,sizeof(uint32_t),0 /* native endianess */,0 /* don't skip bits */, foldprime.getWords());
		mpz_init_set_ui(gmpb_seq,1);
		mpz_init_set_ui(gmpname_b_seq,1);
		mpz_init_set_ui(gmpb_seq_qual,1);
		mpz_init_set_ui(gmpb_seq_tags,1);
		mpz_init(gmpcrc);
		mpz_init(gmpfoldcrc);
		mpz_init(gmpprod);
		#endif
	}
	~PrimeProduct()
	{
		#if defined(LIBMAUS2_HAVE_GMP)
		mpz_clear(gmpprime);
		mpz_clear(gmpfoldprime);
		mpz_clear(gmpb_seq);
		mpz_clear(gmpname_b_seq);
		mpz_clear(gmpb_seq_qual);
		mpz_clear(gmpb_seq_tags);
		mpz_clear(gmpfoldcrc);
		mpz_clear(gmpprod);
		#endif
	}

	void push(context_type const & context)
	{
		count += 1;
		#if defined(LIBMAUS2_HAVE_GMP)
		updateProduct(gmpb_seq,context.flags_seq_digest);
		updateProduct(gmpname_b_seq,context.name_flags_seq_digest);
		updateProduct(gmpb_seq_qual,context.flags_seq_qual_digest);
		updateProduct(gmpb_seq_tags,context.flags_seq_tags_digest);
		#else
		updateProduct(b_seq,context.flags_seq_digest);
		updateProduct(name_b_seq,context.name_flags_seq_digest);
		updateProduct(b_seq_qual,context.flags_seq_qual_digest);
		updateProduct(b_seq_tags,context.flags_seq_tags_digest);
		#endif
	}

	void push (this_type const & subsetproducts)
	{
		count += subsetproducts.count;
		#if defined(LIBMAUS2_HAVE_GMP)
		updateProduct(gmpb_seq,convertNumber(subsetproducts.gmpb_seq));
		updateProduct(gmpname_b_seq,convertNumber(subsetproducts.gmpname_b_seq));
		updateProduct(gmpb_seq_qual,convertNumber(subsetproducts.gmpb_seq_qual));
		updateProduct(gmpb_seq_tags,convertNumber(subsetproducts.gmpb_seq_tags));
		#else
		updateProduct(b_seq,subsetproducts.b_seq);
		updateProduct(name_b_seq,subsetproducts.name_b_seq);
		updateProduct(b_seq_qual,subsetproducts.b_seq_qual);
		updateProduct(b_seq_tags,subsetproducts.b_seq_tags);
		#endif
	};

	bool operator== (this_type const & other) const
	{
		return count==other.count
			#if defined(LIBMAUS2_HAVE_GMP)
			&& (mpz_cmp(gmpb_seq,other.gmpb_seq) == 0)
			&& (mpz_cmp(gmpname_b_seq,other.gmpname_b_seq) == 0)
			&& (mpz_cmp(gmpb_seq_qual,other.gmpb_seq_qual) == 0)
			&& (mpz_cmp(gmpb_seq_tags,other.gmpb_seq_tags) == 0)
			#else
			&& b_seq==other.b_seq
			&& name_b_seq==other.name_b_seq
			&& b_seq_qual==other.b_seq_qual
			&& b_seq_tags==other.b_seq_tags
			#endif
			;
	};

	void get_b_seq(std::vector<uint8_t> & H) const
	{
		#if defined(LIBMAUS2_HAVE_GMP)
		libmaus2::math::UnsignedInteger<primeWidth/32> U(convertNumber(gmpb_seq));
		#else
		libmaus2::math::UnsignedInteger<primeWidth/32> U(b_seq);
		#endif

		U.getHexVector(H);
	}

	void get_name_b_seq(std::vector<uint8_t> & H) const
	{
		#if defined(LIBMAUS2_HAVE_GMP)
		libmaus2::math::UnsignedInteger<primeWidth/32> U(convertNumber(gmpname_b_seq));
		#else
		libmaus2::math::UnsignedInteger<primeWidth/32> U(name_b_seq);
		#endif

		U.getHexVector(H);
	}

	void get_b_seq_qual(std::vector<uint8_t> & H) const
	{
		#if defined(LIBMAUS2_HAVE_GMP)
		libmaus2::math::UnsignedInteger<primeWidth/32> U(convertNumber(gmpb_seq_qual));
		#else
		libmaus2::math::UnsignedInteger<primeWidth/32> U(b_seq_qual);
		#endif

		U.getHexVector(H);
	}

	void get_b_seq_tags(std::vector<uint8_t> & H) const
	{
		#if defined(LIBMAUS2_HAVE_GMP)
		libmaus2::math::UnsignedInteger<primeWidth/32> U(convertNumber(gmpb_seq_tags));
		#else
		libmaus2::math::UnsignedInteger<primeWidth/32> U(b_seq_tags);
		#endif

		U.getHexVector(H);
	}

	std::string get_b_seq() const
	{
		std::ostringstream ostr;
		ostr << std::hex
			#if defined(LIBMAUS2_HAVE_GMP)
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(convertNumber(gmpb_seq))
			#else
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(b_seq)
			#endif
			;
		return ostr.str();
	}

	std::string get_name_b_seq() const
	{
		std::ostringstream ostr;
		ostr << std::hex
			#if defined(LIBMAUS2_HAVE_GMP)
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(convertNumber(gmpname_b_seq))
			#else
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(name_b_seq)
			#endif
			;
		return ostr.str();
	}

	std::string get_b_seq_qual() const
	{
		std::ostringstream ostr;
		ostr << std::hex
			#if defined(LIBMAUS2_HAVE_GMP)
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(convertNumber(gmpb_seq_qual))
			#else
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(b_seq_qual)
			#endif
			;
		return ostr.str();
	}

	std::string get_b_seq_tags() const
	{
		std::ostringstream ostr;
		ostr << std::hex
			#if defined(LIBMAUS2_HAVE_GMP)
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(convertNumber(gmpb_seq_tags))
			#else
			<< libmaus2::math::UnsignedInteger<primeWidth/32>(b_seq_tags)
			#endif
			;
		return ostr.str();
	}

	uint64_t get_count() const
	{
		return count;
	}
};

template<typename _context_type, size_t _primeWidth, size_t truncate>
struct PrimeSums
{
	typedef _context_type context_type;
	typedef PrimeSums<context_type,_primeWidth,truncate> this_type;

	// width of the prime in bits
	static size_t const primeWidth = _primeWidth;
	// width of the sums in bits
	static size_t const sumWidth = 1 + TemplateMax<primeWidth/32, context_type::digest_type::digestlength/4>::value;
	// prime
	static libmaus2::math::UnsignedInteger<sumWidth> const prime;

	private:
	uint64_t count;
	uint64_t modcount;

	libmaus2::math::UnsignedInteger<sumWidth> b_seq;
	libmaus2::math::UnsignedInteger<sumWidth> name_b_seq;
	libmaus2::math::UnsignedInteger<sumWidth> b_seq_qual;
	libmaus2::math::UnsignedInteger<sumWidth> b_seq_tags;

	// get prime width in bits
	unsigned int getPrimeWidth() const
	{
		for ( unsigned int i = 0; i < sumWidth; ++i )
		{
			// most significant to least significant word
			uint32_t v = prime[sumWidth-i-1];

			// if word is not null
			if ( v )
			{
				// get word shift
				unsigned int s = (sumWidth-i-1)*32;

				if ( (v & 0xFFFF0000UL) )
					v >>= 16, s += 16;
				if ( (v & 0xFF00UL) )
					v >>= 8, s += 8;
				if ( (v & 0xF0UL) )
					v >>= 4, s += 4;
				if ( (v & 0xCUL) )
					v >>= 2, s += 2;

				// add bit shift
				while ( v )
				{
					s++;
					v >>= 1;
				}

				return s;
			}
		}

		return 0;
	}

	// get prime width in hex digits
	unsigned int getPrimeHexWidth() const
	{
		unsigned int const primewidth = getPrimeWidth();
		return (primewidth + 3)/4;
	}

	std::string shortenHexDigest(std::string const & s) const
	{
		if ( truncate )
		{
			return s.substr(s.size() - (truncate/32)*8);
		}
		else
		{
			assert ( s.size() >= getPrimeHexWidth() );
			return s.substr(s.size()-getPrimeHexWidth());
		}
	}

	template<size_t k>
	void updateSum(libmaus2::math::UnsignedInteger<sumWidth> & sum, libmaus2::math::UnsignedInteger<k> const & crc)
	{
		// prime must be larger than crc to guarantee that (crc%prime) != 0 assuming crc!=0
		if ( expect_false(crc.isNull()) )
			sum += libmaus2::math::UnsignedInteger<sumWidth>(1);
		else
			sum += crc;

		if ( expect_false ( (++modcount & ((1ul<<30)-1)) == 0 ) )
		{
			sum %= prime;
			modcount = 0;
		}
	}

	public:
	PrimeSums() : count(0), modcount(0), b_seq(0), name_b_seq(0), b_seq_qual(0), b_seq_tags(0) {}

	void push(context_type const & context)
	{
		count += 1;
		updateSum(b_seq     ,context.flags_seq_digest);
		updateSum(name_b_seq,context.name_flags_seq_digest);
		updateSum(b_seq_qual,context.flags_seq_qual_digest);
		updateSum(b_seq_tags,context.flags_seq_tags_digest);
	}

	void push (this_type const & subsetproducts)
	{
		count += subsetproducts.count;
		b_seq      = ((b_seq      % prime) + (subsetproducts.b_seq      % prime)) % prime;
		name_b_seq = ((name_b_seq % prime) + (subsetproducts.name_b_seq % prime)) % prime;
		b_seq_qual = ((b_seq_qual % prime) + (subsetproducts.b_seq_qual % prime)) % prime;
		b_seq_tags = ((b_seq_tags % prime) + (subsetproducts.b_seq_tags % prime)) % prime;
	};

	bool operator== (this_type const & other) const
	{
		return count==other.count
			&& (b_seq%prime)==(other.b_seq%prime)
			&& (name_b_seq%prime)==(other.name_b_seq%prime)
			&& (b_seq_qual%prime)==(other.b_seq_qual%prime)
			&& (b_seq_tags%prime)==(other.b_seq_tags%prime)
			;
	};

	void get_b_seq(std::vector<uint8_t> & H) const
	{
		if ( truncate )
		{
			libmaus2::math::UnsignedInteger<truncate/32> U(b_seq % prime);
			U.getHexVector(H);
		}
		else
		{
			libmaus2::math::UnsignedInteger<primeWidth/32> U(b_seq % prime);
			U.getHexVector(H);
		}
	}

	void get_name_b_seq(std::vector<uint8_t> & H) const
	{
		if ( truncate )
		{
			libmaus2::math::UnsignedInteger<truncate/32> U(name_b_seq % prime);
			U.getHexVector(H);
		}
		else
		{
			libmaus2::math::UnsignedInteger<primeWidth/32> U(name_b_seq % prime);
			U.getHexVector(H);
		}
	}

	void get_b_seq_qual(std::vector<uint8_t> & H) const
	{
		if ( truncate )
		{
			libmaus2::math::UnsignedInteger<truncate/32> U(b_seq_qual % prime);
			U.getHexVector(H);
		}
		else
		{
			libmaus2::math::UnsignedInteger<primeWidth/32> U(b_seq_qual % prime);
			U.getHexVector(H);
		}
	}

	void get_b_seq_tags(std::vector<uint8_t> & H) const
	{
		if ( truncate )
		{
			libmaus2::math::UnsignedInteger<truncate/32> U(b_seq_tags % prime);
			U.getHexVector(H);
		}
		else
		{
			libmaus2::math::UnsignedInteger<primeWidth/32> U(b_seq_tags % prime);
			U.getHexVector(H);
		}
	}

	std::string get_b_seq() const
	{
		std::ostringstream ostr;
		ostr << std::hex << libmaus2::math::UnsignedInteger<primeWidth/32>(b_seq % prime);
		return shortenHexDigest(ostr.str());
	}

	std::string get_name_b_seq() const
	{
		std::ostringstream ostr;
		ostr << std::hex << libmaus2::math::UnsignedInteger<primeWidth/32>(name_b_seq % prime);
		return shortenHexDigest(ostr.str());
	}

	std::string get_b_seq_qual() const
	{
		std::ostringstream ostr;
		ostr << std::hex << libmaus2::math::UnsignedInteger<primeWidth/32>(b_seq_qual % prime);
		return shortenHexDigest(ostr.str());
	}

	std::string get_b_seq_tags() const
	{
		std::ostringstream ostr;
		ostr << std::hex << libmaus2::math::UnsignedInteger<primeWidth/32>(b_seq_tags % prime);
		return shortenHexDigest(ostr.str());
	}

	uint64_t get_count() const
	{
		return count;
	}
};

typedef SeqChksumsSimpleSums<libmaus2::bambam::CRC32SeqChksumUpdateContext> CRC32SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::MD5SeqChksumUpdateContext> MD5SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA1SeqChksumUpdateContext> SHA1SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext> SHA2_224_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext> SHA2_256_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext> SHA3_256_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext> SHA2_256_sse4_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext> SHA2_384_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext> SHA2_512_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext> SHA2_512_sse4_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::MurmurHash3_x64_128_SeqChksumUpdateContext> MurmurHash3_x64_128_SeqChksumsSimpleSums;
typedef SeqChksumsSimpleSums<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext> SHA3_256_SeqChksumsSimpleSums;

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,32> CRC32PrimeProduct32;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct32::productWidth> const CRC32PrimeProduct32::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getMersenne31<CRC32PrimeProduct32::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct32::productWidth> const CRC32PrimeProduct32::foldprime =
	libmaus2::bambam::SeqChksumPrimeNumbers::getMersenne31<CRC32PrimeProduct32::productWidth>() + libmaus2::math::UnsignedInteger<CRC32PrimeProduct32::productWidth>(1);

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,64> CRC32PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct64::productWidth> const CRC32PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<CRC32PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct64::productWidth> const CRC32PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<CRC32PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::MD5SeqChksumUpdateContext,64> MD5PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct64::productWidth> const MD5PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<MD5PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct64::productWidth> const MD5PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<MD5PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA1SeqChksumUpdateContext,64> SHA1PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct64::productWidth> const SHA1PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA1PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct64::productWidth> const SHA1PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA1PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext,64> SHA2_224_PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct64::productWidth> const SHA2_224_PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_224_PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct64::productWidth> const SHA2_224_PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_224_PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext,64> SHA2_256_PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct64::productWidth> const SHA2_256_PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_256_PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct64::productWidth> const SHA2_256_PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_256_PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext,64> SHA3_256_PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct64::productWidth> const SHA3_256_PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA3_256_PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct64::productWidth> const SHA3_256_PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA3_256_PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext,64> SHA2_256_sse4_PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct64::productWidth> const SHA2_256_sse4_PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_256_sse4_PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct64::productWidth> const SHA2_256_sse4_PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_256_sse4_PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext,64> SHA2_384_PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct64::productWidth> const SHA2_384_PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_384_PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct64::productWidth> const SHA2_384_PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_384_PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,64> SHA2_512_PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct64::productWidth> const SHA2_512_PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_512_PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct64::productWidth> const SHA2_512_PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_512_PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,64> SHA2_512_sse4_PrimeProduct64;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct64::productWidth> const SHA2_512_sse4_PrimeProduct64::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_512_sse4_PrimeProduct64::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct64::productWidth> const SHA2_512_sse4_PrimeProduct64::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime64<SHA2_512_sse4_PrimeProduct64::productWidth>();

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,96> CRC32PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct96::productWidth> const CRC32PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<CRC32PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct96::productWidth> const CRC32PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<CRC32PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::MD5SeqChksumUpdateContext,96> MD5PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct96::productWidth> const MD5PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<MD5PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct96::productWidth> const MD5PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<MD5PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA1SeqChksumUpdateContext,96> SHA1PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct96::productWidth> const SHA1PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA1PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct96::productWidth> const SHA1PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA1PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext,96> SHA2_224_PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct96::productWidth> const SHA2_224_PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_224_PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct96::productWidth> const SHA2_224_PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_224_PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext,96> SHA2_256_PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct96::productWidth> const SHA2_256_PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_256_PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct96::productWidth> const SHA2_256_PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_256_PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext,96> SHA3_256_PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct96::productWidth> const SHA3_256_PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA3_256_PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct96::productWidth> const SHA3_256_PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA3_256_PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext,96> SHA2_256_sse4_PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct96::productWidth> const SHA2_256_sse4_PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_256_sse4_PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct96::productWidth> const SHA2_256_sse4_PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_256_sse4_PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext,96> SHA2_384_PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct96::productWidth> const SHA2_384_PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_384_PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct96::productWidth> const SHA2_384_PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_384_PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,96> SHA2_512_PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct96::productWidth> const SHA2_512_PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_512_PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct96::productWidth> const SHA2_512_PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_512_PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,96> SHA2_512_sse4_PrimeProduct96;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct96::productWidth> const SHA2_512_sse4_PrimeProduct96::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_512_sse4_PrimeProduct96::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct96::productWidth> const SHA2_512_sse4_PrimeProduct96::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime96<SHA2_512_sse4_PrimeProduct96::productWidth>();

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,128> CRC32PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct128::productWidth> const CRC32PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<CRC32PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct128::productWidth> const CRC32PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<CRC32PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::MD5SeqChksumUpdateContext,128> MD5PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct128::productWidth> const MD5PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<MD5PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct128::productWidth> const MD5PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<MD5PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA1SeqChksumUpdateContext,128> SHA1PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct128::productWidth> const SHA1PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA1PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct128::productWidth> const SHA1PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA1PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext,128> SHA2_224_PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct128::productWidth> const SHA2_224_PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_224_PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct128::productWidth> const SHA2_224_PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_224_PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext,128> SHA2_256_PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct128::productWidth> const SHA2_256_PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_256_PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct128::productWidth> const SHA2_256_PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_256_PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext,128> SHA3_256_PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct128::productWidth> const SHA3_256_PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA3_256_PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct128::productWidth> const SHA3_256_PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA3_256_PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext,128> SHA2_256_sse4_PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct128::productWidth> const SHA2_256_sse4_PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_256_sse4_PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct128::productWidth> const SHA2_256_sse4_PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_256_sse4_PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext,128> SHA2_384_PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct128::productWidth> const SHA2_384_PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_384_PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct128::productWidth> const SHA2_384_PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_384_PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,128> SHA2_512_PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct128::productWidth> const SHA2_512_PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_512_PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct128::productWidth> const SHA2_512_PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_512_PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,128> SHA2_512_sse4_PrimeProduct128;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct128::productWidth> const SHA2_512_sse4_PrimeProduct128::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_512_sse4_PrimeProduct128::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct128::productWidth> const SHA2_512_sse4_PrimeProduct128::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime128<SHA2_512_sse4_PrimeProduct128::productWidth>();

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,160> CRC32PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct160::productWidth> const CRC32PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<CRC32PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct160::productWidth> const CRC32PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<CRC32PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::MD5SeqChksumUpdateContext,160> MD5PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct160::productWidth> const MD5PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<MD5PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct160::productWidth> const MD5PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<MD5PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA1SeqChksumUpdateContext,160> SHA1PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct160::productWidth> const SHA1PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA1PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct160::productWidth> const SHA1PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA1PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext,160> SHA2_224_PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct160::productWidth> const SHA2_224_PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_224_PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct160::productWidth> const SHA2_224_PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_224_PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext,160> SHA2_256_PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct160::productWidth> const SHA2_256_PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_256_PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct160::productWidth> const SHA2_256_PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_256_PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext,160> SHA3_256_PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct160::productWidth> const SHA3_256_PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA3_256_PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct160::productWidth> const SHA3_256_PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA3_256_PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext,160> SHA2_256_sse4_PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct160::productWidth> const SHA2_256_sse4_PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_256_sse4_PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct160::productWidth> const SHA2_256_sse4_PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_256_sse4_PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext,160> SHA2_384_PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct160::productWidth> const SHA2_384_PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_384_PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct160::productWidth> const SHA2_384_PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_384_PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,160> SHA2_512_PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct160::productWidth> const SHA2_512_PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_512_PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct160::productWidth> const SHA2_512_PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_512_PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,160> SHA2_512_sse4_PrimeProduct160;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct160::productWidth> const SHA2_512_sse4_PrimeProduct160::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_512_sse4_PrimeProduct160::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct160::productWidth> const SHA2_512_sse4_PrimeProduct160::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime160<SHA2_512_sse4_PrimeProduct160::productWidth>();

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,192> CRC32PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct192::productWidth> const CRC32PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<CRC32PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct192::productWidth> const CRC32PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<CRC32PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::MD5SeqChksumUpdateContext,192> MD5PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct192::productWidth> const MD5PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<MD5PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct192::productWidth> const MD5PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<MD5PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA1SeqChksumUpdateContext,192> SHA1PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct192::productWidth> const SHA1PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA1PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct192::productWidth> const SHA1PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA1PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext,192> SHA2_224_PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct192::productWidth> const SHA2_224_PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_224_PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct192::productWidth> const SHA2_224_PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_224_PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext,192> SHA2_256_PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct192::productWidth> const SHA2_256_PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_256_PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct192::productWidth> const SHA2_256_PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_256_PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext,192> SHA3_256_PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct192::productWidth> const SHA3_256_PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA3_256_PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct192::productWidth> const SHA3_256_PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA3_256_PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext,192> SHA2_256_sse4_PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct192::productWidth> const SHA2_256_sse4_PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_256_sse4_PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct192::productWidth> const SHA2_256_sse4_PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_256_sse4_PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext,192> SHA2_384_PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct192::productWidth> const SHA2_384_PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_384_PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct192::productWidth> const SHA2_384_PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_384_PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,192> SHA2_512_PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct192::productWidth> const SHA2_512_PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_512_PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct192::productWidth> const SHA2_512_PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_512_PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,192> SHA2_512_sse4_PrimeProduct192;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct192::productWidth> const SHA2_512_sse4_PrimeProduct192::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_512_sse4_PrimeProduct192::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct192::productWidth> const SHA2_512_sse4_PrimeProduct192::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime192<SHA2_512_sse4_PrimeProduct192::productWidth>();

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,224> CRC32PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct224::productWidth> const CRC32PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<CRC32PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct224::productWidth> const CRC32PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<CRC32PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::MD5SeqChksumUpdateContext,224> MD5PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct224::productWidth> const MD5PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<MD5PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct224::productWidth> const MD5PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<MD5PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA1SeqChksumUpdateContext,224> SHA1PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct224::productWidth> const SHA1PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA1PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct224::productWidth> const SHA1PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA1PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext,224> SHA2_224_PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct224::productWidth> const SHA2_224_PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_224_PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct224::productWidth> const SHA2_224_PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_224_PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext,224> SHA2_256_PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct224::productWidth> const SHA2_256_PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_256_PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct224::productWidth> const SHA2_256_PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_256_PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext,224> SHA3_256_PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct224::productWidth> const SHA3_256_PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA3_256_PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct224::productWidth> const SHA3_256_PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA3_256_PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext,224> SHA2_256_sse4_PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct224::productWidth> const SHA2_256_sse4_PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_256_sse4_PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct224::productWidth> const SHA2_256_sse4_PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_256_sse4_PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext,224> SHA2_384_PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct224::productWidth> const SHA2_384_PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_384_PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct224::productWidth> const SHA2_384_PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_384_PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,224> SHA2_512_PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct224::productWidth> const SHA2_512_PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_512_PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct224::productWidth> const SHA2_512_PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_512_PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,224> SHA2_512_sse4_PrimeProduct224;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct224::productWidth> const SHA2_512_sse4_PrimeProduct224::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_512_sse4_PrimeProduct224::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct224::productWidth> const SHA2_512_sse4_PrimeProduct224::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime224<SHA2_512_sse4_PrimeProduct224::productWidth>();

typedef PrimeProduct<libmaus2::bambam::CRC32SeqChksumUpdateContext,256> CRC32PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct256::productWidth> const CRC32PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<CRC32PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<CRC32PrimeProduct256::productWidth> const CRC32PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<CRC32PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::MD5SeqChksumUpdateContext,256> MD5PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct256::productWidth> const MD5PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<MD5PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<MD5PrimeProduct256::productWidth> const MD5PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<MD5PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA1SeqChksumUpdateContext,256> SHA1PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct256::productWidth> const SHA1PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA1PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA1PrimeProduct256::productWidth> const SHA1PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA1PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_224_SeqChksumUpdateContext,256> SHA2_224_PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct256::productWidth> const SHA2_224_PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_224_PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_224_PrimeProduct256::productWidth> const SHA2_224_PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_224_PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_SeqChksumUpdateContext,256> SHA2_256_PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct256::productWidth> const SHA2_256_PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_256_PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_PrimeProduct256::productWidth> const SHA2_256_PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_256_PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA3_256_SeqChksumUpdateContext,256> SHA3_256_PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct256::productWidth> const SHA3_256_PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA3_256_PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA3_256_PrimeProduct256::productWidth> const SHA3_256_PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA3_256_PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_256_sse4_SeqChksumUpdateContext,256> SHA2_256_sse4_PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct256::productWidth> const SHA2_256_sse4_PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_256_sse4_PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_256_sse4_PrimeProduct256::productWidth> const SHA2_256_sse4_PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_256_sse4_PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_384_SeqChksumUpdateContext,256> SHA2_384_PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct256::productWidth> const SHA2_384_PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_384_PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_384_PrimeProduct256::productWidth> const SHA2_384_PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_384_PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,256> SHA2_512_PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct256::productWidth> const SHA2_512_PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_512_PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeProduct256::productWidth> const SHA2_512_PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_512_PrimeProduct256::productWidth>();

typedef PrimeProduct<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,256> SHA2_512_sse4_PrimeProduct256;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct256::productWidth> const SHA2_512_sse4_PrimeProduct256::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_512_sse4_PrimeProduct256::productWidth>();
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeProduct256::productWidth> const SHA2_512_sse4_PrimeProduct256::foldprime = libmaus2::bambam::SeqChksumPrimeNumbers::getPrime256<SHA2_512_sse4_PrimeProduct256::productWidth>();

typedef PrimeSums<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,544,512> SHA2_512_PrimeSums512;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeSums512::sumWidth> const SHA2_512_PrimeSums512::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getNextPrime512<SHA2_512_PrimeSums512::sumWidth>();

typedef PrimeSums<libmaus2::bambam::SHA2_512_SeqChksumUpdateContext,544,0> SHA2_512_PrimeSums;
template<> libmaus2::math::UnsignedInteger<SHA2_512_PrimeSums::sumWidth> const SHA2_512_PrimeSums::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getMersenne521<SHA2_512_PrimeSums::sumWidth>();

typedef PrimeSums<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,544,512> SHA2_512_sse4_PrimeSums512;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeSums512::sumWidth> const SHA2_512_sse4_PrimeSums512::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getNextPrime512<SHA2_512_sse4_PrimeSums512::sumWidth>();

typedef PrimeSums<libmaus2::bambam::SHA2_512_sse4_SeqChksumUpdateContext,544,0> SHA2_512_sse4_PrimeSums;
template<> libmaus2::math::UnsignedInteger<SHA2_512_sse4_PrimeSums::sumWidth> const SHA2_512_sse4_PrimeSums::prime = libmaus2::bambam::SeqChksumPrimeNumbers::getMersenne521<SHA2_512_sse4_PrimeSums::sumWidth>();

typedef PrimeSums<libmaus2::bambam::MurmurHash3_x64_128_SeqChksumUpdateContext,160,128> MurmurHash3_x64_128_PrimeSums128;
template<> libmaus2::math::UnsignedInteger<MurmurHash3_x64_128_PrimeSums128::sumWidth> const MurmurHash3_x64_128_PrimeSums128::prime =
	libmaus2::bambam::SeqChksumPrimeNumbers::getNextPrime128<MurmurHash3_x64_128_PrimeSums128::sumWidth>();

/**
* Product checksums calculated based on basecalls and (multi segment, first
* and last) bit info, and this combined with the query name, or the basecall
* qualities, or certain BAM auxilary fields.
**/
struct SeqChksumsCRC32Products
{
	typedef libmaus2::bambam::CRC32SeqChksumUpdateContext context_type;

	private:
	uint64_t count;
	uint64_t b_seq;
	uint64_t name_b_seq;
	uint64_t b_seq_qual;
	uint64_t b_seq_tags;

	/**
	* Multiply existing product by new checksum, having altered checksum ready for
	* multiplication in a finite field i.e. 0 < chksum < (2^31 -1)
	* @param product to be updated
	* @param chksum to update product with
	**/
	static void product_munged_chksum_multiply (uint64_t & product, uint32_t chksum) {
		static uint64_t const MERSENNE31 = 0x7FFFFFFFull; // Mersenne Prime 2^31 - 1
		chksum &= MERSENNE31;
		if (!chksum || chksum == MERSENNE31 ) chksum = 1;
		product = ( product * chksum ) % MERSENNE31;
	}

	void push_count(uint64_t const add)
	{
		count += add;
	}

	void push_b_seq(uint32_t const chksum)
	{
		product_munged_chksum_multiply(b_seq,chksum);
	}

	void push_name_b_seq(uint32_t const chksum)
	{
		product_munged_chksum_multiply(name_b_seq,chksum);
	}

	void push_b_seq_qual(uint32_t const chksum)
	{
		product_munged_chksum_multiply(b_seq_qual,chksum);
	}

	void push_b_seq_tags(uint32_t const chksum)
	{
		product_munged_chksum_multiply(b_seq_tags,chksum);
	}

	void push(
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const & name_b_seq,
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const & b_seq,
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const & b_seq_qual,
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const & b_seq_tags
	)
	{
		push_count(1);
		push_name_b_seq(name_b_seq[0]);
		push_b_seq(b_seq[0]);
		push_b_seq_qual(b_seq_qual[0]);
		push_b_seq_tags(b_seq_tags[0]);
	}

	public:
	SeqChksumsCRC32Products() : count(0), b_seq(1), name_b_seq(1), b_seq_qual(1), b_seq_tags(1)
	{
	}

	void get_b_seq(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(b_seq);
		U.getHexVector(H);
	}

	void get_name_b_seq(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(name_b_seq);
		U.getHexVector(H);
	}

	void get_b_seq_qual(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(b_seq_qual);
		U.getHexVector(H);
	}

	void get_b_seq_tags(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(b_seq_tags);
		U.getHexVector(H);
	}

	uint64_t get_b_seq() const
	{
		return b_seq;
	}

	uint64_t get_name_b_seq() const
	{
		return name_b_seq;
	}

	uint64_t get_b_seq_qual() const
	{
		return b_seq_qual;
	}

	uint64_t get_b_seq_tags() const
	{
		return b_seq_tags;
	}

	uint64_t get_count() const
	{
		return count;
	}

	void push(context_type const & context)
	{
		push(
			context.name_flags_seq_digest,
			context.flags_seq_digest,
			context.flags_seq_qual_digest,
			context.flags_seq_tags_digest
		);
	}

	void push (SeqChksumsCRC32Products const & subsetproducts)
	{
		count += subsetproducts.count;
		product_munged_chksum_multiply(b_seq, subsetproducts.b_seq);
		product_munged_chksum_multiply(name_b_seq, subsetproducts.name_b_seq);
		product_munged_chksum_multiply(b_seq_qual, subsetproducts.b_seq_qual);
		product_munged_chksum_multiply(b_seq_tags, subsetproducts.b_seq_tags);
	};
	bool operator== (SeqChksumsCRC32Products const & other) const
	{
		return count==other.count &&
			b_seq==other.b_seq && name_b_seq==other.name_b_seq &&
			b_seq_qual==other.b_seq_qual && b_seq_tags==other.b_seq_tags;
	};
};

/**
 * null checksums for performance testing
**/
struct NullChecksums
{
	typedef libmaus2::bambam::NullSeqChksumUpdateContext context_type;

	private:
	uint64_t count;

	void push_count(uint64_t const add)
	{
		count += add;
	}

	void push(
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const &,
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const &,
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const &,
		libmaus2::math::UnsignedInteger<context_type::digest_type::digestlength/4> const &
	)
	{
		push_count(1);
	}

	public:
	NullChecksums() : count(0)
	{
	}

	void get_b_seq(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(0);
		U.getHexVector(H);
	}

	void get_name_b_seq(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(0);
		U.getHexVector(H);
	}

	void get_b_seq_qual(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(0);
		U.getHexVector(H);
	}

	void get_b_seq_tags(std::vector<uint8_t> & H) const
	{
		libmaus2::math::UnsignedInteger<1> U(0);
		U.getHexVector(H);
	}

	uint64_t get_b_seq() const
	{
		return 0;
	}

	uint64_t get_name_b_seq() const
	{
		return 0;
	}

	uint64_t get_b_seq_qual() const
	{
		return 0;
	}

	uint64_t get_b_seq_tags() const
	{
		return 0;
	}

	uint64_t get_count() const
	{
		return count;
	}

	void push(context_type const & context)
	{
		push(
			context.name_flags_seq_digest,
			context.flags_seq_digest,
			context.flags_seq_qual_digest,
			context.flags_seq_tags_digest
		);
	}

	void push (NullChecksums const & subsetproducts)
	{
		count += subsetproducts.count;
	};
	bool operator== (NullChecksums const & other) const
	{
		return count==other.count;
	};
};

/**
* Finite field products of CRC32 checksums of primary/source sequence data
*
* Checksum products should remain unchanged if primary data is retained no matter what
* alignment information is added or altered, or the ordering of the records.
*
* Products calulated for all records and for those with pass QC bit.
**/
template<typename _crc_container_type>
struct OrderIndependentSeqDataChecksums {
	typedef _crc_container_type crc_container_type;
	typedef typename crc_container_type::context_type context_type;

	private:
	::libmaus2::autoarray::AutoArray<char> A; // check with German: can we change/treat underlying data block to unsigned char / uint8_t?
	::libmaus2::autoarray::AutoArray<char> B; //separate A & B can allow reordering speed up?

	static std::vector<std::string> getDefaultAuxTags() {
		static const char * defaults[] = {"BC","FI","QT","RT","TC"}; //lexical order
		std::vector<std::string> v (defaults, defaults + sizeof(defaults)/sizeof(*defaults));
		return v;
	}

	public:
	std::vector<std::string> const auxtags;
	// to provide fast checking of aux fields when processing and validation at startup
	::libmaus2::bambam::BamAuxFilterVector const auxtagsfilter;
	crc_container_type all;
	crc_container_type pass;
	OrderIndependentSeqDataChecksums() : A(), B(), auxtags(getDefaultAuxTags()), auxtagsfilter(auxtags), all(), pass() { };
	/**
	* Combine primary sequence data from alignment record into checksum products
	*
	* Ignores Supplementary and auxilary alignment records so primary data in not
	* included twice.
	*
	* @param algn BAM alignment record
	**/
	void push(uint8_t const * D, uint32_t const blocksize, typename crc_container_type::context_type & context)
	{
		uint32_t const aflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(D);

		if
		( ! (
			aflags &
			(
				::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY |
				::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY
			)
		) )
		{
			static uint16_t const maskflags =
				::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED |
				::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1 |
				::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2;

			uint8_t const flags = (aflags & maskflags) & 0xFF;

			bool const isqcfail  = (aflags & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FQCFAIL);
			bool const isreverse = (aflags & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE);
			char const * readname = libmaus2::bambam::BamAlignmentDecoderBase::getReadName(D);
			uint32_t const lreadname = libmaus2::bambam::BamAlignmentDecoderBase::getLReadName(D);

			context.pass = !isqcfail;
			context.valid = true;
			uint64_t const len = isreverse ?
				libmaus2::bambam::BamAlignmentDecoderBase::decodeReadRC(D,A)
				:
				libmaus2::bambam::BamAlignmentDecoderBase::decodeRead(D,A);

			#if defined(BAM_SEQ_CHKSUM_DEBUG)
			uint32_t const CRC32_INITIAL = crc32(0L, Z_NULL, 0);
			#endif

			context.ctx_name_flags_seq.init();
			// read name (lreadname bytes)
			context.ctx_name_flags_seq.update(reinterpret_cast<uint8_t const *>(readname),lreadname);
			// flags (1 byte)
			context.ctx_name_flags_seq.update(reinterpret_cast<uint8_t const *>(&flags),1);
			// query sequence
			context.ctx_name_flags_seq.update(reinterpret_cast<uint8_t const *>(A.begin()),len);

			#if defined(BAM_SEQ_CHKSUM_DEBUG)
			uint32_t chksum_name_flags_seq = crc32(CRC32_INITIAL,reinterpret_cast<const unsigned char *>(readname),lreadname);
			chksum_name_flags_seq = crc32(chksum_name_flags_seq,reinterpret_cast<const unsigned char*>( &flags), 1);
			chksum_name_flags_seq = crc32(chksum_name_flags_seq,reinterpret_cast<const unsigned char *>( A.begin()), len);
			#endif

			context.ctx_flags_seq.init();
			// flags
			context.ctx_flags_seq.update(reinterpret_cast<uint8_t const *>(&flags),1);
			// query sequence
			context.ctx_flags_seq.update(reinterpret_cast<uint8_t const *>(A.begin()),len);

			#if defined(BAM_SEQ_CHKSUM_DEBUG)
			uint32_t chksum_flags_seq = crc32(CRC32_INITIAL,reinterpret_cast<const unsigned char*>(&flags), 1);
			chksum_flags_seq = crc32(chksum_flags_seq,reinterpret_cast<const unsigned char *>( A.begin()), len);
			#endif

			// add quality
			uint64_t const lenq = isreverse ?
				libmaus2::bambam::BamAlignmentDecoderBase::decodeQualRC(D,B)
				:
				libmaus2::bambam::BamAlignmentDecoderBase::decodeQual(D,B);

			context.ctx_flags_seq_qual.copyFrom(context.ctx_flags_seq);
			context.ctx_flags_seq_qual.update(reinterpret_cast<uint8_t *>(B.begin()), lenq);

			#if defined(BAM_SEQ_CHKSUM_DEBUG)
			uint32_t chksum_flags_seq_qual = chksum_flags_seq;
			chksum_flags_seq_qual = crc32(chksum_flags_seq_qual,reinterpret_cast<const unsigned char *>( B.begin()), lenq);
			#endif

			// set aux to start pointer of aux area
			uint8_t const * aux = ::libmaus2::bambam::BamAlignmentDecoderBase::getAux(D);
			// end of algn data block (and so of aux area)
			uint8_t const * const auxend = D + blocksize;

			// start of each aux entry for auxtags
			std::vector<uint8_t const *> paux(auxtags.size(),NULL);
			// size of each aux entry for auxtags
			std::vector<uint64_t> saux(auxtags.size(),0);

			while(aux < auxend)
			{
				// get length of aux field in bytes
				uint64_t const auxlen = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxLength(aux);

				// skip over aux tag data we're not interested in
				if( auxtagsfilter(aux[0],aux[1]) )
				{
					std::vector<std::string>::const_iterator it_auxtags = auxtags.begin();
					std::vector<uint8_t const *>::iterator it_paux = paux.begin();
					std::vector<uint64_t>::iterator it_saux = saux.begin();
					// consider each tag in auxtag
					while ( it_auxtags != auxtags.end())
					{
						// until we match the tag for the current aux data
						if(! it_auxtags->compare(0,2,reinterpret_cast<const char *>(aux),2) )
						{
							*it_paux = aux;
							*it_saux = auxlen;
							// stop this loop
							it_auxtags = auxtags.end();
						}
						else
						{
							++it_auxtags;
							++it_paux;
							++it_saux;
						}
					}
				}
				aux += auxlen;
			}
			std::vector<uint8_t const *>::const_iterator it_paux = paux.begin();
			std::vector<uint64_t>::const_iterator it_saux = saux.begin();

			// copy context
			context.ctx_flags_seq_tags.copyFrom(context.ctx_flags_seq);

			#if defined(BAM_SEQ_CHKSUM_DEBUG)
			uint32_t chksum_flags_seq_tags = chksum_flags_seq;
			#endif

			//loop over the chunks of data corresponding to the auxtags
			while (it_paux != paux.end())
			{
				//if data exists push into running checksum
				if(*it_paux)
				{
					context.ctx_flags_seq_tags.update(reinterpret_cast<uint8_t const *>(*it_paux), *it_saux);
					#if defined(BAM_SEQ_CHKSUM_DEBUG)
					chksum_flags_seq_tags = crc32(chksum_flags_seq_tags,reinterpret_cast<const unsigned char *>(*it_paux), *it_saux);
					#endif
				}
				++it_paux;
				++it_saux;
			}


			context.flags_seq_qual_digest = context.ctx_flags_seq_qual.digestui();
			context.name_flags_seq_digest = context.ctx_name_flags_seq.digestui();
			context.flags_seq_digest = context.ctx_flags_seq.digestui();
			context.flags_seq_tags_digest = context.ctx_flags_seq_tags.digestui();

			#if defined(BAM_SEQ_CHKSUM_DEBUG)
			assert ( context.flags_seq_qual_digest[0] == chksum_flags_seq_qual );
			assert ( context.name_flags_seq_digest[0] == chksum_name_flags_seq );
			assert ( context.flags_seq_digest[0] == chksum_flags_seq );
			assert ( context.flags_seq_tags_digest[0] == chksum_flags_seq_tags );
			#endif

			push(context);
		}
		else
		{
			context.valid = false;
		}
	};

	void push(typename crc_container_type::context_type const & context)
	{
		if ( context.valid )
		{
			all.push(context);

			if ( context.pass )
				pass.push(context);
		}
	}

	void push(OrderIndependentSeqDataChecksums const & subsetchksum)
	{
		pass.push(subsetchksum.pass);
		all.push(subsetchksum.all);
	};
	bool operator== (OrderIndependentSeqDataChecksums const & other) const
	{
		return pass==other.pass && all==other.all && auxtags==other.auxtags;
	}
};
template<typename N>
struct ChecksumsArrayErase
{
	static void erase(N *, uint64_t const)
	{

	}
};

template<typename _container_type, typename _header_type>
struct Checksums : public libmaus2::bambam::ChecksumsInterface
{
	typedef _container_type container_type;
	typedef _header_type header_type;
	typedef Checksums<container_type,header_type> this_type;

	std::string const hash;
	header_type const & header;

	OrderIndependentSeqDataChecksums<container_type> chksums;
	libmaus2::autoarray::AutoArray<
		OrderIndependentSeqDataChecksums<container_type>,
		libmaus2::autoarray::alloc_type_cxx,
		ChecksumsArrayErase<OrderIndependentSeqDataChecksums<container_type> > > readgroup_chksums;
	typename OrderIndependentSeqDataChecksums<container_type>::context_type updatecontext;

	Checksums(std::string const & rhash, header_type const & rheader)
	: hash(rhash), header(rheader), chksums(), readgroup_chksums(1 + header.getNumReadGroups(),false), updatecontext()
	{

	}

	void update(libmaus2::bambam::ChecksumsInterface const & IO)
	{
		this_type const & O = dynamic_cast<this_type const &>(IO);
		chksums.push(O.chksums);
		for ( uint64_t i = 0; i < readgroup_chksums.size(); ++i )
			readgroup_chksums[i].push(O.readgroup_chksums[i]);
	}

	void update(uint8_t const * D, uint32_t const blocksize)
	{
		chksums.push(D,blocksize,updatecontext);
		readgroup_chksums[header.getReadGroupId(::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(D,blocksize,"RG"))+1].push(updatecontext);
	}

	void update(libmaus2::bambam::BamAlignment const & algn)
	{
		update(algn.D.begin(),algn.blocksize);
	}

	void get_b_seq_all(std::vector<uint8_t> & H) const
	{
		chksums.all.get_b_seq(H);
	}

	void get_name_b_seq_all(std::vector<uint8_t> & H) const
	{
		chksums.all.get_name_b_seq(H);
	}

	void get_b_seq_qual_all(std::vector<uint8_t> & H) const
	{
		chksums.all.get_b_seq_qual(H);
	}

	void get_b_seq_tags_all(std::vector<uint8_t> & H) const
	{
		chksums.all.get_b_seq_tags(H);
	}

	void get_b_seq_pass(std::vector<uint8_t> & H) const
	{
		chksums.pass.get_b_seq(H);
	}

	void get_name_b_seq_pass(std::vector<uint8_t> & H) const
	{
		chksums.pass.get_name_b_seq(H);
	}

	void get_b_seq_qual_pass(std::vector<uint8_t> & H) const
	{
		chksums.pass.get_b_seq_qual(H);
	}

	void get_b_seq_tags_pass(std::vector<uint8_t> & H) const
	{
		chksums.pass.get_b_seq_tags(H);
	}

	void printVerbose(
		std::ostream & log,
		uint64_t const c,
		libmaus2::bambam::BamAlignment const & algn, double const etime
	)
	{
		log << "[V] " << c/(1024*1024) << " " << chksums.all.get_count() << " " << algn.getName() << " " << algn.isRead1()
			<< " " << algn.isRead2() << " " << ( algn.isReverse() ? algn.getReadRC() : algn.getRead() ) << " "
			<< ( algn.isReverse() ? algn.getQualRC() : algn.getQual() ) << " " << std::hex << (0x0 + algn.getFlags())
			<< std::dec << " " << chksums.all.get_b_seq() << " " << chksums.all.get_b_seq_tags() << " "
			<< " " << etime
			<< std::endl;
	}

	void printChecksumsForBamHeader(std::ostream & out)
	{
		std::ostringstream tagsstr;

		for (std::vector<std::string>::const_iterator it_aux = chksums.auxtags.begin(); it_aux != chksums.auxtags.end(); ++it_aux)
		{
			if (chksums.auxtags.begin() != it_aux)
				tagsstr << ",";
			tagsstr << *it_aux;
		}
		std::string const tags = tagsstr.str();

		out
			<< "@CO\tTY:checksum\tST:all\tPA:all\tHA:" << hash << "\tCO:" << chksums.all.get_count() << "\t"
			<< "BS:" << std::hex << chksums.all.get_b_seq() << std::dec << "\t"
			<< "NS:" << std::hex << chksums.all.get_name_b_seq() << std::dec << "\t"
			<< "SQ:" << std::hex << chksums.all.get_b_seq_qual() << std::dec << "\t"
			<< "ST:" << tags << ":" << std::hex << chksums.all.get_b_seq_tags() << std::dec << "\n";
		out
			<< "@CO\tTY:checksum\tST:all\tPA:pass\tHA:" << hash << "\tCO:" << chksums.pass.get_count() << "\t"
			<< "BS:" << std::hex << chksums.pass.get_b_seq() << std::dec << "\t"
			<< "NS:" << std::hex << chksums.pass.get_name_b_seq() << std::dec << "\t"
			<< "SQ:" << std::hex << chksums.pass.get_b_seq_qual() << std::dec << "\t"
			<< "ST:" << tags << ":" << std::hex << chksums.pass.get_b_seq_tags() << std::dec << "\n";

		if(header.getNumReadGroups())
		{
			OrderIndependentSeqDataChecksums<container_type> chksumschk;
			for(unsigned int i=0; i<=header.getNumReadGroups(); i++)
			{
				chksumschk.push(readgroup_chksums[i]);

				out
					<< "@CO\tTY:checksum\tST:" << (i>0 ? header.getReadGroupIdentifierAsString(i-1) : "") << "\tPA:all\tHA:" << hash << "\tCO:" << readgroup_chksums[i].all.get_count() << "\t"
					<< "BS:" << std::hex << readgroup_chksums[i].all.get_b_seq() << std::dec << "\t"
					<< "NS:" << std::hex << readgroup_chksums[i].all.get_name_b_seq() << std::dec << "\t"
					<< "SQ:" << std::hex << readgroup_chksums[i].all.get_b_seq_qual() << std::dec << "\t"
					<< "ST:" << tags << ":" << std::hex << readgroup_chksums[i].all.get_b_seq_tags() << std::dec << "\n";
				out
					<< "@CO\tTY:checksum\tST:" << (i>0 ? header.getReadGroupIdentifierAsString(i-1) : "") << "\tPA:pass\tHA:" << hash << "\tCO:" << readgroup_chksums[i].pass.get_count() << "\t"
					<< "BS:" << std::hex << readgroup_chksums[i].pass.get_b_seq() << std::dec << "\t"
					<< "NS:" << std::hex << readgroup_chksums[i].pass.get_name_b_seq() << std::dec << "\t"
					<< "SQ:" << std::hex << readgroup_chksums[i].pass.get_b_seq_qual() << std::dec << "\t"
					<< "ST:" << tags << ":" << std::hex << readgroup_chksums[i].pass.get_b_seq_tags() << std::dec << "\n";
			}
			assert(chksumschk == chksums);
		}
	}

	void printChecksums(std::ostream & out)
	{
		out << "###\tset\t" << "count" << "\t" << "\t" << "b_seq" << "\t" << "name_b_seq" << "\t" << "b_seq_qual" << "\t"
			<< "b_seq_tags(";
		for (std::vector<std::string>::const_iterator it_aux = chksums.auxtags.begin(); it_aux != chksums.auxtags.end(); ++it_aux)
		{
			if (chksums.auxtags.begin() != it_aux)
				out << ",";
			out << *it_aux;
		}
		out << ")" << std::endl;
		out << "all\tall\t" << chksums.all.get_count() << "\t" << std::hex << "\t" << chksums.all.get_b_seq() << "\t"
			<< chksums.all.get_name_b_seq() << "\t" << chksums.all.get_b_seq_qual() << "\t" << chksums.all.get_b_seq_tags() << std::dec << std::endl;
		out << "all\tpass\t" << chksums.pass.get_count() << "\t" << std::hex << "\t" << chksums.pass.get_b_seq() << "\t"
			<< chksums.pass.get_name_b_seq() << "\t" << chksums.pass.get_b_seq_qual() << "\t" << chksums.pass.get_b_seq_tags() << std::dec << std::endl;

		if(header.getNumReadGroups())
		{
			OrderIndependentSeqDataChecksums<container_type> chksumschk;
			for(unsigned int i=0; i<=header.getNumReadGroups(); i++)
			{
				chksumschk.push(readgroup_chksums[i]);
				out << (i>0 ? header.getReadGroupIdentifierAsString(i-1) : "") << "\tall\t" << readgroup_chksums[i].all.get_count() << "\t"
					<< std::hex << "\t" << readgroup_chksums[i].all.get_b_seq() << "\t" << readgroup_chksums[i].all.get_name_b_seq() << "\t"
					<< readgroup_chksums[i].all.get_b_seq_qual() << "\t" << readgroup_chksums[i].all.get_b_seq_tags() << std::dec << std::endl;
				out << (i>0 ? header.getReadGroupIdentifierAsString(i-1) : "") << "\tpass\t" << readgroup_chksums[i].pass.get_count() << "\t"
					<< std::hex << "\t" << readgroup_chksums[i].pass.get_b_seq() << "\t" << readgroup_chksums[i].pass.get_name_b_seq() << "\t"
					<< readgroup_chksums[i].pass.get_b_seq_qual() << "\t" << readgroup_chksums[i].pass.get_b_seq_tags() << std::dec << std::endl;
			}
			assert(chksumschk == chksums);
		}
	}
};

std::vector<std::string> libmaus2::bambam::ChecksumsFactory::getSupportedHashVariants()
{
	std::vector<std::string> V;
	V.push_back("crc32prod");
	V.push_back("crc32");
	V.push_back("md5");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1");
	V.push_back("sha224");
	V.push_back("sha256");
	V.push_back("sha384");
	V.push_back("sha512");
	#endif
	V.push_back("crc32prime32");
	V.push_back("crc32prime64");
	V.push_back("md5prime64");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1prime64");
	V.push_back("sha224prime64");
	V.push_back("sha256prime64");
	V.push_back("sha384prime64");
	V.push_back("sha512prime64");
	#endif
	V.push_back("crc32prime96");
	V.push_back("md5prime96");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1prime96");
	V.push_back("sha224prime96");
	V.push_back("sha256prime96");
	V.push_back("sha384prime96");
	V.push_back("sha512prime96");
	#endif
	V.push_back("crc32prime128");
	V.push_back("md5prime128");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1prime128");
	V.push_back("sha224prime128");
	V.push_back("sha256prime128");
	V.push_back("sha384prime128");
	V.push_back("sha512prime128");
	#endif
	V.push_back("crc32prime160");
	V.push_back("md5prime160");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1prime160");
	V.push_back("sha224prime160");
	V.push_back("sha256prime160");
	V.push_back("sha384prime160");
	V.push_back("sha512prime160");
	#endif
	V.push_back("crc32prime192");
	V.push_back("md5prime192");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1prime192");
	V.push_back("sha224prime192");
	V.push_back("sha256prime192");
	V.push_back("sha384prime192");
	V.push_back("sha512prime192");
	#endif
	V.push_back("crc32prime224");
	V.push_back("md5prime224");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1prime224");
	V.push_back("sha224prime224");
	V.push_back("sha256prime224");
	V.push_back("sha384prime224");
	V.push_back("sha512prime224");
	#endif
	V.push_back("crc32prime256");
	V.push_back("md5prime256");
	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha1prime256");
	V.push_back("sha224prime256");
	V.push_back("sha256prime256");
	V.push_back("sha384prime256");
	V.push_back("sha512prime256");
	#endif
	V.push_back("null");

	#if defined(LIBMAUS2_HAVE_NETTLE)
	V.push_back("sha512primesums");
	V.push_back("sha512primesums512");
	#endif

	#if defined(LIBMAUS2_HAVE_NETTLE) && defined(LIBMAUS2_NETTLE_HAVE_SHA3)
	V.push_back("sha3_256");
	#endif

	#if (!defined(LIBMAUS2_HAVE_NETTLE)) && (defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386) && defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY))
	V.push_back("sha256");
	V.push_back("sha512");
	V.push_back("sha256prime64");
	V.push_back("sha512prime64");
	V.push_back("sha256prime96");
	V.push_back("sha512prime96");
	V.push_back("sha256prime128");
	V.push_back("sha512prime128");
	V.push_back("sha256prime160");
	V.push_back("sha512prime160");
	V.push_back("sha256prime192");
	V.push_back("sha512prime192");
	V.push_back("sha256prime224");
	V.push_back("sha512prime224");
	V.push_back("sha256prime256");
	V.push_back("sha512prime256");
	V.push_back("sha512primesums");
	V.push_back("sha512primesums512");
	#endif

	V.push_back("murmur3");
	V.push_back("murmur3primesums128");

	return V;
}

std::string libmaus2::bambam::ChecksumsFactory::getSupportedHashVariantsList()
{
	std::ostringstream ostr;
	std::vector<std::string> const V = getSupportedHashVariants();

	if ( V.size() )
	{
		ostr << V[0];
		for ( uint64_t i = 1; i < V.size(); ++i )
			ostr << "," << V[i];
	}

	return ostr.str();
}


template<typename header_type>
libmaus2::bambam::ChecksumsInterface::unique_ptr_type constructTemplate(std::string const & hash, header_type const & header)
{
	if ( hash == "crc32prod" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SeqChksumsCRC32Products,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	#if defined(LIBMAUS2_HAVE_NETTLE)
	else if ( hash == "sha1" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386) && defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_SeqChksumsSimpleSums,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_SeqChksumsSimpleSums,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386) && defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_SeqChksumsSimpleSums,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_SeqChksumsSimpleSums,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	#if defined(LIBMAUS2_NETTLE_HAVE_SHA3)
	else if ( hash == "sha3_256" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA3_256_SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	#endif
	#endif
	else if ( hash == "murmur3" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MurmurHash3_x64_128_SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "murmur3primesums128" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MurmurHash3_x64_128_PrimeSums128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime32" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct32,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime64" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct64,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5prime64" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5PrimeProduct64,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime96" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct96,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5prime96" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5PrimeProduct96,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime128" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5prime128" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5PrimeProduct128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime160" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct160,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5prime160" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5PrimeProduct160,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime192" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct192,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5prime192" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5PrimeProduct192,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime224" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct224,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5prime224" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5PrimeProduct224,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "crc32prime256" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<CRC32PrimeProduct256,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "md5prime256" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<MD5PrimeProduct256,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	#if defined(LIBMAUS2_HAVE_NETTLE)
	else if ( hash == "sha1prime64" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1PrimeProduct64,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224prime64" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_PrimeProduct64,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime64" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			//std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct64,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_PrimeProduct64,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384prime64" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_PrimeProduct64,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime64" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			//std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct64,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeProduct64,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha1prime96" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1PrimeProduct96,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224prime96" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_PrimeProduct96,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime96" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			//std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct96,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_PrimeProduct96,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384prime96" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_PrimeProduct96,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime96" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct96,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeProduct96,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha1prime128" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1PrimeProduct128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224prime128" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_PrimeProduct128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime128" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct128,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_PrimeProduct128,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384prime128" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_PrimeProduct128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime128" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct128,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeProduct128,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha1prime160" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1PrimeProduct160,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224prime160" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_PrimeProduct160,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime160" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct160,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_PrimeProduct160,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384prime160" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_PrimeProduct160,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime160" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct160,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeProduct160,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha1prime192" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1PrimeProduct192,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224prime192" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_PrimeProduct192,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime192" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct192,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_PrimeProduct192,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384prime192" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_PrimeProduct192,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime192" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct192,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeProduct192,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha1prime224" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1PrimeProduct224,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224prime224" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_PrimeProduct224,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime224" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct224,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_PrimeProduct224,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384prime224" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_PrimeProduct224,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime224" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct224,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeProduct224,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha1prime256" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA1PrimeProduct256,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha224prime256" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_224_PrimeProduct256,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime256" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_256" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct256,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_PrimeProduct256,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha384prime256" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_384_PrimeProduct256,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime256" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct256,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeProduct256,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	#endif
	else if ( hash == "null" )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<NullChecksums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	#if defined(LIBMAUS2_HAVE_NETTLE)
	else if ( hash == "sha512primesums" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeSums,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeSums,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else if ( hash == "sha512primesums512" )
	{
		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
		{
			// std::cerr << "[V] running sse4 SHA2_512" << std::endl;
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeSums512,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
		else
		#endif
		{
			libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_PrimeSums512,header_type>(hash,header));
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	#endif
	#if (! defined(NETTLE)) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386) && defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
	else if ( hash == "sha256" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_SeqChksumsSimpleSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime64" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct64,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime96" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct96,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime128" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime160" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct160,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime192" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct192,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime224" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct224,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha256prime256" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_256_sse4_PrimeProduct256,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime64" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct64,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime96" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct96,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime128" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct128,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime160" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct160,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime192" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct192,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime224" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct224,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512prime256" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeProduct256,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512primesums" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeSums,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	else if ( hash == "sha512primesums512" && libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(new Checksums<SHA2_512_sse4_PrimeSums512,header_type>(hash,header));
		return UNIQUE_PTR_MOVE(tptr);
	}
	#endif
	else
	{
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "unsupported hash type " << hash << std::endl;
		lme.finish();
		throw lme;
	}
}

libmaus2::bambam::ChecksumsInterface::unique_ptr_type libmaus2::bambam::ChecksumsFactory::construct(std::string const & hash, libmaus2::bambam::BamHeader const & header)
{
	libmaus2::bambam::ChecksumsInterface::unique_ptr_type uptr(constructTemplate(hash,header));
	return UNIQUE_PTR_MOVE(uptr);
}
libmaus2::bambam::ChecksumsInterface::unique_ptr_type libmaus2::bambam::ChecksumsFactory::construct(std::string const & hash, libmaus2::bambam::BamHeaderLowMem const & header)
{
	libmaus2::bambam::ChecksumsInterface::unique_ptr_type uptr(constructTemplate(hash,header));
	return UNIQUE_PTR_MOVE(uptr);
}
