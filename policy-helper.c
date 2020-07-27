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

static inline uint64_t addrfix(uint64_t vaddr)
{
    CPUClass *cc = CPU_GET_CLASS(policy_validator_hack_cpu_state);
    uint64_t page_addr = cc->get_phys_page_debug(policy_validator_hack_cpu_state, vaddr);
    uint64_t page_offset = vaddr & (qemu_real_host_page_size-1);
    if (vaddr == 0xffffffe000000000) {
      printf("vaddr: %lx, paddr: %lx\n", vaddr, page_addr | page_offset);
    }
    if (page_addr == -1)
        return vaddr;
    return page_addr | page_offset;
}


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

   if (pc >= 0x10190 && pc <= 0x103dc) {
     qemu_log("pc: %lx (%lx)\n", (uint64_t)pc, (uint64_t)addrfix(pc));
   }

   policy_validator_hack_env = env;
   policy_validator_hack_cpu_state = cpu_state;
   if (!e_v_validate((uint64_t)pc, opcode)) {
      char *msg = g_malloc(1024);
      policy_validator_violation_msg(msg, 1024);
      qemu_log("Violation PC phys: %lx\n", (uint64_t)addrfix(pc));
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
