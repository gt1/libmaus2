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
#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/util/ArgInfo.hpp>

struct SequenceComparison
{
	libmaus2::bambam::BamHeader header;

	::libmaus2::autoarray::AutoArray<char> seqa;
	::libmaus2::autoarray::AutoArray<char> seqb;
	
	::libmaus2::autoarray::AutoArray<int64_t> mapa;
	::libmaus2::autoarray::AutoArray<int64_t> mapb;
	libmaus2::autoarray::AutoArray<libmaus2::bambam::cigar_operation> cigop;

	SequenceComparison(libmaus2::bambam::BamHeader const & rheader) : header(rheader)
	{
	
	}
	
	bool operator()(libmaus2::bambam::BamAlignment & A, libmaus2::bambam::BamAlignment & B)
	{
		uint64_t la = A.decodeRead(seqa);
		uint64_t lb = B.decodeRead(seqb);

		bool eq = (la == lb) && ((strncmp(seqa.begin(),seqb.begin(),la) == 0));
		
		B.reverseComplementInplace();
		la = A.decodeRead(seqa);
		lb = B.decodeRead(seqb);
		B.reverseComplementInplace();

		eq = eq || ( (la == lb) && ((strncmp(seqa.begin(),seqb.begin(),la) == 0)) );

		return eq;
	}

	void flip(
		libmaus2::bambam::BamAlignment & ala_1, libmaus2::bambam::BamAlignment & ala_2
	)
	{
		ala_1.reverseComplementInplace();

		uint16_t const fmask = static_cast<uint16_t>(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE);
		uint16_t const rmask = static_cast<uint16_t>(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMREVERSE);

		ala_1.putFlags ( ala_1.getFlags() ^ fmask );
		ala_2.putFlags ( ala_2.getFlags() ^ rmask );
	}
	
	bool opposite(libmaus2::bambam::BamAlignment & A, libmaus2::bambam::BamAlignment & B)
	{
		B.reverseComplementInplace();
		uint64_t const la = A.decodeRead(seqa);
		uint64_t const lb = B.decodeRead(seqb);
		bool const op = (la == lb) && (strncmp(seqa.begin(),seqb.begin(),la) == 0);
		B.reverseComplementInplace();
	
		#if 0	
		if ( op )
		{
			std::cerr  << "opposite\n" 
				<< A.formatAlignment(header) << "\n" 
				<< B.formatAlignment(header) << "\n";
		}
		else
		{
			std::cerr  << "non opposite\n" 
				<< A.formatAlignment(header) << "\n" 
				<< B.formatAlignment(header) << "\n";		
		}
		#endif
		
		return op;
	}
	
	void checkFlip(
		libmaus2::bambam::BamAlignment & ala_1, libmaus2::bambam::BamAlignment & ala_2,
		libmaus2::bambam::BamAlignment & alb_1, libmaus2::bambam::BamAlignment & alb_2
	)
	{
		if ( ala_1.isMapped() && !alb_1.isMapped() && ala_1.isReverse() != alb_1.isReverse() )
			flip(alb_1,alb_2);
		if ( ala_2.isMapped() && !alb_2.isMapped() && ala_2.isReverse() != alb_2.isReverse() )
			flip(alb_2,alb_1);
		if ( alb_1.isMapped() && !ala_1.isMapped() && alb_1.isReverse() != ala_1.isReverse() )
			flip(ala_1,ala_2);
		if ( alb_2.isMapped() && !ala_2.isMapped() && alb_2.isReverse() != ala_2.isReverse() )
			flip(ala_2,ala_1);
	}

	void checkFlipRaw(
		libmaus2::bambam::BamAlignment & ala_1, libmaus2::bambam::BamAlignment & ala_2,
		libmaus2::bambam::BamAlignment & alb_1, libmaus2::bambam::BamAlignment & alb_2
	)
	{
		if ( ala_1.isMapped() && (!alb_1.isMapped()) && opposite(ala_1,alb_1) )
			flip(alb_1,alb_2);
		if ( ala_2.isMapped() && (!alb_2.isMapped()) && opposite(ala_2,alb_2) )
			flip(alb_2,alb_1);
		if ( alb_1.isMapped() && (!ala_1.isMapped()) && opposite(alb_1,ala_1) )
			flip(ala_1,ala_2);
		if ( alb_2.isMapped() && (!ala_2.isMapped()) && opposite(alb_2,ala_2) )
			flip(ala_2,ala_1);
	}

	void checkFlipRawUnmapped(
		libmaus2::bambam::BamAlignment & ala_1, libmaus2::bambam::BamAlignment & ala_2,
		libmaus2::bambam::BamAlignment & alb_1, libmaus2::bambam::BamAlignment & alb_2
	)
	{
		if ( (!ala_1.isMapped()) && (!alb_1.isMapped()) && opposite(ala_1,alb_1) )
			flip(alb_1,alb_2);
		if ( (!ala_2.isMapped()) && (!alb_2.isMapped()) && opposite(ala_2,alb_2) )
			flip(alb_2,alb_1);
	}
	
	void fillMap(libmaus2::bambam::BamAlignment & A, ::libmaus2::autoarray::AutoArray<int64_t> & M)
	{
		uint64_t const l = A.getLseq();
		if ( l > M.size() )
			M = ::libmaus2::autoarray::AutoArray<int64_t>(l,false);
		std::fill(M.begin(),M.end(),-1);

		if ( A.isMapped() )
		{
			uint32_t const numop = A.getCigarOperations(cigop);
			uint32_t i = 0;
			
			int64_t readpos = 0;
			int64_t refpos = -1;
			
			for ( ; 
				i < numop && 
				cigop[i].first != ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH &&
				cigop[i].first != ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL &&
				cigop[i].first != ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF 
				; ++i
			)
			{
				switch ( cigop[i].first )
				{
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
						assert(0);
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS:
						readpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
						// refpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
						// refpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP:
						readpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP:
						// readpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
						break;
				}
			}
			
			refpos = A.getPos();

			for ( ; i < numop ; ++i )
			{
				switch ( cigop[i].first )
				{
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
						for ( int64_t j = 0; j < cigop[i].second; ++j )
							M[readpos++] = refpos++;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS:
						readpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
						refpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
						refpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP:
						readpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP:
						// readpos += cigop[i].second;
						break;
					case ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
						break;
				}
			
			}
		}
	}
	
	bool matchingMatchPos(libmaus2::bambam::BamAlignment & A, libmaus2::bambam::BamAlignment & B)
	{
		if ( A.isMapped() && B.isMapped() )
		{
			fillMap(A,mapa);
			fillMap(B,mapb);

			uint64_t const la = A.getLseq();
			uint64_t const lb = B.getLseq();
			
			if ( la != lb )
				return false;
				
			bool ok = true;
				
			// std::cerr << std::string(80,'-') << std::endl;
				
			for ( uint64_t i = 0; ok && i < la; ++i )
			{
				if ( mapa[i] >= 0 && mapb[i] >= 0 && mapa[i] != mapb[i] )
					ok = false;
					
				// std::cerr << mapa[i] << " , " << mapb[i] << std::endl;
			}
			
			return ok;
		}
		else if ( !A.isMapped() && !B.isMapped() )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		
		std::string const fna = arginfo.getRestArg<std::string>(0);
		std::string const fnb = arginfo.getRestArg<std::string>(1);
		
		libmaus2::bambam::BamDecoder bama(fna);
		libmaus2::bambam::BamDecoder bamb(fnb);

		libmaus2::bambam::BamHeader const & heada = bama.getHeader();
		// libmaus2::bambam::BamHeader const & headb = bamb.getHeader();
		
		libmaus2::bambam::BamAlignment & ala = bama.getAlignment();
		libmaus2::bambam::BamAlignment & alb = bamb.getAlignment();
		
		libmaus2::bambam::BamAlignment ala_1, ala_2;
		libmaus2::bambam::BamAlignment alb_1, alb_2;
		
		::libmaus2::bambam::BamFormatAuxiliary aux;
		// uint64_t alcnt = 0;
		// bool eq = true;
		// uint64_t const mod = 16*1024*1024;
		
		SequenceComparison seqcomp(heada);
		
		::libmaus2::autoarray::AutoArray<char> ala_1_r;
		::libmaus2::autoarray::AutoArray<char> ala_2_r;
		::libmaus2::autoarray::AutoArray<char> alb_1_r;
		::libmaus2::autoarray::AutoArray<char> alb_2_r;
		
		uint64_t cnteq = 0;
		uint64_t cntdif = 0;
		uint64_t cntdifmap = 0;
		uint64_t cntdiflow = 0;
		uint64_t cntdifhigh = 0;
		uint64_t mapa = 0;
		uint64_t mapb = 0;
		
		while ( bama.readAlignment() )
		{
			ala_1.swap(ala);
			
			if ( ! bama.readAlignment() )
			{
				libmaus2::exception::LibMausException se;
				se.getStream() << "EOF on " << fna << " while looking for mate." << std::endl;
				se.finish();
				throw se;				
			}
			
			ala_2.swap(ala);
		
			if ( ! bamb.readAlignment() )
			{
				libmaus2::exception::LibMausException se;
				se.getStream() << "EOF on " << fnb << std::endl;
				se.finish();
				throw se;
			}
			
			alb_1.swap(alb);

			if ( ! bamb.readAlignment() )
			{
				libmaus2::exception::LibMausException se;
				se.getStream() << "EOF on " << fnb << " while looking for mate." << std::endl;
				se.finish();
				throw se;
			}
			
			alb_2.swap(alb);
						
			if ( seqcomp(ala_1,alb_2) && seqcomp(ala_2,alb_1) && (!seqcomp(ala_1,ala_2)) )
			{
				uint16_t const rmask = 
					static_cast<uint16_t>(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1) 
					| 
					static_cast<uint16_t>(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2);

				alb_1.putFlags ( (alb_1.getFlags() ^ rmask) );
				alb_2.putFlags ( (alb_2.getFlags() ^ rmask) );
				alb_1.swap(alb_2);
			}
			
			seqcomp.checkFlip   (ala_1,ala_2,alb_1,alb_2);
			seqcomp.checkFlipRaw(ala_1,ala_2,alb_1,alb_2);
			seqcomp.checkFlipRawUnmapped(ala_1,ala_2,alb_1,alb_2);
			
			bool diff = false;
			
			// diff = diff ||  (ala_1.getFlags() != alb_1.getFlags()) || (ala_2.getFlags() != alb_2.getFlags());
			diff = diff || (!seqcomp.matchingMatchPos(ala_1,alb_1));
			diff = diff || (!seqcomp.matchingMatchPos(ala_2,alb_2));
			
			unsigned int const mapcnta = (ala_1.isMapped() ? 1 : 0) + (ala_2.isMapped() ? 1 : 0);
			unsigned int const mapcntb = (alb_1.isMapped() ? 1 : 0) + (alb_2.isMapped() ? 1 : 0);
			
			mapa += mapcnta;
			mapb += mapcntb;
				
			if ( diff )
			{
				cntdif++;
				
				if ( mapcnta == mapcntb )
				{
					cntdifmap++;
			
					int64_t const mapq_a_1 = ala_1.isMapped() ? ala_1.getMapQ() : 0;
					int64_t const mapq_a_2 = ala_2.isMapped() ? ala_2.getMapQ() : 0;
					int64_t const mapq_b_1 = alb_1.isMapped() ? alb_1.getMapQ() : 0;
					int64_t const mapq_b_2 = alb_2.isMapped() ? alb_2.getMapQ() : 0;
					
					if ( 
						(ala_1.isMapped() && (mapq_a_1 <= 3))
						||
						(ala_2.isMapped() && (mapq_a_2 <= 3))
						||
						(alb_1.isMapped() && (mapq_b_1 <= 3))
						||
						(alb_2.isMapped() && (mapq_b_2 <= 3))
					)
					{
						cntdiflow++;
					}
					else
					{
						cntdifhigh++;

						#if 0
						std::string const sama_1 = ala_1.formatAlignment(heada,aux);
						std::string const sama_2 = ala_2.formatAlignment(heada,aux);
						
						std::string const samb_1 = alb_1.formatAlignment(headb,aux);
						std::string const samb_2 = alb_2.formatAlignment(headb,aux);
				
						std::cout << "***DIFF***" << std::endl;	
						std::cout << sama_1 << std::endl;
						std::cout << samb_1 << std::endl;
						std::cout << sama_2 << std::endl;
						std::cout << samb_2 << std::endl;
						#endif

					}
					
					#if 0
					std::string const sama_1 = ala_1.formatAlignment(heada,aux);
					std::string const sama_2 = ala_2.formatAlignment(heada,aux);
					
					std::string const samb_1 = alb_1.formatAlignment(headb,aux);
					std::string const samb_2 = alb_2.formatAlignment(headb,aux);
			
					std::cout << "***DIFF***" << std::endl;	
					std::cout << sama_1 << std::endl;
					std::cout << samb_1 << std::endl;
					std::cout << sama_2 << std::endl;
					std::cout << samb_2 << std::endl;
					#endif
				}
			}
			else
			{
				cnteq++;

				#if 0
				std::string const sama_1 = ala_1.formatAlignment(heada,aux);
				std::string const sama_2 = ala_2.formatAlignment(heada,aux);
				
				std::string const samb_1 = alb_1.formatAlignment(headb,aux);
				std::string const samb_2 = alb_2.formatAlignment(headb,aux);

				std::cout << "***EQ***" << std::endl;	
				std::cout << sama_1 << std::endl;
				std::cout << samb_1 << std::endl;
				std::cout << sama_2 << std::endl;
				std::cout << samb_2 << std::endl;
				#endif
			}
			
			#if 0
			if ( 
				(ala.blocksize != alb.blocksize) 
				||
				memcmp(ala.D.begin(),alb.D.begin(),ala.blocksize)
			)
			{
				std::string const sama = ala.formatAlignment(heada,aux);
				std::string const samb = alb.formatAlignment(headb,aux);
			
				if ( sama != samb )
				{
					std::cerr << "--- Difference ---" << std::endl;
					std::cerr << sama << std::endl;
					std::cerr << samb << std::endl;
					eq = false;
				}
			
			}

			if ( (++alcnt) % (mod) == 0 )
				std::cerr << "[V] " << alcnt/(mod) << std::endl;
			#endif
			
			if ( (cnteq + cntdif) % (128*1024) == 0 )
			{
				std::cerr << "eq=" << cnteq << " dif=" << cntdif << " difmap=" << cntdifmap 
					<< " diflow=" << cntdiflow
					<< " difhigh=" << cntdifhigh
					<< " mapcnta=" << mapa
					<< " mapcntb=" << mapb
					<< std::endl;
			}
		}

		std::cerr << "eq=" << cnteq << " dif=" << cntdif << " difmap=" << cntdifmap 
			<< " diflow=" << cntdiflow
			<< " difhigh=" << cntdifhigh
			<< " mapcnta=" << mapa
			<< " mapcntb=" << mapb
			<< std::endl;

		if ( bama.readAlignment() )
		{
			libmaus2::exception::LibMausException se;
			se.getStream() << "EOF on " << fna << std::endl;
			se.finish();
			throw se;
		}
		
		// std::cerr << (eq?"equal":"different") << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
