#include "algorithm.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>

#define INFO(x) if (verbose_) std::cout << "\e[1;32m" << x << "\e[0m" << std::endl;
#define WARN(x) if (verbose_) std::cout << "\e[1;33m" << x << "\e[0m" << std::endl;
#define ERROR(x) if (verbose_) std::cout << "\e[1;31m" << x << "\e[0m" << std::endl;

namespace rise
{

RiseClassifier::RiseClassifier(bool verbose) : verbose_(verbose) {}

void RiseClassifier::train(const Dataframe& df)
{
  rs_.clear();

  rs_.reserve(df.get_number_of_records());

  DistanceCache dcache;

  std::clock_t start = clock();

  for (const Instance& instance : df.get_instances())
  {
    Rule::Ptr rule = std::make_shared<Rule>(instance, df.get_xmeta());
    rule->evaluate_rule(df);
    rs_.insert(rule);
  }

  double acc = accuracy(df, dcache, true);

  INFO("Initial accuracy (Leave One Out): " << acc*100 << "%");

  bool increase_acc = true;
  bool new_rules = false;

  while (increase_acc or new_rules)
  {
    increase_acc = false;
    new_rules = false;
    std::vector<Rule::Ptr> freeze(rs_.begin(), rs_.end());
    for (const Rule::Ptr& rule : freeze)
    {
      const Instance* nearest = find_nearest_instance(df, rule);
      if (nearest)
      {
        auto new_rule = rule->adapt(*nearest);
        new_rule->evaluate_rule(df);
        DistanceCache bk = dcache;
        double delta_acc = delta_accuracy(df, new_rule, dcache);
        if (delta_acc >= 0)
        {
          if (delta_acc > 0)
          {
            acc += delta_acc;
            increase_acc = true;
            INFO("Accuracy increased: delta_acc=" << delta_acc*100 <<
                 "% (total acc=" << acc*100 << "%)");
          }
          if (rs_.count(new_rule) == 0)
          {
            new_rules = true;
            rs_.insert(new_rule);
          }
          rs_.erase(rule);
        }
        else dcache = bk;
      }
    }
    INFO("Current size of RuleSet: " << rs_.size() <<
         " (increase_acc: " << (increase_acc? "true" : "false") <<
         ", new_rules: " << (new_rules? "true" : "false") <<
         ", elapsed: " << ((double)(clock()-start))/CLOCKS_PER_SEC << "s)");
  }
  train_time_ = ((double)(clock()-start))/CLOCKS_PER_SEC;
  INFO("Total elapsed: " << train_time_ << 's');
  acc = accuracy(df, dcache, false);
  INFO("Final LOO (Leave One Out) accuracy (only in training!!): " << acc*100 << '%');
}

double RiseClassifier::test(const Dataframe& df) const
{
  double acc = 0;
  for (const Instance& instance : df.get_instances())
  {
    std::string category = classify(instance);
    if (category == instance.get_class()) acc += 1;
  }
  acc /= df.get_number_of_records();
  return acc;
}

std::string RiseClassifier::to_str() const
{
  std::ostringstream oss;
  oss << rs_.size() << " rules:";
  for (const Rule::Ptr& rule : rs_)
  {
    oss << '\n' << *rule;
  }
  return oss.str();
}

Rule::Ptr RiseClassifier::classify(const Instance& instance, double &min_dist, bool loo) const
{
  std::vector<Rule::Ptr> candidates;
  min_dist = std::numeric_limits<double>::infinity();
  Rule::Ptr winner;
  for (const Rule::Ptr& rule : rs_)
  {
    double dist = rule->distance(instance);
    if (not winner)
    {
      winner = rule;
      min_dist = dist;
      continue;
    }
    if (loo and dist < 1e-9 and rule->get_n_instances_covered() < 2) continue;
    if (dist < min_dist-1e-9)
    {
      min_dist = dist;
      winner = rule;
    }
    else if (std::fabs(dist - min_dist) <= 1e-9 and
             rule->get_f1_score() > winner->get_f1_score())
    {
      winner = rule;
    }
  }
  return winner;
}

std::string RiseClassifier::classify(const Instance& instance, bool loo) const
{
  double min_dist;
  return classify(instance, min_dist, loo)->get_consequent();
}

double RiseClassifier::accuracy(const Dataframe& df, DistanceCache& dcache, bool loo) const
{
  dcache = DistanceCache(df.get_number_of_records());
  int n_correctly_classified = 0;
  for (int idx = 0; idx < df.get_number_of_records(); ++idx)
  {
    const Instance& instance = df.get_instances()[idx];
    dcache[idx].first = classify(instance, dcache[idx].second, loo);
    if (dcache[idx].first->get_consequent() == instance.get_class())
      ++n_correctly_classified;
  }
  return ((double)n_correctly_classified)/df.get_number_of_records();
}

double RiseClassifier::delta_accuracy(const Dataframe& df,
    const Rule::Ptr& new_rule, DistanceCache& dcache) const
{
  int rescued = 0;
  for (int idx = 0; idx < df.get_number_of_records(); ++idx)
  {
    const Instance& instance = df.get_instances()[idx];
    double dist = new_rule->distance(instance);
    bool wins_instance = dist < dcache[idx].second-1e-9 or
      (std::fabs(dist - dcache[idx].second) <= 1e-9 and
       new_rule->get_f1_score() > dcache[idx].first->get_f1_score());
    if (wins_instance)
    {
      bool new_is_correct = new_rule->get_consequent() == instance.get_class();
      bool old_is_correct = dcache[idx].first->get_consequent() == instance.get_class();
      if (new_is_correct and not old_is_correct) ++rescued;
      else if (not new_is_correct and old_is_correct) --rescued;
      dcache[idx].first = new_rule;
      dcache[idx].second = dist;
    }
  }
  return ((double)rescued)/df.get_number_of_records();
}

const Instance* RiseClassifier::find_nearest_instance(const Dataframe& df,
    const Rule::Ptr& rule)
{
  double min_distance = std::numeric_limits<double>::infinity();
  const Instance* nearest = nullptr;
  for (const Instance& instance : df.get_instances())
  {
    double distance = rule->distance(instance);
    if (instance.get_class() == rule->get_consequent() and
        distance > 1e-9)
    {
      nearest = &instance;
      if (distance < min_distance) min_distance = distance;
    }
  }
  return nearest;
}

}

