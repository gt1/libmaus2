ACLOCAL_AMFLAGS=-I m4

abs_builddir = @abs_builddir@

lib_LTLIBRARIES = libmaus.la libmauslsf.la 

libmaus_modules_LTLIBRARIES = libmaus_kmlocal_mod.la libmaus_consensus_mod.la libmaus_scram_mod.la
libmaus_modulesdir = $(libdir)/libmaus/$(PACKAGE_VERSION)

libmaus_la_SOURCES = libmaus/StaticInitialization.cpp \
	libmaus/autoarray/AutoArray.cpp \
	libmaus/bitio/CompactArray.cpp libmaus/rank/RankTable.cpp \
	libmaus/rank/log2table.cpp libmaus/fastx/isFastQ.cpp libmaus/wavelet/toWaveletTreeBits.cpp \
	libmaus/suffixsort/divsufsort.cpp libmaus/rmq/FischerSystematicSuccinctRMQ.cpp libmaus/lcp/OracleLCP.cpp \
	libmaus/util/StackTrace.cpp libmaus/lcs/OverlapComputation.cpp \
	libmaus/lcs/HashContainer.cpp \
	libmaus/lcs/HashContainer2.cpp \
	libmaus/util/md5.cpp libmaus/util/I386CacheLineSize.cpp \
	libmaus/math/primes16.cpp \
	libmaus/util/OutputFileNameTools.cpp \
	libmaus/select/ESelectBase.cpp \
	libmaus/bambam/CigarStringParser.cpp \
	libmaus/consensus/Consensus.cpp \
	libmaus/graph/EdgeList.cpp \
	libmaus/fastx/Phred.cpp \
	libmaus/network/sendReceiveFd.c \
	libmaus/quantisation/ClusterComputation.cpp \
	libmaus/lz/SnappyCompress.cpp \
	libmaus/bambam/CollatingBamDecoder.cpp \
	libmaus/bambam/ReadEndsContainer.cpp \
	libmaus/bambam/ReadEnds.cpp \
	libmaus/util/TempFileRemovalContainer.cpp \
	libmaus/network/LogReceiver.cpp \
	libmaus/huffman/IndexLoaderBase.cpp \
	libmaus/bambam/BamAlignmentDecoderBase.cpp \
	libmaus/bambam/StrCmpNum.cpp \
	libmaus/huffman/KvInitResult.cpp \
	libmaus/fm/MausFmToBwaConversion.cpp \
	libmaus/util/TempFileNameGenerator.cpp \
	libmaus/util/TempFileNameGeneratorState.cpp \
	libmaus/util/ArgInfo.cpp \
	libmaus/util/Array832.cpp \
	libmaus/util/Array864.cpp \
	libmaus/util/BitList.cpp \
	libmaus/util/VarBitList.cpp \
	libmaus/util/Concat.cpp \
	libmaus/util/ConcatRequest.cpp \
	libmaus/util/ConsoleColor.cpp \
	libmaus/util/Demangle.cpp \
	libmaus/util/DigitTable.cpp \
	libmaus/util/DynamicLoading.cpp \
	libmaus/util/ForkProcess.cpp \
	libmaus/util/LSFSim.cpp \
	libmaus/util/ForkProcessLSFSim.cpp \
	libmaus/util/GetFileSize.cpp \
	libmaus/util/Histogram.cpp \
	libmaus/util/HistogramSet.cpp \
	libmaus/util/IncreasingList.cpp \
	libmaus/util/IntervalTree.cpp \
	libmaus/util/GenericIntervalTree.cpp \
	libmaus/util/LogPipe.cpp \
	libmaus/util/LogPipeMultiplex.cpp \
	libmaus/util/LogPipeMultiplexGeneric.cpp \
	libmaus/util/LookupIntervalTree.cpp \
	libmaus/util/md5_interface.cpp \
	libmaus/util/MemLimit.cpp \
	libmaus/util/MemUsage.cpp \
	libmaus/util/NearestNeighbourDictionary.cpp \
	libmaus/util/NegativeDifferenceArray32.cpp \
	libmaus/util/NegativeDifferenceArray64.cpp \
	libmaus/util/nstat.cpp \
	libmaus/util/PositiveDifferenceArray32.cpp \
	libmaus/util/PositiveDifferenceArray64.cpp \
	libmaus/util/PosixExecute.cpp \
	libmaus/util/PosixFileDescriptor.cpp \
	libmaus/util/PosixInputFile.cpp \
	libmaus/util/SaturatingCounter.cpp \
	libmaus/util/StringSerialisation.cpp \
	libmaus/util/SuccinctBorderArray.cpp \
	libmaus/util/Terminal.cpp \
	libmaus/util/OctetString.cpp \
	libmaus/util/Utf8String.cpp \
	libmaus/util/Utf8BlockIndexDecoder.cpp \
	libmaus/util/Utf8BlockIndex.cpp \
	libmaus/util/Utf8DecoderBuffer.cpp \
	libmaus/util/Utf8DecoderWrapper.cpp \
	libmaus/suffixtree/CompressedSuffixTree.cpp \
	libmaus/bambam/AlignmentValidity.cpp \
	libmaus/bambam/BamFlagBase.cpp \
	libmaus/bambam/ReadGroup.cpp \
	libmaus/bambam/DuplicationMetrics.cpp \
	libmaus/bambam/BamDefaultAdapters.cpp \
	libmaus/bambam/BamRange.cpp \
	libmaus/graph/GraphEdge.cpp \
	libmaus/bambam/BamIndexBinChunk.cpp \
	libmaus/bambam/BamIndexLinearChunk.cpp \
	libmaus/huffman/HuffmanTree.cpp \
	libmaus/bambam/BamAlignmentDecoderInfo.cpp \
	libmaus/lz/Lz4Base.cpp \
	libmaus/lz/Lz4Compress.cpp \
	libmaus/lz/Lz4DecoderBuffer.cpp \
	libmaus/lz/lz4.c \
	libmaus/fm/BidirectionalIndexInterval.cpp  \
	libmaus/fm/BidirectionalIndexIntervalSymbol.cpp \
	libmaus/gamma/SparseGammaGapFileIndexDecoderEntry.cpp \
	libmaus/fastx/FastALineParserLineInfo.cpp \
	libmaus/fastx/FastAMatchTable.cpp \
	libmaus/lcs/EditDistanceResult.cpp \
	libmaus/fastx/FastABgzfIndexEntry.cpp \
	libmaus/fastx/FastABgzfIndex.cpp \
	libmaus/parallel/ThreadWorkPackage.cpp \
	libmaus/parallel/SimpleThreadWorkPackage.cpp \
	libmaus/lz/RAZFIndex.cpp \
	libmaus/fastx/FastAIndexEntry.cpp \
	libmaus/fastx/FastAIndex.cpp \
	libmaus/network/HttpAbsoluteUrl.cpp \
	libmaus/hashing/Crc32.cpp \
	libmaus/lz/IGzipDeflate.cpp \
	libmaus/lcs/LocalBaseConstants.cpp \
	libmaus/lcs/LocalEditDistanceResult.cpp \
	libmaus/bambam/BamHeader.cpp \
	libmaus/bambam/GeneFlatFile.cpp \
	libmaus/bambam/GeneFlatFileEntry.cpp \
	libmaus/trie/SimpleTrie.cpp \
	libmaus/network/GnuTLSInit.cpp \
	libmaus/network/GnuTLSSocket.cpp

noinst_HEADERS = libmaus/lz/lz4.h

libmaus_la_CPPFLAGS = @LIBMAUSCPPFLAGS@ @SNAPPYCPPFLAGS@ @ZLIBCPPFLAGS@ @WARNCPPFLAGS@ @LIBCURLCPPFLAGS@ @OPENSSLCPPFLAGS@ @GNUTLSCPPFLAGS@ @IGZIPCPPFLAGS@
libmaus_la_LDFLAGS = @LIBMAUSLDFLAGS@ @SNAPPYLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@ @IGZIPLDFLAGS@ @GNUTLSLDFLAGS@
libmaus_la_LIBADD = @DLLIB@ @SNAPPYLIBS@ @ZLIBLIBS@ @LIBCURLLIBS@ @OPENSSLLIBS@ @IGZIPLIBS@ @GNUTLSLIBS@

libmauslsf_la_LDFLAGS = @LSFLDFLAGS@ @LIBMAUSLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@
libmauslsf_la_SOURCES = libmaus/lsf/LSFProcess.cpp
libmauslsf_la_CPPFLAGS = @WARNCPPFLAGS@ ${LSFCPPFLAGS}
libmauslsf_la_LIBADD = libmaus.la ${LSFLIBS}

libmaus_kmlocal_mod_la_CPPFLAGS = @KMLOCALCPPFLAGS@ @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS} @WARNCPPFLAGS@
libmaus_kmlocal_mod_la_CXXFLAGS = @KMLOCALCXXFLAGS@ @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus_kmlocal_mod_la_LDFLAGS  = @KMLOCALLDFLAGS@  @LIBMAUSLDFLAGS@                 -module  -shared -export-dynamic ${AM_LDFLAGS} -avoid-version
libmaus_kmlocal_mod_la_LIBADD   = @KMLOCALLIBS@     @LIBMAUSLIBS@
libmaus_kmlocal_mod_la_SOURCES  = libmaus/quantisation/KMLocalInterface.cpp

libmaus_consensus_mod_la_CPPFLAGS = @SEQANCPPFLAGS@                 @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS} @WARNCPPFLAGS@
libmaus_consensus_mod_la_CXXFLAGS = @SEQANCXXFLAGS@                 @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus_consensus_mod_la_LDFLAGS  = @SEQANLDFLAGS@  @SEQANLIBS@     @LIBMAUSLDFLAGS@  @LIBMAUSLIBS@     -module  -shared -export-dynamic ${AM_LDFLAGS} -avoid-version
libmaus_consensus_mod_la_LIBADD   = @SEQANLIBS@ @LIBMAUSLIBS@
libmaus_consensus_mod_la_SOURCES  = libmaus/consensus/ConsensusBase.cpp libmaus/consensus/ScoringMatrix.cpp \
	libmaus/fastx/Phred.cpp

libmaus_scram_mod_la_CPPFLAGS = @IOLIBCPPFLAGS@ @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS} @WARNCPPFLAGS@
libmaus_scram_mod_la_CXXFLAGS = @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus_scram_mod_la_LDFLAGS  = @IOLIBLDFLAGS@  @IOLIBLIBS@     @LIBMAUSLDFLAGS@  @LIBMAUSLIBS@     -module  -shared -export-dynamic ${AM_LDFLAGS} -avoid-version
libmaus_scram_mod_la_LIBADD   = @IOLIBLIBS@ @LIBMAUSLIBS@
libmaus_scram_mod_la_SOURCES  = libmaus/bambam/Scram.c

libmaus_includedir=$(includedir)/libmaus/
libmaus_include_HEADERS=libmaus/LibMausConfig.hpp

libmausautoarray_includedir=$(includedir)/libmaus/autoarray/
libmausautoarray_include_HEADERS = libmaus/autoarray/AutoArray.hpp libmaus/autoarray/AutoArray2d.hpp

libmausgamma_includedir=$(includedir)/libmaus/gamma/
libmausgamma_include_HEADERS = libmaus/gamma/GammaEncoder.hpp libmaus/gamma/GammaDecoder.hpp \
	libmaus/gamma/GammaGapEncoder.hpp libmaus/gamma/GammaGapDecoder.hpp \
	libmaus/gamma/GammaRLDecoder.hpp  libmaus/gamma/GammaRLEncoder.hpp \
	libmaus/gamma/SparseGammaGapEncoder.hpp libmaus/gamma/SparseGammaGapDecoder.hpp libmaus/gamma/SparseGammaGapMerge.hpp \
	libmaus/gamma/SparseGammaGapFile.hpp \
	libmaus/gamma/SparseGammaGapMultiFile.hpp \
	libmaus/gamma/SparseGammaGapFileSet.hpp \
	libmaus/gamma/SparseGammaGapFileLevelSet.hpp \
	libmaus/gamma/SparseGammaGapFileIndexDecoderEntry.hpp \
	libmaus/gamma/SparseGammaGapFileIndexDecoder.hpp \
	libmaus/gamma/SparseGammaGapFileIndexMultiDecoder.hpp \
	libmaus/gamma/SparseGammaGapConcatDecoder.hpp \
	libmaus/gamma/SparseGammaGapMultiFileSet.hpp \
	libmaus/gamma/SparseGammaGapMultiFileLevelSet.hpp \
	libmaus/gamma/SparseGammaGapBlockEncoder.hpp

libmausregex_includedir=$(includedir)/libmaus/regex/
libmausregex_include_HEADERS = libmaus/regex/PosixRegex.hpp

libmausserialize_includedir=$(includedir)/libmaus/serialize/
libmausserialize_include_HEADERS = libmaus/serialize/Serialize.hpp

libmaustypes_includedir=$(includedir)/libmaus/types/
libmaustypes_include_HEADERS = libmaus/types/types.hpp

libmauslz_includedir=$(includedir)/libmaus/lz/
libmauslz_include_HEADERS = libmaus/lz/Inflate.hpp libmaus/lz/Deflate.hpp libmaus/lz/GzipHeader.hpp libmaus/lz/StreamWrapper.hpp \
	libmaus/lz/GzipSingleStream.hpp libmaus/lz/GzipStream.hpp libmaus/lz/GzipStreamWrapper.hpp \
	libmaus/lz/BufferedGzipStream.hpp libmaus/lz/StdOstreamSink.hpp libmaus/lz/IstreamSource.hpp \
	libmaus/lz/SnappyCompress.hpp libmaus/lz/BgzfInflate.hpp libmaus/lz/BgzfInflateWrapper.hpp \
	libmaus/lz/BgzfInflateStream.hpp \
	libmaus/lz/BgzfDeflate.hpp \
	libmaus/lz/BgzfInflateBase.hpp \
	libmaus/lz/BgzfInflateBlock.hpp \
	libmaus/lz/BgzfInflateParallel.hpp \
	libmaus/lz/BgzfInflateParallelWrapper.hpp \
	libmaus/lz/BgzfInflateParallelStream.hpp \
	libmaus/lz/BgzfDeflateBase.hpp \
	libmaus/lz/BgzfInflateBlockIdComparator.hpp \
	libmaus/lz/BgzfInflateBlockIdInfo.hpp \
	libmaus/lz/BgzfInflateParallelContext.hpp \
	libmaus/lz/BgzfInflateParallelThread.hpp \
	libmaus/lz/BgzfDeflateBlockIdInfo.hpp \
	libmaus/lz/BgzfDeflateBlockIdComparator.hpp \
	libmaus/lz/BgzfDeflateParallelContext.hpp \
	libmaus/lz/BgzfDeflateParallelThread.hpp \
	libmaus/lz/BgzfConstants.hpp \
	libmaus/lz/BgzfDeflateHeaderFunctions.hpp \
	libmaus/lz/BgzfDeflateInputBufferBase.hpp \
	libmaus/lz/BgzfDeflateOutputBufferBase.hpp \
	libmaus/lz/BgzfDeflateZStreamBase.hpp \
	libmaus/lz/BgzfInflateHeaderBase.hpp \
	libmaus/lz/BgzfInflateZStreamBase.hpp \
	libmaus/lz/BgzfRecode.hpp \
	libmaus/lz/BgzfThreadOpBase.hpp \
	libmaus/lz/BgzfInflateDeflateParallel.hpp \
	libmaus/lz/BgzfInflateDeflateParallelThread.hpp \
	libmaus/lz/BgzfDeflateParallel.hpp \
	libmaus/lz/BgzfVirtualOffset.hpp \
	libmaus/lz/BgzfInflateDeflateParallelWrapper.hpp \
	libmaus/lz/BgzfInflateDeflateParallelInputStream.hpp \
	libmaus/lz/BgzfDeflateOutputCallback.hpp \
	libmaus/lz/BgzfDeflateOutputCallbackMD5.hpp \
	libmaus/lz/BgzfParallelRecodeDeflateBase.hpp \
	libmaus/lz/BgzfRecodeParallel.hpp \
	libmaus/lz/BgzfDeflateZStreamBaseFlushInfo.hpp \
	libmaus/lz/Lz4Base.hpp \
	libmaus/lz/Lz4CompressStreamBuffer.hpp \
	libmaus/lz/Lz4CompressWrapper.hpp \
	libmaus/lz/Lz4Decoder.hpp \
	libmaus/lz/Lz4Index.hpp \
	libmaus/lz/Lz4Compress.hpp \
	libmaus/lz/Lz4CompressStream.hpp \
	libmaus/lz/Lz4DecoderBuffer.hpp \
	libmaus/lz/GzipOutputStreamBuffer.hpp \
	libmaus/lz/GzipOutputStream.hpp \
	libmaus/lz/BgzfInflateInfo.hpp \
	libmaus/lz/BgzfStreamWrapper.hpp \
	libmaus/lz/CompressorObject.hpp \
	libmaus/lz/CompressorObjectFactory.hpp \
	libmaus/lz/SimpleCompressedOutputStream.hpp \
	libmaus/lz/SnappyCompressorObject.hpp \
	libmaus/lz/SnappyCompressorObjectFactory.hpp \
	libmaus/lz/DecompressorObject.hpp \
	libmaus/lz/DecompressorObjectFactory.hpp \
	libmaus/lz/SnappyDecompressorObject.hpp \
	libmaus/lz/SnappyDecompressorObjectFactory.hpp \
	libmaus/lz/SimpleCompressedInputStream.hpp \
	libmaus/lz/SimpleCompressedConcatInputStreamFragment.hpp \
	libmaus/lz/SimpleCompressedConcatInputStream.hpp \
	libmaus/lz/SnappyInputStreamArrayFile.hpp \
	libmaus/lz/SnappyInputStreamArray.hpp \
	libmaus/lz/SnappyStringInputStream.hpp \
	libmaus/lz/SnappyOffsetFileInputStream.hpp \
	libmaus/lz/SnappyFileInputStream.hpp \
	libmaus/lz/SnappyInputStream.hpp \
	libmaus/lz/SnappyFileOutputStream.hpp \
	libmaus/lz/SnappyOutputStream.hpp \
	libmaus/lz/SimpleCompressedStreamInterval.hpp \
	libmaus/lz/ZlibCompressorObjectFactory.hpp \
	libmaus/lz/ZlibDecompressorObjectFactory.hpp \
	libmaus/lz/ZlibCompressorObject.hpp \
	libmaus/lz/ZlibDecompressorObject.hpp \
	libmaus/lz/SimpleCompressedStreamNamedInterval.hpp \
	libmaus/lz/SimpleCompressedInputBlock.hpp \
	libmaus/lz/SimpleCompressedInputBlockConcat.hpp \
	libmaus/lz/SimpleCompressedInputBlockConcatBlock.hpp \
	libmaus/lz/RAZFConstants.hpp \
	libmaus/lz/RAZFDecoderBuffer.hpp \
	libmaus/lz/RAZFDecoder.hpp \
	libmaus/lz/RAZFIndex.hpp \
	libmaus/lz/IGzipDeflate.hpp \
	libmaus/lz/IsGzip.hpp

libmausbambam_includedir=$(includedir)/libmaus/bambam/
libmausbambam_include_HEADERS = libmaus/bambam/DecoderBase.hpp libmaus/bambam/Chromosome.hpp \
	libmaus/bambam/BamAlignmentDecoderBase.hpp \
	libmaus/bambam/BamFormatAuxiliary.hpp \
	libmaus/bambam/EncoderBase.hpp \
	libmaus/bambam/BamHeader.hpp \
	libmaus/bambam/BamAlignment.hpp \
	libmaus/bambam/BamDecoder.hpp \
	libmaus/bambam/BamAlignmentComparator.hpp \
	libmaus/bambam/MergeQueueElement.hpp \
	libmaus/bambam/CollatingBamDecoder.hpp \
	libmaus/bambam/CigarOperation.hpp \
	libmaus/bambam/CigarStringParser.hpp \
	libmaus/bambam/BamSeqEncodeTable.hpp \
	libmaus/bambam/BamAlignmentEncoderBase.hpp \
	libmaus/bambam/BamFlagBase.hpp \
	libmaus/bambam/BamWriter.hpp \
	libmaus/bambam/HeaderLine.hpp \
	libmaus/bambam/ProgramHeaderLineSet.hpp \
	libmaus/bambam/ReadEnds.hpp \
	libmaus/bambam/ReadEndsHeapPairComparator.hpp \
	libmaus/bambam/CompactReadEndsBase.hpp \
	libmaus/bambam/CompactReadEndsComparator.hpp \
	libmaus/bambam/SortedFragDecoder.hpp \
	libmaus/bambam/ReadEndsContainer.hpp \
	libmaus/bambam/DuplicationMetrics.hpp \
	libmaus/bambam/OpticalComparator.hpp \
	libmaus/bambam/BamAlignmentHeapComparator.hpp \
	libmaus/bambam/BamAlignmentNameComparator.hpp \
	libmaus/bambam/BamAlignmentPosComparator.hpp \
	libmaus/bambam/BamEntryContainer.hpp \
	libmaus/bambam/Scram.h \
	libmaus/bambam/AlignmentValidity.hpp \
	libmaus/bambam/ScramDecoder.hpp \
	libmaus/bambam/ScramEncoder.hpp \
	libmaus/bambam/SnappyAlignmentMergeInput.hpp \
	libmaus/bambam/BamAlignmentPrinter.hpp \
	libmaus/bambam/BamAlignmentSortingCircularHashEntryOverflow.hpp \
	libmaus/bambam/CircularHashCollatingBamDecoder.hpp \
	libmaus/bambam/BamToFastqOutputFileSet.hpp \
	libmaus/bambam/BamAlignmentDecoder.hpp \
	libmaus/bambam/CollatingBamDecoderAlignmentInputCallback.hpp \
	libmaus/bambam/ReadGroup.hpp \
	libmaus/bambam/BamAuxFilterVector.hpp \
	libmaus/bambam/BamDefaultAdapters.hpp \
	libmaus/bambam/AdapterOffsetStrand.hpp \
	libmaus/bambam/AdapterFilter.hpp \
	libmaus/bambam/BamIndexBin.hpp \
	libmaus/bambam/BamIndexLinear.hpp \
	libmaus/bambam/BamIndexRef.hpp \
	libmaus/bambam/BamIndex.hpp \
	libmaus/bambam/BamRange.hpp \
	libmaus/bambam/BamRangeParser.hpp \
	libmaus/bambam/BamRangeDecoder.hpp \
	libmaus/bambam/BamParallelRewrite.hpp \
	libmaus/bambam/BamAlignmentExpungeCallback.hpp \
	libmaus/bambam/StrCmpNum.hpp \
	libmaus/bambam/BamCatHeader.hpp \
	libmaus/bambam/BamCat.hpp \
	libmaus/bambam/BamMergeCoordinate.hpp \
	libmaus/bambam/BamMergeTemplate.hpp \
	libmaus/bambam/BamMergeQueryName.hpp \
	libmaus/bambam/BamIndexBinChunk.hpp \
	libmaus/bambam/BamIndexLinearChunk.hpp \
	libmaus/bambam/BamIndexGenerator.hpp \
	libmaus/bambam/BgzfDeflateOutputCallbackBamIndex.hpp \
	libmaus/bambam/BamAlignmentInputCallbackBam.hpp \
	libmaus/bambam/BamAlignmentInputCallbackSnappy.hpp \
	libmaus/bambam/BamHeaderUpdate.hpp \
	libmaus/bambam/DupSetCallback.hpp \
	libmaus/bambam/DupSetCallbackSet.hpp \
	libmaus/bambam/DupSetCallbackStream.hpp \
	libmaus/bambam/DupSetCallbackVector.hpp \
	libmaus/bambam/BamAlignmentListNode.hpp \
	libmaus/bambam/BamAlignmentPairListNode.hpp \
	libmaus/bambam/ReadEndsListNode.hpp \
	libmaus/bambam/BamAlignmentFreeList.hpp \
	libmaus/bambam/BamAlignmentPairFreeList.hpp \
	libmaus/bambam/ReadEndsFreeList.hpp \
	libmaus/bambam/DupMarkBase.hpp \
	libmaus/bambam/BamAlignmentSnappyInput.hpp \
	libmaus/bambam/BamAlignmentInputPositionUpdateCallback.hpp \
	libmaus/bambam/BamAlignmentInputPositionCallbackNull.hpp \
	libmaus/bambam/BamBlockWriterBase.hpp \
	libmaus/bambam/BamBlockWriterBaseFactory.hpp \
	libmaus/bambam/BamAlignmentDecoderInfo.hpp \
	libmaus/bambam/BamAlignmentDecoderFactory.hpp \
	libmaus/bambam/BamMultiAlignmentDecoderFactory.hpp \
	libmaus/bambam/BamIndexMetaInfo.hpp \
	libmaus/bambam/MdStringComputationContext.hpp \
	libmaus/bambam/BamAlignmentFilter.hpp \
	libmaus/bambam/BamAlignmentReadGroupFilter.hpp \
	libmaus/bambam/MdNmRecalculation.hpp \
	libmaus/bambam/CramRange.hpp \
	libmaus/bambam/ChromosomeVectorMerge.hpp \
	libmaus/bambam/BamHeaderLowMem.hpp \
	libmaus/bambam/GeneFlatFile.hpp \
	libmaus/bambam/GeneFlatFileEntry.hpp

libmausarithmetic_includedir=$(includedir)/libmaus/arithmetic/
libmausarithmetic_include_HEADERS =\
	libmaus/arithmetic/ArithCoder.hpp \
	libmaus/arithmetic/ArithEncoder.hpp \
	libmaus/arithmetic/ArithDecoder.hpp \
	libmaus/arithmetic/RangeCoder.hpp \
	libmaus/arithmetic/RangeEncoder.hpp \
	libmaus/arithmetic/RangeDecoder.hpp

libmausbitio_includedir=$(includedir)/libmaus/bitio
libmausbitio_include_HEADERS=\
	libmaus/bitio/BitIO.hpp \
	libmaus/bitio/BitIOOutput.hpp \
	libmaus/bitio/CompactArray.hpp \
	libmaus/bitio/CompactArrayBase.hpp \
	libmaus/bitio/OutputBuffer.hpp \
	libmaus/bitio/BitIOInput.hpp \
	libmaus/bitio/getBits.hpp \
	libmaus/bitio/ReverseTable.hpp \
	libmaus/bitio/CompactQueue.hpp \
	libmaus/bitio/putBits.hpp \
	libmaus/bitio/SwapWordBitBlock.hpp \
	libmaus/bitio/swapBitBlocksByReversal.hpp \
	libmaus/bitio/putBit.hpp \
	libmaus/bitio/getBit.hpp \
	libmaus/bitio/readElias.hpp \
	libmaus/bitio/BitWriter.hpp \
	libmaus/bitio/CheckedBitWriter.hpp \
	libmaus/bitio/FastWriteBitWriter.hpp \
	libmaus/bitio/BitVector.hpp \
	libmaus/bitio/SignedCompactArray.hpp \
	libmaus/bitio/BufferIterator.hpp \
	libmaus/bitio/BitVectorConcat.hpp \
	libmaus/bitio/CompactSparseArray.hpp \
	libmaus/bitio/CompactOffsetArray.hpp \
	libmaus/bitio/CompactOffsetOneArray.hpp \
	libmaus/bitio/ArrayDecode.hpp \
	libmaus/bitio/BitStreamFileDecoder.hpp \
	libmaus/bitio/Clz.hpp \
	libmaus/bitio/CompactDecoderBuffer.hpp \
	libmaus/bitio/Ctz.hpp \
	libmaus/bitio/CompactArrayWriter.hpp \
	libmaus/bitio/CompactArrayWriterFile.hpp \
	libmaus/bitio/PacDecoderBuffer.hpp \
	libmaus/bitio/MarkerFastWriteBitWriter.hpp \
	libmaus/bitio/BitVectorInput.hpp \
	libmaus/bitio/BitVectorOutput.hpp

libmausrank_includedir=$(includedir)/libmaus/rank
libmausrank_include_HEADERS=\
	libmaus/rank/ChooseCache.hpp \
	libmaus/rank/CodeBase.hpp \
	libmaus/rank/DecodeCache.hpp \
	libmaus/rank/ERank222BAppend.hpp \
	libmaus/rank/ERank222B.hpp \
	libmaus/rank/ERank222BP.hpp \
	libmaus/rank/ERank222.hpp \
	libmaus/rank/ERank22B.hpp \
	libmaus/rank/ERank22.hpp \
	libmaus/rank/ERank2.hpp \
	libmaus/rank/ERank3C.hpp \
	libmaus/rank/ERank3.hpp \
	libmaus/rank/ERankBase.hpp \
	libmaus/rank/ERank.hpp \
	libmaus/rank/log2table.hpp \
	libmaus/rank/popcnt.hpp \
	libmaus/rank/RankTable.hpp \
	libmaus/rank/Proc01.hpp \
	libmaus/rank/EncodeCache.hpp \
	libmaus/rank/CacheLineRank.hpp \
	libmaus/rank/ImpCacheLineRank.hpp \
	libmaus/rank/ERank222BBase.hpp \
	libmaus/rank/ERank222BAppendDynamic.hpp \
	libmaus/rank/RunLengthBitVectorBase.hpp \
	libmaus/rank/RunLengthBitVector.hpp \
	libmaus/rank/RunLengthBitVectorGenerator.hpp \
	libmaus/rank/RunLengthBitVectorStream.hpp \
	libmaus/rank/RunLengthBitVectorGeneratorBase.hpp \
	libmaus/rank/RunLengthBitVectorGeneratorGammaBase.hpp \
	libmaus/rank/FastRank.hpp
	
libmausselect_includedir=$(includedir)/libmaus/select
libmausselect_include_HEADERS=\
	libmaus/select/ESelect222B.hpp \
	libmaus/select/ESelectBase.hpp \
	libmaus/select/ESelectSimple.hpp \
	libmaus/select/ImpCacheLineSelectSupport.hpp

libmausdeleter_includedir=$(includedir)/libmaus/deleter
libmausdeleter_include_HEADERS=\
	libmaus/deleter/Deleter.hpp 

libmauswavelet_includedir=$(includedir)/libmaus/wavelet
libmauswavelet_include_HEADERS=\
	libmaus/wavelet/toWaveletTreeBits.hpp \
	libmaus/wavelet/WaveletTree.hpp \
	libmaus/wavelet/HuffmanWaveletTree.hpp \
	libmaus/wavelet/DynamicHuffmanWaveletTree.hpp \
	libmaus/wavelet/DynamicWaveletTree.hpp \
	libmaus/wavelet/ExternalWaveletGenerator.hpp \
	libmaus/wavelet/ImpWaveletTree.hpp \
	libmaus/wavelet/ImpExternalWaveletGenerator.hpp \
	libmaus/wavelet/ImpExternalWaveletGeneratorHuffmanParallel.hpp \
	libmaus/wavelet/ImpExternalWaveletGeneratorHuffman.hpp \
	libmaus/wavelet/ImpHuffmanWaveletTree.hpp \
	libmaus/wavelet/Utf8ToImpHuffmanWaveletTree.hpp \
	libmaus/wavelet/Utf8ToImpCompactHuffmanWaveletTree.hpp \
	libmaus/wavelet/ImpCompactHuffmanWaveletTree.hpp \
	libmaus/wavelet/ImpExternalWaveletGeneratorCompactHuffman.hpp \
	libmaus/wavelet/ImpExternalWaveletGeneratorCompactHuffmanParallel.hpp

libmaushuffman_includedir=$(includedir)/libmaus/huffman
libmaushuffman_include_HEADERS=\
	libmaus/huffman/CompressTable.hpp \
	libmaus/huffman/DecodeTable.hpp \
	libmaus/huffman/EncodeTable.hpp \
	libmaus/huffman/hufEncodeString.hpp \
	libmaus/huffman/HuffmanBase.hpp \
	libmaus/huffman/huffman.hpp \
	libmaus/huffman/HuffmanSorting.hpp \
	libmaus/huffman/HuffmanTreeInnerNode.hpp \
	libmaus/huffman/HuffmanTreeLeafComparator.hpp \
	libmaus/huffman/HuffmanTreeLeaf.hpp \
	libmaus/huffman/HuffmanTreeNode.hpp \
	libmaus/huffman/CanonicalEncoder.hpp \
	libmaus/huffman/RLDecoder.hpp \
	libmaus/huffman/RLEncoder.hpp \
	libmaus/huffman/HuffmanEncoderFile.hpp \
	libmaus/huffman/IndexLoader.hpp \
	libmaus/huffman/GapDecoder.hpp \
	libmaus/huffman/GapEncoder.hpp \
	libmaus/huffman/FileStreamBaseType.hpp \
	libmaus/huffman/BitInputBuffer.hpp \
	libmaus/huffman/PairAddSecond.hpp \
	libmaus/huffman/IndexEntry.hpp \
	libmaus/huffman/IndexLoaderBase.hpp \
	libmaus/huffman/RLDecoderIndexBase.hpp \
	libmaus/huffman/GapDecoderIndexBase.hpp \
	libmaus/huffman/IndexDecoderDataArray.hpp \
	libmaus/huffman/HuffmanInputFile.hpp \
	libmaus/huffman/KvInitResult.hpp \
	libmaus/huffman/HuffmanTree.hpp

libmausmath_includedir=$(includedir)/libmaus/math
libmausmath_include_HEADERS=\
	libmaus/math/isqrt.hpp \
	libmaus/math/bitsPerNum.hpp \
	libmaus/math/MetaLog.hpp \
	libmaus/math/lowbits.hpp \
	libmaus/math/numbits.hpp \
	libmaus/math/gcd.hpp \
	libmaus/math/binom.hpp \
	libmaus/math/primes16.hpp \
	libmaus/math/ilog.hpp \
	libmaus/math/DoubleCode.hpp \
	libmaus/math/iabs.hpp \
	libmaus/math/IPower.hpp \
	libmaus/math/Log.hpp \
	libmaus/math/ipow.hpp

libmausrl_includedir=$(includedir)/libmaus/rl
libmausrl_include_HEADERS=\
	libmaus/rl/RLIndex.hpp

libmausutil_includedir=$(includedir)/libmaus/util
libmausutil_include_HEADERS=\
	libmaus/util/stringFunctions.hpp \
	libmaus/util/BitList.hpp \
	libmaus/util/VarBitList.hpp \
	libmaus/util/VarList.hpp \
	libmaus/util/IntervalTree.hpp \
	libmaus/util/LookupIntervalTree.hpp \
	libmaus/util/StringSerialisation.hpp \
	libmaus/util/NumberSerialisation.hpp \
	libmaus/util/GetFileSize.hpp \
	libmaus/util/unique_ptr.hpp \
	libmaus/util/shared_ptr.hpp \
	libmaus/util/TempFileNameGenerator.hpp \
	libmaus/util/TempFileNameGeneratorState.hpp \
	libmaus/util/Demangle.hpp \
	libmaus/util/BitsPerNum.hpp \
	libmaus/util/ArgInfo.hpp \
	libmaus/util/Concat.hpp \
	libmaus/util/ConcatRequest.hpp \
	libmaus/util/PosixExecute.hpp \
	libmaus/util/StackTrace.hpp \
	libmaus/util/CompactNumberArray.hpp \
	libmaus/util/ArrayIterator.hpp \
	libmaus/util/OffsetArray.hpp \
	libmaus/util/Array832.hpp \
	libmaus/util/Array864.hpp \
	libmaus/util/NegativeDifferenceArray32.hpp \
	libmaus/util/NegativeDifferenceArray64.hpp \
	libmaus/util/PositiveDifferenceArray32.hpp \
	libmaus/util/PositiveDifferenceArray64.hpp \
	libmaus/util/UniqueIndex.hpp \
	libmaus/util/Terminal.hpp \
	libmaus/util/nstat.hpp \
	libmaus/util/WriteableString.hpp \
	libmaus/util/ForkProcess.hpp \
	libmaus/util/LogPipe.hpp \
	libmaus/util/MoveStack.hpp \
	libmaus/util/LongestIncreasingSubsequence.hpp \
	libmaus/util/utf8.hpp \
	libmaus/util/md5.hpp \
	libmaus/util/md5.h \
	libmaus/util/MemLimit.hpp \
	libmaus/util/Histogram.hpp \
	libmaus/util/IncreasingList.hpp \
	libmaus/util/GenericSerialisation.hpp \
	libmaus/util/ConsoleColor.hpp \
	libmaus/util/LogPipeMultiplex.hpp \
	libmaus/util/LogPipeMultiplexGeneric.hpp \
	libmaus/util/iterator.hpp \
	libmaus/util/SaturatingCounter.hpp \
	libmaus/util/I386CacheLineSize.hpp \
	libmaus/util/IncreasingStack.hpp \
	libmaus/util/NearestNeighbourDictionary.hpp \
	libmaus/util/MemUsage.hpp \
	libmaus/util/unordered_set.hpp \
	libmaus/util/unordered_map.hpp \
	libmaus/util/ReverseByteOrder.hpp \
	libmaus/util/SimpleBloomFilter.hpp \
	libmaus/util/SimpleCountingHash.hpp \
	libmaus/util/SimpleHashMap.hpp \
	libmaus/util/SimpleHashSet.hpp \
	libmaus/util/OutputFileNameTools.hpp \
	libmaus/util/LELoad.hpp \
	libmaus/util/GetObject.hpp \
	libmaus/util/PutObject.hpp \
	libmaus/util/PutObjectReverse.hpp \
	libmaus/util/CountPutObject.hpp \
	libmaus/util/CountPutIterator.hpp \
	libmaus/util/PosixFileDescriptor.hpp \
	libmaus/util/DigitTable.hpp \
	libmaus/util/TempFileRemovalContainer.hpp \
	libmaus/util/DynamicLoading.hpp \
	libmaus/util/Object.hpp \
	libmaus/util/KMP.hpp \
	libmaus/util/NumberMapSerialisation.hpp \
	libmaus/util/Utf8BlockIndex.hpp \
	libmaus/util/Utf8DecoderBuffer.hpp \
	libmaus/util/Utf8String.hpp \
	libmaus/util/UnsignedCharVariant.hpp \
	libmaus/util/LSFSim.hpp \
	libmaus/util/ForkProcessLSFSim.hpp \
	libmaus/util/CountGetObject.hpp \
	libmaus/util/HistogramSet.hpp \
	libmaus/util/GenericIntervalTree.hpp \
	libmaus/util/PosixInputFile.hpp \
	libmaus/util/OctetString.hpp \
	libmaus/util/Utf8StringPairAdapter.hpp \
	libmaus/util/Utf8BlockIndexDecoder.hpp \
	libmaus/util/Utf8DecoderWrapper.hpp \
	libmaus/util/TempFileContainer.hpp \
	libmaus/util/FileTempFileContainer.hpp \
	libmaus/util/MemTempFileContainer.hpp \
	libmaus/util/ImpCompactNumberArray.hpp \
	libmaus/util/ContainerGetObject.hpp \
	libmaus/util/PushBuffer.hpp \
	libmaus/util/SuccinctBorderArray.hpp \
	libmaus/util/LineAccessor.hpp \
	libmaus/util/StringMapCompare.hpp \
	libmaus/util/FreeList.hpp \
	libmaus/util/ToUpperTable.hpp \
	libmaus/util/BorderArray.hpp

libmausuint_includedir=$(includedir)/libmaus/uint
libmausuint_include_HEADERS=\
	libmaus/uint/uint.hpp 

libmausbp_includedir=$(includedir)/libmaus/bp
libmausbp_include_HEADERS=\
	libmaus/bp/BalancedParenthesesBase.hpp \
	libmaus/bp/BalancedParentheses.hpp

libmausbitbtree_includedir=$(includedir)/libmaus/bitbtree
libmausbitbtree_include_HEADERS=\
	libmaus/bitbtree/bitbtree.hpp

libmauscumfreq_includedir=$(includedir)/libmaus/cumfreq
libmauscumfreq_include_HEADERS=\
	libmaus/cumfreq/DynamicCumulativeFrequencies.hpp \
	libmaus/cumfreq/SlowCumFreq.hpp

libmauseta_includedir=$(includedir)/libmaus/eta
libmauseta_include_HEADERS=\
	libmaus/eta/LinearETA.hpp 

libmaustrie_includedir=$(includedir)/libmaus/trie
libmaustrie_include_HEADERS=\
	libmaus/trie/TrieState.hpp \
	libmaus/trie/SimpleTrie.hpp

libmausaio_includedir=$(includedir)/libmaus/aio
libmausaio_include_HEADERS=\
	libmaus/aio/AsynchronousBufferReader.hpp \
	libmaus/aio/AsynchronousWriter.hpp \
	libmaus/aio/AsynchronousReader.hpp \
	libmaus/aio/OutputBuffer8.hpp \
	libmaus/aio/OutputBuffer84Bit.hpp \
	libmaus/aio/TerminatedOutputBuffer84Bit.hpp \
	libmaus/aio/OutputBuffer.hpp \
	libmaus/aio/OutputFile8Array.hpp \
	libmaus/aio/GenericInput.hpp \
	libmaus/aio/MetaOutputFile8Array.hpp \
	libmaus/aio/SynchronousOutputFile8Array.hpp \
	libmaus/aio/MetaOutputBuffer8.hpp \
	libmaus/aio/SynchronousOutputBuffer8.hpp \
	libmaus/aio/BlockSynchronousOutputBuffer8.hpp \
	libmaus/aio/GenericOutput.hpp \
	libmaus/aio/MemInBuffer.hpp \
	libmaus/aio/MemOutBuffer.hpp \
	libmaus/aio/BufferedOutput.hpp \
	libmaus/aio/FileFragment.hpp \
	libmaus/aio/AsynchronousFastReaderBase.hpp \
	libmaus/aio/SynchronousFastReaderBase.hpp \
	libmaus/aio/LimitedGenericInput.hpp \
	libmaus/aio/SynchronousGenericInput.hpp \
	libmaus/aio/LimitedSynchronousGenericInput.hpp \
	libmaus/aio/ReorderConcatGenericInput.hpp \
	libmaus/aio/GetLength.hpp \
	libmaus/aio/CompactDecoder4.hpp \
	libmaus/aio/CompactEncoder4.hpp \
	libmaus/aio/WriteCompactArray.hpp \
	libmaus/aio/SynchronousGenericOutput.hpp \
	libmaus/aio/SynchronousOutputBuffer8Posix.hpp \
	libmaus/aio/SynchronousGenericOutputPosix.hpp \
	libmaus/aio/PutOutputIterator.hpp \
	libmaus/aio/CheckedInputStream.hpp \
	libmaus/aio/CheckedOutputStream.hpp \
	libmaus/aio/CircularBuffer.hpp \
	libmaus/aio/CircularWrapper.hpp \
	libmaus/aio/IStreamWrapper.hpp \
	libmaus/aio/CheckedInputStreamAtOffset.hpp \
	libmaus/aio/Buffer.hpp \
	libmaus/aio/SingleFileFragmentMerge.hpp \
	libmaus/aio/CheckedInputOutputStream.hpp \
	libmaus/aio/PosixFdInput.hpp \
	libmaus/aio/PosixFdInputWrapper.hpp \
	libmaus/aio/PosixFdInputStream.hpp \
	libmaus/aio/PosixFdOutputStreamBuffer.hpp \
	libmaus/aio/PosixFdOutputStream.hpp \
	libmaus/aio/PosixFdInputStreamBuffer.hpp \
	libmaus/aio/LinuxStreamingPosixFdOutputStreamBuffer.hpp \
	libmaus/aio/LinuxStreamingPosixFdOutputStream.hpp \
	libmaus/aio/IsKnownLocalFileSystem.hpp

libmausparallel_includedir=$(includedir)/libmaus/parallel
libmausparallel_include_HEADERS=\
	libmaus/parallel/OMPLock.hpp \
	libmaus/parallel/PosixMutex.hpp \
	libmaus/parallel/PosixSpinLock.hpp \
	libmaus/parallel/PosixSemaphore.hpp \
	libmaus/parallel/SynchronousQueue.hpp \
	libmaus/parallel/TerminatableSynchronousQueue.hpp \
	libmaus/parallel/PosixMutexArray.hpp \
	libmaus/parallel/PosixThread.hpp \
	libmaus/parallel/Pipe.hpp \
	libmaus/parallel/SynchronousHeap.hpp \
	libmaus/parallel/TerminatableSynchronousHeap.hpp \
	libmaus/parallel/SynchronousReorderSet.hpp \
	libmaus/parallel/TerminatableSynchronousReorderSet.hpp  \
	libmaus/parallel/SynchronousCounter.hpp \
	libmaus/parallel/PosixProcess.hpp \
	libmaus/parallel/OMPNumThreadsScope.hpp \
	libmaus/parallel/LockedBool.hpp \
	libmaus/parallel/ThreadWorkPackage.hpp \
	libmaus/parallel/ThreadWorkPackageComparator.hpp \
	libmaus/parallel/ThreadPoolInterfaceEnqueTermInterface.hpp \
	libmaus/parallel/ThreadWorkPackageDispatcher.hpp \
	libmaus/parallel/ThreadPoolInterface.hpp \
	libmaus/parallel/ThreadPoolThread.hpp \
	libmaus/parallel/ThreadPool.hpp \
	libmaus/parallel/SimpleThreadPool.hpp \
	libmaus/parallel/SimpleThreadPoolInterfaceEnqueTermInterface.hpp \
	libmaus/parallel/SimpleThreadPoolInterface.hpp \
	libmaus/parallel/SimpleThreadPoolThread.hpp \
	libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp \
	libmaus/parallel/SimpleThreadWorkPackageComparator.hpp \
	libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp \
	libmaus/parallel/SimpleThreadWorkPackage.hpp \
	libmaus/parallel/LockedQueue.hpp \
	libmaus/parallel/NumCpus.hpp

libmausfastx_includedir=$(includedir)/libmaus/fastx
libmausfastx_include_HEADERS=\
	libmaus/fastx/CharTermTable.hpp \
	libmaus/fastx/SpaceTable.hpp \
	libmaus/fastx/CharBuffer.hpp \
	libmaus/fastx/Pattern.hpp \
	libmaus/fastx/acgtnMap.hpp \
	libmaus/fastx/FastAReader.hpp \
	libmaus/fastx/PatternBlock.hpp \
	libmaus/fastx/FastIDBlock.hpp \
	libmaus/fastx/FastQReader.hpp \
	libmaus/fastx/FASTQEntry.hpp \
	libmaus/fastx/isFastQ.hpp \
	libmaus/fastx/MultiWordDNABitBuffer.hpp \
	libmaus/fastx/SingleWordDNABitBuffer.hpp \
	libmaus/fastx/FastInterval.hpp \
	libmaus/fastx/FastAReaderSplit.hpp \
	libmaus/fastx/FastQReaderSplit.hpp \
	libmaus/fastx/ConcatWriter.hpp \
	libmaus/fastx/FastFirstToLastReverseAdapter.hpp \
	libmaus/fastx/FastServer.hpp \
	libmaus/fastx/FastClient.hpp \
	libmaus/fastx/CompactFastDecoderBase.hpp \
	libmaus/fastx/CompactFastDecoder.hpp \
	libmaus/fastx/CompactFastEncoder.hpp \
	libmaus/fastx/CompactReadContainer.hpp \
	libmaus/fastx/SocketFastAReader.hpp \
	libmaus/fastx/SocketFastQReader.hpp \
	libmaus/fastx/SubSamplingFastReader.hpp \
	libmaus/fastx/SlashStrip.hpp \
	libmaus/fastx/FirstWhiteSpaceSlashStrip.hpp \
	libmaus/fastx/Phred.hpp \
	libmaus/fastx/FastQElement.hpp \
	libmaus/fastx/ComputeFastXIntervals.hpp \
	libmaus/fastx/FqWeightQuantiser.hpp \
	libmaus/fastx/CompactFastQHeader.hpp \
	libmaus/fastx/CompactFastQContext.hpp \
	libmaus/fastx/CompactFastQDecoderBase.hpp \
	libmaus/fastx/CompactFastQContainerDictionary.hpp \
	libmaus/fastx/CompactFastQContainer.hpp \
	libmaus/fastx/CompactFastQContainerDictionaryCreator.hpp \
	libmaus/fastx/CompactFastQBlockGenerator.hpp \
	libmaus/fastx/CompactFastQSingleBlockReader.hpp \
	libmaus/fastx/CompactFastQMultiBlockReader.hpp \
	libmaus/fastx/CompactFastTerminator.hpp \
	libmaus/fastx/CompactFastReplace.hpp \
	libmaus/fastx/QReorder.hpp \
	libmaus/fastx/StreamFastReaderBase.hpp \
	libmaus/fastx/StreamFastQReader.hpp \
	libmaus/fastx/GzipStreamFastQReader.hpp \
	libmaus/fastx/GzipFileFastQReader.hpp \
	libmaus/fastx/FastQBgzfWriter.hpp \
	libmaus/fastx/StreamFastAReader.hpp \
	libmaus/fastx/FastALineParserLineInfo.hpp \
	libmaus/fastx/FastADefinedBasesTable.hpp \
	libmaus/fastx/FastAMapTable.hpp \
	libmaus/fastx/FastAMatchTable.hpp \
	libmaus/fastx/FastALineParser.hpp \
	libmaus/fastx/FastAStreamWrapper.hpp \
	libmaus/fastx/FastAStream.hpp \
	libmaus/fastx/FastAStreamSet.hpp \
	libmaus/fastx/BgzfFastAIndexEntry.hpp \
	libmaus/fastx/BgzfFastAIndexEntryNameComparator.hpp \
	libmaus/fastx/BgzfFastAIndexEntryIdComparator.hpp \
	libmaus/fastx/FastABgzfIndexEntry.hpp \
	libmaus/fastx/FastABgzfIndex.hpp \
	libmaus/fastx/FastABgzfDecoder.hpp \
	libmaus/fastx/FastABgzfDecoderBuffer.hpp \
	libmaus/fastx/FastAIndexEntry.hpp \
	libmaus/fastx/FastAIndex.hpp \
	libmaus/fastx/FastAInfo.hpp \
	libmaus/fastx/FastATwoBitTable.hpp

libmaustiming_includedir=$(includedir)/libmaus/timing
libmaustiming_include_HEADERS=\
	libmaus/timing/RealTimeClock.hpp

libmausexception_includedir=$(includedir)/libmaus/exception
libmausexception_include_HEADERS=\
	libmaus/exception/LibMausException.hpp

libmaussorting_includedir=$(includedir)/libmaus/sorting
libmaussorting_include_HEADERS=\
	libmaus/sorting/sorting.hpp \
	libmaus/sorting/ParallelRadixSort.hpp \
	libmaus/sorting/SerialRadixSort64.hpp \
	libmaus/sorting/PairFileSorting.hpp \
	libmaus/sorting/InPlaceParallelSort.hpp \
	libmaus/sorting/MergingReadBack.hpp \
	libmaus/sorting/SortingBufferedOutputFile.hpp \
	libmaus/sorting/MergeStepBinSearchResult.hpp \
	libmaus/sorting/ParallelStableSort.hpp

libmaushashing_includedir=$(includedir)/libmaus/hashing
libmaushashing_include_HEADERS=\
	libmaus/hashing/hash.hpp \
	libmaus/hashing/CircularHash.hpp \
	libmaus/hashing/ConstantStringHash.hpp \
	libmaus/hashing/Crc32.hpp

libmauslf_includedir=$(includedir)/libmaus/lf
libmauslf_include_HEADERS=\
	libmaus/lf/LF.hpp \
	libmaus/lf/HuffmanLF.hpp \
	libmaus/lf/LFZero.hpp \
	libmaus/lf/DArray.hpp \
	libmaus/lf/ImpCompactHuffmanWaveletLF.hpp \
	libmaus/lf/LFBase.hpp \
	libmaus/lf/QuickLF.hpp \
	libmaus/lf/MultiRankLF.hpp \
	libmaus/lf/MultiRankCacheLF.hpp \
	libmaus/lf/ImpCacheLineRankLF.hpp \
	libmaus/lf/ImpCacheLineRankLFNonZero.hpp \
	libmaus/lf/ImpHuffmanWaveletLF.hpp \
	libmaus/lf/ImpWTLF.hpp \
	libmaus/lf/RLIndexLF.hpp

libmausfm_includedir=$(includedir)/libmaus/fm
libmausfm_include_HEADERS=\
	libmaus/fm/FM.hpp \
	libmaus/fm/SampledSA.hpp \
	libmaus/fm/SampledISA.hpp \
	libmaus/fm/MausFmToBwaConversion.hpp \
	libmaus/fm/BidirectionalIndexInterval.hpp \
	libmaus/fm/BidirectionalIndexIntervalSymbol.hpp \
	libmaus/fm/DictionaryInfo.hpp  libmaus/fm/DictionaryInfoImpCompactHuffmanWaveletTree.hpp \
	libmaus/fm/BidirectionalDnaIndexTemplate.hpp
	
libmauslcp_includedir=$(includedir)/libmaus/lcp
libmauslcp_include_HEADERS=\
	libmaus/lcp/LCP.hpp \
	libmaus/lcp/OracleLCP.hpp

libmausrandom_includedir=$(includedir)/libmaus/random
libmausrandom_include_HEADERS=\
	libmaus/random/Random.hpp

libmausgraph_includedir=$(includedir)/libmaus/graph
libmausgraph_include_HEADERS=\
	libmaus/graph/TripleEdge.hpp \
	libmaus/graph/TripleEdgeInput.hpp \
	libmaus/graph/TripleEdgeOutput.hpp \
	libmaus/graph/TripleEdgeOutputMerge.hpp \
	libmaus/graph/TripleEdgeOperations.hpp \
	libmaus/graph/TripleEdgeBuffer.hpp \
	libmaus/graph/TripleEdgeBufferSet.hpp \
	libmaus/graph/TripleEdgeOutputSet.hpp \
	libmaus/graph/U64Buffer.hpp \
	libmaus/graph/TripleIOTest.hpp \
	libmaus/graph/EdgeList.hpp \
	libmaus/graph/EdgeBuffer.hpp \
	libmaus/graph/EdgeListLocalFlush.hpp \
	libmaus/graph/EdgeListLocalLockedFlush.hpp \
	libmaus/graph/EdgeListLocked.hpp \
	libmaus/graph/EdgeBufferLocal.hpp \
	libmaus/graph/EdgeBufferLocalLocked.hpp \
	libmaus/graph/EdgeListLockedSet.hpp \
	libmaus/graph/LocalEdgeSet.hpp \
	libmaus/graph/TripleEdgeCompact.hpp \
	libmaus/graph/FilteredGraphTypes.hpp \
	libmaus/graph/GraphEdge.hpp \
	libmaus/graph/GraphEdgeBuffer.hpp \
	libmaus/graph/GraphEdgeBlockBuffer.hpp \
	libmaus/graph/GraphEdgeHeapComparator.hpp \
	libmaus/graph/GraphEdgeMerge.hpp

libmausbtree_includedir=$(includedir)/libmaus/btree
libmausbtree_include_HEADERS=\
	libmaus/btree/btree.hpp

libmauslru_includedir=$(includedir)/libmaus/lru
libmauslru_include_HEADERS=\
	libmaus/lru/LRU.hpp

libmausclustering_includedir=$(includedir)/libmaus/clustering
libmausclustering_include_HEADERS=\
	libmaus/clustering/HashCreation.hpp \
	libmaus/clustering/HashCreationBase.hpp \
	libmaus/clustering/ProduceEdgeRequest.hpp \
	libmaus/clustering/KmerBase.hpp

libmaussuffixsort_includedir=$(includedir)/libmaus/suffixsort
libmaussuffixsort_include_HEADERS=\
	libmaus/suffixsort/SkewSuffixSort.hpp \
	libmaus/suffixsort/divsufsort.hpp \
	libmaus/suffixsort/SAIS_Mori.hpp \
	libmaus/suffixsort/SuccinctFactorList.hpp \
	libmaus/suffixsort/SAISUtils.hpp \
	libmaus/suffixsort/OSInduce.hpp \
	libmaus/suffixsort/CircularBwt.hpp \
	libmaus/suffixsort/CircularSuffixComparator.hpp \
	libmaus/suffixsort/BwtMergeTempFileNameSet.hpp \
	libmaus/suffixsort/BwtMergeZBlock.hpp \
	libmaus/suffixsort/BwtMergeTempFileNameSetVector.hpp \
	libmaus/suffixsort/BwtMergeBlockSortResult.hpp \
	libmaus/suffixsort/BwtMergeZBlockRequest.hpp \
	libmaus/suffixsort/BwtMergeZBlockRequestVector.hpp \
	libmaus/suffixsort/GapMergePacket.hpp \
	libmaus/suffixsort/BwtMergeEnumBase.hpp \
	libmaus/suffixsort/ByteInputTypes.hpp \
	libmaus/suffixsort/CompactInputTypes.hpp \
	libmaus/suffixsort/PacInputTypes.hpp \
	libmaus/suffixsort/PacTermInputTypes.hpp \
	libmaus/suffixsort/Utf8InputTypes.hpp \
	libmaus/suffixsort/Lz4InputTypes.hpp \
	libmaus/suffixsort/GapArrayByteDecoderBuffer.hpp \
	libmaus/suffixsort/GapArrayByte.hpp \
	libmaus/suffixsort/GapArrayByteDecoder.hpp \
	libmaus/suffixsort/GapArrayByteOverflowKeyAccessor.hpp

libmaussuffixtree_includedir=$(includedir)/libmaus/suffixtree
libmaussuffixtree_include_HEADERS=\
	libmaus/suffixtree/CompressedSuffixTree.hpp

libmausrmq_includedir=$(includedir)/libmaus/rmq
libmausrmq_include_HEADERS=\
	libmaus/rmq/FischerSystematicSuccinctRMQ.hpp \
	libmaus/rmq/QuickDynamicRMQ.hpp \
	libmaus/rmq/CartesianTree.hpp \
	libmaus/rmq/RMMTree.hpp

libmausnetwork_includedir=$(includedir)/libmaus/network
libmausnetwork_include_HEADERS=\
	libmaus/network/Socket.hpp \
	libmaus/network/GetHostName.hpp \
	libmaus/network/LogSocket.hpp \
	libmaus/network/SocketFastReaderBase.hpp \
	libmaus/network/Interface.hpp \
	libmaus/network/GetDottedAddress.hpp \
	libmaus/network/UDPSocket.hpp \
	libmaus/network/GenericServer.hpp \
	libmaus/network/SocketOutputBuffer8.hpp \
	libmaus/network/SocketOutputBuffer8Set.hpp \
	libmaus/network/sendReceiveFd.h \
	libmaus/network/SocketPair.hpp \
	libmaus/network/FileDescriptorPasser.hpp \
	libmaus/network/LogReceiverTestProcess.hpp \
	libmaus/network/LogReceiver.hpp \
	libmaus/network/LogReceiverDispatcherBase.hpp \
	libmaus/network/CurlInit.hpp \
	libmaus/network/OpenSSLInit.hpp \
	libmaus/network/SocketInputStreamBuffer.hpp \
	libmaus/network/SocketInputStream.hpp \
	libmaus/network/HttpAbsoluteUrl.hpp \
	libmaus/network/SocketInputOutputInterface.hpp \
	libmaus/network/OpenSSLSocket.hpp \
	libmaus/network/FtpUrl.hpp \
	libmaus/network/HttpBody.hpp \
	libmaus/network/HttpHeader.hpp \
	libmaus/network/FtpSocket.hpp \
	libmaus/network/UrlBase.hpp \
	libmaus/network/HttpHeaderWrapper.hpp \
	libmaus/network/HttpBodyWrapper.hpp \
	libmaus/network/FtpSocketWrapper.hpp \
	libmaus/network/HttpSocketInputStreamBuffer.hpp \
	libmaus/network/HttpSocketInputStream.hpp \
	libmaus/network/FtpSocketInputStreamBuffer.hpp \
	libmaus/network/FtpSocketInputStream.hpp \
	libmaus/network/FileUrl.hpp \
	libmaus/network/UrlInputStreamBufferWrapper.hpp \
	libmaus/network/UrlInputStream.hpp \
	libmaus/network/GnuTLSInit.hpp \
	libmaus/network/GnuTLSSocket.hpp

libmaussv_includedir=$(includedir)/libmaus/sv
libmaussv_include_HEADERS=\
	libmaus/sv/sv.hpp 

libmauslcs_includedir=$(includedir)/libmaus/lcs
libmauslcs_include_HEADERS=\
	libmaus/lcs/SuffixArrayLCS.hpp \
	libmaus/lcs/LCSRef.hpp \
	libmaus/lcs/LCSDynamic.hpp \
	libmaus/lcs/SuffixPrefix.hpp \
	libmaus/lcs/OverlapComputation.hpp \
	libmaus/lcs/LCS.hpp \
	libmaus/lcs/GenericEditDistance.hpp \
	libmaus/lcs/EditDistanceResult.hpp \
	libmaus/lcs/BaseConstants.hpp \
	libmaus/lcs/PenaltyConstants.hpp \
	libmaus/lcs/TraceContainer.hpp \
	libmaus/lcs/GenericAlignmentPrint.hpp \
	libmaus/lcs/SuffixPrefixResult.hpp \
	libmaus/lcs/SuffixPrefixAlignmentPrint.hpp \
	libmaus/lcs/OffsetElement.hpp \
	libmaus/lcs/HashContainer.hpp \
	libmaus/lcs/HashContainer2.hpp \
	libmaus/lcs/DistNodeType.hpp \
	libmaus/lcs/OverlapComputationBlockRequest.hpp \
	libmaus/lcs/OverlapOrientation.hpp \
	libmaus/lcs/OrientationWeightEncoding.hpp \
	libmaus/lcs/CheckOverlapResult.hpp \
	libmaus/lcs/CheckOverlapResultIdComparator.hpp \
	libmaus/lcs/CheckOverlapResultIdHeapComparator.hpp \
	libmaus/lcs/CheckOverlapResultInput.hpp \
	libmaus/lcs/CheckOverlapResultInputPushBack.hpp \
	libmaus/lcs/CheckOverlapResultMergeInput.hpp \
	libmaus/lcs/CheckOverlapScoreComparatorGreater.hpp \
	libmaus/lcs/CheckOverlapResultOperations.hpp \
	libmaus/lcs/HammingOverlapDetection.hpp \
	libmaus/lcs/AlignmentPrint.hpp \
	libmaus/lcs/AlignmentTraceContainer.hpp \
	libmaus/lcs/EditDistance.hpp \
	libmaus/lcs/BandedEditDistance.hpp \
	libmaus/lcs/MetaEditDistance.hpp \
	libmaus/lcs/EditDistanceTraceContainer.hpp \
	libmaus/lcs/EditDistancePriorityType.hpp \
	libmaus/lcs/LocalBaseConstants.hpp \
	libmaus/lcs/LocalPenaltyConstants.hpp \
	libmaus/lcs/LocalAlignmentTraceContainer.hpp \
	libmaus/lcs/LocalEditDistanceTraceContainer.hpp \
	libmaus/lcs/LocalAlignmentPrint.hpp \
	libmaus/lcs/LocalEditDistance.hpp \
	libmaus/lcs/LocalEditDistanceResult.hpp \
	libmaus/lcs/MetaLocalEditDistance.hpp \
	libmaus/lcs/BandedLocalEditDistance.hpp

libmausquantisation_includedir=$(includedir)/libmaus/quantisation/
libmausquantisation_include_HEADERS = libmaus/quantisation/Quantiser.hpp libmaus/quantisation/ClusterComputation.hpp libmaus/quantisation/KmeansDataType.hpp \
	libmaus/quantisation/KMLocalInterface.hpp

	
libmauslsf_includedir=$(includedir)/libmaus/lsf
libmauslsf_include_HEADERS=\
	libmaus/lsf/LSFProcess.hpp \
	libmaus/lsf/ProcessSet.hpp \
	libmaus/lsf/Process.hpp \
	libmaus/lsf/LSFDispatcherBase.hpp \
	libmaus/lsf/LSFStateBase.hpp \
	libmaus/lsf/DispatchedLsfProcess.hpp  \
	libmaus/lsf/SimpleDispatchedLsfProcessSet.hpp \
	libmaus/lsf/LSFTaskQueue.hpp

libmausconsensus_includedir=$(includedir)/libmaus/consensus/
libmausconsensus_include_HEADERS =\
	libmaus/consensus/Consensus.hpp \
	libmaus/consensus/ScoringMatrix.hpp

EXTRA_PROGRAMS = testlsf testlsflist
noinst_PROGRAMS = \
	testAIO \
	testArithmetic \
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
	testkmlocal \
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
	testcurl \
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
	testgnutls

testgunzip_SOURCES = test/testgunzip.cpp
testgunzip_LDADD = libmaus.la

testsimpletrie_SOURCES = test/testsimpletrie.cpp
testsimpletrie_LDADD = libmaus.la

testthreadpool_SOURCES = test/testthreadpool.cpp
testthreadpool_LDADD = libmaus.la

testsimplethreadpool_SOURCES = test/testsimplethreadpool.cpp
testsimplethreadpool_LDADD = libmaus.la

testthreadpoolbamdecode_SOURCES = test/testthreadpoolbamdecode.cpp
testthreadpoolbamdecode_LDADD = libmaus.la

testsimplethreadpoolbamdecode_SOURCES = test/testsimplethreadpoolbamdecode.cpp
testsimplethreadpoolbamdecode_LDADD = libmaus.la @SNAPPYLIBS@
testsimplethreadpoolbamdecode_CPPFLAGS = @SNAPPYCPPFLAGS@
testsimplethreadpoolbamdecode_LDFLAGS = @SNAPPYLDFLAGS@

testsyncfastreaderbase_SOURCES = test/testsyncfastreaderbase.cpp
testsyncfastreaderbase_LDADD = libmaus.la

testblocksynchronous_SOURCES = test/testblocksynchronous.cpp
testblocksynchronous_LDADD = libmaus.la

testcachealignedalloc_SOURCES = test/testcachealignedalloc.cpp
testcachealignedalloc_LDADD = libmaus.la

testsyncreorderset_SOURCES = test/testsyncreorderset.cpp
testsyncreorderset_LDADD = libmaus.la

testhashcollisions_SOURCES = test/testhashcollisions.cpp
testhashcollisions_LDADD = libmaus.la

testsimplebloomcollisions_SOURCES = test/testsimplebloomcollisions.cpp
testsimplebloomcollisions_LDADD = libmaus.la

testsimplehash_SOURCES = test/testsimplehash.cpp
testsimplehash_LDADD = libmaus.la

testsimplecountinghash_SOURCES = test/testsimplecountinghash.cpp
testsimplecountinghash_LDADD = libmaus.la

testregex_SOURCES = test/testregex.cpp
testregex_LDADD = libmaus.la

testlogging_SOURCES = test/testlogging.cpp
testlogging_LDADD = libmaus.la

testTrie_SOURCES = test/testTrie.cpp
testTrie_LDADD = libmaus.la

testFork_SOURCES = test/testFork.cpp
testFork_LDADD = libmaus.la

testArithmetic_SOURCES = test/testArithmetic.cpp
testArithmetic_LDADD = libmaus.la

testBitIO_SOURCES = test/testBitIO.cpp
testBitIO_LDADD = libmaus.la

testDivSufSort_SOURCES = test/testDivSufSort.cpp
testDivSufSort_LDADD = libmaus.la

testRank_SOURCES = test/testRank.cpp
testRank_LDADD = libmaus.la ${SDSLLIBS}
testRank_CPPFLAGS = ${SDSLCPPFLAGS}

testHuffman_SOURCES = test/testHuffman.cpp
testHuffman_LDADD = libmaus.la

testbitbtree_SOURCES = test/testbitbtree.cpp
testbitbtree_LDADD = libmaus.la

testBWTDyn_SOURCES = test/testBWTDyn.cpp
testBWTDyn_LDADD = libmaus.la

testBWTHufDyn_SOURCES = test/testBWTHufDyn.cpp
testBWTHufDyn_LDADD = libmaus.la

testAIO_SOURCES = test/testAIO.cpp
testAIO_LDADD = libmaus.la

testFastRead_SOURCES = test/testFastRead.cpp
testFastRead_LDADD = libmaus.la

testDNABitWords_SOURCES = test/testDNABitWords.cpp
testDNABitWords_LDADD = libmaus.la

testbtree_SOURCES = test/testbtree.cpp
testbtree_LDADD = libmaus.la

testToWaveletTreeBits_SOURCES = test/testToWaveletTreeBits.cpp
testToWaveletTreeBits_LDADD = libmaus.la

testWaveletExternal_SOURCES = test/testWaveletExternal.cpp
testWaveletExternal_LDADD = libmaus.la

testIsa_SOURCES = test/testIsa.cpp
testIsa_LDADD = libmaus.la

testOS_SOURCES = test/testOS.cpp
testOS_LDADD = libmaus.la

testlz_SOURCES = test/testlz.cpp
testlz_LDADD = libmaus.la

testbandedlcs_SOURCES = test/testbandedlcs.cpp
testbandedlcs_LDADD = libmaus.la

testbamwriter_SOURCES = test/testbamwriter.cpp
testbamwriter_LDADD = libmaus.la

testConsensus_SOURCES = test/testConsensus.cpp
testConsensus_LDADD = libmaus.la

testhashlcs_SOURCES = test/testhashlcs.cpp
testhashlcs_LDADD = libmaus.la

testkmlocal_SOURCES = test/testkmlocal.cpp
testkmlocal_LDADD = libmaus.la

testarraydecode_SOURCES = test/testarraydecode.cpp
testarraydecode_LDADD = libmaus.la

testsnappy_SOURCES = test/testsnappy.cpp
testsnappy_CPPFLAGS = @SNAPPYCPPFLAGS@
testsnappy_LDFLAGS = @SNAPPYLDFLAGS@
testsnappy_LDADD = libmaus.la @SNAPPYLIBS@

testsnappyconcat_SOURCES = test/testsnappyconcat.cpp
testsnappyconcat_CPPFLAGS = @SNAPPYCPPFLAGS@
testsnappyconcat_LDFLAGS = @SNAPPYLDFLAGS@
testsnappyconcat_LDADD = libmaus.la @SNAPPYLIBS@

testlsf_SOURCES = test/testlsf.cpp
testlsf_CPPFLAGS = ${AM_CPPFLAGS} ${LSFCPPFLAGS}
testlsf_LDFLAGS = ${LSFLDFLAGS}
testlsf_LDADD = libmaus.la libmauslsf.la ${LSFLIBS}

testlsfdispatcher_SOURCES = test/testlsfdispatcher.cpp
testlsfdispatcher_LDADD = libmaus.la

testlsfclient_SOURCES = test/testlsfclient.cpp
testlsfclient_LDADD = libmaus.la

testlsfclientfw_SOURCES = test/testlsfclientfw.cpp
testlsfclientfw_LDADD = libmaus.la

testdl_SOURCES = test/testdl.cpp
testdl_LDADD = libmaus.la ${DLLIB}

testdifenc_SOURCES = test/testdifenc.cpp
testdifenc_LDADD = libmaus.la

testkmp_SOURCES = test/testkmp.cpp
testkmp_LDADD = libmaus.la

testbwtmergeinternal_SOURCES = test/testbwtmergeinternal.cpp
testbwtmergeinternal_LDADD = libmaus.la

testfastarewrite_SOURCES = test/testfastarewrite.cpp
testfastarewrite_LDADD = libmaus.la

testfastarecode_SOURCES = test/testfastarecode.cpp
testfastarecode_LDADD = libmaus.la

testbamtofastq_SOURCES = test/testbamtofastq.cpp
testbamtofastq_LDADD = libmaus.la @SNAPPYLIBS@
testbamtofastq_CPPFLAGS = @SNAPPYCPPFLAGS@
testbamtofastq_LDFLAGS = @SNAPPYLDFLAGS@ @DLLIB@

testbamrange_SOURCES = test/testbamrange.cpp
testbamrange_LDADD = libmaus.la

testbamdamage_SOURCES = test/testbamdamage.cpp
testbamdamage_LDADD = libmaus.la @SNAPPYLIBS@
testbamdamage_CPPFLAGS = @SNAPPYCPPFLAGS@
testbamdamage_LDFLAGS = @SNAPPYLDFLAGS@

testgamma_SOURCES = test/testgamma.cpp
testgamma_LDADD = libmaus.la

testcompact_SOURCES = test/testcompact.cpp
testcompact_LDADD = libmaus.la

testbytetocompact_SOURCES = test/testbytetocompact.cpp
testbytetocompact_LDADD = libmaus.la

testscramtosam_SOURCES = test/testscramtosam.cpp
testscramtosam_LDADD = libmaus.la @DLLIB@

testpacdecode_SOURCES = test/testpacdecode.cpp
testpacdecode_LDADD = libmaus.la

testutf8split_SOURCES = test/testutf8split.cpp
testutf8split_LDADD = libmaus.la

testcirchash_SOURCES = test/testcirchash.cpp
testcirchash_CPPFLAGS = @SNAPPYCPPFLAGS@
testcirchash_LDFLAGS = @SNAPPYLDFLAGS@
testcirchash_LDADD = libmaus.la @SNAPPYLIBS@ ${DLLIB}

testrmmtree_SOURCES = test/testrmmtree.cpp
testrmmtree_LDADD = libmaus.la

testcst_SOURCES = test/testcst.cpp
testcst_LDADD = libmaus.la

testbamcmp_SOURCES = test/testbamcmp.cpp
testbamcmp_LDADD = libmaus.la

testbamdif_SOURCES = test/testbamdif.cpp
testbamdif_LDADD = libmaus.la

testqreorder_SOURCES = test/testqreorder.cpp
testqreorder_LDADD = libmaus.la

testfqbgzf_SOURCES = test/testfqbgzf.cpp
testfqbgzf_LDADD = libmaus.la

testoverlap_SOURCES = test/testoverlap.cpp
testoverlap_LDADD = libmaus.la

testcompactfastrandom_SOURCES = test/testcompactfastrandom.cpp
testcompactfastrandom_LDADD = libmaus.la

testbamindex_SOURCES = test/testbamindex.cpp
testbamindex_LDADD = libmaus.la

testlsflist_SOURCES = test/testlsflist.cpp
testlsflist_CPPFLAGS = ${AM_CPPFLAGS} ${LSFCPPFLAGS}
testlsflist_LDFLAGS = ${LSFLDFLAGS}
testlsflist_LDADD = libmaus.la libmauslsf.la ${LSFLIBS}

testbamrewrite_SOURCES = test/testbamrewrite.cpp
testbamrewrite_LDADD = libmaus.la

testfastqstream_SOURCES = test/testfastqstream.cpp
testfastqstream_LDADD = libmaus.la

testsort_SOURCES = test/testsort.cpp
testsort_LDADD = libmaus.la

testexternalsort_SOURCES = test/testexternalsort.cpp
testexternalsort_LDADD = libmaus.la

testcollatespeed_SOURCES = test/testcollatespeed.cpp
testcollatespeed_CPPFLAGS = @SNAPPYCPPFLAGS@
testcollatespeed_LDFLAGS = @SNAPPYLDFLAGS@
testcollatespeed_LDADD = libmaus.la @SNAPPYLIBS@ ${DLLIB}

testbamheaderparser_SOURCES = test/testbamheaderparser.cpp
testbamheaderparser_LDADD = libmaus.la

testbammergecoordinate_SOURCES = test/testbammergecoordinate.cpp
testbammergecoordinate_LDADD = libmaus.la @DLLIB@

testbammergequeryname_SOURCES = test/testbammergequeryname.cpp
testbammergequeryname_LDADD = libmaus.la @DLLIB@

testbamcat_SOURCES = test/testbamcat.cpp
testbamcat_LDADD = libmaus.la @DLLIB@

testbammergecollate_SOURCES = test/testbammergecollate.cpp
testbammergecollate_LDFLAGS = @SNAPPYLDFLAGS@
testbammergecollate_LDADD = libmaus.la @SNAPPYLIBS@ ${DLLIB}
testbammergecollate_CPPFLAGS = @SNAPPYCPPFLAGS@

testautoarray2d_SOURCES = test/testautoarray2d.cpp
testautoarray2d_LDADD = libmaus.la

testhist_SOURCES = test/testhist.cpp
testhist_LDADD = libmaus.la

testfastaparse_SOURCES = test/testfastaparse.cpp
testfastaparse_LDADD = libmaus.la

testvalidatebamindex_SOURCES = test/testvalidatebamindex.cpp
testvalidatebamindex_LDADD = libmaus.la

testeditdistance_SOURCES = test/testeditdistance.cpp
testeditdistance_LDADD = libmaus.la

testgaparraybyte_SOURCES = test/testgaparraybyte.cpp
testgaparraybyte_LDADD = libmaus.la

testzlibconcat_SOURCES = test/testzlibconcat.cpp
testzlibconcat_LDADD = libmaus.la
testzlibconcat_CPPFLAGS = @ZLIBCPPFLAGS@ @WARNCPPFLAGS@

testsimplecompressblock_SOURCES = test/testsimplecompressblock.cpp
testsimplecompressblock_LDADD = libmaus.la @SNAPPYLIBS@ ${DLLIB}
testsimplecompressblock_CPPFLAGS =  @ZLIBCPPFLAGS@ @WARNCPPFLAGS@ @SNAPPYCPPFLAGS@
testsimplecompressblock_LDFLAGS = @SNAPPYLDFLAGS@

testrazdec_SOURCES = test/testrazdec.cpp
testrazdec_LDADD = libmaus.la
testrazdec_CPPFLAGS = @ZLIBCPPFLAGS@ @WARNCPPFLAGS@

testfaindex_SOURCES = test/testfaindex.cpp
testfaindex_LDADD = libmaus.la

testcurl_SOURCES = test/testcurl.cpp
testcurl_LDADD = libmaus.la
testcurl_CPPFLAGS =  @LIBCURLCPPFLAGS@ @WARNCPPFLAGS@
testcurl_LDFLAGS = @LIBCURLLIBS@

testfastablockrecode_SOURCES = test/testfastablockrecode.cpp
testfastablockrecode_LDADD = libmaus.la

testhttp_SOURCES = test/testhttp.cpp
testhttp_LDADD = libmaus.la
testhttp_CPPFLAGS = @OPENSSLCPPFLAGS@ @GNUTLSCPPFLAGS@ @WARNCPPFLAGS@
testhttp_LDFLAGS = @OPENSSLLIBS@ @GNUTLSLIBS@

testftp_SOURCES = test/testftp.cpp
testftp_LDADD = libmaus.la
testftp_CPPFLAGS = @OPENSSLCPPFLAGS@ @GNUTLSCPPFLAGS@ @WARNCPPFLAGS@
testftp_LDFLAGS = @OPENSSLLIBS@ @GNUTLSLIBS@

testurlinputstream_SOURCES = test/testurlinputstream.cpp
testurlinputstream_LDADD = libmaus.la
testurlinputstream_CPPFLAGS = @OPENSSLCPPFLAGS@ @GNUTLSCPPFLAGS@ @WARNCPPFLAGS@
testurlinputstream_LDFLAGS = @OPENSSLLIBS@ @GNUTLSLIBS@

testrlhist_SOURCES = test/testrlhist.cpp
testrlhist_LDADD = libmaus.la

testnumcpus_SOURCES = test/testnumcpus.cpp
testnumcpus_LDADD = libmaus.la

testdiag_SOURCES = test/testdiag.cpp
testdiag_LDADD = libmaus.la
testdiag_CPPFLAGS = ${AM_CPPFLAGS}
testdiag_LDFLAGS = ${AM_LDFLAGS}

testlinuxstreamingwrite_SOURCES = test/testlinuxstreamingwrite.cpp
testlinuxstreamingwrite_LDADD = libmaus.la
testlinuxstreamingwrite_CPPFLAGS = ${AM_CPPFLAGS}
testlinuxstreamingwrite_LDFLAGS = ${AM_LDFLAGS}

testlowmembamheaderparser_SOURCES = test/testlowmembamheaderparser.cpp
testlowmembamheaderparser_LDADD = libmaus.la
testlowmembamheaderparser_CPPFLAGS = ${AM_CPPFLAGS}
testlowmembamheaderparser_LDFLAGS = ${AM_LDFLAGS}

testgeneflatfile_SOURCES = test/testgeneflatfile.cpp
testgeneflatfile_LDADD = libmaus.la
testgeneflatfile_CPPFLAGS = ${AM_CPPFLAGS}
testgeneflatfile_LDFLAGS = ${AM_LDFLAGS}

testgnutls_SOURCES = test/testgnutls.cpp
testgnutls_LDADD = libmaus.la
testgnutls_CPPFLAGS = ${AM_CPPFLAGS} @GNUTLSCPPFLAGS@
testgnutls_LDFLAGS = ${AM_LDFLAGS} @GNUTLSLIBS@
