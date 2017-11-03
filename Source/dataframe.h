#ifndef DATAFRAME_H
#define DATAFRAME_H

#include "common.h"
#include "csv_reader.h"

#include <set>

namespace rise
{

class Dataframe;

class Dataframe : public Stringifiable
{
  public:

    enum NDistance { GODEL, SVDM, KL };

    Dataframe();

    Dataframe(const std::string& datafile, const std::string& metafile, char delim=',');

    Dataframe(const Dataframe& other) = delete;

    Dataframe& operator=(const Dataframe& other) = delete;

    void init_lu(NDistance type, double q=1.0);

    const std::vector<AttributeMeta::Ptr>& get_xmeta() const { return xmeta_; }

    const AttributeMeta::Ptr& get_ymeta() const { return ymeta_; }

    const std::vector<Instance>& get_instances() const { return database_; }

    int get_number_of_records() const { return database_.size(); }

    int get_number_of_x_attributes() const { return xmeta_.size(); }

    int get_number_of_missing_values() const;

    void shuffle();

    void conditional_probs(int column, std::map<CategoryPair, double>& results) const;

    void split(int fold_idx, int k, Dataframe& train, Dataframe& val) const;

    virtual std::string to_str() const override;

  private:

    int read_metadata(const std::string& metafile);

    void fill_database(const std::vector<CsvRow>& raw_data);

    void fill_domains();

    void filter(int column, const std::string& category, std::set<int>& instances) const;

    void init_godel();

    void init_svdm(double q);

    void init_kl();
    
    std::vector<AttributeMeta::Ptr> xmeta_;
    AttributeMeta::Ptr ymeta_;
    std::vector<Instance> database_;

};

} /* end namespace rise */

#endif


