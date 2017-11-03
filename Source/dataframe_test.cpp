#include "dataframe.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
  srand(42);
  if (argc != 2)
  {
    std::cerr << "Usage: dataframe_test datasetname\n";
    return -1;
  }
  try
  {
    std::string datafile = std::string("../Data/") + argv[1] + '/' + argv[1] + ".data";
    std::string metafile = std::string("../Data/") + argv[1] + '/' + argv[1] + ".meta";
    rise::Dataframe df(datafile, metafile);
    df.shuffle();
    df.init_lu(rise::Dataframe::KL);
    std::cout << df << std::endl;
    //rise::Dataframe train, val;
    //for (int fold_idx = 0; fold_idx < 10; ++fold_idx)
    //{
      //std::cout << "Fold " << fold_idx << ": " << std::endl;
      //df.split(fold_idx, 10, train, val);
      //std::cout << train << std::endl << val << std::endl;
    //}
    //std::map<rise::CategoryPair, double> cp;
    //df.conditional_probs(1, cp);
    //for (const auto& entry : cp)
    //{
      //std::cout << "P(" << entry.first.second << '|' << entry.first.first <<
        //") = " << entry.second << std::endl;
    //}
  }
  catch (rise::RiseException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}

