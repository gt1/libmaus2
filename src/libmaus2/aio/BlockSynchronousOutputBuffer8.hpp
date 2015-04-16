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


#if ! defined(LIBMAUS_AIO_BLOCKSYNCHRONOUSOUTPUTBUFFER8_HPP)
#define LIBMAUS_AIO_BLOCKSYNCHRONOUSOUTPUTBUFFER8_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/SynchronousOutputBuffer8.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <fstream>
#include <vector>
#include <map>
#include <iomanip>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * fixed size buffer with elements of type _data_type
		 **/
		template<typename _data_type>
		struct BlockBufferTemplate
		{
			//! data type
			typedef _data_type data_type;
			//! this type
			typedef BlockBufferTemplate<data_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:	
			::libmaus2::autoarray::AutoArray<data_type> B;
			data_type * const pa;
			data_type * pc;
			data_type * const pe;
			
			public:
			/**
			 * constructor
			 *
			 * @param s buffer size in elements
			 **/
			BlockBufferTemplate(uint64_t const s)
			: B(s), pa(B.get()), pc(pa), pe(pa+s)
			{
			
			}
			
			/**
			 * put a single element
			 *
			 * @param v element to be put
			 * @return true if buffer is full after putting the element
			 **/
			bool put(data_type const v)
			{
				*(pc++) = v;
				return pc == pe;
			}
			
			/**
			 * @return number of elements in buffer
			 **/
			uint64_t getFill() const
			{
				return pc - pa;
			}
			
			/**
			 * @return number of bytes in buffer
			 **/
			uint64_t getFillBytes() const
			{
				return getFill() * sizeof(data_type);
			}
			
			/**
			 * reset buffer
			 **/
			void reset()
			{
				pc = pa; 
			}
		
			/**
			 * write out buffer and reset it
			 *
			 * @param out output stream
			 **/
			void writeOut(std::ostream & out)
			{
				out.write ( reinterpret_cast<char const *>(pa) , getFillBytes() );
				reset();
			}
		
			/**
			 * fill buffer and then write it to out
			 *
			 * @param out output stream
			 **/
			void writeFull(std::ostream & out)
			{
				while ( pc != pe ) 
					put(0);

				assert ( getFill() == B.getN() );
				
				writeOut(out);
			}
		};

		/**
		 * block buffer for uint64_t data type
		 **/
		typedef BlockBufferTemplate<uint64_t> BlockBuffer;
	
		/**
		 * synchronous blockwise output class split by hash values. impelemetation
		 * first writes all files to a single stream and then splits it up upon request
		 **/
		struct BlockSynchronousOutputBuffer8
		{
			//! output stream type
			typedef std::ofstream ostr_type;
			//! output stream pointer type
			typedef ::libmaus2::util::unique_ptr<ostr_type>::type ostr_ptr_type;

			private:
			std::string const filename;
			ostr_ptr_type ostr;
			std::string const idxfilename;
			uint64_t const h;
			uint64_t const s;

			::libmaus2::autoarray::AutoArray < BlockBuffer::unique_ptr_type > B;			
			::libmaus2::aio::SynchronousOutputBuffer8::unique_ptr_type idx;

			/**
			 * write out buffer for value hash
			 *
			 * @param hash buffer id
			 **/
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

			public:
			/**
			 * constructor
			 *
			 * @param rfilename output file name
			 * @param rh number of buffers
			 * @param rs buffer size
			 **/
			BlockSynchronousOutputBuffer8(
				std::string const & rfilename, 
				uint64_t const rh,  /* number of buffers */
				uint64_t const rs /* size of single buffer */
			)
			: filename(rfilename), ostr( new ostr_type ( filename.c_str(),std::ios::binary ) ), 
			  idxfilename(filename+".idx"), h(rh), s(rs), B(h), idx(new ::libmaus2::aio::SynchronousOutputBuffer8(idxfilename,1024))
			{
				for ( uint64_t i = 0; i < h; ++i )
				{
					BlockBuffer::unique_ptr_type tBi( new BlockBuffer(s) );
					B[i] = UNIQUE_PTR_MOVE(tBi);
				}
			}

			/**
			 * put data in buffer hash
			 *
			 * @param hash buffer id
			 * @param data element to be put in stream with id hash
			 **/
			void put(uint64_t const hash, uint64_t const data)
			{
				if ( B[hash]->put(data) )
				        writeBuffer(hash);
			}

			/**
			 * flush buffers for all hash values
			 **/
			void flush()
			{
			        for ( uint64_t i = 0; i < h; ++i )
			        {
			                if ( B[i]->getFill() )
			                        writeBuffer(i);
                                        B[i].reset();
                                }

                                idx->flush();
                                idx.reset();
                                ostr->flush();
                                ostr->close();
                                ostr.reset();
			}
			
			/**
			 * extract written vectors by hash value
			 *
			 * @return map where keys are the hash values and value are 
			 *         vectors of the values written for the respective hash values
			 **/
			std::map<uint64_t,std::vector<uint64_t> > extract()
			{
			        ::libmaus2::aio::SynchronousGenericInput<uint64_t> idxin ( idxfilename, 16 );
			        ::libmaus2::aio::SynchronousGenericInput<uint64_t> filein ( filename, 1024 );

                                bool ok = true;
                                uint64_t hv;
                                ok = ok && idxin.getNext(hv);			
                                uint64_t wv;
                                ok = ok && idxin.getNext(wv);
                                
                                std::map<uint64_t,std::vector<uint64_t> > M;
                                while ( ok )
                                {
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
			
			/**
			 * distribute contents of the single file to multiple files
			 *
			 * @return vector containing the names of the files created for the single hash values.
			 *         if a hash value has not received any elements than the 
			 *         corresponding file name is the empty string
			 **/
			std::vector < std::string > distribute()
			{
			        std::vector < std::string > filenames;
			        
			        for ( uint64_t i = 0; i < h; ++i )
			        {
			                std::ostringstream fnostr;
			                fnostr << filename << "." << std::setw(6) << std::setfill('0') << i;
			                filenames.push_back(fnostr.str());
                                        remove ( filenames[i].c_str() );
                                }

                                #if 0
			        uint64_t const fsize = ::libmaus2::util::GetFileSize::getFileSize(filename);
			        uint64_t lastval = 0;
			        uint64_t const fracscale = 5000;
			        #endif
			        uint64_t proc = 0;
                                std::ifstream istr(filename.c_str(), std::ios::binary);
			        ::libmaus2::aio::SynchronousGenericInput<uint64_t> idxin ( idxfilename, 16 );
			        
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

			/**
			 * presort block file in blocks of size maxsortmem. this only sorts
			 * by hash value, the order of the values in the blocks remains the same
			 *
			 * @param maxsortmem maximum in memory block size (in bytes)
			 **/
			void presort(uint64_t const maxsortmem)
			{
			        ::libmaus2::aio::SynchronousGenericInput<uint64_t> idxin ( idxfilename, 16 );
			        
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
			        ::libmaus2::autoarray::AutoArray < uint64_t > D( sortblocks * s, false );
			        // offsets into D for blocks
                                ::libmaus2::autoarray::AutoArray < uint64_t > H(h+1,false);
			        ::libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > BD( sortblocks, false );
			        
			        std::fstream file(filename.c_str(), std::ios::in | std::ios::out | std::ios::binary );
			        
        			::libmaus2::aio::SynchronousOutputBuffer8::unique_ptr_type idxcomp(
        			        new ::libmaus2::aio::SynchronousOutputBuffer8(filename + ".idxcmp",1024));
			        
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

                                std::string const idxcompfilename = idxcomp->getFilename();
			        idxcomp->flush();
			        idxcomp.reset();
			        
			        remove ( idxfilename.c_str() );
			        rename ( idxcompfilename.c_str(), idxfilename.c_str() );
			}

		};
	}
}
#endif
