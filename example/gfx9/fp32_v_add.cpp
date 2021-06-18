#include <string.h>
#include <iostream>
#include <fstream>

#include "dispatch.hpp"
#include "op_params.hpp"

using namespace amd::dispatch;

class VectorAdd : public Dispatch {
private:
  Buffer* in1;
  Buffer* in2;
  Buffer* out;
  unsigned length;
  std::string co_path;
  std::string build_cmd;

public:
  VectorAdd(std::string &co_path, std::string &build_cmd)
    : Dispatch(),
      length(64),
      co_path{std::move(co_path) },
      build_cmd{std::move(build_cmd) } { }

  bool SetupCodeObject() override {
    output << "Execute: " << build_cmd << std::endl;
    if (system(build_cmd.c_str())) {
        output  << "Error: assembly failed \n";
        return false;
    }

    return LoadCodeObjectFromFile(co_path);
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
    std::string co_path;
    std::string build_cmd;

    Options cli_ops(10);

    cli_ops.Add(&co_path, "-c", "", string(""), "Code object path", str2str);
    cli_ops.Add(&build_cmd, "-asm", "", string(""), "Shader build cmd", str2str);

    for (int i = 1; i <= argc-1; i += 2)
    {
        if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "-help"))
        {
            cli_ops.ShowHelp();
            exit(0);
            return false;
        }

        bool merged_flag = false;
        if (!cli_ops.ProcessArg(argv[i], argv[i+1], &merged_flag))
        {
            std::cerr << "Unknown flag or flag without value: " << argv[i] << "\n";
            return false;
        }

        if (merged_flag)
        {
            i--;
            continue;
        }

        if (argv[i+1] && cli_ops.MatchArg(argv[i+1]))
        {
            std::cerr << "Argument \"" << argv[i+1]
                << "\" is aliased with command line flags\n\t maybe real argument is missed for flag \""
                << argv[i] << "\"\n";
            return false;
        }
    }

    return VectorAdd(co_path, build_cmd).RunMain();
}
