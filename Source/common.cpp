#include "common.h"
#include <algorithm>
#include <typeinfo>

namespace rise
{

// Attribute methods

Attribute::Attribute(const std::string& name) : name_(name) {}

// AttributeValue methods

AttributeValue::AttributeValue(const Attribute& attribute) : attribute_(attribute) {}

// RealAttribute methods

RealAttribute::RealAttribute(const std::string& name, double minimum, double maximum)
  : Attribute(name), minimum_(minimum), maximum_(maximum) {}

std::string RealAttribute::to_str() const
{
  std::ostringstream oss;
  oss << get_name() << " (Real attribute in the range [" << minimum_
      << ", " << maximum_ << "])";
  return oss.str();
}

// RealAttributeValue methods

RealAttributeValue::RealAttributeValue(const Attribute& attribute, double number)
  : AttributeValue(attribute), number_(number)
{
  try
  {
    const RealAttribute& r_attr = dynamic_cast<const RealAttribute&>(attribute);
    if (number < r_attr.get_minimum()) throw RiseException(std::to_string(number) + " is less than minimum");
    if (number > r_attr.get_maximum()) throw RiseException(std::to_string(number) + " is greater than maximum");
  }
  catch (std::bad_cast&)
  {
    throw RiseException(std::string("Attribute ") + attribute.get_name() + " is not numeric");
  }
}

// NominalAttribute methods

NominalAttribute::NominalAttribute(const std::string& name, const std::vector<std::string>& domain)
  : Attribute(name), domain_(domain) {}

int NominalAttribute::get_index(const std::string& category) const
{
  return std::find(domain_.begin(), domain_.end(), category) - domain_.begin();
}

std::string NominalAttribute::to_str() const
{
  std::ostringstream oss;
  oss << get_name() << " (Nominal attribute with domain "
      << container2str(domain_) << ')';
  return oss.str();
}

// NominalAttributeValue methods

NominalAttributeValue::NominalAttributeValue(const Attribute& attribute, const std::string& category)
  : AttributeValue(attribute)
{
  try
  {
    const NominalAttribute& n_attr = dynamic_cast<const NominalAttribute&>(attribute);
    index_ = n_attr.get_index(category);
    if (index_ >= (int)n_attr.get_domain().size())
    {
      throw RiseException(std::string("Unknown category: ") + category);
    }
  }
  catch (std::bad_cast&)
  {
    throw RiseException(std::string("Attribute ") + attribute.get_name() + " is not nominal");
  }
}

const std::string& NominalAttributeValue::get_category() const
{
  const NominalAttribute& n_attr = dynamic_cast<const NominalAttribute&>(get_attribute());
  return n_attr.get_domain()[index_];
}

// MissingAttributeValue methods

MissingAttributeValue::MissingAttributeValue(const Attribute& attribute)
  : AttributeValue(attribute) {}

// Free methods' implementation

std::ostream& operator<<(std::ostream& os, const Stringifiable& strable)
{
  return os << strable.to_str();
}

} /* end namespace rise */

