// --------------------------------------------------------------------
// FILE:    options.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// Command line for cgen. Follows the jnutils program_options idiom:
// a singleton with build/check/setup/usage/version/query methods.
// No positional arguments, see R-2.
// --------------------------------------------------------------------
#pragma once
#include <boost/program_options.hpp>
#include <memory>
#include <string>

namespace po = boost::program_options;

namespace cgen
{

class Opt
{
public:
  // ------------------------------------------------------------------
  // singleton
  // ------------------------------------------------------------------
  static Opt *getInstance() {
    if(!instance) instance = new Opt();
    return instance;
  }
  // ------------------------------------------------------------------
  // support methods
  // ------------------------------------------------------------------
  void build_options(po::options_description &stdOpts,
                     po::options_description &hiddenOpts);

  bool check_options(po::variables_map &vm,
                     po::options_description &visibleOpts);

  // false means stop, exit_code() carries the status
  bool setup_options(int ac, char **av);

  void usage(po::options_description &opts);
  void version();
  void query_options();

  int exit_code() const { return exit_code_; }
  // ------------------------------------------------------------------
  // the option set
  // ------------------------------------------------------------------
  std::string cmd;          // check or emit
  std::string config;       // path to the system file
  std::string output;       // output directory, emit only
  bool        eoe{false};   // exit on first error
  bool        _query_options{false};
  // ------------------------------------------------------------------
  po::variables_map vm;
  static Opt *instance;

private:
  Opt() {}
  Opt(const Opt &)            = delete;
  Opt(Opt &&)                 = delete;
  Opt &operator=(const Opt &) = delete;

  int exit_code_{0};
};

} // namespace cgen

extern std::shared_ptr<cgen::Opt> opts;
