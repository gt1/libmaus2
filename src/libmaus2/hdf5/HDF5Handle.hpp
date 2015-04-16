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

#if ! defined(LIBMAUS_HDF5_HDF5HANDLE_HPP)
#define LIBMAUS_HDF5_HDF5HANDLE_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_HDF5)
#include <hdf5.h>
#include <hdf5_hl.h>
#include <libmaus/hdf5/HDF5MuteErrorHandler.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace hdf5
	{
		struct HDF5Handle
		{
			typedef HDF5Handle this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			enum handle_type_enum { 
				hdf5_handle_type_none,
				hdf5_handle_type_file, 
				hdf5_handle_type_group,
				hdf5_handle_type_datatype,
				hdf5_handle_type_object,
				hdf5_handle_type_attribute,
				hdf5_handle_type_dataspace,
				hdf5_handle_type_dataset
			};
			hid_t handle;
			handle_type_enum handle_type;
			
			HDF5Handle()
			: handle_type(hdf5_handle_type_none)
			{
			
			}
			
			HDF5Handle(hid_t const rhandle, handle_type_enum rhandle_type)
			: handle(rhandle), handle_type(rhandle_type)
			{
			
			}
			
			public:
			~HDF5Handle()
			{
				HDF5MuteErrorHandler mute;
			
				herr_t status = 0;
				
				switch ( handle_type )
				{
					case hdf5_handle_type_none:
						break;
					case hdf5_handle_type_file:
						status = H5Fclose(handle);
						break;
					case hdf5_handle_type_group:
						status = H5Gclose(handle);
						break;
					case hdf5_handle_type_datatype:
						status = H5Tclose(handle);
						break;
					case hdf5_handle_type_object:
						status = H5Oclose(handle);
						break;
					case hdf5_handle_type_attribute:
						status = H5Aclose(handle);
						break;
					case hdf5_handle_type_dataspace:
						status = H5Sclose(handle);
						break;
					case hdf5_handle_type_dataset:
						status = H5Dclose(handle);
						break;
				}
				
				if ( status < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::~HDF5Handle: Failed to close HDF5 handle" << std::endl;
					lme.finish();
					throw lme;
				}
			}
			
			static shared_ptr_type createFileHandle(char const * filename)
			{
				HDF5MuteErrorHandler mute;
				
				HDF5Handle::shared_ptr_type shandle(new HDF5Handle);
				
				shandle->handle = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
				
				if ( shandle->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::createFileHandle: failed to open handle for " << filename << std::endl;
					lme.finish();
					throw lme;
				}
				
				shandle->handle_type = hdf5_handle_type_file;
				
				return shandle;
			}
			
			static shared_ptr_type createMemoryHandle(char * data, size_t const len)
			{
				HDF5MuteErrorHandler mute;
				
				HDF5Handle::shared_ptr_type shandle(new HDF5Handle);

				shandle->handle = H5LTopen_file_image(data,len,0);

				if ( shandle->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::createMemoryHandle: failed to open handle" << std::endl;
					lme.finish();
					throw lme;
				}
				
				shandle->handle_type = hdf5_handle_type_file;
				
				return shandle;
			}

			shared_ptr_type getRootGroup() const
			{
				HDF5MuteErrorHandler mute;
				
				HDF5Handle::shared_ptr_type shandle(new HDF5Handle);

				shandle->handle = H5Gopen(handle, "/",H5P_DEFAULT);
        
				if ( shandle->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::getRootGroup: failed" << std::endl;
					lme.finish();
					throw lme;
				}
				
				shandle->handle_type = hdf5_handle_type_group;
				
				return shandle;
			}

			bool hasGroup(char const * name) const
			{
				HDF5MuteErrorHandler mute;
				
				hid_t group = H5Gopen(handle,name,H5P_DEFAULT);
		
				if ( group < 0 )
				{
					return false;
				}
				else
				{
					H5Gclose(group);
					return true;
				}
			}

			shared_ptr_type getGroup(char const * name) const
			{
				HDF5MuteErrorHandler mute;
				
				HDF5Handle::shared_ptr_type shandle(new HDF5Handle);

				shandle->handle = H5Gopen(handle, name,H5P_DEFAULT);
        
				if ( shandle->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::getGroup: failed for " << name << std::endl;
					lme.finish();
					throw lme;
				}
				
				shandle->handle_type = hdf5_handle_type_group;
				
				return shandle;
			}

			shared_ptr_type getObject() const
			{
				HDF5MuteErrorHandler mute;
				
				HDF5Handle::shared_ptr_type shandle(new HDF5Handle);

				shandle->handle = H5Oopen(handle,".",H5P_DEFAULT);
				
				if ( shandle->handle < 0 )
				{				
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::getObject: failed for." << std::endl;
					lme.finish();
					throw lme;
				}
				
				shandle->handle_type = hdf5_handle_type_object;
				
				return shandle;
			}
			
			H5O_type_t getType() const
			{
				HDF5MuteErrorHandler mute;
				H5O_info_t infobuf;
				herr_t status = H5Oget_info_by_name (handle, ".", &infobuf, H5P_DEFAULT);
				
				if ( status < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::getType: failed" << std::endl;
					lme.finish();
					throw lme;
				}
				
				return infobuf.type;
			}
			
			std::string getName() const
			{
				shared_ptr_type obj = getObject();
				ssize_t len = H5Iget_name(obj->handle,0,0);
				libmaus::autoarray::AutoArray<char> A(len+1,false);
				A[len] = 0;
				H5Iget_name(obj->handle,A.begin(),A.size());
				return std::string(A.begin(),A.begin()+len);
			}

			shared_ptr_type getPath(char const * path) const
			{
				shared_ptr_type cgroup;
				
				if ( *path && *path == '/' )
				{
					cgroup = getGroup("/");
					++path;
				}
				else
				{
					cgroup = getGroup(".");
				}
			
				while ( *path != 0 )
				{
					char const * pathe = path;
					while ( *pathe && *pathe != '/' )
						++pathe;

					std::string subpath(path,pathe);

					if ( cgroup->getType() != H5O_TYPE_GROUP )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HDF5Handle::getPath: path element" << subpath << " originates from a non group" << std::endl;
						lme.finish();
						throw lme;
					}

					std::pair<bool,H5O_type_t> childpresent = cgroup->hasChild(subpath);
					
					if ( ! childpresent.first )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HDF5Handle::getPath: path element" << subpath << " does not exist" << std::endl;
						lme.finish();
						throw lme;
					}
					
					switch ( childpresent.second )
					{
						case H5O_TYPE_GROUP:
							cgroup = cgroup->getGroup(subpath.c_str());
							break;
						case H5O_TYPE_DATASET:
							cgroup = cgroup->getDataset(subpath.c_str());
							break;
						default:
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "HDF5Handle::getPath: path element" << subpath << " is not a group or dataset" << std::endl;
							lme.finish();
							throw lme;			
						}
					}
					
					path = pathe;
					if ( *path )
						++path;
				}
				
				return cgroup;
			}
			
			static herr_t enumChildren(hid_t loc_id, const char *name, const H5L_info_t * /* info */, void *operator_data)
			{
				H5O_info_t infobuf;
				herr_t status = 0;
				status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
				std::vector< std::pair<std::string, H5O_type_t> > * V = reinterpret_cast<std::vector< std::pair<std::string, H5O_type_t> > *>(operator_data);
				
				if ( status < 0 )
					return status;
				
				V->push_back(std::pair<std::string, H5O_type_t>(name,infobuf.type));

				return status;
			}
			
			std::vector< std::pair<std::string, H5O_type_t> > getChildren() const
			{
				std::vector< std::pair<std::string, H5O_type_t> > V;
				hsize_t * idx = NULL;
				
				H5Literate (
					handle, 
					H5_INDEX_NAME, 
					H5_ITER_NATIVE, 
					idx, 
					enumChildren, 
					&V );
				
				return V;
			}
			
			struct HasChildInfo
			{
				std::string const & name;
				bool present;
				H5O_type_t type;
				
				HasChildInfo(std::string const & rname)
				: name(rname), present(false)
				{
					
				}
			};

			static herr_t hasChildCallback(hid_t loc_id, const char *name, const H5L_info_t * /* info */, void *operator_data)
			{
				HasChildInfo * info = reinterpret_cast<HasChildInfo *>(operator_data);
				herr_t status = 0;
				
				if ( std::string(name) == info->name )
				{					
					H5O_info_t infobuf;
					status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
					
					if ( status < 0 )
						return status;
						
					info->present = true;
					info->type = infobuf.type;
				}				
	
				return status;
			}
			
			std::pair<bool,H5O_type_t> hasChild(std::string const & name) const
			{
				HasChildInfo info(name);
				hsize_t * idx = NULL;
				
				H5Literate (
					handle, 
					H5_INDEX_NAME, 
					H5_ITER_NATIVE, 
					idx, 
					hasChildCallback, 
					&info );

				return std::pair<bool,H5O_type_t>(info.present,info.type);
			}
			
			bool hasPath(char const * path) const
			{
				// clone current
				shared_ptr_type cgroup = getGroup(".");
				
				// if path starts with a slash
				if ( *path && *path == '/' )
				{
					cgroup = getGroup("/");
					++path;
				}
			
				while ( *path != 0 )
				{
					char const * pathe = path;
					while ( *pathe && *pathe != '/' )
						++pathe;
						
					std::string subpath(path,pathe);
																				
					if ( !cgroup->hasChild(subpath.c_str()).first )
						return false;
					
					path = pathe;
					if ( *path )
						++path;
					
					if ( *path )
						cgroup = cgroup->getGroup(subpath.c_str());
				}
				
				return true;
			}

			struct HasAttributeInfo
			{
				std::string const & name;
				bool present;
				
				HasAttributeInfo(std::string const & rname)
				: name(rname), present(false) {}
			};
			
			static herr_t hasAttribute(hid_t loc_id, const char *name, const H5A_info_t * /* ainfo */, void *opdata)
			{
				HasAttributeInfo * info = reinterpret_cast<HasAttributeInfo *>(opdata);
			
				shared_ptr_type attr(new this_type);
				shared_ptr_type dataspace(new this_type);
				shared_ptr_type datatype(new this_type);
				
				attr->handle = H5Aopen(loc_id,name,H5P_DEFAULT);
				
				if ( attr->handle < 0 )
					return -1;
				
				attr->handle_type = hdf5_handle_type_attribute;
				
				dataspace->handle = H5Aget_space(attr->handle);
				
				if ( dataspace->handle < 0 )
					return -1;
				
				dataspace->handle_type = hdf5_handle_type_dataspace;
				
				datatype->handle = H5Aget_type(attr->handle);
				
				if ( datatype->handle < 0 )
					return -1;
				
				datatype->handle = hdf5_handle_type_datatype;

				if ( std::string(name) == info->name )
					info->present = true;

				return 0;
			}
		
			bool hasAttribute(std::string const & name) const
			{
				HasAttributeInfo info(name);
				herr_t ret = H5Aiterate2(handle, H5_INDEX_NAME, H5_ITER_INC, NULL, hasAttribute, &info);
				
				if ( ret < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::hasAttribute: failed" << std::endl;
					lme.finish();
					throw lme;				
				}
				
				return info.present;
			}

			static herr_t getAttributes(hid_t loc_id, const char *name, const H5A_info_t * /* ainfo */, void *opdata)
			{
				std::vector<std::string> * info = reinterpret_cast< std::vector<std::string> * >(opdata);
			
				shared_ptr_type attr(new this_type);
				shared_ptr_type dataspace(new this_type);
				shared_ptr_type datatype(new this_type);
				
				attr->handle = H5Aopen(loc_id,name,H5P_DEFAULT);
				
				if ( attr->handle < 0 )
					return -1;
				
				attr->handle_type = hdf5_handle_type_attribute;
				
				dataspace->handle = H5Aget_space(attr->handle);
				
				if ( dataspace->handle < 0 )
					return -1;
				
				dataspace->handle_type = hdf5_handle_type_dataspace;
				
				datatype->handle = H5Aget_type(attr->handle);
				
				if ( datatype->handle < 0 )
					return -1;
				
				datatype->handle = hdf5_handle_type_datatype;

				info->push_back(name);

				return 0;
			}
			
			std::vector<std::string> getAttributes() const
			{
				std::vector<std::string> info;
				herr_t ret = H5Aiterate2(handle, H5_INDEX_NAME, H5_ITER_INC, NULL, getAttributes, &info);
				
				if ( ret < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::getAttributes: failed" << std::endl;
					lme.finish();
					throw lme;				
				}
				
				return info;
			}
			
			shared_ptr_type getDataset(char const * name) const
			{
				shared_ptr_type dataset(new this_type);

				dataset->handle = H5Dopen(handle,name,H5P_DEFAULT);
				
				if ( dataset->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::getDataset: failed for " << name << std::endl;
					lme.finish();
					throw lme;								
				}
				
				dataset->handle_type = hdf5_handle_type_dataset;
				
				return dataset;
			}
			
			std::string datasetDecodeString() const
			{
				assert ( getType() == H5O_TYPE_DATASET );

				shared_ptr_type datatype(new this_type);
				shared_ptr_type dataspace(new this_type);

				dataspace->handle = H5Dget_space(handle);
				
				if ( dataspace->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::datasetToString: failed" << std::endl;
					lme.finish();
					throw lme;								

				}
				
				dataspace->handle_type = hdf5_handle_type_dataspace;

				datatype->handle = H5Dget_type(handle);
				
				if ( datatype->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::datasetToString: failed" << std::endl;
					lme.finish();
					throw lme;								
				}
				
				datatype->handle_type = hdf5_handle_type_datatype;
				
				H5T_class_t dclass = H5Tget_class(datatype->handle);
				int const rank = H5Sget_simple_extent_ndims(dataspace->handle);
				
				if ( dclass == H5T_STRING && rank == 0 && (! (H5Tis_variable_str(datatype->handle))) )
				{
					size_t const size = H5Tget_size(datatype->handle);
					libmaus::autoarray::AutoArray<char> A(size,false);					
					herr_t err = H5Dread(handle, datatype->handle, H5S_ALL, H5S_ALL, H5P_DEFAULT, A.begin());
					
					if ( err < 0 )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HDF5Handle::datasetToString: failed" << std::endl;
						lme.finish();
						throw lme;														
					}
					
					return std::string(A.begin(),A.end());
				}
				else
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::datasetToString: failed, not a fixed length string" << std::endl;
					lme.finish();
					throw lme;													
				}
			}

			std::string decodeAttributeString(const char *name) const
			{
				shared_ptr_type attr(new this_type);
				shared_ptr_type dataspace(new this_type);
				shared_ptr_type datatype(new this_type);

				attr->handle = H5Aopen(handle,name,H5P_DEFAULT);
				
				if ( attr->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
					lme.finish();
					throw lme;													
				}
				
				attr->handle_type = hdf5_handle_type_attribute;
				
				dataspace->handle = H5Aget_space(attr->handle);
				
				if ( dataspace->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
					lme.finish();
					throw lme;
				}
				
				dataspace->handle_type = hdf5_handle_type_dataspace;
				
				datatype->handle = H5Aget_type(attr->handle);
				
				if ( datatype->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
					lme.finish();
					throw lme;
				}
				
				datatype->handle_type = hdf5_handle_type_datatype;

				H5T_class_t dclass = H5Tget_class(datatype->handle);
				int const rank = H5Sget_simple_extent_ndims(dataspace->handle);
				
				if ( dclass == H5T_STRING && rank == 0 && (! (H5Tis_variable_str(datatype->handle))) )
				{
					size_t const size = H5Tget_size(datatype->handle);
					libmaus::autoarray::AutoArray<char> A(size,false);					
					herr_t err = H5Aread(attr->handle, datatype->handle, A.begin());
					
					if ( err < 0 )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
						lme.finish();
						throw lme;														
					}
					
					return std::string(A.begin(),A.end());
				}
				else if ( dclass == H5T_STRING && rank == 0 && H5Tis_variable_str(datatype->handle) )
				{
					char const * c = 0;
					herr_t err = H5Aread(attr->handle, datatype->handle, &c);

					if ( err < 0 )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
						lme.finish();
						throw lme;														
					}
					
					std::string const s = c;
					
					return s;
				}
				else if ( dclass == H5T_FLOAT && rank == 0 )
				{
					double d;
					herr_t err = H5Aread(attr->handle, H5T_NATIVE_DOUBLE, &d);

					if ( err < 0 )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
						lme.finish();
						throw lme;														
					}
					
					std::ostringstream ostr;
					ostr << d;
					
					return ostr.str();
				}
				else if ( dclass == H5T_INTEGER && rank == 0 )
				{
					H5T_sign_t const sign = H5Tget_sign(datatype->handle);
				
					switch ( sign )
					{
						case H5T_SGN_NONE:
						{
							uint64_t n;
							herr_t err = H5Aread(attr->handle,H5T_NATIVE_UINT64,&n);

							if ( err < 0 )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
								lme.finish();
								throw lme;														
							}
							
							std::ostringstream ostr;
							ostr << n;
							
							return ostr.str();
						}
						case H5T_SGN_2:
						{
							int64_t n;
							herr_t err = H5Aread(attr->handle,H5T_NATIVE_INT64,&n);

							if ( err < 0 )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
								lme.finish();
								throw lme;														
							}
							
							std::ostringstream ostr;
							ostr << n;
							
							return ostr.str();
						}
						default:
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "HDF5Handle::decodeAttributeString: failed" << std::endl;
							lme.finish();
							throw lme;
						}
					}
				}
				else
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeString: failed (not a string, class="<< dclass << ")" << std::endl;
					lme.finish();
					throw lme;																	
				}
			}

			double decodeAttributeDouble(const char *name)
			{
				shared_ptr_type attr(new this_type);
				shared_ptr_type dataspace(new this_type);
				shared_ptr_type datatype(new this_type);

				attr->handle = H5Aopen(handle,name,H5P_DEFAULT);
				
				if ( attr->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeDouble: failed" << std::endl;
					lme.finish();
					throw lme;													
				}
				
				attr->handle_type = hdf5_handle_type_attribute;
				
				dataspace->handle = H5Aget_space(attr->handle);
				
				if ( dataspace->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeDouble: failed" << std::endl;
					lme.finish();
					throw lme;
				}
				
				dataspace->handle_type = hdf5_handle_type_dataspace;
				
				datatype->handle = H5Aget_type(attr->handle);
				
				if ( datatype->handle < 0 )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeDouble: failed" << std::endl;
					lme.finish();
					throw lme;
				}
				
				datatype->handle_type = hdf5_handle_type_datatype;

				H5T_class_t dclass = H5Tget_class(datatype->handle);
				int const rank = H5Sget_simple_extent_ndims(dataspace->handle);

				if ( dclass == H5T_FLOAT && rank == 0 )
				{
					double d;
					herr_t err = H5Aread(attr->handle, H5T_NATIVE_DOUBLE /* datatype->handle */, &d);
					
					if ( err < 0 )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HDF5Handle::decodeAttributeDouble: failed" << std::endl;
						lme.finish();
						throw lme;														
					}
					
					return d;
				}
				else
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HDF5Handle::decodeAttributeDouble: failed (not a string)" << std::endl;
					lme.finish();
					throw lme;																	
				}
			}
		};	
	}
}
#endif

#endif
