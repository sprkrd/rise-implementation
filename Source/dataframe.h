#ifndef DATAFRAME_H
#define DATAFRAME_H

#include "common.h"

#include <set>

namespace rise
{

class Dataframe;

class Dataframe : public Stringifiable
{
  public:

    Dataframe();

    Dataframe(const std::string& datafile, const std::string& metafile, char delim=',');

    Dataframe(const Dataframe& other);

    Dataframe& operator=(const Dataframe& other);

    const std::vector<Attribute*> get_attributes() const { return attributes_; }

    const std::vector<Instance> get_instances() const { return database_; }

    int get_target_column() const { return target_idx_; }

    int get_number_of_records() const { return database_.size(); }

    int get_number_of_attributes() const { return attributes_.size(); }

    int get_number_of_missing_values() const;

    void filter(int column, int category_index, std::set<int>& result) const;

    bool is_view() const { return view_; }

    void shuffle();

    void fold(int fold_idx, int k, Dataframe& train, Dataframe& val) const;

    virtual std::string to_str() const override;

    ~Dataframe();

  private:

    void copy_other(const Dataframe& other, int start=0, int end=-1);

    void destroy_data();

    void read_metadata(const std::string& metafile);

    std::vector<Attribute*> attributes_;
    std::vector<Instance> database_;
    int target_idx_;
    bool view_;

};

} /* end namespace rise */

#endif


