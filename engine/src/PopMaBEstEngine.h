/*
#############################################################################
#                                                                           #
# BSD 3-Clause License (see https://opensource.org/licenses/BSD-3-Clause)   #
#                                                                           #
# Copyright (c) 2011-2020 Institut Curie, 26 rue d'Ulm, Paris, France       #
# All rights reserved.                                                      #
#                                                                           #
# Redistribution and use in source and binary forms, with or without        #
# modification, are permitted provided that the following conditions are    #
# met:                                                                      #
#                                                                           #
# 1. Redistributions of source code must retain the above copyright notice, #
# this list of conditions and the following disclaimer.                     #
#                                                                           #
# 2. Redistributions in binary form must reproduce the above copyright      #
# notice, this list of conditions and the following disclaimer in the       #
# documentation and/or other materials provided with the distribution.      #
#                                                                           #
# 3. Neither the name of the copyright holder nor the names of its          #
# contributors may be used to endorse or promote products derived from this #
# software without specific prior written permission.                       #
#                                                                           #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       #
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED #
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           #
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER #
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  #
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       #
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        #
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    #
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      #
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        #
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              #
#                                                                           #
#############################################################################

   Module:
     PopMaBEstEngine.h

   Authors:
     Vincent Noël <vincent.noel@curie.fr>

   Date:
     March 2021
*/

#ifndef _POPMABESTENGINE_H_
#define _POPMABESTENGINE_H_

#include <string>
#include <map>
#include <vector>
#include <assert.h>

// #include "MetaEngine.h"
#include "BooleanNetwork.h"
#include "PopCumulator.h"
#include "RandomGenerator.h"
#include "RunConfig.h"
#include "FixedPointDisplayer.h"
#include "PopProbTrajDisplayer.h"

struct ArgWrapper;


class PopMaBEstEngine {



  Network* network;
  RunConfig* runconfig;

  double time_tick;
  double max_time;
  unsigned int sample_count;
  bool discrete_time;
  unsigned int thread_count;
  
  PopNetworkState reference_state;
  unsigned int refnode_count;

  mutable long long elapsed_core_runtime, user_core_runtime, elapsed_statdist_runtime, user_statdist_runtime, elapsed_epilogue_runtime, user_epilogue_runtime;
  STATE_MAP<NetworkState_Impl, unsigned int> fixpoints;
  std::vector<STATE_MAP<NetworkState_Impl, unsigned int>*> fixpoint_map_v;
  
  PopCumulator* merged_cumulator;
  std::vector<PopCumulator*> cumulator_v;

  STATE_MAP<NetworkState_Impl, unsigned int>* mergeFixpointMaps();

public:


public:
  static const std::string VERSION;
  
  PopMaBEstEngine(Network* network, RunConfig* runconfig);

  void run(std::ostream* output_traj);

  ~PopMaBEstEngine();
  
  static void init();
  static void loadUserFuncs(const char* module);

  long long getElapsedCoreRunTime() const {return elapsed_core_runtime;}
  long long getUserCoreRunTime() const {return user_core_runtime;}

  long long getElapsedEpilogueRunTime() const {return elapsed_epilogue_runtime;}
  long long getUserEpilogueRunTime() const {return user_epilogue_runtime;}

  long long getElapsedStatDistRunTime() const {return elapsed_statdist_runtime;}
  long long getUserStatDistRunTime() const {return user_statdist_runtime;}

  bool converges() const {return fixpoints.size() > 0;}
  const STATE_MAP<NetworkState_Impl, unsigned int>& getFixpoints() const {return fixpoints;}

  PopCumulator* getMergedCumulator() {
    return merged_cumulator; 
  }

  void displayFixpoints(FixedPointDisplayer* displayer) const;
  void displayPopProbTraj(PopProbTrajDisplayer* displayer) const;
  void display(PopProbTrajDisplayer* pop_probtraj_displayer, FixedPointDisplayer* fp_displayer) const;
  
  std::vector<ArgWrapper*> arg_wrapper_v;
  PopNetworkState_Impl getTargetNode(RandomGenerator* random_generator, const STATE_MAP<PopNetworkState_Impl, double, PopNetworkState_ImplHash, PopNetworkState_ImplEquality> popNodeTransitionRates, double total_rate) const;
  double computeTH(const MAP<NodeIndex, double>& nodeTransitionRates, double total_rate) const;
  void epilogue();
  static void* threadWrapper(void *arg);
  void runThread(PopCumulator* cumulator, unsigned int start_count_thread, unsigned int sample_count_thread, RandomGeneratorFactory* randgen_factory, int seed, STATE_MAP<NetworkState_Impl, unsigned int>* fixpoint_map, std::ostream* output_traj);
};

#endif
