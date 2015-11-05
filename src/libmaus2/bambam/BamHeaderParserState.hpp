/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_BAMHEADERPARSERSTATE_HPP)
#define LIBMAUS2_BAMBAM_BAMHEADERPARSERSTATE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bambam/BamHeaderParserStateBase.hpp>
#include <libmaus2/bambam/Chromosome.hpp>
#include <libmaus2/bambam/DecoderBase.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/util/PutObject.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamHeaderParserState : public ::libmaus2::bambam::BamHeaderParserStateBase, public ::libmaus2::bambam::DecoderBase
		{
			enum bam_header_parse_state
			{
				bam_header_read_magic,
				bam_header_read_l_text,
				bam_header_read_text,
				bam_header_read_n_ref,
				bam_header_reaf_ref_l_name,
				bam_header_read_ref_name,
				bam_header_read_ref_l_ref,
				bam_header_read_done,
				bam_header_read_failed
			};

			size_t byteSize() const
			{
				return
					sizeof(state) +
					sizeof(b_magic_read) +
					sizeof(b_l_text_read) +
					sizeof(l_text) +
					text.byteSize() +
					sizeof(b_text_read) +
					sizeof(b_n_ref) +
					sizeof(n_ref) +
					sizeof(b_ref) +
					sizeof(b_l_name_read) +
					sizeof(l_name) +
					sizeof(b_name_read) +
					name.byteSize() +
					sizeof(b_l_ref_read) +
					sizeof(l_ref); // add chromosomes
			}

			// state
			bam_header_parse_state state;

			// number of magic bytes read
			uint64_t b_magic_read;

			// number of text length bytes read
			uint64_t b_l_text_read;
			// length of text
			uint64_t l_text;
			libmaus2::autoarray::AutoArray<char> text;

			// number of text bytes read
			uint64_t b_text_read;

			// number of nref bytes read
			uint64_t b_n_ref;
			// number of reference sequences
			uint64_t n_ref;
			// number of references read
			uint64_t b_ref;

			// number of name length bytes read
			uint64_t b_l_name_read;
			// length of name
			uint64_t l_name;

			// number of name bytes read
			uint64_t b_name_read;
			// name
			libmaus2::autoarray::AutoArray<char> name;

			// number of reference sequence length bytes read
			uint64_t b_l_ref_read;
			// length of ref seq
			uint64_t l_ref;

			//! chromosome (reference sequence meta data) vector
			std::vector< ::libmaus2::bambam::Chromosome > chromosomes;

			uint64_t getSeriliasedLength() const
			{
				libmaus2::util::CountPutObject CPO;
				serialise(CPO);
				return CPO.c;
			}

			libmaus2::autoarray::AutoArray<char> getSerialised() const
			{
				uint64_t const len = getSeriliasedLength();
				libmaus2::autoarray::AutoArray<char> B(len,false);
				libmaus2::util::PutObject<char *> C(B.begin());
				serialise(C);
				return B;
			}

			template<typename stream_type>
			void serialise(stream_type & out) const
			{
				out.put('B');
				out.put('A');
				out.put('M');
				out.put('\1');

				out.put((l_text >>  0) & 0xFF);
				out.put((l_text >>  8) & 0xFF);
				out.put((l_text >> 16) & 0xFF);
				out.put((l_text >> 24) & 0xFF);
				out.write(text.begin(),l_text);

				out.put((chromosomes.size() >>  0) & 0xFF);
				out.put((chromosomes.size() >>  8) & 0xFF);
				out.put((chromosomes.size() >> 16) & 0xFF);
				out.put((chromosomes.size() >> 24) & 0xFF);
				for ( uint64_t i = 0; i < chromosomes.size(); ++i )
				{
					::libmaus2::bambam::Chromosome const & C = chromosomes[i];
					std::pair<char const *, char const *> name = C.getName();
					size_t const namesize = name.second-name.first;

					out.put(((namesize+1) >>  0) & 0xFF);
					out.put(((namesize+1) >>  8) & 0xFF);
					out.put(((namesize+1) >> 16) & 0xFF);
					out.put(((namesize+1) >> 24) & 0xFF);
					out.write(name.first,namesize);
					out.put(0);

					out.put((C.getLength() >>  0) & 0xFF);
					out.put((C.getLength() >>  8) & 0xFF);
					out.put((C.getLength() >> 16) & 0xFF);
					out.put((C.getLength() >> 24) & 0xFF);
				}
			}

			BamHeaderParserState()
			: state(bam_header_read_magic),
			  b_magic_read(0),
			  b_l_text_read(0),
			  l_text(0),
			  text(),
			  b_text_read(0),
			  b_n_ref(0),
			  n_ref(0),
			  b_ref(0),
			  b_l_name_read(0),
			  l_name(0),
			  b_name_read(0),
			  name(0),
			  b_l_ref_read(0),
			  l_ref(0),
			  chromosomes()
			{

			}

			template<typename stream_type>
			std::pair<bool,uint64_t> parseHeader(stream_type & in, uint64_t const n = std::numeric_limits<uint64_t>::max())
			{
				uint64_t r = 0;

				while ( r != n && this->state != bam_header_read_done && this->state != bam_header_read_failed )
				{
					switch ( this->state )
					{
						case bam_header_read_magic:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							if (
								(this->b_magic_read == 0 && sym == 'B')
								||
								(this->b_magic_read == 1 && sym == 'A')
								||
								(this->b_magic_read == 2 && sym == 'M')
								||
								(this->b_magic_read == 3 && sym == '\1')
							)
							{
								this->b_magic_read++;
							}
							else
							{
								libmaus2::exception::LibMausException se;
								se.getStream() << "Wrong magic number for BAM file." << std::endl;
								se.finish();
								throw se;
							}

							// switch to next state if we got the whole magic
							if ( this->b_magic_read == 4 )
								this->state = bam_header_read_l_text;

							break;
						}
						case bam_header_read_l_text:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							this->l_text |= static_cast<uint64_t>(sym) << (8*this->b_l_text_read);

							this->b_l_text_read++;

							if ( this->b_l_text_read == 4 )
							{
								if ( this->l_text )
								{
									this->text.resize(this->l_text);
									this->state = bam_header_read_text;
								}
								else
									this->state = bam_header_read_n_ref;
							}
							break;
						}
						case bam_header_read_text:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							this->text[this->b_text_read++] = static_cast<char>(static_cast<unsigned char>(sym));

							if ( this->b_text_read == this->l_text )
							{
								// removing padding null bytes
								while ( this->l_text && (!this->text[this->l_text-1]) )
									--this->l_text;

								#if 0
								std::cerr <<
									std::string(
										this->text.begin(),
										this->text.begin()+this->l_text);
								#endif

								this->state = bam_header_read_n_ref;
							}

							break;
						}
						case bam_header_read_n_ref:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							this->n_ref |= static_cast<uint64_t>(sym) << (8*this->b_n_ref);
							this->b_n_ref += 1;

							if ( this->b_n_ref == 4 )
							{
								this->chromosomes.resize(this->n_ref);

								if ( this->n_ref )
									this->state = bam_header_reaf_ref_l_name;
								else
									this->state = bam_header_read_done;
							}

							break;
						}
						case bam_header_reaf_ref_l_name:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							this->l_name |= static_cast<uint64_t>(sym) << (8*this->b_l_name_read);
							this->b_l_name_read += 1;

							if ( this->b_l_name_read == 4 )
							{
								this->b_name_read = 0;
								if ( this->l_name > this->name.size() )
									this->name.resize(this->l_name);

								if ( this->l_name )
									this->state = bam_header_read_ref_name;
								else
									this->state = bam_header_read_ref_l_ref;
							}
							break;
						}
						case bam_header_read_ref_name:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							this->name[this->b_name_read] = static_cast<char>(static_cast<unsigned char>(sym));
							this->b_name_read += 1;

							if ( this->b_name_read == this->l_name )
								this->state = bam_header_read_ref_l_ref;

							break;
						}
						case bam_header_read_ref_l_ref:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							this->l_ref |= static_cast<uint64_t>(sym) << (8*this->b_l_ref_read);
							this->b_l_ref_read += 1;

							if ( this->b_l_ref_read == 4 )
							{
								// remove padding null bytes from name
								while ( this->l_name && (!this->name[this->l_name-1]) )
									-- this->l_name;

								this->chromosomes[this->b_ref] = ::libmaus2::bambam::Chromosome(
									std::string(this->name.begin(),this->name.begin()+this->l_name),
									this->l_ref
								);

								#if 0
								std::cerr <<
									std::string(this->name.begin(),this->name.begin()+this->l_name)
									<< "\t"
									<< this->l_ref << std::endl;
								#endif

								this->b_ref += 1;

								if ( this->b_ref == this->n_ref )
								{
									this->state = bam_header_read_done;
								}
								else
								{
									this->b_l_name_read = 0;
									this->l_name = 0;
									this->b_name_read = 0;
									this->b_l_ref_read = 0;
									this->l_ref = 0;
									this->state = bam_header_reaf_ref_l_name;
								}
							}
							break;
						}
						case bam_header_read_done:
						{
							break;
						}
						case bam_header_read_failed:
						{
							break;
						}
						default:
							this->state = bam_header_read_failed;
							break;
					}
				}

				if ( this->state == bam_header_read_failed )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "BamHeader::parseHeader failed." << std::endl;
					se.finish();
					throw se;
				}

				return
					std::pair<bool,uint64_t>(this->state == bam_header_read_done,r);
			}
		};
	}
}
#endif
