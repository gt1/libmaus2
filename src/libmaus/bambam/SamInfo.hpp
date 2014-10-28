/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_SAMINFO_HPP)
#define LIBMAUS_BAMBAM_SAMINFO_HPP

#include <libmaus/bambam/SamInfoBase.hpp>
#include <libmaus/bambam/BamFlagBase.hpp>
#include <libmaus/bambam/CigarOperation.hpp>
#include <libmaus/bambam/BamAlignmentEncoderBase.hpp>
#include <libmaus/bambam/BamHeaderLowMem.hpp>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/math/DecimalNumberParser.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct SamInfo : public libmaus::bambam::SamInfoBase
		{
			typedef SamInfo this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			enum
			{
				sam_info_mandatory_columns = 11
			};
						
			char const * qname;
			sam_info_base_field_status qnamedefined;
			size_t qnamelen;
			
			int32_t flag;

			char const * rname;
			sam_info_base_field_status rnamedefined;
			size_t rnamelen;
			
			int32_t pos;
			int32_t mapq;

			char const * cigar;
			sam_info_base_field_status cigardefined;	
			size_t cigarlen;

			char const * rnext;
			sam_info_base_field_status rnextdefined;
			size_t rnextlen;

			int32_t pnext;
			int32_t tlen;

			char const * seq;
			sam_info_base_field_status seqdefined;
			size_t seqlen;

			libmaus::autoarray::AutoArray<char> tqual;
			char const * qual;
			sam_info_base_field_status qualdefined;
			size_t quallen;
			
			c_ptr_type_pair fields[sam_info_mandatory_columns];
			
			std::vector<libmaus::bambam::cigar_operation> cigopvec;
			uint64_t cigopvecfill;

			BamSeqEncodeTable const seqenc;
			
			libmaus::bambam::BamHeader const & header;

			//! trie for sequence names
			::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type SQTrie;

			std::vector<unsigned int> numLengthUnsigned;

			::libmaus::fastx::EntityBuffer<uint8_t,libmaus::bambam::BamAlignment::D_array_alloc_type> buffer;
			libmaus::bambam::BamAlignment::unique_ptr_type Palgn;
			libmaus::bambam::BamAlignment & algn;

			/**
			 * compute trie for sequence names
			 *
			 * @param header SAM header
			 * @return trie for sequence names
			 **/
			static ::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type computeSQTrie(libmaus::bambam::BamHeader const & header)
			{
				::libmaus::trie::Trie<char> trienofailure;
				std::vector<std::string> dict;
				for ( uint64_t i = 0; i < header.getNumRef(); ++i )
					dict.push_back(header.getRefIDName(i));
				trienofailure.insertContainer(dict);
				::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHTnofailure 
					(trienofailure.toLinearHashTrie<uint32_t>());

				return UNIQUE_PTR_MOVE(LHTnofailure);
			}

			int64_t getRefIdForName(char const * a, char const * e) const
			{
				return SQTrie->searchCompleteNoFailure(a,e);
			}
			
			static unsigned int computeNumLengthUnsigned(uint64_t n)
			{
				unsigned int c = 0;
				while ( n )
				{
					c += 1;
					n /= 10;
				}
				
				return c;
			}
			
			static std::vector<unsigned int> computeNumLengthUnsigned()
			{
				std::vector<unsigned int> V(9);
				V[1] = computeNumLengthUnsigned(std::numeric_limits<uint8_t>::max());
				V[2] = computeNumLengthUnsigned(std::numeric_limits<uint16_t>::max());
				V[4] = computeNumLengthUnsigned(std::numeric_limits<uint32_t>::max());
				V[8] = computeNumLengthUnsigned(std::numeric_limits<uint64_t>::max());
				return V;
			}
			

			SamInfo(libmaus::bambam::BamHeader const & rheader) 
			: qname(0), rname(0), cigar(0), rnext(0), seq(0), qual(0),
			  header(rheader), SQTrie(computeSQTrie(header)), numLengthUnsigned(computeNumLengthUnsigned()), Palgn(new libmaus::bambam::BamAlignment), algn(*Palgn)
			{
			
			}
			
			SamInfo(libmaus::bambam::BamHeader const & rheader, libmaus::bambam::BamAlignment & ralgn)
			: qname(0), rname(0), cigar(0), rnext(0), seq(0), qual(0),
			  header(rheader), SQTrie(computeSQTrie(header)), numLengthUnsigned(computeNumLengthUnsigned()), Palgn(), algn(ralgn)
			{
			
			}

			void parseCigar(char const * p, char const * const pe)
			{
				cigopvecfill = 0;

				while ( p != pe )
				{
					if ( ! DT[static_cast<uint8_t>(*p)] )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseCigar: invalid cigar string " << std::string(cigar,cigar+cigarlen) << " (number expected)\n";
						lme.finish();
						throw lme;
					}

					uint64_t num = *(p++) - '0';
					
					while ( p != pe && DT[static_cast<uint8_t>(*p)] )
					{
						num *= 10;
						num += (*(p++))-'0';
					}
					
					libmaus::bambam::BamFlagBase::bam_cigar_ops op;
					
					if ( p == pe )
					{					
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseCigar: invalid cigar string " << std::string(cigar,cigar+cigarlen) << " (string ends on number)\n";
						lme.finish();
						throw lme;
					}

					switch ( *(p++) )
					{
						case 'M':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CMATCH;
							break;
						case 'I':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CINS;
							break;
						case 'S':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CSOFT_CLIP;
							break;
						case '=':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CEQUAL;
							break;
						case 'X':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDIFF;
							break;
						case 'D':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDEL;
							break;
						case 'N':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CREF_SKIP;
							break;
						case 'H':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CHARD_CLIP;
							break;
						case 'P':
							op = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CPAD;
							break;
						default:
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "libmaus::bambam::SamInfo::parseCigar: invalid cigar operator " << p[-1] << "\n";
							lme.finish();
							throw lme;
						}
					}
				
					if ( cigopvecfill >= cigopvec.size() )
						cigopvec.push_back(libmaus::bambam::cigar_operation(op,num));
					else	
						cigopvec[cigopvecfill] = libmaus::bambam::cigar_operation(op,num);
					
					cigopvecfill++;
				}			
			}
			
			template<typename number_type, char dt>
			void parseNumberArray(char const * pc, char const * p, char const * pp)
			{
				std::vector<number_type> V;
				
				while ( pp != p )
				{
					if ( *pp != ',' )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseNumberArray: malformed optional field (unexpected non , character) " << std::string(pc,p);
						lme.finish();
						throw lme;																							
					}
					
					// skip ,
					++pp;
					
					char const * ppp = pp;
					while ( ppp != p && *ppp != ',' )
						++ppp;
						
					switch ( dt )
					{
						case 'c':
						{
							V.push_back(DNP.parseSignedNumber<int8_t>(pp,ppp));
							break;
						}
						case 'C':
						{
							V.push_back(DNP.parseUnsignedNumber<uint8_t>(pp,ppp));
							break;
						}
						case 's':
						{
							V.push_back(DNP.parseSignedNumber<int16_t>(pp,ppp));
							break;
						}
						case 'S':
						{
							V.push_back(DNP.parseUnsignedNumber<uint16_t>(pp,ppp));
							break;
						}
						case 'i':
						{
							V.push_back(DNP.parseSignedNumber<int32_t>(pp,ppp));
							break;
						}
						case 'I':
						{
							V.push_back(DNP.parseUnsignedNumber<uint32_t>(pp,ppp));
							break;
						}
						case 'f':
						{
							std::istringstream istr(std::string(pp,ppp));
							float f;
							istr >> f;
							if ( ! istr )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "libmaus::bambam::SamInfo::parseNumberArray: malformed optional field (unparsable floating point number) " << std::string(pc,p);
								lme.finish();
								throw lme;																										
							}
							if ( istr.get() != std::istream::traits_type::eof() )
							{											
								libmaus::exception::LibMausException lme;
								lme.getStream() << "libmaus::bambam::SamInfo::parseNumberArray: malformed optional field (additional data after float number) " << std::string(pc,p);
								lme.finish();
								throw lme;																										
							}
							
							V.push_back(f);
						}
					}
						
					pp = ppp;
				}

				char const tag[3] = { pc[0], pc[1], 0 };
				libmaus::bambam::BamAlignmentEncoderBase::putAuxNumberArray(buffer,&tag[0],dt,V.begin(),V.size());
			}

			void parseSamLine(char const * pa, char const * pe)
			{
				assert ( pa != pe );
				assert ( *pe == '\n' );
				
				uint64_t col = 0;				
				char const * p = pa;
				while ( p != pe && col < sam_info_mandatory_columns )
				{
					char const * pc = p;
					while ( p != pe && *p != '\t' )
						++p;
				
					fields[col][0] = pc;
					fields[col][1] = p;
					
					/* skip over tab */
					if ( p != pe )
						++p;
					col += 1;
				}
				
				if ( col != sam_info_mandatory_columns )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: defect SAM line " << std::string(pa,pe);
					lme.finish();
					throw lme;					
				}
				
				parseSamLine(&fields[0]);
				
				int64_t const refid = (rnamedefined == sam_info_base_field_defined) ? getRefIdForName(rname,rname+rnamelen) : -1;
				int64_t const nextrefid = (rnextdefined == sam_info_base_field_defined) ? getRefIdForName(rnext,rnext+rnextlen) : -1;

				buffer.reset();
				
				libmaus::bambam::BamAlignmentEncoderBase::encodeAlignment(
					buffer,
					seqenc,
					qname,
					qnamelen,
					refid,
					pos-1,
					mapq,
					flag,
					cigopvec.begin(),
					cigopvecfill,
					nextrefid,
					pnext-1,
					tlen,
					seq,
					seqlen,
					qual,
					(seqlen && static_cast<uint8_t>(qual[0])==255) ? 0 : 33
				);
				
				while ( p != pe )
				{
					char const * pc = p;
					while ( p != pe && *p != '\t' )
						++p;
				
					#if 0
					std::cerr << "aux field: " << std::string(pc,p) << std::endl;
					#endif
					
					if ( 
						p-pc >= 5 &&
						 AT[static_cast<uint8_t>(pc[0])] &&
						ADT[static_cast<uint8_t>(pc[1])] &&
						pc[2] == ':' &&
						pc[4] == ':'
					)
					{
						switch ( pc[3] )
						{
							// single character
							case 'A':
							{
								char const * pp = pc+5;
								
								if ( p - pp != 1 )
								{
									libmaus::exception::LibMausException lme;
									lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field (length of character field is not 1) " << std::string(pc,p);
									lme.finish();
									throw lme;
								}

								char const tag[3] = { pc[0], pc[1], 0 };								
								libmaus::bambam::BamAlignmentEncoderBase::putAuxNumber(buffer,&tag[0],'A',*pp);
								
								break;
							}
							// integer (signed 32 bit)
							case 'i':
							{
								int32_t const n = DNP.parseSignedNumber<int32_t>(pc + 5,p);								
								char const tag[3] = { pc[0], pc[1], 0 };
								libmaus::bambam::BamAlignmentEncoderBase::putAuxNumber(buffer,&tag[0],'i',n);
							
								break;
							}
							/* float number */
							case 'f':
							{
								std::istringstream istr(std::string(pc+5,p));
								float f;
								istr >> f;

								if ( ! istr )
								{
									libmaus::exception::LibMausException lme;
									lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field (unparsable float number) " << std::string(pc,p);
									lme.finish();
									throw lme;																							
								}
								
								if ( istr.get() != std::istream::traits_type::eof() )
								{								
									libmaus::exception::LibMausException lme;
									lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field (additional data after float number) " << std::string(pc,p);
									lme.finish();
									throw lme;																							
								}

								char const tag[3] = { pc[0], pc[1], 0 };
								libmaus::bambam::BamAlignmentEncoderBase::putAuxNumber(buffer,&tag[0],'f',f);

								break;
							}
							/* string */
							case 'Z':
							{
								for ( char const * pp = pc+5; pp != p; ++pp )
									if ( ! SZPT[static_cast<uint8_t>(*pp)] )
									{
										libmaus::exception::LibMausException lme;
										lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field (string field with invalid character) " << std::string(pc,p);
										lme.finish();
										throw lme;															
									}

								char const tag[3] = { pc[0], pc[1], 0 };
								libmaus::bambam::BamAlignmentEncoderBase::putAuxString(buffer,&tag[0],pc+5,p);
								
								break;
							}
							case 'H':
							{
								for ( char const * pp = pc+5; pp != p; ++pp )
									if ( ! (
										DT[static_cast<uint8_t>(*pp)]
										||
										(*pp >= 'A' && *pp <= 'F')
										) 
									)
									{
										libmaus::exception::LibMausException lme;
										lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field (hex field with invalid character) " << std::string(pc,p);
										lme.finish();
										throw lme;															
									}

								char const tag[3] = { pc[0], pc[1], 0 };
								libmaus::bambam::BamAlignmentEncoderBase::putAuxHexString(buffer,&tag[0],pc+5,p);
							
								break;
							}
							case 'B':
							{
								char const * pp = pc + 5;

								if ( pp == p )
								{
										libmaus::exception::LibMausException lme;
										lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field (array data type missing) " << std::string(pc,p);
										lme.finish();
										throw lme;																							
								}
								
								char const dt = *(pp++);

								switch ( dt )
								{
									case 'c':
										parseNumberArray<int8_t,'c'>(pc,p,pp);
										break;
									case 'C':
										parseNumberArray<uint8_t,'C'>(pc,p,pp);
										break;
									case 's':
										parseNumberArray<int16_t,'s'>(pc,p,pp);
										break;
									case 'S':
										parseNumberArray<uint16_t,'S'>(pc,p,pp);
										break;
									case 'i':
										parseNumberArray<int32_t,'i'>(pc,p,pp);
										break;
									case 'I':
										parseNumberArray<uint32_t,'I'>(pc,p,pp);
										break;
									case 'f':
										parseNumberArray<float,'f'>(pc,p,pp);
										break;
								}
								break;
							}
							default:
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field (unknown type) " << std::string(pc,p);
								lme.finish();
								throw lme;						
							}
						}
					}
					else
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: malformed optional field " << std::string(pc,p) << "\n";
						lme.finish();
						throw lme;						
					}
					
					/* skip over tab */
					if ( p != pe )
						++p;
					col += 1;
				}

				algn.blocksize = buffer.swapBuffer(algn.D);				
			}

			void parseSamLine(c_ptr_type_pair * fields)
			{
				parseStringField(fields[0], qnamedefined);
				qname = fields[0][0];
				qnamelen = fields[0][1] - fields[0][0];

				flag = parseNumberField(fields[1],"flag");
				
				parseStringField(fields[2], rnamedefined);
				rname = fields[2][0];
				rnamelen = fields[2][1]-fields[2][0];
				
				pos = parseNumberField(fields[3],"pos");
				mapq = parseNumberField(fields[4],"mapq");
				
				parseStringField(fields[5],cigardefined);
				cigar = fields[5][0];
				cigarlen = fields[5][1]-fields[5][0];

				parseStringField(fields[6],rnextdefined);
				rnext = fields[6][0];
				rnextlen = fields[6][1]-fields[6][0];
				
				pnext = parseNumberField(fields[7],"pnext");

				tlen = parseNumberField(fields[8],"tlen");
				
				parseStringField(fields[9],seqdefined);
				seq = fields[9][0];
				seqlen = (seqdefined == sam_info_base_field_defined) ? (fields[9][1] - fields[9][0]) : 0;

				parseStringField(fields[10],qualdefined);
				qual = fields[10][0];
				quallen = (qualdefined == sam_info_base_field_defined) ? fields[10][1]-fields[10][0] : 0;

				// copy rname to rnext if rnext is =
				if ( rnamedefined && rnextdefined && rnextlen == 1 && rnext[0] == '=' )
				{
					rnext = rname;
					rnextlen = rnamelen;
				}

				// fill undefined quality string
				if ( 
					seqdefined == sam_info_base_field_defined
					&&
					qualdefined == sam_info_base_field_undefined
				)
				{
					if ( tqual.size() < seqlen )
						tqual = libmaus::autoarray::AutoArray<char>(seqlen,false);
					memset(tqual.begin(),static_cast<char>(255),seqlen);
					qual = tqual.begin();
					quallen = seqlen;
				}

				// parse cigar string
				if ( cigardefined )
				{
					parseCigar(cigar,cigar+cigarlen);
				}
				else
				{
					cigopvecfill = 0;
				}
				
				/* input validation starts here */
				
				#if 0
				/* the name is validated when we construct the BAM block down stream, so */
				/* there is no need to check it here */
				if ( qnamedefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = qname;
					char const * pe = qname + qnamelen;
					
					while ( p != pe )
					{
						ok = ok && qnameValid[static_cast<uint8_t>(*p)];
						++p;
					}
					
					if ( (!ok) )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid query name " << std::string(qname,qname+qnamelen) << "\n";
						lme.finish();
						throw lme;
					}
				}
				#endif
				
				if ( flag < 0 || flag >= static_cast<int32_t>(1u<<16) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid flag " << flag << "\n";
					lme.finish();
					throw lme;
				}
				if ( rnamedefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = rname;
					char const * const pe = rname + rnamelen;

					if ( p == pe )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid empty rname field\n";
						lme.finish();
						throw lme;
					}
						
					ok = ok && rnameFirstValid[static_cast<uint8_t>(*p)];
					++p;
					
					while ( p != pe )
					{
						ok = ok && rnameOtherValid[static_cast<uint8_t>(*p)];
						++p;
					}
					
					if ( ! ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid rname field " << std::string(rname,rname+rnamelen) << "\n";
						lme.finish();
						throw lme;
					}
				}
				if ( pos < 0 || pos >= static_cast<int32_t>(1u<<29) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid pos field " << pos << "\n";
					lme.finish();
					throw lme;
				}
				if ( mapq < 0 || mapq >= static_cast<int32_t>(1u<<8) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid mapq field " << mapq << "\n";
					lme.finish();
					throw lme;
				}
				if ( rnextdefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = rnext;
					char const * const pe = p + rnextlen;

					if ( p == pe )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid empty rnext field\n";
						lme.finish();
						throw lme;						
					}
						
					ok = ok && rnameFirstValid[static_cast<uint8_t>(*p)];
					++p;
				
					while ( p != pe )
					{
						ok = ok && rnameOtherValid[static_cast<uint8_t>(*p)];
						++p;
					}
				
					if ( ! ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid rnext field " << std::string(rnext,rnext+rnextlen) << " length " << rnextlen << "\n";
						lme.finish();
						throw lme;						
					}
				}
				if ( pnext < 0 || pnext >= static_cast<int32_t>(1u<<29) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid pnext field " << pnext << "\n";
					lme.finish();
					throw lme;						
				}
				if ( tlen < ((-(static_cast<int32_t>(1u<<29)))+1) || tlen >= static_cast<int32_t>(1u<<29) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid tlen field " << tlen << "\n";
					lme.finish();
					throw lme;						
				}
				if ( seqdefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = seq;
					char const * pe = seq + seqlen;
					
					while ( p != pe )
					{
						ok = ok && seqValid[static_cast<uint8_t>(*p)];
						++p;
					}
					
					if ( !ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid sequence string " << std::string(seq,seq+seqlen) << "\n";
						lme.finish();
						throw lme;						
					}
					if ( p == seq )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid empty sequence string\n";
						lme.finish();
						throw lme;						
					}
				}
				if ( qualdefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = qual;
					char const * pe = qual + quallen;
					
					while ( p != pe )
					{
						ok = ok && qualValid[static_cast<uint8_t>(*p)];
						++p;
					}
					
					if ( !ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid base quality string " << std::string(qual,qual+quallen) << "\n";
						lme.finish();
						throw lme;						
					}
					if ( p == qual )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid empty base quality string\n";
						lme.finish();
						throw lme;						
					}
				}
				if ( 
					qualdefined == sam_info_base_field_defined 
					&&
					seqdefined == sam_info_base_field_defined 
				)
				{
					if ( seqlen != quallen )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: sequence length " << seqlen << " does not match length of quality string " << quallen << "\n";
						lme.finish();
						throw lme;						
					}	
				}

				bool qcfail = flag & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FQCFAIL;
				bool secondary = flag & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FSECONDARY;
				// annotation (see SAM spec section 3.2, padded)
				bool const annot = qcfail && secondary;

				if ( 
					(cigardefined == sam_info_base_field_defined) && 
					(seqdefined == sam_info_base_field_defined) && 
					(!annot) &&
					(! (flag & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FUNMAP ))
				)
				{
					uint64_t exseqlen = 0;
					
					for ( uint64_t i = 0; i < cigopvecfill; ++i )
						switch ( cigopvec[i].first )
						{
							case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CMATCH:
							case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CINS:
							case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CSOFT_CLIP:
							case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CEQUAL:
							case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDIFF:
								exseqlen += cigopvec[i].second;
								break;
							default:
								break;
						}

					if ( exseqlen != seqlen )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid cigar string " << std::string(cigar,cigar+cigarlen) << " for sequence " << std::string(seq,seq+seqlen) << " (sum " << exseqlen << " over match,ins,softclip,equal,diff does not match length of query sequence " << seqlen << ")\n";
						lme.finish();
						throw lme;
					}
				}
			}

		};
	}
}
#endif
