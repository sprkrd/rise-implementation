#include "dataframe.h"

#include "csv_reader.h"

#include <cstdlib>
#include <limits>
#include <set>

namespace rise
{

namespace /* Utils for internal usage */
{

void fill_domain(Attribute* attribute, int column, const std::vector<CsvRow>& raw_data)
{
  if (NominalAttribute* n_attr = dynamic_cast<NominalAttribute*>(attribute))
  {
    std::set<std::string> domain_set;
    for (const CsvRow& row : raw_data)
    {
      if (row[column] != "?") domain_set.insert(row[column]);
    }
    n_attr->set_domain(std::vector<std::string>(domain_set.begin(), domain_set.end()));
  }
  else if (RealAttribute* r_attr = dynamic_cast<RealAttribute*>(attribute))
  {
    double minimum = std::numeric_limits<double>::max();
    double maximum = std::numeric_limits<double>::min();
    for (const CsvRow& row : raw_data)
    {
      if (row[column] != "?")
      {
        try
        {
          double number = std::stod(row[column]);
          if (number < minimum) minimum = number;
          if (number > maximum) maximum = number;
        }
        catch (std::invalid_argument&)
        {
          throw RiseException(std::string("Non real quantity: ") + row[column]);
        }
      }
    }
    r_attr->set_minimum(minimum);
    r_attr->set_maximum(maximum);
  }
}

void fill_column(Attribute* attribute, int column, const std::vector<CsvRow>& raw_data,
    std::vector<Instance>& database)
{
  bool nominal = (bool)dynamic_cast<NominalAttribute*>(attribute);
  for (int idx = 0; idx < (int)database.size(); ++idx)
  {
    if (raw_data[idx][column] == "?")
    {
      database[idx][column] = new MissingAttributeValue(*attribute);
    }
    else if (nominal)
    {
      database[idx][column] = new NominalAttributeValue(*attribute, raw_data[idx][column]);
    }
    else 
    {
      database[idx][column] = new RealAttributeValue(*attribute, std::stod(raw_data[idx][column]));
    }
  }
}

void instance2stream(std::ostream& os, const Instance& instance, bool newline=true)
{
  bool first = true;
  for (AttributeValue* value : instance)
  {
    if (not first) os << ',';
    os << *value;
    first = false;
  }
  if (newline) os << '\n';
}

}

Dataframe::Dataframe() {}

Dataframe::Dataframe(const std::string& datafile, const std::string& metafile, char delim)
  : view_(false)
{
  read_metadata(metafile);
  CsvReader reader(datafile, delim);
  std::vector<CsvRow> raw_database;
  CsvRow row;
  while (reader.next_row(row))
  {
    if (row.size() != attributes_.size()) throw RiseException("Inconsistent number of columns");
    raw_database.push_back(row);
  }

  database_ = std::vector<Instance>(raw_database.size(), Instance(attributes_.size(), nullptr));

  for (int column = 0; column < (int)attributes_.size(); ++column)
  {
    fill_domain(attributes_[column], column, raw_database);
    fill_column(attributes_[column], column, raw_database, database_);
  }

}

Dataframe::Dataframe(const Dataframe& other) : view_(true)
{
  copy_other(other);
}

Dataframe& Dataframe::operator=(const Dataframe& other)
{
  copy_other(other);
  return *this;
}

int Dataframe::get_number_of_missing_values() const
{
  int count = 0;
  for (const Instance& instance : database_)
  {
    for (const AttributeValue* value : instance)
    {
      if (value->missing()) ++count;
    }
  }
  return count;
}

void Dataframe::filter(int column, int category_index, std::set<int>& result) const
{
  result.clear();
  if (not dynamic_cast<NominalAttribute*>(attributes_[column]))
  {
    throw RiseException("Trying to filter non-nominal column");
  }
  for (int idx = 0; idx < (int)database_.size(); ++idx)
  {
    auto n_attr = dynamic_cast<NominalAttributeValue*>(database_[idx][column]);
    if (n_attr and n_attr->get_category_index() == category_index) result.insert(idx);
  }
}

void Dataframe::shuffle()
{
  for (int idx = 0; idx < (int)database_.size(); ++idx)
  {
    int jdx = rand() % database_.size();
    database_[idx].swap(database_[jdx]);
  }
}

void Dataframe::fold(int fold_idx, int k, Dataframe& train, Dataframe& val) const
{
  int n_records = get_number_of_records();
  int fold_max_size = n_records / k;
  int val_start = fold_idx*fold_max_size;
  int val_end = fold_idx == k-1? n_records : (fold_idx+1)*fold_max_size;
  val.copy_other(*this, val_start, val_end);
  train.copy_other(*this, 0, val_start);
  for (int idx = val_end; idx < n_records; ++idx)
  {
    train.database_.push_back(database_[idx]);
  }
}

std::string Dataframe::to_str() const
{
  std::ostringstream oss;
  int n_records = get_number_of_records();
  int n_attr = get_number_of_attributes();
  int n_missing = get_number_of_missing_values();
  double p_missing = (100.0*n_missing)/(database_.size()*(attributes_.size()-1));
  oss << "#Records: " << n_records << '\n'
      << "#Attributes: " << (n_attr - 1) << " + 1 target class\n"
      << "#Missing values: " << n_missing << " (" << p_missing << "%)\n"
      << "View: " << (view_? "true" : "false") << '\n'
      << "Attributes' description:\n";
  for (int idx = 0; idx < (int)attributes_.size(); ++idx)
  {
    oss << "  " << (idx+1) << ". " << *attributes_[idx];
    if (idx == target_idx_) oss << " (target)";
    oss << '\n';
  }
  if (database_.size() < 30)
  {
    oss << "Database:\n";
    for (int idx = 0; idx < (int)database_.size(); ++idx)
    {
      instance2stream(oss, database_[idx], idx != (int)database_.size() - 1);
    }
  }
  else
  {
    oss << "Database excerpt:\n";
    instance2stream(oss, database_[0]);
    instance2stream(oss, database_[1]);
    oss << "...\n";
    instance2stream(oss, database_.back(), false);
  }
  return oss.str();
}

Dataframe::~Dataframe()
{
  if (not view_) destroy_data();
}

void Dataframe::copy_other(const Dataframe& other, int start, int end)
{
  if (not view_) destroy_data();
  if (end < 0) end = other.database_.size();
  attributes_ = other.attributes_;
  database_.resize(end - start);
  for (int idx = start; idx < end; ++idx)
  {
    database_[idx-start] = other.database_[idx];
  }
  target_idx_ = other.target_idx_;
  view_ = true;
}

void Dataframe::destroy_data()
{
  for (Instance& instance : database_)
  {
    for (AttributeValue* attr_val : instance) delete attr_val;
  }
  for (Attribute* attribute : attributes_) delete attribute;
}

void Dataframe::read_metadata(const std::string& metafile)
{
  std::ifstream in(metafile);
  if (not in) throw RiseException(std::string("Cannot read metafile: ") + metafile);
  int n_columns;
  if (not (in >> n_columns)) throw RiseException("Cannot read number of columns from metafile");
  attributes_.resize(n_columns);
  for (int idx = 0; idx < n_columns; ++idx)
  {
    std::string attr_type, attr_name;
    if (not (in >> attr_type >> attr_name))
    {
      throw RiseException("The metafile does not define all the attributes");
    }
    if (attr_type == "Real") attributes_[idx] = new RealAttribute(attr_name);
    else if (attr_type == "Nominal") attributes_[idx] = new NominalAttribute(attr_name);
    else
    {
      throw RiseException(std::string("Unknown attribute type: ") + attr_type +
          " (must be either \"Real\" or \"Nominal\")");
    }
  }
  std::string target_attribute;

  target_idx_ = -1;
  if (not (in >> target_attribute)) throw RiseException("Cannot read target attribute");
  while (++target_idx_ < (int)attributes_.size() and
         attributes_[target_idx_]->get_name() != target_attribute);
  if (target_idx_ == (int)attributes_.size())
  {
    throw RiseException(std::string("Cannot find target's name: ") + target_attribute);
  }
  if (not dynamic_cast<NominalAttribute*>(attributes_[target_idx_]))
  {
    throw RiseException(std::string("Specified target attribute (") + target_attribute +
          ") is not nominal");
  }
}

}
