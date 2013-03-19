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


#if ! defined(LIBMAUS_AIO_BLOCKSYNCHRONOUSOUTPUTBUFFER8_HPP)
#define LIBMAUS_AIO_BLOCKSYNCHRONOUSOUTPUTBUFFER8_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/aio/SynchronousOutputBuffer8.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <fstream>
#include <vector>
#include <map>
#include <iomanip>

namespace libmaus
{
	namespace aio
	{
		// fixed size buffer
		template<typename _data_type>
		struct BlockBufferTemplate
		{
			typedef _data_type data_type;
		
			typedef BlockBufferTemplate<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus::autoarray::AutoArray<data_type> B;
			data_type * const pa;
			data_type * pc;
			data_type * const pe;
			
			BlockBufferTemplate(uint64_t const s)
			: B(s), pa(B.get()), pc(pa), pe(pa+s)
			{
			
			}
			
			bool put(data_type const v)
			{
				*(pc++) = v;
				return pc == pe;
			}
			
			uint64_t getFill() const
			{
				return pc - pa;
			}
			
			uint64_t getFillBytes() const
			{
				return getFill() * sizeof(data_type);
			}
			
			void reset()
			{
				pc = pa; 
			}
		
			// write buffer	
			void writeOut(std::ostream & out)
			{
				out.write ( reinterpret_cast<char const *>(pa) , getFillBytes() );
				reset();
			}
		
			// fill buffer and then write it
			void writeFull(std::ostream & out)
			{
				while ( pc != pe ) 
					put(0);

				assert ( getFill() == B.getN() );
				
				writeOut(out);
			}
		};

		typedef BlockBufferTemplate<uint64_t> BlockBuffer;
	
		struct BlockSynchronousOutputBuffer8
		{
			typedef std::ofstream ostr_type;
			typedef ::libmaus::util::unique_ptr<ostr_type>::type ostr_ptr_type;

			std::string const filename;
			ostr_ptr_type ostr;
			std::string const idxfilename;
			uint64_t const h;
			uint64_t const s;

			::libmaus::autoarray::AutoArray < BlockBuffer::unique_ptr_type > B;			
			::libmaus::aio::SynchronousOutputBuffer8::unique_ptr_type idx;

			BlockSynchronousOutputBuffer8(
				std::string const & rfilename, 
				uint64_t const rh,  /* number of buffers */
				uint64_t const rs /* size of single buffer */
			)
			: filename(rfilename), ostr( new ostr_type ( filename.c_str(),std::ios::binary ) ), 
			  idxfilename(filename+".idx"), h(rh), s(rs), B(h), idx(new ::libmaus::aio::SynchronousOutputBuffer8(idxfilename,1024))
			{
				for ( uint64_t i = 0; i < h; ++i )
					B[i] = UNIQUE_PTR_MOVE(BlockBuffer::unique_ptr_type ( new BlockBuffer(s) ));
			}

			// write buffer for value hash
			void writeBuffer(uint64_t const hash)
			{
				// write hash
			        idx->put(hash);
			        uint64_t const numwords = B[hash]->getFill();
			        // write number of words in buffer
			        idx->put(numwords);

			        // write buffer
			        B[hash]->writeOut(*ostr);
			}
			
			void flush()
			{
			        for ( uint64_t i = 0; i < h; ++i )
			        {
			                // std::cerr << "(" << i;
			                if ( B[i]->getFill() )
			                        writeBuffer(i);
                                        B[i].reset();
                                        // std::cerr << ")";
                                }

                                idx->flush();
                                idx.reset();
                                ostr->flush();
                                ostr->close();
                                ostr.reset();
			}
			
			std::map<uint64_t,std::vector<uint64_t> > extract()
			{
			        ::libmaus::aio::SynchronousGenericInput<uint64_t> idxin ( idxfilename, 16 );
			        ::libmaus::aio::SynchronousGenericInput<uint64_t> filein ( filename, 1024 );

                                bool ok = true;
                                uint64_t hv;
                                ok = ok && idxin.getNext(hv);			
                                uint64_t wv;
                                ok = ok && idxin.getNext(wv);
                                
                                std::map<uint64_t,std::vector<uint64_t> > M;
                                while ( ok )
                                {
                                        // std::cerr << "Reading block of size " << wv << " for " << hv << std::endl;
                                
                                        for ( uint64_t i = 0; i < wv; ++i )
                                        {
                                                uint64_t vv;
                                                bool vok = filein.getNext(vv);
                                                assert ( vok );
                                                M [ hv ] . push_back(vv);
                                        }
                                
                                        ok = ok && idxin.getNext(hv);			
                                        ok = ok && idxin.getNext(wv);
                                }
                                
                                return M;
			}
			
			// distribute contents to multiple files
			std::vector < std::string > distribute()
			{
			        std::vector < std::string > filenames;
			        
			        for ( uint64_t i = 0; i < h; ++i )
			        {
			                std::ostringstream fnostr;
			                fnostr << filename << "." << std::setw(6) << std::setfill('0') << i;
			                filenames.push_back(fnostr.str());

                                        remove ( filenames[i].c_str() );
                                        
                                        /*
			                std::ofstream out(filenames[i].c_str(), std::ios::binary);
			                assert ( out );
			                out.flush();
			                out.close();
			                */
                                }

                                #if 0
			        uint64_t const fsize = ::libmaus::util::GetFileSize::getFileSize(filename);
			        uint64_t lastval = 0;
			        uint64_t const fracscale = 5000;
			        #endif
			        uint64_t proc = 0;
                                std::ifstream istr(filename.c_str(), std::ios::binary);
			        ::libmaus::aio::SynchronousGenericInput<uint64_t> idxin ( idxfilename, 16 );
			        
			        bool ok = true;
			        
			        autoarray::AutoArray<uint64_t> buf(16*1024,false);
			        
			        while ( ok )
			        {
			                uint64_t hv;
                			ok = ok && idxin.getNext(hv);
                			uint64_t wv;
	                		ok = ok && idxin.getNext(wv);			        

                                        if ( ok )
                                        {
                                                std::ofstream out(filenames[hv].c_str(), std::ios::binary | std::ios::app );
                                                
                                                while ( wv )
                                                {
                                                        uint64_t const toread = std::min(wv,buf.getN());
                                                        istr.read ( reinterpret_cast<char *>(buf.get()), toread*sizeof(uint64_t));
                                                        assert ( istr );
                                                        assert ( istr.gcount() == static_cast<int64_t>(toread*sizeof(uint64_t)) );
                                                        out.write ( reinterpret_cast<char const *>(buf.get()), toread*sizeof(uint64_t));
                                                        assert ( out );
                                                        wv -= toread;
                                                        
                                                        proc += toread*sizeof(uint64_t);
                                                }
                                                
                                                out.flush();
                                                out.close();
                                                
                                                /*
                                                uint64_t const newfrac = (fracscale*proc)/fsize;
                                                
                                                if ( newfrac != lastval )
                                                {
                                                        std::cerr << "(" << 
                                                                static_cast<double>(proc)/static_cast<double>(fsize)
                                                                << ")";
                                                        lastval = newfrac;
                                                }
                                                */
                                        }
			        }
			        
			        remove ( filename.c_str() );
			        remove ( idxfilename.c_str() );

			        for ( uint64_t i = 0; i < h; ++i )
			        {
			                std::ifstream istr(filenames[i].c_str(),std::ios::binary);
			                
			                if ( ! istr.is_open() )
                                                filenames[i] = std::string();                                        
                                }
			        
			        return filenames;
			}

			void presort(uint64_t const maxsortmem)
			{
			        ::libmaus::aio::SynchronousGenericInput<uint64_t> idxin ( idxfilename, 16 );
			        
			        assert ( maxsortmem >= (h+1)*sizeof(uint64_t) );
			        
			        // number of blocks we can load into memory at one time
			        uint64_t const sortblocks = 
			                std::max ( static_cast<uint64_t>(1),
        			                static_cast<uint64_t>(
        			                        (maxsortmem - (h+1)*sizeof(uint64_t))
        	        		                / 
	        	        	                ( (s + 2) * sizeof(uint64_t) )
                                                )
                                        )
                                        ;

				// sortblocks blocks in internal memory
			        ::libmaus::autoarray::AutoArray < uint64_t > D( sortblocks * s, false );
			        // offsets into D for blocks
                                ::libmaus::autoarray::AutoArray < uint64_t > H(h+1,false);
			        ::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > BD( sortblocks, false );
			        
			        std::fstream file(filename.c_str(), std::ios::in | std::ios::out | std::ios::binary );
			        
        			::libmaus::aio::SynchronousOutputBuffer8::unique_ptr_type idxcomp(
        			        new ::libmaus::aio::SynchronousOutputBuffer8(filename + ".idxcmp",1024));
			        
			        bool ok = true;
			        
			        #if 0
			        uint64_t lastval = 0;
			        uint64_t fracscale = 5000;
			        #endif
			        
			        while ( ok )
			        {
			                uint64_t b = 0;
			                // erase H
			                std::fill ( H.get(), H.get() + H.getN() , 0ull );
			                
			                while ( ok && b < sortblocks )
			                {
			                        uint64_t hv;
                			        ok = ok && idxin.getNext(hv);
                			        uint64_t wv;
	                		        ok = ok && idxin.getNext(wv);			        

                                                if ( ok )
                                                {
                                                        assert ( hv < h );
                                                        H [ hv ] += wv;
                                                        BD [ b ] = std::pair<uint64_t,uint64_t>(hv,wv);
                                                        b += 1;                                                        
                                                }            
			                }
			                
			                // std::cerr << "Sorting " << b << " blocks." << std::endl;
			                
			                if ( b )
			                {
			                        {
			                                uint64_t c = 0;
			                                for ( uint64_t i = 0; i < H.getN(); ++i )
			                                {
			                                        uint64_t const t = H[i];
			                                        H[i] = c;
			                                        c += t;
			                                }
                                                }
                                                
                                                for ( uint64_t i = 0; i < h; ++i )
                                                        if ( H[i+1]-H[i] )
                                                        {
                                                                idxcomp->put(i);
                                                                idxcomp->put(H[i+1]-H[i]);
                                                                // std::cerr << "Writing " << (H[i+1]-H[i]) << " words for " << i << std::endl;
                                                        }
                                                
                                                int64_t rew = 0;
                                                for ( uint64_t i = 0; i < b; ++i )
                                                {
                                                        uint64_t const hv = BD[i].first;
                                                        uint64_t const wv = BD[i].second;
                                        
                                                        // std::cerr << "Reading " << wv << " words for " << hv << std::endl;
                                                                                
                                                        file.read ( reinterpret_cast<char *>(D.get() + H[hv]), wv*sizeof(uint64_t) );
                                                        assert ( file.gcount() == static_cast<int64_t>(wv*sizeof(uint64_t)) );
                                                        assert ( file );
                                                        rew += file.gcount();
                                                        H[hv] += wv;
                                                }
                                                
                                                file.seekp ( file.tellp(), std::ios::beg );
                                                file.seekp ( -rew, std::ios::cur);
                                                
                                                file.write(reinterpret_cast<char const *>(D.get()), H[h]*sizeof(uint64_t));
                                                
                                                file.seekg ( file.tellg(), std::ios::beg );
                                                
                                                /*
                                                uint64_t const pos = file.tellg();
                                                uint64_t const newfrac = (pos*fracscale)/fsize;
                                                
                                                if ( newfrac != lastval )
                                                {
                                                        std::cerr << "(" << static_cast<double>(pos)/static_cast<double>(fsize) << ")";
                                                        lastval = newfrac;
                                                }
                                                */
			                }
			        }

                                // std::cerr << "(f";			        
			        file.flush();
			        file.close();
			        // std::cerr << ")";

                                std::string const idxcompfilename = idxcomp->filename;			        
			        idxcomp->flush();
			        idxcomp.reset();
			        
			        remove ( idxfilename.c_str() );
			        rename ( idxcompfilename.c_str(), idxfilename.c_str() );
			}

			void put(uint64_t const hash, uint64_t const data)
			{
				if ( B[hash]->put(data) )
				        writeBuffer(hash);
			}
		};
	}
}
#endif
