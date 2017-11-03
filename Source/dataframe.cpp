#include "dataframe.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>

namespace rise
{

namespace /* Utils for internal usage */
{

void push_column(std::vector<CsvRow>& table, int column)
{
  for (CsvRow& row : table)
  {
    for (int idx = column+1; idx < row.size(); ++idx)
    {
      std::swap(row[idx-1], row[idx]);
    }
  }
}

void intersect(const std::set<int>& s1, const std::set<int>& s2, std::set<int>& intersection)
{
  intersection.clear();
  const std::set<int>& s1_ = s1.size() < s2.size()? s1 : s2;
  const std::set<int>& s2_ = &s1_ != &s1? s1 : s2;
  for (int x : s1_)
  {
    if (s2_.count(x)) intersection.insert(x);
  }
}

} /* end anonymous namespace */

Dataframe::Dataframe() {}

Dataframe::Dataframe(const std::string& datafile, const std::string& metafile, char delim)
{
  int target_column = read_metadata(metafile);
  CsvReader reader(datafile, delim);
  std::vector<CsvRow> raw_database;
  CsvRow row;
  while (reader.next_row(row))
  {
    if (row.size() != xmeta_.size()+1) throw RiseException("Inconsistent number of columns");
    raw_database.push_back(row);
  }
  /* move target column to last column */
  push_column(raw_database, target_column);
  /* transform raw data to internal representation */
  fill_database(raw_database);
  /* fill metainformation about attributes (i.e. domains) */
  fill_domains();
}

void Dataframe::init_lu(NDistance type, double q)
{
  switch (type)
  {
    case GODEL: init_godel(); break;
    case SVDM: init_svdm(q); break;
    case KL: init_kl(); break;
  }
}

int Dataframe::get_number_of_missing_values() const
{
  int count = 0;
  for (const Instance& instance : database_)
  {
    for (const Attribute::Ptr& value : instance.get_x())
    {
      if (not value) ++count;
    }
  }
  return count;
}

void Dataframe::shuffle()
{
  for (int idx = 0; idx < (int)database_.size(); ++idx)
  {
    int jdx = rand() % database_.size();
    std::swap(database_[idx], database_[jdx]);
  }
}

void Dataframe::conditional_probs(int column, std::map<CategoryPair, double>& results) const
{
  std::map<std::string, std::set<int>> filt_by_attr;
  std::map<std::string, std::set<int>> filt_by_class;
  auto ameta = std::dynamic_pointer_cast<NominalAttributeMeta>(xmeta_[column]);
  auto cmeta = std::dynamic_pointer_cast<NominalAttributeMeta>(ymeta_);
  for (const std::string& attr_value : ameta->get_domain())
  {
    filter(column, attr_value, filt_by_attr[attr_value]);
  }
  for (const std::string& class_value : cmeta->get_domain())
  {
    int y_column = get_number_of_x_attributes() + 1;
    filter(y_column, class_value, filt_by_class[class_value]);
  }
  for (const std::string& attr_value : ameta->get_domain())
  {
    for (const std::string& class_value : cmeta->get_domain())
    {
      std::set<int> intersection;
      intersect(filt_by_attr[attr_value], filt_by_class[class_value], intersection);
      double num = intersection.size();
      double den = filt_by_attr[attr_value].size();
      results[std::make_pair(attr_value, class_value)] = num/den;
    }
  }
}

void Dataframe::split(int fold_idx, int k, Dataframe& train, Dataframe& val) const
{
  int n_records = get_number_of_records();
  int fold_max_size = n_records / k;
  int val_start = fold_idx*fold_max_size;
  int val_end = fold_idx == k-1? n_records : (fold_idx+1)*fold_max_size;
  train.database_.clear();
  val.database_.clear();
  train.xmeta_ = val.xmeta_ = xmeta_;
  train.ymeta_ = val.ymeta_ = ymeta_;
  train.database_.reserve(n_records - (val_end - val_start));
  val.database_.reserve(val_end - val_start);
  for (int idx = 0; idx < val_start; ++idx)
  {
    train.database_.push_back(database_[idx]);
  }
  for (int idx = val_end; idx < n_records; ++idx)
  {
    train.database_.push_back(database_[idx]);
  }
  for (int idx = val_start; idx < val_end; ++idx)
  {
    val.database_.push_back(database_[idx]);
  }
}

std::string Dataframe::to_str() const
{
  std::ostringstream oss;
  int n_records = get_number_of_records();
  int n_attr = get_number_of_x_attributes();
  int n_missing = get_number_of_missing_values();
  double p_missing = (100.0*n_missing)/(n_records*n_attr);
  oss << "#Records: " << n_records << '\n'
      << "#Attributes: " << n_attr << " + 1 target class\n"
      << "#Missing values: " << n_missing << " (" << p_missing << "%)\n"
      << "Attributes' description:\n";
  for (int idx = 0; idx < n_attr; ++idx)
  {
    oss << "  " << (idx+1) << ". " << *xmeta_[idx] << '\n';
  }
  oss << "  " << (n_attr+1) << ". " << *ymeta_ << " (target) \n";
  if (database_.size() < 25)
  {
    oss << "Database:\n";
    for (const Instance& instance : database_)
    {
      oss << instance << '\n';
    }
  }
  else
  {
    oss << "Database excerpt:\n"
        << database_[0] << '\n'
        << database_[1] << '\n'
        << "...\n"
        << database_.back();
  }
  return oss.str();
}

int Dataframe::read_metadata(const std::string& metafile)
{
  std::ifstream in(metafile);
  if (not in) throw RiseException(std::string("Cannot read metafile: ") + metafile);
  int n_columns;
  if (not (in >> n_columns)) throw RiseException("Cannot read number of columns from metafile");
  if (n_columns < 2) throw RiseException("There must be at least two columns");
  std::vector<AttributeMeta::Ptr> all_meta(n_columns);
  for (int idx = 0; idx < n_columns; ++idx)
  {
    std::string attr_type, attr_name;
    if (not (in >> attr_type >> attr_name))
    {
      throw RiseException("The metafile does not define all the attributes");
    }
    if (attr_type == "Real")
    {
      all_meta[idx] = std::make_shared<RealAttributeMeta>(attr_name);
    }
    else if (attr_type == "Nominal")
    {
      all_meta[idx] = std::make_shared<NominalAttributeMeta>(attr_name);
    }
    else
    {
      throw RiseException(std::string("Unknown attribute type: ") + attr_type +
          " (must be either \"Real\" or \"Nominal\")");
    }
  }
  std::string target_attribute;
  int target_column = -1;
  if (not (in >> target_attribute)) throw RiseException("Cannot read target attribute");
  for (int idx = 0; idx < all_meta.size(); ++idx)
  {
    if (all_meta[idx]->get_name() == target_attribute)
    {
      target_column = idx;
      ymeta_ = all_meta[idx];
    }
    else xmeta_.push_back(all_meta[idx]);
  }
  if (target_column < 0)
  {
    throw RiseException(std::string("Cannot find target's name: ") + target_attribute);
  }
  if (not std::dynamic_pointer_cast<NominalAttributeMeta>(ymeta_))
  {
    throw RiseException(std::string("Specified target attribute (") + target_attribute +
          ") is not nominal");
  }
  return target_column;
}

void Dataframe::fill_database(const std::vector<CsvRow>& raw_data)
{
  database_.reserve(raw_data.size());
  for (const CsvRow& row : raw_data)
  {
    int idx = database_.size();
    std::vector<Attribute::Ptr> x;
    x.reserve(xmeta_.size());
    Attribute::Ptr y = NominalAttribute::create(row.back());
    for (int idx = 0; idx < xmeta_.size(); ++idx)
    {
      if (std::dynamic_pointer_cast<RealAttributeMeta>(xmeta_[idx]))
      {
        x.push_back(RealAttribute::create(row[idx]));
      }
      else
      {
        x.push_back(NominalAttribute::create(row[idx]));
      }
    }
    database_.push_back(Instance(idx, x, y));
  }
}

void Dataframe::fill_domains()
{
  for (int column = 0; column < xmeta_.size()+1; ++column)
  {
    AttributeMeta::Ptr& meta = column < xmeta_.size()? xmeta_[column] : ymeta_;
    if (auto nmeta = std::dynamic_pointer_cast<NominalAttributeMeta>(meta))
    {
      std::set<std::string> domain_set;
      for (const Instance& instance : database_)
      {
        const Attribute::Ptr& attr = column < xmeta_.size()? instance.get_x()[column] :
                                                             instance.get_y();
        auto nattr = std::dynamic_pointer_cast<NominalAttribute>(attr);
        if (nattr) domain_set.insert(nattr->get_category());
      }
      nmeta->set_domain(domain_set);
    }
    else if (auto rmeta = std::dynamic_pointer_cast<RealAttributeMeta>(meta))
    {
      double lo = std::numeric_limits<double>::infinity();
      double up = -lo;
      for (const Instance& instance : database_)
      {
        const Attribute::Ptr& attr = column < xmeta_.size()? instance.get_x()[column] :
                                                             instance.get_y();
        auto rattr = std::dynamic_pointer_cast<RealAttribute>(attr);
        if (rattr)
        {
          double number = rattr->get_number();
          if (number < lo) lo = number;
          if (number > up) up = number;
        }
      }
      rmeta->set_lower_bound(lo);
      rmeta->set_upper_bound(up);
    }
  }
}

void Dataframe::filter(int column, const std::string& category, std::set<int>& instances) const
{
  instances.clear();
  int nx_columns = get_number_of_x_attributes();
  for (int idx = 0; idx < database_.size(); ++idx)
  {
    const Attribute::Ptr& attribute = column < nx_columns?
      database_[idx].get_x()[column] : database_[idx].get_y();
    if (auto nattr = std::dynamic_pointer_cast<NominalAttribute>(attribute))
    {
      if (nattr->get_category() == category) instances.insert(idx);
    }
  }
}

void Dataframe::init_godel()
{
  for (const auto& meta : xmeta_)
  {
    if (auto nmeta = std::dynamic_pointer_cast<NominalAttributeMeta>(meta))
    {
      std::map<CategoryPair, double> lu;
      for (const std::string& v1 : nmeta->get_domain())
      {
        for (const std::string& v2 : nmeta->get_domain())
        {
          lu[std::make_pair(v1, v2)] = v1 == v2? 0 : 1;
        }
      }
      nmeta->set_lookup(lu);
    }
  }
}

void Dataframe::init_svdm(double q)
{
  for (int idx = 0; idx < xmeta_.size(); ++idx)
  {
    if (auto nmeta = std::dynamic_pointer_cast<NominalAttributeMeta>(xmeta_[idx]))
    {
      auto cmeta = std::dynamic_pointer_cast<NominalAttributeMeta>(ymeta_);
      std::map<CategoryPair, double> lu;
      std::map<CategoryPair, double> cp;
      conditional_probs(idx, cp);
      for (const std::string& v1 : nmeta->get_domain())
      {
        for (const std::string& v2 : nmeta->get_domain())
        {
          auto v = std::make_pair(v1, v2);
          for (const std::string& c : cmeta->get_domain())
          {
            auto p1 = std::make_pair(v1,c);
            auto p2 = std::make_pair(v2,c);
            lu[v] += std::pow(std::fabs(cp[p1]-cp[p2]), q);
          }
          lu[v] /= cmeta->get_domain().size();
        }
      }
      nmeta->set_lookup(lu);
    }
  }
}

void Dataframe::init_kl()
{
  for (int idx = 0; idx < xmeta_.size(); ++idx)
  {
    if (auto nmeta = std::dynamic_pointer_cast<NominalAttributeMeta>(xmeta_[idx]))
    {
      auto cmeta = std::dynamic_pointer_cast<NominalAttributeMeta>(ymeta_);
      std::map<CategoryPair, double> lu;
      std::map<CategoryPair, double> cp;
      conditional_probs(idx, cp);
      for (const std::string& v1 : nmeta->get_domain())
      {
        for (const std::string& v2 : nmeta->get_domain())
        {
          auto v = std::make_pair(v1, v2);
          for (const std::string& c : cmeta->get_domain())
          {
            auto p1 = std::make_pair(v1,c);
            auto p2 = std::make_pair(v2,c);
            if (cp[p1] > 0)
            {
              if (cp[p2] > 0) lu[v] -= cp[p1]*std::log2(cp[p2]/cp[p1]);
              else 
              {
                lu[v] = std::numeric_limits<double>::infinity();
                break;
              }
            }
          }
          lu[v] = (1 - std::exp(-lu[v]))/(1 + std::exp(-lu[v]));
        }
      }
      nmeta->set_lookup(lu);
    }
  }
}

} /* end namespace rise */
