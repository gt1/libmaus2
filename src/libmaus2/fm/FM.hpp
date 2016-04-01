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


#if ! defined(FM_HPP)
#define FM_HPP

#include <libmaus2/lf/LF.hpp>
#include <libmaus2/fm/SampledSA.hpp>
#include <libmaus2/fm/SampledISA.hpp>
#include <libmaus2/lcp/LCP.hpp>
#include <libmaus2/lcp/SuccinctLCP.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
        namespace fm
        {
                template<typename _lf_type>
                struct FM
                {
                	typedef _lf_type lf_type;
                        typedef FM<lf_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

                        typedef ::libmaus2::fm::SimpleSampledSA<lf_type> sampled_sa_type;
                        typedef ::libmaus2::fm::SampledISA<lf_type> sampled_isa_type;
                        typedef ::libmaus2::lcp::SuccinctLCP<lf_type,sampled_sa_type,sampled_isa_type> lcp_type;
			typedef typename lf_type::shared_ptr_type lf_ptr_type;
			typedef typename lcp_type::unique_ptr_type lcp_ptr_type;

                        unsigned int haveReverse;
                        unsigned int haveLCP;
                        lf_ptr_type lf;
                        typename sampled_sa_type::unique_ptr_type sa;
                        typename sampled_isa_type::unique_ptr_type isa;
                        bitio::CompactArray::unique_ptr_type text;
                        lcp_ptr_type lcp;
                        lf_ptr_type lfrev;
                        autoarray::AutoArray<uint64_t> R;
                        autoarray::AutoArray<int> M;
                        autoarray::AutoArray<bool> MActive;

                        void freeISA()
                        {
                                isa.reset(0);
                        }

                        void freeLCP()
                        {
                                lcp.reset(0);
                        }

                        void freeRev()
                        {
                                lfrev.reset(0);
                        }

                        bitio::CompactArray::unique_ptr_type toTextCompact(uint64_t const numthreads = 1) const
                        {
                                bitio::CompactArray::unique_ptr_type text ( new bitio::CompactArray(lf->getN(),lf->getB()) );

                                uint64_t const minoffset = text->minparoffset();

                                #if defined(_OPENMP)
                                uint64_t const blocksize = std::max ( minoffset , static_cast<uint64_t>(1024*1024) );
                                #else
                                uint64_t const blocksize = lf->getN();
                                #endif

                                std::cerr << "extracting text";

                                ::libmaus2::parallel::OMPLock cerrlock;
                                uint64_t blocksfinished = 0;
                                int64_t const numblocks = static_cast<int64_t>((lf->getN() + blocksize-1)/blocksize);

                                #if defined(_OPENMP)
                                #pragma omp parallel for num_threads(numthreads)
                                #endif
                                for ( int64_t block = 0; block < numblocks; ++block )
                                {
                                        uint64_t const low = block * blocksize;
                                        uint64_t const high = std::min( lf->getN(), low + blocksize );
                                        uint64_t const width = high-low;

                                        if ( width )
                                        {
                                                uint64_t r = getISA( (high) % getN());

                                                for ( uint64_t i = 0; i < width; ++i )
                                                {
                                                        text -> set ( high-i-1, (*lf)[r] );
                                                        r = (*lf)(r);
                                                }
                                        }

                                        cerrlock.lock();
                                        blocksfinished++;
                                        double const finished = static_cast<double>(blocksfinished) / numblocks;
                                        std::cerr << "(" << finished << ")";
                                        cerrlock.unlock();
                                        // std::cerr << "block " << block << " low " << low << " high " << high << " width " << width << std::endl;
                                }
                                std::cerr << std::endl;

                                return UNIQUE_PTR_MOVE(text);
                        }

                        uint64_t serialize(std::ostream & out)
                        {
                                uint64_t s = 0;
                                s += ::libmaus2::serialize::Serialize<unsigned int>::serialize(out,haveReverse);
                                s += ::libmaus2::serialize::Serialize<unsigned int>::serialize(out,haveLCP);
                                s += lf->serialize(out);
                                s += sa->serialize(out);
                                s += isa->serialize(out);
                                if ( haveLCP )
                                        s += lcp->serialize(out);
                                if ( haveReverse)
                                        s += lfrev->serialize(out);
                                s += R.serialize(out);
                                return s;
                        }

                        uint64_t deserialize(std::istream & in)
                        {
                                uint64_t s = 0;

                                std::cerr << "FM reading haveReverse...";
                                s += ::libmaus2::serialize::Serialize<unsigned int>::deserialize(in,&haveReverse);
                                std::cerr << haveReverse << std::endl;

                                std::cerr << "FM reading haveLCP...";
                                s += ::libmaus2::serialize::Serialize<unsigned int>::deserialize(in,&haveLCP);
                                std::cerr << haveLCP << std::endl;

                                std::cerr << "FM reading LF..." << std::endl;
                                lf = lf_ptr_type(new lf_type(in,s));
                                std::cerr << "FM reading LF completed" << std::endl;

                                std::cerr << "FM reading SA..." << std::endl;
                                sa = UNIQUE_PTR_MOVE(typename sampled_sa_type::unique_ptr_type(new sampled_sa_type(lf.get(),in,s)));
                                std::cerr << "FM reading SA completed" << std::endl;

                                std::cerr << "FM reading ISA..." << std::endl;
                                isa = UNIQUE_PTR_MOVE(typename sampled_isa_type::unique_ptr_type(new sampled_isa_type(lf.get(),in,s)));
                                std::cerr << "FM reading ISA completed" << std::endl;

                                if ( haveLCP )
                                {
                                        std::cerr << "FM reading LCP..." << std::endl;
                                        lcp = UNIQUE_PTR_MOVE(lcp_ptr_type(new lcp_type(in,*(sa.get()),s)));
                                        std::cerr << "FM reading LCP completed" << std::endl;
                                }
                                if ( haveReverse )
                                {
                                        std::cerr << "FM reading rev LF..." << std::endl;
                                        lfrev = lf_ptr_type(new lf_type(in,s));
                                        std::cerr << "FM reading rev LF completed" << std::endl;
                                }

                                std::cerr << "FM reading R...";
                                s += R.deserialize(in);
                                std::cerr << "done." << std::endl;

                                if ( R.getN() )
                                {
                                        M = MfromR(R);
                                        MActive = MActivefromR(R);
                                }

                                std::cerr << "FM: " << s << " bytes = " << s*8 << " bits = " << (s+(1024*1024-1))/(1024*1024) << " mb" << std::endl;

                                return s;
                        }

                        FM(std::istream & in)
                        {
                                deserialize(in);
                        }

                        FM(std::istream & in, uint64_t & s)
                        {
                                s += deserialize(in);
                        }

                        static autoarray::AutoArray<int> MfromR(autoarray::AutoArray<uint64_t> const & R)
                        {
                                autoarray::AutoArray<int> M(256);

                                for ( uint64_t i = 0; i < 256; ++i )
                                        M[i] = -1;

                                for ( uint64_t i = 0; i < R.getN(); ++i )
                                        if ( R[i] < 256 )
                                                M[R[i]] = i;

                                return M;
                        }

                        static autoarray::AutoArray<bool> MActivefromR(autoarray::AutoArray<uint64_t> const & R)
                        {
                                autoarray::AutoArray<bool> MActive(256);

                                for ( uint64_t i = 0; i < 256; ++i )
                                        MActive[i] = false;

                                for ( uint64_t i = 0; i < R.getN(); ++i )
                                        if ( R[i] < 256 )
                                                MActive[R[i]] = true;

                                return MActive;
                        }

                        FM(lf_ptr_type & rlf, uint64_t sasamplingrate, uint64_t isasamplingrate)
                        : haveReverse(false), haveLCP(false), lf(rlf),
                          sa( new sampled_sa_type(lf.get(), sasamplingrate) ),
                          isa( new sampled_isa_type(lf.get(), isasamplingrate) ),
                          // lcp ( new lcp_type(*lf,*sa,*isa) ),
                          lcp(),
                          lfrev(), R(), M(), MActive()
                        {}

                        FM(
				lf_ptr_type & rlf,
				typename sampled_sa_type::unique_ptr_type & rsa,
				typename sampled_isa_type::unique_ptr_type & risa
				)
                        : haveReverse(false), haveLCP(false), lf(rlf),
                          sa( UNIQUE_PTR_MOVE ( rsa ) ),
                          isa( UNIQUE_PTR_MOVE ( risa ) ),
                          lcp(),
                          lfrev(), R(), M(), MActive()
                        {}

                        FM(
				lf_ptr_type & rlf,
				typename sampled_sa_type::unique_ptr_type & rsa,
				typename sampled_isa_type::unique_ptr_type & risa,
				lf_ptr_type & rlfrev
				)
                        : haveReverse(true), haveLCP(false), lf(rlf),
                          sa( UNIQUE_PTR_MOVE ( rsa ) ),
                          isa( UNIQUE_PTR_MOVE ( risa ) ),
                          lcp(),
                          lfrev(rlfrev), R(), M(), MActive()
                        {}

                        FM(
                                autoarray::AutoArray<uint64_t> & rR,
                                bitio::CompactArray::unique_ptr_type ABWT,
                                uint64_t sasamplingrate,
                                uint64_t isasamplingrate,
                                bitio::CompactArray::unique_ptr_type ABWTrev
                        )
                        : haveReverse(true), haveLCP(true), lf(new lf_type (ABWT)), sa( new sampled_sa_type(lf.get(), sasamplingrate) ),
                          isa( new sampled_isa_type(lf.get(), isasamplingrate) ),
                          lcp ( new lcp_type(*lf,*sa,*isa) ),
                          lfrev(new lf_type(ABWTrev)),
                          R(rR),
                          M(MfromR(R)),
                          MActive(MActivefromR(R))
                        {}

                        FM(
                                autoarray::AutoArray<uint64_t> & rR,
                                bitio::CompactArray::unique_ptr_type ABWT,
                                uint64_t sasamplingrate,
                                uint64_t isasamplingrate,
                                bool usetext = false
                        )
                        : haveReverse(false), haveLCP(true), lf(new lf_type (ABWT)), sa( new sampled_sa_type(lf.get(), sasamplingrate) ),
                          isa( new sampled_isa_type(lf.get(), isasamplingrate) ),
                          text ( usetext ? UNIQUE_PTR_MOVE(toTextCompact()) : bitio::CompactArray::unique_ptr_type() ),
                          lcp ( usetext ? new lcp_type(*lf,*sa,*isa,bitio::CompactArray::const_iterator(text.get())) : new lcp_type(*lf,*sa,*isa) ),
                          R(rR),
                          M(MfromR(R)),
                          MActive(MActivefromR(R))
                        {
                                #if 0
                                std::cerr << "Checking phi function on LF...";
		                #if defined(_OPENMP)
		                uint64_t const numthreads = 1;
		                #pragma omp parallel for num_threads(numthreads)
		                #endif
                                for ( unsigned int r = 0; r < getN(); ++r )
                                        assert ( lf->phi(r) == getISA((getSA(r) + 1) % getN()) );
                                std::cerr << "done." << std::endl;
                                #endif

                                #if 0
                                std::cerr << "Comparing with non-succinct LCP...";
                                std::string text(getN(),' ');
                                for ( unsigned int i = 0; i < getN(); ++i )
                                        text[i] = (*this)[i];
                                autoarray::AutoArray<unsigned int> ASA(getN(),false); unsigned int * const PSA = ASA.get();
                                for ( unsigned int i = 0; i < getN(); ++i )
                                        PSA[i] = getSA(i);
                                autoarray::AutoArray<unsigned int> ALCP(getN(),false);
                                SuccinctLCP::computeLCP(text.c_str(), getN(), PSA, ALCP.get());
                                for ( unsigned int i = 0; i < getN(); ++i )
                                        assert ( (*lcp)[i] == ALCP[i] );
                                std::cerr << "done." << std::endl;
                                #endif
                        }

                        FM(
                                autoarray::AutoArray<uint64_t> & rR,
                                bitio::CompactArray::unique_ptr_type ABWT,
                                uint64_t sasamplingrate,
                                uint64_t isasamplingrate,
                                ::libmaus2::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode,
                                bool usetext = false
                        )
                        : haveReverse(false), haveLCP(true), lf(new lf_type (ABWT,ahnode)), sa( new sampled_sa_type(lf.get(), sasamplingrate) ),
                          isa( new sampled_isa_type(lf.get(), isasamplingrate) ),
                          text ( usetext ? UNIQUE_PTR_MOVE(toTextCompact()) : bitio::CompactArray::unique_ptr_type() ),
                          lcp ( usetext ? new lcp_type(*lf,*sa,*isa,bitio::CompactArray::const_iterator(text.get())) : new lcp_type(*lf,*sa,*isa) ),
                          R(rR),
                          M(MfromR(R)),
                          MActive(MActivefromR(R))
                        {
                                #if 0
                                std::cerr << "Checking phi function on LF...";
                                #if defined(_OPENMP)
                                uint64_t const numthreads = 1;
                                #pragma omp parallel for num_threads(numthreads)
                                #endif
                                for ( unsigned int r = 0; r < getN(); ++r )
                                        assert ( lf->phi(r) == getISA((getSA(r) + 1) % getN()) );
                                std::cerr << "done." << std::endl;
                                #endif

                                #if 0
                                std::cerr << "Comparing with non-succinct LCP...";
                                std::string text(getN(),' ');
                                for ( unsigned int i = 0; i < getN(); ++i )
                                        text[i] = (*this)[i];
                                autoarray::AutoArray<unsigned int> ASA(getN(),false); unsigned int * const PSA = ASA.get();
                                for ( unsigned int i = 0; i < getN(); ++i )
                                        PSA[i] = getSA(i);
                                autoarray::AutoArray<unsigned int> ALCP(getN(),false);
                                SuccinctLCP::computeLCP(text.c_str(), getN(), PSA, ALCP.get());
                                for ( unsigned int i = 0; i < getN(); ++i )
                                        assert ( (*lcp)[i] == ALCP[i] );
                                std::cerr << "done." << std::endl;
                                #endif
                        }

                        uint64_t getN() const { return lf->getN(); }
                        uint64_t getB() const { return lf->getB(); }
                        uint64_t getSA(uint64_t const r) const { return (*sa)[r]; }
                        uint64_t getISA(uint64_t const r) const { return (*isa)[r]; }
                        uint64_t getLCP(uint64_t const r) const { return (*lcp)[r]; }

                        int64_t operator[](uint64_t pos) const
                        {
                                return (*lf)[getISA( (pos+1 ) % getN())];
                                // return (*(lf->W))[getISA( (pos+1 ) % getN())];
                        }

                        template<typename iterator>
                        bool mapAlphabet(iterator a, iterator b)
                        {
                                for ( iterator i = a; i != b; ++i )
                                        if ( (*i >= 256) || (!MActive[*i]) )
                                                return false;

                                for ( iterator i = a; i != b; ++i )
                                        *i = M[*i];

                                return true;
                        }

                        void checkLCP(uint64_t const numthreads = 1)
                        {
                        	#if defined(_OPENMP)
                        	#pragma omp parallel for num_threads(numthreads)
                        	#endif
                                for ( uint64_t r = 1; r < getN(); ++r )
                                {
                                        uint64_t i = getSA(r-1);
                                        uint64_t j = getSA(r);
                                        if ( i > j ) std::swap(i,j);
                                        assert (i < j );
                                        FM const & fm = *this;
                                        lcp_type const & LCP = *lcp;

                                        uint64_t l = 0;
                                        while (
                                                (i != getN())
                                                &&
                                                (fm[i] == fm[j])
                                        )
                                                ++i, ++j, ++l;

                                        if ( l != LCP[r] )
                                        {
                                                std::cerr << l << " != " << LCP[r] << std::endl;
                                        }

                                        assert ( l == LCP[r] );

                                        if ( (r& ((1ull<<16)-1) ) == 0 )
                                                std::cerr << r << "/" << getN() << std::endl;
                                }
                        }

                        template<typename iter>
                        void extract(uint64_t pos, uint64_t len, iter * A) const
                        {
                                uint64_t r = getISA( (pos+len) % getN());

                                for ( int64_t i = (len-1); i >= 0; --i, r = (*lf)(r) )
                                        A[i] = (*(lf->W))[r];
                        }

                        template<typename iter>
                        void extractIterator(uint64_t pos, uint64_t len, iter A) const
                        {
                                uint64_t r = getISA( (pos+len) % getN());

                                for ( int64_t i = (len-1); i >= 0; --i, r = (*lf)(r) )
                                        A[i] = (*(lf->W))[r];
                        }

                        template<typename iter>
                        void extractIteratorParallel(uint64_t pos, uint64_t len, iter A, uint64_t const numthreads) const
                        {
                        	uint64_t const n = getN();
                        	uint64_t const sn = (len + numthreads-1)/numthreads;
                        	uint64_t const numpacks = (len + sn - 1) / sn;

                        	#if defined(_OPENMP)
                        	#pragma omp parallel for num_threads(numpacks)
                        	#endif
                        	for ( uint64_t t = 0; t < numpacks; ++t )
                        	{
                        		uint64_t const low = pos + t*sn;
                        		uint64_t const high = std::min(low+sn,pos+len);
                        		uint64_t r = getISA( high % n );
                        		uint64_t re = getISA(low % n);

                        		iter B = A + high;
                        		if ( high-low )
                        		{
                        			std::pair<int64_t,uint64_t> const P = lf->extendedLF(r);
                        			*(--B) = P.first;
                        			r = P.second;
                        		}

                        		while ( r != re )
                        		{
                        			std::pair<int64_t,uint64_t> const P = lf->extendedLF(r);
                        			*(--B) = P.first;
                        			r = P.second;
					}
                        	}
                        }

                        template<typename iterator>
                        inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
                        {
                                lf->search(query,m,sp,ep);
                        }

                        void extendRight(uint64_t const sym, uint64_t & spf, uint64_t & epf, uint64_t & spr, uint64_t & epr) const
                        {
                                assert ( haveReverse );

                                // forward search using reverse
                                uint64_t smaller, larger;
                                lfrev->W->smallerLarger( sym, spr, epr, smaller, larger );
                                // update straight
                                spf += smaller;
                                epf -= larger;

                                // backward search on reverse
                                spr = lfrev->step(sym, spr);
                                epr = lfrev->step(sym, epr);

                        }

                        void extendLeft(uint64_t const sym, uint64_t & spf, uint64_t & epf, uint64_t & spr, uint64_t & epr) const
                        {
                                // forward search using reverse
                                uint64_t smaller, larger;
                                lf->W->smallerLarger( sym, spf, epf, smaller, larger );
                                // update straight
                                spr += smaller;
                                epr -= larger;

                                // backward search on reverse
                                spf = lf->step(sym, spf);
                                epf = lf->step(sym, epf);
                        }

                        template<typename iterator>
                        void searchPatternSuffix(
                                iterator const & q,
                                uint64_t const m,
                                uint64_t & mright,
                                uint64_t & back_left,
                                uint64_t & back_right) const
                        {
                                mright = 0, back_left = 0, back_right = getN();

                                for ( ; mright < m; ++mright )
                                {
                                        uint64_t const t_back_left = lf->step(q[m-mright-1], back_left);
                                        uint64_t const t_back_right = lf->step(q[m-mright-1], back_right);

                                        if ( t_back_left == t_back_right )
                                                break;

                                        back_left = t_back_left, back_right = t_back_right;
                                }
                        }

                        template<typename iterator>
                        void searchPatternPrefix(
                                iterator const & q,
                                uint64_t const m,
                                uint64_t & mleft,
                                uint64_t & front_left,
                                uint64_t & front_right) const
                        {
                                mleft = 0, front_left = 0, front_right = getN();
                                uint64_t back_left = 0, back_right = getN();

                                for ( ; mleft < m; ++mleft )
                                {
                                        uint64_t t_front_left = front_left, t_front_right = front_right;

                                        extendRight(q[mleft], t_front_left, t_front_right, back_left, back_right);

                                        if ( t_front_left == t_front_right )
                                                break;

                                        front_left = t_front_left, front_right = t_front_right;
                                }
                        }
                };
        }
}
#endif
