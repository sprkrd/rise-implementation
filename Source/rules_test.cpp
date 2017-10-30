#include "rules.h"
#include <iostream>

int main(int argc, char* argv[])
{
  srand(42);
  
  try
  {
    rise::Dataframe df("../Data/crx/crx.data", "../Data/crx/crx.meta");
    df.shuffle();
    std::cout << df << std::endl;
    rise::RealDistance rd;
    rise::Condition* c1 = new rise::RealRangeCondition(*df.get_attributes()[1], 50, 60);
    std::cout << *c1 << ", distance 1st instance: " << rd.distance(*c1, *df.get_instances()[0][1])
              << ", covers?: " << c1->covers(*df.get_instances()[0][1]) << std::endl;
    std::cout << *c1 << ", distance 2nd instance: " << rd.distance(*c1, *df.get_instances()[1][1])
              << ", covers?: " << c1->covers(*df.get_instances()[1][1]) << std::endl;

    rise::NominalDistance nd(rise::NominalDistance::KL, df, 5, 1.0);
    rise::Condition* c2 = new rise::NominalCondition(*df.get_attributes()[5], 0);
    std::cout << *c2 << ", distance 1st instance: " << nd.distance(*c2, *df.get_instances()[0][5])
              << ", covers?: " << c2->covers(*df.get_instances()[0][5]) << std::endl;
    std::cout << *c2 << ", distance 2nd instance: " << nd.distance(*c2, *df.get_instances()[1][5])
              << ", covers?: " << c2->covers(*df.get_instances()[1][5]) << std::endl;

    rise::Rule rule(df.get_instances()[0], df.get_target_column());
    rise::Metric metric(df.get_attributes().size());
    for (int idx = 0; idx < (int)df.get_attributes().size(); ++idx)
    {
      if (dynamic_cast<const rise::RealAttribute*>(df.get_attributes()[idx]))
      {
        metric[idx] = new rise::RealDistance();
      }
      else
      {
        metric[idx] = new rise::NominalDistance(rise::NominalDistance::SVDM, df, idx);
      }
    }
    std::cout << rule << std::endl;
    std::cout << "covers 1st instance?: " << rule.covers(df.get_instances()[0]) << std::endl;
    std::cout << "distance 1st instance?: " << rule.distance(df.get_instances()[0], metric) << std::endl;
    std::cout << "covers 2nd instance?: " << rule.covers(df.get_instances()[1]) << std::endl;
    std::cout << "distance 2nd instance?: " << rule.distance(df.get_instances()[1], metric) << std::endl;
  }
  catch (rise::RiseException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}

