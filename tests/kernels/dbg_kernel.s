.include "gpr_alloc.inc"

.hsa_code_object_version 2,1
.hsa_code_object_isa

.GPR_ALLOC_BEGIN
  kernarg = 0
  gid_x = 2

  .VGPR_ALLOC_FROM 0
  .VGPR_ALLOC tid
  .VGPR_ALLOC tid_dump
.GPR_ALLOC_END

.text
.p2align 8
.amdgpu_hsa_kernel dbg_kernel

dbg_kernel:
  .amd_kernel_code_t
    is_ptr64 = 1
    enable_sgpr_kernarg_segment_ptr = 1
    enable_sgpr_workgroup_id_x = 1
    kernarg_segment_byte_size = 24
    compute_pgm_rsrc2_user_sgpr = 2
    granulated_workitem_vgpr_count = .AUTO_VGPR_GRANULATED_COUNT
    granulated_wavefront_sgpr_count = .AUTO_SGPR_GRANULATED_COUNT
    wavefront_sgpr_count = .AUTO_SGPR_COUNT
    workitem_vgpr_count = .AUTO_VGPR_COUNT
  .end_amd_kernel_code_t

  v_mov_b32 v[tid_dump], v[tid]
  s_trap 2
  s_endpgm
