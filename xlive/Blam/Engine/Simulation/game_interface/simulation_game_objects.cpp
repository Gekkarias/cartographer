#include "stdafx.h"
#include "simulation_game_objects.h"

#include "simulation_game_interface.h"
#include "simulation_game_internal.h"

#include "Blam/Engine/cache/cache_files.h"
#include "Blam/Engine/memory/bitstream.h"
#include "Blam/Engine/models/models.h"
#include "Blam/Engine/objects/object_definition.h"
#include "Blam/Engine/scenario/scenario.h"

#include "H2MOD/Tags/TagInterface.h"

#include "Util/Hooks/Hook.h"

// Ensure we aren't sending a variant index that's the same as the default variant
// Inefficient since the variant will be set to it regardless
// If we sync the variant index of the "default" variant this can also cause an issue where the default dialouge isn't selected but that variants specific dialouge
bool simulation_object_variant_should_sync(s_simulation_object_creation_data* creation_data);

// Builds creation data for objects
void __stdcall c_simulation_object_entity_definition__object_build_creation_data(void* _this, datum object_index, s_simulation_object_creation_data* creation_data)
{
    object_datum* object = object_get_fast_unsafe<object_datum>(object_index);
    creation_data->object_definition_index = object->tag_definition_index;
    creation_data->object_scenario_datum_index = object->placement_index;
    creation_data->multiplayer_spawn_monitor_index = object->netgame_equipment_index;
    creation_data->model_variant_index = object->model_variant_id;
    creation_data->emblem_info = object->emblem_info;
    return;
}

bool simulation_object_variant_should_sync(s_simulation_object_creation_data* creation_data)
{
    bool sync_variant = creation_data->model_variant_index != NONE;
    object_definition* object_def = (object_definition*)tag_get_fast(creation_data->object_definition_index);
    if (datum model_tag_index = object_def->model.TagIndex;
        model_tag_index != NONE && sync_variant)
    {
        s_model_definition* model_definition = (s_model_definition*)tag_get_fast(model_tag_index);

        // Confirm that the "defualt" variant is not the one we are trying to sync
        sync_variant = model_definition->variants[creation_data->model_variant_index]->name != object_def->default_model_variant;
    }

    return sync_variant;
}

typedef void(__stdcall* c_simulation_unit_entity_definition_creation_encode_t)(void* thisptr, void* creation_data, bitstream* stream);
c_simulation_unit_entity_definition_creation_encode_t p_c_simulation_unit_entity_definition_encode;
void __stdcall c_simulation_object_entity_definition__object_creation_encode(void* _this, s_simulation_object_creation_data* creation_data, bitstream* packet)
{
    bool model_variant_id_exists = simulation_object_variant_should_sync(creation_data);

    packet->data_encode_bool("model-variant-index-exists", model_variant_id_exists);
    if (model_variant_id_exists)
    {
        packet->data_encode_integer("model-variant-index", creation_data->model_variant_index, 6);    // 6 bits since k_maximum_variants_per_model is 64
    }

    p_c_simulation_unit_entity_definition_encode(_this, creation_data, packet);
    return;
}

typedef bool(__stdcall* c_simulation_unit_entity_definition_creation_decode_t)(void* thisptr, void* creation_data, bitstream* stream);
c_simulation_unit_entity_definition_creation_decode_t p_c_simulation_unit_entity_definition_decode;
bool __stdcall c_simulation_object_entity_definition__object_creation_decode(void* _this, s_simulation_object_creation_data* creation_data, bitstream* packet)
{
    if (packet->data_decode_bool("model-variant-index-exists"))
    {
        creation_data->model_variant_index = packet->data_decode_integer("model-variant-index", 6);    // 6 bits since k_maximum_variants_per_model is 64
    }
    else
    {
        creation_data->model_variant_index = NONE;
    }
    return p_c_simulation_unit_entity_definition_decode(_this, creation_data, packet);
}

bool __stdcall c_simulation_object_entity_definition__object_setup_placement_data(void* _this,
    s_simulation_object_creation_data* object_creation_data,
    s_simulation_object_state_data* state_data,
    uint32* flags,
    object_placement_data* placement_data)
{
    bool result = false;
    if (object_creation_data->object_scenario_datum_index == NONE)
    {
        object_placement_data_new(placement_data, object_creation_data->object_definition_index, NONE, 0);
        placement_data->flags.set(_scenario_object_placement_bit_1, true);
        placement_data->flags.set(_scenario_object_placement_bit_4, true);
        placement_data->emblem_info = object_creation_data->emblem_info;
        if (TEST_BIT(*flags, 1))
        {
            placement_data->position = state_data->position;
            *flags &= ~FLAG(1);
        }
        if (TEST_BIT(*flags, 2))
        {
            placement_data->forward = state_data->forward;
            placement_data->up = state_data->up;
            *flags &= ~FLAG(2);
        }
        if (TEST_BIT(*flags, 3))
        {
            placement_data->scale = state_data->scale;
            *flags &= ~FLAG(3);
        }
        if (TEST_BIT(*flags, 4))
        {
            placement_data->translational_velocity = state_data->translational_velocity;
            *flags &= ~FLAG(4);
        }
        if (TEST_BIT(*flags, 5))
        {
            placement_data->angular_velocity = state_data->angular_velocity;
            *flags &= ~FLAG(5);
        }

        // Set variant of the object
        if (object_creation_data->model_variant_index != NONE && object_creation_data->object_definition_index != NONE)
        {
            object_definition* object_def = (object_definition*)tag_get_fast(object_creation_data->object_definition_index);
            if (object_def->model.TagIndex != NONE)
            {
                s_model_definition* model_def = (s_model_definition*)tag_get_fast(object_def->model.TagIndex);
                if (object_creation_data->model_variant_index < model_def->variants.size)
                {
                    placement_data->variant_name = model_def->variants[object_creation_data->model_variant_index]->name;
                }
            }
            
        }
        result = true;
    }
    return result;
}

int32 __stdcall c_simulation_object_entity_definition__object_creation_required_bits(void* _this)
{
    return simulation_definition_table_index_bits() + 92 + 6;
}

datum __fastcall c_simulation_object_entity_definition__object_create_object(void* _this,
    s_simulation_object_creation_data* object_creation,
    s_simulation_object_state_data* state_data,
    uint32* flags,
    object_placement_data* placement_data)
{
    typedef datum(__thiscall* object_create_object_t)(void*, s_simulation_object_creation_data*, s_simulation_object_state_data*, uint32*, object_placement_data*);
    auto p_object_create_object = Memory::GetAddress<object_create_object_t>(0x1F32DB, 0x1DE374);
    return p_object_create_object(_this, object_creation, state_data, flags, placement_data);
}

void simulation_game_objects_apply_patches(void)
{
    DetourClassFunc(Memory::GetAddress<uint8*>(0x1F27D1, 0x1DD86A), (uint8*)c_simulation_object_entity_definition__object_creation_required_bits, 8);
    DetourClassFunc(Memory::GetAddress<uint8*>(0x1F24ED, 0x1DD586), (uint8*)c_simulation_object_entity_definition__object_build_creation_data, 9);
    DetourClassFunc(Memory::GetAddress<uint8*>(0x1F2704, 0x1DD79D), (uint8*)c_simulation_object_entity_definition__object_setup_placement_data, 10);
    p_c_simulation_unit_entity_definition_encode = (c_simulation_unit_entity_definition_creation_encode_t)DetourClassFunc(
        Memory::GetAddress<uint8*>(0x1F3B11, 0x1DEBAA),
        (uint8*)c_simulation_object_entity_definition__object_creation_encode,
        8);
    p_c_simulation_unit_entity_definition_decode = (c_simulation_unit_entity_definition_creation_decode_t)DetourClassFunc(
        Memory::GetAddress<uint8*>(0x1F3BDD, 0x1DEC76),
        (uint8*)c_simulation_object_entity_definition__object_creation_decode,
        10);
    return;
}