// --------------------------------------------------------------------
// FILE:    main.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// cgen, the cache generator front end. Follows the jnutils main
// idiom: the singletons are defined here and nowhere else.
// --------------------------------------------------------------------
#include "driver.h"
#include "options.h"
#include "msg.h"

Msg *Msg::instance = 0;
std::unique_ptr<Msg> msg(Msg::getInstance());

namespace cgen { Opt *Opt::instance = 0; }
std::shared_ptr<cgen::Opt> opts(cgen::Opt::getInstance());

int main(int ac, char **av)
{
  msg->setWho("cgen");

  if(!opts->setup_options(ac, av)) return opts->exit_code();

  cgen::Driver::Args args;
  args.cmd    = opts->cmd;
  args.config = opts->config;
  args.output = opts->output;
  args.vars   = opts->vars;
  args.tool   = opts->tool;
  args.eoe    = opts->eoe;

  cgen::Driver drv(args);
  return drv.run();
}
