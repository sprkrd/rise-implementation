#include "dataframe.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
  srand(42);
  if (argc != 3)
  {
    std::cerr << "Usage: dataframe_test filename.data filename.meta\n";
    return -1;
  }
  try
  {
    rise::Dataframe df(argv[1], argv[2]);
    df.shuffle();
    std::cout << df << std::endl;
    std::set<int> filtered_instances;
    rise::Dataframe train, val;
    for (int fold_idx = 0; fold_idx < 10; ++fold_idx)
    {
      std::cout << "Fold " << fold_idx << ": " << std::endl;
      df.fold(fold_idx, 10, train, val);
      std::cout << train << std::endl << val << std::endl;
    }
  }
  catch (rise::RiseException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}

