use strict;

my $usage = << "ENDOFUSAGE";
Usage: $0 [<options>] <gcnasm_source>
    gcnasm_source          the source s file
    options
        -bs <size>      debug buffer size (mandatory)
        -ba <address>   debug buffer address (mandatory)
        -ha <address>   debug hidden buffer address (mandatory)
        -l <line>       line number to break (mandatory)
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
my $line    = 0;
my $endpgm  = "s_endpgm";
my $output  = 0;
my $bufsize;
my $bufaddr;
my $hidaddr;
my $target;
my $input;

while (scalar @ARGV) {
  my $str = shift @ARGV;
  if ($str eq "-bs")  {  $bufsize =            shift @ARGV;  next;   }
  if ($str eq "-ba")  {  $bufaddr =            shift @ARGV;  next;   }
  if ($str eq "-ha")  {  $hidaddr =            shift @ARGV;  next;   }
  if ($str eq "-l")   {  $line    =            shift @ARGV;  next;   }
  if ($str eq "-o")   {  $_ = shift @ARGV;
                          open $fo, '>', $_ || die "$usage\nCould not open '$_': $!\n";
                                                              next;   }
  if ($str eq "-w")   {  @watches = split /:/, shift @ARGV;  next;   }
  if ($str eq "-e")   {  $endpgm  =            shift @ARGV;  next;   }
  if ($str eq "-t")   {  $target  =            shift @ARGV;  next;   }
  if ($str eq "-h")   {  print "$usage\n";                   last;   }

  open $input, '<', $str || die "$usage\nCould not open '$str: $!";
}

die $usage unless $line && $input;

my $n_var   = scalar @watches;

my $dump_vars = "$watches[0]";
for (my $i = 1; $i < scalar @watches; $i += 1) {
  $dump_vars = "$dump_vars, $watches[$i]";
}

$target = 1 unless $target;
my $loopcounter = << "LOOPCOUNTER";
  // inc counter and check it
  s_add_u32       ttmp4, ttmp4, 1
  s_cbranch_scc1  debug_dumping_loop_counter_lab1
debug_dumping_loop_counter_lab0:
  s_cmp_eq_u32    ttmp4, $target
  s_cbranch_scc0  goto_skip_dump_instruction
  s_cmp_lg_u32    ttmp4, $target
  s_branch        debug_dumping_loop_counter_continue
debug_dumping_loop_counter_lab1:
  s_cmp_lg_u32    ttmp4, $target
  s_cbranch_scc1  goto_skip_dump_instruction
  s_cmp_eq_u32    ttmp4, $target
debug_dumping_loop_counter_continue:
LOOPCOUNTER

my $trap_handler = << "TRAPHANDLER";
.amdgpu_hsa_kernel trap_handler

trap_handler:
  .amd_kernel_code_t
    is_ptr64 = 1
    enable_sgpr_queue_ptr = 1
    granulated_workitem_vgpr_count = 1
    granulated_wavefront_sgpr_count = 1
    user_sgpr_count = 2
  .end_amd_kernel_code_t

.macro m_return_pc offset=0
  s_add_u32           ttmp0, ttmp0, \\offset
  s_addc_u32          ttmp1, ttmp1, 0x0

  // Return to shader at unmodified PC.
  s_rfe_b64           [ttmp0, ttmp1]
.endm

.macro m_init_debug_srd addr, n_val
  s_mov_b32           ttmp8, 0xFFFFFFFF & \\addr
  s_mov_b32           ttmp9, (\\addr >> 32)
  s_mov_b32           ttmp11, 0x804fac
  // TODO: change n_var to buffer size
  s_add_u32           ttmp9, ttmp9, (\\n_val << 18)
.endm

.macro m_init_buffer_debug_srd
  m_init_debug_srd $bufaddr, ($n_var + 1)
.endm

.macro m_init_hidden_debug_srd
  m_init_debug_srd $hidaddr, 1
.endm

  .set SQ_WAVE_PC_HI_TRAP_ID_SHIFT           , 16
  .set SQ_WAVE_PC_HI_TRAP_ID_SIZE            , 8
  .set SQ_WAVE_PC_HI_TRAP_ID_BFE             , (SQ_WAVE_PC_HI_TRAP_ID_SHIFT | (SQ_WAVE_PC_HI_TRAP_ID_SIZE << 16))
  .set TTMP11_SAVE_RCNT_FIRST_REPLAY_SHIFT   , 26
  .set SQ_WAVE_IB_STS_FIRST_REPLAY_SHIFT     , 15
  .set SQ_WAVE_IB_STS_RCNT_FIRST_REPLAY_MASK , 0x1F8000

  // vgprDbg
  // sgprDbgStmp      = ttmp2
  // sgprDbgCounter   = ttmp4
  // sgprDbgSoff      = ttmp5
  // sgprDbgDumpCount = ttmp7
  // sgprDbgSrd       = [ttmp8, ttmp9, ttmp10, ttmp11]
  //  n_var           = $n_var
  //  vars            = $dump_vars
trap_entry:
  s_bfe_u32           ttmp2, ttmp1, SQ_WAVE_PC_HI_TRAP_ID_BFE

  // if not trap 1 or trap 2 then continue execution
  s_cmp_ge_u32        ttmp2, 0x5
  s_cbranch_scc1      trap_exit

  s_cmp_eq_u32        ttmp2, 0x2       // goto debug start if trapId = 2
  s_cbranch_scc1      trap_2
  s_cmp_eq_u32        ttmp2, 0x3       // goto  dump watch if trapId = 3
  s_cbranch_scc1      trap_3
  s_cmp_eq_u32        ttmp2, 0x4       // goto  restore vgprDbg if trapId = 4
  s_cbranch_scc1      trap_4

trap_1:
  // init buffer offset
  s_mul_i32           ttmp5,  s[2] , 8 //gid_x * waves_in_group
  v_readfirstlane_b32 ttmp4,  v0
  s_lshr_b32          ttmp4,  ttmp4, 6 //wave_size_log2
  s_add_u32           ttmp5,  ttmp5, ttmp4
  s_mov_b32           ttmp7,  ttmp5
  s_mul_i32           ttmp5,  ttmp5, 64 * (1 + $n_var) * 4
  s_mul_i32           ttmp7,  ttmp7, 64 * (1) * 4
  s_mov_b32           ttmp4,  0

  s_branch            goto_skip_dump_instruction

trap_2:
$loopcounter

  m_init_hidden_debug_srd
  s_mov_b32           ttmp12    , exec_lo
  s_mov_b32           ttmp13    , exec_hi
  s_mov_b64           exec      , -1
  buffer_store_dword  v[vgprDbg], off, [ttmp8, ttmp9, ttmp10, ttmp11], ttmp7, offset:0  // save vgprDbg

  m_init_buffer_debug_srd
  v_mov_b32           v[vgprDbg], 0x7777777
  v_writelane_b32     v[vgprDbg], ttmp8 , 1
  v_writelane_b32     v[vgprDbg], ttmp9 , 2
  v_writelane_b32     v[vgprDbg], ttmp10, 3
  v_writelane_b32     v[vgprDbg], ttmp11, 4
  s_getreg_b32        ttmp2     , hwreg(4, 0, 32)   //  fun stuff
  v_writelane_b32     v[vgprDbg], ttmp2, 5
  s_getreg_b32        ttmp2     , hwreg(5, 0, 32)
  v_writelane_b32     v[vgprDbg], ttmp2, 6
  s_getreg_b32        ttmp2     , hwreg(6, 0, 32)
  v_writelane_b32     v[vgprDbg], ttmp2, 7
  v_writelane_b32     v[vgprDbg], ttmp12, 8
  v_writelane_b32     v[vgprDbg], ttmp13, 9
  buffer_store_dword  v[vgprDbg], off, [ttmp8, ttmp9, ttmp10, ttmp11], ttmp5, offset:0
  s_mov_b32           exec_lo   , ttmp12
  s_mov_b32           exec_hi   , ttmp13

  // if counter == target then goto_debug_dump
  s_branch            goto_debug_dump

trap_3:
  m_init_buffer_debug_srd
  s_add_u32           ttmp5, ttmp5, 0x4
  buffer_store_dword  v[vgprDbg], off, [ttmp8, ttmp9, ttmp10, ttmp11], ttmp5, offset:0
  s_branch            trap_exit

trap_4:
  m_init_hidden_debug_srd
  buffer_load_dword   v[vgprDbg], off, [ttmp8, ttmp9, ttmp10, ttmp11], ttmp7, offset:0
  s_waitcnt           0

trap_exit:
goto_skip_dump_instruction:
  m_return_pc         4

goto_debug_dump:
  m_return_pc         8
TRAPHANDLER

my $current_line = 0;
while (<$input>) {
  if (/\.end_amd_kernel_code_t/) {
    print $fo $_ . ".set vgprDbg, 0\n";
    print $fo      "s_trap 1\n";
  }
  elsif ($current_line == $line) {
    print $fo 
"s_trap       2
s_branch     skip_dump\n";

  foreach my $watch (@watches) {
    print $fo 
"v_mov_b32    v[vgprDbg], $watch
s_trap       3\n";
  }
  print $fo "s_trap       4 // restore vgprDbg\n";
  print $fo "$endpgm\n";
  print $fo "skip_dump:\n$_";
  }
  else {
    print $fo $_;
  }
  $current_line++;
}
print $fo "\n$trap_handler\n";

die "Break line out of range" if $current_line < $line;