#include "qemu/osdep.h"
#include "cpu.h"
#include "tcg/tcg-op.h"
#include "policy_validator.h"
#include <stdio.h>

static PolicyValidatorConfig policy_validator;

CPURISCVState* policy_validator_hack_env;
CPUState* policy_validator_hack_cpu_state;

static inline uint64_t policy_validator_reg_reader(uint32_t reg_num)
{
    if(reg_num == 0) return 0;

    return (uint64_t)policy_validator_hack_env->gpr[reg_num];
}

static inline uint64_t policy_validator_address_fixer(uint64_t vaddr)
{
    CPUClass *cc = CPU_GET_CLASS(policy_validator_hack_cpu_state);
    uint64_t page_addr = cc->get_phys_page_debug(policy_validator_hack_cpu_state, vaddr);
    uint64_t page_offset = vaddr & (qemu_real_host_page_size-1);
    if (page_addr == -1)
        return vaddr;
    return page_addr | page_offset;
 }

bool policy_validator_enabled(void)
{
    return policy_validator.enabled;
}

void set_policy_validator_enabled(bool val)
{
    policy_validator.enabled = val;
}

void set_policy_validator_cfg_path(const char* path)
{
    policy_validator.validator_cfg_path = path;
}


const char* get_policy_validator_cfg_path(void)
{
    return policy_validator.validator_cfg_path;
}

void set_policy_validator_metadata(void)
{
#ifdef ENABLE_VALIDATOR
    if (policy_validator_enabled())
        e_v_set_metadata(policy_validator.validator_cfg_path);
#endif
}

void policy_validator_init(void)
{
#ifdef ENABLE_VALIDATOR
    if (policy_validator_enabled()) {
        e_v_set_metadata(policy_validator.validator_cfg_path);
        e_v_set_callbacks(policy_validator_reg_reader,
                          NULL, policy_validator_address_fixer);
    }
#endif
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

void policy_validator_rule_cache_stats(void)
{
#ifdef ENABLE_VALIDATOR
    if (policy_validator_enabled())
        e_v_rule_cache_stats();
#endif
}
