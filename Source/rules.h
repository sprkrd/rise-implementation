#ifndef RULES_H
#define RULES_H

#include "dataframe.h"

namespace rise
{

class Condition;
class MatchAllCondition;
class RealRangeCondition;
class NominalCondition;
class Distance;
class RealDistance;
class NominalDistance;
typedef std::vector<Distance*> Metric;
class Rule;

class Condition : public Stringifiable
{
  public:

    Condition(const Attribute& attribute);

    const Attribute& get_attribute() const { return attribute_; }

    virtual bool covers(const AttributeValue& value) const = 0;

    virtual bool matches_all() const { return false; }

    virtual ~Condition() {}

  private:

    const Attribute& attribute_;
};

class MatchAllCondition : public Condition
{
  public:

    MatchAllCondition(const Attribute& attribute);

    virtual bool covers(const AttributeValue&) const override { return true; }

    virtual bool matches_all() const override { return true; }

    virtual std::string to_str() const override { return "true"; };

};

class RealRangeCondition : public Condition
{
  public:

    RealRangeCondition(const Attribute& attribute, double minimum, double maximum);

    virtual bool covers(const AttributeValue& value) const override;

    double get_minimum() const { return minimum_; }

    double get_maximum() const { return maximum_; }

    std::string to_str() const override;

  private:

    double minimum_, maximum_;

};

class NominalCondition : public Condition
{
  public:

    NominalCondition(const Attribute& attribute, int category);

    virtual bool covers(const AttributeValue& value) const override;

    int get_category_index() const { return category_; }

    std::string to_str() const override;

  private:

    int category_;
};

class Distance
{
  public:

    virtual double distance(const Condition& condition, const AttributeValue& value) const = 0;

    virtual ~Distance() {};
};

class RealDistance : public Distance
{
  public:

    RealDistance();

    virtual double distance(const Condition& condition,
        const AttributeValue& value) const override;
};

class NominalDistance : public Distance
{
  public:

    enum Type {SAME, SVDM, KL};

    NominalDistance(Type type, const Dataframe& df, int column, double q=1.0);

    virtual double distance(const Condition& condition,
        const AttributeValue& value) const override;

  private:

    void init_godel_distance(int n);

    void init_svdm(const NominalAttribute* n_attr, const Dataframe& df, int column, double q);

    void init_kl(const NominalAttribute* n_attr, const Dataframe& df, int column);

    std::vector<double> lookup_table_;

};

class Rule : public Stringifiable
{
  public:

    Rule(const Instance& instance, int target_column);

    Rule(const Rule& other, const Instance& instance);
    
    //Rule(const std::vector<Condition*> conditions);

    Rule(const Rule& rule) = delete;

    Rule& operator=(const Rule& rule) = delete;

    double distance(const Instance& instance, const Metric& metric) const;

    bool covers(const Instance& instance) const;

    std::string to_str() const override;

    ~Rule();

  private:

    std::vector<Condition*> conditions_;

};

} /* end namespace rise */

#endif
