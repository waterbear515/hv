#pragma once

#include "page-tables.h"
#include "guest-context.h"
#include "ept.h"

#include <ia32.hpp>

namespace hv {

// TODO: why are all these constants defined in vcpu.h?

// selectors for the host GDT
inline constexpr segment_selector host_cs_selector = { 0, 0, 1 };
inline constexpr segment_selector host_tr_selector = { 0, 0, 2 };

// number of available descriptor slots in the host GDT
inline constexpr size_t host_gdt_descriptor_count = 4;

// number of available descriptor slots in the host IDT
inline constexpr size_t host_idt_descriptor_count = 256;

// size of the host stack for handling vm-exits
inline constexpr size_t host_stack_size = 0x6000;

// the first 128GB of physical memory is mapped to this pml4 entry
inline constexpr uint64_t host_physical_memory_pml4_idx = 255;

// directly access physical memory by using [base + offset]
inline uint8_t* const host_physical_memory_base = reinterpret_cast<uint8_t*>(
  host_physical_memory_pml4_idx << (9 + 9 + 9 + 12));

// guest virtual-processor identifier
inline constexpr uint16_t guest_vpid = 1;

// signature that is returned by the ping hypercall
inline constexpr uint64_t hypervisor_signature = 'jono';

struct vcpu_cached_data {
  // maximum number of bits in a physical address (MAXPHYSADDR)
  uint64_t max_phys_addr;

  // reserved bits in CR0/CR4
  uint64_t vmx_cr0_fixed0;
  uint64_t vmx_cr0_fixed1;
  uint64_t vmx_cr4_fixed0;
  uint64_t vmx_cr4_fixed1;

  // mask of unsupported processor state components for XCR0
  uint64_t xcr0_unsupported_mask;

  // IA32_FEATURE_CONTROL
  ia32_feature_control_register feature_control;

  // IA32_VMX_MISC
  ia32_vmx_misc_register vmx_misc;

  // CPUID 0x01
  cpuid_eax_01 cpuid_01;
};

struct vcpu {
  // 4 KiB vmxon region
  alignas(0x1000) vmxon vmxon;

  // 4 KiB vmcs region
  alignas(0x1000) vmcs vmcs;

  // 4 KiB msr bitmap
  alignas(0x1000) vmx_msr_bitmap msr_bitmap;

  // host stack used for handling vm-exits
  alignas(0x1000) uint8_t host_stack[host_stack_size];

  // host interrupt descriptor table
  alignas(0x1000) segment_descriptor_interrupt_gate_64 host_idt[host_idt_descriptor_count];

  // host global descriptor table
  alignas(0x1000) segment_descriptor_32 host_gdt[host_gdt_descriptor_count];

  // host task state segment
  alignas(0x1000) task_state_segment_64 host_tss;

  // EPT paging structures
  alignas(0x1000) vcpu_ept_data ept;

  // cached values that are assumed to NEVER change
  vcpu_cached_data cached;

  // pointer to the current guest context, set in exit-handler
  guest_context* ctx;

  // current TSC offset
  uint64_t tsc_offset;

  // current preemption timer
  uint64_t preemption_timer;

  // the latency caused by world-transitions
  uint64_t vm_exit_tsc_latency;

  // whether to use TSC offsetting for the current vm-exit--false by default
  bool hide_vm_exit_latency;
};

// virtualize the specified cpu. this assumes that execution is already
// restricted to the desired logical proocessor.
bool virtualize_cpu(vcpu* cpu);

// toggle vm-exiting for the specified MSR through the MSR bitmap
void enable_exiting_for_msr(vcpu* cpu, uint32_t msr, bool enabled);

} // namespace hv

