#pragma once
#include "Blam/Engine/math/real_math.h"

struct melee_aim_assist_parameters
{
	real_angle magnetism_angle_degrees;		// the maximum angle that magnetism works at full strength
	real32 magnetism_range_world_units;	// the maximum distance that magnetism works at full strength

	real32 throttle_magnitude;
	real32 throttle_minimum_distance;
	real32 throttle_maximum_adjustment_angle_degrees;
};
CHECK_STRUCT_SIZE(melee_aim_assist_parameters, 20);

struct aim_assist_parameters
{
	real_angle autoaim_angle_degrees;// the maximum angle that autoaim works at full strength
	real32 autoaim_range_world_units;	// the maximum distance that autoaim works at full strength
	real32 magnetism_angle_degrees;// the maximum angle that magnetism works at full strength
	real32 magnetism_range_world_units;// the maximum distance that magnetism works at full strength
	real32 deviation_angle_degrees;	// the maximum angle that a projectile is allowed to deviate from the gun barrel

	real32 unused[4];
};
CHECK_STRUCT_SIZE(aim_assist_parameters, 36);

struct s_aim_assist_targetting_data
{
	datum target_object;
	datum model_target;
	datum target_player;
	char gap_C[12];
	signed __int16 auto_aim_flags;
	char gap_1A[2];
	real32 primary_auto_aim_level;
	real32 secondary_auto_aim_level;
};
CHECK_STRUCT_SIZE(s_aim_assist_targetting_data, 0x24);

void aim_assist_apply_patches();
