

#include "Util.h"
#include "Parameters.h"
#include "Matcher.h"
#include "Debug.h"
#include "DBReader.h"
#include "DBWriter.h"
#include "IndexReader.h"
#include "FileUtil.h"
#include "TranslateNucl.h"
#include "Sequence.h"
#include "Orf.h"
#include "MemoryMapped.h"

#include <map>

#ifdef OPENMP
#include <omp.h>
#endif

#include "LocalParameters.h"
#include "SRADBReader.h"
#include "IndexReader.h"
#include "SRAUtil.h"

/*
query       Query sequence label
target      Target sequenc label
evalue      E-value
gapopen     Number of gap opens
pident      Percentage of identical matches
nident      Number of identical matches
qstart      1-based start position of alignment in query sequence
qend        1-based end position of alignment in query sequence
qlen        Query sequence length
tstart      1-based start position of alignment in target sequence
tend        1-based end position of alignment in target sequence
tlen        Target sequence length
alnlen      Number of alignment columns
raw         Raw alignment score
bits        Bit score
cigar       Alignment as string M=letter pair, D=delete (gap in query), I=insert (gap in target)
qseq        Full-length query sequence
tseq        Full-length target sequence
qheader     Header of Query sequence
theader     Header of Target sequence
qaln        Aligned query sequence with gaps
taln        Aligned target sequence with gaps
qframe      Query frame (-3 to +3)
tframe      Target frame (-3 to +3)
mismatch    Number of mismatches
qcov        Fraction of query sequence covered by alignment
tcov        Fraction of target sequence covered by alignment
qset        Query set
tset        Target set
 */

void printAlnSeq(std::string &out, const char *seq, unsigned int offset,
                 const std::string &bt, bool reverse, bool isReverseStrand) {
    unsigned int seqPos = 0;
    for (uint32_t i = 0; i < bt.size(); ++i) {
        char seqChar = isReverseStrand ? Orf::complement(seq[offset - seqPos]) : seq[offset + seqPos];
        switch (bt[i]) {
            case 'M':
                out.append(1, seqChar);
                seqPos++;
                break;
            case 'I':
                if (reverse) {
                    out.append(1, '-');
                } else {
                    out.append(1, seqChar);
                    seqPos++;
                }
                break;
            case 'D':
                if (reverse) {
                    out.append(1, seqChar);
                    seqPos++;
                } else {
                    out.append(1, '-');
                }
                break;
        }
    }
}

std::map<unsigned int, std::string> readSet(const std::string &file);

int convertsraalignments(int argc, const char **argv, const Command &command) {
    LocalParameters &par = LocalParameters::getLocalInstance();
    par.parseParameters(argc, argv, command, true, 0, 0);

    const int format = par.formatAlignmentMode;

    if (format == Parameters::FORMAT_ALIGNMENT_HTML || format == Parameters::FORMAT_ALIGNMENT_SAM) {
        Debug(Debug::ERROR) << "Only BLAST tab separated output format is supported for SRA alignments.\n";
        EXIT(EXIT_FAILURE);
    }

    bool needSequenceDB = false;
    bool needBacktrace = false;
    bool needFullHeaders = false;
    bool needLookup = false;
    bool needSource = false;
    bool needTaxonomy = false;
    bool needTaxonomyMapping = false;

    const std::vector<int> outcodes = Parameters::getOutputFormat(
            format,
            par.outfmt,
            needSequenceDB,
            needBacktrace,
            needFullHeaders,
            needLookup,
            needSource,
            needTaxonomyMapping,
            needTaxonomy
    );

    if (needTaxonomy || needTaxonomyMapping) {
        Debug(Debug::ERROR) << "Taxonomy output is not supported for SRA alignments.\n";
        EXIT(EXIT_FAILURE);
    }

    if (needLookup) {
        Debug(Debug::ERROR) << "Lookup output is not supported for SRA alignments.\n";
        EXIT(EXIT_FAILURE);
    }

    int dbaccessMode = needSequenceDB ? (DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA)
                                      : (DBReader<unsigned int>::USE_INDEX);

    const bool touch = (par.preloadMode != Parameters::PRELOAD_MODE_MMAP);

    IndexReader qDbr(par.db1, par.threads, IndexReader::SRC_SEQUENCES,
                     (touch) ? (IndexReader::PRELOAD_INDEX | IndexReader::PRELOAD_DATA) : 0, dbaccessMode);
    IndexReader qDbrHeader(par.db1, par.threads, IndexReader::SRC_HEADERS,
                           (touch) ? (IndexReader::PRELOAD_INDEX | IndexReader::PRELOAD_DATA) : 0);

    SRADBReader tDbr = SRADBReader(par.db2.c_str(), par.db2Index.c_str(), par.threads,
                           DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA);
    tDbr.open(DBReader<unsigned int>::NOSORT | DBReader<unsigned int>::LINEAR_ACCCESS);

    SRADBReader tDbrHeader = SRADBReader((par.db2 + "_h").c_str(), (par.db2 + "_h.index").c_str(), par.threads,
                                 DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA);
    tDbrHeader.open(DBReader<unsigned int>::NOSORT | DBReader<unsigned int>::LINEAR_ACCCESS);

    bool isQueryNucs = Parameters::isEqualDbtype(qDbr.sequenceReader->getDbtype(), Parameters::DBTYPE_NUCLEOTIDES);
    bool isTargetNucs = Parameters::isEqualDbtype(tDbr.getDbtype(), Parameters::DBTYPE_NUCLEOTIDES);

    int gapOpen, gapExtend;
    SubstitutionMatrix *subMat = nullptr;
    if (isQueryNucs && isTargetNucs) {
        subMat = new NucleotideMatrix(par.scoringMatrixFile.values.nucleotide().c_str(), 1.0, 0.0);
        gapOpen = par.gapOpen.values.nucleotide();
        gapExtend = par.gapExtend.values.nucleotide();
    } else {
        subMat = new SubstitutionMatrix(par.scoringMatrixFile.values.aminoacid().c_str(), 2.0, 0.0);
        gapOpen = par.gapOpen.values.aminoacid();
        gapExtend = par.gapExtend.values.aminoacid();
    }

    EvalueComputation *evaluer = nullptr;
    bool isQueryProfile = false;
    bool isTargetProfile = false;
    if (needSequenceDB) {
        isQueryProfile = Parameters::isEqualDbtype(qDbr.sequenceReader->getDbtype(), Parameters::DBTYPE_HMM_PROFILE);
        isTargetProfile = Parameters::isEqualDbtype(tDbr.getDbtype(), Parameters::DBTYPE_HMM_PROFILE);
        evaluer = new EvalueComputation(tDbr.getAminoAcidDBSize(), subMat, gapOpen, gapExtend);
    }

    DBReader<unsigned int> alnDbr(par.db3.c_str(), par.db3Index.c_str(), par.threads,
                                  DBReader<unsigned int>::USE_INDEX | DBReader<unsigned int>::USE_DATA);
    alnDbr.open(DBReader<unsigned int>::LINEAR_ACCCESS);

    unsigned int localThreads = 1;
#ifdef OPENMP
    localThreads = std::min((unsigned int) par.threads, (unsigned int) alnDbr.getSize());
#endif

    const bool shouldCompress = par.dbOut && par.compressed;
    const int dbType = par.dbOut ? Parameters::DBTYPE_GENERIC_DB : Parameters::DBTYPE_OMIT_FILE;
    DBWriter resultWriter(par.db4.c_str(), par.db4Index.c_str(), localThreads, shouldCompress, dbType);
    resultWriter.open();

    const bool isDb = par.dbOut;
//    TranslateNucl translateNucl(static_cast<TranslateNucl::GenCode>(par.translationTable));

    Debug::Progress progress(alnDbr.getSize());
#pragma omp parallel num_threads(localThreads)
    {
        unsigned int thread_idx = 0;
#ifdef OPENMP
        thread_idx = static_cast<unsigned int>(omp_get_thread_num());
#endif
        char buffer[1024];

        std::string result;
        result.reserve(1024 * 1024);

        std::string queryProfData;
        queryProfData.reserve(1024);

        std::string queryBuffer;
        queryBuffer.reserve(1024);

        std::string queryHeaderBuffer;
        queryHeaderBuffer.reserve(1024);

        std::string targetProfData;
        targetProfData.reserve(1024);

        std::string newBacktrace;
        newBacktrace.reserve(1024);

#pragma omp  for schedule(dynamic, 10)
        for (size_t i = 0; i < alnDbr.getSize(); i++) {
            progress.updateProgress();

            const unsigned int queryKey = alnDbr.getDbKey(i);
            char *querySeqData = nullptr;
            size_t querySeqLen = 0;
            queryProfData.clear();
            if (needSequenceDB) {
                size_t qId = qDbr.sequenceReader->getId(queryKey);
                querySeqData = qDbr.sequenceReader->getData(qId, thread_idx);
//                querySeqEntryLen =
                querySeqLen = qDbr.sequenceReader->getEntryLen(qId);
                if (isQueryProfile) {
                    Sequence::extractProfileConsensus(querySeqData, querySeqLen, *subMat, queryProfData);
                }
            }

            size_t qHeaderId = qDbrHeader.sequenceReader->getId(queryKey);
            const char *qHeader = qDbrHeader.sequenceReader->getData(qHeaderId, thread_idx);
            size_t qHeaderLen = qDbrHeader.sequenceReader->getSeqLen(qHeaderId);
            std::string queryId = Util::parseFastaHeader(qHeader);

            char *data = alnDbr.getData(i, thread_idx);
            while (*data != '\0') {
                Matcher::result_t res = Matcher::parseAlignmentRecord(data, true);
                data = Util::skipLine(data);

                if (res.backtrace.empty() && needBacktrace) {
                    Debug(Debug::ERROR)
                            << "Backtrace cigar is missing in the alignment result. Please recompute the alignment with the -a flag.\n"
                               "Command: mmseqs align " << par.db1 << " " << par.db2 << " " << par.db3 << " "
                            << "alnNew -a\n";
                    EXIT(EXIT_FAILURE);
                }

                size_t tHeaderId = (unsigned int) res.dbOrfStartPos; //tDbrHeader->sequenceReader->getId(res.dbKey);
                char *tmpTHeader = tDbrHeader.getData(tHeaderId, thread_idx);
                size_t tHeaderLen = strlen(tmpTHeader); //tDbrHeader.getSeqLen(tHeaderId);
                char *tHeader = new char[tHeaderLen + 1];
                SRAUtil::stripInvalidChars(tmpTHeader, tHeader);
                std::string targetId = Util::parseFastaHeader(tHeader);

                unsigned int gapOpenCount = 0;
                unsigned int alnLen = res.alnLength;
                unsigned int missMatchCount = 0;
                unsigned int identical = 0;
                if (!res.backtrace.empty()) {
                    size_t matchCount = 0;
                    alnLen = 0;
                    for (size_t pos = 0; pos < res.backtrace.size(); pos++) {
                        int cnt = 0;
                        if (isdigit(res.backtrace[pos])) {
                            cnt += Util::fast_atoi<int>(res.backtrace.c_str() + pos);
                            while (isdigit(res.backtrace[pos])) {
                                pos++;
                            }
                        }
                        alnLen += cnt;
                        switch (res.backtrace[pos]) {
                            case 'M':
                                matchCount += cnt;
                                break;
                            case 'D':
                            case 'I':
                                gapOpenCount += 1;
                                break;
                        }
                    }
                    identical = static_cast<unsigned int>(res.seqId * static_cast<float>(alnLen) + 0.5);
                    missMatchCount = static_cast<unsigned int>( matchCount - identical);
                } else {
                    const int adjustQstart = (res.qStartPos == -1) ? 0 : res.qStartPos;
                    const int adjustDBstart = (res.dbStartPos == -1) ? 0 : res.dbStartPos;
                    const float bestMatchEstimate = static_cast<float>(std::min(abs(res.qEndPos - adjustQstart),
                                                                                abs(res.dbEndPos - adjustDBstart)));
                    missMatchCount = static_cast<unsigned int>(bestMatchEstimate * (1.0f - res.seqId) + 0.5);
                }

                switch (format) {
                    case Parameters::FORMAT_ALIGNMENT_BLAST_TAB: {
                        if (outcodes.empty()) {
                            int count = snprintf(buffer, sizeof(buffer),
                                                 "%s\t%s\t%1.3f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.2E\t%d\n",
                                                 queryId.c_str(), targetId.c_str(), res.seqId, alnLen,
                                                 missMatchCount, gapOpenCount,
                                                 res.qStartPos + 1, res.qEndPos + 1,
                                                 res.dbStartPos + 1, res.dbEndPos + 1,
                                                 res.eval, res.score);
                            if (count < 0 || static_cast<size_t>(count) >= sizeof(buffer)) {
                                Debug(Debug::WARNING) << "Truncated line in entry" << i << "!\n";
                                continue;
                            }
                            result.append(buffer, count);
                        } else {
                            char *targetSeqData = nullptr;
                            targetProfData.clear();
                            size_t targetSeqLen = 0;
                            if (needSequenceDB) {
                                size_t tId = res.dbOrfStartPos; // tDbr->sequenceReader->getId(res.dbKey);
                                targetSeqData = tDbr.getData(tId, thread_idx);
                                targetSeqLen = tDbr.getSeqLen(tId);
                                if (isTargetProfile) {
                                    Sequence::extractProfileConsensus(targetSeqData, targetSeqLen,  *subMat, targetProfData);
                                }
                            }
                            for (size_t i = 0; i < outcodes.size(); i++) {
                                switch (outcodes[i]) {
                                    case Parameters::OUTFMT_QUERY:
                                        result.append(queryId);
                                        break;
                                    case Parameters::OUTFMT_TARGET:
                                        result.append(targetId);
                                        break;
                                    case Parameters::OUTFMT_EVALUE:
                                        result.append(SSTR(res.eval));
                                        break;
                                    case Parameters::OUTFMT_GAPOPEN:
                                        result.append(SSTR(gapOpenCount));
                                        break;
                                    case Parameters::OUTFMT_FIDENT:
                                        result.append(SSTR(res.seqId));
                                        break;
                                    case Parameters::OUTFMT_PIDENT:
                                        result.append(SSTR(res.seqId * 100));
                                        break;
                                    case Parameters::OUTFMT_NIDENT:
                                        result.append(SSTR(identical));
                                        break;
                                    case Parameters::OUTFMT_QSTART:
                                        result.append(SSTR(res.qStartPos + 1));
                                        break;
                                    case Parameters::OUTFMT_QEND:
                                        result.append(SSTR(res.qEndPos + 1));
                                        break;
                                    case Parameters::OUTFMT_QLEN:
                                        result.append(SSTR(res.qLen));
                                        break;
                                    case Parameters::OUTFMT_TSTART:
                                        result.append(SSTR(res.dbStartPos + 1));
                                        break;
                                    case Parameters::OUTFMT_TEND:
                                        result.append(SSTR(res.dbEndPos + 1));
                                        break;
                                    case Parameters::OUTFMT_TLEN:
                                        result.append(SSTR(res.dbLen));
                                        break;
                                    case Parameters::OUTFMT_ALNLEN:
                                        result.append(SSTR(alnLen));
                                        break;
                                    case Parameters::OUTFMT_RAW:
                                        result.append(
                                                SSTR(static_cast<int>(
                                                             evaluer->computeRawScoreFromBitScore(res.score) + 0.5
                                                     ))
                                        );
                                        break;
                                    case Parameters::OUTFMT_BITS:
                                        result.append(SSTR(res.score));
                                        break;
                                    case Parameters::OUTFMT_CIGAR:
                                        result.append(SSTR(res.backtrace));
                                        newBacktrace.clear();
                                        break;
                                    case Parameters::OUTFMT_QSEQ:
                                        if (isQueryProfile) {
                                            result.append(queryProfData.c_str(), res.qLen);
                                        } else {
                                            result.append(querySeqData, res.qLen);
                                        }
                                        break;
                                    case Parameters::OUTFMT_TSEQ:
                                        if (isTargetProfile) {
                                            result.append(targetProfData.c_str(), res.dbLen);
                                        } else {
                                            result.append(targetSeqData, res.dbLen);
                                        }
                                        break;
                                    case Parameters::OUTFMT_QHEADER:
                                        result.append(qHeader, qHeaderLen);
                                        break;
                                    case Parameters::OUTFMT_THEADER:
                                        result.append(tHeader, tHeaderLen);
                                        break;
                                    case Parameters::OUTFMT_QALN:
                                        if (isQueryProfile) {
                                            printAlnSeq(result, queryProfData.c_str(), res.qStartPos,
                                                        Matcher::uncompressAlignment(res.backtrace), false,
                                                        (res.qStartPos > res.qEndPos));
                                        } else {
                                            printAlnSeq(result, querySeqData, res.qStartPos,
                                                        Matcher::uncompressAlignment(res.backtrace), false,
                                                        (res.qStartPos > res.qEndPos));
                                        }
                                        break;
                                    case Parameters::OUTFMT_TALN: {
                                        if (isTargetProfile) {
                                            printAlnSeq(result, targetProfData.c_str(), res.dbStartPos,
                                                        Matcher::uncompressAlignment(res.backtrace), true,
                                                        (res.dbStartPos > res.dbEndPos));
                                        } else {
                                            printAlnSeq(result, targetSeqData, res.dbStartPos,
                                                        Matcher::uncompressAlignment(res.backtrace), true,
                                                        (res.dbStartPos > res.dbEndPos));
                                        }
                                        break;
                                    }
                                    case Parameters::OUTFMT_MISMATCH:
                                        result.append(SSTR(missMatchCount));
                                        break;
                                    case Parameters::OUTFMT_QCOV:
                                        result.append(SSTR(res.qcov));
                                        break;
                                    case Parameters::OUTFMT_TCOV:
                                        result.append(SSTR(res.dbcov));
                                        break;
                                    case Parameters::OUTFMT_EMPTY:
                                        result.push_back('-');
                                        break;
                                    case Parameters::OUTFMT_QORFSTART:
                                        result.append(SSTR(res.queryOrfStartPos));
                                        break;
                                    case Parameters::OUTFMT_QORFEND:
                                        result.append(SSTR(res.queryOrfEndPos));
                                        break;
                                    case Parameters::OUTFMT_TORFSTART:
                                        result.append(SSTR(res.dbOrfStartPos));
                                        break;
                                    case Parameters::OUTFMT_TORFEND:
                                        result.append(SSTR(res.dbOrfEndPos));
                                        break;
                                }
                                if (i < outcodes.size() - 1) {
                                    result.push_back('\t');
                                }
                            }
                            result.push_back('\n');
                        }
                        break;
                    }
                    case Parameters::FORMAT_ALIGNMENT_BLAST_WITH_LEN: {
                        int count = snprintf(buffer, sizeof(buffer),
                                             "%s\t%s\t%1.3f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.2E\t%d\t%d\t%d\n",
                                             queryId.c_str(), targetId.c_str(), res.seqId, alnLen,
                                             missMatchCount, gapOpenCount,
                                             res.qStartPos + 1, res.qEndPos + 1,
                                             res.dbStartPos + 1, res.dbEndPos + 1,
                                             res.eval, res.score,
                                             res.qLen, res.dbLen);

                        if (count < 0 || static_cast<size_t>(count) >= sizeof(buffer)) {
                            Debug(Debug::WARNING) << "Truncated line in entry" << i << "!\n";
                            continue;
                        }

                        result.append(buffer, count);
                        break;
                    }
                    default:
                        Debug(Debug::ERROR) << "Not implemented yet";
                        EXIT(EXIT_FAILURE);
                }
                delete[] tHeader;
            }

            resultWriter.writeData(result.c_str(), result.size(), queryKey, thread_idx, isDb);
            result.clear();
        }
    }

    // tsv output
    resultWriter.close(true);
    if (isDb == false) {
        FileUtil::remove(par.db4Index.c_str());
    }

    alnDbr.close();

    if (needSequenceDB) {
        tDbr.close();

        delete evaluer;
        evaluer = nullptr;
    }

    tDbrHeader.close();

    delete subMat;
    subMat = nullptr;

    return EXIT_SUCCESS;
}
