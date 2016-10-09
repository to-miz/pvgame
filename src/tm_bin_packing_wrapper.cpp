#define TM_BIN_PACKING_IMPLEMENTATION
#define TMBP_ASSERT assert
#define TMBP_OWN_TYPES
typedef regioni tmbp_rect;
typedef int32 tmbp_size_t;
typedef struct {
	int32 width, height;
} tmbp_dim_;
typedef tmbp_dim_ tmbp_dim;
typedef int32 tmbp_int;
#define TMBP_INT_MAX INT32_MAX
#define TMBP_INT_MIN INT32_MIN
#include <tm_bin_packing.h>