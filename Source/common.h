#ifndef COMMON_H
#define COMMON_H

#include <exception>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace rise
{

class Stringifiable;
class RiseException;
class Attribute;
class AttributeValue;
class RealAttribute;
class RealAttributeValue;
class NominalAttribute;
class NominalAttributeValue;
class MissingAttributeValue;
typedef std::vector<AttributeValue*> Instance;

class Stringifiable
{
  public:

    virtual std::string to_str() const =0;

    virtual ~Stringifiable() {}
};

class RiseException : public std::exception
{
  public:

    int columns_;
    explicit RiseException(const std::string& msg) noexcept : msg_(msg) {}

    virtual const char* what() const noexcept override { return msg_.c_str(); }

  private:

    std::string msg_;

};

class Attribute : public Stringifiable
{
  public:

    Attribute(const std::string& name);

    const std::string& get_name() const { return name_; }

    virtual ~Attribute() {}

  private:

    std::string name_;

};

class AttributeValue : public Stringifiable
{
  public:

    AttributeValue(const Attribute& attribute);

    const Attribute& get_attribute() const { return attribute_; }

    virtual bool missing() const { return false; };

  private:

    const Attribute& attribute_;
};

class RealAttribute : public Attribute
{
  public:

    RealAttribute(const std::string& name, double minimum=0, double maximum=0);

    double get_minimum() const { return minimum_; }

    void set_minimum(double minimum) { minimum_ = minimum; }

    double get_maximum() const { return maximum_; }

    void set_maximum(double maximum) { maximum_ = maximum; }

    virtual std::string to_str() const override;

  private:

    double minimum_, maximum_;
};

class RealAttributeValue : public AttributeValue
{
  public:

    RealAttributeValue(const Attribute& attribute, double number);

    double get_number() const { return number_; }

    virtual std::string to_str() const override { return std::to_string(number_); }

  private:

    double number_;
};

class NominalAttribute : public Attribute
{
  public:

    NominalAttribute(const std::string& name,
        const std::vector<std::string>& domain=std::vector<std::string>());

    int get_index(const std::string& category) const;

    const std::vector<std::string>& get_domain() const { return domain_; }

    void set_domain(const std::vector<std::string>& domain) { domain_ = domain; }

    virtual std::string to_str() const override;
      
  private:

    std::vector<std::string> domain_;

};

class NominalAttributeValue : public AttributeValue
{
  public:

    NominalAttributeValue(const Attribute& attribute, const std::string& category);

    int get_category_index() const { return index_; }

    const std::string& get_category() const;

    virtual std::string to_str() const override { return get_category(); }

  private:

    int index_;

};

class MissingAttributeValue : public AttributeValue
{
  public:

    MissingAttributeValue(const Attribute& attribute);

    virtual std::string to_str() const override { return "?"; }

    virtual bool missing() const override { return true; }
};

std::ostream& operator<<(std::ostream& os, const Stringifiable& strable);

template <class Container>
std::string container2str(const Container& ctr,
    const std::string& separator=",",
    const std::string& open="[",
    const std::string& close="]")
{
  std::ostringstream oss;
  bool first = true;
  oss << open;
  for (const auto& element : ctr)
  {
    if (not first) oss << separator;
    oss << element;
    first = false;
  }
  oss << close;
  return oss.str();
}

} /* end namespace rise */

#endif

