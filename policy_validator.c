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

void policy_validator_violation_msg(char* dest, int n)
{
#ifdef ENABLE_VALIDATOR
    e_v_violation_msg(dest, n);
#endif
}

void policy_validator_get_pc_tag(char *dest, int n)
{
#ifdef ENABLE_VALIDATOR
    e_v_pc_tag(dest, n);
#endif
}

void policy_validator_get_csr_tag(char *dest, int n, uint64_t addr)
{
#ifdef ENABLE_VALIDATOR
    e_v_csr_tag(dest, n, addr);
#endif
}

void policy_validator_get_reg_tag(char *dest, int n, uint64_t addr)
{
#ifdef ENABLE_VALIDATOR
    e_v_reg_tag(dest, n, addr);
#endif
}

void policy_validator_get_mem_tag(char *dest, int n, uint64_t addr)
{
#ifdef ENABLE_VALIDATOR
    e_v_mem_tag(dest, n, addr);
#endif
}

void policy_validator_set_pc_watch(void)
{
#ifdef ENABLE_VALIDATOR
    static bool watching = true;
    e_v_set_pc_watch(watching);
    watching = !watching;
#endif
}

void policy_validator_set_reg_watch(uint64_t addr)
{
#ifdef ENABLE_VALIDATOR
    e_v_set_reg_watch(addr);
#endif
}

void policy_validator_set_csr_watch(uint64_t addr)
{
#ifdef ENABLE_VALIDATOR
    e_v_set_csr_watch(addr);
#endif
}

void policy_validator_set_mem_watch(uint64_t addr)
{
#ifdef ENABLE_VALIDATOR
    e_v_set_mem_watch(addr);
#endif
}
