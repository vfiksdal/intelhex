#ifndef _IHEX_H_
#define _IHEX_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

// Uncomment to assert EOF record
//#define REQUIRE_EOF

//! Enumerates the different log levels
typedef enum {
    LL_DEBUG,
    LL_INFO,
    LL_WARNING,
    LL_ERROR
} loglevel_t;

//! The logging callback definition
typedef void (*logger_t)(loglevel_t level,char *msg,...);

//! Default logging callback
void defaultlogger(loglevel_t level,char *msg,...);

//! Logging callback
extern logger_t ihexlogger;

//! Struct to hold a memory segment
typedef struct segment_t {
    uint8_t *buffer;
    uint32_t address;
    uint32_t size;
} segment_t;

//! Struct to hold a sequence of memory segments
typedef struct memory_t {
    struct segment_t *segment;
    struct memory_t *next;
    unsigned long start;
} memory_t;

// Segment mangement
void segment_init(segment_t *segment,uint32_t address,uint32_t size);
void segment_free(segment_t *segment);
void segment_set(segment_t *segment,uint32_t index,uint8_t value);
uint8_t segment_get(segment_t *segment,uint32_t index);

// Memory management
void memory_init(struct memory_t *memory);
void memory_free(struct memory_t *memory);
unsigned int memory_count(memory_t *memory);
unsigned int memory_size(memory_t *memory);
void memory_consilidate(struct memory_t *memory);
void memory_add(struct memory_t *memory,struct segment_t *segment);

// Parsers
int parse_hex(char *data,unsigned int length);
int parse_string(char *data,unsigned int length,struct memory_t *memory);
int parse_file(char *filename,memory_t *memory);


#endif

