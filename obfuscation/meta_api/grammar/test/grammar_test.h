#ifndef GRAMMAR_TEST_H
#define GRAMMAR_TEST_H

#include <string>
#include <vector>

struct _MetaAPI_AbstractStmt;
typedef struct _MetaAPI_AbstractStmt MetaAPI_AbstractStmt;

struct MetaAPI_Function {
    std::vector<MetaAPI_AbstractStmt *> implementation;
};

#include <meta_api_implementation.h>

#define DEBUG(x) do { printf x; printf("\n"); } while (0)
#define FATAL(x) do { printf x; printf("\n"); exit(1); } while (0)
#define ASSERT(x, y) do { if (!(x)) FATAL(y); } while (0)

typedef uint32_t t_uint32;

#endif
