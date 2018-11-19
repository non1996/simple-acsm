#include "pattern_match.h"
#include "test\test_allocator.h"

#define TRIE

int main(int argc, char **argv) {
	char *pattern_file = "pattern.txt";
	char *string_file = "string.txt";
	char *token_file = "fenci_result.txt";
	char *mode_ac = "acsm";
	char *mode_trie = "trie";
	char *ac_output_file = "ac_out.txt";
	char *trie_output_file = "trie_out.txt";

#ifdef AC
	if (argc != 4) {
		printf("Usage: acmatch [string file] [pattern file] [result file]\n");
		return 0;
	}
	char *argvs[5] = { argv[0], mode_ac, argv[1], argv[2], argv[3] };
#elif defined(TRIE)
	if (argc != 4) {
		printf("Usage: triematch [token file] [pattern file] [result file]\n");
		return 0;
	}
	char *argvs[5] = { argv[0], mode_trie, argv[1], argv[2], argv[3] };
#endif
	pattern_match *pm = new(pattern_match, 5, argvs);

	pattern_match_work(pm);

	delete(pattern_match, pm);

	return 0;
}
