#include "algorithm.h"
#include <cmath>
#include <iostream>
#include <string>

struct Options
{
  std::string datafile, metafile;
  rise::Dataframe::NDistance dtype;
  double q;
  int folds;
};

bool read_options(int argc, char* argv[], Options& options)
{
  if (argc < 4) return false;
  if (argc > 5) return false;
  options.datafile = std::string("../Data/") + argv[1] + '/' + argv[1] + ".data";
  options.metafile = std::string("../Data/") + argv[1] + '/' + argv[1] + ".meta";
  std::string dtype = std::string(argv[2]);
  if (dtype == "godel") options.dtype = rise::Dataframe::GODEL;
  else if (dtype == "svdm") options.dtype = rise::Dataframe::SVDM;
  else if (dtype == "kl") options.dtype = rise::Dataframe::KL;
  else return false;
  if (dtype == "svdm")
  {
    options.q = std::stod(argv[3]);
    options.folds = std::stoi(argv[4]);
  }
  else
  {
    options.folds = std::stoi(argv[3]);
  }
  std::cout << "Options:\n"
            << "  datafile: " << options.datafile << '\n'
            << "  metafile: " << options.metafile << '\n'
            << "  dtype: " << dtype << '\n'
            << "  q (only relevand in svdm): " << options.q << '\n'
            << "  folds: " << options.folds << std::endl;
  return true;
}

void stats(const std::vector<double>& v, double& mean, double& stdev)
{
  double mean_sq = 0;
  mean = 0;
  for (double n : v)
  {
    mean += n;
    mean_sq += n*n;
  }
  mean /= v.size();
  mean_sq /= v.size();
  stdev = std::sqrt(mean_sq - mean*mean);
}

int main(int argc, char* argv[])
{
  srand(42); // reproducible results
  Options options;
  if (not read_options(argc, argv, options))
  {
    std::cerr << "Usage: " << argv[0] << " datasetname {godel|svdm|kl} [q] #folds\n";
    return -1;
  }
  try
  {
    rise::Dataframe df(options.datafile, options.metafile);
    df.shuffle();
    std::cout << df << std::endl;
    if (options.folds == 1)
    {
      df.init_lu(options.dtype, options.q);
      rise::RiseClassifier classifier(true);
      classifier.train(df);
      std::cout << classifier << std::endl;
    }
    else
    {
      std::vector<double> acc_fold(options.folds);
      std::vector<double> elapsed_fold(options.folds);
      rise::Dataframe train, val;
      rise::RiseClassifier classifier(false);
      for (int fold = 0; fold < options.folds; ++fold)
      {
        df.split(fold, options.folds, train, val);
        train.init_lu(options.dtype, options.q);
        classifier.train(train);
        elapsed_fold[fold] = classifier.get_train_time();
        acc_fold[fold] = classifier.test(val);
        std::cout << "Accuracy in fold " << fold << "(%): " << 100*acc_fold[fold] << std::endl;
        std::cout << "Train time in fold " << fold << "(s): " << elapsed_fold[fold] << std::endl;
      }
      double mean_acc, stdev_acc, mean_elapsed, stdev_elapsed;
      stats(acc_fold, mean_acc, stdev_acc);
      stats(elapsed_fold, mean_elapsed, stdev_elapsed);
      std::cout << "Accuracy(%): " << mean_acc*100.0 << " (+- " << stdev_acc*100.0 << ')' << std::endl;
      std::cout << "Elapsed(s): " << mean_elapsed << " (+- " << stdev_elapsed << ')' << std::endl;
    }
  }
  catch (rise::RiseException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}
