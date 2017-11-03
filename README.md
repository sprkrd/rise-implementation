# rise-implementation
Implementation of the RISE rule-based classifier (Pedro Domingos, 1996)

The repository is structured as follows:
* ./Data contains the data sets used for evaluation
* ./Source contains the source files of the implementation (headers and .cpp)
* ./Documentation contains the LaTeX files and a compiles PDF of a report that briefly describes RISE and the results obtained with this implementation

## Practical details about the implementation

This section says exactly the same as the one with the same name from the report.

The implementation has been developed in C++. We use the C++11 standard, so it is necessary a compiler that supports it (we have used GCC 5.4). The application is self-contained: there is no need of 3rd party libraries to build everything. In order to compile the library and the binaries it is enough to execute \texttt{make all} from the implementation base folder:

```bash
$ make -j4
```

This will create a \texttt{build} directory with the compiled objects, the shared library and all the executables. The most important binary is the one called \texttt{./build/rise\_classifier} (the other ones are modular tests). It should be always executed from the \texttt{build} folder so it can find the data sets (the path is coded relative to the executable location as \texttt{../Data}). The usage of the binary is as follows:

```bash
./rise_classifier 
Usage: rise_classifier datasetname {godel|svdm|kl} [q] #folds
```

The first argument of the program must be one of the data sets in the \texttt{Data} folder. The second argument is the type of distance that is considered between nominal values. The next argument should be the q parameter of the SVDM distance if \texttt{svdm} is the chosen distance (this is ommited otherwise). The number of folds to train and test with k-fold cross validation. If the number of folds is 1, the whole data set if used for training (there is no testing phase), and the rule base is output to the screen. Moreover, during the execution of the algorithm, there is periodic feedback reporting the evolution of the rule set. Examples of calls:

```bash
./rise_classifier crx godel 10 # run on the Credit Approval data set
                               # with Godel distance and 10 folds.
./rise_classifier lenses svdm 2 1 # run on the C. Lenses data set using
                                  # SVDM(2) distance. Output rules.
./rise_classifier hepatitis kl 1 # run on the Hepatitis data set using
                                 # the KL distance. Output rules
```
