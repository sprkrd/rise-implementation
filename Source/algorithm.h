#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "dataframe.h"
#include "rules.h"

namespace rise
{

class RiseClassifier : public Stringifiable
{
  public:

    RiseClassifier(bool verbose=false);

    void train(const Dataframe& df);

    double test(const Dataframe& df) const;

    double get_train_time() const { return train_time_; }

    virtual std::string to_str() const override;

  private:

    typedef std::pair<Rule::Ptr, double> RuleAndDistance;
    typedef std::vector<RuleAndDistance> DistanceCache;

    bool verbose_;
    RuleSet rs_;
    double train_time_;

    double acc(const Dataframe& df) const;

    Rule::Ptr classify(const Instance& instance, double &min_dist, bool loo=false) const;

    std::string classify(const Instance& instance, bool loo=false) const;

    double accuracy(const Dataframe& df, DistanceCache& dcache, bool loo=false) const;

    double delta_accuracy(const Dataframe& df, const Rule::Ptr& new_rule,
        DistanceCache& dcache) const;

    static const Instance* find_nearest_instance(const Dataframe& df, const Rule::Ptr& rule);

};

}

#endif

