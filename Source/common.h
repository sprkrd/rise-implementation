#ifndef COMMON_H
#define COMMON_H

#include <exception>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define S(ptr) (ptr ? ptr->to_str() : "?")

namespace rise
{

class Stringifiable;
class RiseException;
class Attribute;
class RealAttribute;
class NominalAttribute;
class AttributeMeta;
class RealAttributeMeta;
class NominalAttributeMeta;
class Instance;
typedef std::pair<std::string, std::string> CategoryPair;

class Stringifiable
{
  public:

    virtual std::string to_str() const =0;

    virtual ~Stringifiable() {}
};

class RiseException : public std::exception
{
  public:

    explicit RiseException(const std::string& msg) noexcept : msg_(msg) {}

    virtual const char* what() const noexcept override { return msg_.c_str(); }

  private:

    std::string msg_;
};

class Attribute : public Stringifiable
{
  public:

    typedef std::shared_ptr<Attribute> Ptr;

    static Ptr create(void) { return Ptr(); }

    virtual bool operator==(const Attribute& other) const = 0;

    virtual ~Attribute() {}
};

class RealAttribute : public Attribute
{
  public:

    typedef std::shared_ptr<RealAttribute> Ptr;

    static Ptr create(double value);

    static Ptr create(const std::string& value);

    RealAttribute(double number) : number_(number) {}

    double get_number() const { return number_; }

    virtual bool operator==(const Attribute& other) const override;

    virtual std::string to_str() const { return std::to_string(number_); }

  private:

    double number_;
};

class NominalAttribute: public Attribute
{
  public:

    typedef std::shared_ptr<NominalAttribute> Ptr;

    static Ptr create(const std::string& value);

    NominalAttribute(const std::string& category) : category_(category) {}

    const std::string& get_category() const { return category_; }

    virtual bool operator==(const Attribute& other) const override;

    virtual std::string to_str() const { return category_; }

  private:

    std::string category_;
};

class AttributeMeta : public Stringifiable
{
  public:

    typedef std::shared_ptr<AttributeMeta> Ptr;

    AttributeMeta(const std::string& name) : name_(name) {}

    const std::string& get_name() const { return name_; }

    virtual ~AttributeMeta() {}

  private:

    std::string name_;
};

class RealAttributeMeta : public AttributeMeta
{
  public:

    typedef std::shared_ptr<RealAttributeMeta> Ptr;

    RealAttributeMeta(const std::string& name) : AttributeMeta(name) {}

    double get_lower_bound() const { return lower_bound_; }

    double get_upper_bound() const { return upper_bound_; }

    double get_range() const { return upper_bound_ - lower_bound_; }

    void set_lower_bound(double lo) { lower_bound_ = lo; }

    void set_upper_bound(double up) { upper_bound_ = up; }

    virtual std::string to_str() const override;

  private:

    double lower_bound_, upper_bound_;
};

class NominalAttributeMeta : public AttributeMeta
{
  public:

    typedef std::shared_ptr<NominalAttributeMeta> Ptr;

    NominalAttributeMeta(const std::string& name) : AttributeMeta(name) {}

    const std::set<std::string>& get_domain() const { return domain_; }

    void set_domain(const std::set<std::string>& domain) { domain_ = domain; }

    void set_lookup(const std::map<CategoryPair, double>& lu) { distance_lu_ = lu; }

    double lookup_distance(const std::string& c1, const std::string& c2) const;

    virtual std::string to_str() const override;

  private:

    std::set<std::string> domain_;
    std::map<CategoryPair, double> distance_lu_;
};

class Instance : public Stringifiable
{
  public:

    Instance(int index, const std::vector<Attribute::Ptr>& x, const Attribute::Ptr& y)
      : index_(index), x_(x), y_(y) {}

    int get_index() const { return index_; }

    const std::vector<Attribute::Ptr>& get_x() const { return x_; }

    const Attribute::Ptr& get_y() const { return y_; }

    const std::string& get_class() const;

    virtual std::string to_str() const override;

  private:

    int index_;
    std::vector<Attribute::Ptr> x_;
    Attribute::Ptr y_;
};

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

std::ostream& operator<<(std::ostream& os, const Stringifiable& strable);

} /* end namespace rise */

#endif

