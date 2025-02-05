#include "stdafx.h"
#include "matrix_math.h"

void __cdecl matrix4x3_from_orientation(real_matrix4x3* matrix, const real_orientation* orientation)
{
	INVOKE(0x778CB, 0x754C1, matrix4x3_from_orientation, matrix, orientation);
	return;
}

void __cdecl matrix4x3_rotation_from_quaternion(real_matrix4x3* matrix, const real_quaternion* quaternion)
{
	INVOKE(0x7784E, 0x75444, matrix4x3_rotation_from_quaternion, matrix, quaternion);
	return;
}

void __cdecl matrix4x3_rotation_to_quaternion(const real_matrix4x3* matrix, real_quaternion* out)
{
	INVOKE(0x791A2, 0x76CF3, matrix4x3_rotation_to_quaternion, matrix, out);
	return;
}

void matrix4x3_interpolate(const real_matrix4x3* previous, const real_matrix4x3* target, float fractional_ticks, real_matrix4x3* out_mat)
{
	real_quaternion q1_previous, q2_target, q3_interpolated;
	// first we convert matrix to quaternion rotation
	matrix4x3_rotation_to_quaternion(previous, &q1_previous);
	matrix4x3_rotation_to_quaternion(target, &q2_target);
	// interpolate the rotation quaternion
	fast_quaternion_interpolate_and_normalize(&q1_previous, &q2_target, fractional_ticks, &q3_interpolated);
	// and convert back to matrix format, since that's what the game uses
	matrix4x3_rotation_from_quaternion(out_mat, &q3_interpolated);
	// interpolate scale and the position of the node
	scale_interpolate(previous->scale, target->scale, fractional_ticks, &out_mat->scale);
	points_interpolate(&previous->position, &target->position, fractional_ticks, &out_mat->position);
}
