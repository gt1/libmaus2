/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_FASTX_KMERCOUNT_HPP)
#define LIBMAUS2_FASTX_KMERCOUNT_HPP

#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/fastx/LineBufferFastAReader.hpp>
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/sorting/SortingBufferedOutputFile.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/PrefixSums.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>

namespace libmaus2
{
	namespace fastx
	{
		/*
		 * kmer counting class for small k (<= 16)
		 */
		struct KmerCount
		{
			unsigned int const k;
			uint64_t const numk;
			uint64_t const bitsperword;
			libmaus2::autoarray::AutoArray<uint8_t> A8;
			libmaus2::autoarray::AutoArray<uint64_t> O8;
			libmaus2::rank::ERank222B::unique_ptr_type R8;
			libmaus2::autoarray::AutoArray<uint16_t> A16;
			libmaus2::autoarray::AutoArray<uint64_t> O16;
			libmaus2::rank::ERank222B::unique_ptr_type R16;
			libmaus2::autoarray::AutoArray<uint32_t> A32;
			libmaus2::autoarray::AutoArray<uint64_t> O32;
			libmaus2::rank::ERank222B::unique_ptr_type R32;
			std::map<uint64_t,uint64_t> O64;

			KmerCount(unsigned int const rk) : k(rk), numk(1ull << (2*k)), bitsperword(CHAR_BIT * sizeof(uint64_t))
			{
			}

			struct WordEncoder
			{
				typedef uint64_t data_type;

				std::ostream & out;
				uint64_t n;

				WordEncoder(std::ostream & rout)
				: out(rout), n(0)
				{}

				void put(uint64_t const v)
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,v);
					++n;
				}

				uint64_t getWrittenWords() const
				{
					return n;
				}
			};

			struct WordDecoder
			{
				typedef uint64_t data_type;
				typedef WordDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & in;

				WordDecoder(std::string const & fn, uint64_t const o)
				: PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI)
				{
					in.clear();
					in.seekg(o);
				}

				bool getNext(data_type & v)
				{
					v = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					return true;
				}
			};

			struct BitStreamDecoder
			{
				typedef BitStreamDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & in;
				uint64_t n;
				uint64_t v;
				uint64_t b;

				static uint64_t readNumber(std::istream & in, uint64_t const o)
				{
					in.clear();
					in.seekg(o);
					return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				}

				BitStreamDecoder(std::string const & fn, uint64_t const o)
				: PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI), n(readNumber(in,o)*CHAR_BIT*sizeof(uint64_t)), v(0), b(0)
				{

				}

				bool getNext(bool & r)
				{
					if ( n )
					{
						if ( ! b )
						{
							v = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
							b = 1ull << 63;
						}

						r = (v & b) != 0;
						b >>= 1;
						n--;

						return true;
					}
					else
					{
						return false;
					}
				}
			};

			template<typename _data_type>
			struct ArrayDecoder
			{
				typedef _data_type data_type;
				typedef ArrayDecoder<data_type> this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & in;
				uint64_t n;

				static uint64_t readNumber(std::istream & in, uint64_t const o)
				{
					in.clear();
					in.seekg(o);
					return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				}

				ArrayDecoder(std::string const & fn, uint64_t const o)
				: PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI), n(readNumber(in,o))
				{

				}

				bool getNext(uint64_t & v)
				{
					if ( n )
					{
						v = libmaus2::util::NumberSerialisation::deserialiseNumber(in,sizeof(data_type));
						n -= 1;
						return true;
					}
					else
					{
						return false;
					}
				}
			};

			struct MapDecoder
			{
				typedef MapDecoder this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & in;
				uint64_t n;

				static uint64_t readNumber(std::istream & in, uint64_t const o)
				{
					in.clear();
					in.seekg(o);
					return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				}

				MapDecoder(std::string const & fn, uint64_t const o)
				: PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI), n(readNumber(in,o))
				{

				}

				bool getNext(uint64_t & v)
				{
					if ( n )
					{
						libmaus2::util::NumberSerialisation::deserialiseNumber(in); // key
						v = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
						n -= 1;
						return true;
					}
					else
					{
						return false;
					}
				}
			};

			struct StreamingDecoder
			{
				typedef StreamingDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & in;

				WordDecoder::unique_ptr_type WD;
				libmaus2::gamma::GammaDecoder<WordDecoder>::unique_ptr_type A8;
				BitStreamDecoder::unique_ptr_type O8;
				ArrayDecoder<uint16_t>::unique_ptr_type A16;
				BitStreamDecoder::unique_ptr_type O16;
				ArrayDecoder<uint32_t>::unique_ptr_type A32;
				BitStreamDecoder::unique_ptr_type O32;
				MapDecoder::unique_ptr_type MD;

				uint64_t n;

				StreamingDecoder(std::string const & fn)
				: PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI)
				{
					in.clear();
					in.seekg(-static_cast<int64_t>(sizeof(uint64_t)),std::ios::end);

					uint64_t const off = libmaus2::util::NumberSerialisation::deserialiseNumber(in);

					in.clear();
					in.seekg(off);

					std::vector<uint64_t> O;
					O.resize(libmaus2::util::NumberSerialisation::deserialiseNumber(in));
					for ( uint64_t i = 0; i < O.size(); ++i )
					{
						O[i] = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					}

					{
						WordDecoder::unique_ptr_type tWD(new WordDecoder(fn,O[0]));
						WD = UNIQUE_PTR_MOVE(tWD);
						n = libmaus2::util::NumberSerialisation::deserialiseNumber(WD->in);
						libmaus2::gamma::GammaDecoder<WordDecoder>::unique_ptr_type tA8(new libmaus2::gamma::GammaDecoder<WordDecoder>(*WD));
						A8 = UNIQUE_PTR_MOVE(tA8);
					}

					{
						BitStreamDecoder::unique_ptr_type TO8(new BitStreamDecoder(fn,O[1]));
						O8 = UNIQUE_PTR_MOVE(TO8);
					}

					{
						ArrayDecoder<uint16_t>::unique_ptr_type TA16(new ArrayDecoder<uint16_t>(fn,O[2]));
						A16 = UNIQUE_PTR_MOVE(TA16);
					}

					{
						BitStreamDecoder::unique_ptr_type TO16(new BitStreamDecoder(fn,O[3]));
						O16 = UNIQUE_PTR_MOVE(TO16);
					}

					{
						ArrayDecoder<uint32_t>::unique_ptr_type TA32(new ArrayDecoder<uint32_t>(fn,O[4]));
						A32 = UNIQUE_PTR_MOVE(TA32);
					}

					{
						BitStreamDecoder::unique_ptr_type TO32(new BitStreamDecoder(fn,O[5]));
						O32 = UNIQUE_PTR_MOVE(TO32);
					}

					{
						MapDecoder::unique_ptr_type TMD(new MapDecoder(fn,O[6]));
						MD = UNIQUE_PTR_MOVE(TMD);
					}
				}

				bool getNext(uint64_t & v)
				{
					if ( n )
					{
						bool o8;
						bool const o8ok = O8->getNext(o8);
						assert ( o8ok );

						// no 8 bit overflow
						if ( !o8 )
						{
							v = A8->decode();
						}
						else
						{
							bool o16;
							bool const o16ok = O16->getNext(o16);
							assert ( o16ok );

							// no 16 bit overflow
							if ( !o16 )
							{
								A8->decode();
								bool const a16ok = A16->getNext(v);
								assert ( a16ok );
							}
							else
							{
								bool o32;
								bool const o32ok = O32->getNext(o32);
								assert ( o32ok );

								// no 32 bit overflow
								if ( ! o32 )
								{
									A8->decode(); // skip unused
									A16->getNext(v); // skip unused
									bool const a32ok = A32->getNext(v);
									assert ( a32ok );
								}
								else
								{
									A8->decode(); // skip unused
									A16->getNext(v); // skip unused
									A32->getNext(v); // skip unused
									bool const ok = MD->getNext(v);
									assert ( ok );
								}
							}
						}

						n -= 1;
						return true;
					}
					else
					{
						return false;
					}
				}
			};

			uint64_t serialise(std::string const & fn) const
			{
				libmaus2::aio::OutputStreamInstance OSI(fn);
				return serialise(OSI);
			}

			uint64_t serialise(std::ostream & out) const
			{
				uint64_t o = 0;
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,k);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,numk);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,bitsperword);

				std::vector<uint64_t> O;

				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,A8.size());

				WordEncoder WE(out);
				libmaus2::gamma::GammaEncoder<WordEncoder> GE(WE);
				for ( uint64_t i = 0; i < A8.size(); ++i )
					GE.encode(A8[i]);
				GE.flush();
				o += WE.getWrittenWords() * sizeof(uint64_t);

				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O8.size());
				for ( uint64_t i = 0; i < O8.size(); ++i )
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O8[i],sizeof(uint64_t));

				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,A16.size());
				for ( uint64_t i = 0; i < A16.size(); ++i )
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,A16[i],sizeof(uint16_t));

				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O16.size());
				for ( uint64_t i = 0; i < O16.size(); ++i )
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O16[i],sizeof(uint64_t));

				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,A32.size());
				for ( uint64_t i = 0; i < A32.size(); ++i )
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,A32[i],sizeof(uint32_t));

				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O32.size());
				for ( uint64_t i = 0; i < O32.size(); ++i )
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O32[i],sizeof(uint64_t));

				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O64.size());
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = O64.begin(); ita != O64.end(); ++ita )
				{
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,ita->first);
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,ita->second);
				}

				// push offset for O table
				O.push_back(o);
				o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O.size());
				for ( uint64_t i = 0; i < O.size(); ++i )
					o += libmaus2::util::NumberSerialisation::serialiseNumber(out,O[i]);

				return o;
			}

			struct KmerObject
			{
				uint64_t v;

				KmerObject() {}
				KmerObject(uint64_t const rv) : v(rv) {}
				KmerObject(std::istream & in) : v(libmaus2::util::NumberSerialisation::deserialiseNumber(in)) {}

				std::ostream & serialise(std::ostream & out) const
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,v);
					return out;
				}

				std::istream & deserialise(std::istream & in)
				{
					*this = KmerObject(in);
					return in;
				}

				bool operator<(KmerObject const & O) const
				{
					return v < O.v;
				}
			};

			void fill8(
				std::string const & fn, unsigned int const numthreads,
				std::string const tmpfnbase
			)
			{
				std::vector<std::string> Vtmp(numthreads);
				libmaus2::autoarray::AutoArray<libmaus2::aio::OutputStreamInstance::unique_ptr_type> AOS(numthreads);
				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					std::ostringstream ostr;
					ostr << tmpfnbase << "_fill8_" << std::setw(6) << std::setfill('0') << i << std::setw(0);
					Vtmp[i] = ostr.str();
					libmaus2::util::TempFileRemovalContainer::addTempFile(Vtmp[i]);
					libmaus2::aio::OutputStreamInstance::unique_ptr_type tptr(new libmaus2::aio::OutputStreamInstance(Vtmp[i]));
					AOS[i] = UNIQUE_PTR_MOVE(tptr);
				}

				A8.resize(0);
				A8 = libmaus2::autoarray::AutoArray<uint8_t>(numk,false);
				O8.resize(0);
				O8 = libmaus2::autoarray::AutoArray<uint64_t>(numk/bitsperword,false);

				// #define FILL8_DEBUG
				#if defined(FILL8_DEBUG)
				libmaus2::autoarray::AutoArray<uint64_t> AA(numk);
				#endif

				libmaus2::autoarray::AutoArray<char> Aname;
				libmaus2::autoarray::AutoArray<char> Adata;
				libmaus2::autoarray::AutoArray<uint64_t> Arlen;

				libmaus2::fastx::LineBufferFastAReader LBFAR(fn);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < A8.size(); ++i )
					A8[i] = 0;
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < O8.size(); ++i )
					O8[i] = 0;

				bool reading = true;
				uint64_t const bufsize = 256*1024*1024;
				uint64_t gnreads = 0;

				while ( reading )
				{
					uint64_t o_data = 0;
					uint64_t o_name = 0;
					uint64_t o_rlen = 0;
					uint64_t rlen = 0;
					while ( o_data < bufsize && (reading = LBFAR.getNext(Aname,o_name,Adata,o_data,rlen,false /* term */,true /* map */, false/*pad*/)) )
					{
						Arlen.push(o_rlen,rlen);
					}
					uint64_t const nreads = o_rlen;
					Arlen.push(o_rlen,0);
					libmaus2::util::PrefixSums::parallelPrefixSums(Arlen.begin(),Arlen.begin()+o_rlen,numthreads);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
					#endif
					for ( uint64_t i = 0; i < nreads; ++i )
					{
						#if defined(_OPENMP)
						uint64_t const tid = omp_get_thread_num();
						#else
						uint64_t const tid = 0;
						#endif

						libmaus2::aio::OutputStreamInstance & OS = *(AOS[tid]);

						libmaus2::fastx::SingleWordDNABitBuffer SWDBf(k);
						libmaus2::fastx::SingleWordDNABitBuffer SWDBr(k);

						uint64_t o = Arlen[i];
						uint64_t const oek1 = o + (k-1);
						uint64_t const oe = Arlen[i+1];

						for ( unsigned int i = o; i < oek1; ++i )
						{
							SWDBf.pushBackUnmasked(Adata[i]);
							SWDBr.pushFront(Adata[i]^3);
						}
						for ( uint64_t i = oek1; i < oe; ++i )
						{
							SWDBf.pushBackMasked(Adata[i]);

							if ( __sync_add_and_fetch(A8.begin()+SWDBf.buffer,1) == 0 )
							{
								libmaus2::bitio::setBitSync(O8.begin(),SWDBf.buffer);
								KmerObject(SWDBf.buffer).serialise(OS);
							}
							if ( __sync_add_and_fetch(A8.begin()+SWDBr.buffer,1) == 0 )
							{
								libmaus2::bitio::setBitSync(O8.begin(),SWDBr.buffer);
								KmerObject(SWDBr.buffer).serialise(OS);
							}

							#if defined(FILL8_DEBUG)
							__sync_add_and_fetch(AA.begin() + SWDBf.buffer,1);
							__sync_add_and_fetch(AA.begin() + SWDBr.buffer,1);
							#endif
						}
					}

					gnreads += nreads;

					std::cerr << "[V] KmerCount::fill " << gnreads << std::endl;
				}

				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					AOS[i]->flush();
					AOS[i].reset();
				}

				std::cerr << "[V] KmerCount::fill sorting overflow files...";
				std::ostringstream fnostr;
				fnostr << tmpfnbase << "_fill8";
				std::string const outfn = fnostr.str();
				libmaus2::util::TempFileRemovalContainer::addTempFile(outfn);
				libmaus2::sorting::SerialisingSortingBufferedOutputFile<KmerObject>::reduce(Vtmp,outfn,16384ull /* bs */,1024/*backblocksize*/);
				for ( uint64_t i = 0; i < Vtmp.size(); ++i )
					libmaus2::aio::FileRemoval::removeFile(Vtmp[i]);
				std::cerr << "done." << std::endl;

				libmaus2::rank::ERank222B::unique_ptr_type tR8(new libmaus2::rank::ERank222B(O8.begin(),O8.size() * sizeof(uint64_t) * CHAR_BIT));
				R8 = UNIQUE_PTR_MOVE(tR8);
				uint64_t cnt16 = R8->rank1(O8.size() * sizeof(uint64_t) * CHAR_BIT-1);

				O16.resize(0);
				uint64_t const w16 = (cnt16 + 63)/64;
				O16 = libmaus2::autoarray::AutoArray<uint64_t>(w16,false);
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( uint64_t i = 0; i < w16; ++i )
					O16[i] = 0;
				A16.resize(0);
				A16 = libmaus2::autoarray::AutoArray<uint16_t>(cnt16,false);
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( uint64_t i = 0; i < cnt16; ++i )
					A16[i] = 0;

				#if 0
				assert ( cnt == R8->rank1(O8.size() * sizeof(uint64_t) * CHAR_BIT-1) );
				#endif

				std::ostringstream fn32ostr;
				fn32ostr << tmpfnbase << "_fill8_32";
				std::string const out32fn = fn32ostr.str();
				libmaus2::util::TempFileRemovalContainer::addTempFile(out32fn);

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI(new libmaus2::aio::InputStreamInstance(outfn));
				libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI32(new libmaus2::aio::OutputStreamInstance(out32fn));
				std::pair<uint64_t,uint64_t> P(0,0);
				uint64_t const m8 = static_cast<uint64_t>(std::numeric_limits<uint8_t>::max())+1;
				uint64_t const m16 = static_cast<uint64_t>(std::numeric_limits<uint16_t>::max())+1;
				while ( PISI->peek() != ::std::istream::traits_type::eof() )
				{
					KmerObject K(*PISI);

					if ( K.v != P.first )
					{
						if ( P.second )
						{
							uint64_t const c = (P.second * m8) + A8[P.first];

							if ( c < m16 )
								A16[R8->rankm1(P.first)] = c;
							else
							{
								libmaus2::util::NumberSerialisation::serialiseNumber(*POSI32,P.first);
								libmaus2::util::NumberSerialisation::serialiseNumber(*POSI32,c);
								libmaus2::bitio::setBit(O16.begin(),R8->rankm1(P.first));
							}

							#if defined(FILL8_DEBUG)
							// std::cerr << "overflow " << P.first << " " << P.second << " " << c << " " << AA[P.first] << std::endl;
							assert ( c == AA[P.first] );
							#endif
						}
						P.first = K.v;
						P.second = 0;
					}
					assert ( P.first == K.v );
					P.second += 1;
				}
				if ( P.second )
				{
					uint64_t const c = (P.second * m8) + A8[P.first];

					if ( c < m16 )
						A16[R8->rankm1(P.first)] = c;
					else
					{
						libmaus2::util::NumberSerialisation::serialiseNumber(*POSI32,P.first);
						libmaus2::util::NumberSerialisation::serialiseNumber(*POSI32,c);
						libmaus2::bitio::setBit(O16.begin(),R8->rankm1(P.first));
					}

					#if defined(FILL8_DEBUG)
					// std::cerr << "overflow " << P.first << " " << P.second << " " << c << " " << AA[P.first] << std::endl;
					assert ( c == AA[P.first] );
					#endif
				}
				POSI32->flush();
				POSI32.reset();
				PISI.reset();
				libmaus2::aio::FileRemoval::removeFile(outfn);

				libmaus2::rank::ERank222B::unique_ptr_type tR16(new libmaus2::rank::ERank222B(O16.begin(),O16.size() * sizeof(uint64_t) * CHAR_BIT));
				R16 = UNIQUE_PTR_MOVE(tR16);
				uint64_t cnt32 = R16->rank1(O16.size() * sizeof(uint64_t) * CHAR_BIT-1);
				uint64_t w32 = (cnt32+63)/64;

				O32.resize(0);
				O32 = libmaus2::autoarray::AutoArray<uint64_t>(w32,false);
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( uint64_t i = 0; i < w32; ++i )
					O32[i] = 0;
				A32.resize(0);
				A32 = libmaus2::autoarray::AutoArray<uint32_t>(cnt32,false);
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( uint64_t i = 0; i < cnt32; ++i )
					A32[i] = 0;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI32(new libmaus2::aio::InputStreamInstance(out32fn));
				uint64_t const m32 = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())+1;
				for ( uint64_t j = 0; PISI32->peek() != std::istream::traits_type::eof(); ++j )
				{
					uint64_t const k = libmaus2::util::NumberSerialisation::deserialiseNumber(*PISI32);
					uint64_t const v = libmaus2::util::NumberSerialisation::deserialiseNumber(*PISI32);

					assert ( libmaus2::bitio::getBit(O8.begin(),k) );
					uint64_t const r8 = R8->rankm1(k);
					assert ( libmaus2::bitio::getBit(O16.begin(),r8) );
					uint64_t const r16 = R16->rankm1(r8);
					assert ( r16 == j );

					#if defined(FILL8_DEBUG)
					assert ( AA[k] == v );
					#endif

					if ( v < m32 )
					{
						A32[j] = v;
					}
					else
					{
						libmaus2::bitio::setBit(O32.begin(),j);
						O64[k] = v;
					}
				}
				PISI32.reset();
				libmaus2::aio::FileRemoval::removeFile(out32fn);

				libmaus2::rank::ERank222B::unique_ptr_type tR32(new libmaus2::rank::ERank222B(O32.begin(),O32.size() * sizeof(uint64_t) * CHAR_BIT));
				R32 = UNIQUE_PTR_MOVE(tR32);

				#if defined(FILL8_DEBUG)
				std::cerr << "[V] checking" << std::endl;
				for ( uint64_t i = 0; i < numk; ++i )
				{
					if ( get(i) != AA[i] )
					{
						if ( AA[i] < m8 )
						{
							assert ( libmaus2::bitio::getBit(O8.begin(),i) == 0 );
							assert ( A8[i] == AA[i] );
						}
						else
						{
							assert ( libmaus2::bitio::getBit(O8.begin(),i) );

							uint64_t const r8 = R8->rankm1(i);

							if ( AA[i] < m16 )
							{
								assert ( libmaus2::bitio::getBit(O16.begin(),r8) == 0 );
								assert ( A16[r8] == AA[i] );
							}
							else
							{
								assert ( libmaus2::bitio::getBit(O16.begin(),r8) );

								uint64_t const r16 = R16->rankm1(r8);

								if ( AA[i] < m32 )
								{
									assert ( libmaus2::bitio::getBit(O32.begin(),r16) == 0 );
									assert ( A32[r16] == AA[i] );
								}
								else
								{
									assert ( libmaus2::bitio::getBit(O32.begin(),r16) );
									assert ( O64.find(i) != O64.end() );
									assert ( O64.find(i)->second == AA[i] );
								}
							}
						}

						std::cerr << i << " " << AA[i] << " " << get(i) << std::endl;
						assert ( get(i) == AA[i] );
					}
				}
				#endif

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < A8.size(); ++i )
					if ( libmaus2::bitio::getBit(O8.begin(),i) )
						A8[i] = 0;
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < A16.size(); ++i )
					if ( libmaus2::bitio::getBit(O16.begin(),i) )
						A16[i] = 0;
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < A32.size(); ++i )
					if ( libmaus2::bitio::getBit(O32.begin(),i) )
						A32[i] = 0;
			}

			void fill(std::string const & fn, uint64_t const numthreads, std::string const & tmpfilebase)
			{
				fill8(fn,numthreads,tmpfilebase);

				assert ( R8 );
				assert ( R16 );
				assert ( R32 );
				std::cerr << "[V] KmerCount::fill";
				std::cerr
					<< " 8:0 " << (O8.size() ? R8->rank0(O8.size() * CHAR_BIT * sizeof(uint64_t) - 1) : 0)
					<< " 8:1 " << (O8.size() ? R8->rank1(O8.size() * CHAR_BIT * sizeof(uint64_t) - 1) : 0);
				if ( O16.size() )
					std::cerr << " 16:1 " << (O16.size()?R16->rank1(O16.size() * CHAR_BIT * sizeof(uint64_t) - 1):0);
				if ( O32.size() )
					std::cerr << " 32:1 " << (O32.size()?R32->rank1(O32.size() * CHAR_BIT * sizeof(uint64_t) - 1):0);
				std::cerr << std::endl;
			}

			bool overflow8(uint64_t const v) const
			{
				return libmaus2::bitio::getBit(O8.begin(),v);
			}

			bool overflow16(uint64_t const v) const
			{
				if ( overflow8(v) )
				{
					uint64_t const r8 = R8->rankm1(v);
					return libmaus2::bitio::getBit(O16.begin(),r8);
				}
				else
				{
					return false;
				}
			}

			bool overflow32(uint64_t const v) const
			{
				if ( overflow16(v) )
				{
					uint64_t const r8 = R8->rankm1(v);
					uint64_t const r16 = R16->rankm1(r8);
					return libmaus2::bitio::getBit(O32.begin(),r16);
				}
				else
				{
					return false;
				}
			}

			uint64_t get(uint64_t const v) const
			{
				if ( overflow8(v) )
				{
					uint64_t const r8 = R8->rankm1(v);

					if ( libmaus2::bitio::getBit(O16.begin(),r8) )
					{
						uint64_t const r16 = R16->rankm1(r8);

						if ( libmaus2::bitio::getBit(O32.begin(),r16) )
						{
							assert ( O64.find(r16) != O64.end() );
							return A32[r16] + (1ull << 32) * O64.find(r16)->second;
						}
						else
						{
							return A32[r16];
						}
					}
					else
					{
						return A16[r8];
					}
				}
				else
				{
					return A8[v];
				}
			}

			void print(std::ostream & out, uint64_t const thres = 0)
			{
				libmaus2::fastx::SingleWordDNABitBuffer SWDB(k);
				for ( uint64_t i = 0; i < numk; ++i )
				{
					uint64_t const v = get(i);
					if ( v >= thres )
					{
						SWDB.buffer = i;
						out << SWDB << "\t" << v << '\n';
					}
				}
			}
		};
	}
}
#endif
