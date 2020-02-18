.hsa_code_object_version 2,1
.hsa_code_object_isa

.text
.p2align 8
.amdgpu_hsa_kernel trap_handler

trap_handler:
  .amd_kernel_code_t
    is_ptr64 = 1
    enable_sgpr_queue_ptr = 1
    granulated_workitem_vgpr_count = 1
    granulated_wavefront_sgpr_count = 1
    user_sgpr_count = 2
  .end_amd_kernel_code_t

  v_add_u32 v1, v1, 7

  s_add_u32 ttmp0, ttmp0, 4
  s_addc_u32 ttmp1, ttmp1, 0

  s_rfe_b64 [ttmp0, ttmp1]
