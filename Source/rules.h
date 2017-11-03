#ifndef RULES_H
#define RULES_H

#include "common.h"
#include "dataframe.h"

#include <unordered_set>

namespace rise
{

class Condition;
class RealCondition;
class NominalCondition;
class Rule;

class Condition : public Stringifiable
{
  public:

    typedef std::shared_ptr<Condition> Ptr;

    Condition(const AttributeMeta::Ptr& meta) : meta_(meta) {};

    const AttributeMeta::Ptr& get_meta() const { return meta_; }

    virtual bool covers(const Attribute::Ptr& attr) const = 0;

    virtual double distance(const Attribute::Ptr& attr) const = 0;

    virtual Ptr adapt(const Attribute::Ptr& attr) const = 0;

    virtual bool operator==(const Condition& other) const = 0;

    virtual bool operator!=(const Condition& other) const
    {
      return not (*this == other);
    }

    virtual std::size_t hash() const = 0;

  private:

    AttributeMeta::Ptr meta_;
};

class RealCondition : public Condition
{
  public:

    typedef std::shared_ptr<RealCondition> Ptr;

    RealCondition(const AttributeMeta::Ptr& meta, double lo, double up)
      : Condition(meta), lower_bound_(lo), upper_bound_(up) {}

    virtual bool covers(const Attribute::Ptr& attr) const override;

    virtual double distance(const Attribute::Ptr& attr) const override;

    virtual Condition::Ptr adapt(const Attribute::Ptr& attr) const override;

    virtual bool operator==(const Condition& other) const override;

    virtual std::size_t hash() const override { return 0; } 

    virtual std::string to_str() const override;

  private:

    static double EPSILON;

    double lower_bound_, upper_bound_;
};

class NominalCondition : public Condition
{
  public:

    typedef std::shared_ptr<NominalCondition> Ptr;

    NominalCondition(const AttributeMeta::Ptr& meta, const std::string& category)
      : Condition(meta), category_(category) {}

    virtual bool covers(const Attribute::Ptr& attr) const override;

    virtual double distance(const Attribute::Ptr& attr) const override;

    virtual Condition::Ptr adapt(const Attribute::Ptr& attr) const  override;

    virtual bool operator==(const Condition& other) const override;

    virtual std::size_t hash() const override;

    virtual std::string to_str() const override;

  private:

    std::string category_;

};

class Rule : public Stringifiable
{
  public:

    typedef std::shared_ptr<Rule> Ptr;

    Rule(const Instance& instance, const std::vector<AttributeMeta::Ptr>& meta);

    const std::string& get_consequent() const;

    bool covers(const Instance& instance) const;

    double distance(const Instance& instance) const;

    Rule::Ptr adapt(const Instance& instance) const;

    bool operator==(const Rule& other) const;

    void evaluate_rule(const Dataframe& df);

    int get_n_instances_covered() const { return n_instances_covered_; }

    double get_coverage() const { return coverage_; }

    double get_precision() const { return precision_; }

    double get_f1_score() const { return 2.0/(1/coverage_ + 1/precision_); }
    
    //double get_f1_score() const { return precision_; }

    std::size_t hash() const;

    virtual std::string to_str() const override;

  private:

    std::vector<Condition::Ptr> antecedent_;
    Attribute::Ptr consequent_;
    int n_instances_covered_;
    double coverage_, precision_;
};

struct rule_hash
{
  std::size_t operator()(const Rule::Ptr& rule) const
  {
    return rule->hash();
  }
};

struct rule_eq
{
  bool operator()(const Rule::Ptr& r1, const Rule::Ptr& r2) const
  {
    return *r1 == *r2;
  }
};

typedef std::unordered_set<Rule::Ptr, rule_hash, rule_eq> RuleSet;

} /* end namespace rise */

#endif
