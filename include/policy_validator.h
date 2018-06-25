#ifndef POLICY_VALIDATOR_H
#define POLICY_VALIDATOR_H

#include <stdbool.h>
#include <stdint.h>
#ifdef ENABLE_VALIDATOR
#include "qemu_interface.h"
#endif

typedef struct PolicyValidatorConfig {
    bool enabled;
    const char* policy_path;
    const char* tag_info_file;
} PolicyValidatorConfig;


void set_policy_validator_enabled(bool val);
bool policy_validator_enabled(void);

void set_policy_validator_policy_path(const char* path);
void set_policy_validator_tag_info_file(const char* file);

const char* get_policy_validator_policy_path(void);
const char* get_policy_validator_tag_info_file(void);

void set_policy_validator_metadata(void);
bool policy_validator_commit(void);

void policy_validator_violation_msg(char* dest, int n);
void policy_validator_get_pc_tag(char *dest, int n);
void policy_validator_get_mem_tag(char *dest, int n, uint64_t addr);
void policy_validator_get_csr_tag(char *dest, int n, uint64_t addr);
void policy_validator_get_reg_tag(char *dest, int n, uint64_t addr);

#endif /* POLICY_VALIDATOR_H */
