#include "FileUtil.h"
#include "DBReader.h"
#include "DBWriter.h"
#include "BitManipulateMacros.h"

#include "LocalParameters.h"
#include "SRADBWriter.h"
#include "KSeqWrapper.h"

#include "SRAUtil.h"

#ifdef OPENMP
#include <omp.h>
#endif

int convert2sradb(int argc, const char **argv, const Command &command) {
    Parameters &par = Parameters::getInstance();
    par.parseParameters(argc, argv, command, true, Parameters::PARSE_VARIADIC, 0);

    const char newline = '\n';

    // Get the last input file as the output file
    std::string outputDataFile = par.db2;

    // Determine whether the input file is a directory
    if (FileUtil::directoryExists(par.db1.c_str())) {
        Debug(Debug::ERROR) << "File " << par.db1 << " is a directory" << newline;
        EXIT(EXIT_FAILURE);
    }

    // Determine whether it is fasta input or database input
    //    Here we assume that the input is a database if it has a
    //    corresponding ".dbtype" file
    const bool isDbInput = FileUtil::fileExists(par.db1dbtype.c_str());

    // Name output files and database type
    int outputDbType = Parameters::DBTYPE_AMINO_ACIDS;

    const unsigned int localThreads = par.threads;

    std::string outputIndexFile = outputDataFile + ".index";
    std::string outputHdrDataFile = outputDataFile + "_h";
    std::string outputHdrIndexFile = outputDataFile + "_h.index";

    unsigned int entries_num = 0;

    Debug::Progress progress;

    SRADBWriter hdrWriter(outputHdrDataFile.c_str(), outputHdrIndexFile.c_str(), localThreads, par.compressed, Parameters::DBTYPE_GENERIC_DB);
    hdrWriter.open();

    SRADBWriter seqWriter(outputDataFile.c_str(), outputIndexFile.c_str(), localThreads, par.compressed, Parameters::DBTYPE_AMINO_ACIDS);
    seqWriter.open();

    size_t fileCount = -1;
    DBReader<unsigned int> *reader = NULL;
    DBReader<unsigned int> *hdrReader = NULL;
    std::vector<std::string> filenames;

    // We accept either a
    if (isDbInput) {
        reader = new DBReader<unsigned int>(
            par.db1.c_str(), par.db1Index.c_str(),
            localThreads,
            DBReader<unsigned int>::USE_DATA | DBReader<unsigned int>::USE_INDEX
        );
        reader->open(DBReader<unsigned int>::NOSORT);
        hdrReader = new DBReader<unsigned int>(
            par.hdr1.c_str(), par.hdr1Index.c_str(),
            localThreads,
            DBReader<unsigned int>::USE_DATA | DBReader<unsigned int>::USE_INDEX
        );
        hdrReader->open(DBReader<unsigned int>::NOSORT);
        fileCount = reader->getSize();
    } else {
        filenames = SRAUtil::getFileNamesFromFile(par.db1);
        fileCount = filenames.size();
    }

#pragma omp parallel num_threads(localThreads)
    {
        unsigned int thread_idx = 0;
#ifdef OPENMP
       thread_idx = static_cast<unsigned int>(omp_get_thread_num());
#endif
        // char buffer[4096];

        std::string header;
        header.reserve(1024);

#pragma omp for
        for (size_t fileIdx = 0; fileIdx < fileCount; fileIdx++) {
            unsigned int numEntriesInCurrFile = 0;
            header.clear();

            KSeqWrapper *kseq = NULL;
            std::string seq = ">";
            if (isDbInput) {
                // FIXME: this might also be wrong in certain cases; we need a CI to test all the cases
                unsigned int trueId = reader->getIndex(fileIdx)->id;
                seq.append(hdrReader->getData(trueId, thread_idx));
                seq.append(reader->getData(fileIdx, thread_idx));
                kseq = new KSeqBuffer(seq.c_str(), seq.length());
            } else {
                kseq = KSeqFactory(filenames[fileIdx].c_str());
            }

            while (kseq->ReadEntry()) {
                progress.updateProgress();
                const KSeqWrapper::KSeqEntry &e = kseq->entry;
                SRAUtil::stripInvalidChars(e.sequence.s);

                if (e.name.l == 0) {
                    Debug(Debug::ERROR) << "Fasta entry " << entries_num << " is invalid\n";
                    EXIT(EXIT_FAILURE);
                }
                // Header creation
                header.append(e.name.s, e.name.l);
                if (e.comment.l > 0) {
                    header.append(" ", 1);
                    header.append(e.comment.s, e.comment.l);
                }

                std::string headerId = Util::parseFastaHeader(header.c_str());
                if (headerId.empty()) {
                    // An identifier is necessary for these two cases, so we should just give up
                    Debug(Debug::WARNING) << "Cannot extract identifier from entry " << entries_num << newline;
                }
                header.push_back(newline);

                // Write header
                hdrWriter.writeData(header.c_str(), header.length(), thread_idx, true, true);

                unsigned long rem = e.sequence.l % 3;
                int padding = rem == 0 ? 0 : 1;
                rem = (rem == 0) ? 3 : rem;

                unsigned short *resultBuffer = (unsigned short *) calloc((e.sequence.l / 3 + padding),
                                                                        sizeof(unsigned short));

                size_t i = 0;
                if (e.sequence.l > 3) {
                    for (; i < e.sequence.l - 3; i = i + 3) {
                        resultBuffer[i / 3] = PACK_TO_SHORT(e.sequence.s[i],
                                                            e.sequence.s[i + 1],
                                                            e.sequence.s[i + 2]);
                    }
                }
                resultBuffer[i / 3] = 0U;
                for (unsigned int j = 0; j < 3; j++) {
                    resultBuffer[i / 3] <<= 5U;
                    if (j < rem && e.sequence.s[i + j] != '\n') {
                        resultBuffer[i / 3] |= GET_LAST_5_BITS(e.sequence.s[i + j]);
                    }
                }
                resultBuffer[i / 3] |= 0x8000U; // Set last bit
                const char *packedSeq = reinterpret_cast<const char *>(resultBuffer);

                seqWriter.writeStart(thread_idx);
                seqWriter.writeAdd(packedSeq, sizeof(unsigned short) * (e.sequence.l / 3 + padding), thread_idx);
                seqWriter.writeEnd(thread_idx, false);

                entries_num++;
                numEntriesInCurrFile++;
                header.clear();
                free(resultBuffer);
            }
            delete kseq;
            kseq = nullptr;
        }
    }

    SRADBWriter::writeDbtypeFile(seqWriter.getDataFileName(), outputDbType, par.compressed);

    hdrWriter.close(true);
    seqWriter.close(true);

    if (isDbInput) {
        reader->close();
        hdrReader->close();
        delete reader;
        reader = nullptr;
        delete hdrReader;
        hdrReader = nullptr;
    }

    if (entries_num == 0) {
        Debug(Debug::ERROR) << "The input files have no entry: ";
        Debug(Debug::ERROR) << " - " << par.db1 << "\n";
        Debug(Debug::ERROR) << "Only files in fasta/fastq[.gz|bz2] format are supported\n";
        EXIT(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
