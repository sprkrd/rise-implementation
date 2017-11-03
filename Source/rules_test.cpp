#include "dataframe.h"
#include "rules.h"
#include <iostream>

int main(int argc, char* argv[])
{
  srand(42);
  
  try
  {
    rise::Dataframe df("../Data/crx/crx.data", "../Data/crx/crx.meta");
    df.shuffle();
    df.init_lu(rise::Dataframe::SVDM);
    std::cout << df << std::endl;
    rise::Rule rule1(df.get_instances()[0], df.get_xmeta());
    rule1.evaluate_rule(df);
    std::cout << "rule1: " << rule1 << std::endl;
    std::cout << "rule1 covers 1st instance: " << rule1.covers(df.get_instances()[0]) << std::endl;
    std::cout << "D(rule,1st instance): " << rule1.distance(df.get_instances()[0]) << std::endl;  
    std::cout << "rule1 covers 2n instance: " << rule1.covers(df.get_instances()[1]) << std::endl;
    std::cout << "D(rule,2n instance): " << rule1.distance(df.get_instances()[1]) << std::endl;
    auto rule2 = rule1.adapt(df.get_instances()[1]);
    rule2->evaluate_rule(df);
    std::cout << "rule2: " << *rule2 << std::endl;
    std::cout << (rule1 == rule1) << std:: endl;
    std::cout << (rule1 == *rule2) << std:: endl;
    std::cout << (*rule2 == *rule2) << std:: endl;
  }
  catch (rise::RiseException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}

