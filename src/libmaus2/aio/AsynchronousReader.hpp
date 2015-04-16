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

#if !defined(ASYNCHRONOUSREADER_HPP)
#define ASYNCHRONOUSREADER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <iostream>

#if defined(LIBMAUS_HAVE_PTHREADS)
namespace libmaus2
{
	namespace aio
	{
		/**
		 * asynchronous block reading data block for FastA/FastQ
		 **/
		template<typename reader_type>
                struct AsynchronousStreamReaderData
                {
                	//! type of pattern stored in this data set
                        typedef typename reader_type::pattern_type pattern_type;
                        //! type of blocks stored in this object
                        typedef typename reader_type::block_type data_type;

                        private:
                        //! input stream
                        reader_type & file;
                        
                        //! size of single block (in patterns)
                        unsigned int const blocksize;
                        //! number of blocks
                        unsigned int const numblocks;

                        //! patterns
                        ::libmaus2::autoarray::AutoArray < pattern_type > blocks;
                        //! pointers/offsets into blocks array demarking blocks
                        ::libmaus2::autoarray::AutoArray < data_type > blockp;

                        public:
                        /**
                         * constructor
                         *
                         * @param rfile FastA/FastQ input file
                         * @param rblocksize size of each block in patterns
                         * @param rnumblocks number of blocks
                         **/
                        AsynchronousStreamReaderData(
                                reader_type & rfile , 
                                unsigned int const rblocksize, 
                                unsigned int const rnumblocks 
                        )
                        : file(rfile), blocksize(rblocksize), numblocks(rnumblocks), blocks(blocksize*numblocks), blockp(numblocks)
                        {
                                for ( unsigned int i = 0; i < numblocks; ++i )
                                {
                                        blockp[i].patterns = blocks.get() + i * blocksize;
                                        blockp[i].blocksize = 0;
                                        blockp[i].blockid = i;
                                }
                        }
                       
                       	/**
                       	 * @return number of blocks in this object
                       	 **/ 
                        unsigned int getNumEntities()
                        {
                                return numblocks;
                        }
                        
                        /**
                         * @param id block id
                         * @return data for block id
                         **/
                        data_type * getData(unsigned int const id)
                        {
                                return &blockp[id];
                        }
                        
                        /**
                         * map a data block to its id
                         *
                         * @param data block
                         * @return id of data block
                         **/
                        unsigned int dataToId(data_type const * data)
                        {
                               return data->blockid; 
                        }
                        /**
                         * generate data (fill block blockid)
                         *
                         * @param blockid id of block to be filled
                         * @return true if any data was deposited in the block
                         **/
                        bool generateData(unsigned int const blockid)
                        {
                                blockp[blockid].blocksize = file.fillPatternBlock(blockp[blockid].patterns,blocksize);        
                                return blockp[blockid].blocksize != 0;
                        }
                };

		/**
		 * asynchronous block reading data block for FastA/FastQ (id lines only)
		 **/
                template<typename reader_type>
                struct AsynchronousIdData
                {
                        typedef reader_type file_type;
                        typedef typename reader_type::idblock_type data_type;

                        private:
                        file_type & file;
                        
                        unsigned int const numblocks;
                        unsigned int const blocksize;

                        ::libmaus2::autoarray::AutoArray < data_type > blockp;

                        public:
                        /**
                         * constructor
                         *
                         * @param rfile FastA/FastQ input file
                         * @param rblocksize size of each block in patterns
                         * @param rnumblocks number of blocks
                         **/
                        AsynchronousIdData(file_type & rfile , unsigned int const rblocksize, unsigned int const rnumblocks)
                        : file(rfile), numblocks(rnumblocks), blocksize(rblocksize), blockp(numblocks)
                        {
                                for ( unsigned int i = 0; i < numblocks; ++i )
                                        blockp[i].blockid = i;
                        }
                        
                       	/**
                       	 * @return number of blocks in this object
                       	 **/ 
                        unsigned int getNumEntities()
                        {
                                return numblocks;
                        }
                        
                        /**
                         * @param id block id
                         * @return data for block id
                         **/
                        data_type * getData(unsigned int const id)
                        {
                                return &blockp[id];
                        }

                        /**
                         * map a data block to its id
                         *
                         * @param data block
                         * @return id of data block
                         **/
                        unsigned int dataToId(data_type const * data)
                        {
                               return data->blockid; 
                        }

                        /**
                         * generate data (fill block blockid)
                         *
                         * @param blockid id of block to be filled
                         * @return true if any data was deposited in the block
                         **/
                        bool generateData(unsigned int const blockid)
                        {
                                blockp[blockid].blocksize = file.fillIdBlock(blockp[blockid],blocksize);
                                return blockp[blockid].blocksize != 0;
                        }
                };

                /**
                 * class for asynchronous block wise reading of FastA/FastQ data
                 **/
                template<typename data_type>
                struct AsynchronousStreamReader
                {
                	private:
                        data_type & data;        
                        
			::libmaus2::parallel::SynchronousQueue<unsigned int> unfilled;
                        ::libmaus2::parallel::TerminatableSynchronousQueue<unsigned int> filled;
                        pthread_t thread;

                        void * readerThread()
                        {
                                while ( !filled.isTerminated() )
                                {
                                        unsigned int const blockid = unfilled.deque();

                                        bool const generated = data.generateData(blockid);
                                        
                                        if ( ! generated )
                                                filled.terminate();
                                        else
                                                filled.enque(blockid);
                                }

                                return 0;
                        }
                        
                        static void * dispatcher ( void * object )
                        {
                                AsynchronousStreamReader * reader = reinterpret_cast < AsynchronousStreamReader * > (object);
                                pthread_exit ( reader->readerThread() );
                                return 0;
                        }

                        public:
                        /**
                         * get next block
                         *
                         * @return next block (returns null pointer if there are no more blocks)
                         **/
                        typename data_type::data_type * getBlock()
                        {
                                unsigned int blockid;
                                try
                                {
                                        blockid = filled.deque();
                                }
                                catch(...)
                                {
                                        return 0;
                                }

                                return data.getData(blockid);
                        }
                        
                        /**
                         * return block
                         *
                         * @param block pointer to block
                         **/
                        void returnBlock(typename data_type::data_type * block)
                        {
                                unfilled.enque(data.dataToId(block));
                        }
                        
                        /**
                         * @return number of blocks currently available without blocking
                         **/
                        unsigned int getFillState()
                        {
                                return filled.getFillState();
                        }

                        /**
                         * constructor
                         *
                         * @param rdata data object (AsynchronousStreamReaderData or AsynchronousIdData)
                         **/
                        AsynchronousStreamReader ( data_type & rdata ) : data(rdata)
                        {
                                for ( unsigned int i = 0; i < data.getNumEntities(); ++i )
                                        unfilled.enque(i);
                                
                                if ( pthread_create ( & thread, 0, dispatcher, this ) )
                                {
                                	libmaus2::exception::LibMausException se;
                                        se.getStream() << "Cannot create reader thread.";
                                        se.finish();
                                        throw se;
                                }
                        }
                        /**
                         * destructor
                         **/
                        ~AsynchronousStreamReader()
                        {
                                void * p = 0;
                                pthread_join ( thread, & p );
                        }
                };
	}
}
#endif

#endif
