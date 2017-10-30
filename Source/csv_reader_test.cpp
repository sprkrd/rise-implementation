#include "csv_reader.h"
#include <iostream>

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: csv_reader_test filename.csv\n";
    return -1;
  }

  int columns = -1;

  try
  {
    rise::CsvReader reader(argv[1]);
    rise::CsvRow row;
    int nrecords = 0;
    while (reader.next_row(row))
    {
      nrecords += 1;
      if (columns == -1) columns = row.size();
      else if (columns != (int)row.size()) std::cerr << "Warning! number of columns not consistent\n";
      //std::cout << rise::container2str(row) << std::endl;
    }
    std::cout << "#Records: " << nrecords << "; #Attributes: " << columns << std::endl;
  }
  catch (rise::RiseException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}
