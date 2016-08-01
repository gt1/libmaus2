ACLOCAL_AMFLAGS=-I m4

EXTRA_DIST = \
	libmaus2/digest/sha256_sse4.asm \
	libmaus2/digest/sha512_sse4.asm \
	libmaus2/digest/sha256_avx1.asm \
	libmaus2/digest/sha512_avx.asm \
	libmaus2/digest/sha256_avx2_rorx2.asm \
	libmaus2/digest/sha256_avx2_rorx8.asm \
	libmaus2/digest/sha512_avx2_rorx.asm \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentScoreTemplate.cpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentTemplate.cpp \
	libmaus2/lcs/SimdX86GlobalAlignmentTemplate.cpp

abs_builddir = @abs_builddir@
abs_srcdir = @abs_srcdir@

lib_LTLIBRARIES = libmaus2.la libmaus2lsf.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
lib_LIBRARIES = libmaus2digests.a libmaus2digests_sse4.a libmaus2_irods.a libmaus2seqchksumsfactory.a libmaus2fftw.a

libmaus2_simd_align_128_la_SOURCES = \
	libmaus2/lcs/SimdX86GlobalAlignmentConstants128.cpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_8.cpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_16.cpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_8.cpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_16.cpp
libmaus2_simd_align_128_la_CXXFLAGS = @MSSE2@ @MSSSE3@ @MSSE41@ @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS}
libmaus2_simd_align_128_la_LDFLAGS = @LIBMAUSLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@

libmaus2_simd_align_256_la_SOURCES = \
	libmaus2/lcs/SimdX86GlobalAlignmentConstants256.cpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_8.cpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_16.cpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentScoreY256_16.cpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_8.cpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_16.cpp
libmaus2_simd_align_256_la_CXXFLAGS = @MSSE2@ @MSSSE3@ @MSSE41@ @MAVX2@ @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS}
libmaus2_simd_align_256_la_LDFLAGS = @LIBMAUSLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@

libmaus2_modules_LTLIBRARIES = libmaus2_consensus_mod.la libmaus2_scram_mod.la
libmaus2_modulesdir = $(libdir)/libmaus2/$(PACKAGE_VERSION)

libmaus2digests_a_SOURCES = libmaus2/digest/SHA2_256_sse4.cpp libmaus2/digest/SHA2_512_sse4.cpp libmaus2/digest/DigestFactory_SHA2_ASM.cpp
libmaus2digests_a_DEPENDENCIES = @DIGESTASMOBJ@
libmaus2digests_a_LIBADD = @DIGESTASMOBJ@

libmaus2digests_sse4_a_SOURCES = libmaus2/digest/CRC32C_sse42.cpp libmaus2/digest/DigestFactory_CRC32C_SSE42.cpp
libmaus2digests_sse4_a_CXXFLAGS = @MSSE4@

libmaus2_irods_a_SOURCES = libmaus2/irods/IRodsSystem.cpp libmaus2/irods/IRodsFileBase.cpp
libmaus2_irods_a_CPPFLAGS = @IRODSCPPFLAGS@ @WARNCPPFLAGS@

libmaus2seqchksumsfactory_a_SOURCES = libmaus2/bambam/ChecksumsFactory.cpp
libmaus2seqchksumsfactory_a_CPPFLAGS = @GMPCPPFLAGS@

libmaus2fftw_a_SOURCES = libmaus2/math/binom_fftw.cpp libmaus2/math/Convolution_fftw.cpp
libmaus2fftw_a_CXXFLAGS = @FFTWCPPFLAGS@

CLEANFILES=sha256_sse4.o sha256_avx1.o sha256_avx2_rorx2.o sha256_avx2_rorx8.o sha512_avx2_rorx.o sha512_avx.o sha512_sse4.o

sha256_sse4.o: $(abs_srcdir)/libmaus2/digest/sha256_sse4.asm
	@YASMEXE@ -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o sha256_sse4.o $(abs_srcdir)/libmaus2/digest/sha256_sse4.asm
sha256_avx1.o: $(abs_srcdir)/libmaus2/digest/sha256_avx1.asm
	@YASMEXE@ -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o sha256_avx1.o $(abs_srcdir)/libmaus2/digest/sha256_avx1.asm
sha256_avx2_rorx2.o: $(abs_srcdir)/libmaus2/digest/sha256_avx2_rorx2.asm
	@YASMEXE@ -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o sha256_avx2_rorx2.o $(abs_srcdir)/libmaus2/digest/sha256_avx2_rorx2.asm
sha256_avx2_rorx8.o: $(abs_srcdir)/libmaus2/digest/sha256_avx2_rorx8.asm
	@YASMEXE@ -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o sha256_avx2_rorx8.o $(abs_srcdir)/libmaus2/digest/sha256_avx2_rorx8.asm
sha512_avx2_rorx.o: $(abs_srcdir)/libmaus2/digest/sha512_avx2_rorx.asm
	@YASMEXE@ -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o sha512_avx2_rorx.o $(abs_srcdir)/libmaus2/digest/sha512_avx2_rorx.asm
sha512_avx.o: $(abs_srcdir)/libmaus2/digest/sha512_avx.asm
	@YASMEXE@ -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o sha512_avx.o $(abs_srcdir)/libmaus2/digest/sha512_avx.asm
sha512_sse4.o: $(abs_srcdir)/libmaus2/digest/sha512_sse4.asm
	@YASMEXE@ -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o sha512_sse4.o $(abs_srcdir)/libmaus2/digest/sha512_sse4.asm

libmaus2_la_SOURCES = libmaus2/StaticInitialization.cpp \
	libmaus2/autoarray/AutoArray.cpp \
	libmaus2/bitio/CompactArray.cpp libmaus2/rank/RankTable.cpp \
	libmaus2/rank/log2table.cpp libmaus2/fastx/isFastQ.cpp libmaus2/wavelet/toWaveletTreeBits.cpp \
	libmaus2/suffixsort/divsufsort.cpp libmaus2/rmq/FischerSystematicSuccinctRMQ.cpp libmaus2/lcp/OracleLCP.cpp \
	libmaus2/util/StackTrace.cpp libmaus2/lcs/OverlapComputation.cpp \
	libmaus2/lcs/HashContainer.cpp \
	libmaus2/lcs/HashContainer2.cpp \
	libmaus2/digest/md5.cpp \
	libmaus2/util/I386CacheLineSize.cpp \
	libmaus2/math/primes16.cpp \
	libmaus2/util/OutputFileNameTools.cpp \
	libmaus2/select/ESelectBase.cpp \
	libmaus2/bambam/CigarStringParser.cpp \
	libmaus2/consensus/Consensus.cpp \
	libmaus2/graph/EdgeList.cpp \
	libmaus2/fastx/Phred.cpp \
	libmaus2/network/sendReceiveFd.c \
	libmaus2/lz/SnappyCompress.cpp \
	libmaus2/bambam/CollatingBamDecoder.cpp \
	libmaus2/bambam/ReadEndsContainer.cpp \
	libmaus2/bambam/ReadEnds.cpp \
	libmaus2/network/LogReceiver.cpp \
	libmaus2/huffman/IndexLoaderBase.cpp \
	libmaus2/bambam/BamAlignmentDecoderBase.cpp \
	libmaus2/bambam/StrCmpNum.cpp \
	libmaus2/huffman/KvInitResult.cpp \
	libmaus2/fm/MausFmToBwaConversion.cpp \
	libmaus2/util/TempFileNameGenerator.cpp \
	libmaus2/util/TempFileNameGeneratorState.cpp \
	libmaus2/util/ArgInfo.cpp \
	libmaus2/util/Array832.cpp \
	libmaus2/util/Array864.cpp \
	libmaus2/util/BitList.cpp \
	libmaus2/util/VarBitList.cpp \
	libmaus2/util/Concat.cpp \
	libmaus2/util/ConcatRequest.cpp \
	libmaus2/util/ConsoleColor.cpp \
	libmaus2/util/Demangle.cpp \
	libmaus2/util/DynamicLoading.cpp \
	libmaus2/util/ForkProcess.cpp \
	libmaus2/util/LSFSim.cpp \
	libmaus2/util/ForkProcessLSFSim.cpp \
	libmaus2/util/GetFileSize.cpp \
	libmaus2/util/Histogram.cpp \
	libmaus2/util/HistogramSet.cpp \
	libmaus2/util/IncreasingList.cpp \
	libmaus2/util/IntervalTree.cpp \
	libmaus2/util/GenericIntervalTree.cpp \
	libmaus2/util/LogPipe.cpp \
	libmaus2/util/LogPipeMultiplex.cpp \
	libmaus2/util/LogPipeMultiplexGeneric.cpp \
	libmaus2/util/LookupIntervalTree.cpp \
	libmaus2/digest/md5_interface.cpp \
	libmaus2/util/MemLimit.cpp \
	libmaus2/util/MemUsage.cpp \
	libmaus2/util/NearestNeighbourDictionary.cpp \
	libmaus2/util/NegativeDifferenceArray32.cpp \
	libmaus2/util/NegativeDifferenceArray64.cpp \
	libmaus2/util/nstat.cpp \
	libmaus2/util/PositiveDifferenceArray32.cpp \
	libmaus2/util/PositiveDifferenceArray64.cpp \
	libmaus2/util/PosixExecute.cpp \
	libmaus2/util/PosixFileDescriptor.cpp \
	libmaus2/util/PosixInputFile.cpp \
	libmaus2/util/SaturatingCounter.cpp \
	libmaus2/util/StringSerialisation.cpp \
	libmaus2/util/SuccinctBorderArray.cpp \
	libmaus2/util/Terminal.cpp \
	libmaus2/util/OctetString.cpp \
	libmaus2/util/Utf8String.cpp \
	libmaus2/util/Utf8BlockIndexDecoder.cpp \
	libmaus2/util/Utf8BlockIndex.cpp \
	libmaus2/util/Utf8DecoderBuffer.cpp \
	libmaus2/util/Utf8DecoderWrapper.cpp \
	libmaus2/suffixtree/CompressedSuffixTree.cpp \
	libmaus2/bambam/AlignmentValidity.cpp \
	libmaus2/bambam/BamFlagBase.cpp \
	libmaus2/bambam/ReadGroup.cpp \
	libmaus2/bambam/DuplicationMetrics.cpp \
	libmaus2/bambam/BamDefaultAdapters.cpp \
	libmaus2/bambam/BamRange.cpp \
	libmaus2/graph/GraphEdge.cpp \
	libmaus2/bambam/BamIndexBinChunk.cpp \
	libmaus2/bambam/BamIndexLinearChunk.cpp \
	libmaus2/huffman/HuffmanTree.cpp \
	libmaus2/bambam/BamAlignmentDecoderInfo.cpp \
	libmaus2/lz/Lz4Base.cpp \
	libmaus2/lz/Lz4Compress.cpp \
	libmaus2/lz/Lz4DecoderBuffer.cpp \
	libmaus2/lz/lz4.c \
	libmaus2/fm/BidirectionalIndexInterval.cpp  \
	libmaus2/fm/BidirectionalIndexIntervalSymbol.cpp \
	libmaus2/gamma/SparseGammaGapFileIndexDecoderEntry.cpp \
	libmaus2/fastx/FastALineParserLineInfo.cpp \
	libmaus2/fastx/FastAMatchTable.cpp \
	libmaus2/lcs/EditDistanceResult.cpp \
	libmaus2/fastx/FastABgzfIndexEntry.cpp \
	libmaus2/fastx/FastABgzfIndex.cpp \
	libmaus2/parallel/ThreadWorkPackage.cpp \
	libmaus2/parallel/SimpleThreadWorkPackage.cpp \
	libmaus2/lz/RAZFIndex.cpp \
	libmaus2/fastx/FastAIndexEntry.cpp \
	libmaus2/fastx/FastAIndex.cpp \
	libmaus2/network/HttpAbsoluteUrl.cpp \
	libmaus2/hashing/Crc32.cpp \
	libmaus2/hashing/MurmurHash3.cpp \
	libmaus2/lz/IGzipDeflate.cpp \
	libmaus2/lcs/LocalBaseConstants.cpp \
	libmaus2/lcs/LocalEditDistanceResult.cpp \
	libmaus2/bambam/BamHeaderParserStateBase.cpp \
	libmaus2/bambam/GeneFlatFile.cpp \
	libmaus2/bambam/GeneFlatFileEntry.cpp \
	libmaus2/trie/SimpleTrie.cpp \
	libmaus2/network/GnuTLSInit.cpp \
	libmaus2/network/GnuTLSSocket.cpp \
	libmaus2/bambam/BamStreamingMarkDuplicatesSupport.cpp \
	libmaus2/alignment/FactorMatchInfo.cpp \
	libmaus2/alignment/KmerInfo.cpp \
	libmaus2/bambam/SamInfoBase.cpp \
	libmaus2/digest/SHA1.cpp \
	libmaus2/digest/SHA2_224.cpp \
	libmaus2/digest/SHA2_256.cpp \
	libmaus2/digest/SHA2_384.cpp \
	libmaus2/digest/SHA2_512.cpp \
	libmaus2/digest/CRC32.cpp \
	libmaus2/digest/CRC32C_Core.cpp \
	libmaus2/digest/CRC32C.cpp \
	libmaus2/digest/CRC32_Core.cpp \
	libmaus2/bambam/parallel/ValidationFragment.cpp \
	libmaus2/bambam/parallel/GenericInputControlStreamInfo.cpp \
	libmaus2/fastx/NameInfoBase.cpp \
	libmaus2/bambam/parallel/RefIdInterval.cpp \
	libmaus2/fastx/RefPathToken.cpp \
	libmaus2/fastx/RefPathTokenVector.cpp \
	libmaus2/dazzler/db/FastaInfo.cpp \
	libmaus2/dazzler/db/IndexBase.cpp \
	libmaus2/dazzler/db/DatabaseFile.cpp \
	libmaus2/dazzler/db/Read.cpp \
	libmaus2/dazzler/align/Path.cpp \
	libmaus2/dazzler/align/Overlap.cpp \
	libmaus2/lcs/AlignmentStatistics.cpp \
	libmaus2/fastx/SingleWordDNABitBuffer.cpp \
	libmaus2/util/LongestIncreasingSubsequenceExtendedResult.cpp \
	libmaus2/lcs/AlignerFactory.cpp \
	libmaus2/lcs/BandedAlignerFactory.cpp \
	libmaus2/aio/InputStreamInstance.cpp \
	libmaus2/lcs/DalignerNP.cpp \
	libmaus2/aio/OutputStreamInstance.cpp \
	libmaus2/aio/OutputStreamFactoryContainer.cpp \
	libmaus2/dazzler/align/TraceBlock.cpp \
	libmaus2/dazzler/align/ApproximateRun.cpp \
	libmaus2/suffixsort/BwtMergeBlockSortRequest.cpp \
	libmaus2/dazzler/align/OverlapMeta.cpp \
	libmaus2/lcs/DalignerLocalAlignment.cpp \
	libmaus2/dazzler/align/Tandem.cpp \
	libmaus2/dazzler/align/TandemVector.cpp \
	libmaus2/math/GmpInteger.cpp \
	libmaus2/math/GmpFloat.cpp \
	libmaus2/dazzler/align/TracePoint.cpp \
	libmaus2/lz/XzDecoder.cpp \
	libmaus2/util/ArgParser.cpp \
	libmaus2/lcs/FragmentEnvelopeYScore.cpp \
	libmaus2/lcs/FragmentEnvelopeFragment.cpp \
	libmaus2/fastx/DNAIndexMetaDataSequence.cpp \
	libmaus2/fastx/DNAIndexMetaData.cpp \
	libmaus2/bambam/PileVectorElement.cpp \
	libmaus2/digest/SHA3_256.cpp \
	libmaus2/fastx/DNAIndexMetaDataBigBand.cpp \
	libmaus2/fastx/DNAIndexMetaDataBigBandBiDir.cpp \
	libmaus2/lcs/EnvelopeFragment.cpp \
	libmaus2/rank/DNARankBiDirRange.cpp \
	libmaus2/lcs/NNPTraceElement.cpp \
	libmaus2/lcs/NNPTraceContainer.cpp \
	libmaus2/lcs/NNPAlignResult.cpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequest.cpp \
	libmaus2/suffixsort/bwtb3m/BwtMergeSort.cpp \
	libmaus2/suffixsort/bwtb3m/BwtComputeSSA.cpp \
	libmaus2/fastx/FastAToCompact4BigBand.cpp \
	libmaus2/suffixsort/bwtb3m/BwtMergeSortResult.cpp \
	libmaus2/util/HugePages.cpp \
	libmaus2/lz/ZlibInterface.cpp \
	libmaus2/suffixsort/bwtb3m/BwtSelectSSA.cpp \
	libmaus2/aio/FileRemoval.cpp \
	libmaus2/rank/DNARankMEM.cpp \
	libmaus2/lcs/ChainAddQueueElement.cpp \
	libmaus2/lcs/Chain.cpp \
	libmaus2/lcs/ChainNode.cpp \
	libmaus2/lcs/ChainNodeInfo.cpp \
	libmaus2/lcs/ChainRemoveQueueElement.cpp \
	libmaus2/lcs/ChainSplayTreeNode.cpp

noinst_HEADERS = libmaus2/lz/lz4.h

libmaus2_la_CPPFLAGS = @LIBMAUSCPPFLAGS@ @SNAPPYCPPFLAGS@ @ZLIBCPPFLAGS@ @WARNCPPFLAGS@ @GNUTLSCPPFLAGS@ @NETTLECPPFLAGS@ @IGZIPCPPFLAGS@ @DALIGNERCPPFLAGS@ @GMPCPPFLAGS@ @LIBMAUS2_LZMA_CPPFLAGS@
libmaus2_la_CXXFLAGS = @SEQANCXXFLAGS@ @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS}
libmaus2_la_LDFLAGS = @LIBMAUSLDFLAGS@ @SNAPPYLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@ @IGZIPLDFLAGS@ @GNUTLSLDFLAGS@ @NETTLELDFLAGS@ @DALIGNERLDFLAGS@ @GMPLDFLAGS@
libmaus2_la_LIBADD = @DLLIB@ @SNAPPYLIBS@ @ZLIBLIBS@ @IGZIPLIBS@ @GNUTLSLIBS@ @NETTLELIBS@ @DALIGNERLIBS@ @GMPLIBS@ @LIBMAUS2_LZMA_LIBS@

libmaus2lsf_la_LDFLAGS = @LSFLDFLAGS@ @LIBMAUSLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@
libmaus2lsf_la_SOURCES = libmaus2/lsf/LSFProcess.cpp
libmaus2lsf_la_CPPFLAGS = @WARNCPPFLAGS@ ${LSFCPPFLAGS}
libmaus2lsf_la_LIBADD = ${LSFLIBS}

libmaus2_consensus_mod_la_CPPFLAGS = @SEQANCPPFLAGS@                 @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS} @WARNCPPFLAGS@
libmaus2_consensus_mod_la_CXXFLAGS = @SEQANCXXFLAGS@                 @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus2_consensus_mod_la_LDFLAGS  = @SEQANLDFLAGS@  @SEQANLIBS@     @LIBMAUSLDFLAGS@  @LIBMAUSLIBS@     -module  -shared -export-dynamic ${AM_LDFLAGS} -avoid-version
libmaus2_consensus_mod_la_LIBADD   = @SEQANLIBS@ @LIBMAUSLIBS@
libmaus2_consensus_mod_la_SOURCES  = libmaus2/consensus/ConsensusBase.cpp libmaus2/consensus/ScoringMatrix.cpp \
	libmaus2/fastx/Phred.cpp

libmaus2_scram_mod_la_CPPFLAGS = @IOLIBCPPFLAGS@ @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS} @WARNCPPFLAGS@
libmaus2_scram_mod_la_CXXFLAGS = @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus2_scram_mod_la_LDFLAGS  = @IOLIBLDFLAGS@  @IOLIBLIBS@     @LIBMAUSLDFLAGS@  @LIBMAUSLIBS@     -module  -shared -export-dynamic ${AM_LDFLAGS} -avoid-version
libmaus2_scram_mod_la_LIBADD   = @IOLIBLIBS@ @LIBMAUSLIBS@
libmaus2_scram_mod_la_SOURCES  = libmaus2/bambam/Scram.c

libmaus2_includedir=$(includedir)/libmaus2/
libmaus2_include_HEADERS=libmaus2/LibMausConfig.hpp

libmaus2dazzlerdb_includedir=$(includedir)/libmaus2/dazzler/db
libmaus2dazzlerdb_include_HEADERS = libmaus2/dazzler/db/IndexBase.hpp  libmaus2/dazzler/db/FastaInfo.hpp libmaus2/dazzler/db/DatabaseFile.hpp libmaus2/dazzler/db/InputBase.hpp \
	libmaus2/dazzler/db/GetByteCounter.hpp libmaus2/dazzler/db/Read.hpp libmaus2/dazzler/db/OutputBase.hpp \
	libmaus2/dazzler/db/TrackAnnoInterface.hpp libmaus2/dazzler/db/TrackAnno.hpp \
	libmaus2/dazzler/db/Track.hpp libmaus2/dazzler/db/InqualContainer.hpp libmaus2/dazzler/db/PartTrackContainer.hpp \
	libmaus2/dazzler/db/ReadLRUContainer.hpp libmaus2/dazzler/db/SatsContainer.hpp libmaus2/dazzler/db/ReadContainer.hpp \
	libmaus2/dazzler/db/StringTrackWriter.hpp
	
libmaus2dazzleralign_includedir=$(includedir)/libmaus2/dazzler/align
libmaus2dazzleralign_include_HEADERS = libmaus2/dazzler/align/AlignmentFile.hpp libmaus2/dazzler/align/Path.hpp libmaus2/dazzler/align/Overlap.hpp \
	libmaus2/dazzler/align/TraceBlock.hpp libmaus2/dazzler/align/TraceBlockVectorCache.hpp libmaus2/dazzler/align/ApproximateRun.hpp \
	libmaus2/dazzler/align/ApproximateRunPartContainer.hpp libmaus2/dazzler/align/SortingOverlapOutputBuffer.hpp \
	libmaus2/dazzler/align/OverlapHeapComparator.hpp libmaus2/dazzler/align/SortingOverlapBlockInput.hpp \
	libmaus2/dazzler/align/SortingOverlapOutputBufferMerger.hpp libmaus2/dazzler/align/AlignmentSplitWriter.hpp \
	libmaus2/dazzler/align/OverlapMeta.hpp libmaus2/dazzler/align/OverlapIndexerBase.hpp \
	libmaus2/dazzler/align/OverlapIndexer.hpp libmaus2/dazzler/align/AlignmentWriter.hpp \
	libmaus2/dazzler/align/OverlapMetaIteratorGet.hpp libmaus2/dazzler/align/AlignmentFileRegion.hpp \
	libmaus2/dazzler/align/CompactRunPartContainer.hpp \
	libmaus2/dazzler/align/Tandem.hpp libmaus2/dazzler/align/TandemVector.hpp \
	libmaus2/dazzler/align/TandemVectorPartContainer.hpp \
	libmaus2/dazzler/align/TracePoint.hpp libmaus2/dazzler/align/OverlapParser.hpp \
	libmaus2/dazzler/align/OverlapData.hpp libmaus2/dazzler/align/SimpleOverlapParser.hpp \
	libmaus2/dazzler/align/SimpleOverlapVectorParser.hpp \
	libmaus2/dazzler/align/LASToBamConverterBase.hpp \
	libmaus2/dazzler/align/RefMapEntry.hpp \
	libmaus2/dazzler/align/RefMapEntryVector.hpp

libmaus2autoarray_includedir=$(includedir)/libmaus2/autoarray/
libmaus2autoarray_include_HEADERS = libmaus2/autoarray/AutoArray.hpp libmaus2/autoarray/AutoArray2d.hpp libmaus2/autoarray/AutoArrayMultiMap.hpp

libmaus2index_includedir=$(includedir)/libmaus2/index/
libmaus2index_include_HEADERS = libmaus2/index/ExternalMemoryIndexGenerator.hpp \
	libmaus2/index/ExternalMemoryIndexRecord.hpp \
	libmaus2/index/ExternalMemoryIndexDecoder.hpp \
	libmaus2/index/ExternalMemoryIndexDecoderFindLargestSmallerResult.hpp

libmaus2gamma_includedir=$(includedir)/libmaus2/gamma/
libmaus2gamma_include_HEADERS = libmaus2/gamma/GammaEncoder.hpp libmaus2/gamma/GammaDecoder.hpp \
	libmaus2/gamma/GammaGapEncoder.hpp libmaus2/gamma/GammaGapDecoder.hpp \
	libmaus2/gamma/GammaRLDecoder.hpp  libmaus2/gamma/GammaRLEncoder.hpp \
	libmaus2/gamma/SparseGammaGapEncoder.hpp libmaus2/gamma/SparseGammaGapDecoder.hpp libmaus2/gamma/SparseGammaGapMerge.hpp \
	libmaus2/gamma/SparseGammaGapFile.hpp \
	libmaus2/gamma/SparseGammaGapMultiFile.hpp \
	libmaus2/gamma/SparseGammaGapFileSet.hpp \
	libmaus2/gamma/SparseGammaGapFileLevelSet.hpp \
	libmaus2/gamma/SparseGammaGapFileIndexDecoderEntry.hpp \
	libmaus2/gamma/SparseGammaGapFileIndexDecoder.hpp \
	libmaus2/gamma/SparseGammaGapFileIndexMultiDecoder.hpp \
	libmaus2/gamma/SparseGammaGapConcatDecoder.hpp \
	libmaus2/gamma/SparseGammaGapMultiFileSet.hpp \
	libmaus2/gamma/SparseGammaGapMultiFileLevelSet.hpp \
	libmaus2/gamma/SparseGammaGapBlockEncoder.hpp \
	libmaus2/gamma/GammaDifferenceEncoder.hpp \
	libmaus2/gamma/GammaDifferenceDecoder.hpp \
	libmaus2/gamma/GammaDifferenceMerge.hpp \
	libmaus2/gamma/GammaDifferenceOutputBuffer.hpp \
	libmaus2/gamma/GammaEncoderBase.hpp \
	libmaus2/gamma/SimpleGammaEncoderData.hpp \
	libmaus2/gamma/SimpleGammaEncoder.hpp \
	libmaus2/gamma/GammaDecoderBase.hpp \
	libmaus2/gamma/SimpleGammaDecoderData.hpp \
	libmaus2/gamma/SimpleGammaDecoder.hpp \
	libmaus2/gamma/SimpleGammaEncoderArray.hpp \
	libmaus2/gamma/GammaPartitionEncoder.hpp \
	libmaus2/gamma/GammaIntervalEncoder.hpp \
	libmaus2/gamma/GammaPartitionDecoder.hpp \
	libmaus2/gamma/GammaIntervalDecoder.hpp \
	libmaus2/gamma/GammaPDEncoder.hpp \
	libmaus2/gamma/GammaPDDecoder.hpp \
	libmaus2/gamma/GammaPDIndexDecoderBase.hpp \
	libmaus2/gamma/GammaPDIndexDecoder.hpp \
	libmaus2/gamma/Interval.hpp \
	libmaus2/gamma/FlaggedInterval.hpp \
	libmaus2/gamma/GammaFlaggedPartitionDecoder.hpp \
	libmaus2/gamma/GammaFlaggedPartitionEncoder.hpp \
	libmaus2/gamma/GammaFlaggedIntervalDecoder.hpp \
	libmaus2/gamma/GammaFlaggedIntervalEncoder.hpp

libmaus2regex_includedir=$(includedir)/libmaus2/regex/
libmaus2regex_include_HEADERS = libmaus2/regex/PosixRegex.hpp

libmaus2serialize_includedir=$(includedir)/libmaus2/serialize/
libmaus2serialize_include_HEADERS = libmaus2/serialize/Serialize.hpp

libmaus2types_includedir=$(includedir)/libmaus2/types/
libmaus2types_include_HEADERS = libmaus2/types/types.hpp

libmaus2lz_includedir=$(includedir)/libmaus2/lz/
libmaus2lz_include_HEADERS = libmaus2/lz/Inflate.hpp libmaus2/lz/Deflate.hpp libmaus2/lz/GzipHeader.hpp libmaus2/lz/StreamWrapper.hpp \
	libmaus2/lz/GzipSingleStream.hpp libmaus2/lz/GzipStream.hpp libmaus2/lz/GzipStreamWrapper.hpp \
	libmaus2/lz/BufferedGzipStream.hpp libmaus2/lz/StdOstreamSink.hpp libmaus2/lz/IstreamSource.hpp \
	libmaus2/lz/SnappyCompress.hpp libmaus2/lz/BgzfInflate.hpp libmaus2/lz/BgzfInflateWrapper.hpp \
	libmaus2/lz/BgzfInflateStream.hpp \
	libmaus2/lz/BgzfDeflate.hpp \
	libmaus2/lz/BgzfInflateBase.hpp \
	libmaus2/lz/BgzfInflateBlock.hpp \
	libmaus2/lz/BgzfInflateParallel.hpp \
	libmaus2/lz/BgzfInflateParallelWrapper.hpp \
	libmaus2/lz/BgzfInflateParallelStream.hpp \
	libmaus2/lz/BgzfDeflateBase.hpp \
	libmaus2/lz/BgzfInflateBlockIdComparator.hpp \
	libmaus2/lz/BgzfInflateBlockIdInfo.hpp \
	libmaus2/lz/BgzfInflateParallelContext.hpp \
	libmaus2/lz/BgzfInflateParallelThread.hpp \
	libmaus2/lz/BgzfDeflateBlockIdInfo.hpp \
	libmaus2/lz/BgzfDeflateBlockIdComparator.hpp \
	libmaus2/lz/BgzfDeflateParallelContext.hpp \
	libmaus2/lz/BgzfDeflateParallelThread.hpp \
	libmaus2/lz/BgzfConstants.hpp \
	libmaus2/lz/BgzfDeflateHeaderFunctions.hpp \
	libmaus2/lz/BgzfDeflateInputBufferBase.hpp \
	libmaus2/lz/BgzfDeflateOutputBufferBase.hpp \
	libmaus2/lz/BgzfDeflateZStreamBase.hpp \
	libmaus2/lz/BgzfInflateHeaderBase.hpp \
	libmaus2/lz/BgzfInflateZStreamBase.hpp \
	libmaus2/lz/BgzfRecode.hpp \
	libmaus2/lz/BgzfThreadOpBase.hpp \
	libmaus2/lz/BgzfInflateDeflateParallel.hpp \
	libmaus2/lz/BgzfInflateDeflateParallelThread.hpp \
	libmaus2/lz/BgzfDeflateParallel.hpp \
	libmaus2/lz/BgzfVirtualOffset.hpp \
	libmaus2/lz/BgzfInflateDeflateParallelWrapper.hpp \
	libmaus2/lz/BgzfInflateDeflateParallelInputStream.hpp \
	libmaus2/lz/BgzfDeflateOutputCallback.hpp \
	libmaus2/lz/BgzfDeflateOutputCallbackMD5.hpp \
	libmaus2/lz/BgzfParallelRecodeDeflateBase.hpp \
	libmaus2/lz/BgzfRecodeParallel.hpp \
	libmaus2/lz/BgzfDeflateZStreamBaseFlushInfo.hpp \
	libmaus2/lz/Lz4Base.hpp \
	libmaus2/lz/Lz4CompressStreamBuffer.hpp \
	libmaus2/lz/Lz4CompressWrapper.hpp \
	libmaus2/lz/Lz4Decoder.hpp \
	libmaus2/lz/Lz4Index.hpp \
	libmaus2/lz/Lz4Compress.hpp \
	libmaus2/lz/Lz4CompressStream.hpp \
	libmaus2/lz/Lz4DecoderBuffer.hpp \
	libmaus2/lz/GzipOutputStreamBuffer.hpp \
	libmaus2/lz/GzipOutputStream.hpp \
	libmaus2/lz/BgzfInflateInfo.hpp \
	libmaus2/lz/BgzfStreamWrapper.hpp \
	libmaus2/lz/CompressorObject.hpp \
	libmaus2/lz/CompressorObjectFactory.hpp \
	libmaus2/lz/SimpleCompressedOutputStream.hpp \
	libmaus2/lz/SnappyCompressorObject.hpp \
	libmaus2/lz/SnappyCompressorObjectFactory.hpp \
	libmaus2/lz/DecompressorObject.hpp \
	libmaus2/lz/DecompressorObjectFactory.hpp \
	libmaus2/lz/SnappyDecompressorObject.hpp \
	libmaus2/lz/SnappyDecompressorObjectFactory.hpp \
	libmaus2/lz/SimpleCompressedInputStream.hpp \
	libmaus2/lz/SimpleCompressedConcatInputStreamFragment.hpp \
	libmaus2/lz/SimpleCompressedConcatInputStream.hpp \
	libmaus2/lz/SnappyInputStreamArrayFile.hpp \
	libmaus2/lz/SnappyInputStreamArray.hpp \
	libmaus2/lz/SnappyStringInputStream.hpp \
	libmaus2/lz/SnappyOffsetFileInputStream.hpp \
	libmaus2/lz/SnappyFileInputStream.hpp \
	libmaus2/lz/SnappyInputStream.hpp \
	libmaus2/lz/SnappyFileOutputStream.hpp \
	libmaus2/lz/SnappyOutputStream.hpp \
	libmaus2/lz/SimpleCompressedStreamInterval.hpp \
	libmaus2/lz/ZlibCompressorObjectFactory.hpp \
	libmaus2/lz/ZlibDecompressorObjectFactory.hpp \
	libmaus2/lz/ZlibCompressorObject.hpp \
	libmaus2/lz/ZlibDecompressorObject.hpp \
	libmaus2/lz/SimpleCompressedStreamNamedInterval.hpp \
	libmaus2/lz/SimpleCompressedInputBlock.hpp \
	libmaus2/lz/SimpleCompressedInputBlockConcat.hpp \
	libmaus2/lz/SimpleCompressedInputBlockConcatBlock.hpp \
	libmaus2/lz/RAZFConstants.hpp \
	libmaus2/lz/RAZFDecoderBuffer.hpp \
	libmaus2/lz/RAZFDecoder.hpp \
	libmaus2/lz/RAZFIndex.hpp \
	libmaus2/lz/IGzipDeflate.hpp \
	libmaus2/lz/IsGzip.hpp \
	libmaus2/lz/BufferedGzipStreamWrapper.hpp \
	libmaus2/lz/StreamWrapperBuffer.hpp \
	libmaus2/lz/BufferedGzipStreamBuffer.hpp \
	libmaus2/lz/PlainOrGzipStreamBufferWrapper.hpp \
	libmaus2/lz/PlainOrGzipStream.hpp \
	libmaus2/lz/LineSplittingGzipOutputStreamBuffer.hpp \
	libmaus2/lz/LineSplittingGzipOutputStream.hpp \
	libmaus2/lz/CompressorObjectFreeListAllocator.hpp \
	libmaus2/lz/SnappyCompressorObjectFreeListAllocator.hpp \
	libmaus2/lz/SnappyCompressorObjectFactoryWrapper.hpp \
	libmaus2/lz/ZlibCompressorObjectFactoryWrapper.hpp \
	libmaus2/lz/ZlibCompressorObjectFreeListAllocator.hpp \
	libmaus2/lz/CompressorObjectFreeListAllocatorFactory.hpp \
	libmaus2/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp \
	libmaus2/lz/BgzfDeflateOutputBufferBaseAllocator.hpp \
	libmaus2/lz/BgzfDeflateZStreamBaseAllocator.hpp \
	libmaus2/lz/BgzfDeflateZStreamBaseTypeInfo.hpp \
	libmaus2/lz/BgzfInflateZStreamBaseAllocator.hpp \
	libmaus2/lz/BgzfInflateZStreamBaseTypeInfo.hpp \
	libmaus2/lz/XzDecoder.hpp \
	libmaus2/lz/XzInputStreamBuffer.hpp \
	libmaus2/lz/XzInputStream.hpp \
	libmaus2/lz/BlockInflate.hpp \
	libmaus2/lz/ConcatBlockInflate.hpp \
	libmaus2/lz/ZlibInterface.hpp

libmaus2bambam_includedir=$(includedir)/libmaus2/bambam/
libmaus2bambam_include_HEADERS = libmaus2/bambam/DecoderBase.hpp libmaus2/bambam/Chromosome.hpp \
	libmaus2/bambam/BamAlignmentDecoderBase.hpp \
	libmaus2/bambam/BamFormatAuxiliary.hpp \
	libmaus2/bambam/EncoderBase.hpp \
	libmaus2/bambam/BamHeader.hpp \
	libmaus2/bambam/BamAlignment.hpp \
	libmaus2/bambam/BamDecoder.hpp \
	libmaus2/bambam/BamAlignmentComparator.hpp \
	libmaus2/bambam/MergeQueueElement.hpp \
	libmaus2/bambam/CollatingBamDecoder.hpp \
	libmaus2/bambam/CigarOperation.hpp \
	libmaus2/bambam/CigarStringParser.hpp \
	libmaus2/bambam/BamSeqEncodeTable.hpp \
	libmaus2/bambam/BamAlignmentEncoderBase.hpp \
	libmaus2/bambam/BamFlagBase.hpp \
	libmaus2/bambam/BamWriter.hpp \
	libmaus2/bambam/HeaderLine.hpp \
	libmaus2/bambam/ProgramHeaderLineSet.hpp \
	libmaus2/bambam/ReadEnds.hpp \
	libmaus2/bambam/ReadEndsHeapPairComparator.hpp \
	libmaus2/bambam/CompactReadEndsBase.hpp \
	libmaus2/bambam/CompactReadEndsComparator.hpp \
	libmaus2/bambam/SortedFragDecoder.hpp \
	libmaus2/bambam/ReadEndsContainer.hpp \
	libmaus2/bambam/DuplicationMetrics.hpp \
	libmaus2/bambam/OpticalComparator.hpp \
	libmaus2/bambam/BamAlignmentHeapComparator.hpp \
	libmaus2/bambam/BamAlignmentNameComparator.hpp \
	libmaus2/bambam/BamAlignmentPosComparator.hpp \
	libmaus2/bambam/BamEntryContainer.hpp \
	libmaus2/bambam/Scram.h \
	libmaus2/bambam/AlignmentValidity.hpp \
	libmaus2/bambam/ScramDecoder.hpp \
	libmaus2/bambam/ScramEncoder.hpp \
	libmaus2/bambam/SnappyAlignmentMergeInput.hpp \
	libmaus2/bambam/BamAlignmentPrinter.hpp \
	libmaus2/bambam/BamAlignmentSortingCircularHashEntryOverflow.hpp \
	libmaus2/bambam/CircularHashCollatingBamDecoder.hpp \
	libmaus2/bambam/BamToFastqOutputFileSet.hpp \
	libmaus2/bambam/BamAlignmentDecoder.hpp \
	libmaus2/bambam/CollatingBamDecoderAlignmentInputCallback.hpp \
	libmaus2/bambam/ReadGroup.hpp \
	libmaus2/bambam/BamAuxFilterVector.hpp \
	libmaus2/bambam/BamDefaultAdapters.hpp \
	libmaus2/bambam/AdapterOffsetStrand.hpp \
	libmaus2/bambam/AdapterFilter.hpp \
	libmaus2/bambam/BamIndexBin.hpp \
	libmaus2/bambam/BamIndexLinear.hpp \
	libmaus2/bambam/BamIndexRef.hpp \
	libmaus2/bambam/BamIndex.hpp \
	libmaus2/bambam/BamRange.hpp \
	libmaus2/bambam/BamRangeParser.hpp \
	libmaus2/bambam/BamRangeDecoder.hpp \
	libmaus2/bambam/BamParallelRewrite.hpp \
	libmaus2/bambam/BamAlignmentExpungeCallback.hpp \
	libmaus2/bambam/StrCmpNum.hpp \
	libmaus2/bambam/BamCatHeader.hpp \
	libmaus2/bambam/BamCat.hpp \
	libmaus2/bambam/BamMergeCoordinate.hpp \
	libmaus2/bambam/BamMergeTemplate.hpp \
	libmaus2/bambam/BamMergeQueryName.hpp \
	libmaus2/bambam/BamIndexBinChunk.hpp \
	libmaus2/bambam/BamIndexLinearChunk.hpp \
	libmaus2/bambam/BamIndexGenerator.hpp \
	libmaus2/bambam/BgzfDeflateOutputCallbackBamIndex.hpp \
	libmaus2/bambam/BamAlignmentInputCallbackBam.hpp \
	libmaus2/bambam/BamAlignmentInputCallbackSnappy.hpp \
	libmaus2/bambam/BamHeaderUpdate.hpp \
	libmaus2/bambam/DupSetCallback.hpp \
	libmaus2/bambam/DupSetCallbackSet.hpp \
	libmaus2/bambam/DupSetCallbackStream.hpp \
	libmaus2/bambam/DupSetCallbackVector.hpp \
	libmaus2/bambam/BamAlignmentListNode.hpp \
	libmaus2/bambam/BamAlignmentPairListNode.hpp \
	libmaus2/bambam/ReadEndsListNode.hpp \
	libmaus2/bambam/BamAlignmentFreeList.hpp \
	libmaus2/bambam/BamAlignmentPairFreeList.hpp \
	libmaus2/bambam/ReadEndsFreeList.hpp \
	libmaus2/bambam/DupMarkBase.hpp \
	libmaus2/bambam/BamAlignmentSnappyInput.hpp \
	libmaus2/bambam/BamAlignmentInputPositionUpdateCallback.hpp \
	libmaus2/bambam/BamAlignmentInputPositionCallbackNull.hpp \
	libmaus2/bambam/BamBlockWriterBase.hpp \
	libmaus2/bambam/BamBlockWriterBaseFactory.hpp \
	libmaus2/bambam/BamAlignmentDecoderInfo.hpp \
	libmaus2/bambam/BamAlignmentDecoderFactory.hpp \
	libmaus2/bambam/BamMultiAlignmentDecoderFactory.hpp \
	libmaus2/bambam/BamIndexMetaInfo.hpp \
	libmaus2/bambam/MdStringComputationContext.hpp \
	libmaus2/bambam/BamAlignmentFilter.hpp \
	libmaus2/bambam/BamAlignmentReadGroupFilter.hpp \
	libmaus2/bambam/MdNmRecalculation.hpp \
	libmaus2/bambam/CramRange.hpp \
	libmaus2/bambam/ChromosomeVectorMerge.hpp \
	libmaus2/bambam/BamHeaderLowMem.hpp \
	libmaus2/bambam/GeneFlatFile.hpp \
	libmaus2/bambam/GeneFlatFileEntry.hpp \
	libmaus2/bambam/BamStreamingMarkDuplicatesSupport.hpp \
	libmaus2/bambam/BamStreamingMarkDuplicates.hpp \
	libmaus2/bambam/BamHeaderParserStateBase.hpp \
	libmaus2/bambam/BamHeaderParserState.hpp \
	libmaus2/bambam/BamAlignmentReg2Bin.hpp \
	libmaus2/bambam/SamInfoBase.hpp \
	libmaus2/bambam/SamInfo.hpp \
	libmaus2/bambam/SamPrintableTable.hpp \
	libmaus2/bambam/SamZPrintableTable.hpp \
	libmaus2/bambam/SamDecoder.hpp \
	libmaus2/bambam/SamDecoderWrapper.hpp \
	libmaus2/bambam/BamAlignmentFixedSizeData.hpp \
	libmaus2/bambam/BamAuxSortingBufferEntry.hpp \
	libmaus2/bambam/BamAuxSortingBuffer.hpp \
	libmaus2/bambam/ReadEndsContainerBase.hpp \
	libmaus2/bambam/ReadEndsBlockDecoderBase.hpp \
	libmaus2/bambam/ReadEndsBlockDecoderBaseCollection.hpp \
	libmaus2/bambam/ReadEndsStreamDecoderBase.hpp \
	libmaus2/bambam/ReadEndsStreamDecoderFileBase.hpp \
	libmaus2/bambam/ReadEndsStreamDecoder.hpp \
	libmaus2/bambam/ReadEndsBasePointerComparator.hpp \
	libmaus2/bambam/ReadEndsBase.hpp \
	libmaus2/bambam/ReadEndsBaseLongHashAttributeComparator.hpp \
	libmaus2/bambam/ReadEndsBaseShortHashAttributeComparator.hpp \
	libmaus2/bambam/ScramInputContainer.hpp \
	libmaus2/bambam/SamEncoder.hpp \
	libmaus2/bambam/ReadEndsBlockDecoderBaseCollectionInfoBase.hpp \
	libmaus2/bambam/ReadEndsBlockDecoderBaseCollectionInfo.hpp \
	libmaus2/bambam/ReadEndsBlockIndexSet.hpp \
	libmaus2/bambam/DupSetCallbackSharedVector.hpp \
	libmaus2/bambam/SamInfoAllocator.hpp \
	libmaus2/bambam/SamInfoTypeInfo.hpp \
	libmaus2/bambam/RgInfo.hpp \
	libmaus2/bambam/SeqChksumPrimeNumbers.hpp \
	libmaus2/bambam/SeqChksumUpdateContext.hpp \
	libmaus2/bambam/ChecksumsInterface.hpp \
	libmaus2/bambam/ChecksumsFactory.hpp \
	libmaus2/bambam/ChecksumsInterfaceAllocator.hpp \
	libmaus2/bambam/ChecksumsInterfaceTypeInfo.hpp \
	libmaus2/bambam/StrCmpNumRecode.hpp \
	libmaus2/bambam/PileVectorElement.hpp \
	libmaus2/bambam/CigarStringReader.hpp \
	libmaus2/bambam/BamAlignmentHashComparator.hpp

libmaus2bambamparallel_includedir=$(includedir)/libmaus2/bambam/parallel/
libmaus2bambamparallel_include_HEADERS = \
	libmaus2/bambam/parallel/AddPendingCompressBufferWriteInterface.hpp \
	libmaus2/bambam/parallel/AddWritePendingBgzfBlockInterface.hpp \
	libmaus2/bambam/parallel/AlignmentBlockCompressPackage.hpp \
	libmaus2/bambam/parallel/AlignmentBlockCompressPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/AlignmentBufferAllocator.hpp \
	libmaus2/bambam/parallel/AlignmentBufferHeapComparator.hpp \
	libmaus2/bambam/parallel/AlignmentBuffer.hpp \
	libmaus2/bambam/parallel/AlignmentBufferReinsertInterface.hpp \
	libmaus2/bambam/parallel/AlignmentBufferReturnInterface.hpp \
	libmaus2/bambam/parallel/AlignmentBufferTypeInfo.hpp \
	libmaus2/bambam/parallel/AlignmentRewriteBufferAddPendingInterface.hpp \
	libmaus2/bambam/parallel/AlignmentRewriteBufferAllocator.hpp \
	libmaus2/bambam/parallel/AlignmentRewriteBufferCoordinateComparator.hpp \
	libmaus2/bambam/parallel/AlignmentRewriteBuffer.hpp \
	libmaus2/bambam/parallel/AlignmentRewriteBufferPosComparator.hpp \
	libmaus2/bambam/parallel/AlignmentRewriteBufferReinsertForFillingInterface.hpp \
	libmaus2/bambam/parallel/AlignmentRewritePosMergeSortPackage.hpp \
	libmaus2/bambam/parallel/AlignmentRewritePosSortBaseSortPackage.hpp \
	libmaus2/bambam/parallel/AlignmentRewritePosSortContextBaseBlockSortedInterface.hpp \
	libmaus2/bambam/parallel/AlignmentRewritePosSortContext.hpp \
	libmaus2/bambam/parallel/AlignmentRewritePosSortContextMergePackageFinished.hpp \
	libmaus2/bambam/parallel/BaseMergeSortWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/BaseSortWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/BaseSortWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/BgzfInflateZStreamBaseGetInterface.hpp \
	libmaus2/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp \
	libmaus2/bambam/parallel/BgzfLinearMemCompressWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/BgzfLinearMemCompressWorkPackage.hpp \
	libmaus2/bambam/parallel/BgzfLinearMemCompressWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/BgzfOutputBlockWrittenInterface.hpp \
	libmaus2/bambam/parallel/BlockCompressPackageDispatcher.hpp \
	libmaus2/bambam/parallel/CompressBufferAllocator.hpp \
	libmaus2/bambam/parallel/CompressBufferHeapComparator.hpp \
	libmaus2/bambam/parallel/CompressBuffer.hpp \
	libmaus2/bambam/parallel/CompressionPendingElement.hpp \
	libmaus2/bambam/parallel/ControlInputInfo.hpp \
	libmaus2/bambam/parallel/DecompressBlocksWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/DecompressBlocksWorkPackage.hpp \
	libmaus2/bambam/parallel/DecompressBlocksWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/DecompressBlockWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/DecompressBlockWorkPackage.hpp \
	libmaus2/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/DecompressedBlockAddPendingInterface.hpp \
	libmaus2/bambam/parallel/DecompressedBlockAllocator.hpp \
	libmaus2/bambam/parallel/DecompressedBlock.hpp \
	libmaus2/bambam/parallel/DecompressedBlockReturnInterface.hpp \
	libmaus2/bambam/parallel/DecompressedBlockTypeInfo.hpp \
	libmaus2/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp \
	libmaus2/bambam/parallel/DecompressedPendingObject.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferAllocator.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferBaseSortPackage.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferFragment.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferHeapComparator.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBuffer.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferMergeSortPackage.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferPointerTypeSwitchHelper.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferPointerTypeSwitch.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteFragmentCompleteInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackage.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferSortContextBaseBlockSortedInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferSortContext.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferSortContextMergePackageFinished.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferSortFinishedInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferTypeInfo.hpp \
	libmaus2/bambam/parallel/GetBgzfDeflateZStreamBaseInterface.hpp \
	libmaus2/bambam/parallel/InputBlockAddPendingInterface.hpp \
	libmaus2/bambam/parallel/InputBlockAllocator.hpp \
	libmaus2/bambam/parallel/InputBlock.hpp \
	libmaus2/bambam/parallel/InputBlockReturnInterface.hpp \
	libmaus2/bambam/parallel/InputBlockTypeInfo.hpp \
	libmaus2/bambam/parallel/InputBlockWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/InputBlockWorkPackage.hpp \
	libmaus2/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/MergeSortWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/ParseBlockWorkPackage.hpp \
	libmaus2/bambam/parallel/ParsedBlockAddPendingInterface.hpp \
	libmaus2/bambam/parallel/ParsedBlockStallInterface.hpp \
	libmaus2/bambam/parallel/ParseInfoHeaderCompleteCallback.hpp \
	libmaus2/bambam/parallel/ParseInfo.hpp \
	libmaus2/bambam/parallel/ParsePackageReturnInterface.hpp \
	libmaus2/bambam/parallel/PushBackSpace.hpp \
	libmaus2/bambam/parallel/PutBgzfDeflateZStreamBaseInterface.hpp \
	libmaus2/bambam/parallel/RequeReadInterface.hpp \
	libmaus2/bambam/parallel/ReturnBgzfOutputBufferInterface.hpp \
	libmaus2/bambam/parallel/ReturnCompressionPendingElementInterface.hpp \
	libmaus2/bambam/parallel/ReturnRewriteBufferInterface.hpp \
	libmaus2/bambam/parallel/RewriteBlockWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/RewriteBlockWorkPackage.hpp \
	libmaus2/bambam/parallel/RewritePackageReturnInterface.hpp \
	libmaus2/bambam/parallel/SmallLinearBlockCompressionPendingObjectFinishedInterface.hpp \
	libmaus2/bambam/parallel/SmallLinearBlockCompressionPendingObjectHeapComparator.hpp \
	libmaus2/bambam/parallel/SmallLinearBlockCompressionPendingObject.hpp \
	libmaus2/bambam/parallel/SortControl.hpp \
	libmaus2/bambam/parallel/SortFinishedInterface.hpp \
	libmaus2/bambam/parallel/ValidateBlockAddPendingInterface.hpp \
	libmaus2/bambam/parallel/ValidateBlockFragmentAddPendingInterface.hpp \
	libmaus2/bambam/parallel/ValidateBlockFragmentPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/ValidateBlockFragmentWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/ValidateBlockFragmentWorkPackage.hpp \
	libmaus2/bambam/parallel/ValidateBlockWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/ValidateBlockWorkPackage.hpp \
	libmaus2/bambam/parallel/ValidatePackageReturnInterface.hpp \
	libmaus2/bambam/parallel/ValidationFragment.hpp \
	libmaus2/bambam/parallel/WriteBlockWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/WriteBlockWorkPackage.hpp \
	libmaus2/bambam/parallel/WriteBlockWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/WritePendingObjectHeapComparator.hpp \
	libmaus2/bambam/parallel/WritePendingObject.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferBaseSortWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferReorderWorkPackage.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageFinishedInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferNameComparator.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteUpdateInterval.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackage.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/ReadEndsContainerAllocator.hpp \
	libmaus2/bambam/parallel/ReadEndsContainerTypeInfo.hpp \
	libmaus2/bambam/parallel/ReadEndsContainerFreeListInterface.hpp \
	libmaus2/bambam/parallel/AddDuplicationMetricsInterface.hpp \
	libmaus2/bambam/parallel/PairReadEndsContainerFlushWorkPackage.hpp \
	libmaus2/bambam/parallel/FragReadEndsContainerFlushWorkPackage.hpp \
	libmaus2/bambam/parallel/FragReadEndsContainerFlushWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FragReadEndsContainerFlushFinishedInterface.hpp \
	libmaus2/bambam/parallel/FragReadEndsContainerFlushWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/PairReadEndsContainerFlushWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/PairReadEndsContainerFlushFinishedInterface.hpp \
	libmaus2/bambam/parallel/ReadEndsMergeRequest.hpp \
	libmaus2/bambam/parallel/FragReadEndsMergeWorkPackage.hpp \
	libmaus2/bambam/parallel/FragReadEndsMergeWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FragReadEndsMergeWorkPackageFinishedInterface.hpp \
	libmaus2/bambam/parallel/FragReadEndsMergeWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/PairReadEndsMergeWorkPackage.hpp \
	libmaus2/bambam/parallel/PairReadEndsMergeWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/PairReadEndsMergeWorkPackageFinishedInterface.hpp \
	libmaus2/bambam/parallel/PairReadEndsMergeWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/MemInputBlock.hpp \
	libmaus2/bambam/parallel/GenericInputBamParseObject.hpp \
	libmaus2/bambam/parallel/GenericInputBamParseWorkPackageBlockParsedInterface.hpp \
	libmaus2/bambam/parallel/GenericInputBamParseWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputBamParseWorkPackage.hpp \
	libmaus2/bambam/parallel/GenericInputBamParseWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputBase.hpp \
	libmaus2/bambam/parallel/GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputBgzfDecompressionWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputBgzfDecompressionWorkPackage.hpp \
	libmaus2/bambam/parallel/GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputBgzfDecompressionWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface.hpp \
	libmaus2/bambam/parallel/GenericInputBlock.hpp \
	libmaus2/bambam/parallel/GenericInputControlMergeBlockFinishedInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlMergeHeapEntry.hpp \
	libmaus2/bambam/parallel/GenericInputControlMergeRequeue.hpp \
	libmaus2/bambam/parallel/GenericInputControlReadAddPendingInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlReadWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputControlReadWorkPackage.hpp \
	libmaus2/bambam/parallel/GenericInputControlReadWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlSetMergeStallSlotInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlStreamInfo.hpp \
	libmaus2/bambam/parallel/GenericInputControlSubBlockPendingHeapComparator.hpp \
	libmaus2/bambam/parallel/GenericInputControlSubBlockPending.hpp \
	libmaus2/bambam/parallel/GenericInputHeapComparator.hpp \
	libmaus2/bambam/parallel/GenericInputMergeDecompressedBlockReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputMergeWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputMergeWorkPackage.hpp \
	libmaus2/bambam/parallel/GenericInputMergeWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputMergeWorkRequest.hpp \
	libmaus2/bambam/parallel/GenericInputSingleDataBamParseInfo.hpp \
	libmaus2/bambam/parallel/GenericInputSingleData.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockCompressionFinishedInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockCompressionWorkPackage.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockCompressionWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockCompressionWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlCompressionPending.hpp \
	libmaus2/bambam/parallel/GenericInputControlCompressionPendingHeapComparator.hpp \
	libmaus2/bambam/parallel/GenericInputControlGetCompressorInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlPutCompressorInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlReorderWorkPackage.hpp \
	libmaus2/bambam/parallel/GenericInputControlReorderWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputControlReorderWorkPackageFinishedInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlReorderWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockWritePackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockWritePackageBlockWrittenInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockWritePackageReturnInterface.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlockWritePackage.hpp \
	libmaus2/bambam/parallel/GenericInputBlockSubBlockInfo.hpp \
	libmaus2/bambam/parallel/GenericInputBlockFillResult.hpp \
	libmaus2/bambam/parallel/GenericInputBlockTypeInfo.hpp \
	libmaus2/bambam/parallel/GenericInputBlockAllocator.hpp \
	libmaus2/bambam/parallel/DecompressedBlockHeapComparator.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferBaseSortWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferMergeSortWorkPackage.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferMergeSortWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/PairReadEndsContainerFlushWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferMergeSortWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferPosComparator.hpp \
	libmaus2/bambam/parallel/SamParsePending.hpp \
	libmaus2/bambam/parallel/SamParsePendingHeapComparator.hpp \
	libmaus2/bambam/parallel/SamParseWorkPackage.hpp \
	libmaus2/bambam/parallel/BlockSortControlBase.hpp \
	libmaus2/bambam/parallel/SamParsePutSamInfoInterface.hpp \
	libmaus2/bambam/parallel/SamParseGetSamInfoInterface.hpp \
	libmaus2/bambam/parallel/SamParseFragmentParsedInterface.hpp \
	libmaus2/bambam/parallel/SamParseDecompressedBlockFinishedInterface.hpp \
	libmaus2/bambam/parallel/SamParseWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/SamParseWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/BlockSortControl.hpp \
	libmaus2/bambam/parallel/BlockMergeControl.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlocksWritePackage.hpp \
	libmaus2/bambam/parallel/FastQInputDescBase.hpp \
	libmaus2/bambam/parallel/FastqToBamControlInputBlockHeapComparator.hpp \
	libmaus2/bambam/parallel/FastqToBamControlSubReadPending.hpp \
	libmaus2/bambam/parallel/FastqToBamControlSubReadPendingHeapComparator.hpp \
	libmaus2/bambam/parallel/DecompressedBlockReorderHeapComparator.hpp \
	libmaus2/bambam/parallel/FastQInputDesc.hpp \
	libmaus2/bambam/parallel/FastqInputPackage.hpp \
	libmaus2/bambam/parallel/FastqInputPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FastqInputPackageAddPendingInterface.hpp \
	libmaus2/bambam/parallel/FastqInputPackageDispatcher.hpp \
	libmaus2/bambam/parallel/FastqParsePackage.hpp \
	libmaus2/bambam/parallel/FastqParsePackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FastqParsePackageFinishedInterface.hpp \
	libmaus2/bambam/parallel/FastqParsePackageDispatcher.hpp \
	libmaus2/bambam/parallel/FastqToBamControl.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlocksWritePackageDispatcher.hpp \
	libmaus2/bambam/parallel/GenericInputControlBlocksWritePackageReturnInterface.hpp \
	libmaus2/bambam/parallel/ChecksumsInterfaceGetInterface.hpp \
	libmaus2/bambam/parallel/ChecksumsInterfacePutInterface.hpp \
	libmaus2/bambam/parallel/FileChecksumBlockWorkPackage.hpp \
	libmaus2/bambam/parallel/FileChecksumBlockWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/FileChecksumBlockFinishedInterface.hpp \
	libmaus2/bambam/parallel/FileChecksumBlockWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/BamBlockIndexingWorkPackage.hpp \
	libmaus2/bambam/parallel/BamBlockIndexingWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/BamBlockIndexingBlockFinishedInterface.hpp \
	libmaus2/bambam/parallel/BamBlockIndexingWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/CramInterface.h \
	libmaus2/bambam/parallel/CramOutputBlock.hpp \
	libmaus2/bambam/parallel/CramOutputBlockSizeComparator.hpp \
	libmaus2/bambam/parallel/SamEncoderObject.hpp \
	libmaus2/bambam/parallel/CramOutputBlockWritePackage.hpp \
	libmaus2/bambam/parallel/CramOutputBlockWritePackageReturnInterface.hpp \
	libmaus2/bambam/parallel/CramOutputBlockWritePackageFinishedInterface.hpp \
	libmaus2/bambam/parallel/CramOutputBlockWritePackageDispatcher.hpp \
	libmaus2/bambam/parallel/CramPassPointerObject.hpp \
	libmaus2/bambam/parallel/CramPassPointerObjectComparator.hpp \
	libmaus2/bambam/parallel/CramPassPointerObjectTypeInfo.hpp \
	libmaus2/bambam/parallel/CramPassPointerObjectAllocator.hpp \
	libmaus2/bambam/parallel/ScramCramEncoding.hpp \
	libmaus2/bambam/parallel/CramOutputBlockIdComparator.hpp \
	libmaus2/bambam/parallel/FragmentAlignmentBufferQueryNameComparator.hpp \
	libmaus2/bambam/parallel/CramEncodingWorkPackage.hpp \
	libmaus2/bambam/parallel/SamEncodingWorkPackage.hpp \
	libmaus2/bambam/parallel/SamEncodingWorkPackageWrapper.hpp \
	libmaus2/bambam/parallel/CramOutputBlockChecksumPackage.hpp \
	libmaus2/bambam/parallel/CramOutputBlockChecksumPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/CramOutputBlockChecksumComputedInterface.hpp \
	libmaus2/bambam/parallel/CramOutputBlockChecksumPackageDispatcher.hpp \
	libmaus2/bambam/parallel/CramEncodingWorkPackageReturnInterface.hpp \
	libmaus2/bambam/parallel/SamEncodingWorkPackageWrapperReturnInterface.hpp \
	libmaus2/bambam/parallel/SamEncodingWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/BlockMergeControlTypeBase.hpp \
	libmaus2/bambam/parallel/CramEncodingSupportData.hpp \
	libmaus2/bambam/parallel/CramEncodingWorkPackageDispatcher.hpp \
	libmaus2/bambam/parallel/RefIdInterval.hpp

libmaus2bitio_includedir=$(includedir)/libmaus2/bitio
libmaus2bitio_include_HEADERS=\
	libmaus2/bitio/BitIO.hpp \
	libmaus2/bitio/BitIOOutput.hpp \
	libmaus2/bitio/CompactArray.hpp \
	libmaus2/bitio/CompactArrayBase.hpp \
	libmaus2/bitio/OutputBuffer.hpp \
	libmaus2/bitio/BitIOInput.hpp \
	libmaus2/bitio/getBits.hpp \
	libmaus2/bitio/ReverseTable.hpp \
	libmaus2/bitio/CompactQueue.hpp \
	libmaus2/bitio/putBits.hpp \
	libmaus2/bitio/SwapWordBitBlock.hpp \
	libmaus2/bitio/swapBitBlocksByReversal.hpp \
	libmaus2/bitio/putBit.hpp \
	libmaus2/bitio/getBit.hpp \
	libmaus2/bitio/readElias.hpp \
	libmaus2/bitio/BitWriter.hpp \
	libmaus2/bitio/CheckedBitWriter.hpp \
	libmaus2/bitio/FastWriteBitWriter.hpp \
	libmaus2/bitio/BitVector.hpp \
	libmaus2/bitio/SignedCompactArray.hpp \
	libmaus2/bitio/BufferIterator.hpp \
	libmaus2/bitio/BitVectorConcat.hpp \
	libmaus2/bitio/CompactSparseArray.hpp \
	libmaus2/bitio/CompactOffsetArray.hpp \
	libmaus2/bitio/CompactOffsetOneArray.hpp \
	libmaus2/bitio/ArrayDecode.hpp \
	libmaus2/bitio/BitStreamFileDecoder.hpp \
	libmaus2/bitio/Clz.hpp \
	libmaus2/bitio/CompactDecoderBuffer.hpp \
	libmaus2/bitio/Ctz.hpp \
	libmaus2/bitio/CompactArrayWriter.hpp \
	libmaus2/bitio/CompactArrayWriterFile.hpp \
	libmaus2/bitio/PacDecoderBuffer.hpp \
	libmaus2/bitio/MarkerFastWriteBitWriter.hpp \
	libmaus2/bitio/BitVectorInput.hpp \
	libmaus2/bitio/BitVectorOutput.hpp

libmaus2rank_includedir=$(includedir)/libmaus2/rank
libmaus2rank_include_HEADERS=\
	libmaus2/rank/ChooseCache.hpp \
	libmaus2/rank/CodeBase.hpp \
	libmaus2/rank/DecodeCache.hpp \
	libmaus2/rank/ERank222BAppend.hpp \
	libmaus2/rank/ERank222B.hpp \
	libmaus2/rank/ERank222BP.hpp \
	libmaus2/rank/ERank222.hpp \
	libmaus2/rank/ERank22B.hpp \
	libmaus2/rank/ERank22.hpp \
	libmaus2/rank/ERank2.hpp \
	libmaus2/rank/ERank3C.hpp \
	libmaus2/rank/ERank3.hpp \
	libmaus2/rank/ERankBase.hpp \
	libmaus2/rank/ERank.hpp \
	libmaus2/rank/log2table.hpp \
	libmaus2/rank/popcnt.hpp \
	libmaus2/rank/RankTable.hpp \
	libmaus2/rank/Proc01.hpp \
	libmaus2/rank/EncodeCache.hpp \
	libmaus2/rank/CacheLineRank.hpp \
	libmaus2/rank/ImpCacheLineRank.hpp \
	libmaus2/rank/ERank222BBase.hpp \
	libmaus2/rank/ERank222BAppendDynamic.hpp \
	libmaus2/rank/RunLengthBitVectorBase.hpp \
	libmaus2/rank/RunLengthBitVector.hpp \
	libmaus2/rank/RunLengthBitVectorGenerator.hpp \
	libmaus2/rank/RunLengthBitVectorStream.hpp \
	libmaus2/rank/RunLengthBitVectorGeneratorBase.hpp \
	libmaus2/rank/RunLengthBitVectorGeneratorGammaBase.hpp \
	libmaus2/rank/FastRank.hpp \
	libmaus2/rank/BSwapBase.hpp \
	libmaus2/rank/DNARank.hpp \
	libmaus2/rank/DNARankKmerCache.hpp \
	libmaus2/rank/DNARankBiDir.hpp \
	libmaus2/rank/DNARankBiDirRange.hpp \
	libmaus2/rank/DNARankMEM.hpp \
	libmaus2/rank/DNARankMEMPosComparator.hpp \
	libmaus2/rank/DNARankSMEMContext.hpp \
	libmaus2/rank/DNARankSMEMComputation.hpp \
	libmaus2/rank/DNARankGetPosition.hpp \
	libmaus2/rank/DNARankBiDirRangeSizeComparator.hpp \
	libmaus2/rank/DNARankSuffixTreeNodeEnumeratorQueueElement.hpp \
	libmaus2/rank/DNARankSuffixTreeNodeEnumerator.hpp

libmaus2geometry_includedir=$(includedir)/libmaus2/geometry
libmaus2geometry_include_HEADERS=\
	libmaus2/geometry/RangeSet.hpp
	
libmaus2select_includedir=$(includedir)/libmaus2/select
libmaus2select_include_HEADERS=\
	libmaus2/select/ESelect222B.hpp \
	libmaus2/select/ESelectBase.hpp \
	libmaus2/select/ESelectSimple.hpp \
	libmaus2/select/ImpCacheLineSelectSupport.hpp

libmaus2deleter_includedir=$(includedir)/libmaus2/deleter
libmaus2deleter_include_HEADERS=\
	libmaus2/deleter/Deleter.hpp 

libmaus2wavelet_includedir=$(includedir)/libmaus2/wavelet
libmaus2wavelet_include_HEADERS=\
	libmaus2/wavelet/toWaveletTreeBits.hpp \
	libmaus2/wavelet/WaveletTree.hpp \
	libmaus2/wavelet/HuffmanWaveletTree.hpp \
	libmaus2/wavelet/DynamicHuffmanWaveletTree.hpp \
	libmaus2/wavelet/DynamicWaveletTree.hpp \
	libmaus2/wavelet/ExternalWaveletGenerator.hpp \
	libmaus2/wavelet/ImpWaveletTree.hpp \
	libmaus2/wavelet/ImpExternalWaveletGenerator.hpp \
	libmaus2/wavelet/ImpExternalWaveletGeneratorHuffmanParallel.hpp \
	libmaus2/wavelet/ImpExternalWaveletGeneratorHuffman.hpp \
	libmaus2/wavelet/ImpHuffmanWaveletTree.hpp \
	libmaus2/wavelet/Utf8ToImpHuffmanWaveletTree.hpp \
	libmaus2/wavelet/Utf8ToImpCompactHuffmanWaveletTree.hpp \
	libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp \
	libmaus2/wavelet/ImpExternalWaveletGeneratorCompactHuffman.hpp \
	libmaus2/wavelet/ImpExternalWaveletGeneratorCompactHuffmanParallel.hpp \
	libmaus2/wavelet/RlToHwtBase.hpp \
	libmaus2/wavelet/RlToHwtTermRequest.hpp

libmaus2huffman_includedir=$(includedir)/libmaus2/huffman
libmaus2huffman_include_HEADERS=\
	libmaus2/huffman/CompressTable.hpp \
	libmaus2/huffman/DecodeTable.hpp \
	libmaus2/huffman/EncodeTable.hpp \
	libmaus2/huffman/hufEncodeString.hpp \
	libmaus2/huffman/HuffmanBase.hpp \
	libmaus2/huffman/huffman.hpp \
	libmaus2/huffman/HuffmanSorting.hpp \
	libmaus2/huffman/HuffmanTreeInnerNode.hpp \
	libmaus2/huffman/HuffmanTreeLeafComparator.hpp \
	libmaus2/huffman/HuffmanTreeLeaf.hpp \
	libmaus2/huffman/HuffmanTreeNode.hpp \
	libmaus2/huffman/CanonicalEncoder.hpp \
	libmaus2/huffman/RLDecoder.hpp \
	libmaus2/huffman/RLEncoder.hpp \
	libmaus2/huffman/HuffmanEncoderFile.hpp \
	libmaus2/huffman/IndexLoader.hpp \
	libmaus2/huffman/GapDecoder.hpp \
	libmaus2/huffman/GapEncoder.hpp \
	libmaus2/huffman/FileStreamBaseType.hpp \
	libmaus2/huffman/BitInputBuffer.hpp \
	libmaus2/huffman/PairAddSecond.hpp \
	libmaus2/huffman/IndexEntry.hpp \
	libmaus2/huffman/IndexLoaderBase.hpp \
	libmaus2/huffman/RLDecoderIndexBase.hpp \
	libmaus2/huffman/GapDecoderIndexBase.hpp \
	libmaus2/huffman/IndexDecoderDataArray.hpp \
	libmaus2/huffman/HuffmanInputFile.hpp \
	libmaus2/huffman/KvInitResult.hpp \
	libmaus2/huffman/HuffmanTree.hpp \
	libmaus2/huffman/SymCountEncoder.hpp \
	libmaus2/huffman/SymCountDecoder.hpp \
	libmaus2/huffman/SymCount.hpp \
	libmaus2/huffman/IndexWriter.hpp \
	libmaus2/huffman/SymCountRun.hpp \
	libmaus2/huffman/RLInitType.hpp \
	libmaus2/huffman/RLRun.hpp \
	libmaus2/huffman/LFInfo.hpp \
	libmaus2/huffman/LFSupportDecoder.hpp \
	libmaus2/huffman/LFSupportEncoder.hpp \
	libmaus2/huffman/LFSupportBitDecoder.hpp \
	libmaus2/huffman/LFSetBit.hpp \
	libmaus2/huffman/LFSetBitDecoder.hpp \
	libmaus2/huffman/LFSetBitEncoder.hpp \
	libmaus2/huffman/LFValueEncoder.hpp \
	libmaus2/huffman/LFValueDecoder.hpp \
	libmaus2/huffman/LFValue.hpp \
	libmaus2/huffman/LFPhiPairDecoder.hpp \
	libmaus2/huffman/LFPhiPairEncoder.hpp \
	libmaus2/huffman/LFPhiPair.hpp \
	libmaus2/huffman/LFPhiPairLCP.hpp \
	libmaus2/huffman/LFPhiPairLCPDecoder.hpp \
	libmaus2/huffman/LFPhiPairLCPEncoder.hpp \
	libmaus2/huffman/LFRankLCP.hpp \
	libmaus2/huffman/LFRankLCPDecoder.hpp \
	libmaus2/huffman/LFRankLCPEncoder.hpp \
	libmaus2/huffman/SymBit.hpp \
	libmaus2/huffman/SymBitDecoder.hpp \
	libmaus2/huffman/SymBitEncoder.hpp \
	libmaus2/huffman/SymBitRun.hpp \
	libmaus2/huffman/LFRankPos.hpp \
	libmaus2/huffman/LFRankPosEncoder.hpp \
	libmaus2/huffman/LFRankPosDecoder.hpp \
	libmaus2/huffman/LFSymRankPos.hpp \
	libmaus2/huffman/LFSymRankPosDecoder.hpp \
	libmaus2/huffman/LFSymRankPosEncoder.hpp 

libmaus2math_includedir=$(includedir)/libmaus2/math
libmaus2math_include_HEADERS=\
	libmaus2/math/isqrt.hpp \
	libmaus2/math/bitsPerNum.hpp \
	libmaus2/math/MetaLog.hpp \
	libmaus2/math/lowbits.hpp \
	libmaus2/math/numbits.hpp \
	libmaus2/math/gcd.hpp \
	libmaus2/math/binom.hpp \
	libmaus2/math/primes16.hpp \
	libmaus2/math/ilog.hpp \
	libmaus2/math/DoubleCode.hpp \
	libmaus2/math/iabs.hpp \
	libmaus2/math/IPower.hpp \
	libmaus2/math/Log.hpp \
	libmaus2/math/ipow.hpp \
	libmaus2/math/IntegerInterval.hpp \
	libmaus2/math/DecimalNumberParser.hpp \
	libmaus2/math/UnsignedInteger.hpp \
	libmaus2/math/Rational.hpp \
	libmaus2/math/BernoulliNumber.hpp \
	libmaus2/math/Faulhaber.hpp \
	libmaus2/math/RepetetiveKmerFreqEstimate.hpp \
	libmaus2/math/GmpInteger.hpp \
	libmaus2/math/GmpFloat.hpp \
	libmaus2/math/Convolution.hpp

libmaus2rl_includedir=$(includedir)/libmaus2/rl
libmaus2rl_include_HEADERS=\
	libmaus2/rl/RLIndex.hpp

libmaus2util_includedir=$(includedir)/libmaus2/util
libmaus2util_include_HEADERS=\
	libmaus2/util/stringFunctions.hpp \
	libmaus2/util/BitList.hpp \
	libmaus2/util/VarBitList.hpp \
	libmaus2/util/VarList.hpp \
	libmaus2/util/IntervalTree.hpp \
	libmaus2/util/LookupIntervalTree.hpp \
	libmaus2/util/StringSerialisation.hpp \
	libmaus2/util/NumberSerialisation.hpp \
	libmaus2/util/GetFileSize.hpp \
	libmaus2/util/unique_ptr.hpp \
	libmaus2/util/shared_ptr.hpp \
	libmaus2/util/TempFileNameGenerator.hpp \
	libmaus2/util/TempFileNameGeneratorState.hpp \
	libmaus2/util/Demangle.hpp \
	libmaus2/util/BitsPerNum.hpp \
	libmaus2/util/ArgInfo.hpp \
	libmaus2/util/Concat.hpp \
	libmaus2/util/ConcatRequest.hpp \
	libmaus2/util/PosixExecute.hpp \
	libmaus2/util/StackTrace.hpp \
	libmaus2/util/CompactNumberArray.hpp \
	libmaus2/util/ArrayIterator.hpp \
	libmaus2/util/OffsetArray.hpp \
	libmaus2/util/Array832.hpp \
	libmaus2/util/Array864.hpp \
	libmaus2/util/NegativeDifferenceArray32.hpp \
	libmaus2/util/NegativeDifferenceArray64.hpp \
	libmaus2/util/PositiveDifferenceArray32.hpp \
	libmaus2/util/PositiveDifferenceArray64.hpp \
	libmaus2/util/UniqueIndex.hpp \
	libmaus2/util/Terminal.hpp \
	libmaus2/util/nstat.hpp \
	libmaus2/util/WriteableString.hpp \
	libmaus2/util/ForkProcess.hpp \
	libmaus2/util/LogPipe.hpp \
	libmaus2/util/MoveStack.hpp \
	libmaus2/util/LongestIncreasingSubsequence.hpp \
	libmaus2/util/utf8.hpp \
	libmaus2/util/MemLimit.hpp \
	libmaus2/util/Histogram.hpp \
	libmaus2/util/IncreasingList.hpp \
	libmaus2/util/GenericSerialisation.hpp \
	libmaus2/util/ConsoleColor.hpp \
	libmaus2/util/LogPipeMultiplex.hpp \
	libmaus2/util/LogPipeMultiplexGeneric.hpp \
	libmaus2/util/iterator.hpp \
	libmaus2/util/SaturatingCounter.hpp \
	libmaus2/util/I386CacheLineSize.hpp \
	libmaus2/util/IncreasingStack.hpp \
	libmaus2/util/NearestNeighbourDictionary.hpp \
	libmaus2/util/MemUsage.hpp \
	libmaus2/util/unordered_set.hpp \
	libmaus2/util/unordered_map.hpp \
	libmaus2/util/ReverseByteOrder.hpp \
	libmaus2/util/SimpleBloomFilter.hpp \
	libmaus2/util/SimpleCountingHash.hpp \
	libmaus2/util/SimpleHashMap.hpp \
	libmaus2/util/SimpleHashSet.hpp \
	libmaus2/util/OutputFileNameTools.hpp \
	libmaus2/util/LELoad.hpp \
	libmaus2/util/GetObject.hpp \
	libmaus2/util/PutObject.hpp \
	libmaus2/util/PutObjectReverse.hpp \
	libmaus2/util/CountPutObject.hpp \
	libmaus2/util/CountPutIterator.hpp \
	libmaus2/util/PosixFileDescriptor.hpp \
	libmaus2/util/AlphaDigitTable.hpp \
	libmaus2/util/AlphaTable.hpp \
	libmaus2/util/DigitTable.hpp \
	libmaus2/util/TempFileRemovalContainer.hpp \
	libmaus2/util/DynamicLoading.hpp \
	libmaus2/util/Object.hpp \
	libmaus2/util/KMP.hpp \
	libmaus2/util/NumberMapSerialisation.hpp \
	libmaus2/util/Utf8BlockIndex.hpp \
	libmaus2/util/Utf8DecoderBuffer.hpp \
	libmaus2/util/Utf8String.hpp \
	libmaus2/util/UnsignedCharVariant.hpp \
	libmaus2/util/LSFSim.hpp \
	libmaus2/util/ForkProcessLSFSim.hpp \
	libmaus2/util/CountGetObject.hpp \
	libmaus2/util/HistogramSet.hpp \
	libmaus2/util/GenericIntervalTree.hpp \
	libmaus2/util/PosixInputFile.hpp \
	libmaus2/util/OctetString.hpp \
	libmaus2/util/Utf8StringPairAdapter.hpp \
	libmaus2/util/Utf8BlockIndexDecoder.hpp \
	libmaus2/util/Utf8DecoderWrapper.hpp \
	libmaus2/util/TempFileContainer.hpp \
	libmaus2/util/FileTempFileContainer.hpp \
	libmaus2/util/MemTempFileContainer.hpp \
	libmaus2/util/ImpCompactNumberArray.hpp \
	libmaus2/util/ContainerGetObject.hpp \
	libmaus2/util/PushBuffer.hpp \
	libmaus2/util/SuccinctBorderArray.hpp \
	libmaus2/util/LineAccessor.hpp \
	libmaus2/util/StringMapCompare.hpp \
	libmaus2/util/FreeList.hpp \
	libmaus2/util/ToUpperTable.hpp \
	libmaus2/util/BorderArray.hpp \
	libmaus2/util/SimpleHashMapConstants.hpp \
	libmaus2/util/SimpleHashMapKeyPrint.hpp \
	libmaus2/util/GrowingFreeList.hpp \
	libmaus2/util/SimpleHashMapHashCompute.hpp \
	libmaus2/util/SimpleHashMapInsDel.hpp \
	libmaus2/util/SimpleHashMapNumberCast.hpp \
	libmaus2/util/SimpleHashSetInsDel.hpp \
	libmaus2/util/LineBuffer.hpp \
	libmaus2/util/UnsignedIntegerIndexIterator.hpp \
	libmaus2/util/FiniteSizeHeap.hpp \
	libmaus2/util/UnitNum.hpp \
	libmaus2/util/NotDigitOrTermTable.hpp \
	libmaus2/util/ArgInfoParseBase.hpp \
	libmaus2/util/ReverseAdapter.hpp \
	libmaus2/util/MemoryStatistics.hpp \
	libmaus2/util/LongestIncreasingSubsequenceExtendedResult.hpp \
	libmaus2/util/ArgParser.hpp \
	libmaus2/util/PrefixSums.hpp \
	libmaus2/util/NumberToString.hpp \
	libmaus2/util/SplayTree.hpp \
	libmaus2/util/Queue.hpp \
	libmaus2/util/ContainerElementFreeList.hpp \
	libmaus2/util/HugePages.hpp \
	libmaus2/util/HugePageAllocator.hpp \
	libmaus2/util/StringAllocTypes.hpp \
	libmaus2/util/Destructable.hpp \
	libmaus2/util/SimpleQueue.hpp \
	libmaus2/util/SimpleHashMapDisplaceMaskFunction.hpp

libmaus2uint_includedir=$(includedir)/libmaus2/uint
libmaus2uint_include_HEADERS=\
	libmaus2/uint/uint.hpp 

libmaus2bp_includedir=$(includedir)/libmaus2/bp
libmaus2bp_include_HEADERS=\
	libmaus2/bp/BalancedParenthesesBase.hpp \
	libmaus2/bp/BalancedParentheses.hpp

libmaus2bitbtree_includedir=$(includedir)/libmaus2/bitbtree
libmaus2bitbtree_include_HEADERS=\
	libmaus2/bitbtree/bitbtree.hpp

libmaus2cumfreq_includedir=$(includedir)/libmaus2/cumfreq
libmaus2cumfreq_include_HEADERS=\
	libmaus2/cumfreq/DynamicCumulativeFrequencies.hpp \
	libmaus2/cumfreq/SlowCumFreq.hpp

libmaus2eta_includedir=$(includedir)/libmaus2/eta
libmaus2eta_include_HEADERS=\
	libmaus2/eta/LinearETA.hpp 

libmaus2trie_includedir=$(includedir)/libmaus2/trie
libmaus2trie_include_HEADERS=\
	libmaus2/trie/TrieState.hpp \
	libmaus2/trie/SimpleTrie.hpp

libmaus2aio_includedir=$(includedir)/libmaus2/aio
libmaus2aio_include_HEADERS=\
	libmaus2/aio/AsynchronousBufferReader.hpp \
	libmaus2/aio/AsynchronousWriter.hpp \
	libmaus2/aio/AsynchronousReader.hpp \
	libmaus2/aio/OutputBuffer8.hpp \
	libmaus2/aio/OutputBuffer84Bit.hpp \
	libmaus2/aio/TerminatedOutputBuffer84Bit.hpp \
	libmaus2/aio/OutputBuffer.hpp \
	libmaus2/aio/OutputFile8Array.hpp \
	libmaus2/aio/GenericInput.hpp \
	libmaus2/aio/MetaOutputFile8Array.hpp \
	libmaus2/aio/SynchronousOutputFile8Array.hpp \
	libmaus2/aio/MetaOutputBuffer8.hpp \
	libmaus2/aio/SynchronousOutputBuffer8.hpp \
	libmaus2/aio/BlockSynchronousOutputBuffer8.hpp \
	libmaus2/aio/GenericOutput.hpp \
	libmaus2/aio/MemInBuffer.hpp \
	libmaus2/aio/MemOutBuffer.hpp \
	libmaus2/aio/BufferedOutput.hpp \
	libmaus2/aio/FileFragment.hpp \
	libmaus2/aio/AsynchronousFastReaderBase.hpp \
	libmaus2/aio/SynchronousFastReaderBase.hpp \
	libmaus2/aio/LimitedGenericInput.hpp \
	libmaus2/aio/SynchronousGenericInput.hpp \
	libmaus2/aio/LimitedSynchronousGenericInput.hpp \
	libmaus2/aio/ReorderConcatGenericInput.hpp \
	libmaus2/aio/GetLength.hpp \
	libmaus2/aio/CompactDecoder4.hpp \
	libmaus2/aio/CompactEncoder4.hpp \
	libmaus2/aio/WriteCompactArray.hpp \
	libmaus2/aio/SynchronousGenericOutput.hpp \
	libmaus2/aio/SynchronousOutputBuffer8Posix.hpp \
	libmaus2/aio/SynchronousGenericOutputPosix.hpp \
	libmaus2/aio/PutOutputIterator.hpp \
	libmaus2/aio/CheckedInputStream.hpp \
	libmaus2/aio/CheckedOutputStream.hpp \
	libmaus2/aio/CircularBuffer.hpp \
	libmaus2/aio/CircularWrapper.hpp \
	libmaus2/aio/IStreamWrapper.hpp \
	libmaus2/aio/CheckedInputStreamAtOffset.hpp \
	libmaus2/aio/Buffer.hpp \
	libmaus2/aio/SingleFileFragmentMerge.hpp \
	libmaus2/aio/CheckedInputOutputStream.hpp \
	libmaus2/aio/PosixFdInput.hpp \
	libmaus2/aio/PosixFdInputWrapper.hpp \
	libmaus2/aio/PosixFdInputStream.hpp \
	libmaus2/aio/PosixFdOutputStreamBuffer.hpp \
	libmaus2/aio/PosixFdOutputStream.hpp \
	libmaus2/aio/PosixFdInputStreamBuffer.hpp \
	libmaus2/aio/LinuxStreamingPosixFdOutputStreamBuffer.hpp \
	libmaus2/aio/LinuxStreamingPosixFdOutputStream.hpp \
	libmaus2/aio/IsKnownLocalFileSystem.hpp \
	libmaus2/aio/LineSplittingPosixFdOutputStreamBuffer.hpp \
	libmaus2/aio/LineSplittingPosixFdOutputStream.hpp \
	libmaus2/aio/NamedTemporaryFile.hpp \
	libmaus2/aio/NamedTemporaryFileTypeInfo.hpp \
	libmaus2/aio/NamedTemporaryFileAllocator.hpp \
	libmaus2/aio/InputStreamWrapper.hpp \
	libmaus2/aio/InputStream.hpp \
	libmaus2/aio/InputStreamFactory.hpp \
	libmaus2/aio/PosixFdInputStreamFactory.hpp \
	libmaus2/aio/InputStreamFactoryContainer.hpp \
	libmaus2/aio/OutputStreamWrapper.hpp \
	libmaus2/aio/OutputStream.hpp \
	libmaus2/aio/OutputStreamFactory.hpp \
	libmaus2/aio/OutputStreamFactoryContainer.hpp \
	libmaus2/aio/PosixFdOutputStreamFactory.hpp \
	libmaus2/aio/InputOutputStreamFactoryContainer.hpp \
	libmaus2/aio/InputOutputStreamFactory.hpp \
	libmaus2/aio/InputOutputStream.hpp \
	libmaus2/aio/StdInputOutputStreamFactory.hpp \
	libmaus2/aio/InputOutputStreamWrapper.hpp \
	libmaus2/aio/PosixFdInputOutputStreamBuffer.hpp \
	libmaus2/aio/PosixFdInputOutputStream.hpp \
	libmaus2/aio/PosixFdInputOutputStreamFactory.hpp \
	libmaus2/aio/MemoryFileAdapter.hpp \
	libmaus2/aio/MemoryFileContainer.hpp \
	libmaus2/aio/MemoryFile.hpp \
	libmaus2/aio/MemoryInputOutputStreamBuffer.hpp \
	libmaus2/aio/MemoryInputOutputStreamFactory.hpp \
	libmaus2/aio/MemoryInputOutputStream.hpp \
	libmaus2/aio/MemoryInputStreamBuffer.hpp \
	libmaus2/aio/MemoryInputStreamFactory.hpp \
	libmaus2/aio/MemoryInputStream.hpp \
	libmaus2/aio/MemoryOutputStreamBuffer.hpp \
	libmaus2/aio/MemoryOutputStreamFactory.hpp \
	libmaus2/aio/MemoryOutputStream.hpp \
	libmaus2/aio/FileRemoval.hpp \
	libmaus2/aio/InputStreamPointerWrapper.hpp \
	libmaus2/aio/InputStreamInstance.hpp \
	libmaus2/aio/InputStreamInstanceAtOffset.hpp \
	libmaus2/aio/OutputStreamPointerWrapper.hpp \
	libmaus2/aio/OutputStreamInstance.hpp \
	libmaus2/aio/ConcatInputStreamBuffer.hpp \
	libmaus2/aio/ConcatInputStream.hpp \
	libmaus2/aio/SplitWriter.hpp \
	libmaus2/aio/ArrayInputStreamBuffer.hpp \
	libmaus2/aio/ArrayInputStream.hpp \
	libmaus2/aio/ArrayFile.hpp \
	libmaus2/aio/StreamLock.hpp \
	libmaus2/aio/ArrayFileContainer.hpp \
	libmaus2/aio/ArrayInputStreamFactory.hpp \
	libmaus2/aio/PairFileDecoder.hpp \
	libmaus2/aio/SizeMonitorThread.hpp \
	libmaus2/aio/DebugLineOutputStreamBuffer.hpp \
	libmaus2/aio/DebugLineOutputStream.hpp
	
libmaus2parallel_includedir=$(includedir)/libmaus2/parallel
libmaus2parallel_include_HEADERS=\
	libmaus2/parallel/OMPLock.hpp \
	libmaus2/parallel/PosixMutex.hpp \
	libmaus2/parallel/PosixSpinLock.hpp \
	libmaus2/parallel/PosixSemaphore.hpp \
	libmaus2/parallel/SynchronousQueue.hpp \
	libmaus2/parallel/TerminatableSynchronousQueue.hpp \
	libmaus2/parallel/PosixMutexArray.hpp \
	libmaus2/parallel/PosixThread.hpp \
	libmaus2/parallel/Pipe.hpp \
	libmaus2/parallel/SynchronousHeap.hpp \
	libmaus2/parallel/TerminatableSynchronousHeap.hpp \
	libmaus2/parallel/SynchronousReorderSet.hpp \
	libmaus2/parallel/TerminatableSynchronousReorderSet.hpp  \
	libmaus2/parallel/SynchronousCounter.hpp \
	libmaus2/parallel/PosixProcess.hpp \
	libmaus2/parallel/OMPNumThreadsScope.hpp \
	libmaus2/parallel/LockedBool.hpp \
	libmaus2/parallel/ThreadWorkPackage.hpp \
	libmaus2/parallel/ThreadWorkPackageComparator.hpp \
	libmaus2/parallel/ThreadPoolInterfaceEnqueTermInterface.hpp \
	libmaus2/parallel/ThreadWorkPackageDispatcher.hpp \
	libmaus2/parallel/ThreadPoolInterface.hpp \
	libmaus2/parallel/ThreadPoolThread.hpp \
	libmaus2/parallel/ThreadPool.hpp \
	libmaus2/parallel/SimpleThreadPool.hpp \
	libmaus2/parallel/SimpleThreadPoolInterfaceEnqueTermInterface.hpp \
	libmaus2/parallel/SimpleThreadPoolInterface.hpp \
	libmaus2/parallel/SimpleThreadPoolThread.hpp \
	libmaus2/parallel/SimpleThreadPoolWorkPackageFreeList.hpp \
	libmaus2/parallel/SimpleThreadWorkPackageComparator.hpp \
	libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp \
	libmaus2/parallel/SimpleThreadWorkPackage.hpp \
	libmaus2/parallel/LockedQueue.hpp \
	libmaus2/parallel/NumCpus.hpp \
	libmaus2/parallel/LockedFreeList.hpp \
	libmaus2/parallel/LockedHeap.hpp \
	libmaus2/parallel/LockedCounter.hpp \
	libmaus2/parallel/LockedGrowingFreeList.hpp \
	libmaus2/parallel/PosixConditionSemaphore.hpp \
	libmaus2/parallel/NamedPosixSemaphore.hpp \
	libmaus2/parallel/SimpleSemaphoreInterface.hpp \
	libmaus2/parallel/TimedSemaphoreInterface.hpp

libmaus2fastx_includedir=$(includedir)/libmaus2/fastx
libmaus2fastx_include_HEADERS=\
	libmaus2/fastx/CharTermTable.hpp \
	libmaus2/fastx/SpaceTable.hpp \
	libmaus2/fastx/CharBuffer.hpp \
	libmaus2/fastx/Pattern.hpp \
	libmaus2/fastx/acgtnMap.hpp \
	libmaus2/fastx/FastAReader.hpp \
	libmaus2/fastx/PatternBlock.hpp \
	libmaus2/fastx/FastIDBlock.hpp \
	libmaus2/fastx/FastQReader.hpp \
	libmaus2/fastx/FASTQEntry.hpp \
	libmaus2/fastx/isFastQ.hpp \
	libmaus2/fastx/MultiWordDNABitBuffer.hpp \
	libmaus2/fastx/SingleWordDNABitBuffer.hpp \
	libmaus2/fastx/FastInterval.hpp \
	libmaus2/fastx/FastAReaderSplit.hpp \
	libmaus2/fastx/FastQReaderSplit.hpp \
	libmaus2/fastx/ConcatWriter.hpp \
	libmaus2/fastx/FastFirstToLastReverseAdapter.hpp \
	libmaus2/fastx/FastServer.hpp \
	libmaus2/fastx/FastClient.hpp \
	libmaus2/fastx/CompactFastDecoderBase.hpp \
	libmaus2/fastx/CompactFastDecoder.hpp \
	libmaus2/fastx/CompactFastEncoder.hpp \
	libmaus2/fastx/CompactReadContainer.hpp \
	libmaus2/fastx/SocketFastAReader.hpp \
	libmaus2/fastx/SocketFastQReader.hpp \
	libmaus2/fastx/SubSamplingFastReader.hpp \
	libmaus2/fastx/SlashStrip.hpp \
	libmaus2/fastx/FirstWhiteSpaceSlashStrip.hpp \
	libmaus2/fastx/Phred.hpp \
	libmaus2/fastx/FastQElement.hpp \
	libmaus2/fastx/ComputeFastXIntervals.hpp \
	libmaus2/fastx/FqWeightQuantiser.hpp \
	libmaus2/fastx/CompactFastQHeader.hpp \
	libmaus2/fastx/CompactFastQContext.hpp \
	libmaus2/fastx/CompactFastQDecoderBase.hpp \
	libmaus2/fastx/CompactFastQContainerDictionary.hpp \
	libmaus2/fastx/CompactFastQContainer.hpp \
	libmaus2/fastx/CompactFastQContainerDictionaryCreator.hpp \
	libmaus2/fastx/CompactFastQBlockGenerator.hpp \
	libmaus2/fastx/CompactFastQSingleBlockReader.hpp \
	libmaus2/fastx/CompactFastQMultiBlockReader.hpp \
	libmaus2/fastx/CompactFastTerminator.hpp \
	libmaus2/fastx/CompactFastReplace.hpp \
	libmaus2/fastx/QReorder.hpp \
	libmaus2/fastx/StreamFastReaderBase.hpp \
	libmaus2/fastx/StreamFastQReader.hpp \
	libmaus2/fastx/GzipStreamFastQReader.hpp \
	libmaus2/fastx/GzipFileFastQReader.hpp \
	libmaus2/fastx/FastQBgzfWriter.hpp \
	libmaus2/fastx/StreamFastAReader.hpp \
	libmaus2/fastx/FastALineParserLineInfo.hpp \
	libmaus2/fastx/FastADefinedBasesTable.hpp \
	libmaus2/fastx/FastAMapTable.hpp \
	libmaus2/fastx/FastAMatchTable.hpp \
	libmaus2/fastx/FastALineParser.hpp \
	libmaus2/fastx/FastAStreamWrapper.hpp \
	libmaus2/fastx/FastAStream.hpp \
	libmaus2/fastx/FastAStreamSet.hpp \
	libmaus2/fastx/BgzfFastAIndexEntry.hpp \
	libmaus2/fastx/BgzfFastAIndexEntryNameComparator.hpp \
	libmaus2/fastx/BgzfFastAIndexEntryIdComparator.hpp \
	libmaus2/fastx/FastABgzfIndexEntry.hpp \
	libmaus2/fastx/FastABgzfIndex.hpp \
	libmaus2/fastx/FastABgzfDecoder.hpp \
	libmaus2/fastx/FastABgzfDecoderBuffer.hpp \
	libmaus2/fastx/FastAIndexEntry.hpp \
	libmaus2/fastx/FastAIndex.hpp \
	libmaus2/fastx/FastAInfo.hpp \
	libmaus2/fastx/FastATwoBitTable.hpp \
	libmaus2/fastx/FastABPConstants.hpp \
	libmaus2/fastx/FastABPGenerator.hpp \
	libmaus2/fastx/FastaBPSequenceDecoder.hpp \
	libmaus2/fastx/FastaBPDecoder.hpp \
	libmaus2/fastx/NameInfoBase.hpp \
	libmaus2/fastx/NameInfo.hpp \
	libmaus2/fastx/RefPathToken.hpp \
	libmaus2/fastx/RefPathTokenVector.hpp \
	libmaus2/fastx/RefPathTokenVectorSequence.hpp \
	libmaus2/fastx/RefPathLookup.hpp \
	libmaus2/fastx/DNAIndexMetaDataSequence.hpp \
	libmaus2/fastx/DNAIndexMetaData.hpp \
	libmaus2/fastx/DNAIndexMetaDataBigBand.hpp \
	libmaus2/fastx/DNAIndexMetaDataBigBandBiDir.hpp \
	libmaus2/fastx/CoordinateCache.hpp \
	libmaus2/fastx/LineBufferFastAReader.hpp\
	libmaus2/fastx/FastAToCompact4BigBandBiDir.hpp \
	libmaus2/fastx/FastAToCompact4BigBand.hpp \
	libmaus2/fastx/FastAIndexGenerator.hpp \
	libmaus2/fastx/CoordinateCacheBiDir.hpp \
	libmaus2/fastx/KmerFreqInWindowStats.hpp

libmaus2timing_includedir=$(includedir)/libmaus2/timing
libmaus2timing_include_HEADERS=\
	libmaus2/timing/RealTimeClock.hpp

libmaus2exception_includedir=$(includedir)/libmaus2/exception
libmaus2exception_include_HEADERS=\
	libmaus2/exception/LibMausException.hpp

libmaus2sorting_includedir=$(includedir)/libmaus2/sorting
libmaus2sorting_include_HEADERS=\
	libmaus2/sorting/sorting.hpp \
	libmaus2/sorting/ParallelRadixSort.hpp \
	libmaus2/sorting/SerialRadixSort64.hpp \
	libmaus2/sorting/PairFileSorting.hpp \
	libmaus2/sorting/InPlaceParallelSort.hpp \
	libmaus2/sorting/MergingReadBack.hpp \
	libmaus2/sorting/SortingBufferedOutputFile.hpp \
	libmaus2/sorting/MergeStepBinSearchResult.hpp \
	libmaus2/sorting/ParallelStableSort.hpp \
	libmaus2/sorting/SemiExternalKeyTupleSort.hpp \
	libmaus2/sorting/InterleavedRadixSort.hpp \
	libmaus2/sorting/ParallelRunLengthRadixUnsort.hpp \
	libmaus2/sorting/ParallelRunLengthRadixSort.hpp \
	libmaus2/sorting/ParallelExternalRadixSort.hpp

libmaus2hashing_includedir=$(includedir)/libmaus2/hashing
libmaus2hashing_include_HEADERS=\
	libmaus2/hashing/hash.hpp \
	libmaus2/hashing/CircularHash.hpp \
	libmaus2/hashing/ConstantStringHash.hpp \
	libmaus2/hashing/Crc32.hpp \
	libmaus2/hashing/MurmurHash3.h

libmaus2digest_includedir=$(includedir)/libmaus2/digest
libmaus2digest_include_HEADERS=\
	libmaus2/digest/DigestBase.hpp \
	libmaus2/digest/SHA1.hpp \
	libmaus2/digest/SHA2_224.hpp \
	libmaus2/digest/SHA2_256.hpp \
	libmaus2/digest/SHA2_384.hpp \
	libmaus2/digest/SHA2_512.hpp \
	libmaus2/digest/CRC32.hpp \
	libmaus2/digest/Digests.hpp \
	libmaus2/digest/Null.hpp \
	libmaus2/digest/SHA2_256_sse4.hpp \
	libmaus2/digest/sha256.h \
	libmaus2/digest/sha512.h \
	libmaus2/digest/SHA2_512_sse4.hpp \
	libmaus2/digest/CRC32C_sse42.hpp \
	libmaus2/digest/CRC32C_Core.hpp \
	libmaus2/digest/CRC32C.hpp \
	libmaus2/digest/CRC32_Core.hpp \
	libmaus2/digest/DigestInterface.hpp \
	libmaus2/digest/DigestFactory.hpp \
	libmaus2/digest/DigestFactoryContainer.hpp \
	libmaus2/digest/DigestFactory_SHA2_ASM.hpp \
	libmaus2/digest/DigestFactory_CRC32C_SSE42.hpp \
	libmaus2/digest/DigestFactoryInterface.hpp \
	libmaus2/digest/md5.hpp \
	libmaus2/digest/md5.h \
	libmaus2/digest/MurmurHash3_x64_128.hpp \
	libmaus2/digest/SHA3_256.hpp

libmaus2lf_includedir=$(includedir)/libmaus2/lf
libmaus2lf_include_HEADERS=\
	libmaus2/lf/LF.hpp \
	libmaus2/lf/HuffmanLF.hpp \
	libmaus2/lf/LFZero.hpp \
	libmaus2/lf/DArray.hpp \
	libmaus2/lf/ImpCompactHuffmanWaveletLF.hpp \
	libmaus2/lf/LFBase.hpp \
	libmaus2/lf/QuickLF.hpp \
	libmaus2/lf/MultiRankLF.hpp \
	libmaus2/lf/MultiRankCacheLF.hpp \
	libmaus2/lf/ImpCacheLineRankLF.hpp \
	libmaus2/lf/ImpCacheLineRankLFNonZero.hpp \
	libmaus2/lf/ImpHuffmanWaveletLF.hpp \
	libmaus2/lf/ImpWTLF.hpp \
	libmaus2/lf/RLIndexLF.hpp

libmaus2fm_includedir=$(includedir)/libmaus2/fm
libmaus2fm_include_HEADERS=\
	libmaus2/fm/FM.hpp \
	libmaus2/fm/SampledSA.hpp \
	libmaus2/fm/SampledISA.hpp \
	libmaus2/fm/MausFmToBwaConversion.hpp \
	libmaus2/fm/BidirectionalIndexInterval.hpp \
	libmaus2/fm/BidirectionalIndexIntervalSymbol.hpp \
	libmaus2/fm/DictionaryInfo.hpp \
	libmaus2/fm/DictionaryInfoImpCompactHuffmanWaveletTree.hpp \
	libmaus2/fm/BidirectionalDnaIndexTemplate.hpp \
	libmaus2/fm/KmerCache.hpp \
	libmaus2/fm/FA4CompactSampledSuffixArray.hpp

libmaus2alignment_includedir=$(includedir)/libmaus2/alignment
libmaus2alignment_include_HEADERS=\
	libmaus2/alignment/KmerTuple.hpp \
	libmaus2/alignment/FactorMatchInfo.hpp \
	libmaus2/alignment/KmerInfo.hpp \
	libmaus2/alignment/FactorMatchInfoPosComparator.hpp \
	libmaus2/alignment/FMIIntervalComparator.hpp \
	libmaus2/alignment/BamLineInfo.hpp \
	libmaus2/alignment/SimpleLocalAligner.hpp

libmaus2lcp_includedir=$(includedir)/libmaus2/lcp
libmaus2lcp_include_HEADERS=\
	libmaus2/lcp/LCP.hpp \
	libmaus2/lcp/OracleLCP.hpp \
	libmaus2/lcp/SuccinctLCP.hpp \
	libmaus2/lcp/WaveletLCPResult.hpp \
	libmaus2/lcp/WaveletLCP.hpp \
	libmaus2/lcp/ComputeSuccinctLCPResult.hpp \
	libmaus2/lcp/PLCPBitDecoder.hpp

libmaus2random_includedir=$(includedir)/libmaus2/random
libmaus2random_include_HEADERS=\
	libmaus2/random/Random.hpp \
	libmaus2/random/UniformUnitRandom.hpp \
	libmaus2/random/GaussianRandom.hpp \
	libmaus2/random/DNABaseNoiseSpiker.hpp

libmaus2graph_includedir=$(includedir)/libmaus2/graph
libmaus2graph_include_HEADERS=\
	libmaus2/graph/TripleEdge.hpp \
	libmaus2/graph/TripleEdgeInput.hpp \
	libmaus2/graph/TripleEdgeOutput.hpp \
	libmaus2/graph/TripleEdgeOutputMerge.hpp \
	libmaus2/graph/TripleEdgeOperations.hpp \
	libmaus2/graph/TripleEdgeBuffer.hpp \
	libmaus2/graph/TripleEdgeBufferSet.hpp \
	libmaus2/graph/TripleEdgeOutputSet.hpp \
	libmaus2/graph/U64Buffer.hpp \
	libmaus2/graph/TripleIOTest.hpp \
	libmaus2/graph/EdgeList.hpp \
	libmaus2/graph/EdgeBuffer.hpp \
	libmaus2/graph/EdgeListLocalFlush.hpp \
	libmaus2/graph/EdgeListLocalLockedFlush.hpp \
	libmaus2/graph/EdgeListLocked.hpp \
	libmaus2/graph/EdgeBufferLocal.hpp \
	libmaus2/graph/EdgeBufferLocalLocked.hpp \
	libmaus2/graph/EdgeListLockedSet.hpp \
	libmaus2/graph/LocalEdgeSet.hpp \
	libmaus2/graph/TripleEdgeCompact.hpp \
	libmaus2/graph/FilteredGraphTypes.hpp \
	libmaus2/graph/GraphEdge.hpp \
	libmaus2/graph/GraphEdgeBuffer.hpp \
	libmaus2/graph/GraphEdgeBlockBuffer.hpp \
	libmaus2/graph/GraphEdgeHeapComparator.hpp \
	libmaus2/graph/GraphEdgeMerge.hpp \
	libmaus2/graph/StronglyConnectedComponents.hpp \
	libmaus2/graph/IdentityTargetProjector.hpp \
	libmaus2/graph/ReachableVertices.hpp \
	libmaus2/graph/TopologicalSorting.hpp \
	libmaus2/graph/POGraph.hpp

libmaus2btree_includedir=$(includedir)/libmaus2/btree
libmaus2btree_include_HEADERS=\
	libmaus2/btree/btree.hpp

libmaus2lru_includedir=$(includedir)/libmaus2/lru
libmaus2lru_include_HEADERS=\
	libmaus2/lru/LRU.hpp \
	libmaus2/lru/SparseLRU.hpp \
	libmaus2/lru/SparseLRUFileBunch.hpp \
	libmaus2/lru/FileBunchLRU.hpp

libmaus2clustering_includedir=$(includedir)/libmaus2/clustering
libmaus2clustering_include_HEADERS=\
	libmaus2/clustering/HashCreation.hpp \
	libmaus2/clustering/HashCreationBase.hpp \
	libmaus2/clustering/ProduceEdgeRequest.hpp \
	libmaus2/clustering/KmerBase.hpp \
	libmaus2/clustering/KMeans.hpp

libmaus2suffixsort_includedir=$(includedir)/libmaus2/suffixsort
libmaus2suffixsort_include_HEADERS=\
	libmaus2/suffixsort/SkewSuffixSort.hpp \
	libmaus2/suffixsort/divsufsort.hpp \
	libmaus2/suffixsort/SAIS_Mori.hpp \
	libmaus2/suffixsort/SuccinctFactorList.hpp \
	libmaus2/suffixsort/SAISUtils.hpp \
	libmaus2/suffixsort/OSInduce.hpp \
	libmaus2/suffixsort/CircularBwt.hpp \
	libmaus2/suffixsort/CircularSuffixComparator.hpp \
	libmaus2/suffixsort/BwtMergeTempFileNameSet.hpp \
	libmaus2/suffixsort/BwtMergeZBlock.hpp \
	libmaus2/suffixsort/BwtMergeTempFileNameSetVector.hpp \
	libmaus2/suffixsort/BwtMergeBlockSortResult.hpp \
	libmaus2/suffixsort/BwtMergeZBlockRequest.hpp \
	libmaus2/suffixsort/BwtMergeZBlockRequestVector.hpp \
	libmaus2/suffixsort/GapMergePacket.hpp \
	libmaus2/suffixsort/BwtMergeEnumBase.hpp \
	libmaus2/suffixsort/ByteInputTypes.hpp \
	libmaus2/suffixsort/CompactInputTypes.hpp \
	libmaus2/suffixsort/PacInputTypes.hpp \
	libmaus2/suffixsort/PacTermInputTypes.hpp \
	libmaus2/suffixsort/Utf8InputTypes.hpp \
	libmaus2/suffixsort/Lz4InputTypes.hpp \
	libmaus2/suffixsort/GapArrayByteDecoderBuffer.hpp \
	libmaus2/suffixsort/GapArrayByte.hpp \
	libmaus2/suffixsort/GapArrayByteDecoder.hpp \
	libmaus2/suffixsort/GapArrayByteOverflowKeyAccessor.hpp \
	libmaus2/suffixsort/BwtMergeBlockSortRequestBase.hpp \
	libmaus2/suffixsort/BwtMergeBlockSortRequest.hpp \
	libmaus2/suffixsort/SmallestRotation.hpp

libmaus2suffixsortbwtb3m_includedir=$(includedir)/libmaus2/suffixsort/bwtb3m
libmaus2suffixsortbwtb3m_include_HEADERS=\
	libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequestQueryObject.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyBlock.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyBaseBlock.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequest.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyMergeBlock.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalBlock.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalSmallBlock.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyMergeExternalBlock.hpp \
	libmaus2/suffixsort/bwtb3m/MergeStrategyConstruction.hpp \
	libmaus2/suffixsort/bwtb3m/BWTB3MBase.hpp \
	libmaus2/suffixsort/bwtb3m/BaseBlockSortThread.hpp \
	libmaus2/suffixsort/bwtb3m/BaseBlockSorting.hpp \
	libmaus2/suffixsort/bwtb3m/BwtMergeSortOptions.hpp \
	libmaus2/suffixsort/bwtb3m/BwtMergeSortTemplate.hpp \
	libmaus2/suffixsort/bwtb3m/BwtMergeSort.hpp \
	libmaus2/suffixsort/bwtb3m/BwtMergeSortResult.hpp \
	libmaus2/suffixsort/bwtb3m/BwtComputeSSA.hpp \
	libmaus2/suffixsort/bwtb3m/BwtSelectSSA.hpp

libmaus2suffixtree_includedir=$(includedir)/libmaus2/suffixtree
libmaus2suffixtree_include_HEADERS=\
	libmaus2/suffixtree/CompressedSuffixTree.hpp

libmaus2rmq_includedir=$(includedir)/libmaus2/rmq
libmaus2rmq_include_HEADERS=\
	libmaus2/rmq/FischerSystematicSuccinctRMQ.hpp \
	libmaus2/rmq/QuickDynamicRMQ.hpp \
	libmaus2/rmq/CartesianTree.hpp \
	libmaus2/rmq/RMMTree.hpp

libmaus2network_includedir=$(includedir)/libmaus2/network
libmaus2network_include_HEADERS=\
	libmaus2/network/Socket.hpp \
	libmaus2/network/GetHostName.hpp \
	libmaus2/network/LogSocket.hpp \
	libmaus2/network/SocketFastReaderBase.hpp \
	libmaus2/network/Interface.hpp \
	libmaus2/network/GetDottedAddress.hpp \
	libmaus2/network/UDPSocket.hpp \
	libmaus2/network/GenericServer.hpp \
	libmaus2/network/SocketOutputBuffer8.hpp \
	libmaus2/network/SocketOutputBuffer8Set.hpp \
	libmaus2/network/sendReceiveFd.h \
	libmaus2/network/SocketPair.hpp \
	libmaus2/network/FileDescriptorPasser.hpp \
	libmaus2/network/LogReceiverTestProcess.hpp \
	libmaus2/network/LogReceiver.hpp \
	libmaus2/network/LogReceiverDispatcherBase.hpp \
	libmaus2/network/SocketInputStreamBuffer.hpp \
	libmaus2/network/SocketInputStream.hpp \
	libmaus2/network/HttpAbsoluteUrl.hpp \
	libmaus2/network/SocketInputOutputInterface.hpp \
	libmaus2/network/FtpUrl.hpp \
	libmaus2/network/HttpBody.hpp \
	libmaus2/network/HttpHeader.hpp \
	libmaus2/network/FtpSocket.hpp \
	libmaus2/network/UrlBase.hpp \
	libmaus2/network/HttpHeaderWrapper.hpp \
	libmaus2/network/HttpBodyWrapper.hpp \
	libmaus2/network/FtpSocketWrapper.hpp \
	libmaus2/network/HttpSocketInputStreamBuffer.hpp \
	libmaus2/network/HttpSocketInputStream.hpp \
	libmaus2/network/FtpSocketInputStreamBuffer.hpp \
	libmaus2/network/FtpSocketInputStream.hpp \
	libmaus2/network/FileUrl.hpp \
	libmaus2/network/UrlInputStreamBufferWrapper.hpp \
	libmaus2/network/UrlInputStream.hpp \
	libmaus2/network/GnuTLSInit.hpp \
	libmaus2/network/GnuTLSSocket.hpp \
	libmaus2/network/SingleFileServer.hpp \
	libmaus2/network/UrlInputStreamFactory.hpp

libmaus2sv_includedir=$(includedir)/libmaus2/sv
libmaus2sv_include_HEADERS=\
	libmaus2/sv/sv.hpp 
	
libmaus2hdf5_includedir=$(includedir)/libmaus2/hdf5
libmaus2hdf5_include_HEADERS=\
	libmaus2/hdf5/HDF5MuteErrorHandler.hpp \
	libmaus2/hdf5/HDF5Handle.hpp
	
libmaus2irods_includedir=$(includedir)/libmaus2/irods
libmaus2irods_include_HEADERS=\
	libmaus2/irods/IRodsFileBase.hpp \
	libmaus2/irods/IRodsSystem.hpp \
	libmaus2/irods/IRodsCommProvider.hpp \
	libmaus2/irods/IRodsInputStreamBuffer.hpp \
	libmaus2/irods/IRodsInputStream.hpp \
	libmaus2/irods/IRodsInputStreamFactory.hpp

libmaus2lcs_includedir=$(includedir)/libmaus2/lcs
libmaus2lcs_include_HEADERS=\
	libmaus2/lcs/SuffixArrayLCS.hpp \
	libmaus2/lcs/LCSRef.hpp \
	libmaus2/lcs/LCSDynamic.hpp \
	libmaus2/lcs/SuffixPrefix.hpp \
	libmaus2/lcs/OverlapComputation.hpp \
	libmaus2/lcs/LCS.hpp \
	libmaus2/lcs/GenericEditDistance.hpp \
	libmaus2/lcs/EditDistanceResult.hpp \
	libmaus2/lcs/BaseConstants.hpp \
	libmaus2/lcs/PenaltyConstants.hpp \
	libmaus2/lcs/TraceContainer.hpp \
	libmaus2/lcs/GenericAlignmentPrint.hpp \
	libmaus2/lcs/SuffixPrefixResult.hpp \
	libmaus2/lcs/SuffixPrefixAlignmentPrint.hpp \
	libmaus2/lcs/OffsetElement.hpp \
	libmaus2/lcs/HashContainer.hpp \
	libmaus2/lcs/HashContainer2.hpp \
	libmaus2/lcs/DistNodeType.hpp \
	libmaus2/lcs/OverlapComputationBlockRequest.hpp \
	libmaus2/lcs/OverlapOrientation.hpp \
	libmaus2/lcs/OrientationWeightEncoding.hpp \
	libmaus2/lcs/CheckOverlapResult.hpp \
	libmaus2/lcs/CheckOverlapResultIdComparator.hpp \
	libmaus2/lcs/CheckOverlapResultIdHeapComparator.hpp \
	libmaus2/lcs/CheckOverlapResultInput.hpp \
	libmaus2/lcs/CheckOverlapResultInputPushBack.hpp \
	libmaus2/lcs/CheckOverlapResultMergeInput.hpp \
	libmaus2/lcs/CheckOverlapScoreComparatorGreater.hpp \
	libmaus2/lcs/CheckOverlapResultOperations.hpp \
	libmaus2/lcs/HammingOverlapDetection.hpp \
	libmaus2/lcs/AlignmentPrint.hpp \
	libmaus2/lcs/AlignmentTraceContainer.hpp \
	libmaus2/lcs/EditDistance.hpp \
	libmaus2/lcs/BandedEditDistance.hpp \
	libmaus2/lcs/MetaEditDistance.hpp \
	libmaus2/lcs/EditDistanceTraceContainer.hpp \
	libmaus2/lcs/EditDistancePriorityType.hpp \
	libmaus2/lcs/LocalBaseConstants.hpp \
	libmaus2/lcs/LocalPenaltyConstants.hpp \
	libmaus2/lcs/LocalAlignmentTraceContainer.hpp \
	libmaus2/lcs/LocalEditDistanceTraceContainer.hpp \
	libmaus2/lcs/LocalAlignmentPrint.hpp \
	libmaus2/lcs/LocalEditDistance.hpp \
	libmaus2/lcs/LocalEditDistanceResult.hpp \
	libmaus2/lcs/MetaLocalEditDistance.hpp \
	libmaus2/lcs/BandedLocalEditDistance.hpp \
	libmaus2/lcs/ND.hpp \
	libmaus2/lcs/NDextend.hpp \
	libmaus2/lcs/AlignmentStatistics.hpp \
	libmaus2/lcs/NDextendAllPass.hpp \
	libmaus2/lcs/NDextendACGTPass.hpp \
	libmaus2/lcs/NDextend1234Pass.hpp \
	libmaus2/lcs/BitVectorResultCallbackInterface.hpp \
	libmaus2/lcs/BitVector.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentConstants128.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentConstants256.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentTemplate.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_16_def.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_16.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_16_undef.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_8_def.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_8.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentX128_8_undef.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_16_def.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_16.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_16_undef.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_8_def.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_8.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentY256_8_undef.hpp \
	libmaus2/lcs/Aligner.hpp \
	libmaus2/lcs/AlignerFactory.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentTemplate.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_16_def.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_16.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_16_undef.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_8_def.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_8.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_8_undef.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_16_def.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_16.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_16_undef.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_8_def.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_8.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_8_undef.hpp \
	libmaus2/lcs/BandedAligner.hpp \
	libmaus2/lcs/BandedAlignerFactory.hpp \
	libmaus2/lcs/NP.hpp \
	libmaus2/lcs/NNP.hpp \
	libmaus2/lcs/NNPTraceElement.hpp \
	libmaus2/lcs/NNPTraceContainer.hpp \
	libmaus2/lcs/NNPAlignResult.hpp \
	libmaus2/lcs/DalignerNP.hpp \
	libmaus2/lcs/DalignerLocalAlignment.hpp \
	libmaus2/lcs/FragmentEnvelopeYScore.hpp \
	libmaus2/lcs/FragmentEnvelopeFragment.hpp \
	libmaus2/lcs/FragmentEnvelope.hpp \
	libmaus2/lcs/EnvelopeFragment.hpp \
	libmaus2/lcs/ScoreBandedAligner.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentScoreTemplate.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentScoreY256_16.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentScoreY256_16_def.hpp \
	libmaus2/lcs/SimdX86BandedGlobalAlignmentScoreY256_16_undef.hpp \
	libmaus2/lcs/SimdX86GlobalAlignmentScoreConstants256.hpp \
	libmaus2/lcs/NNPLocalAligner.hpp \
	libmaus2/lcs/ChainAddQueueElementHeapComparator.hpp \
	libmaus2/lcs/ChainNode.hpp \
	libmaus2/lcs/ChainRemoveQueueElement.hpp \
	libmaus2/lcs/ChainAddQueueElement.hpp \
	libmaus2/lcs/ChainNodeInfo.hpp \
	libmaus2/lcs/ChainSplayTreeNodeComparator.hpp \
	libmaus2/lcs/ChainAlignment.hpp \
	libmaus2/lcs/ChainNodeInfoSet.hpp \
	libmaus2/lcs/ChainSplayTreeNode.hpp \
	libmaus2/lcs/Chain.hpp \
	libmaus2/lcs/ChainRemoveQueueElementComparator.hpp \
	libmaus2/lcs/ChainLeftMostComparator.hpp \
	libmaus2/lcs/SMEMProcessor.hpp

libmaus2quantisation_includedir=$(includedir)/libmaus2/quantisation/
libmaus2quantisation_include_HEADERS = libmaus2/quantisation/Quantiser.hpp libmaus2/quantisation/ClusterComputation.hpp libmaus2/quantisation/KmeansDataType.hpp
	
libmaus2lsf_includedir=$(includedir)/libmaus2/lsf
libmaus2lsf_include_HEADERS=\
	libmaus2/lsf/LSFProcess.hpp \
	libmaus2/lsf/ProcessSet.hpp \
	libmaus2/lsf/Process.hpp \
	libmaus2/lsf/LSFDispatcherBase.hpp \
	libmaus2/lsf/LSFStateBase.hpp \
	libmaus2/lsf/DispatchedLsfProcess.hpp  \
	libmaus2/lsf/SimpleDispatchedLsfProcessSet.hpp \
	libmaus2/lsf/LSFTaskQueue.hpp

libmaus2consensus_includedir=$(includedir)/libmaus2/consensus/
libmaus2consensus_include_HEADERS =\
	libmaus2/consensus/Consensus.hpp \
	libmaus2/consensus/ScoringMatrix.hpp

EXTRA_PROGRAMS = testlsf testlsflist \
	testAIO \
	testbitbtree \
	testBitIO \
	testbtree \
	testBWTDyn \
	testBWTHufDyn \
	testDivSufSort \
	testDNABitWords \
	testFastRead \
	testFork \
	testHuffman \
	testIsa \
	testkmp \
	testlz \
	testOS \
	testRank \
	testToWaveletTreeBits \
	testWaveletExternal \
	testTrie \
	testConsensus \
	testbandedlcs \
	testbamwriter \
	testhashlcs \
	testarraydecode \
	testsnappy \
	testsnappyconcat \
	testgunzip \
	testlogging \
	testregex \
	testhashcollisions \
	testsimplebloomcollisions \
	testsimplehash \
	testsimplecountinghash \
	testsyncreorderset \
	testcachealignedalloc \
	testblocksynchronous \
	testsyncfastreaderbase \
	testlsf \
	testlsfdispatcher \
	testlsfclient \
	testlsfclientfw \
	testlsflist \
	testdl \
	testdifenc \
	testbwtmergeinternal \
	testfastarewrite \
	testfastarecode \
	testbamtofastq \
	testbamrange \
	testbamdamage \
	testgamma \
	testcompact \
	testbytetocompact \
	testscramtosam \
	testpacdecode \
	testutf8split \
	testcirchash \
	testrmmtree \
	testcst \
	testbamcmp \
	testbamdif \
	testqreorder \
	testfqbgzf \
	testoverlap \
	testcompactfastrandom \
	testbamindex \
	testbamrewrite \
	testfastqstream \
	testsort \
	testexternalsort \
	testcollatespeed \
	testbamheaderparser \
	testbammergecoordinate \
	testbammergecollate \
	testbammergequeryname \
	testbamcat \
	testautoarray2d \
	testhist \
	testfastaparse \
	testvalidatebamindex \
	testeditdistance \
	testthreadpool \
	testsimplethreadpool \
	testthreadpoolbamdecode \
	testsimplethreadpoolbamdecode \
	testgaparraybyte \
	testzlibconcat \
	testsimplecompressblock \
	testrazdec \
	testfaindex \
	testfastablockrecode \
	testhttp \
	testftp \
	testrlhist \
	testnumcpus \
	testdiag \
	testlinuxstreamingwrite \
	testlowmembamheaderparser \
	testgeneflatfile \
	testurlinputstream \
	testsimpletrie \
	testgnutls \
	testlru \
	testparallelbamsorting \
	testparallelbamvalidation \
	testbytecnt \
	teststronglyconnectedcomponents \
	testtopologicalsorting \
	testlinebuffer \
	testsamdecoder \
	testdecimalnumberparser \
	testunsignedinteger \
	testmd5 \
	testsha \
	testsinglefileserver \
	testparallelbamrewrite \
	testbammateerase \
	testparallelbamblocksort \
	testhdf5 \
	testmapperm \
	testexternalmemoryindex \
	testhdf5 \
	testirods \
	testfastqtobam \
	testsamrecoding \
	testdazzlerdb \
	testnd \
	testfastashred \
	testalignerspeed \
	testfastasingleline \
	testdazzleralignmentrewrite \
	testxz \
	testdazzlerindex \
	testargparser \
	testfragmentenvelope \
	testdnarank \
	testlowbits \
	testfastaexplode \
	testfaextract \
	testmurmur3 \
	testpairalign \
	testsplaytree \
	testbwtconstruction \
	testbwtconstructionfasta \
	testarrayinput \
	testhugepages \
	testsymcnt \
	testgammapd \
	testpo

noinst_PROGRAMS = @TESTPROGS@

testgunzip_SOURCES = test/testgunzip.cpp
testgunzip_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

teststronglyconnectedcomponents_SOURCES = test/teststronglyconnectedcomponents.cpp
teststronglyconnectedcomponents_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testtopologicalsorting_SOURCES = test/testtopologicalsorting.cpp
testtopologicalsorting_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbytecnt_SOURCES = test/testbytecnt.cpp
testbytecnt_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testlru_SOURCES = test/testlru.cpp
testlru_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsimpletrie_SOURCES = test/testsimpletrie.cpp
testsimpletrie_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testthreadpool_SOURCES = test/testthreadpool.cpp
testthreadpool_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsimplethreadpool_SOURCES = test/testsimplethreadpool.cpp
testsimplethreadpool_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testthreadpoolbamdecode_SOURCES = test/testthreadpoolbamdecode.cpp
testthreadpoolbamdecode_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsimplethreadpoolbamdecode_SOURCES = test/testsimplethreadpoolbamdecode.cpp
testsimplethreadpoolbamdecode_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@
testsimplethreadpoolbamdecode_CPPFLAGS = @SNAPPYCPPFLAGS@
testsimplethreadpoolbamdecode_LDFLAGS = @SNAPPYLDFLAGS@

testsyncfastreaderbase_SOURCES = test/testsyncfastreaderbase.cpp
testsyncfastreaderbase_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testblocksynchronous_SOURCES = test/testblocksynchronous.cpp
testblocksynchronous_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testcachealignedalloc_SOURCES = test/testcachealignedalloc.cpp
testcachealignedalloc_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsyncreorderset_SOURCES = test/testsyncreorderset.cpp
testsyncreorderset_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testhashcollisions_SOURCES = test/testhashcollisions.cpp
testhashcollisions_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsimplebloomcollisions_SOURCES = test/testsimplebloomcollisions.cpp
testsimplebloomcollisions_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsimplehash_SOURCES = test/testsimplehash.cpp
testsimplehash_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsimplecountinghash_SOURCES = test/testsimplecountinghash.cpp
testsimplecountinghash_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testregex_SOURCES = test/testregex.cpp
testregex_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testlogging_SOURCES = test/testlogging.cpp
testlogging_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testTrie_SOURCES = test/testTrie.cpp
testTrie_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testFork_SOURCES = test/testFork.cpp
testFork_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testBitIO_SOURCES = test/testBitIO.cpp
testBitIO_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testDivSufSort_SOURCES = test/testDivSufSort.cpp
testDivSufSort_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testRank_SOURCES = test/testRank.cpp
testRank_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${SDSLLIBS}
testRank_CPPFLAGS = ${SDSLCPPFLAGS}

testHuffman_SOURCES = test/testHuffman.cpp
testHuffman_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbitbtree_SOURCES = test/testbitbtree.cpp
testbitbtree_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testBWTDyn_SOURCES = test/testBWTDyn.cpp
testBWTDyn_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testBWTHufDyn_SOURCES = test/testBWTHufDyn.cpp
testBWTHufDyn_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testAIO_SOURCES = test/testAIO.cpp
testAIO_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testFastRead_SOURCES = test/testFastRead.cpp
testFastRead_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testDNABitWords_SOURCES = test/testDNABitWords.cpp
testDNABitWords_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbtree_SOURCES = test/testbtree.cpp
testbtree_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testToWaveletTreeBits_SOURCES = test/testToWaveletTreeBits.cpp
testToWaveletTreeBits_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testWaveletExternal_SOURCES = test/testWaveletExternal.cpp
testWaveletExternal_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testIsa_SOURCES = test/testIsa.cpp
testIsa_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testOS_SOURCES = test/testOS.cpp
testOS_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testlz_SOURCES = test/testlz.cpp
testlz_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbandedlcs_SOURCES = test/testbandedlcs.cpp
testbandedlcs_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbamwriter_SOURCES = test/testbamwriter.cpp
testbamwriter_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testConsensus_SOURCES = test/testConsensus.cpp
testConsensus_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testhashlcs_SOURCES = test/testhashlcs.cpp
testhashlcs_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testhashlcs_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testarraydecode_SOURCES = test/testarraydecode.cpp
testarraydecode_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsnappy_SOURCES = test/testsnappy.cpp
testsnappy_CPPFLAGS = @SNAPPYCPPFLAGS@
testsnappy_LDFLAGS = @SNAPPYLDFLAGS@
testsnappy_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@

testsnappyconcat_SOURCES = test/testsnappyconcat.cpp
testsnappyconcat_CPPFLAGS = @SNAPPYCPPFLAGS@
testsnappyconcat_LDFLAGS = @SNAPPYLDFLAGS@
testsnappyconcat_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@

testlsf_SOURCES = test/testlsf.cpp
testlsf_CPPFLAGS = ${AM_CPPFLAGS} ${LSFCPPFLAGS}
testlsf_LDFLAGS = ${LSFLDFLAGS}
testlsf_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la libmaus2lsf.la ${LSFLIBS}

testlsfdispatcher_SOURCES = test/testlsfdispatcher.cpp
testlsfdispatcher_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testlsfclient_SOURCES = test/testlsfclient.cpp
testlsfclient_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testlsfclientfw_SOURCES = test/testlsfclientfw.cpp
testlsfclientfw_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testdl_SOURCES = test/testdl.cpp
testdl_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}

testdifenc_SOURCES = test/testdifenc.cpp
testdifenc_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testkmp_SOURCES = test/testkmp.cpp
testkmp_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbwtmergeinternal_SOURCES = test/testbwtmergeinternal.cpp
testbwtmergeinternal_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testfastarewrite_SOURCES = test/testfastarewrite.cpp
testfastarewrite_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testfastarecode_SOURCES = test/testfastarecode.cpp
testfastarecode_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbamtofastq_SOURCES = test/testbamtofastq.cpp
testbamtofastq_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@
testbamtofastq_CPPFLAGS = @SNAPPYCPPFLAGS@
testbamtofastq_LDFLAGS = @SNAPPYLDFLAGS@ @DLLIB@

testbamrange_SOURCES = test/testbamrange.cpp
testbamrange_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbammateerase_SOURCES = test/testbammateerase.cpp
testbammateerase_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbamdamage_SOURCES = test/testbamdamage.cpp
testbamdamage_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@
testbamdamage_CPPFLAGS = @SNAPPYCPPFLAGS@
testbamdamage_LDFLAGS = @SNAPPYLDFLAGS@

testgamma_SOURCES = test/testgamma.cpp
testgamma_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testgamma_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testcompact_SOURCES = test/testcompact.cpp
testcompact_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbytetocompact_SOURCES = test/testbytetocompact.cpp
testbytetocompact_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testscramtosam_SOURCES = test/testscramtosam.cpp
testscramtosam_LDADD    = libmaus2_irods.a libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @IRODSLIBS@ @DLLIB@
testscramtosam_CPPFLAGS = @IRODSCPPFLAGS@ @WARNCPPFLAGS@
testscramtosam_LDFLAGS  = ${AM_LDFLAGS} @IRODSLDFLAGS@
testscramtosam_DEPENDENCIES = libmaus2_irods.a libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testpacdecode_SOURCES = test/testpacdecode.cpp
testpacdecode_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testutf8split_SOURCES = test/testutf8split.cpp
testutf8split_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testcirchash_SOURCES = test/testcirchash.cpp
testcirchash_CPPFLAGS = @SNAPPYCPPFLAGS@
testcirchash_LDFLAGS = @SNAPPYLDFLAGS@
testcirchash_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@ ${DLLIB}

testrmmtree_SOURCES = test/testrmmtree.cpp
testrmmtree_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testcst_SOURCES = test/testcst.cpp
testcst_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbamcmp_SOURCES = test/testbamcmp.cpp
testbamcmp_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbamdif_SOURCES = test/testbamdif.cpp
testbamdif_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testqreorder_SOURCES = test/testqreorder.cpp
testqreorder_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testfqbgzf_SOURCES = test/testfqbgzf.cpp
testfqbgzf_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testoverlap_SOURCES = test/testoverlap.cpp
testoverlap_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testcompactfastrandom_SOURCES = test/testcompactfastrandom.cpp
testcompactfastrandom_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbamindex_SOURCES = test/testbamindex.cpp
testbamindex_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testlsflist_SOURCES = test/testlsflist.cpp
testlsflist_CPPFLAGS = ${AM_CPPFLAGS} ${LSFCPPFLAGS}
testlsflist_LDFLAGS = ${LSFLDFLAGS}
testlsflist_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la libmaus2lsf.la ${LSFLIBS}

testbamrewrite_SOURCES = test/testbamrewrite.cpp
testbamrewrite_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testfastqstream_SOURCES = test/testfastqstream.cpp
testfastqstream_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testsort_SOURCES = test/testsort.cpp
testsort_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testexternalsort_SOURCES = test/testexternalsort.cpp
testexternalsort_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testcollatespeed_SOURCES = test/testcollatespeed.cpp
testcollatespeed_CPPFLAGS = @SNAPPYCPPFLAGS@
testcollatespeed_LDFLAGS = @SNAPPYLDFLAGS@
testcollatespeed_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@ ${DLLIB}

testbamheaderparser_SOURCES = test/testbamheaderparser.cpp
testbamheaderparser_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testbammergecoordinate_SOURCES = test/testbammergecoordinate.cpp
testbammergecoordinate_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @DLLIB@

testbammergequeryname_SOURCES = test/testbammergequeryname.cpp
testbammergequeryname_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @DLLIB@

testbamcat_SOURCES = test/testbamcat.cpp
testbamcat_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @DLLIB@

testbammergecollate_SOURCES = test/testbammergecollate.cpp
testbammergecollate_LDFLAGS = @SNAPPYLDFLAGS@
testbammergecollate_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@ ${DLLIB}
testbammergecollate_CPPFLAGS = @SNAPPYCPPFLAGS@

testautoarray2d_SOURCES = test/testautoarray2d.cpp
testautoarray2d_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testhist_SOURCES = test/testhist.cpp
testhist_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testfastaparse_SOURCES = test/testfastaparse.cpp
testfastaparse_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testvalidatebamindex_SOURCES = test/testvalidatebamindex.cpp
testvalidatebamindex_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testeditdistance_SOURCES = test/testeditdistance.cpp
testeditdistance_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testgaparraybyte_SOURCES = test/testgaparraybyte.cpp
testgaparraybyte_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testzlibconcat_SOURCES = test/testzlibconcat.cpp
testzlibconcat_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testzlibconcat_CPPFLAGS = @ZLIBCPPFLAGS@ @WARNCPPFLAGS@

testsimplecompressblock_SOURCES = test/testsimplecompressblock.cpp
testsimplecompressblock_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@ ${DLLIB}
testsimplecompressblock_CPPFLAGS =  @ZLIBCPPFLAGS@ @WARNCPPFLAGS@ @SNAPPYCPPFLAGS@
testsimplecompressblock_LDFLAGS = @SNAPPYLDFLAGS@

testrazdec_SOURCES = test/testrazdec.cpp
testrazdec_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testrazdec_CPPFLAGS = @ZLIBCPPFLAGS@ @WARNCPPFLAGS@

testfaindex_SOURCES = test/testfaindex.cpp
testfaindex_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testfastablockrecode_SOURCES = test/testfastablockrecode.cpp
testfastablockrecode_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testhttp_SOURCES = test/testhttp.cpp
testhttp_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testhttp_CPPFLAGS = @GNUTLSCPPFLAGS@ @WARNCPPFLAGS@
testhttp_LDFLAGS = @GNUTLSLIBS@

testftp_SOURCES = test/testftp.cpp
testftp_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testftp_CPPFLAGS = @GNUTLSCPPFLAGS@ @WARNCPPFLAGS@
testftp_LDFLAGS = @GNUTLSLIBS@

testurlinputstream_SOURCES = test/testurlinputstream.cpp
testurlinputstream_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testurlinputstream_CPPFLAGS = @GNUTLSCPPFLAGS@ @WARNCPPFLAGS@
testurlinputstream_LDFLAGS = @GNUTLSLIBS@

testrlhist_SOURCES = test/testrlhist.cpp
testrlhist_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testnumcpus_SOURCES = test/testnumcpus.cpp
testnumcpus_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testdiag_SOURCES = test/testdiag.cpp
testdiag_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testdiag_CPPFLAGS = ${AM_CPPFLAGS}
testdiag_LDFLAGS = ${AM_LDFLAGS}

testlinuxstreamingwrite_SOURCES = test/testlinuxstreamingwrite.cpp
testlinuxstreamingwrite_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testlinuxstreamingwrite_CPPFLAGS = ${AM_CPPFLAGS}
testlinuxstreamingwrite_LDFLAGS = ${AM_LDFLAGS}

testlowmembamheaderparser_SOURCES = test/testlowmembamheaderparser.cpp
testlowmembamheaderparser_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testlowmembamheaderparser_CPPFLAGS = ${AM_CPPFLAGS}
testlowmembamheaderparser_LDFLAGS = ${AM_LDFLAGS}

testgeneflatfile_SOURCES = test/testgeneflatfile.cpp
testgeneflatfile_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testgeneflatfile_CPPFLAGS = ${AM_CPPFLAGS}
testgeneflatfile_LDFLAGS = ${AM_LDFLAGS}

testgnutls_SOURCES = test/testgnutls.cpp
testgnutls_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testgnutls_CPPFLAGS = ${AM_CPPFLAGS} @GNUTLSCPPFLAGS@
testgnutls_LDFLAGS = ${AM_LDFLAGS} @GNUTLSLIBS@

testparallelbamsorting_SOURCES = test/testparallelbamsorting.cpp
testparallelbamsorting_CPPFLAGS = @SNAPPYCPPFLAGS@
testparallelbamsorting_LDFLAGS = @SNAPPYLDFLAGS@
testparallelbamsorting_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@ ${DLLIB}

testparallelbamvalidation_SOURCES = test/testparallelbamvalidation.cpp
testparallelbamvalidation_CPPFLAGS = @SNAPPYCPPFLAGS@
testparallelbamvalidation_LDFLAGS = @SNAPPYLDFLAGS@
testparallelbamvalidation_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@ ${DLLIB}

testlinebuffer_SOURCES = test/testlinebuffer.cpp
testlinebuffer_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testlinebuffer_CPPFLAGS = ${AM_CPPFLAGS}
testlinebuffer_LDFLAGS = ${AM_LDFLAGS}

testsamdecoder_SOURCES = test/testsamdecoder.cpp
testsamdecoder_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testsamdecoder_CPPFLAGS = ${AM_CPPFLAGS}
testsamdecoder_LDFLAGS = ${AM_LDFLAGS}

testdecimalnumberparser_SOURCES = test/testdecimalnumberparser.cpp
testdecimalnumberparser_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testdecimalnumberparser_CPPFLAGS = ${AM_CPPFLAGS}
testdecimalnumberparser_LDFLAGS = ${AM_LDFLAGS}

testunsignedinteger_SOURCES = test/testunsignedinteger.cpp
testunsignedinteger_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testunsignedinteger_CPPFLAGS = ${AM_CPPFLAGS}
testunsignedinteger_LDFLAGS = ${AM_LDFLAGS}

testsha_SOURCES = test/testsha.cpp
testsha_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @NETTLELIBS@ libmaus2digests.a libmaus2digests_sse4.a
testsha_CPPFLAGS = ${AM_CPPFLAGS} @NETTLECPPFLAGS@
testsha_LDFLAGS = ${AM_LDFLAGS} @NETTLELDFLAGS@
testsha_DEPENDENCIES = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la libmaus2digests.a libmaus2digests_sse4.a

testmd5_SOURCES = test/testmd5.cpp
testmd5_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testmd5_CPPFLAGS = ${AM_CPPFLAGS}
testmd5_LDFLAGS = ${AM_LDFLAGS}

testsinglefileserver_SOURCES = test/testsinglefileserver.cpp
testsinglefileserver_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testsinglefileserver_CPPFLAGS = ${AM_CPPFLAGS}
testsinglefileserver_LDFLAGS = ${AM_LDFLAGS}

testparallelbamrewrite_SOURCES = test/testparallelbamrewrite.cpp
testparallelbamrewrite_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testparallelbamrewrite_CPPFLAGS = ${AM_CPPFLAGS}
testparallelbamrewrite_LDFLAGS = ${AM_LDFLAGS}

testparallelbamblocksort_SOURCES = test/testparallelbamblocksort.cpp
testparallelbamblocksort_LDADD = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@ ${DLLIB} libmaus2seqchksumsfactory.a libmaus2digests.a libmaus2digests_sse4.a @GMPLIBS@
testparallelbamblocksort_CPPFLAGS = ${AM_CPPFLAGS} ${CPPFLAGS} @WARNCPPFLAGS@ @SNAPPYCPPFLAGS@
testparallelbamblocksort_LDFLAGS = ${AM_LDFLAGS} ${LDFLAGS} @SNAPPYLDFLAGS@ @GMPLDFLAGS@

testhdf5_SOURCES  = test/testhdf5.cpp
testhdf5_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @HDF5LIBS@
testhdf5_CPPFLAGS = @HDF5CPPFLAGS@ @WARNCPPFLAGS@
testhdf5_LDFLAGS  = ${AM_LDFLAGS} @HDF5LDFLAGS@

testmapperm_SOURCES  = test/testmapperm.cpp
testmapperm_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testmapperm_CPPFLAGS = @WARNCPPFLAGS@
testmapperm_LDFLAGS  = ${AM_LDFLAGS}

testexternalmemoryindex_SOURCES  = test/testexternalmemoryindex.cpp
testexternalmemoryindex_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @SNAPPYLIBS@
testexternalmemoryindex_CPPFLAGS = @WARNCPPFLAGS@ @SNAPPYCPPFLAGS@
testexternalmemoryindex_LDFLAGS  = ${AM_LDFLAGS} @SNAPPYLDFLAGS@

testirods_SOURCES  = test/testirods.cpp
testirods_LDADD    = libmaus2_irods.a libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la @IRODSLIBS@ 
testirods_CPPFLAGS = @IRODSCPPFLAGS@ @WARNCPPFLAGS@
testirods_LDFLAGS  = ${AM_LDFLAGS} @IRODSLDFLAGS@
testirods_DEPENDENCIES = libmaus2_irods.a libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la

testfastqtobam_SOURCES  = test/testfastqtobam.cpp
testfastqtobam_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la
testfastqtobam_CPPFLAGS = @WARNCPPFLAGS@
testfastqtobam_LDFLAGS  = ${AM_LDFLAGS}

testsamrecoding_SOURCES  = test/testsamrecoding.cpp
testsamrecoding_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testsamrecoding_CPPFLAGS = @WARNCPPFLAGS@
testsamrecoding_LDFLAGS  = ${AM_LDFLAGS}

testdazzlerdb_SOURCES  = test/testdazzlerdb.cpp
testdazzlerdb_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB} @DALIGNERLIBS@
testdazzlerdb_CPPFLAGS = @WARNCPPFLAGS@ ${AM_CPPFLAGS} @DALIGNERCPPFLAGS@
testdazzlerdb_LDFLAGS  = ${AM_LDFLAGS} @DALIGNERLDFLAGS@

testnd_SOURCES  = test/testnd.cpp
testnd_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testnd_CPPFLAGS = @WARNCPPFLAGS@
testnd_LDFLAGS  = ${AM_LDFLAGS}

testfastashred_SOURCES  = test/testfastashred.cpp
testfastashred_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testfastashred_CPPFLAGS = @WARNCPPFLAGS@
testfastashred_LDFLAGS  = ${AM_LDFLAGS}

testalignerspeed_SOURCES  = test/testalignerspeed.cpp
testalignerspeed_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testalignerspeed_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testfastasingleline_SOURCES  = test/testfastasingleline.cpp
testfastasingleline_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testfastasingleline_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testdazzleralignmentrewrite_SOURCES  = test/testdazzleralignmentrewrite.cpp
testdazzleralignmentrewrite_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testdazzleralignmentrewrite_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testxz_SOURCES  = test/testxz.cpp
testxz_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testxz_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testdazzlerindex_SOURCES  = test/testdazzlerindex.cpp
testdazzlerindex_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testdazzlerindex_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testargparser_SOURCES  = test/testargparser.cpp
testargparser_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testargparser_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testfragmentenvelope_SOURCES  = test/testfragmentenvelope.cpp
testfragmentenvelope_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testfragmentenvelope_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testdnarank_SOURCES  = test/testdnarank.cpp
testdnarank_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testdnarank_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testlowbits_SOURCES  = test/testlowbits.cpp
testlowbits_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testlowbits_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testfastaexplode_SOURCES  = test/testfastaexplode.cpp
testfastaexplode_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testfastaexplode_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testfaextract_SOURCES  = test/testfaextract.cpp
testfaextract_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testfaextract_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testmurmur3_SOURCES  = test/testmurmur3.cpp
testmurmur3_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testmurmur3_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testpairalign_SOURCES  = test/testpairalign.cpp
testpairalign_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testpairalign_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testsplaytree_SOURCES  = test/testsplaytree.cpp
testsplaytree_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testsplaytree_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testbwtconstruction_SOURCES  = test/testbwtconstruction.cpp
testbwtconstruction_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testbwtconstruction_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testbwtconstructionfasta_SOURCES  = test/testbwtconstructionfasta.cpp
testbwtconstructionfasta_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testbwtconstructionfasta_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testarrayinput_SOURCES  = test/testarrayinput.cpp
testarrayinput_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testarrayinput_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testhugepages_SOURCES  = test/testhugepages.cpp
testhugepages_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testhugepages_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testsymcnt_SOURCES  = test/testsymcnt.cpp
testsymcnt_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testsymcnt_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testgammapd_SOURCES  = test/testgammapd.cpp
testgammapd_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testgammapd_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@

testpo_SOURCES  = test/testpo.cpp
testpo_LDADD    = libmaus2.la libmaus2_simd_align_128.la libmaus2_simd_align_256.la ${DLLIB}
testpo_CPPFLAGS = ${AM_CPPFLAGS} @WARNCPPFLAGS@
