/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/bambam/BamMultiAlignmentDecoderFactory.hpp>
#include <libmaus/bambam/SamInfo.hpp>

int main(int argc, char *argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type Pdecoder(libmaus::bambam::BamMultiAlignmentDecoderFactory::construct(arginfo));
		libmaus::bambam::BamAlignmentDecoder & decoder = Pdecoder->getDecoder();
		libmaus::bambam::BamAlignment const & algn = decoder.getAlignment();
		libmaus::bambam::BamHeader const & header = decoder.getHeader();
		::libmaus::bambam::BamFormatAuxiliary aux;
		libmaus::bambam::SamInfo saminfo(header);
		std::ostringstream ostr;
		std::ostringstream checkostr;
		std::string const empty;
		libmaus::bambam::BamAlignment & samalgn = saminfo.algn;
		::libmaus::autoarray::AutoArray<char> A, B;
		libmaus::autoarray::AutoArray < std::pair<uint8_t,uint8_t> > auxA, auxB;
		uint64_t c = 0;		
		
		while ( decoder.readAlignment() )
		{
			ostr.str(empty);
			algn.formatAlignment(ostr,header,aux);
			ostr.put('\n');
			std::string const s = ostr.str();
			saminfo.parseSamLine(s.c_str(),s.c_str()+s.size()-1);
			
			ptrdiff_t const baseblocklength = libmaus::bambam::BamAlignmentDecoderBase::getAux(algn.D.begin()) - algn.D.begin();
			ptrdiff_t const sbaseblocklength = libmaus::bambam::BamAlignmentDecoderBase::getAux(samalgn.D.begin()) - samalgn.D.begin();

			assert ( algn.getLReadName() == samalgn.getLReadName() );
			assert ( strcmp(algn.getName(),samalgn.getName()) == 0 );
			assert ( algn.getRefID() == samalgn.getRefID() );
			assert ( algn.getPos() == samalgn.getPos() );
			assert ( algn.getNextRefID() == samalgn.getNextRefID() );
			assert ( algn.getNextPos() == samalgn.getNextPos() );
			assert ( algn.getFlags() == samalgn.getFlags() );
			assert ( algn.getNCigar() == samalgn.getNCigar() );
			assert ( algn.getTlen() == samalgn.getTlen() );
			assert ( algn.getMapQ() == samalgn.getMapQ() );
			assert ( algn.getLseq() == samalgn.getLseq() );
			for ( uint64_t i = 0; i < algn.getNCigar(); ++i )
				assert ( algn.getCigarField(i) == samalgn.getCigarField(i) );

			// only check bin for aligned lines, recoding kills bin set by fix mates
			assert ( (!algn.isMapped()) || (algn.getBin() == samalgn.getBin()) );

			uint64_t l;
			assert ( (l=algn.decodeRead(A)) == samalgn.decodeRead(B) );
			assert ( memcmp(A.begin(),B.begin(),l) == 0 );
			assert ( (l=algn.decodeQual(A)) == samalgn.decodeQual(B) );
			assert ( memcmp(A.begin(),B.begin(),l) == 0 );

			assert (baseblocklength == sbaseblocklength);
			assert ( (l=algn.enumerateAuxTags(auxA)) == samalgn.enumerateAuxTags(auxB) );
			
			std::sort(auxA.begin(),auxA.begin()+l);
			std::sort(auxB.begin(),auxB.begin()+l);
			
			for ( uint64_t i = 0; i < l; ++i )
			{
				assert ( auxA[i] == auxB[i] );

				char const tag[] = { auxA[i].first, auxA[i].second, 0 };
				
				uint8_t const * fieldA = libmaus::bambam::BamAlignmentDecoderBase::getAux(algn.D.begin(),algn.blocksize,&tag[0]);
				uint8_t const * fieldB = libmaus::bambam::BamAlignmentDecoderBase::getAux(samalgn.D.begin(),samalgn.blocksize,&tag[0]);
				
				assert ( fieldA );
				assert ( fieldB );

				switch ( fieldA[2] )
				{
					case 'A':
					case 'c':
					case 'C':
					case 's': 
					case 'S':
					case 'i': 
					case 'I': 
						assert (
							libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int64_t>(algn.D.begin(),algn.blocksize,&tag[0])
							==						
							libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int64_t>(samalgn.D.begin(),samalgn.blocksize,&tag[0])
						);
						break;
					case 'f':
						assert (
							libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumber<double>(algn.D.begin(),algn.blocksize,&tag[0])
							==						
							libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumber<double>(samalgn.D.begin(),samalgn.blocksize,&tag[0])
						);
						break;
					case 'B':
						if ( fieldA[3] == 'f' )
						{
							assert (
								libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumberArray<double>(algn.D.begin(),algn.blocksize,&tag[0])
								==
								libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumberArray<double>(samalgn.D.begin(),samalgn.blocksize,&tag[0])
							);
						}
						else
						{						
							assert (
								libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumberArray<int64_t>(algn.D.begin(),algn.blocksize,&tag[0])
								==
								libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumberArray<int64_t>(samalgn.D.begin(),samalgn.blocksize,&tag[0])
							);
						}
						break;
					case 'Z':
						assert (
							algn.getAuxString(&tag[0]) &&
							samalgn.getAuxString(&tag[0]) &&
							strcmp(algn.getAuxString(&tag[0]),samalgn.getAuxString(&tag[0])) == 0
						);
						break;
					default:
						std::cerr << "ignoring " << tag << " type " << fieldA[2] << std::endl;
						break;
				}
			}
			
			if ( ++c % (1024*1024) == 0 )
				std::cerr << "[V] " << c << std::endl;			
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
