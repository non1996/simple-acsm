#include "pattern_match.h"
#include "test\test_allocator.h"

int main(int argc, char **argv) {
	/*pattern_match *pm = new(pattern_match, argc, argv);

	pattern_match_work(pm);

	delete(pattern_match, pm);
*/

	test_allocator();

	return 0;
}
