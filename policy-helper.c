/*
 * RISC-V Policy Validator FPU Emulation Helpers for QEMU.
 *
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "cpu.h"
#include "qemu/main-loop.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"

#include "policy_validator.h"

#ifdef ENABLE_VALIDATOR
static bool skipped_commit = false;

extern CPURISCVState* policy_validator_hack_env;
extern CPUState* policy_validator_hack_cpu_state;

void helper_validator_validate(CPURISCVState *env, target_ulong pc, uint32_t opcode, void* cpu_ptr)
{
   /* PC altering instructions will skip the commit helper. */
   CPUState* cpu_state = (CPUState*) cpu_ptr;
   if (skipped_commit) {
      if (e_v_commit()) {
         riscv_raise_exception(env, EXCP_DEBUG, GETPC());
      }
   }
   skipped_commit = true;

   policy_validator_hack_env = env;
   policy_validator_hack_cpu_state = cpu_state;
   if (!e_v_validate((uint64_t)pc, opcode)) {
      char *msg = g_malloc(1024);
      policy_validator_violation_msg(msg, 1024);
      qemu_log("%s", msg);
      g_free(msg);

      if (policy_validator_exc()) {
         riscv_raise_exception(env, RISCV_POLICY_VIOLATION_FAULT, GETPC());
      } else {
         qemu_log("MSG: End test.\n");
         exit(1);
      }
   }
}

void helper_validator_commit(CPURISCVState *env)
{
   skipped_commit = false;

   if (e_v_commit()) {
      riscv_raise_exception(env, EXCP_DEBUG, GETPC());
   }
}
#endif
