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

namespace libmaus
{
	namespace bambam
	{
		struct SamInfo : public libmaus::bambam::SamInfoBase
		{
			enum
			{
				sam_info_mandatory_columns = 11
			};
			
			libmaus::autoarray::AutoArray<char> qname;
			sam_info_base_field_status qnamedefined;
			size_t qnamelen;
			
			int32_t flag;

			libmaus::autoarray::AutoArray<char> rname;
			sam_info_base_field_status rnamedefined;
			size_t rnamelen;
			
			int32_t pos;
			int32_t mapq;

			libmaus::autoarray::AutoArray<char> cigar;
			sam_info_base_field_status cigardefined;	
			size_t cigarlen;

			libmaus::autoarray::AutoArray<char> rnext;
			sam_info_base_field_status rnextdefined;
			size_t rnextlen;

			int32_t pnext;
			int32_t tlen;

			libmaus::autoarray::AutoArray<char> seq;
			sam_info_base_field_status seqdefined;
			size_t seqlen;

			libmaus::autoarray::AutoArray<char> qual;
			sam_info_base_field_status qualdefined;
			size_t quallen;
			
			c_ptr_type_pair fields[sam_info_mandatory_columns];
			
			std::vector<libmaus::bambam::cigar_operation> cigopvec;

			BamSeqEncodeTable const seqenc;

			SamInfo()
			{
			
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

				cigopvec.resize(0);
				p = cigar.begin();
					
				while ( *p )
				{
					uint64_t num = 0;
					
					if ( ! isdigit(*p) )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid cigar string " << cigar.begin() << "\n";
						lme.finish();
						throw lme;
					}
					while ( isdigit(*p) )
					{
						num *= 10;
						num += (*p)-'0';
						++p;
					}
					
					libmaus::bambam::BamFlagBase::bam_cigar_ops op;
					
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
							lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid cigar operatoor " << p[-1] << "\n";
							lme.finish();
							throw lme;
						}
					}
					
					cigopvec.push_back(libmaus::bambam::cigar_operation(op,num));
				}

				::libmaus::fastx::UCharBuffer buffer;
			
				#if 0
					libmaus::bambam::BamAlignmentEncoderBase::encodeAlignment(
						buffer,seqenc,
						name.begin(),
						namelen,
						
					);
				template<
					typename name_iterator,
					typename cigar_iterator,
					typename seq_iterator,
					typename qual_iterator
				>
				static void encodeAlignment(
					::libmaus::fastx::UCharBuffer & buffer,
					BamSeqEncodeTable const & seqenc,
					name_iterator name,
					uint32_t const namelen,
					int32_t const refid,
					int32_t const pos,
					uint32_t const mapq,
					uint32_t const flags,
					cigar_iterator cigar,
					uint32_t const cigarlen,
					int32_t const nextrefid,
					int32_t const nextpos,
					uint32_t const tlen,
					seq_iterator seq,
					uint32_t const seqlen,
					qual_iterator qual,
					uint8_t const qualoffset = 33
				)
				#endif

				#if 0
				r = BamBam_CharBuffer_PutAlignmentC(
					object->aput,
					object->saminfo->flag,
					BamBam_BamFileHeader_FindChromosomeIdByName(object->header,object->saminfo->rname),
					object->saminfo->pos-1,
					BamBam_BamFileHeader_FindChromosomeIdByName(object->header,object->saminfo->rnext),
					object->saminfo->pnext-1,
					object->saminfo->qname,
					object->saminfo->seq,
					object->saminfo->qual,
					object->saminfo->cigar,
					object->saminfo->mapq,
					object->saminfo->tlen
				);
				
				if ( r < 0 )
					return 0;

				#if 0
				fprintf(stderr, "ok: ");
				fwrite(pa,pe-pa,1,stderr);
				fprintf(stderr,"\n");			
				#endif			
				
				return object->aput->calignment;
				#endif

				while ( p != pe )
				{
					char const * pc = p;
					while ( p != pe && *p != '\t' )
						++p;
				
					std::cerr << "aux field: " << std::string(pc,p) << std::endl;
					
					/* skip over tab */
					if ( p != pe )
						++p;
					col += 1;
				}
			}

			void parseSamLine(c_ptr_type_pair * fields)
			{
				parseStringField(fields[0], qname, qnamedefined);
				qnamelen = fields[0][1] - fields[0][0];

				flag = parseNumberField(fields[1],"flag");
				
				parseStringField(fields[2],rname,rnamedefined);
				rnamelen = fields[2][1]-fields[2][0];
				pos = parseNumberField(fields[3],"pos");
				mapq = parseNumberField(fields[4],"mapq");
				
				parseStringField(fields[5],cigar,cigardefined);
				cigarlen = fields[5][1]-fields[5][0];

				parseStringField(fields[6],rnext,rnextdefined);
				rnextlen = fields[6][1]-fields[6][0];
				
				pnext = parseNumberField(fields[7],"pnext");

				tlen = parseNumberField(fields[8],"tlen");
				
				parseStringField(fields[9],seq,seqdefined);
				seqlen = fields[9][1] - fields[9][0];

				parseStringField(fields[10],qual,qualdefined);
				quallen = fields[10][1]-fields[10][0];

				if ( qnamedefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = qname.begin();
					
					while ( *p )
					{
						ok = ok && qnameValid[(int)*p];
						++p;
					}
					
					if ( (!ok) || (p==qname.begin()) )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid query name " << qname.begin() << "\n";
						lme.finish();
						throw lme;
					}
				}
				
				if ( flag < 0 || flag >= (int32_t)(1u<<16) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid flag " << flag << "\n";
					lme.finish();
					throw lme;
				}
				if ( rnamedefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = rname.begin();

					if ( !*p )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid empty rname field\n";
						lme.finish();
						throw lme;
					}
						
					ok = ok && rnameFirstValid[(int)*p];
					++p;
					
					while ( *p )
					{
						ok = ok && rnameOtherValid[(int)*p];
						++p;
					}
					
					if ( ! ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid rname field " << rname.begin() << "\n";
						lme.finish();
						throw lme;
					}
				}

				if ( pos < 0 || pos >= (int32_t)(1u<<29) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid pos field " << pos << "\n";
					lme.finish();
					throw lme;
				}
				if ( mapq < 0 || mapq >= (int32_t)(1u<<8) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid mapq field " << mapq << "\n";
					lme.finish();
					throw lme;
				}
				if ( cigardefined == sam_info_base_field_defined )
				{
					char const * p = cigar.begin();
					size_t exseqlen = 0;
					
					while ( *p )
					{
						uint64_t num = 0;
						
						if ( ! isdigit(*p) )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid cigar string " << cigar.begin() << "\n";
							lme.finish();
							throw lme;
						}
						while ( isdigit(*p) )
						{
							num *= 10;
							num += (*p)-'0';
							++p;
						}
						
						switch ( *(p++) )
						{
							case 'M':
							case 'I':
							case 'S':
							case '=':
							case 'X':
								exseqlen += num;
								break;
							case 'D':
							case 'N':
							case 'H':
							case 'P':
								break;
							default:
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid cigar operatoor " << p[-1] << "\n";
								lme.finish();
								throw lme;
							}
						}
					}
					
					if ( ! (flag & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FUNMAP ) )
					{
						if ( exseqlen != seqlen )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid cigar string " << cigar.begin() << " for sequence " << seq.begin() << "\n";
							lme.finish();
							throw lme;
						}
					}
				}
				if ( rnextdefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = rnext.begin();

					if ( !*p )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid empty rnext field\n";
						lme.finish();
						throw lme;						
					}
						
					if ( *p == '=' && !p[1] )
					{
					
					}
					else
					{
						ok = ok && rnameFirstValid[(int)*p];
						++p;
					
						while ( *p )
						{
							ok = ok && rnameOtherValid[(int)*p];
							++p;
						}
					
						if ( ! ok )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid rnext field " << rnext.begin() << "\n";
							lme.finish();
							throw lme;						
						}
					}
				}
				if ( pnext < 0 || pnext >= (int32_t)(1u<<29) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid pnext field " << pnext << "\n";
					lme.finish();
					throw lme;						
				}
				if ( tlen < ((-((int32_t)(1u<<29)))+1) || tlen >= (int32_t)(1u<<29) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid tlen field " << tlen << "\n";
					lme.finish();
					throw lme;						
				}
				if ( seqdefined == sam_info_base_field_defined )
				{
					int ok = 1;
					char const * p = seq.begin();
					
					while ( *p )
					{
						ok = ok && seqValid[(int)*p];
						++p;
					}
					
					if ( !ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid sequence string " << seq.begin() << "\n";
						lme.finish();
						throw lme;						
					}
					if ( p == seq.begin() )
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
					char const * p = qual.begin();
					
					while ( *p )
					{
						ok = ok && qualValid[(int)*p];
						++p;
					}
					
					if ( !ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfo::parseSamLine: invalid base quality string " << qual.begin() << "\n";
						lme.finish();
						throw lme;						
					}
					if ( p == qual.begin() )
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
				if ( 
					seqdefined == sam_info_base_field_defined
					&&
					qualdefined == sam_info_base_field_undefined
				)
				{
					if ( qual.size() < seqlen )
						qual = libmaus::autoarray::AutoArray<char>(seqlen+1,false);
					qual[seqlen] = 0;
					memset(qual.begin(),255,seqlen);
				}
				if ( rnamedefined && rnextdefined && (rnext.size() >= 2) && rnext[0] == '=' && rnext[1] == 0 )
				{
					if ( rnext.size() < rnamelen )
					{
						rnext = libmaus::autoarray::AutoArray<char>(rnamelen+1,false);
					}

					strcpy(rnext.begin(),rname.begin());
				}
				
				#if 0
				fprintf(stdout,"Got name %s\n", qname.begin());
				fprintf(stdout,"Got flag %d\n", flag);
				fprintf(stdout,"Got rname %s\n", rname.begin());
				fprintf(stdout,"Got pos %d\n", pos);
				fprintf(stdout,"Got mapq %d\n", mapq);
				fprintf(stdout,"Got cigar %s\n", cigar.begin());
				fprintf(stdout,"Got rnext %s\n", rnext.begin());
				fprintf(stdout,"Got pnext %d\n", pnext);
				fprintf(stdout,"Got tlen %d\n", tlen);
				fprintf(stdout,"Got seq %s\n", seq.begin());
				fprintf(stdout,"Got qual %s\n", qual.begin());
				#endif
			}

		};
	}
}
#endif
