ACLOCAL_AMFLAGS=-I m4

abs_builddir = @abs_builddir@

noinst_LTLIBRARIES = testmod.la
testmod_la_LDFLAGS = @LIBMAUSLDFLAGS@ ${AM_LDFLAGS} -module -avoid-version -shared -rpath $(abs_builddir)
testmod_la_SOURCES = test/testmod.cpp

lib_LTLIBRARIES = libmaus.la libmauslsf.la libmaus_kmlocal_mod.la libmaus_consensus_mod.la libmaus_scram_mod.la

libmaus_la_SOURCES = libmaus/StaticInitialization.cpp libmaus/bitio/CompactArray.cpp libmaus/rank/RankTable.cpp \
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
	libmaus/bambam/BamAlignment.cpp \
	libmaus/bambam/BamAlignmentNameComparator.cpp \
	libmaus/huffman/KvInitResult.cpp \
	libmaus/fm/MausFmToBwaConversion.cpp
libmaus_la_CPPFLAGS = @LIBMAUSCPPFLAGS@ @SNAPPYCPPFLAGS@ @ZLIBCPPFLAGS@
libmaus_la_LDFLAGS = @LIBMAUSLDFLAGS@ @SNAPPYLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@
libmaus_la_LIBADD = @DLLIB@ @SNAPPYLIBS@ @ZLIBLIBS@ 

libmauslsf_la_LDFLAGS = @LIBMAUSLDFLAGS@ -version-info ${LIBRARY_VERSION} @LIBMAUSSTATIC@
libmauslsf_la_SOURCES = libmaus/lsf/LSFProcess.cpp
libmauslsf_la_CPPFLAGS = ${LSFCPPFLAGS}
libmauslsf_la_LIBADD = libmaus.la ${LSFLIBS}

libmaus_kmlocal_mod_la_CPPFLAGS = @KMLOCALCPPFLAGS@ @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS}
libmaus_kmlocal_mod_la_CXXFLAGS = @KMLOCALCXXFLAGS@ @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus_kmlocal_mod_la_LDFLAGS  = @KMLOCALLDFLAGS@  @LIBMAUSLDFLAGS@                 -module -avoid-version -shared -export-dynamic # ${AM_LDFLAGS}
libmaus_kmlocal_mod_la_LIBADD   = @KMLOCALLIBS@     @LIBMAUSLIBS@
libmaus_kmlocal_mod_la_SOURCES  = libmaus/quantisation/KMLocalInterface.cpp

libmaus_consensus_mod_la_CPPFLAGS = @SEQANCPPFLAGS@                 @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS}
libmaus_consensus_mod_la_CXXFLAGS = @SEQANCXXFLAGS@                 @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus_consensus_mod_la_LDFLAGS  = @SEQANLDFLAGS@  @SEQANLIBS@     @LIBMAUSLDFLAGS@  @LIBMAUSLIBS@     -module -avoid-version -shared -export-dynamic # ${AM_LDFLAGS}
libmaus_consensus_mod_la_LIBADD   = @SEQANLIBS@ @LIBMAUSLIBS@
libmaus_consensus_mod_la_SOURCES  = libmaus/consensus/ConsensusBase.cpp libmaus/consensus/ScoringMatrix.cpp \
	libmaus/fastx/Phred.cpp

libmaus_scram_mod_la_CPPFLAGS = @IOLIBCPPFLAGS@ @LIBMAUSCPPFLAGS@ ${AM_CPPFLAGS}
libmaus_scram_mod_la_CXXFLAGS = @LIBMAUSCXXFLAGS@ ${AM_CXXFLAGS} -fPIC
libmaus_scram_mod_la_LDFLAGS  = @IOLIBLDFLAGS@  @IOLIBLIBS@     @LIBMAUSLDFLAGS@  @LIBMAUSLIBS@     -module -avoid-version -shared -export-dynamic # ${AM_LDFLAGS}
libmaus_scram_mod_la_LIBADD   = @IOLIBLIBS@ @LIBMAUSLIBS@
libmaus_scram_mod_la_SOURCES  = libmaus/bambam/Scram.c

libmaus_includedir=$(includedir)/libmaus/
libmaus_include_HEADERS=libmaus/LibMausConfig.hpp

libmausautoarray_includedir=$(includedir)/libmaus/autoarray/
libmausautoarray_include_HEADERS = libmaus/autoarray/AutoArray.hpp

libmausgamma_includedir=$(includedir)/libmaus/gamma/
libmausgamma_include_HEADERS = libmaus/gamma/GammaEncoder.hpp libmaus/gamma/GammaDecoder.hpp \
	libmaus/gamma/GammaGapEncoder.hpp libmaus/gamma/GammaGapDecoder.hpp \
	libmaus/gamma/GammaRLDecoder.hpp  libmaus/gamma/GammaRLEncoder.hpp

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
	libmaus/lz/BgzfDeflate.hpp

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
	libmaus/bambam/ScramDecoder.hpp
	     
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
	libmaus/bitio/PacDecoderBuffer.hpp

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
	libmaus/rank/ImpCacheLineRank.hpp
	
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
	libmaus/wavelet/ImpHuffmanWaveletTree.hpp

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
	libmaus/huffman/KvInitResult.hpp

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
	libmaus/math/iabs.hpp

libmausrl_includedir=$(includedir)/libmaus/rl
libmausrl_include_HEADERS=\
	libmaus/rl/RLIndex.hpp

libmausutil_includedir=$(includedir)/libmaus/util
libmausutil_include_HEADERS=\
	libmaus/util/stringFunctions.hpp \
	libmaus/util/BitList.hpp \
	libmaus/util/IntervalTree.hpp \
	libmaus/util/LookupIntervalTree.hpp \
	libmaus/util/StringSerialisation.hpp \
	libmaus/util/NumberSerialisation.hpp \
	libmaus/util/GetFileSize.hpp \
	libmaus/util/unique_ptr.hpp \
	libmaus/util/shared_ptr.hpp \
	libmaus/util/ipow.hpp \
	libmaus/util/TempFileNameGenerator.hpp \
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
	libmaus/util/NegativeDifferenceArray.hpp \
	libmaus/util/PositiveDifferenceArray.hpp \
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
	libmaus/util/CountPutObject.hpp \
	libmaus/util/CountPutIterator.hpp \
	libmaus/util/IStreamWrapper.hpp \
	libmaus/util/PosixFileDescriptor.hpp \
	libmaus/util/DigitTable.hpp \
	libmaus/util/TempFileRemovalContainer.hpp \
	libmaus/util/DynamicLoading.hpp \
	libmaus/util/Object.hpp \
	libmaus/util/KMP.hpp \
	libmaus/util/NumberMapSerialisation.hpp \
	libmaus/util/Utf8BlockIndex.hpp

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
	libmaus/trie/TrieState.hpp 

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
	libmaus/aio/MultiFileConcatWriter.hpp \
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
	libmaus/aio/CircularWrapper.hpp

libmausparallel_includedir=$(includedir)/libmaus/parallel
libmausparallel_include_HEADERS=\
	libmaus/parallel/OMPLock.hpp \
	libmaus/parallel/PosixMutex.hpp \
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
	libmaus/parallel/PosixProcess.hpp

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
	libmaus/fastx/CompactFastReplace.hpp

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
	libmaus/sorting/PairFileSorting.hpp

libmaushashing_includedir=$(includedir)/libmaus/hashing
libmaushashing_include_HEADERS=\
	libmaus/hashing/hash.hpp

libmauslf_includedir=$(includedir)/libmaus/lf
libmauslf_include_HEADERS=\
	libmaus/lf/LF.hpp \
	libmaus/lf/HuffmanLF.hpp \
	libmaus/lf/LFZero.hpp \
	libmaus/lf/DArray.hpp

libmausfm_includedir=$(includedir)/libmaus/fm
libmausfm_include_HEADERS=\
	libmaus/fm/FM.hpp \
	libmaus/fm/SampledSA.hpp \
	libmaus/fm/SampledISA.hpp \
	libmaus/fm/MausFmToBwaConversion.hpp

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
	libmaus/graph/TripleEdgeCompact.hpp

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
	libmaus/suffixsort/GapMergePacket.hpp

libmausrmq_includedir=$(includedir)/libmaus/rmq
libmausrmq_include_HEADERS=\
	libmaus/rmq/FischerSystematicSuccinctRMQ.hpp \
	libmaus/rmq/QuickDynamicRMQ.hpp \
	libmaus/rmq/CartesianTree.hpp

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
	libmaus/network/LogReceiverDispatcherBase.hpp

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
	libmaus/lcs/CheckOverlapResultOperations.hpp

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

EXTRA_PROGRAMS = testlsf
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
	testdl \
	testdifenc \
	testbwtmergeinternal \
	testfastarewrite \
	testfastarecode \
	testbamtofastq \
	testbamdamage \
	testgamma \
	testcompact \
	testbytetocompact \
	testscramtosam \
	testpacdecode \
	testutf8split

testgunzip_SOURCES = test/testgunzip.cpp
testgunzip_LDADD = libmaus.la

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

testlsf_SOURCES = test/testlsf.cpp
testlsf_CPPFLAGS = ${AM_CPPFLAGS} ${LSFCPPFLAGS}
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
testbamtofastq_LDFLAGS = @SNAPPYLDFLAGS@

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
