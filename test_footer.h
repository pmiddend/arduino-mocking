#pragma once

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>

int main(int argc, char *argv[]) {
  {
    if (argc != 2) {
      std::cerr << "usage: " << argv[0] << " <protocol-file>\n";
      return EXIT_FAILURE;
    }
    std::ifstream protocol_stream(argv[1]);
    while (protocol_stream) {
      std::string line;
      if (!std::getline(protocol_stream, line)) {
        break;
      }
      if (line.empty() || line[0] == '#')
        continue;
      protocol.push_back(std::stoi(line));
    }
  }
  /* std::cout << "protocol is:\n"; */
  /* for (auto it = protocol.cbegin(); it != protocol.cend(); ++it) { */
  /*   std::cout << static_cast<int>(*it) << "\n"; */
  /* } */
  /* std::cout << "protocol end\n"; */

  setup();
  for (unsigned i = 0;;i++) {
    std::cout << "iteration " << i << "\n";
    loop();
    if (current_protocol_element >= protocol.size()) {
      std::cout << "No more input, quitting\n";
      return EXIT_SUCCESS;
    }
    std::cout << "<ENTER> for next iteration\n";
    std::getchar();
  }
}
