#ifndef COMMANDDECLARATIONS_H
#define COMMANDDECLARATIONS_H
#include "Command.h"

extern int align(int argc, const char **argv, const Command& command);
extern int alignall(int argc, const char **argv, const Command& command);
extern int alignbykmer(int argc, const char **argv, const Command& command);
extern int apply(int argc, const char **argv, const Command& command);
extern int besthitperset(int argc, const char **argv, const Command &command);
extern int transitivealign(int argc, const char **argv, const Command &command);
extern int clust(int argc, const char **argv, const Command& command);
extern int clusteringworkflow(int argc, const char **argv, const Command& command);
extern int clusterupdate(int argc, const char **argv, const Command& command);
extern int clusthash(int argc, const char **argv, const Command& command);
extern int combinepvalperset(int argc, const char **argv, const Command &command);
extern int compress(int argc, const char **argv, const Command &command);
extern int concatdbs(int argc, const char **argv, const Command& command);
extern int convert2fasta(int argc, const char **argv, const Command& command);
extern int convertalignments(int argc, const char **argv, const Command& command);
extern int convertca3m(int argc, const char **argv, const Command& command);
extern int convertkb(int argc, const char **argv, const Command& command);
extern int convertmsa(int argc, const char **argv, const Command& command);
extern int convertprofiledb(int argc, const char **argv, const Command& command);
extern int createdb(int argc, const char **argv, const Command& command);
extern int createindex(int argc, const char **argv, const Command& command);
extern int createlinindex(int argc, const char **argv, const Command& command);
extern int createseqfiledb(int argc, const char **argv, const Command& command);
extern int createsubdb(int argc, const char **argv, const Command& command);
extern int view(int argc, const char **argv, const Command& command);
extern int rmdb(int argc, const char **argv, const Command& command);
extern int mvdb(int argc, const char **argv, const Command& command);
extern int createtsv(int argc, const char **argv, const Command& command);
extern int databases(int argc, const char **argv, const Command& command);
extern int dbtype(int argc, const char **argv, const Command& command);
extern int decompress(int argc, const char **argv, const Command &command);
extern int diffseqdbs(int argc, const char **argv, const Command& command);
extern int easycluster(int argc, const char **argv, const Command& command);
extern int easyrbh(int argc, const char **argv, const Command& command);
extern int easylinclust(int argc, const char **argv, const Command& command);
extern int easysearch(int argc, const char **argv, const Command& command);
extern int easylinsearch(int argc, const char **argv, const Command& command);
extern int enrich(int argc, const char **argv, const Command& command);
extern int expandaln(int argc, const char **argv, const Command& command);
extern int countkmer(int argc, const char **argv, const Command& command);
extern int extractalignedregion(int argc, const char **argv, const Command& command);
extern int extractdomains(int argc, const char **argv, const Command& command);
extern int extractorfs(int argc, const char **argv, const Command& command);
extern int extractframes(int argc, const char **argv, const Command& command);
extern int filterdb(int argc, const char **argv, const Command& command);
extern int filterresult(int argc, const char **argv, const Command& command);
extern int gff2db(int argc, const char **argv, const Command& command);
extern int masksequence(int argc, const char **argv, const Command& command);
extern int indexdb(int argc, const char **argv, const Command& command);
extern int kmermatcher(int argc, const char **argv, const Command &command);
extern int kmersearch(int argc, const char **argv, const Command &command);
extern int kmerindexdb(int argc, const char **argv, const Command &command);
extern int lca(int argc, const char **argv, const Command& command);
extern int taxonomyreport(int argc, const char **argv, const Command& command);
extern int linclust(int argc, const char **argv, const Command& command);
extern int map(int argc, const char **argv, const Command& command);
extern int maskbygff(int argc, const char **argv, const Command& command);
extern int mergeclusters(int argc, const char **argv, const Command& command);
extern int mergedbs(int argc, const char **argv, const Command& command);
extern int mergeresultsbyset(int argc, const char **argv, const Command &command);
extern int msa2profile(int argc, const char **argv, const Command& command);
extern int msa2result(int argc, const char **argv, const Command& command);
extern int multihitdb(int argc, const char **argv, const Command& command);
extern int multihitsearch(int argc, const char **argv, const Command& command);
extern int offsetalignment(int argc, const char **argv, const Command& command);
extern int orftocontig(int argc, const char **argv, const Command& command);
extern int touchdb(int argc, const char **argv, const Command& command);
extern int prefilter(int argc, const char **argv, const Command& command);
extern int prefixid(int argc, const char **argv, const Command& command);
extern int profile2cs(int argc, const char **argv, const Command& command);
extern int profile2pssm(int argc, const char **argv, const Command& command);
extern int profile2consensus(int argc, const char **argv, const Command& command);
extern int profile2repseq(int argc, const char **argv, const Command& command);
extern int proteinaln2nucl(int argc, const char **argv, const Command& command);
extern int rescorediagonal(int argc, const char **argv, const Command& command);
extern int ungappedprefilter(int argc, const char **argv, const Command& command);
extern int rbh(int argc, const char **argv, const Command& command);
extern int result2flat(int argc, const char **argv, const Command& command);
extern int result2msa(int argc, const char **argv, const Command& command);
extern int result2dnamsa(int argc, const char **argv, const Command& command);
extern int result2pp(int argc, const char **argv, const Command& command);
extern int result2profile(int argc, const char **argv, const Command& command);
extern int result2rbh(int argc, const char **argv, const Command& command);
extern int result2repseq(int argc, const char **argv, const Command& command);
extern int result2stats(int argc, const char **argv, const Command& command);
extern int reverseseq(int argc, const char **argv, const Command& command);
extern int search(int argc, const char **argv, const Command& command);
extern int linsearch(int argc, const char **argv, const Command& command);
extern int sortresult(int argc, const char **argv, const Command& command);
extern int splitdb(int argc, const char **argv, const Command& command);
extern int splitsequence(int argc, const char **argv, const Command& command);
extern int subtractdbs(int argc, const char **argv, const Command& command);
extern int suffixid(int argc, const char **argv, const Command& command);
extern int summarizeheaders(int argc, const char **argv, const Command& command);
extern int summarizeresult(int argc, const char **argv, const Command& command);
extern int summarizealis(int argc, const char **argv, const Command &command);
extern int summarizetabs(int argc, const char **argv, const Command& command);
extern int swapdb(int argc, const char **argv, const Command& command);
extern int swapresults(int argc, const char **argv, const Command& command);
extern int taxonomy(int argc, const char **argv, const Command& command);
extern int taxpercontig(int argc, const char **argv, const Command& command);
extern int easytaxonomy(int argc, const char **argv, const Command& command);
extern int createtaxdb(int argc, const char **argv, const Command& command);
extern int translateaa(int argc, const char **argv, const Command& command);
extern int translatenucs(int argc, const char **argv, const Command& command);
extern int tsv2db(int argc, const char **argv, const Command& command);
extern int tar2db(int argc, const char **argv, const Command& command);
extern int versionstring(int argc, const char **argv, const Command& command);
extern int addtaxonomy(int argc, const char **argv, const Command& command);
extern int filtertaxdb(int argc, const char **argv, const Command& command);
extern int filtertaxseqdb(int argc, const char **argv, const Command& command);
extern int aggregatetax(int argc, const char **argv, const Command& command);
extern int aggregatetaxweights(int argc, const char **argv, const Command& command);
extern int diskspaceavail(int argc, const char **argv, const Command& command);
#endif
