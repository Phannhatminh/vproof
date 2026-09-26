#include <fstream>
#include <iostream>
#include <sstream>

#include "v/interp.hpp"

int main(int argc, char** argv) {
  bool keepGoing = false;
  const char* path = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--keep-going") keepGoing = true;
    else path = argv[i];
  }
  if (!path) {
    std::cerr << "usage: vproof [--keep-going] <file.v>\n";
    return 2;
  }
  std::ifstream in(path);
  if (!in) {
    std::cerr << "cannot open " << path << "\n";
    return 2;
  }
  std::stringstream buf;
  buf << in.rdbuf();

  v::Interp interp;
  try {
    auto rep = interp.run(buf.str(), keepGoing);
    for (const std::string& l : rep.lines) std::cout << l << "\n";
    std::cout << "\n" << rep.checksRun << (rep.checksRun == 1 ? " check, " : " checks, ")
              << rep.checksFailed << " failed";
    if (keepGoing)
      std::cout << ", " << rep.stepsStuck << (rep.stepsStuck == 1 ? " step" : " steps")
                << " stuck";
    std::cout << "\n";
    return rep.checksFailed || rep.stepsStuck ? 1 : 0;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
