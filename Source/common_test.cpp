#include "common.h"

#include <iostream>

int main(int argc, char* argv[])
{
  rise::Attribute* attr1 = new rise::RealAttribute("time", 0, 10);
  rise::Attribute* attr2 = new rise::NominalAttribute("skill", std::vector<std::string>{"low","medium","high"});
  std::cout << *attr1 << std::endl;
  std::cout << *attr2 << std::endl;
  try
  {
    rise::AttributeValue* value1 = new rise::RealAttributeValue(*attr1, 5.0);
    std::cout << *value1 << std::endl;
    delete value1;
    new rise::RealAttributeValue(*attr2, 5.0); // should throw
  }
  catch (rise::RiseException& ex)
  {
    std::cout << ex.what() << std::endl;
  }

  try
  {
    rise::AttributeValue* value1 = new rise::NominalAttributeValue(*attr2, "low");
    std::cout << *value1 << std::endl;
    delete value1;
    new rise::NominalAttributeValue(*attr1, "low"); // should throw
  }
  catch (rise::RiseException& ex)
  {
    std::cout << ex.what() << std::endl;
  }

  try
  {
    new rise::NominalAttributeValue(*attr2, "asdf"); // should throw
  }
  catch (rise::RiseException& ex)
  {
    std::cout << ex.what() << std::endl;
  }
}



