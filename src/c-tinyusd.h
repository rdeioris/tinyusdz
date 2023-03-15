// SPDX-License-Identifier: Apache 2.0
//
// C API(C11) for TinyUSDZ
// This C API is primarily for bindings for other languages.
// Various features/manipulations are missing and not intended to use C API
// solely(at the moment).
//
// NOTE: Use `c_tinyusd` or `CTinyUSD` prefix(`z` is missing) in C API.
//
#ifndef C_TINYUSD_H
#define C_TINYUSD_H

#include <stdint.h>
#include <uchar.h> // size_t

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: Current(2023.03) USD spec does not support 2D or multi-dim array,
// so set max_dim to 1.
#define C_TINYUSD_MAX_DIM (1)

// USD format
typedef enum {
  C_TINYUSD_FORMAT_UNKNOWN,
  C_TINYUSD_FORMAT_AUTO,  // auto detect based on file extension.
  C_TINYUSD_FORMAT_USDA,
  C_TINYUSD_FORMAT_USDC,
  C_TINYUSD_FORMAT_USDZ
} CTinyUSDFormat;

typedef enum {
  C_TINYUSD_AXIS_UNKNOWN,
  C_TINYUSD_AXIS_X,
  C_TINYUSD_AXIS_Y,
  C_TINYUSD_AXIS_Z,
} CTinyUSDAxis;

typedef enum {
  C_TINYUSD_VALUE_TOKEN,
  C_TINYUSD_VALUE_STRING,
  C_TINYUSD_VALUE_BOOL,
  // C_TINYUSD_VALUE_SHORT,
  // C_TINYUSD_VALUE_USHORT,
  C_TINYUSD_VALUE_HALF,
  C_TINYUSD_VALUE_HALF2,
  C_TINYUSD_VALUE_HALF3,
  C_TINYUSD_VALUE_HALF4,
  C_TINYUSD_VALUE_INT,
  C_TINYUSD_VALUE_INT2,
  C_TINYUSD_VALUE_INT3,
  C_TINYUSD_VALUE_INT4,
  C_TINYUSD_VALUE_UINT,
  C_TINYUSD_VALUE_UINT2,
  C_TINYUSD_VALUE_UINT3,
  C_TINYUSD_VALUE_UINT4,
  C_TINYUSD_VALUE_INT64,
  C_TINYUSD_VALUE_UINT64,
  C_TINYUSD_VALUE_FLOAT,
  C_TINYUSD_VALUE_FLOAT2,
  C_TINYUSD_VALUE_FLOAT3,
  C_TINYUSD_VALUE_FLOAT4,
  C_TINYUSD_VALUE_DOUBLE,
  C_TINYUSD_VALUE_DOUBLE2,
  C_TINYUSD_VALUE_DOUBLE3,
  C_TINYUSD_VALUE_DOUBLE4,
  C_TINYUSD_VALUE_QUATH,
  C_TINYUSD_VALUE_QUATF,
  C_TINYUSD_VALUE_QUATD,
  C_TINYUSD_VALUE_COLOR3H,
  C_TINYUSD_VALUE_COLOR3F,
  C_TINYUSD_VALUE_COLOR3D,
  C_TINYUSD_VALUE_COLOR4H,
  C_TINYUSD_VALUE_COLOR4F,
  C_TINYUSD_VALUE_COLOR4D,
  C_TINYUSD_VALUE_TEXCOORD2H,
  C_TINYUSD_VALUE_TEXCOORD2F,
  C_TINYUSD_VALUE_TEXCOORD2D,
  C_TINYUSD_VALUE_TEXCOORD3H,
  C_TINYUSD_VALUE_TEXCOORD3F,
  C_TINYUSD_VALUE_TEXCOORD3D,
  C_TINYUSD_VALUE_NORMAL3H,
  C_TINYUSD_VALUE_NORMAL3F,
  C_TINYUSD_VALUE_NORMAL3D,
  C_TINYUSD_VALUE_VECTOR3H,
  C_TINYUSD_VALUE_VECTOR3F,
  C_TINYUSD_VALUE_VECTOR3D,
  C_TINYUSD_VALUE_POINT3H,
  C_TINYUSD_VALUE_POINT3F,
  C_TINYUSD_VALUE_POINT3D,
  C_TINYUSD_VALUE_MATRIX2D,
  C_TINYUSD_VALUE_MATRIX3D,
  C_TINYUSD_VALUE_MATRIX4D,
  C_TINYUSD_VALUE_FRAME4D,
  C_TINYUSD_VALUE_END,  // terminator
} CTinyUSDValueType;

// assume the number of value types is less than 1024.
#define C_TINYUSD_VALUE_1D_BIT (1 << 10)

// NOTE: No `Geom` prefix to usdGeom prims in C API.
typedef enum {
  C_TINYUSD_PRIM_UNKNOWN,
  C_TINYUSD_PRIM_MODEL,
  C_TINYUSD_PRIM_XFORM,
  C_TINYUSD_PRIM_MESH,
  C_TINYUSD_PRIM_GEOMSUBSET,
  C_TINYUSD_PRIM_MATERIAL,
  C_TINYUSD_PRIM_SHADER,
  // TODO: Add more prim types...
  C_TINYUSD_PRIM_END,
} CTinyUSDPrimType;

// Use lower snake case for frequently used base type.
typedef struct {
  void *data;  // opaque pointer to `tinyusdz::value::token`.
} c_tinyusd_token;

// Create token and set a string to it.
// returns 0 when failed to allocate memory.
int c_tinyusd_token_new(c_tinyusd_token *tok, const char *str);

// Get C char from a token.
// Returned char pointer is valid until `c_tinyusd_token` instance is free'ed.
const char *c_tinyusd_token_str(c_tinyusd_token *tok);

// Free token
// Return 0 when failed to free.
int c_tinyusd_token_free(c_tinyusd_token *tok);

typedef struct {
  void *data;  // opaque pointer to `std::string`.
} c_tinyusd_string;

// Create empty string.
// Return 0 when failed to new
int c_tinyusd_string_new_empty(c_tinyusd_string *s);

// Create string.
// Pass NULL is identical to `c_tinyusd_string_new_empty`.
// Return 0 when failed to new
int c_tinyusd_string_new(c_tinyusd_string *s, const char *str);

// Replace existing string with given `str`.
// `c_tinyusd_string` object must be created beforehand.
// Return 0 when failed to set.
int c_tinyusd_string_replace(c_tinyusd_string *s, const char *str);

// Get C char(`std::string::c_str()`)
// Returned char pointer is valid until `c_tinyusd_string` instance is free'ed.
const char *c_tinyusd_string_str(c_tinyusd_string *s);

int c_tinyusd_string_free(c_tinyusd_string *s);

// Return the name of Prim type.
// Return NULL for unsupported/unknown Prim type.
const char *c_tinyusd_prim_name(CTinyUSDPrimType prim_type);

// Generic Buffer data with type info
typedef struct {
  CTinyUSDValueType value_type;
  int ndim;
  uint64_t shape[C_TINYUSD_MAX_DIM];
  void *data;  // opaque pointer

  // TODO: stride
} CTinyUSDBuffer;

// Returns name of ValueType.
// The pointer points to static Thread-local storage(so thread-safe), thus no
// need to free it.
const char *c_tinyusd_value_type_name(CTinyUSDValueType value_type);

// Returns sizeof(value_type);
// For non-numeric value type(e.g. STRING, TOKEN) and invalid enum value, it
// returns 0. NOTE: Returns 1 for bool type.
uint32_t c_tinyusd_value_type_sizeof(CTinyUSDValueType value_type);

// Returns the number of components of given value_type;
// For example, 3 for C_TINYUSD_VALUE_POINT3F.
// For non-numeric value type(e.g. STRING, TOKEN), it returns 0.
// For scalar type, it returns 1.
uint32_t c_tinyusd_value_type_components(CTinyUSDValueType value_type);

#if 0
// Initialize buffer, but do not allocate memory.
void c_tinyusd_buffer_init(CTinyUSDBuffer *buf, CTinyUSDValueType value_type,
                           int ndim);
#endif

// New buffer with given shape info.
// Returns 1 upon success.
int c_tinyusd_buffer_new(CTinyUSDBuffer *buf, CTinyUSDValueType value_type,
                         int ndim, uint64_t shape[C_TINYUSD_MAX_DIM]);

// Free Buffer's memory. Returns 1 upon success.
int c_tinyusd_buffer_free(CTinyUSDBuffer *buf);

typedef struct {
  CTinyUSDBuffer buffer;
} CTinyUSDAttributeValue;

void c_tinyusd_attribute_value_new(CTinyUSDAttributeValue *val,
                                   const CTinyUSDValueType type,
                                   const CTinyUSDBuffer *buffer);

typedef struct  {
  // c_tinyusd_string prim_part;
  // c_tinyusd_string prop_part;
  void *data;  // opaque pointer to tinyusdz::Path
} CTinyUSDPath;

typedef struct {
  void *data;  // opaque pointer to tinyusdz::Property
} CTinyUSDProperty;

typedef struct {
  void *data;  // opaque pointer to std::map<std::string, tinyusdz::Property>
} CTinyUSDPropertyMap;

typedef struct {
  void *data;  // opaque pointer to tinyusdz::Relationship
} CTinyUSDRelationship;

CTinyUSDRelationship *c_tinyusd_relationsip_new(int n,
                                                const char **targetPaths);
void c_tinyusd_relationsip_delete(CTinyUSDRelationship *rel);

typedef struct {
  void *data;  // opaque pointer to tinyusdz::Attribute
} CTinyUSDAttribute ;

typedef struct {
  // c_tinyusd_string prim_element_name;
  // CTinyUSDPrimType prim_type;

  // CTinyUSDPropertyMap props;
  void *data;  // opaque pointer to tinyusdz::Prim
} CTinyUSDPrim;

int c_tinyusd_prim_new(CTinyUSDPrim *prim, CTinyUSDPrimType prim_type);
int c_tinyusd_prim_free(CTinyUSDPrim *prim);

typedef struct {
  void *data;  // opaque pointer to tinyusd::Stage
} CTinyUSDStage;

int c_tinyusd_stage_new(CTinyUSDStage *stage);
int c_tinyusd_stage_free(CTinyUSDStage *stage);

// Callback function for Stage's root Prim traversal.
// Return 1 for success, Return 0 to stop traversal futher.
typedef int (*CTinyUSDTraversalFunction)(const CTinyUSDPrim *prim,
                                         const CTinyUSDPath *path);

///
/// Traverse root Prims in the Stage and invoke callback function for each Prim.
///
/// @param[in] stage Stage.
/// @param[in] callbacl_fun Callback function.
/// @param[out] err Optional. error message.
///
/// @return 1 upon success. 0 when failed(and `err` will be set).
///
/// When providing `err`, it must be created with `c_tinyusd_string_new` before calling this `c_tinyusd_stage_traverse` function, and an App must free it by calling `c_tinyusd_string_free` after using it.
///
int c_tinyusd_stage_traverse(const CTinyUSDStage *stage,
                             CTinyUSDTraversalFunction callback_fun,
                             c_tinyusd_string *err);

///
/// Detect file format of input file.
///
///
CTinyUSDFormat c_tinyusd_detect_format(const char *filename);

int c_tinyusd_is_usd_file(const char *filename);
int c_tinyusd_is_usda_file(const char *filename);
int c_tinyusd_is_usdc_file(const char *filename);
int c_tinyusd_is_usdz_file(const char *filename);

int c_tinyusd_is_usd_memory(const uint8_t *addr, const size_t nbytes);
int c_tinyusd_is_usda_memory(const uint8_t *addr, const size_t nbytes);
int c_tinyusd_is_usdc_memory(const uint8_t *addr, const size_t nbytes);
int c_tinyusd_is_usdz_memory(const uint8_t *addr, const size_t nbytes);

int c_tinyusd_load_usd_from_file(const char *filename,  CTinyUSDStage *stage, c_tinyusd_string *warn, c_tinyusd_string *err);
int c_tinyusd_load_usda_from_file(const char *filename, CTinyUSDStage *stage, c_tinyusd_string *warn, c_tinyusd_string *err);
int c_tinyusd_load_usdc_from_file(const char *filename, CTinyUSDStage *stage, c_tinyusd_string *warn, c_tinyusd_string *err);
int c_tinyusd_load_usdz_from_file(const char *filename, CTinyUSDStage *stage, c_tinyusd_string *warn, c_tinyusd_string *err);

int c_tinyusd_load_usd_from_memory(const  uint8_t *addr, const size_t nbytes, c_tinyusd_string *warn, c_tinyusd_string *err);
int c_tinyusd_load_usda_from_memory(const uint8_t *addr, const size_t nbytes, c_tinyusd_string *warn, c_tinyusd_string *err);
int c_tinyusd_load_usdc_from_memory(const uint8_t *addr, const size_t nbytes, c_tinyusd_string *warn, c_tinyusd_string *err);
int c_tinyusd_load_usdz_from_memory(const uint8_t *addr, const size_t nbytes, c_tinyusd_string *warn, c_tinyusd_string *err);

#ifdef __cplusplus
}
#endif

#endif  // C_TINYUSD_H
