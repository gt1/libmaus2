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
#if ! defined(LIBMAUS_BAMBAM_BAMBLOCKWRITERBASEFACTORY_HPP)
#define LIBMAUS_BAMBAM_BAMBLOCKWRITERBASEFACTORY_HPP

#include <libmaus/bambam/BamWriter.hpp>
#if defined(LIBMAUS_HAVE_IO_LIB)
#include <libmaus/bambam/ScramEncoder.hpp>
#endif
#include <libmaus/bambam/SamEncoder.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamBlockWriterBaseFactory
		{
			typedef BamBlockWriterBaseFactory this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BamBlockWriterBaseFactory() {}
			virtual ~BamBlockWriterBaseFactory() {}
			
			static std::string levelToString(int const level)
			{
				switch ( level )
				{
					case Z_DEFAULT_COMPRESSION:
						return "zlib default";
					case Z_BEST_SPEED:
						return "fast";
					case Z_BEST_COMPRESSION:
						return "best";
					case Z_NO_COMPRESSION:
						return "uncompressed";				
					#if defined(LIBMAUS_HAVE_IGZIP)
					case libmaus::lz::IGzipDeflate::COMPRESSION_LEVEL:
						return "igzip";
					#endif
					default:
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "BamBlockWriterBaseFactory::levelToString(): Unknown compression level " << level << std::endl;
						se.finish();
						throw se;
					}
				}
			}
			
			static std::string getLevelHelpText()
			{
				std::set<int> S = getValidCompressionLevels();
				std::vector<int> V(S.begin(),S.end());
				
				std::ostringstream ostr;
				for ( std::vector<int>::size_type i = 0; i < V.size(); ++i )
					ostr << V[i] << "=" << levelToString(V[i]) << ((i+1<V.size())?",":"");
					
				return ostr.str();
			}
			
			static std::string getBamOutputLevelHelpText()
			{
				return std::string("compression settings for output bam file (") + getLevelHelpText() + std::string(")");
			}
			
			static std::set<int> getValidCompressionLevels()
			{
				std::set<int> S;
				S.insert(Z_DEFAULT_COMPRESSION);
				S.insert(Z_BEST_SPEED);
				S.insert(Z_BEST_COMPRESSION);
				S.insert(Z_NO_COMPRESSION);
				#if defined(LIBMAUS_HAVE_IGZIP)
				S.insert(libmaus::lz::IGzipDeflate::getCompressionLevel());
				#endif
				return S;
			}

			static int checkCompressionLevel(int const level)
			{
				switch ( level )
				{
					case Z_NO_COMPRESSION:
					case Z_BEST_SPEED:
					case Z_BEST_COMPRESSION:
					case Z_DEFAULT_COMPRESSION:
					#if defined(LIBMAUS_HAVE_IGZIP)
					case libmaus::lz::IGzipDeflate::COMPRESSION_LEVEL:
					#endif
						break;
					default:
					{
						::libmaus::exception::LibMausException se;
						se.getStream()
							<< "Unknown compression level, please use"
							<< " level=" << Z_DEFAULT_COMPRESSION << " (default) or"
							<< " level=" << Z_BEST_SPEED << " (fast) or"
							<< " level=" << Z_BEST_COMPRESSION << " (best) or"
							<< " level=" << Z_NO_COMPRESSION << " (no compression)"
							#if defined(LIBMAUS_HAVE_IGZIP)
							<< " or level=" << libmaus::lz::IGzipDeflate::COMPRESSION_LEVEL << " (igzip)"
							#endif
							<< std::endl;
						se.finish();
						throw se;
					}
						break;
				}
				
				return level;
			}
			
			static std::set<std::string> getValidOutputFormatsSet()
			{
				std::set<std::string> S;
				S.insert("bam");

				#if defined(LIBMAUS_HAVE_IO_LIB)
				S.insert("sam");
				S.insert("cram");
				#endif
				
				return S;
			}
			
			static std::string getValidOutputFormats()
			{
				std::set<std::string> const S = getValidOutputFormatsSet();

				std::ostringstream ostr;
				for ( std::set<std::string>::const_iterator ita = S.begin();
					ita != S.end(); ++ita )
					ostr << ((ita!=S.begin())?",":"") << (*ita);
				
				return ostr.str();
			}
			
			static std::string getDefaultOutputFormat()
			{
				return "bam";
			}

			static libmaus::bambam::BamBlockWriterBase::unique_ptr_type construct(
				libmaus::bambam::BamHeader const & bamheader,
				libmaus::util::ArgInfo const & arginfo,
				std::vector< ::libmaus::lz::BgzfDeflateOutputCallback *> const * rblockoutputcallbacks = 0
			)
			{
				std::string const outputformat = arginfo.getValue<std::string>("outputformat",getDefaultOutputFormat());
				uint64_t const outputthreads = std::max(static_cast<uint64_t>(1),arginfo.getValue<uint64_t>("outputthreads",1));
				bool const outputisstdout = (!arginfo.hasArg("O")) || ( arginfo.getUnparsedValue("O","-") == std::string("-") );
				std::string const outputfilename = arginfo.getUnparsedValue("O","-");

				if ( (outputformat != "bam") && rblockoutputcallbacks && rblockoutputcallbacks->size() )
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "libmaus::bambam::BamBlockWriterBaseFactory: output callbacks are not supported for output formats other than bam" << std::endl;
					ex.finish();
					throw ex;
				}
				
				if ( outputformat == "bam" )
				{
					int const level = checkCompressionLevel(arginfo.getValue("level",Z_DEFAULT_COMPRESSION));
					
					if ( outputthreads == 1 )
					{
						if ( outputisstdout )
						{
							libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::BamWriter(std::cout,bamheader,level,rblockoutputcallbacks));
							return UNIQUE_PTR_MOVE(tptr);
						}
						else
						{
							libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::BamWriter(outputfilename,bamheader,level,rblockoutputcallbacks));
							return UNIQUE_PTR_MOVE(tptr);
						}
					}
					else
					{
						if ( outputisstdout )
						{
							libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::BamParallelWriter(std::cout,outputthreads,bamheader,level,rblockoutputcallbacks));
							return UNIQUE_PTR_MOVE(tptr);
						}
						else
						{
							libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::BamParallelWriter(outputfilename,outputthreads,bamheader,level,rblockoutputcallbacks));
							return UNIQUE_PTR_MOVE(tptr);
						}
					}
				}
				else if ( 
					outputformat == "maussam"					
					#if !defined(LIBMAUS_HAVE_IO_LIB)
					||
					outputformat == "sam"
					#endif
				)
				{
					if ( outputisstdout )
					{
						libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(
							new libmaus::bambam::SamEncoder(std::cout,bamheader)
						);
						return UNIQUE_PTR_MOVE(tptr);
					}
					else
					{
						libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(
							new libmaus::bambam::SamEncoder(outputfilename,bamheader)
						);
						return UNIQUE_PTR_MOVE(tptr);
					}
				
				}
				#if defined(LIBMAUS_HAVE_IO_LIB)
				else if ( outputformat == "sam" )
				{

					if ( outputisstdout )
					{
						libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::ScramEncoder(bamheader,"-","ws","",true /* verbose */));
						return UNIQUE_PTR_MOVE(tptr);
					}
					else
					{					
						libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::ScramEncoder(bamheader,outputfilename,"ws","",true /* verbose */));
						return UNIQUE_PTR_MOVE(tptr);
					}
				}
				else if ( outputformat == "cram" )
				{
					std::string const reference = arginfo.getUnparsedValue("reference","");
					bool const scramverbose = arginfo.getValue<unsigned int>("scramverbose",false);
				
					if ( outputisstdout )
					{
						libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::ScramEncoder(bamheader,"-","wc",reference,scramverbose /* verbose */));
						return UNIQUE_PTR_MOVE(tptr);
					}
					else
					{					
						libmaus::bambam::BamBlockWriterBase::unique_ptr_type tptr(new libmaus::bambam::ScramEncoder(bamheader,outputfilename,"wc",reference,scramverbose /* verbose */));
						return UNIQUE_PTR_MOVE(tptr);
					}
				}
				#endif
				else
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BamBlockWriterBaseFactory::construct(): unknown/unsupported output format " << outputformat << std::endl;
					se.finish();
					throw se;
				}
			}
		};
	}
}
#endif
