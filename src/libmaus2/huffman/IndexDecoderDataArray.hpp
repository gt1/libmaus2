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

#if ! defined(INDEXDECODERDATAARRAY_HPP)
#define INDEXDECODERDATAARRAY_HPP

#include <libmaus2/huffman/IndexLoaderBase.hpp>

#include <libmaus2/bitio/readElias.hpp>
#include <libmaus2/util/ReverseByteOrder.hpp>
#include <libmaus2/huffman/IndexEntry.hpp>
#include <libmaus2/util/iterator.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/bitio/BitIOInput.hpp>
#include <iostream>

#if defined(__linux__)
#include <byteswap.h>
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/endian.h>
#endif

namespace libmaus2
{
	namespace huffman
	{
		template<typename _owner_type>
		struct GetPosAdapter
		{
			typedef _owner_type owner_type;
			typedef GetPosAdapter<owner_type> this_type;

			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef uint64_t value_type;
			typedef libmaus2::aio::InputStreamInstance input_stream_type;
			typedef ::libmaus2::util::shared_ptr< input_stream_type >::type input_stream_pointer_type;

			owner_type const * owner;
			input_stream_pointer_type indexistr;

			GetPosAdapter(owner_type const * rowner, input_stream_pointer_type const & rindexistr) : owner(rowner), indexistr(rindexistr) {}
			value_type get(uint64_t const i) const { return owner->getPos(*indexistr,i); }
		};

		template<typename _owner_type>
		struct GetKeyCntAdapter
		{
			typedef _owner_type owner_type;
			typedef GetKeyCntAdapter<owner_type> this_type;

			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef uint64_t value_type;
			typedef ::libmaus2::aio::InputStreamInstance input_stream_type;
			typedef ::libmaus2::util::shared_ptr< input_stream_type >::type input_stream_pointer_type;

			owner_type const * owner;
			input_stream_pointer_type indexistr;

			GetKeyCntAdapter(owner_type const * rowner, input_stream_pointer_type const & rindexistr) : owner(rowner), indexistr(rindexistr) {}
			value_type get(uint64_t const i) const { return owner->getKeyCnt(*indexistr,i); }
		};

		template<typename _owner_type>
		struct GetValueCntAdapter
		{
			typedef _owner_type owner_type;
			typedef GetValueCntAdapter<owner_type> this_type;

			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef uint64_t value_type;
			typedef ::libmaus2::aio::InputStreamInstance input_stream_type;
			typedef ::libmaus2::util::shared_ptr< input_stream_type >::type input_stream_pointer_type;

			owner_type const * owner;
			input_stream_pointer_type indexistr;

			GetValueCntAdapter(owner_type const * rowner, input_stream_pointer_type const & rindexistr) : owner(rowner), indexistr(rindexistr) {}
			value_type get(uint64_t const i) const { return owner->getValueCnt(*indexistr,i); }
		};

		template<typename _owner_type>
		struct GetKeyValueCntAdapter
		{
			typedef _owner_type owner_type;
			typedef GetKeyValueCntAdapter<owner_type> this_type;

			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef uint64_t value_type;
			typedef ::libmaus2::aio::InputStreamInstance input_stream_type;
			typedef ::libmaus2::util::shared_ptr< input_stream_type >::type input_stream_pointer_type;

			owner_type const * owner;
			input_stream_pointer_type indexistr;

			GetKeyValueCntAdapter(owner_type const * rowner, input_stream_pointer_type const & rindexistr) : owner(rowner), indexistr(rindexistr) {}
			value_type get(uint64_t const i) const { return owner->getKeyValueCnt(*indexistr,i); }
		};

		struct IndexDecoderData : public IndexLoaderBase
		{
			typedef IndexDecoderData this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef ::libmaus2::util::ConstIteratorSharedPointer< GetPosAdapter<this_type>, uint64_t > const_pos_iterator;
			typedef ::libmaus2::util::ConstIteratorSharedPointer< GetKeyCntAdapter<this_type>, uint64_t > const_kcnt_iterator;
			typedef ::libmaus2::util::ConstIteratorSharedPointer< GetValueCntAdapter<this_type>, uint64_t > const_vcnt_iterator;
			typedef ::libmaus2::util::ConstIteratorSharedPointer< GetKeyValueCntAdapter<this_type>, uint64_t > const_kvcnt_iterator;

			typedef ::libmaus2::aio::InputStreamInstance input_stream_type;
			typedef ::libmaus2::util::shared_ptr< input_stream_type >::type input_stream_pointer_type;

			std::string filename;
			uint64_t numentries;
			unsigned int posbits;
			unsigned int kbits;
			uint64_t kacc;
			unsigned int vbits;
			uint64_t vacc;
			uint64_t indexvectorpos;

			GetPosAdapter<this_type>::shared_ptr_type getPosAdapter() const
			{
				input_stream_pointer_type file = openFile();
				GetPosAdapter<this_type>::shared_ptr_type adptr(new GetPosAdapter<this_type>(this,file));
				return adptr;
			}

			GetKeyCntAdapter<this_type>::shared_ptr_type getKeyCntAdapter() const
			{
				input_stream_pointer_type file = openFile();
				GetKeyCntAdapter<this_type>::shared_ptr_type adptr(new GetKeyCntAdapter<this_type>(this,file));
				return adptr;
			}

			GetValueCntAdapter<this_type>::shared_ptr_type getValueCntAdapter() const
			{
				input_stream_pointer_type file = openFile();
				GetValueCntAdapter<this_type>::shared_ptr_type adptr(new GetValueCntAdapter<this_type>(this,file));
				return adptr;
			}

			GetKeyValueCntAdapter<this_type>::shared_ptr_type getKeyValueCntAdapter() const
			{
				input_stream_pointer_type file = openFile();
				GetKeyValueCntAdapter<this_type>::shared_ptr_type adptr(new GetKeyValueCntAdapter<this_type>(this,file));
				return adptr;
			}

			const_pos_iterator pbegin() const { return const_pos_iterator(getPosAdapter()); }
			const_pos_iterator pend(const_pos_iterator const & begin)   const { return begin + numentries + 1; }
			const_kcnt_iterator kbegin() const { return const_kcnt_iterator(getKeyCntAdapter()); }
			const_kcnt_iterator kend(const_kcnt_iterator const & begin)   const { return begin + numentries + 1; }
			const_vcnt_iterator vbegin() const { return const_vcnt_iterator(getValueCntAdapter()); }
			const_vcnt_iterator vend(const_vcnt_iterator const & begin)   const { return begin + numentries + 1; }
			const_kvcnt_iterator kvbegin() const { return const_kvcnt_iterator(getKeyValueCntAdapter()); }
			const_kvcnt_iterator kvend(const_kvcnt_iterator const & begin)   const { return begin + numentries + 1; }

			static uint64_t getKAcc(std::string const & filename)
			{
				IndexDecoderData IDD(filename);
				return IDD.kacc;
			}

			static uint64_t getVAcc(std::string const & filename)
			{
				IndexDecoderData IDD(filename);
				return IDD.vacc;
			}

			static uint64_t getKVAcc(std::string const & filename)
			{
				IndexDecoderData IDD(filename);
				return IDD.kacc + IDD.vacc;
			}

			uint64_t getEntryBitPos(uint64_t const entry) const
			{
				return (indexvectorpos<<3) + entry*(posbits+kbits+vbits);
			}

			uint64_t getPos(uint64_t const entryid) const
			{
				return readEntry(entryid).pos;
			}

			uint64_t getKeyCnt(uint64_t const entryid) const
			{
				return readEntry(entryid).kcnt;
			}

			uint64_t getValueCnt(uint64_t const entryid) const
			{
				return readEntry(entryid).vcnt;
			}

			uint64_t getKeyValueCnt(uint64_t const entryid) const
			{
				IndexEntry const entry = readEntry(entryid);
				return entry.kcnt + entry.vcnt;
			}

			uint64_t getPos(std::istream & istr, uint64_t const entryid) const
			{
				return readEntry(istr,entryid).pos;
			}

			uint64_t getKeyCnt(std::istream & istr, uint64_t const entryid) const
			{
				return readEntry(istr,entryid).kcnt;
			}

			uint64_t getValueCnt(std::istream & istr, uint64_t const entryid) const
			{
				return readEntry(istr,entryid).vcnt;
			}
			uint64_t getKeyValueCnt(std::istream & istr, uint64_t const entryid) const
			{
				IndexEntry const entry = readEntry(istr,entryid);
				return entry.kcnt + entry.vcnt;
			}

			IndexEntry readEntry(std::istream & indexistr, uint64_t const entryid) const
			{
				uint64_t const entrybitpos = getEntryBitPos(entryid);
				uint64_t const entrybytepos = entrybitpos>>3;
				uint64_t const entrybitoff = entrybitpos - (entrybytepos<<3);

				// seek to index position
				indexistr.clear();
				indexistr.seekg(entrybytepos,std::ios::beg);
				if ( static_cast<int64_t>(indexistr.tellg()) != static_cast<int64_t>(entrybytepos) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to seek to index position " << entrybytepos << " in file " << filename << " of size "
						<< ::libmaus2::util::GetFileSize::getFileSize(filename) << std::endl;
					se.finish();
					throw se;
				}
				::libmaus2::bitio::StreamBitInputStream SBIS(indexistr);

				SBIS.read(entrybitoff);

				uint64_t const pos = SBIS.read(posbits);
				uint64_t const kcnt = SBIS.read(kbits);
				uint64_t const vcnt = SBIS.read(vbits);

				return IndexEntry(pos,kcnt,vcnt);
			}

			input_stream_pointer_type openFile() const
			{
				input_stream_pointer_type indexistr(new input_stream_type(filename));
				return indexistr;
			}

			IndexEntry readEntry(uint64_t const entryid) const
			{
				input_stream_pointer_type indexistr = openFile();
				return readEntry(*indexistr,entryid);
			}

			IndexDecoderData & operator=(IndexDecoderData const & o)
			{
				if ( this != &o )
				{
					filename = o.filename;
					numentries = o.numentries;
					posbits = o.posbits;
					kbits = o.kbits;
					kacc = o.kacc;
					vbits = o.vbits;
					vacc = o.vacc;
					indexvectorpos = o.indexvectorpos;
				}
				return *this;
			}

			IndexDecoderData()
			: numentries(0), posbits(0), kbits(0), kacc(0), vbits(0), vacc(0), indexvectorpos(0)
			  // , posadpt(this), kadpt(this), vadpt(this)
			{

			}

			IndexDecoderData(IndexDecoderData const & o)
			: filename(o.filename),
			  numentries(o.numentries),
			  posbits(o.posbits),
			  kbits(o.kbits),
			  kacc(o.kacc),
			  vbits(o.vbits),
			  vacc(o.vacc),
			  indexvectorpos(o.indexvectorpos)
			  // , posadpt(this), kadpt(this), vadpt(this)
			{

			}

			IndexDecoderData(std::string const & rfilename)
			: filename(rfilename),
			  numentries(0), posbits(0), kbits(0), kacc(0), vbits(0), vacc(0), indexvectorpos(0)
			  // , posadpt(this), kadpt(this), vadpt(this)
			{
				uint64_t const indexpos = getIndexPos(filename);

				libmaus2::aio::InputStreamInstance indexistr(filename);

				// seek to index position
				indexistr.clear();
				indexistr.seekg(indexpos,std::ios::beg);
				if ( static_cast<int64_t>(indexistr.tellg()) != static_cast<int64_t>(indexpos) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to seek to position " << indexpos << " of index in file " << filename << " of size "
						<< ::libmaus2::util::GetFileSize::getFileSize(filename) << std::endl;
					se.finish();
					throw se;
				}
				::libmaus2::bitio::StreamBitInputStream SBIS(indexistr);

				// read size of index
				numentries = ::libmaus2::bitio::readElias2(SBIS);
				// pos bits
				posbits = ::libmaus2::bitio::readElias2(SBIS);

				// k bits
				kbits = ::libmaus2::bitio::readElias2(SBIS);
				// k acc
				kacc = ::libmaus2::bitio::readElias2(SBIS);

				// v bits
				vbits = ::libmaus2::bitio::readElias2(SBIS);
				// v acc
				vacc = ::libmaus2::bitio::readElias2(SBIS);

				// align
				SBIS.flush();

				assert ( SBIS.getBitsRead() % 8 == 0 );

				indexvectorpos = indexpos + SBIS.getBitsRead() / 8;
			}
		};

		struct FileBlockOffset
		{
			uint64_t fileptr;
			uint64_t blockptr;
			uint64_t offset;

			FileBlockOffset()
			: fileptr(0), blockptr(0), offset(0) {}
			FileBlockOffset(uint64_t const rfileptr, uint64_t const rblockptr, uint64_t const roffset)
			: fileptr(rfileptr), blockptr(rblockptr), offset(roffset) {}
		};

		struct KvAdapter
		{
			typedef KvAdapter this_type;
			typedef ::libmaus2::util::ConstIterator<this_type,uint64_t> const_iterator;

			::libmaus2::autoarray::AutoArray<uint64_t> const & kvec;
			::libmaus2::autoarray::AutoArray<uint64_t> const & vvec;

			KvAdapter(
				::libmaus2::autoarray::AutoArray<uint64_t> const & rkvec,
				::libmaus2::autoarray::AutoArray<uint64_t> const & rvvec
			) : kvec(rkvec), vvec(rvvec)
			{

			}

			uint64_t get(uint64_t const i) const
			{
				return kvec[i] + vvec[i];
			}
			uint64_t operator[](uint64_t const i) const
			{
				return get(i);
			}
			uint64_t size() const
			{
				return kvec.size();
			}

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}
			const_iterator end() const
			{
				return const_iterator(this,size());
			}
		};

		struct IndexDecoderDataArray
		{
			typedef IndexDecoderDataArray this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			::libmaus2::autoarray::AutoArray<IndexDecoderData> data;
			::libmaus2::autoarray::AutoArray<uint64_t> kvec;
			::libmaus2::autoarray::AutoArray<uint64_t> vvec;

			uint64_t getVSum() const
			{
				if ( vvec.size() )
					return vvec[vvec.size()-1];
				else
					return 0;
			}

			FileBlockOffset findKBlock(uint64_t const offset) const
			{
				uint64_t const * p = std::lower_bound(kvec.begin(),kvec.end(),offset);

				// beyond end
				if ( p == kvec.end() )
					return FileBlockOffset(data.size(),0,0);

				uint64_t const fileptr = (offset == *p) ? (p-kvec.begin()) : ((p-kvec.begin())-1);

				// beyond end (pointing at the symbol right beyound the end)
				if ( fileptr >= data.size() )
					return FileBlockOffset(data.size(),0,0);

				assert ( fileptr+1 < kvec.size() );
				assert ( offset >= kvec[fileptr] );
				assert ( offset  < kvec[fileptr+1] );

				IndexDecoderData const & fdata = data[fileptr];
				uint64_t const fileoffset = offset-kvec[fileptr];

				IndexDecoderData::const_kcnt_iterator kbegin = fdata.kbegin();
				IndexDecoderData::const_kcnt_iterator kend = fdata.kend(kbegin);
				IndexDecoderData::const_kcnt_iterator kit =
					::std::lower_bound(kbegin,kend,fileoffset);

				assert ( kit != kend );

				uint64_t const blockptr = (*kit == fileoffset) ? (kit-kbegin) : ((kit-kbegin)-1);

				return FileBlockOffset(fileptr,blockptr,offset-(kvec[fileptr]+fdata.getKeyCnt(blockptr)));
			}

			FileBlockOffset findVBlock(uint64_t const offset) const
			{
				uint64_t const * p = std::lower_bound(vvec.begin(),vvec.end(),offset);

				// beyond end
				if ( p == vvec.end() )
					return FileBlockOffset(data.size(),0,0);

				uint64_t const fileptr = (offset == *p) ? (p-vvec.begin()) : ((p-vvec.begin())-1);

				// beyond end (pointing at the symbol right beyound the end)
				if ( fileptr >= data.size() )
					return FileBlockOffset(data.size(),0,0);

				assert ( fileptr+1 < vvec.size() );
				assert ( offset >= vvec[fileptr] );
				assert ( offset  < vvec[fileptr+1] );

				IndexDecoderData const & fdata = data[fileptr];
				uint64_t const fileoffset = offset-vvec[fileptr];

				IndexDecoderData::const_vcnt_iterator vbegin = fdata.vbegin();
				IndexDecoderData::const_vcnt_iterator vend = fdata.vend(vbegin);
				IndexDecoderData::const_vcnt_iterator vit =
					::std::lower_bound(vbegin,vend,fileoffset);

				assert ( vit != vend );

				uint64_t const blockptr = (*vit == fileoffset) ? (vit-vbegin) : ((vit-vbegin)-1);

				return FileBlockOffset(fileptr,blockptr,offset-(vvec[fileptr]+fdata.getValueCnt(blockptr)));
			}

			FileBlockOffset findKVBlock(uint64_t const offset) const
			{
				KvAdapter kvadapter(kvec,vvec);

				KvAdapter::const_iterator p = std::lower_bound(kvadapter.begin(),kvadapter.end(),offset);

				// beyond end
				if ( p == kvadapter.end() )
					return FileBlockOffset(data.size(),0,0);

				uint64_t const fileptr = (offset == *p) ? (p-kvadapter.begin()) : ((p-kvadapter.begin())-1);

				// beyond end (pointing at the symbol right beyound the end)
				if ( fileptr >= data.size() )
					return FileBlockOffset(data.size(),0,0);

				assert ( fileptr+1 < kvadapter.size() );
				assert ( offset >= kvadapter[fileptr] );
				assert ( offset  < kvadapter[fileptr+1] );

				IndexDecoderData const & fdata = data[fileptr];
				uint64_t const fileoffset = offset-kvadapter[fileptr];

				IndexDecoderData::const_kvcnt_iterator kvbegin = fdata.kvbegin();
				IndexDecoderData::const_kvcnt_iterator kvend = fdata.kvend(kvbegin);
				IndexDecoderData::const_kvcnt_iterator kvit =
					::std::lower_bound(kvbegin,kvend,fileoffset);

				assert ( kvit != kvend );

				uint64_t const blockptr = (*kvit == fileoffset) ? (kvit-kvbegin) : ((kvit-kvbegin)-1);

				return FileBlockOffset(fileptr,blockptr,fileoffset-fdata.getKeyValueCnt(blockptr));
			}

			static unique_ptr_type construct(std::vector<std::string> const & filenames, uint64_t const numthreads)
			{
				unique_ptr_type ptr(new this_type(filenames,numthreads));
				return UNIQUE_PTR_MOVE(ptr);
			}

			IndexDecoderDataArray(std::vector<std::string> const & filenames, uint64_t const numthreads)
			{
				libmaus2::autoarray::AutoArray<uint8_t> Vnonempty(filenames.size(),false);
				std::vector<IndexDecoderData> LVIDD(filenames.size());

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					IndexDecoderData IDD(filenames[i]);

					if ( IDD.numentries && IDD.kacc )
					{
						LVIDD[i] = IDD;
						Vnonempty[i] = true;
					}
					else
					{
						Vnonempty[i] = false;
					}
				}

				uint64_t nonempty = 0;
				for ( uint64_t i = 0; i < Vnonempty.size(); ++i )
					if ( Vnonempty[i] )
						nonempty += 1;

				data = ::libmaus2::autoarray::AutoArray<IndexDecoderData>(nonempty);
				kvec = ::libmaus2::autoarray::AutoArray<uint64_t>(nonempty+1,false);
				vvec = ::libmaus2::autoarray::AutoArray<uint64_t>(nonempty+1,false);

				uint64_t j = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					if ( Vnonempty[i] )
					{
						IndexDecoderData IDD = LVIDD[i];

						data[j] = IDD;
						kvec[j] = IDD.kacc;
						vvec[j] = IDD.vacc;
						j += 1;
					}
				}

				assert ( j == nonempty );

				kvec.prefixSums();
				vvec.prefixSums();
			}
		};
	}
}
#endif
