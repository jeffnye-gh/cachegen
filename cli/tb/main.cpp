// --------------------------------------------------------------------
// FILE:    main.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// gtest entry point. The singletons live here, the same way they
// live in src/main.cpp for cgen itself.
// --------------------------------------------------------------------
#include "options.h"
#include "msg.h"
#include <gtest/gtest.h>

Msg *Msg::instance = 0;
std::unique_ptr<Msg> msg(Msg::getInstance());

namespace cgen { Opt *Opt::instance = 0; }
std::shared_ptr<cgen::Opt> opts(cgen::Opt::getInstance());

int main(int ac, char **av)
{
  msg->setWho("cgen_tests");
  msg->verbose = 1;              // errors only, the suite is the report
  ::testing::InitGoogleTest(&ac, av);
  return RUN_ALL_TESTS();
}
