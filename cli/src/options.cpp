// --------------------------------------------------------------------
// FILE:    options.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "options.h"
#include "msg.h"
#include <iostream>
#include <vector>

using namespace std;

namespace cgen
{

// --------------------------------------------------------------------
// Build the option set and check it. Returns false when the caller
// should stop, with exit_code() holding the status.
// --------------------------------------------------------------------
bool Opt::setup_options(int ac, char **av)
{
  exit_code_ = 0;

  po::options_description visibleOpts(
    "\ncgen, cache generator front end\n "
    "Usage:: cgen [--help|-h|--version|-v] "
    "--cmd={check,emit} --config=<file> [options]");

  po::options_description allOpts("All options");
  po::options_description stdOpts("Standard options");
  po::options_description hiddenOpts("Hidden options");

  build_options(stdOpts, hiddenOpts);

  visibleOpts.add(stdOpts);
  allOpts.add(stdOpts).add(hiddenOpts);

  try {
    po::store(po::command_line_parser(ac, av).options(allOpts).run(), vm);
  } catch(po::error &e) {
    msg->msg("");
    msg->emsg("command line option parsing failed");
    msg->emsg("What: " + string(e.what()));
    usage(visibleOpts);
    exit_code_ = 1;
    return false;
  }

  po::notify(vm);
  return check_options(vm, visibleOpts);
}

// --------------------------------------------------------------------
// Construct the standard and hidden option descriptions. There are
// no positional options.
// --------------------------------------------------------------------
void Opt::build_options(po::options_description &stdOpts,
                        po::options_description &hiddenOpts)
{
  stdOpts.add_options()
    ("help,h", "report usage and exit")

    ("version,v", "report version and exit")

    ("cmd", po::value<string>(&cmd),
     "required, one of check or emit")

    ("config", po::value<string>(&config),
     "required, path to the system file")

    ("output", po::value<string>(&output)->default_value("./output"),
     "output directory, used by emit")

    ("vars", po::value<string>(&vars),
     "path to the master Vars.mk the emitted build includes, used by "
     "emit. Default $CGEN_ROOT/planning/tools/Vars.mk")

    ("tool", po::value<vector<string>>(&tool)->composing(),
     "VAR=PATH, a tool path written into the emitted Vars.mk, "
     "repeatable, used by emit. A path inside $CGEN_ROOT is written "
     "in the $(CGEN_ROOT)/... form")

    ("eoe", po::bool_switch(&eoe),
     "exit on first error, default off")
  ;

  hiddenOpts.add_options()
    ("query_options",
     po::bool_switch(&_query_options)->default_value(false),
     "report option settings and continue")
  ;
}

// --------------------------------------------------------------------
// Sanity on the options, handle --help and --version.
// --------------------------------------------------------------------
bool Opt::check_options(po::variables_map &vm,
                        po::options_description &visibleOpts)
{
  if(vm.count("help"))    { usage(visibleOpts); return false; }
  if(vm.count("version")) { version();          return false; }

  bool ok = true;

  if(cmd.empty()) {
    msg->emsg("--cmd is required, one of check or emit");
    ok = false;
  } else if(cmd != "check" && cmd != "emit") {
    msg->emsg("--cmd " + msg->tq(cmd) +
              " is not recognized, use check or emit");
    ok = false;
  }

  if(config.empty()) {
    msg->emsg("--config is required, path to the system file");
    ok = false;
  }

  if(!ok) {
    usage(visibleOpts);
    exit_code_ = 1;
    return false;
  }

  if(!tool.empty() && cmd != "emit") {
    msg->wmsg("--tool is used by emit only and is ignored by check");
  }
  if(!vars.empty() && cmd != "emit") {
    msg->wmsg("--vars is used by emit only and is ignored by check");
  }

  if(_query_options) query_options();

  return true;
}

// --------------------------------------------------------------------
void Opt::usage(po::options_description &opts)
{
  cout << opts << endl;
}

// --------------------------------------------------------------------
void Opt::version()
{
  msg->imsg("");
  msg->imsg("cgen, cache generator front end");
  msg->imsg("Version: v" + string(VERSION));
  msg->imsg("");
}

// --------------------------------------------------------------------
void Opt::query_options()
{
  msg->imsg("BEG Opt::query_options()");
  msg->imsg("cmd    : " + cmd);
  msg->imsg("config : " + config);
  msg->imsg("output : " + output);
  msg->imsg("vars   : " + (vars.empty() ? string("<default>") : vars));
  for(const string &t : tool) msg->imsg("tool   : " + t);
  msg->imsg("eoe    : " + string(eoe ? "True" : "False"));
  msg->imsg("END Opt::query_options()");
}

} // namespace cgen
