#include <fstream>
#include <iostream>
#include <sstream>

#include "v/interp.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "dùng: vproof <file.v>\n";
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::cerr << "không mở được " << argv[1] << "\n";
    return 2;
  }
  std::stringstream buf;
  buf << in.rdbuf();

  v::Interp interp;
  try {
    auto rep = interp.run(buf.str());
    for (const std::string& l : rep.lines) std::cout << l << "\n";
    std::cout << "\n" << rep.checksRun << " kiểm tra, " << rep.checksFailed << " sai\n";
    return rep.checksFailed ? 1 : 0;
  } catch (const std::exception& e) {
    std::cerr << "lỗi: " << e.what() << "\n";
    return 1;
  }
}
