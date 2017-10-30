#include "rules.h"

#include <cmath>
#include <limits>
#include <typeinfo>
#include <iostream>

namespace rise
{

namespace // tools for internal usage
{

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

void conditional_probability_table(const Dataframe& df, int column, std::vector<double>& table)
{
  int target_column = df.get_target_column();
  auto n_attr = dynamic_cast<NominalAttribute*>(df.get_attributes()[column]);
  auto class_attr = dynamic_cast<NominalAttribute*>(df.get_attributes()[target_column]);
  int m = class_attr->get_domain().size();
  int n = n_attr->get_domain().size();
  table = std::vector<double>(m*n, 0.0);
  std::vector<std::set<int>> filtered_by_class(m);
  std::vector<std::set<int>> filtered_by_attr(n);
  for (int idx = 0; idx < m; ++idx)
  {
    df.filter(target_column, idx, filtered_by_class[idx]);
  }
  for (int idx = 0; idx < n; ++idx)
  {
    df.filter(column, idx, filtered_by_attr[idx]);
  }
  for (int idx = 0; idx < m; ++idx)
  {
    for (int jdx = 0; jdx < n; ++jdx)
    {
      std::set<int> intersection;
      intersect(filtered_by_class[idx], filtered_by_attr[jdx], intersection);
      double num = intersection.size();
      double den = filtered_by_attr[jdx].size();
      table[idx*n + jdx] = num/den;
    }
  }
}

}

// Condition's methods

Condition::Condition(const Attribute& attribute) : attribute_(attribute) {}

// MatchAllCondition's methods

MatchAllCondition::MatchAllCondition(const Attribute& attribute) : Condition(attribute) {}

// RealRangeCondition's methods

RealRangeCondition::RealRangeCondition(const Attribute& attribute, double minimum, double maximum)
  : Condition(attribute), minimum_(minimum), maximum_(maximum)
{
  try
  {
    dynamic_cast<const RealAttribute&>(attribute);
  }
  catch (std::bad_cast&)
  {
    throw RiseException("Given attribute does not have a real domain");
  }
}

bool RealRangeCondition::covers(const AttributeValue& value) const
{
  try
  {
    auto r_value = dynamic_cast<const RealAttributeValue&>(value);
    double number = r_value.get_number();
    return minimum_ <= number and number <= maximum_;
  }
  catch (std::bad_cast&)
  {
    throw RiseException("Given attribute value is not a real number");
  }
}

std::string RealRangeCondition::to_str() const
{
  return std::to_string(minimum_) + "<=" + get_attribute().get_name() +
    "<=" + std::to_string(maximum_);
}

// NominalCondition's methods

NominalCondition::NominalCondition(const Attribute& attribute, int category)
  : Condition(attribute), category_(category)
{
  try
  {
    dynamic_cast<const NominalAttribute&>(attribute);
  }
  catch (std::bad_cast&)
  {
    throw RiseException("Given attribute is not nominal");
  }
}

bool NominalCondition::covers(const AttributeValue& value) const
{
  try
  {
    auto n_value = dynamic_cast<const NominalAttributeValue&>(value);
    return category_ == n_value.get_category_index();
  }
  catch (std::bad_cast&)
  {
    throw RiseException("Given attribute value is not nominal");
  }
}

std::string NominalCondition::to_str() const
{
  auto n_attr = dynamic_cast<const NominalAttribute&>(get_attribute());
  return get_attribute().get_name() + "=" + n_attr.get_domain()[category_];
}

// RealDistance's methods

RealDistance::RealDistance() {}

double RealDistance::distance(const Condition& condition, const AttributeValue& value) const
{
  if (&condition.get_attribute() != &value.get_attribute())
  {
    throw RiseException("Condition and value's attributes are not equal");
  }
  if (value.missing()) return -1.0;
  try
  {
    auto r_cond = dynamic_cast<const RealRangeCondition&>(condition);
    auto num_value = dynamic_cast<const RealAttributeValue&>(value);
    auto attribute = dynamic_cast<const RealAttribute&>(num_value.get_attribute());
    double num = num_value.get_number();
    double lower_bound = r_cond.get_minimum();
    double upper_bound = r_cond.get_maximum();
    double domain_size = attribute.get_maximum() - attribute.get_minimum();
    if (num < lower_bound)
    {
      return (lower_bound - num)/domain_size;
    }
    if (num > upper_bound)
    {
      return (num - upper_bound)/domain_size;
    }
    else return 0.0;
  }
  catch (std::bad_cast&)
  {
    throw RiseException("Trying to compare heterogeneous types");
  }
}

// NominalDistance's methods

NominalDistance::NominalDistance(Type type, const Dataframe& df, int column, double q)
{
  auto n_attr = dynamic_cast<const NominalAttribute*>(df.get_attributes()[column]);
  if (not n_attr) throw RiseException("Non-nominal attribute");
  int n = n_attr->get_domain().size();
  lookup_table_ = std::vector<double>(n*n, 0.0);
  switch (type)
  {
    case SAME:
      init_godel_distance(n);
      break;
    case SVDM:
      init_svdm(n_attr, df, column, q);
      break;
    case KL:
      init_kl(n_attr, df, column);
      break;
  }
  //std::cout << container2str(lookup_table_) << std::endl;
}

double NominalDistance::distance(const Condition& condition, const AttributeValue& value) const
{
  if (&condition.get_attribute() != &value.get_attribute())
  {
    throw RiseException("Condition and value's attributes are not equal");
  }
  if (condition.matches_all() or value.missing()) return -1.0;
  try
  {
    auto n_cond = dynamic_cast<const NominalCondition&>(condition);
    auto nom_value = dynamic_cast<const NominalAttributeValue&>(value);
    auto attribute = dynamic_cast<const NominalAttribute&>(nom_value.get_attribute());
    int n = attribute.get_domain().size();
    if (n*n != (int)lookup_table_.size())
    {
      throw RiseException("Wrong domain size");
    }
    int idx = n_cond.get_category_index();
    int jdx = nom_value.get_category_index();
    return lookup_table_[idx*n + jdx];
  }
  catch (std::bad_cast&)
  {
    throw RiseException("Trying to compare heterogeneous types");
  }
}

void NominalDistance::init_godel_distance(int n)
{
  lookup_table_ = std::vector<double>(n*n, 1.0);
  for (int idx = 0; idx < n; ++idx)
  {
    lookup_table_[idx*(n+1)] = 0.0;
  }
}

void NominalDistance::init_svdm(const NominalAttribute* n_attr, const Dataframe& df,
    int column, double q)
{
  int n = n_attr->get_domain().size();
  std::vector<double> cond_probs;
  conditional_probability_table(df, column, cond_probs);
  int m = cond_probs.size()/n;
  for (int idx = 0; idx < n; ++idx)
  {
    for (int jdx = 0; jdx < n; ++jdx)
    {
      for (int kdx = 0; kdx < m; ++kdx)
      {
        lookup_table_[idx*n + jdx] += std::pow(
            std::fabs(cond_probs[kdx*n+idx] - cond_probs[kdx*n+jdx]), q);
      }
      lookup_table_[idx*n + jdx] /= m;
    }
  }
}

void NominalDistance::init_kl(const NominalAttribute* n_attr, const Dataframe& df,
    int column)
{
  int n = n_attr->get_domain().size();
  std::vector<double> cond_probs;
  conditional_probability_table(df, column, cond_probs);
  int m = cond_probs.size()/n;
  for (int idx = 0; idx < n; ++idx)
  {
    for (int jdx = 0; jdx < n; ++jdx)
    {
      for (int kdx = 0; kdx < m; ++kdx)
      {
        if (cond_probs[kdx*n + idx] > 0.0)
        {
          lookup_table_[idx*n + jdx] -= cond_probs[kdx*n+idx]*
            std::log2(cond_probs[kdx*n+jdx]/cond_probs[kdx*n+idx]);
        }
        else if (cond_probs[kdx*n+jdx] > 0.0)
        {
          lookup_table_[idx*n + jdx] = std::numeric_limits<double>::infinity();
          break;
        }
      }
    }
  }
}

// Rule's methods

Rule::Rule(const Instance& instance, int target_column)
  : conditions_(instance.size(), nullptr)
{
  for (int column = 0; column < (int)instance.size(); ++column)
  {
    if (column != target_column)
    {
      if (instance[column]->missing())
      {
        conditions_[column] = new MatchAllCondition(instance[column]->get_attribute());
      }
      else if (auto r_attr = dynamic_cast<const RealAttributeValue*>(instance[column]))
      {
        conditions_[column] = new RealRangeCondition(r_attr->get_attribute(),
            r_attr->get_number(), r_attr->get_number());
      }
      else if (auto n_attr = dynamic_cast<const NominalAttributeValue*>(instance[column]))
      {
        conditions_[column] = new NominalCondition(n_attr->get_attribute(),
            n_attr->get_category_index());
      }
    }
  }
}

Rule::Rule(const Rule& other, const Instance& instance) : conditions_(other.conditions_.size())
{
  
}

//Rule::Rule(const std::vector<Condition*> conditions) : conditions_(conditions) {}

double Rule::distance(const Instance& instance, const Metric& metric) const
{
  int n = instance.size();
  if (n != (int)metric.size() or n != (int)conditions_.size())
  {
    throw RiseException("Inconsistent instance/metric size");
  }
  int count = 0;
  double distance = 0.0;
  for (int idx = 0; idx < n; ++idx)
  {
    if (metric[idx] != nullptr and conditions_[idx] != nullptr)
    {
      double d = metric[idx]->distance(*conditions_[idx], *instance[idx]);
      if (d >= 0)
      {
        distance += d;
        ++count;
      }
    }
  }
  distance /= count;
  return distance;
}

bool Rule::covers(const Instance& instance) const
{
  int n = instance.size();
  if (n != (int)conditions_.size())
  {
    throw RiseException("Inconsistent instance/metric size");
  }
  for (int idx = 0; idx < n; ++idx)
  {
    if (conditions_[idx] != nullptr)
    {
      if (not conditions_[idx]->covers(*instance[idx])) return false;
    }
  }
  return true;
}

std::string Rule::to_str() const
{
  std::ostringstream oss;
  bool first = true;
  for (Condition* condition : conditions_)
  {
    if (condition != nullptr and not condition->matches_all())
    {
      if (not first) oss << ',';
      oss << '(' <<  *condition << ')';
      first = false;
    }
  }
  return oss.str();
}

Rule::~Rule()
{
  for (Condition* condition : conditions_)
  {
    if (condition != nullptr) delete condition;
  }
}

} /* end namespace rise */


