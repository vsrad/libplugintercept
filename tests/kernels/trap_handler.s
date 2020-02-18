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

  .set SQ_WAVE_PC_HI_TRAP_ID_SHIFT           , 16
  .set SQ_WAVE_PC_HI_TRAP_ID_SIZE            , 8
  .set SQ_WAVE_PC_HI_TRAP_ID_BFE               , (SQ_WAVE_PC_HI_TRAP_ID_SHIFT | (SQ_WAVE_PC_HI_TRAP_ID_SIZE << 16))
  .set TTMP11_SAVE_RCNT_FIRST_REPLAY_SHIFT   , 26
  .set SQ_WAVE_IB_STS_FIRST_REPLAY_SHIFT     , 15
  .set SQ_WAVE_IB_STS_RCNT_FIRST_REPLAY_MASK , 0x1F8000

  // ABI between first and second level trap handler:
  //   ttmp0 = PC[31:0]
  //   ttmp1 = 0[2:0], PCRewind[3:0], HostTrap[0], TrapId[7:0], PC[47:32]
  //   ttmp12 = SQ_WAVE_STATUS
  //   ttmp14 = TMA[31:0]
  //   ttmp15 = TMA[63:32]
  // gfx9:
  //   ttmp11 = SQ_WAVE_IB_STS[20:15], 0[17:0], DebugTrap[0], NoScratch[0], WaveIdInWG[5:0]
  // gfx10:
  //   ttmp11 = SQ_WAVE_IB_STS[25], SQ_WAVE_IB_STS[21:15], 0[15:0], DebugTrap[0], NoScratch[0], WaveIdInWG[5:0]


  // 1. Check user trap ID (non-zero)
  s_bfe_u32            ttmp2, ttmp1, SQ_WAVE_PC_HI_TRAP_ID_BFE
  s_cbranch_scc0       exit_trap

  s_add_u32            s0, s0, 7

exit_trap:
  // Restore SQ_WAVE_IB_STS
  s_lshr_b32           ttmp2, ttmp11, (TTMP11_SAVE_RCNT_FIRST_REPLAY_SHIFT - SQ_WAVE_IB_STS_FIRST_REPLAY_SHIFT)
  s_and_b32            ttmp2, ttmp2, SQ_WAVE_IB_STS_RCNT_FIRST_REPLAY_MASK
  s_setreg_b32         hwreg(HW_REG_IB_STS), ttmp2

  // Restore SQ_WAVE_STATUS.
  s_and_b64            exec, exec, exec // Restore STATUS.EXECZ, not writable by s_setreg_b32
  s_and_b64            vcc, vcc, vcc    // Restore STATUS.VCCZ, not writable by s_setreg_b32
  s_setreg_b32         hwreg(HW_REG_STATUS), ttmp12

  // Return to shader at unmodified PC.
  s_rfe_b64            [ttmp0, ttmp1]
