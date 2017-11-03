#include "common.h"
#include <algorithm>
#include <limits>
#include <typeinfo>

namespace rise
{

// RealAttributeMeta's methods

RealAttribute::Ptr RealAttribute::create(const std::string& value)
{
  if (value == "?") return Ptr(); // missing value
  return std::make_shared<RealAttribute>(std::stod(value));
}

RealAttribute::Ptr RealAttribute::create(double number)
{
  return std::make_shared<RealAttribute>(number);
}

bool RealAttribute::operator==(const Attribute& other) const
{
  try
  {
    auto rattr = dynamic_cast<const RealAttribute&>(other);
    return std::fabs(number_ - rattr.number_) < 1e-5;
  }
  catch (std::bad_cast&)
  {
    return false;
  }
}

// NominalAttribute's methods

NominalAttribute::Ptr NominalAttribute::create(const std::string& value)
{
  if (value == "?") return Ptr();
  return std::make_shared<NominalAttribute>(value);
}

bool NominalAttribute::operator==(const Attribute& other) const
{
  try
  {
    auto nattr = dynamic_cast<const NominalAttribute&>(other);
    return category_ == nattr.category_;
  }
  catch (std::bad_cast&)
  {
    return false;
  }
}

std::string RealAttributeMeta::to_str() const
{
  std::ostringstream oss;
  oss << get_name() << " (real attribute in range [" << lower_bound_
      << ',' << upper_bound_ << "])";
  return oss.str();
}

// NominalAttributeMeta's methods

std::string NominalAttributeMeta::to_str() const
{
  std::ostringstream oss;
  oss << get_name() << " (nominal attribute with domain "
      << container2str(domain_) << ')';
  //for (const auto& entry : distance_lu_)
  //{
    //oss << "\n  D(" << entry.first.first << ',' << entry.first.second << ") = " << entry.second;
  //}
  return oss.str();
}

double NominalAttributeMeta::lookup_distance(const std::string& c1,
    const std::string& c2) const
{
  double distance = std::numeric_limits<double>::infinity();
  auto it = distance_lu_.find(std::make_pair(c1, c2));
  if (it != distance_lu_.end()) distance = it->second;
  return distance;
}

// Instance's methods

const std::string& Instance::get_class() const
{
  return std::dynamic_pointer_cast<NominalAttribute>(y_)->get_category();
}

std::string Instance::to_str() const
{
  std::ostringstream oss;
  oss << index_ << ": x=[";
  bool first = true;
  for (const auto& attr : x_)
  {
    if (not first) oss << ',';
    oss << S(attr);
    first = false;
  }
  oss << "], y=" << S(y_);
  return oss.str();
}

// Free methods' implementation

std::ostream& operator<<(std::ostream& os, const Stringifiable& strable)
{
  return os << strable.to_str();
}

} /* end namespace rise */

