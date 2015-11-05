/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_LCP_SUCCINCTLCP_HPP)
#define LIBMAUS2_LCP_SUCCINCTLCP_HPP

#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/bitio/FastWriteBitWriter.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/util/TempFileContainer.hpp>
#include <libmaus2/util/iterator.hpp>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace libmaus2
{
	namespace lcp
	{
		template<typename _lf_type, typename _sampled_sa_type, typename _sampled_isa_type>
		struct SuccinctLCP
		{
			typedef _lf_type lf_type;
			typedef _sampled_sa_type sampled_sa_type;
			typedef _sampled_isa_type sampled_isa_type;
			typedef SuccinctLCP<lf_type,sampled_sa_type,sampled_isa_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef libmaus2::util::ConstIterator<this_type,uint64_t> const_iterator;

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}

			const_iterator end() const
			{
				return const_iterator(this,n);
			}

			// number of bits required to store l
			static unsigned int bitsPerNum(uint64_t l)
			{
				unsigned int c = 0;

				while ( l )
				{
					c++;
					l>>=1;
				}

				return c;
			}

			// get symbol at position pos
			static inline uint64_t fm(
				uint64_t const pos,
				lf_type const & lf,
				sampled_isa_type const & ISA,
				uint64_t const n
				)
			{
				return lf[ISA[(pos+1)%n]];
			}

			/**
			 * compute LCP array and store it in LCP
			 *
			 * @param y string
			 * @param n length of string
			 * @param p suffix array
			 * @param LCP space to store LCP
			 **/
			template<typename key_iterator, typename elem_type>
			static void computeLCP(key_iterator y, size_t const n, elem_type const * const p, elem_type * LCP)
			{
				::libmaus2::autoarray::AutoArray<elem_type> AR(n,false); elem_type * R = AR.get();

				for ( elem_type i = 0; i < n; ++i ) R[p[i]] = i;

				elem_type l = 0;

				for ( elem_type j = 0; j < n; ++j )
				{
					if ( l > 0 ) l = l-1;
					elem_type i = R[j];

					if ( i != 0 )
					{
						elem_type jp = p[i-1];

						if ( jp > j )
							while ( (jp+l<n) && (y[j+l]==y[jp+l]) ) l++;
						else
							while ( (j+l<n)  && (y[j+l]==y[jp+l]) ) l++;
					}
					else
					{
						l = 0;
					}
					LCP[i] = l;
				}
			}

			static void writePlcpDif(::libmaus2::bitio::FastWriteBitWriterBuffer64Sync & FWBW, uint64_t plcpdif)
			{
				static uint64_t const maxwritebits = 64;

				// std::cerr << "writing " << plcpdif << std::endl;

				while ( plcpdif > maxwritebits )
				{
					FWBW.write(0ull,maxwritebits);
					plcpdif -= maxwritebits;
				}
				assert ( plcpdif <= maxwritebits );
				FWBW.write(1ull,plcpdif);
			}

			/**
			 * write an in memory LCP array as succinct version
			 **/
			template<typename lf_type, typename isa_type, typename lcp_type>
			static void writeSuccinctLCP(
				lf_type & LF,
				isa_type & SISA,
				lcp_type & LCP,
				std::ostream & out,
				libmaus2::util::TempFileContainer & tmpcont,
				bool const verbose = false
			)
			{
				uint64_t const n = LF.getN();

				if ( ! n )
				{
					::libmaus2::serialize::Serialize<uint64_t>::serialize(out,0);
					::libmaus2::serialize::Serialize<uint64_t>::serialize(out,0);
					::libmaus2::serialize::Serialize<uint64_t>::serialize(out,0);
					out.flush();
				}
				else
				{
					#if defined(_OPENMP)
					uint64_t const numthreads = omp_get_max_threads();
					#else
					uint64_t const numthreads = 1;
					#endif

					for ( uint64_t i = 0; i < numthreads; ++i )
						tmpcont.openOutputTempFile(i);

					::libmaus2::autoarray::AutoArray<uint64_t> bitsperthread(numthreads);

					uint64_t const numisa = SISA.SISA.size();
					uint64_t const numisaloop = numisa ? (numisa-1) : 0;

					uint64_t const isaloopsperthread = (numisaloop + numthreads-1)/numthreads;

					uint64_t const rp0 = SISA.SISA[0];
					uint64_t const pdif0 = LCP[rp0] + 1; // store first difference as absolute

					uint64_t const numbits = 2*n + LCP[LF(rp0)];
					::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n);
					::libmaus2::serialize::Serialize<uint64_t>::serialize(out,numbits);
					::libmaus2::serialize::Serialize<uint64_t>::serialize(out,(numbits+63)/64);

					::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(out,8*1024);
					::libmaus2::aio::SynchronousGenericOutput<uint64_t>::iterator_type SGOit(SGO);
					::libmaus2::bitio::FastWriteBitWriterBuffer64Sync FWBW(SGOit);

					uint64_t const isasamplingrate = SISA.isasamplingrate;

					uint64_t bitswritten = 0;
					if ( verbose )
						std::cerr << pdif0 << std::endl;
					writePlcpDif(FWBW,pdif0+1);
					bitswritten += (pdif0+1);

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(numthreads); ++t )
					{
						uint64_t const isapacklow = std::min ( isaloopsperthread*t, numisaloop );
						uint64_t const isapackhigh = std::min ( isapacklow + isaloopsperthread, numisaloop );
						uint64_t const isapackrange = isapackhigh-isapacklow;
						uint64_t const loopstart = isapacklow+1;
						uint64_t const loopend = loopstart + isapackrange;
						::libmaus2::autoarray::AutoArray<uint64_t> plcpbuf(isasamplingrate+1,false);
						uint64_t lbitswritten = 0;

						std::ostream & ltmpfile = tmpcont.getOutputTempFile(t);
						::libmaus2::aio::SynchronousGenericOutput<uint64_t> lSGO(ltmpfile,8*1024);
						::libmaus2::aio::SynchronousGenericOutput<uint64_t>::iterator_type lSGOit(lSGO);
						::libmaus2::bitio::FastWriteBitWriterBuffer64Sync lFWBW(lSGOit);

						// std::cerr << "[" << loopstart << "," << loopend << ")" << " size " << numisa << std::endl;

						for ( uint64_t i = loopstart; i < loopend; ++i )
						{
							uint64_t r = SISA.SISA[i];

							uint64_t * op = plcpbuf.end();
							uint64_t * const opa = plcpbuf.begin()+1;

							while ( op != opa )
							{
								*(--op) = LCP[r];
								r = LF(r);
							}
							*(--op) = LCP[r];

							op = plcpbuf.end()-1;

							while ( op-- != plcpbuf.begin() )
								op[1] = (op[1]+1)-op[0];

							// 1 ...

							for ( op = opa; op != plcpbuf.end(); ++op )
							{
								if ( verbose )
									std::cerr << *op << std::endl;
								writePlcpDif(lFWBW,*op+1);
								lbitswritten += (*op)+1;
							}
						}

						bitsperthread[t] += lbitswritten;

						lFWBW.flush();
						lSGO.flush();
						ltmpfile.flush();

						tmpcont.closeOutputTempFile(t);
					}

					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						std::istream & tmpin = tmpcont.openInputTempFile(t);
						libmaus2::aio::SynchronousGenericInput<uint64_t> SGin(tmpin,8*1024,(bitsperthread[t]+63)/64);
						uint64_t const completeWords = bitsperthread[t]/(8*sizeof(uint64_t));
						uint64_t const restbits = bitsperthread[t] - (8*sizeof(uint64_t)*completeWords);
						uint64_t v = 0;

						// std::cerr << "thread " << t << " complete " << completeWords << " restbits " << restbits << " total bits " << bitsperthread[t] << std::endl;

						for ( uint64_t i = 0; i < completeWords; ++i )
						{
							bool const ok = SGin.getNext(v);
							assert ( ok );
							FWBW.write(v,8*sizeof(uint64_t));
						}

						if ( restbits )
						{
							bool const ok = SGin.getNext(v);
							assert ( ok );
							v >>= (8*sizeof(uint64_t)-restbits);
							FWBW.write(v,restbits);
						}

						tmpcont.closeInputTempFile(t);
					}

					bitswritten += std::accumulate(
						bitsperthread.begin(),
						bitsperthread.end(),
						0ull
					);

					uint64_t const loopprocessed = numisa ? (numisa-1)*isasamplingrate : 0;
					uint64_t const firstprocessed = n ? 1 : 0;
					uint64_t const processed = firstprocessed + loopprocessed;
					uint64_t const rest = LF.getN() - processed;
					::libmaus2::autoarray::AutoArray<uint64_t> plcpbuf(isasamplingrate+1,false);

					if ( verbose )
						std::cerr << "rest=" << rest << std::endl;

					// LF on rank of position 0
					uint64_t rr = LF(rp0);

					for ( uint64_t i = 0; i < rest; ++i )
					{
						plcpbuf[plcpbuf.size()-i-1] = LCP[rr];
						rr = LF(rr);
					}
					plcpbuf[plcpbuf.size()-rest-1] = LCP[rr];

					for ( uint64_t i = 0; i < rest; ++i )
					{
						uint64_t pdif = plcpbuf[plcpbuf.size()-rest+i] + 1 - plcpbuf[plcpbuf.size()-rest+i-1];
						if ( verbose )
							std::cerr << "pdif=" << pdif << std::endl;
						writePlcpDif(FWBW,pdif+1);
						bitswritten += pdif+1;
					}

					FWBW.flush();
					SGO.flush();
					out.flush();

					assert ( numbits == bitswritten );
				}
			}

			/**
			 * compute succinct LCP array. Requires terminator symbol at end of text.
			 *
			 * @param lf LF mapping function
			 * @param SA suffix array
			 * @param ISA inverse suffix array
			 * @param n length of text
			 * @param verbose
			 **/
			static ::libmaus2::autoarray::AutoArray<uint64_t> computeLCP(
				lf_type const & lf,
				sampled_sa_type const & SA,
				sampled_isa_type const & ISA,
				uint64_t const n,
				bool const verbose = false)
			{
				double const bef = clock();
				uint64_t const mask = (1ull << 20)-1;

				::libmaus2::autoarray::AutoArray<uint64_t> AS( (2*n+63)/64);
				uint64_t * const S = AS.get();
				for ( uint64_t i = 0; i < (2*n+63)/64; ++i )
					S[i] = 0;

				uint64_t l0 = 0;
				uint64_t r0 = ISA[0]; // r0 = ISA[j]
				uint64_t ri = r0;
				uint64_t l1 = 0; // l1 = LCP[ ISA[i-1] ]
				uint64_t lcpbits = 0;
				// iterate over positions
				for ( uint64_t i = 0; i < n; ++i )
				{
					if ( l0 > 0 )
						l0 = l0-1;
					else
						ri = lf.phi(ri);

					if ( r0 != 0 )
					{
						// get position for rank r0-1
						uint64_t ip = SA[r0-1];
						uint64_t rip = ISA[(ip+l0+1)%n];

						while ( (i+l0<n) && (ip+l0<n) && (lf[ri] == lf[rip]) )
						{
							l0++;
							ri = lf.phi(ri);
							rip = lf.phi(rip);
						}
					}
					else
					{
						l0 = 0;
					}

					uint64_t const I = l0+1-l1;
					::libmaus2::bitio::putBit(S,lcpbits+I,1);
					lcpbits += (I+1);

					r0 = lf.phi(r0);
					l1 = l0;

					if ( verbose && (!(i & mask)) )
						std::cerr << "\r                                         \r" <<
							static_cast<double>(i)/n << std::flush;
				}

				if ( verbose )
					std::cerr << "\r                                         \r" << 1 << std::endl;

				double const aft = clock();
				if ( verbose )
					std::cerr << "lcpbits = " << lcpbits << " frac " << static_cast<double>(lcpbits)/n << " comptime " << (aft-bef)/CLOCKS_PER_SEC << std::endl;

				return AS;
			}

			/**
			 * compute LCP array with decoded text
			 *
			 * @param lf LF mapping function
			 * @param SA suffix array
			 * @param ISA inverse suffix array
			 * @param n length of text
			 * @param text
			 * @param verbose
			 **/
			template<typename text_type>
			static ::libmaus2::autoarray::AutoArray<uint64_t> computeLCPText(
				lf_type const & lf,
				sampled_sa_type const & SA,
				sampled_isa_type const & ISA,
				uint64_t const n,
				text_type const & text,
				bool const verbose = false
			)
			{
				::libmaus2::timing::RealTimeClock rtc; rtc.start();

				::libmaus2::autoarray::AutoArray<uint64_t> AS( (2*n+63)/64);
				uint64_t * const S = AS.get();
				for ( uint64_t i = 0; i < (2*n+63)/64; ++i )
					S[i] = 0;

				#if defined(_OPENMP)
				uint64_t const subblocksize = 64*1024;
				uint64_t const superblocksize = subblocksize * omp_get_max_threads();
				uint64_t const numsubblocks = (superblocksize + (subblocksize-1)) / subblocksize;
				#else
				uint64_t const subblocksize = 64*1024;
				uint64_t const superblocksize = subblocksize * 1;
				uint64_t const numsubblocks = (superblocksize + (subblocksize-1)) / subblocksize;
				#endif

				::libmaus2::autoarray::AutoArray<uint64_t> R0(superblocksize,false);

				uint64_t const numsuperblocks = (n + (superblocksize-1))/superblocksize;

				uint64_t l0 = 0, l1 = 0, lcpbits = 0;

				for ( uint64_t superblock = 0; superblock < numsuperblocks; ++superblock )
				{
					uint64_t const superlow = superblock * superblocksize;
					uint64_t const superhigh = std::min(n,superlow+superblocksize);
					// uint64_t const superwidth = superhigh-superlow;

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t subblock = 0; subblock < static_cast<int64_t>(numsubblocks); ++subblock )
					{
						uint64_t const sublow = std::min(superlow + subblock * subblocksize,superhigh);
						uint64_t const subhigh = std::min(sublow + subblocksize,superhigh);
						uint64_t const subwidth = subhigh-sublow;

						// std::cerr << "sublow=" << sublow << " subhigh=" << subhigh << " subwidth=" << subwidth << std::endl;

						uint64_t r = ISA[(subhigh % n)];
						uint64_t t = subblock * subblocksize + subwidth;

						for ( uint64_t i = 0; i < subwidth; ++i )
						{
							r = lf(r);
							// std::cerr << "t=" << t << std::endl;
							if ( r )
								R0[--t] = SA[r-1];
							else
								R0[--t] = n;
						}

					}

					for ( uint64_t i = superlow; i < superhigh; ++i )
					{
						if ( l0 > 0 )
							l0 = l0-1;

						uint64_t ip = R0[i-superlow];

						if ( ip != n )
						{
							while ( (i+l0<n) && (ip+l0<n) && (text[i+l0] == text[ip+l0]) )
								l0++;
						}
						else
						{
							l0 = 0;
						}

						uint64_t const I = l0+1-l1;
						::libmaus2::bitio::putBit(S,lcpbits+I,1);
						lcpbits += (I+1);

						l1 = l0;
					}

					if ( verbose )
						std::cerr << "\r                                         \r" << static_cast<double>(superhigh) / n << std::flush;
				}
				if ( verbose )
					std::cerr << "\r                                         \r" << 1 << std::endl;

				if ( verbose )
					std::cerr << "lcpbits = " << lcpbits << " frac " << static_cast<double>(lcpbits)/n << " time " << rtc.getElapsedSeconds() << std::endl;

				return AS;
			}

			// type of select dictionary
			typedef ::libmaus2::rank::ERank222B select_type;
			// length of text
			uint64_t n;
			// suffix array
			sampled_sa_type const * SA;
			// bit representation of lcp array
			::libmaus2::autoarray::AutoArray<uint64_t> ALCP;
			// pointer to start of ALCP
			uint64_t const * LCP;
			// length of bit stream in bits
			uint64_t streambits;
			// select dictionary
			select_type::unique_ptr_type eselect;

			uint64_t byteSize() const
			{
				uint64_t s = 0;

				s += sizeof(uint64_t);
				s += sizeof(sampled_sa_type const *);
				s += ALCP.byteSize();
				s += sizeof(uint64_t const *);
				s += sizeof(uint64_t);
				s += eselect->byteSize();

				return s;
			}

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n);
				s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,streambits);
				s += ALCP.serialize(out);
				return s;
			}

			uint64_t deserialize(std::istream & in, bool const verbose = false)
			{
				uint64_t s = 0;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&n);
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&streambits);
				s += ALCP.deserialize(in);
				LCP = ALCP.get();
				select_type::unique_ptr_type teselect( new select_type( LCP, ((streambits+63)/64)*64 ) );
				eselect = UNIQUE_PTR_MOVE(teselect);
				if ( verbose )
					std::cerr << "LCP: " << s << " bytes = " << s*8 << " bits" << " = " << (s+(1024*1024-1)) / (1024*1024) << " mb" << std::endl;
				return s;
			}

			SuccinctLCP(lf_type const & lf, sampled_sa_type const & rsa, sampled_isa_type const & isa)
			: n(lf.getN()), SA(&rsa), ALCP(computeLCP(lf,rsa,isa,n)), LCP(ALCP.get()),
			  streambits(2*n),
			  eselect( new select_type( LCP, ((streambits+63)/64)*64 ) )
			{}

			template<typename text_type>
			SuccinctLCP(lf_type const & lf, sampled_sa_type const & rsa, sampled_isa_type const & isa, text_type const & text)
			: n(lf.getN()), SA(&rsa), ALCP(computeLCPText(lf,rsa,isa,n,text)), LCP(ALCP.get()),
			  streambits(2*n),
			  eselect( new select_type( LCP, ((streambits+63)/64)*64 ) )
			{}

			SuccinctLCP(std::istream & in, sampled_sa_type const & rsa)
			: SA(&rsa)
			{
				deserialize(in);
			}
			SuccinctLCP(std::istream & in, sampled_sa_type const & rsa, uint64_t & s)
			: SA(&rsa)
			{
				s += deserialize(in);
			}

			static unique_ptr_type load(sampled_sa_type const & rsa, std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance CIS(fn);
				unique_ptr_type ptr(new this_type(CIS,rsa));
				return UNIQUE_PTR_MOVE(ptr);
			}

			uint64_t operator[](uint64_t i) const
			{
				uint64_t const sa = (*SA)[i];
				return eselect->select1(sa) - (sa<<1) - 1;
			}

			uint64_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
		};
	}
}
#endif
