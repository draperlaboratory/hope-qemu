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

void helper_validator_validate(CPURISCVState *env, target_ulong pc, uint32_t opcode)
{
   /* PC altering instructions will skip the commit helper. */
   if (skipped_commit) {
      if (e_v_commit()) {
         riscv_raise_exception(env, EXCP_DEBUG, GETPC());
      }
   }
   skipped_commit = true;

   policy_validator_hack_env = env;
   if (!e_v_validate((uint64_t)pc, opcode)) {
      char *msg = g_malloc(1024);
      policy_validator_violation_msg(msg, 1024);
      qemu_log("%s", msg);
      qemu_log("MSG: End test.\n");
      g_free(msg);
      exit(1);
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
