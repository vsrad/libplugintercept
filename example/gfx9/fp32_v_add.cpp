#include "dispatch.hpp"
#include <string.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

using namespace amd::dispatch;
using namespace boost::program_options;

class VectorAdd : public Dispatch {
private:
  Buffer* in1;
  Buffer* in2;
  Buffer* out;
  unsigned length;
  std::string clang;
  std::string asm_source;
  std::string include_dir;
  std::string output_path;

public:
  VectorAdd(std::string &clang,
    std::string &asm_source,
    std::string &include_dir,
    std::string &output_path)
    : Dispatch(),
      length(64),
      clang{std::move(clang) },
      asm_source{std::move(asm_source) },
      include_dir{std::move(include_dir) },
      output_path{std::move(output_path) } { }

  bool SetupCodeObject() override {
    std::string clang_call = clang + " -x assembler -target amdgcn--amdhsa -mcpu=gfx900 -mno-code-object-v3 -I" + include_dir
      + " -o " + output_path + " " + asm_source;

    output << "Execute: " << clang_call << std::endl;
    if (system(clang_call.c_str())) { output << "Error: kernel build failed - " << asm_source << std::endl; return false; }

    return LoadCodeObjectFromFile(output_path);
  }

  bool Setup() override {
    if (!AllocateKernarg(3 * sizeof(Buffer*))) { return false; }
    in1 = AllocateBuffer(length * sizeof(float));
    in2 = AllocateBuffer(length * sizeof(float));
    for (unsigned i = 0; i < length; ++i) {
      in1->Data<float>(i) = (float)i;
      in2->Data<float>(i) = ((float)i) * 1.25f;
    }
    if (!CopyTo(in1)) { output << "Error: failed to copy to local" << std::endl; return false; }
    if (!CopyTo(in2)) { output << "Error: failed to copy to local" << std::endl; return false; }
    out = AllocateBuffer(length * sizeof(float));

    Kernarg(in1);
    Kernarg(in2);
    Kernarg(out);
    SetGridSize(64);
    SetWorkgroupSize(64);
    return true;
  }

  bool Verify() override {
    if (!CopyFrom(out)) { output << "Error: failed to copy from local" << std::endl; return false; }

    bool ok = true;
    for (unsigned i = 0; i < length; ++i) {
      float f1 = in1->Data<float>(i);
      float f2 = in2->Data<float>(i);
      float res = out->Data<float>(i);
      float expected = f1 + f2;
      if (expected != res){
        output << "Error: validation failed at " << i << ": got " << res << " expected " << expected << std::endl;
        ok = false;
      }
    }
    return ok;
  }
};

int main(int argc, const char** argv)
{
  try {
    options_description desc("General options");
    desc.add_options()
    ("help,h", "usage message")
    ("clang", value<std::string>()->default_value("clang"), "clang compiler path")
    ("asm", value<std::string>()->default_value("fp32_v_add.s"), "kernel source")
    ("include", value<std::string>()->default_value("./"), "inclide directories")
    ("output_path", value<std::string>()->default_value("fp32_v_add.co"), "kernel binary output path")
    ;

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    std::string clang = vm["clang"].as<std::string>();
    std::string asm_source = vm["asm"].as<std::string>();;
    std::string include_dir = vm["include"].as<std::string>();
    std::string output_path = vm["output_path"].as<std::string>();

    return VectorAdd(
      clang,
      asm_source,
      include_dir,
      output_path).RunMain();
  }
  catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}
