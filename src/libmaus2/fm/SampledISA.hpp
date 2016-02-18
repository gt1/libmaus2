/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
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

#if ! defined(SAMPLEDISA_HPP)
#define SAMPLEDISA_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/lf/LF.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>

namespace libmaus2
{
        namespace fm
        {
        	struct SampledISABase
        	{
        		static uint64_t readSamplingRate(std::istream & istr)
        		{
				return libmaus2::serialize::BuiltinLocalSerializer<uint64_t>::deserializeChecked(istr);
        		}

        		static uint64_t readSamplingRate(std::string const & fn)
        		{
				libmaus2::aio::InputStreamInstance ISI(fn);
				return readSamplingRate(ISI);
        		}

                        template<typename writer_type, typename encoder_type>
                        static uint64_t rewriteRankPos(std::istream & istr, writer_type const & writer, encoder_type & encoder)
                        {
				typedef typename writer_type::value_type value_type;
				uint64_t const samplingrate = libmaus2::serialize::BuiltinLocalSerializer<uint64_t>::deserializeChecked(istr);
				uint64_t const nsamples = libmaus2::serialize::BuiltinLocalSerializer<uint64_t>::deserializeChecked(istr);
				writer.writeSize(encoder,nsamples);
				libmaus2::aio::SynchronousGenericInput<uint64_t> Sin(istr,8*1024,nsamples);
				for ( uint64_t i = 0; i < nsamples; ++i )
				{
					uint64_t sample;
					bool const ok = Sin.getNext(sample);
					assert ( ok );
					writer.write(encoder,value_type(sample /* rank */,i*samplingrate /* pos */));
				}
				return samplingrate;
                        }

                        template<typename writer_type, typename encoder_type>
                        static uint64_t rewriteRankPos(std::string const & fn, writer_type const & writer, encoder_type & encoder)
                        {
				libmaus2::aio::InputStreamInstance ISI(fn);
				return rewriteRankPos<writer_type,encoder_type>(ISI,writer,encoder);
                        }

                        template<typename writer_type, typename encoder_type>
                        static uint64_t rewritePosRank(std::istream & istr, writer_type const & writer, encoder_type & encoder)
                        {
				typedef typename writer_type::value_type value_type;
				uint64_t const samplingrate = libmaus2::serialize::BuiltinLocalSerializer<uint64_t>::deserializeChecked(istr);
				uint64_t const nsamples = libmaus2::serialize::BuiltinLocalSerializer<uint64_t>::deserializeChecked(istr);
				writer.writeSize(encoder,nsamples);
				libmaus2::aio::SynchronousGenericInput<uint64_t> Sin(istr,8*1024,nsamples);
				for ( uint64_t i = 0; i < nsamples; ++i )
				{
					uint64_t sample;
					bool const ok = Sin.getNext(sample);
					assert ( ok );
					writer.write(encoder,value_type(i*samplingrate /* pos */,sample /* rank */));
				}
				return samplingrate;
                        }

                        template<typename writer_type, typename encoder_type>
                        static uint64_t rewritePosRank(std::string const & fn, writer_type const & writer, encoder_type & encoder)
                        {
				libmaus2::aio::InputStreamInstance ISI(fn);
				return rewritePosRank<writer_type,encoder_type>(ISI,writer,encoder);
                        }

                        template<typename it>
                        static void writeSampledInverseSuffixArray(
                        	std::ostream & out, it a, uint64_t n, uint64_t samplingrate, uint64_t const inc
			)
                        {
                        	libmaus2::aio::SynchronousGenericOutput<uint64_t> Sout(out,8*1024);
                        	Sout.put(samplingrate);
                        	uint64_t const nwrite = (n + samplingrate-1)/samplingrate;
                        	Sout.put(nwrite);
                        	for ( uint64_t i = 0; i < nwrite; ++i )
				{
					Sout.put(*a);
					a += inc;
				}
				Sout.flush();
				out.flush();
                        }

                        template<typename it>
                        static void writeSampledInverseSuffixArray(
                        	std::string const & fn, it a, uint64_t n, uint64_t samplingrate, uint64_t const inc
			)
                        {
                        	libmaus2::aio::OutputStreamInstance OSI(fn);
                        	writeSampledInverseSuffixArray(OSI,a,n,samplingrate,inc);
                        	OSI.flush();
                        }

                        static uint64_t readUnsignedInt(std::istream & in)
                        {
                                uint64_t i;
                                ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&i);
                                return i;
                        }

                        static uint64_t readUnsignedInt(std::istream & in, uint64_t & s)
                        {
                                uint64_t i;
                                s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&i);
                                return i;
                        }

                        static ::libmaus2::autoarray::AutoArray<uint32_t> readArray32(std::istream & in)
                        {
                                ::libmaus2::autoarray::AutoArray<uint32_t> A;
                                A.deserialize(in);
                                return A;
                        }

                        static ::libmaus2::autoarray::AutoArray<uint32_t> readArray32(std::istream & in, uint64_t & s)
                        {
                                ::libmaus2::autoarray::AutoArray<uint32_t> A;
                                s += A.deserialize(in);
                                return A;
                        }

                        static ::libmaus2::autoarray::AutoArray<uint64_t> readArray64(std::istream & in)
                        {
                                ::libmaus2::autoarray::AutoArray<uint64_t> A;
                                A.deserialize(in);
                                return A;
                        }

                        static ::libmaus2::autoarray::AutoArray<uint64_t> readArray64(std::istream & in, uint64_t & s)
                        {
                                ::libmaus2::autoarray::AutoArray<uint64_t> A;
                                s += A.deserialize(in);
                                return A;
                        }
        	};

                template<typename lf_type>
                struct SampledISA : public SampledISABase
                {
                        typedef SampledISA<lf_type> sampled_isa_type;
                        typedef sampled_isa_type this_type;
                        typedef typename ::libmaus2::util::unique_ptr < this_type >::type unique_ptr_type;

                        lf_type const * lf;
                        uint64_t isasamplingrate;
                        uint64_t isasamplingmask;
                        unsigned int isasamplingshift;
                        ::libmaus2::autoarray::AutoArray<uint64_t> SISA;

                        uint64_t byteSize() const
                        {
                        	return
                        		sizeof(lf_type const *)+
                        		2*sizeof(uint64_t)+
                        		sizeof(unsigned int)+
                        		SISA.byteSize();
                        }

                        void setSamplingRate(uint64_t samplingrate)
                        {
                        	isasamplingrate = samplingrate;
                        	isasamplingmask = isasamplingrate-1;
                        	isasamplingshift = 0;

                        	uint64_t tisasamplingrate = isasamplingrate;

                        	while ( ! (tisasamplingrate&1) )
                        	{
                        		tisasamplingrate >>= 1;
                        		isasamplingshift++;
                        	}
                        	assert ( tisasamplingrate == 1 );
                        	assert ( (1ull << isasamplingshift) == isasamplingrate );
                        }

                        uint64_t serialize(std::ostream & out)
                        {
                                uint64_t s = 0;
                                s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,isasamplingrate);
                                s += SISA.serialize(out);
                                return s;
                        }

                        uint64_t deserialize(std::istream & in)
                        {
                                uint64_t s = 0;
                                setSamplingRate(readUnsignedInt(in,s));
                                SISA = readArray64(in,s);
                                // std::cerr << "ISA: " << s << " bytes = " << s*8 << " bits" << " = " << (s+(1024*1024-1))/(1024*1024) << " mb" << " samplingrate = " << isasamplingrate << std::endl;
                                return s;
                        }

                        static unique_ptr_type load(lf_type const * lf, std::string const & fn)
                        {
                        	libmaus2::aio::InputStreamInstance CIS(fn);
				unique_ptr_type ptr(new this_type(lf,CIS));
                        	return UNIQUE_PTR_MOVE(ptr);
                        }

                        SampledISA(lf_type const * rlf, std::istream & in) : lf(rlf) { deserialize(in); }
                        SampledISA(lf_type const * rlf, std::istream & in, uint64_t & s) : lf(rlf) { s += deserialize(in); }

			SampledISA(
				lf_type const * rlf,
				uint64_t const risasamplingrate,
				::libmaus2::autoarray::AutoArray<uint64_t>  & rSISA)
			: lf(rlf), SISA(rSISA)
			{
				setSamplingRate(risasamplingrate);
			}


                        SampledISA(lf_type const * rlf, uint64_t risasamplingrate, bool const verbose = false)
                        : lf(rlf), isasamplingrate(risasamplingrate),
                          SISA ( (lf->getN() + isasamplingrate - 1)/isasamplingrate )
                        {
				setSamplingRate(risasamplingrate);

				if ( verbose )
	                                std::cerr << "Computing Sampled ISA...";

                                // find rank of position 0 (i.e. search terminating symbol)
                                uint64_t r = lf->zeroPosRank();

                                uint64_t const rr = r;

                                // fill vector
                                if ( verbose )
	                                std::cerr << "(fillingArray...";
                                for ( int64_t i = (lf->getN()-1); i >= 0; --i )
                                {
                                        if ( (i & (1024*1024-1)) == 0 && verbose )
                                                std::cerr << "(" << i/(1024*1024) << ")";

                                        r = (*lf)(r); // LF mapping

                                        if ( i % isasamplingrate == 0 )
                                                SISA[i / isasamplingrate] = r;
                                }
                                if ( verbose )
	                                std::cerr << ")";

                                assert ( r == rr );

        			if ( verbose )
	                                std::cerr << "done." << std::endl;
                        }

                        uint64_t operator[](uint64_t i) const
                        {
                                // next sampled value
                                uint64_t nextpreval = ((i + isasamplingmask) >> isasamplingshift) << isasamplingshift;

                                uint64_t r;

                                if ( nextpreval >= lf->getN() )
                                        r = (*lf)(SISA[0]), // LF
                                        nextpreval = lf->getN()-1;
                                else
                                        r = SISA[ nextpreval >> isasamplingshift ];

                                while ( nextpreval != i )
                                        r = (*lf)(r), // LF
                                        nextpreval--;

                                return r;
                        }
                };
                }
        }
        #endif
