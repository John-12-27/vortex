#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <util.h>

#include "instr.h"
#include "core.h"

using namespace vortex;

Warp::Warp(Core *core, Word id)
    : id_(id)
    , core_(core)
    , active_(false)
    , PC_(STARTUP_ADDR)
    , tmask_(0) {
  iRegFile_.resize(core_->arch().num_threads(), std::vector<Word>(core_->arch().num_regs(), 0));
  fRegFile_.resize(core_->arch().num_threads(), std::vector<Word>(core_->arch().num_regs(), 0));
  vRegFile_.resize(core_->arch().num_regs(), std::vector<Byte>(core_->arch().vsize(), 0));
}

void Warp::eval(pipeline_state_t *pipeline_state) {
  assert(tmask_.any());

  DPH(2, "Fetch: coreid=" << core_->id() << ", wid=" << id_ << ", tmask=");
  for (int i = 0, n = core_->arch().num_threads(); i < n; ++i)
    DPN(2, tmask_.test(n-i-1));
  DPN(2, ", PC=0x" << std::hex << PC_ << std::endl);

  /* Fetch and decode. */    

  Word instr_code = core_->icache_read(PC_, sizeof(Word));
  auto instr = core_->decoder().decode(instr_code);
  if (!instr) {
    std::cout << std::hex << "Error: invalid instruction 0x" << instr_code << ", at PC=" << PC_ << std::endl;
    std::abort();
  }  

  DP(2, "Instr 0x" << std::hex << instr_code << ": " << *instr);

  // Update state
  pipeline_state->cid   = core_->id();
  pipeline_state->wid   = id_;
  pipeline_state->PC    = PC_;
  pipeline_state->tmask = tmask_;
  pipeline_state->rdest = instr->getRDest();
  pipeline_state->rdest_type = instr->getRDType();
    
  // Execute
  this->execute(*instr, pipeline_state);

  DP(4, "Register state:");
  for (int i = 0; i < core_->arch().num_regs(); ++i) {
    DPN(4, "  %r" << std::setfill('0') << std::setw(2) << std::dec << i << ':');
    for (int j = 0; j < core_->arch().num_threads(); ++j) {
      DPN(4, ' ' << std::setfill('0') << std::setw(8) << std::hex << iRegFile_.at(j).at(i) << std::setfill(' ') << ' ');
    }
    DPN(4, std::endl);
  }  
}