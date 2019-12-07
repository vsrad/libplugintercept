.ifndef M_DEBUG_PLUG_DEFINED
.set M_DEBUG_PLUG_DEFINED,1

//n_var    = 1
//vars     = v[tid]

.macro m_dbg_gpr_alloc
  .VGPR_ALLOC dbg_vtmp

  .if .SGPR_NEXT_FREE % 4
    .SGPR_ALLOC_ONCE dbg_soff
  .endif

  .if .SGPR_NEXT_FREE % 4
    .SGPR_ALLOC_ONCE dbg_counter
  .endif

  .if .SGPR_NEXT_FREE % 4
    .SGPR_ALLOC_ONCE dbg_stmp
  .endif

  .SGPR_ALLOC dbg_srd, 4

  .SGPR_ALLOC_ONCE dbg_soff
  .SGPR_ALLOC_ONCE dbg_counter
  .SGPR_ALLOC_ONCE dbg_stmp
  M_DBG_GPR_ALLOC_INSTANTIATED = 1
.endm

.macro m_dbg_init gidx
  debug_init_start:
  s_mul_i32 s[dbg_soff], s[\gidx], 8 //waves_in_group
  v_readfirstlane_b32 s[dbg_counter], v[tid]
  s_lshr_b32 s[dbg_counter], s[dbg_counter], 6 //wave_size_log2
  s_add_u32 s[dbg_soff], s[dbg_soff], s[dbg_counter]
  s_mul_i32 s[dbg_soff], s[dbg_soff], 64 * (1 + 1) * 4

  s_mov_b32 s[dbg_counter], 0
  M_DBG_INIT_INSTANTIATED = 1
  debug_init_end:
.endm

.macro m_debug_plug vars:vararg
  .ifndef M_DBG_INIT_INSTANTIATED
    .error "Debug macro is not instantiated (m_dbg_init)"
  .endif
  .ifndef M_DBG_GPR_ALLOC_INSTANTIATED
    .error "Debug macro is not instantiated (m_dbg_gpr_alloc)"
  .endif
//  debug dumping dongle begin

  v_save   = dbg_vtmp
  s_srd    = dbg_srd
  s_grp    = dbg_soff

  // construct dbg_srd
  s_mov_b32 s[dbg_srd+0], 0x7f7f7f7f
  s_mov_b32 s[dbg_srd+1], 0x7f7f7f7f
  s_mov_b32 s[dbg_srd+3], 0x804fac
  s_add_u32 s[dbg_srd+1], s[dbg_srd+1], ((1 + 1) << 18)

  s_mov_b32 s[dbg_stmp], exec_lo
  s_mov_b32 s[dbg_counter], exec_hi
  v_mov_b32       v[v_save], 0x7777777
  v_writelane_b32 v[v_save], s[s_srd+0], 1
  v_writelane_b32 v[v_save], s[s_srd+1], 2
  v_writelane_b32 v[v_save], s[s_srd+2], 3
  v_writelane_b32 v[v_save], s[s_srd+3], 4
  s_getreg_b32    s[dbg_stmp], hwreg(4, 0, 32)   //  fun stuff
  v_writelane_b32 v[v_save], s[dbg_stmp], 5
  s_getreg_b32    s[dbg_stmp], hwreg(5, 0, 32)
  v_writelane_b32 v[v_save], s[dbg_stmp], 6
  s_getreg_b32    s[dbg_stmp], hwreg(6, 0, 32)
  v_writelane_b32 v[v_save], s[dbg_stmp], 7
  v_writelane_b32 v[v_save], exec_lo, 8
  v_writelane_b32 v[v_save], exec_hi, 9
  s_mov_b64 exec, -1

  buffer_store_dword v[v_save], off, s[s_srd:s_srd+3], s[s_grp] offset:0

  //var to_dump = [v[tid]]
  .if 1 > 0
    buf_offset\@ = 0
    .irp var, \vars
      buf_offset\@ = buf_offset\@ + 4
      v_mov_b32 v[v_save], \var
      buffer_store_dword v[v_save], off, s[s_srd:s_srd+3], s[s_grp] offset:0+buf_offset\@
    .endr
  .endif

  s_mov_b32 exec_lo, s[dbg_stmp]
  s_mov_b32 exec_hi, s[dbg_counter]

  s_endpgm
  debug_dumping_loop_counter_lab_\@:
//  debug dumping_dongle_end_:
.endm
.endif

.include "gpr_alloc.inc"

.hsa_code_object_version 2,1
.hsa_code_object_isa

.GPR_ALLOC_BEGIN
  kernarg = 0
  gid_x = 2
  .SGPR_ALLOC_FROM 6
  .SGPR_ALLOC base_in1, 2
  .SGPR_ALLOC base_in2, 2
  .SGPR_ALLOC base_out, 2

  .VGPR_ALLOC_FROM 0
  .VGPR_ALLOC tid
  .VGPR_ALLOC voffset
  .VGPR_ALLOC vaddr, 2
  .VGPR_ALLOC in1
  .VGPR_ALLOC in2
  .VGPR_ALLOC out
m_dbg_gpr_alloc
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

m_dbg_init gid_x

  v_lshlrev_b32      v[tid], 2, v[tid]
m_debug_plug v[tid]
  s_endpgm
