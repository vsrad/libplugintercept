use strict;

my $usage = << "ENDOFUSAGE";
Usage: $0 [<options>] <gcnasm_source>
    gcnasm_source          the source s file
    options
        -bs <size>      debug buffer size (mandatory)
        -ba <address>   debug buffer address (mandatory)
        -o <file>       output to the <file> rather than STDOUT
        -w <watches>    extra watches supplied colon separated in quotes;
                        watch type can be present
                        (like -w "a:b:c:i")
        -e <command>    instruction to insert after the injection
                        instead of "s_endpgm"; if "NONE" is supplied
                        then none is added
        -t <value>      target value for the loop counter
        -h              print usage information
ENDOFUSAGE

use POSIX;

my $fo      = *STDOUT;
my @watches;
my $endpgm  = "s_endpgm";
my $output  = 0;
my $bufsize;
my $bufaddr;
my $target;
my $input;

while (scalar @ARGV) {
  my $str = shift @ARGV;
  if ($str eq "-bs")  {  $bufsize =            shift @ARGV;  next;   }
  if ($str eq "-ba")  {  $bufaddr =            shift @ARGV;  next;   }
  if ($str eq "-o")   {  $_ = shift @ARGV;
                          open $fo, '>', $_ || die "$usage\nCould not open '$_': $!\n";
                                                              next;   }
  if ($str eq "-w")   {  @watches = split /:/, shift @ARGV;  next;   }
  if ($str eq "-e")   {  $endpgm  =            shift @ARGV;  next;   }
  if ($str eq "-t")   {  $target  =            shift @ARGV;  next;   }
  if ($str eq "-h")   {  print "$usage\n";                   last;   }

  open $input, '<', $str || die "$usage\nCould not open '$str: $!";
}

die $usage unless $input;

my $n_var   = scalar @watches;
my $to_dump = join ', ', @watches;

my $loopcounter = << "LOOPCOUNTER";
  // sgprDbgCounter = ttmp3
  // load counter from the buffer
  s_load_dword        ttmp3, [ttmp4, ttmp5], 0x8
  s_waitcnt           lgkmcnt(0)

  // inc counter and check it
  s_cbranch_scc1      debug_dumping_loop_counter_lab1_\\\@
  s_add_u32           ttmp3, ttmp3, 1
  s_cmp_eq_u32        ttmp3, $target
  s_cbranch_scc0      debug_dumping_loop_counter_lab_\\\@
debug_dumping_loop_counter_lab1_\\\@\:
  s_add_u32           ttmp3, ttmp3, 1
  s_cmp_lt_u32        ttmp3, $target
  s_cbranch_scc1      debug_dumping_loop_counter_lab_\\\@
LOOPCOUNTER
$loopcounter = "" unless $target;

my $dump_vars = "$watches[0]";
for (my $i = 1; $i < scalar @watches; $i += 1) {
  $dump_vars = "$dump_vars, $watches[$i]";
}

my $plug_macro = << "PLUGMACRO";
.macro m_debug_plug vars:vararg
$loopcounter
  // construct sgprDbgSrd
  s_mov_b32       ttmp7, 0x804fac
  // TODO: change n_var to buffer size
  s_add_u32           ttmp5, ttmp5, (($n_var + 1) << 18)
  s_mov_b32           ttmp2, exec_lo
  s_mov_b32           ttmp3, exec_hi
  v_mov_b32           v[vgprDbg], 0x7777777
  v_writelane_b32     v[vgprDbg], ttmp4, 1
  v_writelane_b32     v[vgprDbg], ttmp5, 2
  v_writelane_b32     v[vgprDbg], ttmp6, 3
  v_writelane_b32     v[vgprDbg], ttmp7, 4
  s_getreg_b32        ttmp2     , hwreg(4, 0, 32)   //  fun stuff
  v_writelane_b32     v[vgprDbg], ttmp2, 5
  s_getreg_b32        ttmp2     , hwreg(5, 0, 32)
  v_writelane_b32     v[vgprDbg], ttmp2, 6
  s_getreg_b32        ttmp2     , hwreg(6, 0, 32)
  v_writelane_b32     v[vgprDbg], ttmp2, 7
  v_writelane_b32     v[vgprDbg], exec_lo, 8
  v_writelane_b32     v[vgprDbg], exec_hi, 9
  s_mov_b64 exec,     -1
  buffer_store_dword  v[vgprDbg], off, [ttmp4, ttmp5, ttmp6, ttmp7], ttmp8 offset:0

  //var to_dump = [$to_dump]
  .if $n_var > 0
    buf_offset\\\@ = 0
    .irp var, \\vars
      buf_offset\\\@ = buf_offset\\\@ + 4
      v_mov_b32           v[vgprDbg], \\var
      buffer_store_dword  v[vgprDbg], off, [ttmp4, ttmp5, ttmp6, ttmp7], ttmp8 offset:0+buf_offset\\\@
    .endr
  .endif
  s_mov_b32           exec_lo, ttmp2
  s_mov_b32           exec_hi, ttmp3

  $endpgm
debug_dumping_loop_counter_lab_\\\@\:
  // save counter to the buffer
  s_store_dword       ttmp3, [ttmp4, ttmp5], 0x8 glc
  s_waitcnt           lgkmcnt(0)
.endm

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
  .set SQ_WAVE_PC_HI_TRAP_ID_BFE             , (SQ_WAVE_PC_HI_TRAP_ID_SHIFT | (SQ_WAVE_PC_HI_TRAP_ID_SIZE << 16))
  .set TTMP11_SAVE_RCNT_FIRST_REPLAY_SHIFT   , 26
  .set SQ_WAVE_IB_STS_FIRST_REPLAY_SHIFT     , 15
  .set SQ_WAVE_IB_STS_RCNT_FIRST_REPLAY_MASK , 0x1F8000

  // vgprDbg
  // sgprDbgStmp    = ttmp2
  // sgprDbgCounter = ttmp3
  // sgprDbgSrd     = [ttmp4, ttmp5, ttmp6, ttmp7]
  // sgprDbgSoff    = ttmp8

trap_entry:
  s_bfe_u32           ttmp2, ttmp1, SQ_WAVE_PC_HI_TRAP_ID_BFE

  // if not trap 1 or trap 2 then continue execution
  s_cmp_ge_u32        ttmp2, 0x3
  s_cbranch_scc1      exit_trap

trap_1:
  s_mov_b32           ttmp4, 0xFFFFFFFF & $bufaddr
  s_mov_b32           ttmp5, ($bufaddr >> 32)

  s_cmp_eq_u32        ttmp2, 0x2       // goto debug start if trapId = 2
  s_cbranch_scc1      trap_2

  s_store_dword       s2, [ttmp4, ttmp5], 0x0 glc     // save group id to the buffer
  v_readfirstlane_b32 ttmp2, v0
  s_store_dword       ttmp2, [ttmp4, ttmp5], 0x4 glc  // save thread id to the buffer
  s_mov_b32           ttmp2, 0x0
  s_store_dword       ttmp2, [ttmp4, ttmp5], 0x8 glc  // init counter (zero value)
  s_waitcnt           lgkmcnt(0)
  s_branch            exit_trap

// debug plug resource allocation
//n_var    = $n_var
//vars     = $dump_vars
trap_2:
  s_load_dword        ttmp8, [ttmp4, ttmp5]       // load gid_x value
  s_load_dword        ttmp3, [ttmp4, ttmp5], 0x4  // load tid_x[0] value  
  s_waitcnt           lgkmcnt(1)
  s_mul_i32           ttmp8, ttmp8, 8 //waves_in_group
  s_waitcnt           lgkmcnt(0)
  s_lshr_b32          ttmp3, ttmp3, 6 //wave_size_log2
  s_add_u32           ttmp8, ttmp8, ttmp3
  s_mul_i32           ttmp8, ttmp8, 64 * (1 + $n_var) * 4
  s_mov_b32           ttmp3, 0

dump_watches:
  m_debug_plug $dump_vars

exit_trap:
  s_add_u32           ttmp0, ttmp0, 0x4
  s_addc_u32          ttmp1, ttmp1, 0x0

  // Return to shader at unmodified PC.
  s_rfe_b64           [ttmp0, ttmp1]
PLUGMACRO

my $current_line = 0;
while (<$input>) {
  if (/\.GPR_ALLOC_END/) {
    print $fo ".VGPR_ALLOC vgprDbg // used by trap handler\n$_";
  }
  elsif (/\.end_amd_kernel_code_t/) {
    print $fo "$_ s_trap 1\n";
  }
  else {
    print $fo $_;
  }
  $current_line++;
}
print $fo "\n$plug_macro\n";