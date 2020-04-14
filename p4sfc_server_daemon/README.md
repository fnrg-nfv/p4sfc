# P4SFC server daemon program

This program will run as a daemon in each server in P4SFC system.

## Insert new table entry
```bash
// Parameters for main_controller.
{
    "element_instance_id": id_assigned_by_controller
    "entry_info": {
        "table_name": table_name,
        "match_fields": {
            "match_field_0": value,
            "match_field_1": value,
            ...
            "match_field_n": value,
        },
        "action_name": action_name,
        "action_params": {
            "param_0": value,
            "param_1": value,
            ...
            "param_m": value,
        }
    }
}
map element_instance_id to chain_id and stage_id
map chain_id and stage_id to element

// Parameters for p4_controller
{
    "chain_id": chain_id,
    "stage_id": stage_id,
    "element": element,
    "entry_info": {
        "table_name": table_name,
        "match_fields": {
            "match_field_0": value,
            "match_field_1": value,
            ...
            "match_field_n": value,
        },
        "action_name": action_name,
        "action_params": {
            "param_0": value,
            "param_1": value,
            ...
            "param_m": value,
        }
    }
}
add prefix to table_name and action_name
add chain_id and stage_id to entry_info
invoke element.build_new_entry(p4info_helper, entry_info)

//Parameters for specific element
{
    "p4info_helper": p4info_helper,
    "entry_info": {
        "chain_id": chain_id,
        "stage_id": stage_id,
        "table_name": table_name,
        "match_fields": {
            "match_field_0": value,
            "match_field_1": value,
            ...
            "match_field_n": value,
        },
        "action_name": action_name,
        "action_params": {
            "param_0": value,
            "param_1": value,
            ...
            "param_m": value,
        }
    }
}
generate table_entry according to table_name and action_name by using p4info_helper

```