#ifndef LOG_H
#define LOG_H

#include "afx.h"

#define LOG_DEBUG	1
#define LOG_INFO	2
#define LOG_NOTICE	3
#define LOG_WARM	4
#define LOG_ERROR	5

void log_set_level(uint16_t level);
void log_enable();
void log_disable();

void log_fn(uint16_t level, const char *file, uint32_t line, const char *func, const char *fomat, ...);
void debug_block_fn(const char *file, uint32_t line, const char *func, const char *buf, size_t len);
#define debug_block(buf, len) debug_block_fn(__FILE__, __LINE__, __func__, buf, len)

#define log(level, format, ...) log_fn(level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define log_error(format, ...) log(LOG_ERROR, format, ##__VA_ARGS__)
#define log_warm(format, ...) log(LOG_WARM, format, ##__VA_ARGS__)
#define log_notice(format, ...) log(LOG_NOTICE, format, ##__VA_ARGS__)
#define log_info(format, ...) log(LOG_INFO, format, ##__VA_ARGS__)
#define log_debug(format, ...) log(LOG_DEBUG, format, ##__VA_ARGS__)

#endif // !LOG_H

