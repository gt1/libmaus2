/*
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
*/
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTDECODERBASE_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTDECODERBASE_HPP

#include <libmaus/bambam/Chromosome.hpp>
#include <libmaus/bambam/BamFlagBase.hpp>
#include <libmaus/bambam/DecoderBase.hpp>
#include <libmaus/bambam/BamFormatAuxiliary.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/hashing/hash.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentDecoderBase : public ::libmaus::bambam::DecoderBase, public ::libmaus::bambam::BamFlagBase
		{
			static bool isPaired(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FPAIRED; }
			static bool isProper(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FPROPER_PAIR; }
			static bool isUnmap(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FUNMAP; }
			static bool isMateUnmap(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FMUNMAP; }
			static bool isReverse(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FREVERSE; }
			static bool isMateReverse(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FMREVERSE; }
			static bool isRead1(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FREAD1; }
			static bool isRead2(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FREAD2; }
			static bool isSecondary(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FSECONDARY; }
			static bool isQCFail(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FQCFAIL; }
			static bool isDup(uint32_t const flags) { return flags & LIBMAUS_BAMBAM_FDUP; }
			
			static char const * flagToString(uint32_t const flag)
			{
				switch ( flag )
				{
					case LIBMAUS_BAMBAM_FPAIRED: return "LIBMAUS_BAMBAM_FPAIRED";
					case LIBMAUS_BAMBAM_FPROPER_PAIR: return "LIBMAUS_BAMBAM_FPROPER_PAIR";
					case LIBMAUS_BAMBAM_FUNMAP: return "LIBMAUS_BAMBAM_FUNMAP";
					case LIBMAUS_BAMBAM_FMUNMAP: return "LIBMAUS_BAMBAM_FMUNMAP";
					case LIBMAUS_BAMBAM_FREVERSE: return "LIBMAUS_BAMBAM_FREVERSE";
					case LIBMAUS_BAMBAM_FMREVERSE: return "LIBMAUS_BAMBAM_FMREVERSE";
					case LIBMAUS_BAMBAM_FREAD1: return "LIBMAUS_BAMBAM_FREAD1";
					case LIBMAUS_BAMBAM_FREAD2: return "LIBMAUS_BAMBAM_FREAD2";
					case LIBMAUS_BAMBAM_FSECONDARY: return "LIBMAUS_BAMBAM_FSECONDARY";
					case LIBMAUS_BAMBAM_FQCFAIL: return "LIBMAUS_BAMBAM_FQCFAIL";
					case LIBMAUS_BAMBAM_FDUP: return "LIBMAUS_BAMBAM_FDUP";
					default: return "LIBMAUS_BAMBAM_F?";
				}
			}
			
			static std::string flagsToString(uint32_t const flags)
			{
				unsigned int numflags = 0;
				for ( unsigned int i = 0; i < 11; ++i )
					if ( (flags & (1u << i)) )
						numflags++;
						
				std::ostringstream ostr;
				unsigned int procflags = 0;		
				
				for ( unsigned int i = 0; i < 11; ++i )
					if ( (flags & (1u << i)) )
					{
						ostr << flagToString((flags & (1u << i)));
						if ( procflags++ != numflags )
							ostr << ";";
					}
				
				return ostr.str();
			}
			
			
			static uint32_t hash32(uint8_t const * D)
			{
				return ::libmaus::hashing::EvaHash::hash(
					reinterpret_cast<uint8_t const *>(getReadName(D)),
					getLReadName(D)-1
				);
			}
			
			static uint64_t putFastQ(
				uint8_t const * D,
				libmaus::autoarray::AutoArray<uint8_t> & T
			)
			{
				uint64_t const len = getFastQLength(D);
				if ( T.size() < len ) 
					T = libmaus::autoarray::AutoArray<uint8_t>(len);
				putFastQ(D,T.begin());
				return len;
			}

			static uint64_t getFastQLength(uint8_t const * D)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				
				return
					1 + namelen + ((flags & LIBMAUS_BAMBAM_FPAIRED) ? 2 : 0) + 1 + // name line
					lseq + 1 + // seq line
					1 + 1 + // plus line
					lseq + 1 // quality line
					;
			}
			
			template<typename iterator>
			static iterator putFastQ(uint8_t const * D, iterator it)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				char const * rn = getReadName(D);
				char const * rne = rn + namelen;

				*(it++) = '@';
				while ( rn != rne )
					*(it++) = *(rn++);
				
				if ( (flags & LIBMAUS_BAMBAM_FPAIRED) )
				{
					if ( (flags & LIBMAUS_BAMBAM_FREAD1) )
					{
						*(it++) = '/';
						*(it++) = '1';
					}
					else
					{
						*(it++) = '/';
						*(it++) = '2';
					}
				}
				
				*(it++) = '\n';
				
				if ( flags & LIBMAUS_BAMBAM_FREVERSE )
					it = decodeReadRCIt(D,it,lseq);
				else
					it = decodeRead(D,it,lseq);
					
				*(it++) = '\n';				

				*(it++) = '+';
				*(it++) = '\n';

				if ( flags & LIBMAUS_BAMBAM_FREVERSE )
				{
					uint8_t const * const quale = getQual(D);
					uint8_t const * qualc = quale + lseq;
					
					while ( qualc != quale )
						*(it++) = *(--qualc) + 33;
				}
				else
				{
					uint8_t const * qual = getQual(D);
					uint8_t const * const quale = qual + lseq;
					
					while ( qual != quale )
						*(it++) = (*(qual++)) + 33;				
				}
				*(it++) = '\n';
				
				return it;
			}				
			
			static uint32_t getLEIntegerWrapped(
				uint8_t const * D,
				uint64_t const dn,
				uint64_t o,
				unsigned int const l)
			{
				uint32_t v = 0;
				for ( unsigned int i = 0; i < l; ++i, o=((o+1)&(dn-1)) )
					v |= (static_cast<uint32_t>(D[o]) << (8*i));
				return v;
			}
			static uint32_t getBinMQNLWrapped  (
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return static_cast<uint32_t>(getLEIntegerWrapped(D,dn,(o+8)&(dn-1),4)); }
			static uint32_t getLReadNameWrapped(
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return (getBinMQNLWrapped(D,dn,o) >> 0) & 0xFF; }
			static uint32_t getReadNameOffset(uint64_t const dn, uint64_t const o)
			{
				return (o+32)&(dn-1);
			}
			static uint32_t getFlagNCWrapped  (
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return static_cast<uint32_t>(getLEIntegerWrapped(D,dn,(o+12)&(dn-1),4)); }
			static uint32_t getFlagsWrapped(
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return (getFlagNCWrapped(D,dn,o) >> 16) & 0xFFFF; }
			
			static  int32_t     getRefID    (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+0,4)); }
			static  int32_t     getPos      (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+4,4)); }
			static uint32_t     getBinMQNL  (uint8_t const * D) { return static_cast<uint32_t>(getLEInteger(D+8,4)); }
			static uint32_t     getFlagNC   (uint8_t const * D) { return static_cast<uint32_t>(getLEInteger(D+12,4)); }
			static  int32_t     getLseq     (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+16,4)); }
			static  int32_t     getNextRefID(uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+20,4)); }
			static  int32_t     getNextPos  (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+24,4)); }
			static  int32_t     getTlen     (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+28,4)); }
			static char const * getReadName (uint8_t const * D) { return reinterpret_cast< char    const*>(D+32); }
			static char * getReadName (uint8_t * D) { return reinterpret_cast< char *>(D+32); }
			static uint32_t     getBin      (uint8_t const * D) { return (getBinMQNL(D) >> 16) & 0xFFFFu; }
			static uint32_t     getMapQ     (uint8_t const * D) { return (getBinMQNL(D) >>  8) & 0xFFu; }
			static uint32_t     getLReadName(uint8_t const * D) { return ((getBinMQNL(D) >>  0) & 0xFFu); }
			static std::string  getReadNameS(uint8_t const * D) { char const * c = getReadName(D); return std::string(c,c+getLReadName(D)-1); }
			static uint8_t const * getCigar(uint8_t const * D) { return reinterpret_cast<uint8_t const *>(getReadName(D) + getLReadName(D)); }
			static uint8_t * getCigar(uint8_t * D) { return reinterpret_cast<uint8_t *>(getReadName(D) + getLReadName(D)); }
			static uint32_t getCigarField(uint8_t const * D, uint64_t const i) { return getLEInteger(getCigar(D)+4*i,4); }
			static uint32_t getCigarFieldLength(uint8_t const * D, uint64_t const i) {  return (getCigarField(D,i) >> 4) & ((1ull<<(32-4))-1); }
			static uint32_t getCigarFieldOp(uint8_t const * D, uint64_t const i) {  return (getCigarField(D,i) >> 0) & ((1ull<<(4))-1); }

			static uint64_t getNumPreCigarBytes(uint8_t const * D)
			{
				return getCigar(D) - D;
			}
			static uint64_t getNumCigarBytes(uint8_t const * D)
			{
				return getNCigar(D) * sizeof(uint32_t);
			}
			static uint64_t getNumPreSeqBytes(uint8_t const * D)
			{
				return getSeq(D)-D;
			}
			static uint64_t getNumSeqBytes(uint8_t const * D)
			{
				return ( getLseq(D) + 1 ) / 2;
			}
			
			static char cigarOpToChar(uint32_t const i)
			{
				//                       012345678
				static char const C[] = "MIDNSHP=X";
				static uint32_t const bound = sizeof(C)/sizeof(C[0])-1;
						
				if ( i < bound )
					return C[i];
				else
					return '?';
			}
			
			static uint64_t getLseqByCigar(uint8_t const * D)
			{
				uint64_t seqlen = 0;
				uint64_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);

				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);
					
					// MIS=X
					switch ( op )
					{
						case 0: // M
						case 1: // I
						case 4: // S
						case 7: // =
						case 8: // X
							seqlen += (v >> 4) & ((1ull<<(32-4))-1);
							break;
					}
				}
				
				return seqlen;
			}
			
			static uint64_t getReferenceLength(uint8_t const * D)
			{
				uint64_t reflen = 0;
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);
				
				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);
					
					switch ( op )
					{
						case 0: // M
						case 2: // D
						case 3: // N
						case 7: // =
						case 8: // X
							reflen += (v >> 4) & ((1ull<<(32-4))-1);
							break;
					}
				}
				
				return reflen;
			}
			
			static uint64_t getFrontClipping(uint8_t const * D)
			{
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);
				uint64_t frontclip = 0;
				
				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);
					
					if ( op == 4 /* S */ || op == 5 /* H */ )
						frontclip += static_cast<int64_t>((v >> 4) & ((1ull<<(32-4))-1));
					else
						break;
				}
				
				return frontclip;
			}

			static uint64_t getBackClipping(uint8_t const * D)
			{
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D) + 4*ncigar - 4;
				uint64_t backclip = 0;
				
				for ( uint32_t i = 0; i < ncigar; ++i, cigar-=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);
					
					if ( op == 4 /* S */ || op == 5 /* H */ )
						backclip += static_cast<int64_t>((v >> 4) & ((1ull<<(32-4))-1));
					else
						break;
				}
				
				return backclip;
			}
			
			// first position of aligned base on reference (1 based coordinate)
			static uint64_t getAlignmentStart(uint8_t const * D)
			{
				return getPos(D) + 1;
			}

			// last position of aligned base on reference
			static uint64_t getAlignmentEnd(uint8_t const * D)
			{
				return getAlignmentStart(D) + getReferenceLength(D) - 1;
			}
			
			static int64_t getUnclippedStart(uint8_t const * D)
			{
				return
					static_cast<int64_t>(getAlignmentStart(D)) - 
					static_cast<int64_t>(getFrontClipping(D));
			}

			static int64_t getUnclippedEnd(uint8_t const * D)
			{
				return
					static_cast<int64_t>(getAlignmentEnd(D)) +
					static_cast<int64_t>(getBackClipping(D));
			}
			
			static int64_t getCoordinate(uint8_t const * D)
			{
				if ( isReverse(getFlags(D)) )
				{
					return getUnclippedEnd(D);
				}
				else
				{
					return getUnclippedStart(D);
				}
			}

			static char getCigarFieldOpAsChar(uint8_t const * D, uint64_t const i) { return cigarOpToChar(getCigarFieldOp(D,i)); }
			static uint32_t     getFlags      (uint8_t const * D) { return (getFlagNC(D) >> 16) & 0xFFFFu; }
			static std::string  getFlagsS     (uint8_t const * D) { return flagsToString(getFlags(D)); }
			static uint32_t     getNCigar     (uint8_t const * D) { return (getFlagNC(D) >>  0) & 0xFFFFu; }
			static std::string  getCigarString(uint8_t const * D)
			{
				std::ostringstream ostr;
				for ( uint64_t i = 0; i < getNCigar(D); ++i )
					ostr << getCigarFieldLength(D,i) << getCigarFieldOpAsChar(D,i);
				return ostr.str();
			}
			static uint8_t const * getSeq(uint8_t const * D)
			{
				return getCigar(D) + 4 * getNCigar(D);
			}
			static uint8_t * getSeq(uint8_t * D)
			{
				return getCigar(D) + 4 * getNCigar(D);
			}
			static uint8_t const * getQual(uint8_t const * D)
			{
				return getSeq(D) + ((getLseq(D)+1)/2);
			}
			static uint8_t * getQual(uint8_t * D)
			{
				return getSeq(D) + ((getLseq(D)+1)/2);
			}
			static uint8_t const * getAux(uint8_t const * D)
			{
				return getQual(D) + getLseq(D);
			}
			static uint8_t * getAux(uint8_t * D)
			{
				return getQual(D) + getLseq(D);
			}
			
			static uint64_t getScore(uint8_t const * D)
			{
				uint64_t score = 0;
				uint8_t const * rqual = getQual(D);
				uint8_t const * equal = rqual+getLseq(D);
				for ( uint8_t const * qual = rqual; qual != equal; ++qual )
					if ( *qual >= 15 )
						score += *qual;
				
				return score;
			}
			
			static uint64_t getPrimLengthByType(uint8_t const c)
			{
				switch ( c )
				{
					case 'A': case 'c': case 'C': return sizeof(int8_t);
					case 's': case 'S': return sizeof(int16_t);
					case 'i': case 'I': return sizeof(int32_t);
					case 'f':           return sizeof(float);
					default: return 0;
				}
			}

			static uint64_t getAuxLength(uint8_t const * D)
			{
				switch ( D[2] )
				{
					case 'A': case 'c': case 'C': 
						return 2+1+sizeof(int8_t);
					case 's': case 'S': 
						return 2+1+sizeof(int16_t);
					case 'i': case 'I': 
						return 2+1+sizeof(int32_t);
					case 'f': 
						return 2+1+sizeof(float);
					case 'Z':
					case 'H':
					{
						uint64_t len = 2+1;
						D += len;
						while ( *D )
							len++, D++;
						len++;
						return len;
					}
					case 'B':
					{
						uint8_t const eltype = D[3];
						uint32_t const numel = getLEInteger(D+4,4);
						return 2/*tag*/+1/*B*/+1/*type*/+4/* array length */+numel*getPrimLengthByType(eltype);
					}
					default:
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Unable to handle type " << D[2] << "(" << static_cast<int>(D[2]) << ")" << " in BamAlignmentDecoderBase::getAuxLength()" << std::endl;
						se.finish();
						throw se;
					}
				}
			}

			static std::string auxValueToString(uint8_t const * D)
			{
				std::ostringstream ostr;
				
				// std::cerr << "\n[D] tag " << D[0] << D[1] << " type " << D[2] << std::endl;
				
				switch ( D[2] )
				{
					case 'A': return std::string(1,D[3]);
					case 'c':
					{
						int8_t const v = reinterpret_cast<int8_t const *>(D+3)[0];
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'C':
					{
						uint8_t const v = reinterpret_cast<uint8_t const *>(D+3)[0];
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 's':
					{
						int16_t const v = static_cast<int16_t>(getLEInteger(D+3,2));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'S':
					{
						uint16_t const v = static_cast<uint16_t>(getLEInteger(D+3,2));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'i':
					{
						int32_t const v = static_cast<int32_t>(getLEInteger(D+3,4));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'I':
					{
						uint32_t const v = static_cast<uint32_t>(getLEInteger(D+3,4));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'f':
					{
						uint32_t const u = static_cast<uint32_t>(getLEInteger(D+3,4));
						union numberpun
						{
							float fvalue;
							uint32_t uvalue;
						};
						numberpun np;
						np.uvalue = u;
						float const v = np.fvalue;
						ostr << v;
						break;
					}
					case 'H':
					{
						uint8_t const * p = D+3;
						while ( *p )
							ostr << *(p++);
						break;
					}
					case 'Z':
					{
						uint8_t const * p = D+3;
						while ( *p )
							ostr << *(p++);
						break;
					}
					case 'B':
					{
						uint8_t const type = D[3];
						uint32_t const len = getLEInteger(D+4,4);
						ostr << type;
						
						uint8_t const * p = D+8;
						for ( uint64_t i = 0; i < len; ++i )
						{
							ostr << ",";
							switch ( type )
							{
								case 'A':
								{
									ostr << *p; 
									p += 1; 
									break;
								}
								case 'c':
								{
									int8_t const v = static_cast<int8_t>(getLEInteger(p,1));
									p += 1;
									ostr << static_cast<int64_t>(v);	
									break;
								}
								case 'C':
								{
									uint8_t const v = static_cast<uint8_t>(getLEInteger(p,1));
									p += 1;
									ostr << static_cast<int64_t>(v);	
									break;
								}
								case 's':
								{
									int16_t const v = static_cast<int16_t>(getLEInteger(p,2));
									p += 2;
									ostr << static_cast<int64_t>(v);	
									break;
								}
								case 'S':
								{
									uint16_t const v = static_cast<uint16_t>(getLEInteger(p,2));
									p += 2;
									ostr << static_cast<int64_t>(v);	
									break;
								}
								case 'i':
								{
									int32_t const v = static_cast<int32_t>(getLEInteger(p,4));
									p += 4;
									ostr << static_cast<int64_t>(v);	
									break;
								}
								case 'I':
								{
									uint32_t const v = static_cast<uint32_t>(getLEInteger(p,4));
									p += 4;
									ostr << static_cast<int64_t>(v);
									break;
								}
								case 'f':
								{
									union numberpun
									{
										float fvalue;
										uint32_t uvalue;
									};
									numberpun np;
									np.uvalue = static_cast<uint32_t>(getLEInteger(p,4));
									p += 4;
									ostr << np.fvalue;
									break;
								}
							}
						}				
						break;
					}
					default:
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Unable to handle type " << D[2] << "(" << static_cast<int>(D[2]) << ")" << " in BamAlignmentDecoderBase::auxValueToString()" << std::endl;
						se.finish();
						throw se;				
					}
				}
				
				return ostr.str();
			}
			
				
			static uint8_t decodeSymbolUnchecked(uint8_t const c)
			{
				static char const C[]     = "=ACMGRSVTWYHKDBN";
				return C[c];
			}

			static uint8_t decodeSymbol(uint8_t const c)
			{
				static char const C[]     = "=ACMGRSVTWYHKDBN";
				// static char const RC[] = "=TGMCRSVAWYHKDBN";
				static uint32_t const bound = sizeof(C)/sizeof(C[0])-1;
						
				if ( c < bound )
					return C[c];
				else
					return '?';
			}
			static uint8_t decodeSymbolRCUnchecked(uint8_t const c)
			{
				static char const C[] = "=TGMCRSVAWYHKDBN";
				return C[c];
			}
			static uint8_t decodeSymbolRC(uint8_t const c)
			{
				static char const C[] = "=TGMCRSVAWYHKDBN";
				static uint32_t const bound = sizeof(C)/sizeof(C[0])-1;
						
				if ( c < bound )
					return C[c];
				else
					return '?';
			}
			template<typename iterator>
			static iterator decodeRead(uint8_t const * D, iterator S, uint64_t const seqlen)
			{
				uint8_t const * cseq = getSeq(D);
				
				for ( uint64_t i = 0; i < (seqlen>>1); ++i )
				{
					uint8_t const c = *(cseq++);
					*(S++) = decodeSymbolUnchecked((c >> 4) & 0xF);
					*(S++) = decodeSymbolUnchecked((c >> 0) & 0xF);
				}
				if ( seqlen & 1 )
				{
					uint8_t const c = *(cseq++);
					*(S++) = decodeSymbolUnchecked((c >> 4) & 0xF);
				}
				
				return S;		
			}
			template<typename iterator>
			static uint64_t decodeReadRC(uint8_t const * D, iterator S, uint64_t const seqlen)
			{
				uint8_t const * cseq = getSeq(D);
				S += seqlen;
				
				for ( uint64_t i = 0; i < (seqlen>>1); ++i )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
					*(--S) = decodeSymbolRCUnchecked((c >> 0) & 0xF);
				}
				if ( seqlen & 1 )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
				}
				
				return seqlen;			
			}
			template<typename iterator>
			static iterator decodeReadRCIt(uint8_t const * D, iterator S, uint64_t const seqlen)
			{
				uint8_t const * const cseqe = getSeq(D);
				uint8_t const *       cseq  = cseqe + ((seqlen+1)/2);

				if ( seqlen & 1 )
					*(S++) = decodeSymbolRCUnchecked( ((*(--cseq)) >> 4) & 0xF );
				while ( cseq != cseqe )
				{
					uint8_t const c = *(--cseq);
					*(S++) = decodeSymbolRCUnchecked((c >> 0) & 0xF);
					*(S++) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
				}
				
				return S;				
			}
			static uint64_t decodeRead(uint8_t const * D, ::libmaus::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus::autoarray::AutoArray<char>(seqlen);
				char * S = A.begin();
				
				decodeRead(D,S,seqlen);
				
				return seqlen;
			}
			static std::string decodeReadS(uint8_t const * D, ::libmaus::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = decodeRead(D,A);
				return std::string(A.begin(),A.begin()+seqlen);
			}
			static uint64_t decodeReadRC(uint8_t const * D, ::libmaus::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus::autoarray::AutoArray<char>(seqlen);
				uint8_t const * cseq = getSeq(D);
				char * S = A.begin()+seqlen;
				for ( uint64_t i = 0; i < (seqlen>>1); ++i )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
					*(--S) = decodeSymbolRCUnchecked((c >> 0) & 0xF);
				}
				if ( seqlen & 1 )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
				}
				
				return seqlen;
			}
			static uint64_t decodeQual(uint8_t const * D, ::libmaus::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus::autoarray::AutoArray<char>(seqlen);
				uint8_t const * qual = getQual(D);
				char * S = A.begin();
				for ( uint64_t i = 0; i < seqlen; ++i )
				{
					*(S++) = (*(qual++))+33;
				}
				return seqlen;
			}
			template<typename iterator>
			static iterator decodeQualIt(uint8_t const * D, iterator it, unsigned int const seqlen)
			{
				if ( seqlen )
				{
					uint8_t const * qual = getQual(D);
					uint8_t const * const quale = qual + seqlen;
					
					if ( qual[0] == 255 )
					{
						while ( qual++ != quale )
							*it++ = '*';
					}
					else
					{
						while ( qual != quale )
							*it++ = (*(qual++))+ 33;
					}
				}

				return it;
			}
			template<typename iterator>
			static iterator decodeQualRcIt(uint8_t const * D, iterator it, unsigned int const seqlen)
			{
				if ( seqlen )
				{
					uint8_t const * qual  = getQual(D) + seqlen;
					uint8_t const * const quala = getQual(D);
					
					if ( qual[0] == 255 )
					{
						while ( qual-- != quala )
							*it++ = '*';
					}
					else
					{
						while ( qual != quala )
							*it++ = (*(--qual))+ 33;
					}
				}

				return it;
			}
			static std::string decodeQual(uint8_t const * D)
			{
				uint64_t const seqlen = getLseq(D);
				uint8_t const * qual = getQual(D);
				::std::string s(reinterpret_cast<char const *>(qual),reinterpret_cast<char const *>(qual+seqlen));
				for ( uint64_t i = 0; i < seqlen; ++i )
					s[i] += 33;
				return s;
			}

			static uint64_t decodeQualRC(uint8_t const * D, ::libmaus::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus::autoarray::AutoArray<char>(seqlen);
				uint8_t const * qual = getQual(D);
				char * S = A.begin() + seqlen;
				for ( uint64_t i = 0; i < seqlen; ++i )
				{
					*(--S) = (*(qual++))+33;
				}
				return seqlen;
			}
			
			static char const * idToChromosome(int32_t const id, std::vector< ::libmaus::bambam::Chromosome > const & chromosomes)
			{
				if ( id >= 0 && id < static_cast<int32_t>(chromosomes.size()) )
					return chromosomes[id].name.c_str();
				else
					return "*";
			}
			
			static uint8_t const * getAux(
				uint8_t const * E, uint64_t const blocksize, 
				char const * const tag
			)
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );
				
				uint8_t const * aux = getAux(E);
				
				while ( aux < E+blocksize && *aux )
				{
					if ( aux[0] == tag[0] && aux[1] == tag[1] )
						return aux;

					aux = aux + getAuxLength(aux);
				}
				
				return 0;
			}

			static char const * getAuxString(uint8_t const * E, uint64_t const blocksize, char const * const tag)
			{
				uint8_t const * data = getAux(E,blocksize,tag);
				
				if ( data && data[2] == 'Z' )
					return reinterpret_cast<char const *>(data+3);
				else
					return 0;
			}
						
			static uint8_t const * getAuxRankData(
				uint8_t const * E, uint64_t const blocksize
			)
			{
				return getAux(E,blocksize,"ZR");
			}
			
			static uint64_t getAuxRank(uint8_t const * E, uint64_t const blocksize)
			{
				uint8_t const * data = getAuxRankData(E,blocksize);
				
				if ( ! data )
					return 0;
					
				assert ( data[0] == 'Z' );
				assert ( data[1] == 'R' );
				
				if ( data[2] != 'B' )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "Rank aux tag ZR has wrong data type " << data[2] << std::endl;
					se.finish();
					throw se;
				}
				if ( data[3] != 'C' )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "Rank aux tag ZR has wrong data type " << data[3] << std::endl;
					se.finish();
					throw se;
				}
				if ( data[4] != 8 || data[5] != 0 || data[6] != 0 || data[7] != 0 )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "Rank aux tag ZR has wrong array size" << std::endl;
					se.finish();
					throw se;
				}
				
				return
					(static_cast<uint64_t>(data[8]) << 56) |
					(static_cast<uint64_t>(data[9]) << 48) |
					(static_cast<uint64_t>(data[10]) << 40) |
					(static_cast<uint64_t>(data[11]) << 32) |
					(static_cast<uint64_t>(data[12]) << 24) |
					(static_cast<uint64_t>(data[13]) << 16) |
					(static_cast<uint64_t>(data[14]) <<  8) |
					(static_cast<uint64_t>(data[15]) <<  0);
			}

			static uint8_t const * getAuxEnd(
				uint8_t const * E, uint64_t const blocksize
			)
			{
				uint8_t const * aux = getAux(E);
				
				while ( aux < E+blocksize && *aux )
					aux = aux + getAuxLength(aux);
				
				return aux;
			}

			static uint8_t * getAuxEnd(
				uint8_t * E, uint64_t const blocksize
			)
			{
				uint8_t * aux = getAux(E);
				
				while ( aux < E+blocksize && *aux )
					aux = aux + getAuxLength(aux);
				
				return aux;
			}
			
			static std::string getAuxAsString(uint8_t const * E, uint64_t const blocksize, char const * const tag)
			{
				uint8_t const * D = getAux(E,blocksize,tag);
				
				if ( D )
					return auxValueToString(D);
				else
					return std::string();
			}
			
			static std::string formatAlignment(
				uint8_t const * E, uint64_t const blocksize, std::vector< ::libmaus::bambam::Chromosome > const & chromosomes,
				::libmaus::bambam::BamFormatAuxiliary & auxdata
			)
			{
				std::ostringstream ostr;

				ostr << getReadNameS(E) << "\t";
				ostr << getFlags(E) << "\t";
				ostr << idToChromosome(getRefID(E),chromosomes) << "\t";
				ostr << getPos(E)+1 << "\t";
				ostr << getMapQ(E) << "\t";
				ostr << (getNCigar(E) ? getCigarString(E) : std::string("*")) << "\t";
				
				if ( getRefID(E) == getNextRefID(E) && getRefID(E) >= 0 )
					ostr << "=\t";
				else
					ostr << idToChromosome(getNextRefID(E),chromosomes) << "\t";

				ostr << getNextPos(E)+1 << "\t";
				ostr << getTlen(E) << "\t";

				uint64_t const seqlen = decodeRead(E,auxdata.seq);
				std::string const seqs(auxdata.seq.begin(),auxdata.seq.begin()+seqlen);
				ostr << seqs << "\t";

				if ( seqlen && getQual(E)[0] == 255 )
				{
					ostr << '*';
				}
				else
				{
					uint64_t const quallen = decodeQual(E,auxdata.qual);
					std::string const squal(auxdata.qual.begin(),auxdata.qual.begin()+quallen);
					ostr << squal;
				}
				
				uint8_t const * aux = getAux(E);
				
				while ( aux < E+blocksize && *aux )
				{
					ostr.put('\t');
					ostr.put(aux[0]);
					ostr.put(aux[1]);
					ostr.put(':');
					switch ( aux[2] )
					{
						case 'c':
						case 'C':
						case 's':
						case 'S':
						case 'i':
						case 'I': ostr.put('i'); break;
						default: ostr.put(aux[2]); break;
					}
					ostr.put(':');				
					// ostr << getAuxLength(aux);
					ostr << auxValueToString(aux);
					aux = aux + getAuxLength(aux);
				}

				return ostr.str();
			}

			static std::string formatFastq(
				uint8_t const * E, 
				::libmaus::bambam::BamFormatAuxiliary & auxdata
			)
			{
				std::ostringstream ostr;

				uint32_t const flags = getFlags(E);

				// name
				ostr << "@" << getReadNameS(E);
				
				if ( isPaired(flags) )
				{
					if ( isRead1(flags) )
						ostr << "/1";
					else
						ostr << "/2";
				}
				
				ostr.put('\n');

				uint64_t const seqlen = isReverse(flags) ? decodeReadRC(E,auxdata.seq) : decodeRead(E,auxdata.seq);
				std::string const seqs(auxdata.seq.begin(),auxdata.seq.begin()+seqlen);
				ostr << seqs << "\n";
				
				ostr << "+\n";

				uint64_t const quallen = isReverse(flags) ? decodeQualRC(E,auxdata.qual) : decodeQual(E,auxdata.qual);
				std::string const squal(auxdata.qual.begin(),auxdata.qual.begin()+quallen);
				ostr << squal << "\n";
				
				return ostr.str();
			}
		};
	}
}
#endif
