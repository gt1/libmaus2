/**
    libmaus
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
**/

#if ! defined(CANONICALENCODER_HPP)
#define CANONICALENCODER_HPP

#include <libmaus/util/unordered_map.hpp>
#include <map>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/huffman/huffman.hpp>
#include <libmaus/bitio/readElias.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/math/MetaLog.hpp>
#include <libmaus/huffman/EncodeTable.hpp>
#include <libmaus/huffman/DecodeTable.hpp>
#include <cstdlib>
#include <libmaus/util/ReverseByteOrder.hpp>
#include <libmaus/huffman/FileStreamBaseType.hpp>
#include <libmaus/huffman/BitInputBuffer.hpp>
#include <libmaus/util/SimpleHashMap.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct CanonicalEncoder
		{
			typedef CanonicalEncoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			// typedef ::libmaus::util::unordered_map < int64_t , uint64_t >::type symtorank_type;
			typedef ::libmaus::util::SimpleHashMap<int64_t,uint64_t> symtorank_type;
			typedef symtorank_type::unique_ptr_type symtorank_ptr_type;

			// (symbol, codelength) ordered by rank
			::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > syms;
			::libmaus::autoarray::AutoArray<uint64_t> base;
			::libmaus::autoarray::AutoArray<uint64_t> offset;
			symtorank_ptr_type symtorank;
			
			// decode table
			uint64_t llog;
			::libmaus::autoarray::AutoArray< uint8_t > L;
			
			// encode table
			::libmaus::autoarray::AutoArray< std::pair<uint64_t, unsigned int> > E;

			template<typename writer_type>
			void serialise(writer_type & FWB)
			{
				FWB.writeElias2(syms.size());
				// std::cerr << "Written " << syms.size() << std::endl;
				unsigned int lastsymlen = 0;
				
				for ( uint64_t i = 0; i < syms.size(); ++i )
				{
					unsigned int const len = syms[syms.size()-i-1].second;
					FWB.writeUnary(len-lastsymlen);
					lastsymlen = len;
					FWB.writeElias2(syms[syms.size()-i-1].first);
					
					// std::cerr << "sym " << syms[syms.size()-i-1].first  << " len " << len << std::endl;
				}				
			}

			template<typename reader_type>
			static ::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > deserialise(reader_type & SBIS)
			{
				uint64_t const numsyms = ::libmaus::bitio::readElias2(SBIS);
				//std::cerr << "Read " << numsyms << std::endl;
				::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > syms(numsyms);

				uint64_t lastlen = 0;				
				for ( uint64_t i = 0; i < syms.size(); ++i )
				{
					uint64_t const len = lastlen + ::libmaus::bitio::readUnary(SBIS);
					int64_t const sym = ::libmaus::bitio::readElias2(SBIS);
					syms[syms.size()-i-1] = std::pair<int64_t,uint64_t>(sym,len);
					lastlen = len;
					
					// std::cerr << "sym " << sym << " len " << len << std::endl;
				}
				
				return syms;			
			}

			
			uint64_t getMaxCodeLen() const
			{
				uint64_t maxlen = 0;
				for ( uint64_t i = 0; i < syms.size(); ++i )
					maxlen = std::max(maxlen,syms[i].second);
				return maxlen;
			}
			
			std::string serialise()
			{
				std::ostringstream ostr;
				std::ostream_iterator<uint8_t> ostrit(ostr);
				::libmaus::bitio::FastWriteBitWriterStream8 FWB(ostrit);
				
				serialise(FWB);
				
				FWB.flush();			
				ostr.flush();
				return ostr.str();
			}
			

			static ::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > deserialise(std::string const & serial)
			{
				std::istringstream istr(serial);
				::libmaus::bitio::StreamBitInputStream SBIS(istr);
				return deserialise(SBIS);
			}

			template<bool debug>
			void setupDecodeTableTemplate(uint64_t const rlen)
			{			
				llog = std::min(rlen,static_cast<uint64_t>(getMaxCodeLen()));

				if ( debug )
				{
					std::cerr << "Setting up decode table for code, rlen=" << rlen << " llog=" << llog << std::endl;
					printInt(std::cerr);
				}

				L = ::libmaus::autoarray::AutoArray< uint8_t > (1ull << llog);
				
				if ( debug )
					std::cerr << "L.size()=" << L.size() << std::endl;
				
				for ( uint64_t i = 0; i < syms.size(); ++i )
				{
					uint64_t const codelen = syms[i].second;
					if ( debug )
						std::cerr << "symbol " << syms[i].first << " code length " <<  codelen << std::endl;
					
					if ( codelen <= llog )
					{
						if ( debug )
							std::cerr << "code " << getCode(syms[i].first).first << std::endl;
					
						if ( (!debug) && getCode(syms[i].first).first >= (1ull<<codelen) )
							setupDecodeTableTemplate<true>(rlen);
						if ( (!debug) && getCode(syms[i].first).first >= (1ull<<llog) )
							setupDecodeTableTemplate<true>(rlen);
							
						assert ( getCode(syms[i].first).first < (1ull<<codelen) );
						assert ( getCode(syms[i].first).first < (1ull<<llog) );
					
						uint64_t const codebase = getCode(syms[i].first).first << (llog-codelen);
						uint64_t const numinst = 1ull << (llog-codelen);
						
						if ( debug )
							std::cerr << "codebase=" << codebase << " numinst=" << numinst << std::endl;
						
						for ( uint64_t j = 0; j < numinst; ++j )
						{
							if ( codebase + j >= L.size() )
							{
								setupDecodeTableTemplate<true>(rlen);
							}
							else
							{
								assert ( codebase + j < L.size() );
							}
							L [ codebase + j ] = codelen;
						}
					}
				}
			}

			void setupDecodeTable(uint64_t const rlen)
			{
				setupDecodeTableTemplate<false>(rlen);
			}
			
			struct CodeToTreeQueueElement
			{
				std::pair<uint64_t,uint64_t> interval;
				::libmaus::huffman::HuffmanTreeNode * node;
				uint64_t l;
				
				CodeToTreeQueueElement() : node(0) {}
				CodeToTreeQueueElement(std::pair<uint64_t,uint64_t> const & rinterval,
					::libmaus::huffman::HuffmanTreeNode * rnode,
					uint64_t const rl)
				: interval(rinterval), node(rnode), l(rl)
				{
				
				}
			};
			
			::libmaus::huffman::DecodeTable::unique_ptr_type generateDecodeTable(uint64_t const lookupbits) const
			{
				::libmaus::huffman::HuffmanTreeNode::unique_ptr_type Ptree = codeToTree();
				return ::libmaus::huffman::DecodeTable::unique_ptr_type(new ::libmaus::huffman::DecodeTable(Ptree.get(),lookupbits));
			}

			::libmaus::huffman::EncodeTable<1>::unique_ptr_type generateEncodeTable() const
			{
				::libmaus::huffman::HuffmanTreeNode::unique_ptr_type Ptree = codeToTree();
				return ::libmaus::huffman::EncodeTable<1>::unique_ptr_type(new ::libmaus::huffman::EncodeTable<1>(Ptree.get()));
			}
			
			::libmaus::huffman::HuffmanTreeNode::unique_ptr_type codeToTree() const
			{
				// compute maximal code length
				uint64_t maxl = 0;
				uint64_t numsyms = 0;
				for ( symtorank_type::pair_type const * ita = symtorank->begin();
					ita != symtorank->end(); ++ita )
					if ( ita->first != symtorank_type::unused() )
					{
						int64_t const sym = ita->first;
						uint64_t const len = getCode(sym).second;
						maxl = std::max(maxl,len);		
						numsyms += 1;
					}				
				assert ( maxl <= 64 );

				std::vector < std::pair < uint64_t, int64_t > > cvec(numsyms);
				for ( symtorank_type::pair_type const * ita = symtorank->begin();
					ita != symtorank->end(); ++ita )
					if ( ita->first != symtorank_type::unused() )
					{
						int64_t const sym = ita->first;
						uint64_t const rank = ita->second;
						uint64_t const len = getCode(sym).second;
						uint64_t const code = getCode(sym).first << (maxl-len);
						cvec [ rank ] = std::pair < uint64_t, int64_t >(code,sym);
					}
				
				::std::deque < CodeToTreeQueueElement > Q;
				::libmaus::huffman::HuffmanTreeNode::unique_ptr_type root;
				
				if ( cvec.size() == 1 )
					root = UNIQUE_PTR_MOVE(::libmaus::huffman::HuffmanTreeNode::unique_ptr_type(new ::libmaus::huffman::HuffmanTreeLeaf(cvec[0].second,0)));
				else if ( cvec.size() > 1 )
					root = UNIQUE_PTR_MOVE(::libmaus::huffman::HuffmanTreeNode::unique_ptr_type(new ::libmaus::huffman::HuffmanTreeInnerNode(0,0,0)));
				
				// build tree in breadth first order
				if ( cvec.size() > 1 )
				{
					Q.push_back(CodeToTreeQueueElement ( std::pair<uint64_t,uint64_t>(0,cvec.size()), root.get(), maxl-1 ));
					
					while ( Q.size() )
					{
						CodeToTreeQueueElement const & C = Q.front();
						
						assert ( C.interval.second - C.interval.first );
						// inner node
						if ( C.interval.second - C.interval.first > 1 )
						{
							HuffmanTreeInnerNode * node = dynamic_cast<HuffmanTreeInnerNode *>(C.node);
							assert ( node );
						
							uint64_t numlow = 0;
							uint64_t numhigh = 0;
							for ( uint64_t i = C.interval.first; i < C.interval.second; ++i )
								if ( ((cvec[i].first >> C.l) & 1) )
									numhigh++;
								else
									numlow++;
									
							for ( uint64_t i = 0; i < numlow; ++i )
								assert ( ! ((cvec[C.interval.first + i].first >> C.l) & 1) );
							for ( uint64_t i = 0; i < numhigh; ++i )
								assert ( ((cvec[C.interval.first + numlow + i].first >> C.l) & 1) );
									
							assert ( numlow );
							assert ( numhigh );
							
							if ( numlow == 1 )
							{
								node->left = new HuffmanTreeLeaf(cvec[C.interval.first].second,0);
							}
							else
							{
								node->left = new ::libmaus::huffman::HuffmanTreeInnerNode(0,0,0);
								Q.push_back(CodeToTreeQueueElement ( std::pair<uint64_t,uint64_t>(C.interval.first,C.interval.first+numlow), node->left, C.l-1 ));
							}
							if ( numhigh == 1 )
							{
								node->right = new HuffmanTreeLeaf(cvec[C.interval.second-1].second,0);
							}
							else
							{
								node->right = new ::libmaus::huffman::HuffmanTreeInnerNode(0,0,0);
								Q.push_back(CodeToTreeQueueElement ( std::pair<uint64_t,uint64_t>(C.interval.second-numhigh,C.interval.second), node->right, C.l-1 ));
							}
						}
						
						Q.pop_front();
					}
				}
				
				return UNIQUE_PTR_MOVE(root);
			}

			void setupEncodeTable(uint64_t const size = 256)
			{
				E = ::libmaus::autoarray::AutoArray< std::pair<uint64_t, unsigned int> >(size,false);
				for ( uint64_t i = 0; i < E.size(); ++i )
					E[i] = std::pair<uint64_t, unsigned int>(0,0);

				for ( uint64_t i = 0; i < syms.size(); ++i )
				{
					if ( syms[i].first < static_cast<int64_t>(E.size()) )
					{
						uint64_t const codelen = getCode(syms[i].first).second;
						if ( codelen <= 64 )
						{
							uint64_t const code = getCode(syms[i].first).first;
							E[ syms[i].first ] = std::pair<uint64_t, unsigned int>(code,codelen);
						}
					}
				}
			}

			struct PairCompSec
			{
				bool operator()(std::pair<int64_t,uint64_t> const & A, std::pair<int64_t,uint64_t> const & B) const
				{
					if ( A.second != B.second )
						return A.second > B.second;
					else
						return A.first < B.first;
				}
			};

			std::pair<uint64_t,unsigned int> getCode(int64_t sym) const
			{
				assert ( symtorank->contains(sym) );
				uint64_t const rank = symtorank->get(sym);
				uint64_t const len = syms[rank].second;		
				return std::pair<uint64_t,unsigned int>((base[len] + (rank-offset[len]) ), len);
			}
			
			bool haveSymbol(int64_t const sym) const
			{
				return symtorank->contains(sym);
			}
			
			template<typename writer_type>
			inline void encode(writer_type & W, int64_t const sym) const
			{
				std::pair<uint64_t,unsigned int> const code = getCode(sym);
				W.write(code.first,code.second);
			}

			template<typename writer_type>
			inline void encodeFast(writer_type & W, int64_t const sym) const
			{
				if ( sym < static_cast<int64_t>(E.size()) )
					W.write(E[sym].first,E[sym].second);
				else
					encode(W,sym);
			}
			
			template<typename reader_type>
			int64_t slowDecode(reader_type & R) const
			{
				unsigned int length = 1;
				uint64_t code = R.readBit();
				
				while ( code < base[length] )
				{
					code <<= 1;
					code |= R.readBit();
					length++;
				}
				
				int64_t const rank = offset[length] + code - base[length];
				
				return syms[rank].first;
			}

			template<typename reader_type>
			int64_t fastDecode(reader_type & R) const
			{
				uint64_t code = R.peek(llog);
				unsigned int length = L [ code ];
				
				if ( length )
				{
					assert ( length <= llog );
					code >>= (llog - length);
					int64_t const rank = offset[length] + code - base[length];
					R.erase(length);
					return syms[rank].first;
				}
				else
				{
					R.erase(llog);
					length = llog;
					
					while ( code < base[length] )
					{
						code <<= 1;
						code |= R.readBit();
						length++;
					}
					
					int64_t const rank = offset[length] + code-base[length];
					
					return syms[rank].first;
				}
			}
			
			static std::string bitPrint(int64_t v, int64_t b)
			{
				std::ostringstream ostr;
				
				for ( int64_t i = 0; i < b; ++i )
					ostr << (v & (1 << (b-i-1))?"1":"0");
				
				return ostr.str();
			}
			
			void print(std::ostream & out = std::cerr) const
			{
				for ( uint64_t i = 0; i < syms.size(); ++i )
				{
					int64_t const sym = syms[i].first;

					std::pair<uint64_t,unsigned int> const code = getCode(sym);
					
					out
						<< static_cast<char>(sym)
						<< " "
						<< code.second
						<< " "
						<< bitPrint (code.first,code.second)
						<< std::endl;
				}
			}
			void printInt(std::ostream & out = std::cerr) const
			{
				for ( uint64_t i = 0; i < syms.size(); ++i )
				{
					int64_t const sym = syms[i].first;

					std::pair<uint64_t,unsigned int> const code = getCode(sym);
					
					out
						<< sym
						<< " "
						<< code.second
						<< " "
						<< bitPrint (code.first,code.second)
						<< std::endl;
				}
			}
			
			void init(uint64_t const decodebits)
			{
				if ( syms.size() )
				{
					std::sort(syms.begin(),syms.end(),PairCompSec());
					// longest code
					uint64_t const L = syms[0].second;
					
					if ( L > 64 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "::libmaus::huffman::CanonicalEncoder cannot handle codes longer than 64 bits." << std::endl;
						se.finish();
						throw se;
					}
					
					base = ::libmaus::autoarray::AutoArray<uint64_t>(L+1,false);
					offset = ::libmaus::autoarray::AutoArray<uint64_t>(L+1);
					::libmaus::autoarray::AutoArray<uint64_t> lfreq = ::libmaus::autoarray::AutoArray<uint64_t>(L+1);
					
					// count symbols per length, set offset for each length
					for ( uint64_t i = 0; i < syms.size(); ++i )
					{
						lfreq[syms[i].second]++;
						offset [ syms[syms.size()-i-1].second ] = syms.size()-i-1;
					}
					
					// compute base
					for ( uint64_t l = 0; l <= L; ++l )
					{
						uint64_t t = 0;
							
						for ( uint64_t k = l+1; k <= L; ++k )
							if ( lfreq[k] )
							{
								uint64_t const mk = lfreq[k];
								t += mk * (1ull << (L-k));
							}
							
						uint64_t const b = 1ull << (L-l);
						uint64_t const v = (t + (b-1))/b;
							
						base[l] = v;
					}
					
					unsigned int tablog = 0;
					while ( (1ull << tablog) < syms.size() )
						++tablog;
					double const maxloadf = 0.85;
					if ( syms.size() > maxloadf * (1ull << tablog) )
						tablog++;
					symtorank = UNIQUE_PTR_MOVE(symtorank_ptr_type(new symtorank_type(tablog)));

					// reverse hash table (symbol to rank in sorted table)
					for ( uint64_t i = 0; i < syms.size(); ++i )
					{
						symtorank -> insert ( syms[i].first, i );
						// symtorank [ syms[i].first ] = i;
						assert ( symtorank->contains(syms[i].first) );
						assert ( symtorank->get(syms[i].first) == i );
						// assert ( symtorank.find(syms[i].first)->second == i );
					}
				}	
				
				setupDecodeTable(decodebits);
			}
			
			void init(::libmaus::huffman::HuffmanTreeNode const * root, uint64_t const decodebits)
			{
				// pairs of symbols and depth (code length)
				syms = root->symbolDepthArray();
				init(decodebits);
			}
			
			template<typename iterator>
			CanonicalEncoder(
				iterator a,
				iterator e,
				uint64_t const decodebits = 8
				)
			{
				::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type aroot = ::libmaus::huffman::HuffmanBase::createTree(a,e);	
				init(aroot.get(),decodebits);
			}

			CanonicalEncoder(
				std::map<int64_t,uint64_t> const & F,
				uint64_t const decodebits = 8
				)
			{
				::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type aroot = ::libmaus::huffman::HuffmanBase::createTree(F);
				// std::cerr << "created huffman tree." << std::endl;
				init(aroot.get(),decodebits);
			}
			
			CanonicalEncoder(std::string const & s, uint64_t const decodebits = 8)
			: syms(deserialise(s))
			{
				init(decodebits);
			}
			CanonicalEncoder(
				// symbol depth array
				::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > & rsyms, 
				uint64_t const decodebits = 8
			)
			: syms(rsyms)
			{
				init(decodebits);
			}
			
		};

		struct EscapeCanonicalEncoder : protected CanonicalEncoder
		{
			typedef EscapeCanonicalEncoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef CanonicalEncoder base_type;
			
			static std::map < int64_t,uint64_t > computeFreqMap(
				std::vector < std::pair < uint64_t,uint64_t > > const & freqsyms
			)
			{
				uint64_t restfreq = 0;
				for ( uint64_t i = 63; i < freqsyms.size(); ++i )
					restfreq += freqsyms[i].first;

				std::map<int64_t,uint64_t> freqmap;
				for ( uint64_t i = 0; i < std::min(static_cast<uint64_t>(freqsyms.size()),static_cast<uint64_t>(63ull)); ++i )
					freqmap [ freqsyms[i].second ] = freqsyms[i].first;
				if ( restfreq )
					freqmap [ std::numeric_limits<int>::max() ] = restfreq;
				
				return freqmap;
			}

			EscapeCanonicalEncoder(std::vector < std::pair < uint64_t,uint64_t > > const & freqsyms)
			: CanonicalEncoder(computeFreqMap(freqsyms))
			{
			}
			
			EscapeCanonicalEncoder(::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > & rsyms)
			: CanonicalEncoder(rsyms)
			{
			
			}
			
			template<typename writer_type>
			void encode(writer_type & W, int64_t c)
			{
				if ( base_type::haveSymbol(c) )
					base_type::encode(W,c);
				else
				{
					base_type::encode(W,std::numeric_limits<int>::max());
					W.writeElias2(c);
				}
			}

			template<typename reader_type>
			int64_t slowDecode(reader_type & SBIS)
			{
				int64_t const hsym = base_type::slowDecode(SBIS);
					
				if ( hsym == std::numeric_limits<int>::max() )
					return ::libmaus::bitio::readElias2(SBIS);
				else
					return hsym;
			}
			
			template<typename reader_type>
			int64_t fastDecode(reader_type & SBIS)
			{
				int64_t const hsym = base_type::fastDecode(SBIS);
					
				if ( hsym == std::numeric_limits<int>::max() )
					return ::libmaus::bitio::readElias2(SBIS);
				else
					return hsym;
			}

			template<typename writer_type>
			void serialise(writer_type & FWB)
			{
				base_type::serialise(FWB);
			}

			template<typename iterator>
			static bool needEscape(iterator a, iterator e)
			{
				::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type aroot = ::libmaus::huffman::HuffmanBase::createTree(a,e);	
				return aroot->depth() > 64;			
			}
			static bool needEscape(std::map<int64_t,uint64_t> const & F)
			{
				::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type aroot = ::libmaus::huffman::HuffmanBase::createTree(F);
				return aroot->depth() > 64;
			}
			static bool needEscape(std::vector < std::pair < uint64_t,uint64_t > > const & freqsyms)
			{
				return needEscape(computeFreqMap(freqsyms));
			}
		};
	}
}
#endif
