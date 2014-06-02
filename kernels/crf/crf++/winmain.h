//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: common.h 1588 2007-02-12 09:03:39Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#if defined(_WIN32) || defined(__CYGWIN__)

#include <windows.h>
#include <string>

namespace {
class CommandLine {
 public:
  CommandLine(int argc, wchar_t **argv) : argc_(argc), argv_(0) {
    argv_ = new char * [argc_];
    for (int i = 0; i < argc_; ++i) {
      const std::string arg = WideToUtf8(argv[i]);
      argv_[i] = new char[arg.size() + 1];
      ::memcpy(argv_[i], arg.data(), arg.size());
      argv_[i][arg.size()] = '\0';
    }
  }
  ~CommandLine() {
    for (int i = 0; i < argc_; ++i) {
      delete [] argv_[i];
    }
    delete [] argv_;
  }

  int argc() const { return argc_; }
  char **argv() const { return argv_; }

 private:
  static std::string WideToUtf8(const std::wstring &input) {
    const int output_length = ::WideCharToMultiByte(CP_UTF8, 0,
                                                    input.c_str(), -1, NULL, 0,
                                                    NULL, NULL);
    if (output_length == 0) {
      return "";
    }

    char *input_encoded = new char[output_length + 1];
    const int result = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1,
                                             input_encoded,
                                             output_length + 1, NULL, NULL);
    std::string output;
    if (result > 0) {
      output.assign(input_encoded);
    }
    delete [] input_encoded;
    return output;
  }

  int argc_;
  char **argv_;
};
}  // namespace

#define main(argc, argv) wmain_to_main_wrapper(argc, argv)

int wmain_to_main_wrapper(int argc, char **argv);

int wmain(int argc, wchar_t **argv) {
  CommandLine cmd(argc, argv);
  return wmain_to_main_wrapper(cmd.argc(), cmd.argv());
}
#endif
