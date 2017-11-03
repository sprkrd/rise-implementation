#include "algorithm.h"
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
    df.init_lu(rise::Dataframe::SVDM);
    std::cout << df << std::endl;
    rise::RiseClassifier classifier(true);
    classifier.train(df);
  }
  catch (rise::RiseException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}

