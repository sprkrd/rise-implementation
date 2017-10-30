#ifndef CSV_REkADER_H
#define CSV_READER_H

#include "common.h"

#include <fstream>
#include <string>
#include <vector>


namespace rise
{

typedef std::vector<std::string> CsvRow;
class CsvReader;

class CsvReader
{
  public:
    CsvReader(const std::string& filename, char delim=',');

    bool next_row(CsvRow& row);

  private:

    std::ifstream in_;
    char delim_;
};


} /* end namespace rise */

#endif

