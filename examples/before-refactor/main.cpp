#include "OrderProcessor.hpp"

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

int main() {
  std::cout << "EDUCATIONAL ANTI-PATTERN DEMO — examples/before-refactor\n";
  std::cout << "This path is runnable teaching material. The maintained design is order_demo.\n";

  cs2511::before::OrderProcessor processor;
  const std::vector<std::tuple<std::string, int, int>> items{
      {"SKU-1", 6, 100},
      {"SKU-2", 4, 50},
  };
  const int code = processor.process("1001", "cust-9", "ada@example.com", "5550100", items,
                                     "bulk", "email");
  if (code != 0) {
    std::cerr << processor.lastError() << '\n';
    return 1;
  }

  std::cout << "totalCents=" << processor.lastTotalCents() << '\n';
  for (const std::string& event : processor.events()) {
    std::cout << event << '\n';
  }
  for (const std::string& note : processor.notifications()) {
    std::cout << note << '\n';
  }
  return 0;
}
