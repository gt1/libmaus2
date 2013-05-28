#include <libmaus/quantisation/KMLocalInterface.hpp>
#include <libmaus/quantisation/ClusterComputation.hpp>
#include <libmaus/quantisation/KmeansDataType.hpp>
#include <libmaus/util/DynamicLoading.hpp>

static std::vector<double> * kmeansWrapper(std::vector<double>::const_iterator vita, uint64_t const rn, uint64_t const k, uint64_t const runs, bool const debug = false)
{
	#if defined(LIBMAUS_HAVE_DL_FUNCS)
	::libmaus::util::DynamicLibraryFunction<libmaus_quantisation_kmeansWrapperByTypeC_type> DLF("liblibmaus_kmlocal_mod.so","libmaus_quantisation_kmeansWrapperByTypeC");
	return reinterpret_cast< std::vector<double> *>(DLF.func(::libmaus::quantisation::type_double,&vita,rn,k,runs,debug));
	#else
	::libmaus::exception::LibMausException se;
	se.getStream() << "dynamic loading functionality for modules is not present, cannot call kmeans clustering." << std::endl;
	se.finish();
	throw se;
	#endif
}
static std::vector<double> * kmeansWrapper(std::vector<unsigned int>::const_iterator vita, uint64_t const rn, uint64_t const k, uint64_t const runs, bool const debug = false)
{
	#if defined(LIBMAUS_HAVE_DL_FUNCS)
	::libmaus::util::DynamicLibraryFunction<libmaus_quantisation_kmeansWrapperByTypeC_type> DLF("liblibmaus_kmlocal_mod.so","libmaus_quantisation_kmeansWrapperByTypeC");
	return reinterpret_cast< std::vector<double> *>(DLF.func(::libmaus::quantisation::type_unsigned_int,&vita,rn,k,runs,debug));
	#else
	::libmaus::exception::LibMausException se;
	se.getStream() << "dynamic loading functionality for modules is not present, cannot call kmeans clustering." << std::endl;
	se.finish();
	throw se;
	#endif
}
static std::vector<double> * kmeansWrapper(std::vector<uint64_t>::const_iterator vita, uint64_t const rn, uint64_t const k, uint64_t const runs, bool const debug = false)
{
	#if defined(LIBMAUS_HAVE_DL_FUNCS)
	::libmaus::util::DynamicLibraryFunction<libmaus_quantisation_kmeansWrapperByTypeC_type> DLF("liblibmaus_kmlocal_mod.so","libmaus_quantisation_kmeansWrapperByTypeC");
	return reinterpret_cast< std::vector<double> *>(DLF.func(::libmaus::quantisation::type_uint64_t,&vita,rn,k,runs,debug));
	#else
	::libmaus::exception::LibMausException se;
	se.getStream() << "dynamic loading functionality for modules is not present, cannot call kmeans clustering." << std::endl;
	se.finish();
	throw se;
	#endif
}

template<typename iterator>
::libmaus::quantisation::Quantiser::unique_ptr_type libmaus::quantisation::ClusterComputation::constructQuantiserTemplate(
	iterator ita, uint64_t const n, uint64_t const k, uint64_t const runs)
{
	if ( k < n )
	{
		::libmaus::util::unique_ptr< std::vector<double> >::type pcentres(::kmeansWrapper(ita,n,std::min(k,n),runs));
		
		if ( ! pcentres )
		{
			::libmaus::exception::LibMausException se;
			se.getStream() << "Failed to compute clusters, cluster computation returned null pointer (see error further back)" << std::endl;
			se.finish();
			throw se;
		}
		
		std::vector<double> const & centres = *pcentres;
		::libmaus::quantisation::Quantiser * ptr = new ::libmaus::quantisation::Quantiser(centres);
		::libmaus::quantisation::Quantiser::unique_ptr_type uptr(ptr);
		return UNIQUE_PTR_MOVE(uptr);
	}
	else
	{
		// std::cerr << "libmaus::quantisation::ClusterComputation::constructQuantiserTemplate(): direct." << std::endl;
		std::vector<double> const centres(ita,ita+n);
		
		#if 0
		std::cerr << "centres: {";
		for ( uint64_t i = 0; i < centres.size(); ++ i )
			std::cerr << centres[i] << ";";
		std::cerr << "}\n";
		#endif
		
		::libmaus::quantisation::Quantiser * ptr = new ::libmaus::quantisation::Quantiser(centres);
		::libmaus::quantisation::Quantiser::unique_ptr_type uptr(ptr);
		return UNIQUE_PTR_MOVE(uptr);
	}
}

::libmaus::quantisation::Quantiser::unique_ptr_type libmaus::quantisation::ClusterComputation::constructQuantiser(
	std::vector<double> const & V, uint64_t const k, uint64_t const runs)
{
	return UNIQUE_PTR_MOVE(constructQuantiserTemplate<std::vector<double>::const_iterator>(V.begin(),V.size(),k,runs));
}

::libmaus::quantisation::Quantiser::unique_ptr_type libmaus::quantisation::ClusterComputation::constructQuantiser(
	std::vector<unsigned int> const & V, uint64_t const k, uint64_t const runs)
{
	return UNIQUE_PTR_MOVE(constructQuantiserTemplate<std::vector<unsigned int>::const_iterator>(V.begin(),V.size(),k,runs));
}

::libmaus::quantisation::Quantiser::unique_ptr_type libmaus::quantisation::ClusterComputation::constructQuantiser(
	std::vector<uint64_t> const & V, uint64_t const k, uint64_t const runs)
{
	return UNIQUE_PTR_MOVE(constructQuantiserTemplate<std::vector<uint64_t>::const_iterator>(V.begin(),V.size(),k,runs));
}


