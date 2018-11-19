#include "log.h"
#include "util.h"
#include <time.h>
#include <sys/timeb.h>

typedef struct logfile_t {
	char *filename;
	int32_t fd;

	uint16_t lowest_level;

	bool is_valid;
	bool should_be_closed;
	bool is_temp;
}logfile;

/*	change color of font. */

#ifdef WIN32
#include <Windows.h>

#define RED				4
#define PURPLE			5
#define BRIGHT_BLUE	9
#define BRIGHT_YELLOW	14
#define BRIGHT_RED		12
#define WHITE			7

#define DEBUG_COLOR		PURPLE
#define INFO_COLOR		BRIGHT_BLUE
#define NOTICE_COLOR	BRIGHT_YELLOW
#define WARM_COLOR		RED
#define ERROR_COLOR		BRIGHT_RED

#define SLASH '\\'

static inline void set_color(uint16_t x) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (x >= 0 && x <= 15) ? x : WHITE);
}
#endif

#ifdef __linux__
#define BLACK 			"\033[30m"
#define RED 			"\033[31m"
#define GREEN			"\033[32m"
#define YELLOW			"\033[33m"
#define BLUE			"\033[34m"
#define PURPLE			"\033[35m"
#define DEEP_GREEN		"\033[36m"
#define WHITE			"\033[37m"

#define DEBUG_COLOR		PURPLE
#define INFO_COLOR		BLUE
#define NOTICE_COLOR	YELLOW
#define WARM_COLOR		DEEP_GREEN
#define ERROR_COLOR		RED

#define set_color(color) printf(color)

#define SLASH '/'
#endif

static bool do_log = true;
static uint16_t log_level = LOG_NOTICE;

/*	Translate log level to string.	*/
const char *log_level_to_string(uint16_t level) {
	switch (level) {
	case LOG_DEBUG:
		set_color(DEBUG_COLOR);
		return "[debug]";
	case LOG_INFO:
		set_color(INFO_COLOR);
		return "[info]";
	case LOG_NOTICE:
		set_color(NOTICE_COLOR);
		return "[notice]";
	case LOG_WARM:
		set_color(WARM_COLOR);
		return "[warm]";
	case LOG_ERROR:
		set_color(ERROR_COLOR);
		return "[error]";
	default:
		return "???";
	}
}

/*	Set logger level. */
void log_set_level(uint16_t level) {
	log_level = level;
}

void log_enable() {
	do_log = true;
}

void log_disable() {
	do_log = false;
}

/*  Get timestamp and translate to date time and print.	*/
void log_print_daytime() {
#if 0
	static char time_buf[128];
	time_t curr_time;
	uint32_t curr_msec;
	struct tm *t;

	curr_time = get_timestamp();
	curr_msec = curr_time % 1000;
	curr_time /= 1000;
	t = localtime(&curr_time);
	sprintf(time_buf, "%dth %s %d %d:%d:%d.%d", t->tm_mday, month_to_string(t->tm_mon),
		t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, curr_msec);
	printf("%s ", time_buf);
#else
	struct timeb t;
	ftime(&t);
	printf("%lu.%d ", t.time, t.millitm);
#endif
}

/*	Print log, format is:
 *	[Date] [Log level] [function]: [log content] <file> [file name], <line> [line num].*/
void log_fn(uint16_t level,
	const char *file, uint32_t line, const char *func,
	const char *fomat, ...) {
	va_list args;
	
	if (do_log) {
//		if (level < log_level)
//			return;

		log_print_daytime();
		
		printf("%s ", log_level_to_string(level));

		va_start(args, fomat);
		printf("%s(): ", func);
		set_color(WHITE);
		vprintf(fomat, args);
		printf("\t<file> %s, <line> %d.", 
			strrchr(file, SLASH) ? strrchr(file, SLASH) + 1 : file, 
			line);
		putchar('\n');
		va_end(args);
	}
}

void debug_block_fn(const char *file, uint32_t line, const char *func, const char *buf, size_t len) {
	static size_t print_len;
	static char print_buf[512 * 5 + 1];

	print_len = (len > 512) ? 512 : len;
	if (do_log) {
		for (size_t index = 0; index < print_len; ++index)
			sprintf(print_buf + index * 5, "0x%02x ", (unsigned char)buf[index] & 0xff);
		print_buf[len * 5] = '\0';
		log_fn(LOG_DEBUG, file, line, func, print_buf);
	}
}
