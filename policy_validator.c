#include "policy_validator.h"
#include <stdio.h>

static PolicyValidatorConfig policy_validator;

bool policy_validator_enabled(void)
{
    return policy_validator.enabled;
}

void set_policy_validator_enabled(bool val)
{
    policy_validator.enabled = val;
}

void set_policy_validator_policy_path(const char* path)
{
    printf("set policy dir path to%s\n", path);
    policy_validator.policy_path = path;
}

void set_policy_validator_tag_info_file(const char* file)
{
    printf("set policy tag file to%s\n", file);
    policy_validator.tag_info_file = file;
}

const char* get_policy_validator_policy_path(void)
{
    return policy_validator.policy_path;
}

const char* get_policy_validator_tag_info_file(void)
{
    return policy_validator.tag_info_file;
}

void set_policy_validator_metadata(void)
{
#ifdef ENABLE_VALIDATOR
    if (policy_validator_enabled())
        e_v_set_metadata(policy_validator.policy_path,
                         policy_validator.tag_info_file);
#endif
}


bool policy_validator_commit(void)
{
#ifdef ENABLE_VALIDATOR
    if (policy_validator_enabled())
        return e_v_commit();
#endif
    return true;
}
