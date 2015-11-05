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

#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/suffixtree/CompressedSuffixTree.hpp>

#include <libmaus2/eta/LinearETA.hpp>
#include <libmaus2/parallel/SynchronousCounter.hpp>
#include <libmaus2/util/ReverseAdapter.hpp>

#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>
#include <libmaus2/lcs/NDextend.hpp>

uint64_t countLeafsByTraversal(libmaus2::suffixtree::CompressedSuffixTree const & CST)
{
	libmaus2::eta::LinearETA eta(CST.n);

	typedef libmaus2::suffixtree::CompressedSuffixTree::Node Node;
	libmaus2::parallel::SynchronousCounter<uint64_t> leafs = 0;
	std::stack< std::pair<Node,uint64_t> > S; S.push(std::pair<Node,uint64_t>(CST.root(),0));
	uint64_t const Sigma = CST.getSigma();
	libmaus2::autoarray::AutoArray<Node> children(Sigma,false);

	std::deque < Node > Q;
	uint64_t const frac = 128;
	uint64_t const maxtdepth = 64;

	while ( S.size() )
	{
		std::pair<Node,uint64_t> const P = S.top(); S.pop();
		Node const & parent = P.first;
		uint64_t const tdepth = P.second;

		if ( CST.count(parent) < 2 || CST.count(parent) <= CST.n/frac || tdepth >= maxtdepth )
		{
			assert ( parent.ep-parent.sp );
			Q.push_back(parent);
		}
		else
		{
			uint64_t const numc = CST.enumerateChildren(parent,children.begin());

			for ( uint64_t i = 0; i < numc; ++i )
				S.push(std::pair<Node,uint64_t>(children[numc-i-1],tdepth+1));
		}
	}

	libmaus2::parallel::OMPLock lock;

	#if defined(_OPENMP)
	#pragma omp parallel
	#endif
	while ( Q.size() )
	{
		libmaus2::autoarray::AutoArray<Node> lchildren(Sigma,false);
		Node node(0,0);

		lock.lock();
		if ( Q.size() )
		{
			node = Q.front();
			Q.pop_front();
		}
		lock.unlock();

		std::stack<Node> SL;
		if ( node.ep-node.sp )
			SL.push(node);

		while ( SL.size() )
		{
			Node const parent = SL.top(); SL.pop();

			if ( parent.ep-parent.sp > 1 )
			{
				uint64_t const numc = CST.enumerateChildren(parent,lchildren.begin());

				for ( uint64_t i = 0; i < numc; ++i )
					SL.push(lchildren[numc-i-1]);
			}
			else
			{
				if ( (leafs++ % (32*1024)) == 0 )
				{
					lock.lock();
					std::cerr << static_cast<double>(leafs)/CST.n << "\t" << eta.eta(leafs) << std::endl;
					lock.unlock();
				}
			}
		}
	}

	std::cerr << "Q.size()=" << Q.size() << " leafs=" << leafs << " n=" << CST.n << std::endl;

	return leafs;
}

template<typename word_type>
word_type rc(word_type v, uint64_t const k)
{
	word_type w = 0;

	for ( uint64_t i = 0; i < k; ++i )
	{
		w <<= 2;
		w |= libmaus2::fastx::invertN(v & 3);
		v >>= 2;
	}

	return w;
}

char decode(char c)
{
	if ( c == 0 )
		return 'l';
	else
		return libmaus2::fastx::remapChar(c-1);
}

std::string decode(std::string s)
{
	for ( uint64_t i = 0; i < s.size(); ++i )
		s[i] = decode(s[i]);
	return s;
}

struct MatchEntry
{
	uint64_t word;
	uint64_t abspos;
	uint64_t offset;
	bool valid;

	MatchEntry() : word(), abspos(), offset(), valid(false) {}
	MatchEntry(uint64_t const rword, uint64_t const rabspos, uint64_t const roffset)
	: word(rword), abspos(rabspos), offset(roffset), valid(true) {}

	std::ostream & print(std::ostream & out, unsigned int const k) const
	{
		libmaus2::fastx::SingleWordDNABitBuffer pb(k);
		pb.buffer = word;
		return out << "MatchEntry(word=" << pb << ",abspos=" << abspos << ",offset=" << offset << ",valid=" << valid << ")";
	}
};

std::ostream & operator<<(std::ostream & out, MatchEntry const & M)
{
	return out << "MatchEntry(word=" << M.word << ",abspos=" << M.abspos << ",offset=" << M.offset << ",valid=" << M.valid << ")";
}

struct MatchEntryAbsPosComparator
{
	bool operator()(MatchEntry const & A, MatchEntry const & B) const
	{
		if ( A.abspos != B.abspos )
			return A.abspos < B.abspos;
		else
			return A.offset < B.offset;
	}
};

void enumerateMulitpleKMers(libmaus2::suffixtree::CompressedSuffixTree const & CST, uint64_t const k)
{
	// get text
	std::cerr << "Decoding text...";
	libmaus2::autoarray::AutoArray<char> text;
	uint64_t const textn = CST.extractTextParallel(text);
	std::cerr << "done." << std::endl;

	std::vector<uint64_t> tpos;
	std::cerr << "Decomposing text into blocks...";
	for ( uint64_t i = 0; i < textn; ++i )
		if ( text[i] == 0 )
			tpos.push_back(i);
	std::cerr << "done." << std::endl;

	#if 0
	uint64_t const emask = libmaus2::math::lowbits(k);
	#endif
	uint64_t const emask1 = k ? libmaus2::math::lowbits(k-1) : 0;

	for ( uint64_t j = 0; j < tpos.size(); ++j )
	{
		uint64_t const low = j?(tpos[j-1]+1):0;
		uint64_t const high = tpos[j];

		std::cerr << "[" << low << "," << high << ")\n";

		if ( high-low >= k )
		{
			std::map < uint64_t, std::set<uint64_t> > Pseen;

			uint64_t z = low;
			libmaus2::fastx::SingleWordDNABitBuffer fb(k), rb(k);
			uint64_t e = 0;

			for ( uint64_t i = 0; i+1 < k; ++i )
			{
				char const c = text[z++]-1;

				e &= emask1;
				e <<= 1;

				if ( c < 4 )
				{
					fb.pushBackMasked(c);
					rb.pushFront(libmaus2::fastx::invertN(c));
				}
				else
				{
					fb.pushBackMasked(0);
					rb.pushFront(0);
					e |= 1;
				}
			}

			while ( z < high )
			{
				char const c = text[z++]-1;

				e &= emask1;
				e <<= 1;

				if ( c < 4 )
				{
					fb.pushBackMasked(c);
					rb.pushFront(libmaus2::fastx::invertN(c));
				}
				else
				{
					fb.pushBackMasked(0);
					rb.pushFront(0);
					e |= 1;
				}

				uint64_t const p0 = z-k;

				while ( Pseen.size() && Pseen.begin()->first < p0 )
					Pseen.erase ( Pseen.begin() );

				// error free?
				if ( ! e )
				{
					libmaus2::suffixtree::CompressedSuffixTree::Node node = CST.root();
					for ( uint64_t x = 0; x < k; ++x )
						node = CST.backwardExtend(node,text[z-x-1]);
					assert ( node.sp != node.ep );

					if ( node.ep - node.sp > 1 )
					{
						bool ok = false;
						std::vector<uint64_t> P1;

						for ( uint64_t i = node.sp; i < node.ep; ++i )
						{
				        		uint64_t const pos = (*(CST.SSA))[i];
				        		if ( pos == p0 )
				        			ok = true;
							else
								P1.push_back(pos);
						}

						std::sort ( P1.begin(), P1.end() );

						for ( uint64_t i = 0; i < P1.size(); ++i )
						{
							uint64_t p1 = P1[i];

							if (
								(
									Pseen.find(p0) == Pseen.end() ||
									Pseen.find(p0)->second.find(p1) == Pseen.find(p0)->second.end()
								)
								#if 1
								&&
								std::abs(static_cast<int64_t>(p0)-static_cast<int64_t>(p1)) >= static_cast<int64_t>(k)
								#endif
							)
							{
								#if 1
								libmaus2::util::ReverseAdapter<char const *> RA(text.begin(),text.begin()+p0+k);
								libmaus2::util::ReverseAdapter<char const *> RB(text.begin(),text.begin()+p1+k);
								libmaus2::util::ConstIterator<libmaus2::util::ReverseAdapter<char const *>,char> CA(&RA);
								libmaus2::util::ConstIterator<libmaus2::util::ReverseAdapter<char const *>,char> CB(&RB);
								libmaus2::lcs::NDextendDNAMapped1 ndr;
								ndr.process<
									libmaus2::util::ConstIterator<libmaus2::util::ReverseAdapter<char const *>,char>,
									libmaus2::util::ConstIterator<libmaus2::util::ReverseAdapter<char const *>,char>
								>(
									CA,p0+k,
									CB,p1+k,
									std::numeric_limits<uint64_t>::max(),
									20,20,
									true /* check self */
								);
								std::pair<uint64_t,uint64_t> Rlen = ndr.getStringLengthUsed();
								uint64_t const ep0 = p0 - (Rlen.first-k);
								uint64_t const ep1 = p1 - (Rlen.second-k);
								#else
								uint64_t const ep0 = p0;
								uint64_t const ep1 = p1;
								#endif

								libmaus2::lcs::NDextendDNAMapped1 nd;
								#if 0
								bool const ok =
								#endif
									nd.process<char const *, char const *>(
									text.begin()+ep0,text.size()-ep0,
									text.begin()+ep1,text.size()-ep1,
									std::numeric_limits<uint64_t>::max(),
									20,
									20,
									true /* check self */
								);

								std::pair<uint64_t,uint64_t> Alen = nd.getStringLengthUsed();

								if ( std::max(Alen.first,Alen.second) >= 100 )
								{
									std::cerr
										<< "region A=[" << ep0 << "," << ep0+Alen.first << ") "
										<< "region B=[" << ep1 << "," << ep1+Alen.second << ") "
										<< p0 << std::endl;
									#if 0
									nd.printAlignmentLines<char const *, char const *, char(*)(char)>(
										std::cerr,
										text.begin()+ep0,Alen.first,
										text.begin()+ep1,Alen.second,
										80,
										decode
									);
									#endif
								}

								std::vector < std::pair<uint64_t,uint64_t> > const koff = nd.getKMatchOffsets(k);

								for ( uint64_t i = 0; i < koff.size(); ++i )
									Pseen [ ep0 + koff[i].first ] .insert ( ep1 + koff[i].second);

								#if 0
								ndr.printAlignmentLines<
									libmaus2::util::ConstIterator<ReverseAdapter<char const *>,char>,
									libmaus2::util::ConstIterator<ReverseAdapter<char const *>,char>,
									char(*)(char)
								>(
									std::cerr,
									CA,Rlen.first,
									CB,Rlen.second,
									80,
									decode
								);
								#endif
							}
						}
						assert ( ok );
					}
				}

				#if 0
				std::cerr << std::string(80,'-') << std::endl;
				std::cerr << fb << std::endl;
				std::cerr << rb << std::endl;
				#endif

				if ( (z & (1024*1024-1)) == 0 )
					std::cerr << z << std::endl;
			}
		}
	}

	#if 0
	#if 0
	for ( uint64_t j = 0; j < tpos.size(); ++j )
	{
		uint64_t const low = j?(tpos[j-1]+1):0;
		uint64_t const high = tpos[j];

		if ( high-low >= k )
		{
			uint64_t z = low;
			libmaus2::fastx::SingleWordDNABitBuffer fb(k), rb(k);

			for ( uint64_t i = 0; i+1 < k; ++i )
			{
				char const c = text[z++]-1;
				fb.pushBackMasked(c);
				rb.pushFront(libmaus2::fastx::invertN(c));
			}

			while ( z < high )
			{
				char const c = text[z++]-1;
				fb.pushBackMasked(c);
				rb.pushFront(libmaus2::fastx::invertN(c));

				#if 0
				std::cerr << std::string(80,'-') << std::endl;
				std::cerr << fb << std::endl;
				std::cerr << rb << std::endl;
				#endif
			}
		}
	}
	#endif

	libmaus2::suffixtree::CompressedSuffixTree::lcp_type const & LCP = *(CST.LCP);
        typedef libmaus2::uint128_t word_type;
	std::vector<word_type> words;

	uint64_t const numthreads = libmaus2::parallel::OMPNumThreadsScope::getMaxThreads();
	uint64_t const tnumblocks = 32*numthreads;
        uint64_t const blocksize = (CST.n + tnumblocks-1)/tnumblocks;
        uint64_t blow = 0;

        std::cerr << "Decoding LCP array...";
	libmaus2::autoarray::AutoArray<uint8_t> ulcp(CST.n);
	#if defined(_OPENMP)
	#pragma omp parallel for schedule(dynamic,1)
	#endif
	for ( uint64_t i = 0; i < CST.n; ++i )
	{
		uint64_t const u = LCP[i];
		if ( u >= std::numeric_limits<uint8_t>::max() )
			ulcp[i] = std::numeric_limits<uint8_t>::max();
		else
			ulcp[i] = u;
	}
	std::cerr << "done." << std::endl;

  	std::cerr << "computing split points";
        std::vector<uint64_t> splitpoints;

        while ( blow < CST.n )
        {
        	std::cerr.put('.');
        	assert ( ulcp[blow] < k );

        	splitpoints.push_back(blow);

        	blow += std::min(blocksize,CST.n-blow);
        	while ( blow < CST.n && ulcp[blow] >= k )
        	{
        		#if 0
        		uint64_t const pos = (*(CST.SSA))[blow];
        		std::cerr << decode(std::string(text.begin()+pos,text.begin()+pos+k)) << std::endl;
        		std::cerr << ulcp[blow] << std::endl;
        		#endif

        		++blow;
		}
        }

        splitpoints.push_back(CST.n);
        std::cerr << "done." << std::endl;

        typedef std::vector < word_type > word_vector_type;
        typedef libmaus2::util::shared_ptr<word_vector_type>::type word_vector_ptr_type;

        std::vector < word_vector_ptr_type > wordvecs(splitpoints.size()-1);

        std::cerr << "computing words...";
        #if defined(_OPENMP)
        #pragma omp parallel for schedule(dynamic,1)
        #endif
        for ( uint64_t t = 0; t < splitpoints.size()-1; ++t )
        {
        	word_vector_ptr_type pwords(new word_vector_type);
        	wordvecs[t] = pwords;
        	std::vector<word_type> & lwords = *pwords;

		uint64_t low = splitpoints[t];

		while ( low < splitpoints[t+1] )
		{
			assert ( ulcp[low] < k );

			uint64_t high = low+1;

			while ( high < splitpoints[t+1] && ulcp[high] >= k )
				++high;

			uint64_t const pos = (*(CST.SSA))[low];
			if ( pos + k <= textn )
			{
				char const * t_f = text.begin()+pos;
				bool ok = true;
				for ( uint64_t i = 0; i < k; ++i )
					if ( t_f[i] < 1 || t_f[i] > 4 )
				 		ok = false;

				if ( ok )
				{
					word_type wf = 0;

					for ( uint64_t i = 0; i < k; ++i )
					{
						wf <<= 2;
						wf |= t_f[i]-1;
					}

					lwords.push_back((wf << 32)|(high-low));
				}
			}

			low = high;
		}
		std::cerr.put('.');
        }
        std::cerr << "done." << std::endl;

        word_vector_type wv;
        for ( uint64_t j = 0; j < wordvecs.size(); ++j )
        {
        	word_vector_type const & lwv = *(wordvecs[j]);
        	for ( uint64_t i = 0; i < lwv.size(); ++i )
        		wv.push_back(lwv[i]);
		wordvecs[j] = word_vector_ptr_type();
        }
        std::sort(wv.begin(),wv.end());

        uint64_t erased = 0;
        for ( uint64_t i = 0; i < wv.size(); ++i )
        {
        	word_type const w = (wv[i] >> 32);
		word_type const r = rc(w,k);

        	#if 0
        	libmaus2::fastx::SingleWordDNABitBuffer SWDB(k);
        	SWDB.buffer = w;
        	std::cerr << SWDB.toString() << std::endl;
        	SWDB.buffer = r;
        	std::cerr << SWDB.toString() << std::endl;
        	#endif

        	if ( w == r )
        	{

        	}
        	else if ( w < r )
        	{
        		word_vector_type::iterator it = std::lower_bound(wv.begin(),wv.end(),r << 32);

        		#if 0
        		if ( it != wv.end() )
        		{
		        	libmaus2::fastx::SingleWordDNABitBuffer SWDB(k);
				SWDB.buffer = r;
		        	std::cerr << "word " << SWDB.toString() << std::endl;
		        	SWDB.buffer = (*it)>>32;
		        	std::cerr << "closest " << SWDB.toString() << std::endl;
        		}
        		#endif

	        	if ( it != wv.end() && (*it>>32) == r )
	        	{
	        		wv[i] =
	        			((wv[i] >> 32) << 32)
	        			|
	        			(
	        				(wv[i] & 0xFFFFFFFFUL)
	        				+
	        				(*it   & 0xFFFFFFFFUL)
					);
				*it = (*it>>32)<<32;
				erased += 1;
	        	}
        	}
        }

        uint64_t o = 0;
        for ( uint64_t i = 0; i < wv.size(); ++i )
        	if ( wv[i] & 0xFFFFFFFFUL )
        		wv[o++] = wv[i];
	wv.resize(o);

	libmaus2::util::Histogram kmerhist;
        for ( uint64_t i = 0; i < wv.size(); ++i )
        	kmerhist ( wv[i] & 0xFFFFFFFFUL );

	kmerhist.print(std::cout);

	uint64_t const k1mask = k ? ::libmaus2::math::lowbits(k-1) : 0;

	// loop over reference sequences (chromosomes)
	for ( uint64_t j = 0; j < tpos.size(); ++j )
	{
		uint64_t const low = j?(tpos[j-1]+1):0;
		uint64_t const high = tpos[j];
		uint64_t emask = 0;

		// if length is at least k
		if ( high-low >= k )
		{
			uint64_t z = low;
			libmaus2::fastx::SingleWordDNABitBuffer fb(k), rb(k);
			libmaus2::fastx::SingleWordDNABitBuffer pb(k);

			// read first k-1 symbols
			for ( uint64_t i = 0; i+1 < k; ++i )
			{
				char const c = text[z++]-1;

				emask &= k1mask;
				emask <<= 1;

				if ( c & 4 )
				{
					emask |= 1;
					fb.pushBackMasked(0);
					rb.pushFront(libmaus2::fastx::invertN(0));
				}
				else
				{
					fb.pushBackMasked(c);
					rb.pushFront(libmaus2::fastx::invertN(c));
					emask |= 0;
				}
			}

			std::vector<uint64_t> frpos;
			std::vector<uint64_t> rrpos;

			// read rest of symbols and compute positions of repetetive kmers on this refseq
			while ( z < high )
			{
				char c = text[z++]-1;

				emask &= k1mask;
				emask <<= 1;

				if ( c & 4 )
				{
					emask |= 1;
					fb.pushBackMasked(0);
					rb.pushFront(libmaus2::fastx::invertN(0));
				}
				else
				{
					fb.pushBackMasked(c);
					rb.pushFront(libmaus2::fastx::invertN(c));
					emask |= 0;
				}

				if ( ! emask )
				{
		        		word_vector_type::const_iterator itf = std::lower_bound(wv.begin(),wv.end(),word_type(fb.buffer) << 32);
		        		word_vector_type::const_iterator itr = std::lower_bound(wv.begin(),wv.end(),word_type(rb.buffer) << 32);

					if ( (itf != wv.end() && (*itf >> 32) == word_type(fb.buffer)) )
						frpos.push_back(z-k);

					if ( (itr != wv.end() && (*itr >> 32) == word_type(rb.buffer)) )
						rrpos.push_back(z-k);
				}

				#if 0
				std::cerr << std::string(80,'-') << std::endl;
				std::cerr << fb << std::endl;
				std::cerr << rb << std::endl;
				#endif
			}

			std::cerr << "refseq/chromosome [" << low << "-" << high << ")=" << (high-low) << "::" << frpos.size() << "," << rrpos.size() << std::endl;

			// decompose list of positions into intervals and process intervals
			uint64_t ilow = 0;
			uint64_t const nearthres = 16+k;
			while ( ilow < frpos.size() )
			{
				uint64_t ihigh = ilow+1;

				uint64_t maxdif = 0;
				while ( ihigh < frpos.size() && frpos[ihigh]-frpos[ihigh-1] <= nearthres )
				{
					maxdif = std::max(maxdif,frpos[ihigh]-frpos[ihigh-1]);
					++ihigh;
				}

				uint64_t const olow = frpos[ilow];
				std::vector<MatchEntry> matches;

				for ( uint64_t y = ilow; y < ihigh; ++y )
				{
					// offset from start of region
					uint64_t const off = frpos[y] - olow;

					// compute match positions
					libmaus2::suffixtree::CompressedSuffixTree::Node node = CST.root();
					pb.reset();
					for ( uint64_t x = 0; x < k; ++x )
					{
						node = CST.backwardExtend(node,text[frpos[y]+k-x-1]);
						pb.pushFront(text[frpos[y]+k-x-1]-1);
					}
					assert ( node.sp < node.ep );

					bool found = false;

					// decode matches
					for ( uint64_t w = node.sp; w < node.ep; ++w )
					{
						uint64_t const abspos = (*(CST.SSA))[w];

						// match in this region
						if ( abspos == frpos[y] )
						{
							found = true;
						}
						// match in other place
						else
						{
							matches.push_back(MatchEntry(pb.buffer,abspos,off));
						}
					}

					assert ( found );
				}

				// sort by position,offset
				std::sort(matches.begin(),matches.end(),MatchEntryAbsPosComparator());

				std::cerr << "ilow=" << ilow << " ihigh=" << ihigh << " maxdif=" << maxdif << " matches.size()=" << matches.size() << std::endl;

				// compute match intervals
				uint64_t alow = 0;
				uint64_t const maxkmerdif = 1024;
				while ( alow < matches.size() )
				{
					uint64_t ahigh = alow+1;
					while ( ahigh < matches.size() && (matches[ahigh].abspos - matches[ahigh-1].abspos < maxkmerdif) )
						++ahigh;

					uint64_t klow = alow;
					std::vector< std::pair<uint64_t,uint64_t> > matchIntervals;

					// sequence of equal kmer
					while ( klow < ahigh )
					{
						uint64_t khigh = klow;
						while ( khigh < ahigh && matches[khigh].word == matches[klow].word )
							++khigh;

						#if 0
						if ( khigh - klow > 1 )
						{
							std::cerr << "ambig " << khigh-klow << std::endl;
							for ( uint64_t i = klow; i < khigh; ++i )
							{
								matches[i].print(std::cerr,k);
								std::cerr << std::endl;
							}						}
						else
						{
							std::cerr << "unique: ";
							matches[klow].print(std::cerr,k);
							std::cerr << std::endl;
						}
						#endif

						matchIntervals.push_back(std::pair<uint64_t,uint64_t>(klow,khigh));

						klow = khigh;
					}

					alow = ahigh;

					bool subdone = false;
					std::vector < std::vector < MatchEntry > > submatches;
					double const erate = 0.15;

					do
					{
						uint64_t o = 0;
						for ( uint64_t i = 0; i < matchIntervals.size(); ++i )
							if ( matchIntervals[i].second != matchIntervals[i].first )
								matchIntervals[o++] = matchIntervals[i];
						matchIntervals.resize(o);
						subdone = (o == 0);

						if ( ! subdone )
						{
							MatchEntry const firstentry = matches[matchIntervals[0].first++];
							std::vector < MatchEntry > lsub;
							lsub.push_back(firstentry);

							for ( uint64_t i = 1; i < matchIntervals.size(); ++i )
							{
								for ( uint64_t j = matchIntervals[i].first; j < matchIntervals[i].second; ++j )
								{
									MatchEntry const & candidate = matches[j];

									if (
										candidate.offset > lsub.back().offset
										&&
										candidate.abspos > lsub.back().abspos
										&&
										std::abs(
											static_cast<int64_t>(candidate.offset - lsub.back().offset)-
											static_cast<int64_t>(candidate.abspos - lsub.back().abspos)
										)
										<= std::max(20.0,
											erate * std::max(
												static_cast<int64_t>(candidate.offset - lsub.back().offset),
												static_cast<int64_t>(candidate.abspos - lsub.back().abspos)
											)
										)
									)
									{
										lsub.push_back(matches[j]);
										std::swap(matches[matchIntervals[i].first],matches[j]);
										matchIntervals[i].first += 1;
										break;
									}
								}
							}

							#if 0
							for ( uint64_t i = 0; i < lsub.size(); ++i )
							{
								std::cerr << lsub[i] << std::endl;
							}
							std::cerr << std::string(80,'*') << std::endl;
							#endif

							submatches.push_back(lsub);
						}
					} while ( ! subdone ) ;

					bool rel = false;
					for ( uint64_t j = 0; j < submatches.size(); ++j )
					{
						std::vector < MatchEntry > const & lsub = submatches[j];

						uint64_t const a_t_base = olow + lsub[0].offset;
						uint64_t const a_t_len  = (lsub.back().offset + k) - lsub[0].offset;
						uint64_t const b_t_base = lsub[0].abspos;
						uint64_t const b_t_len  = (lsub.back().abspos + k) - lsub[0].abspos;

						#if 0
						char const * reg_a_low  = text.begin() + olow + lsub[0].offset;
						char const * reg_a_high = text.begin() + olow + lsub.back().offset + k;
						char const * reg_b_low  = text.begin() + lsub[0].abspos;
						char const * reg_b_high = text.begin() + lsub.back().abspos + k;
						#endif

						char const * reg_a_low = text.begin() + a_t_base;
						char const * reg_a_high = reg_a_low + a_t_len;
						char const * reg_b_low = text.begin() + b_t_base;
						char const * reg_b_high = reg_b_low + b_t_len;

						if ( std::max(a_t_len,b_t_len) >= 100 )
						{
							libmaus2::lcs::NDextendDNAMapped1 nd;
							bool const ok = nd.process(
								reg_a_low,reg_a_high-reg_a_low,reg_b_low,reg_b_high-reg_b_low,
								std::numeric_limits<uint64_t>::max(),
								40
							);

							if ( ok )
							{
								std::vector < libmaus2::lcs::AlignmentTraceContainer::ClipPair > LQ = nd.lowQuality(64,6);

								if ( ! LQ.size() )
								{
									uint64_t apos = a_t_base;
									uint64_t alen = a_t_len;
									uint64_t bpos = b_t_base;
									uint64_t blen = b_t_len;

									std::cerr << "region A [" << apos << "," << apos + alen << ") ";
									std::cerr << "region B [" << bpos << "," << bpos + blen << ") ";
									std::cerr << std::endl;

									std::pair<uint64_t,uint64_t> const sufP = nd.suffixPositive(1,1,1,1);
									alen -= sufP.first;
									blen -= sufP.second;
									std::pair<uint64_t,uint64_t> const preP = nd.prefixPositive(1,1,1,1);
									apos += preP.first;
									bpos += preP.second;
									alen -= preP.first;
									blen -= preP.second;

									nd.printAlignmentLines<char const *, char const *, char(*)(char)>(
										std::cerr,
										text.begin() + apos,
										alen,
										text.begin() + bpos,
										blen,
										80,
										decode
									);
								}
								else
								{
									uint64_t alow = 0;
									uint64_t blow = 0;

									typedef std::pair<uint64_t,uint64_t> up;
									typedef std::pair<up,up> upp;
									std::vector < upp > HQ;

									if (
										LQ.front().A.first > 0 &&
										LQ.front().B.first > 0
									)
									{
										HQ.push_back(
											upp(up(alow,LQ.front().A.first),
											up(blow,LQ.front().B.first))
										);

										#if 0
										libmaus2::lcs::NDextendDNAMapped1 rend;
										bool const subok = rend.process(
											reg_a_low + alow,
											LQ.front().A.first,
											reg_b_low + blow,
											LQ.front().B.first
										);

										rend.printAlignmentLines<char const *, char const *, char(*)(char)>(
											std::cerr,
											reg_a_low + alow,
											LQ.front().A.first,
											reg_b_low + blow,
											LQ.front().B.first,
											80,
											decode
										);
										#endif

										alow += LQ.front().A.first;
										blow += LQ.front().B.first;
									}

									for ( uint64_t i = 0; i < LQ.size(); ++i )
									{
										// skip over low quality area
										alow += LQ[i].A.second - LQ[i].A.first;
										blow += LQ[i].B.second - LQ[i].B.first;

										#if 0
										std::cerr << "LQ[" << i << "]="
											<< "[" << LQ[i].A.first << "," << LQ[i].A.second << ") "
											<< "[" << LQ[i].B.first << "," << LQ[i].B.second << ")\n";
										#endif

										if ( i+1 < LQ.size() )
										{
											#if 0
											std::cerr << "LQ[" << i+1 << "]="
												<< "[" << LQ[i+1].A.first << "," << LQ[i+1].A.second << ") "
												<< "[" << LQ[i+1].B.first << "," << LQ[i+1].B.second << ")\n";
											#endif

											uint64_t const alen = LQ[i+1].A.first - LQ[i].A.second;
											uint64_t const blen = LQ[i+1].B.first - LQ[i].B.second;

											HQ.push_back(
												upp(up(alow,alen),
												up(blow,alen))
											);

											#if 0
											std::cerr << "alen=" << alen << std::endl;
											std::cerr << "blen=" << blen << std::endl;

											libmaus2::lcs::NDextendDNAMapped1 rend;
											bool const subok = rend.process(reg_a_low + alow,alen,reg_b_low + blow,blen);

											rend.printAlignmentLines<char const *, char const *, char(*)(char)>(
												std::cerr,reg_a_low + alow,alen,reg_b_low + blow,blen,80,decode
											);
											#endif

											alow += alen;
											blow += blen;
										}
									}

									if (
										LQ.back().A.second < (reg_a_high-reg_a_low) &&
										LQ.back().B.second < (reg_b_high-reg_b_low)
									)
									{
										uint64_t const alen = (reg_a_high-reg_a_low) - LQ.back().A.second;
										uint64_t const blen = (reg_b_high-reg_b_low) - LQ.back().B.second;

										HQ.push_back(
											upp(up(alow,alen),
											up(blow,alen))
										);

										#if 0
										std::cerr << "alen*=" << alen << std::endl;
										std::cerr << "reg_a_high-reg_a_low=" << reg_a_high-reg_a_low << std::endl;
										std::cerr << "blen*=" << blen << std::endl;
										std::cerr << "reg_b_high-reg_b_low=" << reg_b_high-reg_b_low << std::endl;

										libmaus2::lcs::NDextendDNAMapped1 rend;
										bool const subok = rend.process(reg_a_low + alow,alen,reg_b_low + blow,blen);

										rend.printAlignmentLines<char const *, char const *, char(*)(char)>(
											std::cerr,reg_a_low + alow,alen,reg_b_low + blow,blen,80,decode
										);
										#endif

										alow += alen;
										blow += blen;
									}

									for ( uint64_t i = 0; i < HQ.size(); ++i )
									{
										if ( std::max(HQ[i].first.second,HQ[i].second.second) >= 100 )
										{
											uint64_t apos = (reg_a_low + HQ[i].first.first)-text.begin();
											uint64_t alen = HQ[i].first.second;
											uint64_t bpos = (reg_b_low + HQ[i].second.first)-text.begin();
											uint64_t blen = HQ[i].second.second;

											libmaus2::lcs::NDextendDNAMapped1 rend;
											rend.process(text.begin()+apos,alen,text.begin()+bpos,blen,
												std::numeric_limits<uint64_t>::max(),40);

											std::pair<uint64_t,uint64_t> const sufP = rend.suffixPositive(1,1,1,1);
											alen -= sufP.first;
											blen -= sufP.second;
											std::pair<uint64_t,uint64_t> const preP = rend.prefixPositive(1,1,1,1);
											apos += preP.first;
											bpos += preP.second;
											alen -= preP.first;
											blen -= preP.second;

											std::cerr << "region A ["
												<< apos
												<< ","
												<< (apos+alen) << ") ";
											std::cerr << "region B ["
												<< bpos
												<< ","
												<< (bpos+blen) << ")";
											std::cerr << std::endl;

											rend.printAlignmentLines<char const *, char const *, char(*)(char)>(
												std::cerr,
												text.begin()+apos,alen,text.begin()+bpos,blen,
												80,decode
											);
										}
									}
								}

								// std::cerr << std::string(80,'E') << std::endl;
								rel = true;
							}
						}
					}

					#if 0
					if ( rel )
						std::cerr << std::string(80,'-') << std::endl;
					#endif
				}


				ilow = ihigh;
			}
		}
	}

	// wv now contains the list of kmers with frequencies
	#endif
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const prefix = arginfo.getRestArg<std::string>(0);
		std::string const hwtname = prefix+".hwt";
		std::string const saname = prefix+".sa";
		std::string const isaname = prefix+".isa";
		std::string const lcpname = prefix+".lcp";
		std::string const rmmname = prefix+".rmm";
		std::cerr << "Loading suffix tree...";
		libmaus2::suffixtree::CompressedSuffixTree CST(hwtname,saname,isaname,lcpname,rmmname);
		std::cerr << "done." << std::endl;

		enumerateMulitpleKMers(CST,24);

		#if 0
		libmaus2::autoarray::AutoArray<char> text;
		uint64_t const textn = CST.extractTextParallel(text);

		std::cerr << "textn=" << textn << std::endl;

		for ( uint64_t i = 0; i < textn; ++i )
			if ( text[i] > 0 )
				text[i] = libmaus2::fastx::remapChar(text[i]-1);
			else
				text[i] = '\n';
		std::cout.write(text.begin(),textn);
		#endif

		#if 0
		uint64_t const leafs = countLeafsByTraversal(CST);

		assert ( CST.n == leafs );
		#endif

		#if 0
		#if 0
		// serialise cst and read it back
		std::ostringstream ostr;
		CST.serialise(ostr);
		std::istringstream istr(ostr.str());
		libmaus2::suffixtree::CompressedSuffixTree rCST(istr);
		#endif

		std::cerr << "[0] = " << CST.backwardExtend(CST.root(),0) << std::endl;
		std::cerr << "[1] = " << CST.backwardExtend(CST.root(),1) << std::endl;
		std::cerr << "[2] = " << CST.backwardExtend(CST.root(),2) << std::endl;
		std::cerr << "[3] = " << CST.backwardExtend(CST.root(),3) << std::endl;
		std::cerr << "[4] = " << CST.backwardExtend(CST.root(),4) << std::endl;
		std::cerr << "[5] = " << CST.backwardExtend(CST.root(),5) << std::endl;

		typedef libmaus2::suffixtree::CompressedSuffixTree::Node Node;
		Node node = CST.root();
		std::cerr << CST.parent(CST.firstChild(CST.root())) << std::endl;
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " firstChild=" << CST.firstChild(node) << " next=" << CST.nextSibling(CST.firstChild(node)) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " slink=" << CST.slink(node) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " slink=" << CST.slink(node) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " slink=" << CST.slink(node) << std::endl;

		std::cerr << "LCP[0]=" << (*(CST.LCP))[0] << std::endl;
		std::cerr << "LCP[1]=" << (*(CST.LCP))[1] << std::endl;
		std::cerr << "LCP[2]=" << (*(CST.LCP))[2] << std::endl;
		std::cerr << "LCP[3]=" << (*(CST.LCP))[3] << std::endl;

		node = CST.root();
		std::cerr << "root=" << node << std::endl;
		node = CST.firstChild(node);
		while ( node.sp != node.ep )
		{
			std::cerr << "child " << node << std::endl;
			node = CST.nextSibling(node);
		}

		for ( uint64_t i = 0; i < 7; ++i )
			std::cerr << "["<<i<<"]=" << CST.child(CST.root(),i) << std::endl;

		std::cerr << CST.child(CST.child(CST.root(),1),1) << std::endl;

		std::cerr << "byteSize=" << CST.byteSize() << std::endl;
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
