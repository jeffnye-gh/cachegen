#include "options.h"
#include <iomanip>
using namespace std;
using namespace boost;
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Options::setupOptions(int ac,char**av)
{
  //msg.imsg("+Options::setupOptions");
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
  if(load_json) { if(!loadFromJson()) return false; }
  if(!checkOpts(vm)) return false;
  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void Options::buildOpts(po::options_description &stdOpts,
                        po::options_description &hideOpts,
                        po::positional_options_description &posOpts)
{
  //msg.imsg("+Options::buildOpts");
  stdOpts.add_options()
    ("help,h","...")
    ("version,v","Report version and exit")
    ("release_notes,r","Report release notes and exit")

    //Command options
    ("dry_run",
      po::bool_switch(&dry_run)->default_value(false),
      "Verify and report settings, but do not generate")

    ("generate",
      po::bool_switch(&generate)->default_value(false),
      "Generate the data files for simulation")

    ("run",
      po::bool_switch(&run)->default_value(false),
      "Run the simulator model")

    ("cmd_file",
      po::value<string>(&cmd_file)->default_value("data/command.cmd"),
      "Simulator command file")

    ("interactive",
      po::bool_switch(&interactive)->default_value(false),
      "Start simulator interactive session after option parsing")

    ("load_json",
      po::bool_switch(&load_json)->default_value(false),
      "Load simulator options from json file")

    ("json_file",
      po::value<string>(&json_file)->default_value("data/config.json"),
      "Configuration data file")

    ("preload_bits",
      po::bool_switch(&preload_bits)->default_value(false),
      "Preload bit array")

    ("preload_dary",
      po::bool_switch(&preload_dary)->default_value(false),
      "Preload data array")

    ("preload_tags",
      po::bool_switch(&preload_tags)->default_value(false),
      "Preload tag arrays")

    ("preload_mm",
      po::bool_switch(&preload_mm)->default_value(true),
      "Preload main memory")

    // data generation and access
    ("tc_prefix,p",
      po::value<string>(&tc_prefix)->default_value("tc_"),
     "File prefix, typ. test case name")

    ("data_dir,o",
      po::value<string>(&data_dir)->default_value("data"),
     "Config data directory")

    //L1 options
    ("l1_capacity",
      po::value<uint32_t>(&l1_capacity)->default_value(1048576),
     "L1 cache size in decimal bytes")

    ("l1_line_size",
      po::value<uint32_t>(&l1_line_size)->default_value(32),
     "L1 line size in decimal bytes")

    ("l1_associativity",
      po::value<uint32_t>(&l1_associativity)->default_value(4),
     "L1 associativity")

    ("l1_read_miss_policy",
      po::value<string>(&l1_read_miss_policy)->default_value("ALLOCATE"),
     "X")

    ("l1_write_miss_policy",
      po::value<string>(&l1_write_miss_policy)->default_value("ALLOCATE"),
     "X")

    ("l1_write_hit_policy",
      po::value<string>(&l1_write_hit_policy)->default_value("NO-WRITE-THRU"),
     "X")

    ("l1_replacement_policy",
      po::value<string>(&l1_replacement_policy)->default_value("PLRU"),
     "Read and write miss replacement policy")

    ("l1_coherency_protocol",
      po::value<string>(&l1_coherency_protocol)->default_value("NONE"),
     "X")

    ("l1_victim_buffer_size",
      po::value<uint32_t>(&l1_victim_buffer_size)->default_value(0),
     "X")

    ("l1_store_buffer_size",
      po::value<uint32_t>(&l1_store_buffer_size)->default_value(0),
     "X")

    ("l1_tag_type",
      po::value<string>(&l1_tag_type)->default_value("PHYSICAL"),
     "X")

    ("l1_critical_word_first",
      po::bool_switch(&l1_critical_word_first)->default_value(true),
     "On miss, return the target word in parallel with fill.")

    ("l1_mmu_present",
      po::bool_switch(&l1_mmu_present)->default_value(false),
     "X")

    ("l1_mpu_present",
      po::bool_switch(&l1_mpu_present)->default_value(false),
     "X")

    //MM options
    ("mm_address_bits",
      po::value<uint32_t>(&mm_address_bits)->default_value(32),
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
      po::value<uint32_t>(&mm_fetch_size)->default_value(32),
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
  //msg.imsg("+Options::checkOpts");

  if(vm.count("version"))       { version();  return true; }
  if(vm.count("release_notes")) { relNotes(); return true; }

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

  //FIXME: add checks for limited values
  u.to_upper(l1_replacement_policy);
  u.to_upper(l1_read_miss_policy);
  u.to_upper(l1_write_miss_policy);
  u.to_upper(l1_write_hit_policy);
  u.to_upper(l1_tag_type);

  u.to_upper(l1_replacement_policy);
  if(l1_replacement_policy != "PLRU") {
    return reportBadOption("l1_replacement_policy","PLRU");
    return false;
  }

  if(l1_victim_buffer_size != 0) {
    return reportBadOption("l1_victim_buffer_size",0);
  }

  if(l1_store_buffer_size != 0) {
    return reportBadOption("l1_store_buffer_size",0);
  }

  if(mm_fetch_size != l1_line_size) {
    return reportBadOption("mm_fetch_size",l1_line_size);
  }

  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Options::loadFromJson()
{
  //msg.imsg("+Options::loadFromJson");

  ifstream in(json_file);
  if(!in.is_open()) {
    msg.emsg("Could not open file "+u.tq(json_file));
    return false;
  }

  Json::Reader reader;
  bool parsingSuccessful = reader.parse(in,json);
  if ( !parsingSuccessful ) {
    msg.emsg("Failed to parse MDL");
    cout << reader.getFormattedErrorMessages();
    return false;
  }

  json_format_version = json["json_format_version"].asString();

  cmd_file  = json["cmd_file"].asString();
  data_dir  = json["data_dir"].asString();
  json_file = json["json_file"].asString();

  l1_address_bits  = json["l1_address_bits"].asUInt();
  l1_assocBits     = json["l1_assocBits"].asUInt();
  l1_associativity = json["l1_associativity"].asUInt();
  l1_blockBits     = json["l1_blockBits"].asUInt();
  l1_blocks        = json["l1_blocks"].asUInt();
  l1_line_size     = json["l1_line_size"].asUInt();
  l1_lru_bits      = json["l1_lru_bits"].asUInt();

  l1_capacity            = json["l1_capacity"].asUInt();
  l1_capacity_value      = json["l1_capacity_value"].asUInt();
  l1_capacity_units      = json["l1_capacity_units"].asString();
  l1_coherency_protocol  = json["l1_coherency_protocol"].asString();
  l1_critical_word_first = json["l1_critical_word_first"].asBool();
  l1_mmu_present         = json["l1_mmu_present"].asBool();
  l1_mpu_present         = json["l1_mpu_present"].asBool();
  l1_read_miss_policy = json["l1_read_miss_policy"].asString();
  l1_replacement_policy = json["l1_replacement_policy"].asString();

  l1_offMsb   = json["l1_offMsb"].asUInt();
  l1_offLsb   = json["l1_offLsb"].asUInt();
  l1_offMask  = json["l1_offMask"].asUInt();
  l1_offShift = json["l1_offShift"].asUInt();
  l1_offBits  = json["l1_offBits"].asUInt();

  l1_wrdShift = json["l1_wrdShift"].asUInt();
  l1_wrdMask  = json["l1_wrdMask"].asUInt();

  l1_setMsb = json["l1_setMsb"].asUInt();
  l1_setLsb = json["l1_setLsb"].asUInt();
  l1_setBits = json["l1_setBits"].asUInt();
  l1_setMask = json["l1_setMask"].asUInt();
  l1_setShift = json["l1_setShift"].asUInt();
  l1_sets = json["l1_sets"].asUInt();
  l1_store_buffer_size = json["l1_store_buffer_size"].asUInt();
  l1_tagBits = json["l1_tagBits"].asUInt();
  l1_tagLsb = json["l1_tagLsb"].asUInt();
  l1_tagMask = json["l1_tagMask"].asUInt();
  l1_tagMsb = json["l1_tagMsb"].asUInt();
  l1_tagShift = json["l1_tagShift"].asUInt();
  l1_tag_type = json["l1_tag_type"].asString();
  l1_victim_buffer_size = json["l1_victim_buffer_size"].asUInt();
  l1_write_hit_policy = json["l1_write_hit_policy"].asString();
  l1_write_miss_policy = json["l1_write_miss_policy"].asString();

  mm_capacity       = json["mm_capacity_value"].asUInt64();
  mm_capacity_units = json["mm_capacity_units"].asString();

  mm_entries    = json["mm_entries"].asUInt();
  mm_fetch_size = json["mm_fetch_size"].asUInt();
  mm_lineLsb    = json["mm_lineLsb"].asUInt();

  mm_lineMask  = json["mm_lineMask"].asUInt();
  mm_lineMsb   = json["mm_lineMsb"].asUInt();
  mm_lineShift = json["mm_lineShift"].asUInt();
  tc_prefix    = json["tc_prefix"].asString();

  preload_mm   = json["preload_mm"].asBool();
  preload_tags = json["preload_tags"].asBool();
  preload_bits = json["preload_bits"].asBool();
  preload_dary = json["preload_dary"].asBool();

  mm_file   = json["mm_file"].asString();
  bits_file = json["bits_file"].asString();
  tags_file = json["tags_file"].asString();

  Json::Value r = json["dary_files"];

  if(r.isNull()) {
    msg.wmsg("No dary files detected");
  } else {
    if(!r.isArray()) {
      msg.emsg("Expected a JSON array for dary files");
      return false;
    }
  }

  for(Json::Value::iterator q=r.begin();q!=r.end();++q) {
    string name  = (*q).get("name","").asString();
    if(name.length() > 0) daryFiles.push_back(name);
  }

  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Options::reportBadOption(string opt, uint32_t val)
  { return reportBadOption(opt,::to_string(val)); }
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Options::reportBadOption(string opt, string val)
{
  msg.emsg("Bad option: "+opt+" must equal "+val);
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
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void Options::info()
{
  cout<<"json_format_version "<<json_format_version<<'\n';
  
  cout<<"l1_capacity "<< l1_capacity<<'\n';
  cout<<"l1_capacity_value "<< l1_capacity_value<<'\n';
  cout<<"l1_capacity_units "<< l1_capacity_units<<'\n';
  cout<<"l1_line_size "<< l1_line_size<<'\n';
  cout<<"l1_associativity "<< l1_associativity<<'\n';

  cout<<"l1_read_miss_policy "<< l1_read_miss_policy<<'\n';
  cout<<"l1_write_miss_policy "<< l1_write_miss_policy<<'\n';
  cout<<"l1_write_hit_policy "<< l1_write_hit_policy<<'\n';
  cout<<"l1_replacement_policy "<< l1_replacement_policy<<'\n';
  cout<<"l1_coherency_protocol "<< l1_coherency_protocol<<'\n';
  cout<<"l1_victim_buffer_size "<< l1_victim_buffer_size<<'\n';
  cout<<"l1_store_buffer_size "<< l1_store_buffer_size<<'\n';
  cout<<"l1_tag_type "<< l1_tag_type<<'\n';

  cout<<"l1_critical_word_first "<< l1_critical_word_first<<'\n';
  cout<<"l1_mmu_present "<< l1_mmu_present<<'\n';
  cout<<"l1_mpu_present "<< l1_mpu_present<<'\n';
  cout<<"interactive "<< interactive<<'\n';

  cout<<"mm_address_bits "<< mm_address_bits<<'\n';
  cout<<"mm_capacity "<< mm_capacity<<'\n';
  cout<<"mm_capacity_value "<< mm_capacity_value<<'\n';
  cout<<"mm_capacity_units "<< mm_capacity_units<<'\n';
  cout<<"mm_fetch_size "<< mm_fetch_size<<'\n';
  cout<<"mm_entries "<< mm_entries<<'\n';
  cout<<"mm_lineMsb "<< mm_lineMsb<<'\n';
  cout<<"mm_lineLsb "<< mm_lineLsb<<'\n';
  cout<<"mm_lineShift "<< mm_lineShift<<'\n';
  cout<<"mm_lineMask "<< mm_lineMask<<'\n';

  cout<<"tc_prefix "<< tc_prefix<<'\n';
  cout<<"data_dir "<< data_dir<<'\n';
  cout<<"json_file "<< json_file<<'\n';
  cout<<"cmd_file "<< cmd_file<<'\n';

  cout<<"default_mm_entries "<< default_mm_entries<<'\n';
  cout<<"l1_word_size "<< l1_word_size<<'\n';

  cout<<"l1_address_bits "<< l1_address_bits<<'\n';

  cout<<"l1_blocks "<< l1_blocks<<'\n';
  cout<<"l1_blockBits "<< l1_blockBits<<'\n';

  cout<<"l1_assocBits "<< l1_assocBits<<'\n';

  cout<<"l1_tagBits "<< l1_tagBits<<'\n';
  cout<<"l1_tagMsb "<< l1_tagMsb<<'\n';
  cout<<"l1_tagLsb "<< l1_tagLsb<<'\n';
  cout<<"l1_tagMask "<< l1_tagMask<<'\n';
  cout<<"l1_tagShift "<< l1_tagShift<<'\n';

  cout<<"l1_sets "<< l1_sets<<'\n';
  cout<<"l1_setBits "<< l1_setBits<<'\n';
  cout<<"l1_setMsb "<< l1_setMsb<<'\n';
  cout<<"l1_setLsb "<< l1_setLsb<<'\n';
  cout<<"l1_setMask "<< l1_setMask<<'\n';
  cout<<"l1_setShift "<< l1_setShift<<'\n';

  cout<<"l1_offBits "<< l1_offBits<<'\n';
  cout<<"l1_offMsb "<< l1_offMsb<<'\n';
  cout<<"l1_offLsb "<< l1_offLsb<<'\n';
  cout<<"l1_offMask "<< l1_offMask<<'\n';
  cout<<"l1_offShift "<< l1_offShift<<'\n';

  cout<<"l1_wrdMask "<< l1_wrdMask<<'\n';
  cout<<"l1_wrdShift "<< l1_wrdShift<<'\n';

  cout<<"l1_lru_bits "<< l1_lru_bits<<'\n';
  cout<<"l1_tagKB "<< l1_tagKB<<'\n';

  cout<<"preload_mm   "<< preload_mm<<endl;
  cout<<"preload_tags "<< preload_tags<<endl;
  cout<<"preload_bits "<< preload_bits<<endl;
  cout<<"preload_dary "<< preload_dary<<endl;

  cout<<"bits_file "<< bits_file<<'\n';
  cout<<"tags_file "<< tags_file<<'\n';
  cout<<"mm_file "<< mm_file<<'\n';
  for(auto s : daryFiles) {
    cout<<"daryFile "<<s<<'\n';
  }
}

