/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup obj
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  OBJ_AXIS_X_UP = 0,
  OBJ_AXIS_Y_UP = 1,
  OBJ_AXIS_Z_UP = 2,
  OBJ_AXIS_NEGATIVE_X_UP = 3,
  OBJ_AXIS_NEGATIVE_Y_UP = 4,
  OBJ_AXIS_NEGATIVE_Z_UP = 5,
} eTransformAxisUp;

typedef enum {
  OBJ_AXIS_X_FORWARD = 0,
  OBJ_AXIS_Y_FORWARD = 1,
  OBJ_AXIS_Z_FORWARD = 2,
  OBJ_AXIS_NEGATIVE_X_FORWARD = 3,
  OBJ_AXIS_NEGATIVE_Y_FORWARD = 4,
  OBJ_AXIS_NEGATIVE_Z_FORWARD = 5,
} eTransformAxisForward;

static const int TOTAL_AXES = 3;


struct OBJImportParams {
  /** Full path to the source OBJ file to import. */
  char filepath[260];
  /* Value 0 disables clamping. */
  float clamp_size;
  eTransformAxisForward forward_axis;
  eTransformAxisUp up_axis;
};

#ifdef __cplusplus
}
#endif
