#include <httplib.h>
#include <iostream>

int main(void)
{
  // IMPORTANT: 1st parameter must be a hostname or an IP adress string.
  httplib::Client cli("localhost", 1234);

  auto res = cli.Get("/hi");
  if (res && res->status == 200) {
    std::cout << res->body << std::endl;
  }
  res = cli.Get("/stop");
  if (res && res->status == 200) {
    std::cout << res->body << std::endl;
  }
}