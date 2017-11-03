#include "common.h"

#include <iostream>

int main(int argc, char* argv[])
{
  auto attr1 = rise::RealAttribute::create(5.0);
  auto attr2 = rise::RealAttribute::create("5.0");
  auto attr3 = rise::RealAttribute::create("?");
  auto attr4 = rise::NominalAttribute::create("sunny");
  std::cout << "attr1: " << S(attr1) << std::endl;
  std::cout << "attr2: " << S(attr2) << std::endl;
  std::cout << "attr3: " << S(attr3) << std::endl;
  std::cout << "attr4: " << S(attr4) << std::endl;
}



