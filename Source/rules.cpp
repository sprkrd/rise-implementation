#include "rules.h"
#include <cmath>
#include <functional>
#include <limits>
#include <typeinfo>

namespace rise
{

namespace // tools for internal usage
{
} /* end anonymous namespace */

// RealCondition's methods

double RealCondition::EPSILON = 1e-7;

bool RealCondition::covers(const Attribute::Ptr& attr) const
{
  if (auto rattr = std::dynamic_pointer_cast<RealAttribute>(attr))
  {
    double number = rattr->get_number();
    return number >= lower_bound_ and number <= upper_bound_;
  }
  return false;
}

double RealCondition::distance(const Attribute::Ptr& attr) const
{
  auto meta = std::dynamic_pointer_cast<RealAttributeMeta>(get_meta());
  if (auto rattr = std::dynamic_pointer_cast<RealAttribute>(attr))
  {
    double number = rattr->get_number();
    if (number < lower_bound_) return (lower_bound_ - number)/meta->get_range();
    else if (number > upper_bound_) return (number - upper_bound_)/meta->get_range();
    else return 0;
  }
  return -1;
}

Condition::Ptr RealCondition::adapt(const Attribute::Ptr& attr) const
{
  double lo = lower_bound_;
  double up = upper_bound_;
  if (auto rattr = std::dynamic_pointer_cast<RealAttribute>(attr))
  {
    double number = rattr->get_number();
    if (number < lower_bound_) lo = number;
    else if (number > upper_bound_) up = number;
  }
  return std::make_shared<RealCondition>(get_meta(), lo, up);
}

bool RealCondition::operator==(const Condition& other) const
{
  try
  {
    auto c = dynamic_cast<const RealCondition&>(other);
    return std::fabs(lower_bound_ - c.lower_bound_) < EPSILON and 
           std::fabs(upper_bound_ - c.upper_bound_) < EPSILON;
  }
  catch (std::bad_cast&)
  {
    return false;
  }
}

std::string RealCondition::to_str() const
{
  std::ostringstream oss;
  oss << lower_bound_ << "<=" << get_meta()->get_name() << "<=" << upper_bound_;
  return oss.str();
}

// NominalCondition's methods

bool NominalCondition::covers(const Attribute::Ptr& attr) const
{
  if (auto nattr = std::dynamic_pointer_cast<NominalAttribute>(attr))
  {
    return nattr->get_category() == category_;
  }
  return false;
}

double NominalCondition::distance(const Attribute::Ptr& attr) const
{
  auto meta = std::dynamic_pointer_cast<NominalAttributeMeta>(get_meta());
  if (auto nattr = std::dynamic_pointer_cast<NominalAttribute>(attr))
  {
    return meta->lookup_distance(category_, nattr->get_category());
  }
  return -1;
}

Condition::Ptr NominalCondition::adapt(const Attribute::Ptr& attr) const
{
  if (auto nattr = std::dynamic_pointer_cast<NominalAttribute>(attr))
  {
    if (nattr->get_category() != category_) return Condition::Ptr();
  }
  return std::make_shared<NominalCondition>(get_meta(), category_);
}

bool NominalCondition::operator==(const Condition& other) const
{
  try
  {
    auto c = dynamic_cast<const NominalCondition&>(other);
    return category_ == c.category_;
  }
  catch (std::bad_cast&)
  {
    return false;
  }
}

std::size_t NominalCondition::hash() const
{
  std::hash<std::string> h;
  return h(category_);
}

std::string NominalCondition::to_str() const
{
  std::ostringstream oss;
  oss << get_meta()->get_name() << "=" << category_;
  return oss.str();
}

// Rule's methods

Rule::Rule(const Instance& instance, const std::vector<AttributeMeta::Ptr>& meta)
{
  const std::vector<Attribute::Ptr>& x = instance.get_x();
  if (x.size() != meta.size()) throw RiseException("Different size of meta vector and x");
  consequent_ = instance.get_y();
  antecedent_.resize(instance.get_x().size());
  for (int idx = 0; idx < x.size(); ++idx)
  {
    if (auto rattr = std::dynamic_pointer_cast<RealAttribute>(x[idx]))
    {
      antecedent_[idx] = std::make_shared<RealCondition>(
          meta[idx], rattr->get_number(), rattr->get_number());
    }
    else if (auto nattr = std::dynamic_pointer_cast<NominalAttribute>(x[idx]))
    {
      antecedent_[idx] = std::make_shared<NominalCondition>(
          meta[idx], nattr->get_category());
    }
  }
}

const std::string& Rule::get_consequent() const
{
  return std::dynamic_pointer_cast<NominalAttribute>(consequent_)->get_category();
}

bool Rule::covers(const Instance& instance) const
{
  const std::vector<Attribute::Ptr>& x = instance.get_x();
  for (int idx = 0; idx < x.size(); ++idx)
  {
    if (antecedent_[idx] and not antecedent_[idx]->covers(x[idx])) return false;
  }
  return true;
}

double Rule::distance(const Instance& instance) const
{
  const std::vector<Attribute::Ptr>& x = instance.get_x();
  double dist_total = 0.0;
  int count = 0;
  for (int idx = 0; idx < x.size(); ++idx)
  {

    if (not antecedent_[idx]) ++count;
    else
    {
      double d = antecedent_[idx]->distance(x[idx]);
      if (d >= 0)
      {
        dist_total += d;
        ++count;
      }
    }
  }
  dist_total /= count;
  return dist_total;
}

Rule::Ptr Rule::adapt(const Instance& instance) const
{
  const std::vector<Attribute::Ptr>& x = instance.get_x();
  auto rule = std::make_shared<Rule>(*this);
  for (int idx = 0; idx < x.size(); ++idx)
  {
    if (rule->antecedent_[idx])
    {
      rule->antecedent_[idx] = rule->antecedent_[idx]->adapt(x[idx]);
    }
  }
  return rule;
}

bool Rule::operator==(const Rule& other) const
{

  for (int idx = 0; idx < antecedent_.size(); ++idx)
  {
    if (antecedent_[idx] and other.antecedent_[idx] and
        *antecedent_[idx] != *other.antecedent_[idx]) return false;
    else if ((bool)antecedent_[idx] xor (bool)other.antecedent_[idx]) return false;
  }
  return *consequent_ == *other.consequent_;;
}

void Rule::evaluate_rule(const Dataframe& df)
{
  int instances_same_class = 0;
  int correctly_classified = 0;
  n_instances_covered_ = 0;
  const std::string& rule_class = get_consequent();
  for (const Instance& instance : df.get_instances())
  {
    auto instance_class = std::dynamic_pointer_cast<NominalAttribute>(instance.get_y());
    if (covers(instance))
    {
      ++n_instances_covered_;
      if (instance_class->get_category() == rule_class)
      {
        ++correctly_classified;
      }
    }
    if (instance_class->get_category() == rule_class)
    {
      ++instances_same_class;
    }
    coverage_ = ((double)correctly_classified) / instances_same_class;
    precision_ = ((double)correctly_classified) / n_instances_covered_;
  }
}

std::size_t Rule::hash() const
{
  std::size_t h = 0;
  std::hash<std::string> hs;
  for (const auto& cond : antecedent_)
  {
    if (cond) h ^= cond->hash() + 0x9e3779b9 + (h<<6) + (h>>2);
  }
  h ^= hs(std::dynamic_pointer_cast<NominalAttribute>(consequent_)->get_category())
    + 0x9e3779b9 + (h<<6) + (h>>2);
  return h;
}

std::string Rule::to_str() const
{
  std::ostringstream oss;
  bool first = true;
  for (const auto& condition : antecedent_)
  {
    if (condition)
    {
      if (not first) oss << ", ";
      oss << *condition;
      first = false;
    }
  }
  oss << " -> " << *consequent_ << " (coverage: " << coverage_*100.0
      << "%, precision: " << precision_*100.0 << "%, f1: " << get_f1_score() << ')';
  return oss.str();
}

} /* end namespace rise */


