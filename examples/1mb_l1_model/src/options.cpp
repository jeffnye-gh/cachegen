#include "options.h"
#include <iomanip>
using namespace std;
using namespace boost;
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Options::setupOptions(int ac,char**av)
{
  msg.dmsg("+setupOptions");
  po::options_description stdOpts(
    "\nUsage:: cgen [-h|-v|-r]  -o <output> { input files }"
  );

  po::options_description hideOpts;
  po::positional_options_description posOpts;
  buildOpts(stdOpts,hideOpts,posOpts);
  po::options_description combined;

  combined.add(stdOpts); 
  combined.add(hideOpts); 

  //Add the combined options, it does not work when using separate 
  //  .options(stdOpts)
  //  .options(hideOpts)
  //
  try {
    po::store(po::command_line_parser(ac, av)
        .options(combined)
        .positional(posOpts)
        .run(), vm);
  } catch(const std::exception& e) {

    msg.msg("");
    msg.emsg("Command line option parsing failed");
    msg.emsg("What: " + string(e.what()));
    usage(stdOpts);

    exit(1);
  }

  po::notify(vm);

  //Only display the visible options
  if(vm.count("help")) { usage(stdOpts); return true; }
  if(!checkOpts(vm)) return false;
  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void Options::buildOpts(po::options_description &stdOpts,
                        po::options_description &hideOpts,
                        po::positional_options_description &posOpts)
{
  stdOpts.add_options()
    ("help,h","...")
    ("version,v","Report version and exit")
    ("release_notes,r","Report release notes and exit")

    //Output options
    ("dry_run",
      po::bool_switch(&dry_run)->default_value(false),
      "Verify and report settings, but do not generate")

    ("output_file_prefix,p",
      po::value<string>(&output_file_prefix)->default_value("tc_"),
     "Output file prefix, typ. test case name")

    ("output_dir,o",
      po::value<string>(&output_file_prefix)->default_value("output"),
     "Output directory")

    //L1 options
    ("l1_capacity",
      po::value<uint64_t>(&l1_capacity)->default_value(1048576),
     "L1 cache size in decimal bytes")

    ("l1_line_size",
      po::value<uint64_t>(&l1_line_size)->default_value(32),
     "L1 line size in decimal bytes")

    ("l1_associativity",
      po::value<uint64_t>(&l1_associativity)->default_value(4),
     "L1 associativity")

    ("l1_critical_word_first",
      po::bool_switch(&l1_critical_word_first)->default_value(true),
     "On miss, return the target word in parallel with fill.")

    //MM options
    ("mm_address_bits",
      po::value<uint64_t>(&mm_address_bits)->default_value(32),
     "Main memory address bits. Main memory accepts"
     "byte addresses in this range. The setting of mm_fetch_size determines "
     "how many of the LSB's are ignored, mm_capacity determines the first "
     "valid MSB of the address. The default mm_capacity = 8MB, this sets "
     "the address range to 22:0, the default mm_fetch_size is 32 bytes. "
     "Therefore the effective address range is 22:5")

    ("mm_capacity",
      po::value<uint64_t>(&mm_capacity)->default_value(8388608),
     "Main memory instantiated memory")

    ("mm_fetch_size",
      po::value<uint64_t>(&mm_fetch_size)->default_value(32),
     "Main memory fetch size, decimal bytes")

  ;

//  hideOpts.add_options()
//    ("fname",po::value<vector<string> >(&input_files),"input files")
//  ;

  posOpts.add("fname",-1);
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Options::checkOpts(po::variables_map &vm)
{
  if(vm.count("version"))       { version();       return true; }
  if(vm.count("release_notes")) { relNotes();      return true; }

  if(l1_capacity != 1048576) {
    return reportBadOption("l1_capacity",1048576);
  }

  if(l1_line_size != 32) {
    return reportBadOption("l1_line_size",32);
  }

  if(l1_associativity != 4) {
    return reportBadOption("l1_associativity",4);
  }

  if(l1_critical_word_first != true) {
    return reportBadOption("l1_critical_word_first",true);
    return false;
  }

//  if(mm_address_bits != 32) {
//    return reportBadOption("mm_address_bits",32);
//  }

//  if(mm_capacity != 8388608) {
//    return reportBadOption("mm_capacity",8388608);
//  }

  if(mm_fetch_size != l1_line_size) {
    return reportBadOption("mm_fetch_size",l1_line_size);
  }

  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Options::reportBadOption(string opt, uint64_t val)
{
  msg.emsg("Bad option: "+opt+" must equal "+::to_string(val));
  return false;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void Options::version()
{
  msg.imsg("CacheGen, version"+string(VERSION));
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void Options::relNotes()
{
  version();
  msg.imsg("Release notes");
  msg.imsg("  -- no release notes at present");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void Options::usage(po::options_description &visible)
{
  cout<<visible<<endl;
}

