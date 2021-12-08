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
     MetaEngine.h

   Authors:
     Vincent Noel <contact@vincent-noel.fr>
 
   Date:
     March 2019
*/

#ifndef _METAENGINE_H_
#define _METAENGINE_H_


#ifdef MPI_COMPAT
#include <mpi.h>
#endif

#include <string>
#include <map>
#include <vector>
#include <assert.h>
#include <sys/time.h>

#include "BooleanNetwork.h"
#include "Cumulator.h"
#include "RandomGenerator.h"
#include "RunConfig.h"
#include "FixedPointDisplayer.h"

struct EnsembleArgWrapper;

class MetaEngine {

protected:

  Network* network;
  RunConfig* runconfig;

  double time_tick;
  double max_time;
  unsigned int sample_count;
  unsigned int statdist_trajcount;
  bool discrete_time;
  unsigned int thread_count;
  
  NetworkState reference_state;
  unsigned int refnode_count;
  NetworkState refnode_mask;
  mutable long long elapsed_core_runtime, user_core_runtime, elapsed_statdist_runtime, user_statdist_runtime, elapsed_epilogue_runtime, user_epilogue_runtime;
  STATE_MAP<NetworkState_Impl, unsigned int> fixpoints;
  std::vector<STATE_MAP<NetworkState_Impl, unsigned int>*> fixpoint_map_v;
  
  Cumulator* merged_cumulator;
  std::vector<Cumulator*> cumulator_v;

  STATE_MAP<NetworkState_Impl, unsigned int>* mergeFixpointMaps();
  
#ifdef MPI_COMPAT
  STATE_MAP<NetworkState_Impl, unsigned int>* mergeMPIFixpointMaps(STATE_MAP<NetworkState_Impl, unsigned int>*, bool pack=true);
  STATE_MAP<NetworkState_Impl, unsigned int>* MPI_Unpack_Fixpoints(STATE_MAP<NetworkState_Impl, unsigned int>* fp_map, char* buff, unsigned int buff_size);
  static char* MPI_Pack_Fixpoints(const STATE_MAP<NetworkState_Impl, unsigned int>* fp_map, int dest, unsigned int * buff_size);
  static void MPI_Send_Fixpoints(const STATE_MAP<NetworkState_Impl, unsigned int>* fp_map, int dest);
  static void MPI_Recv_Fixpoints(STATE_MAP<NetworkState_Impl, unsigned int>* fp_map, int origin);

  // Number of processes
  int world_size;
  
  // Rank of the process
  int world_rank;
  
  // Global sample count          
  unsigned int global_sample_count;
  unsigned int global_statdist_trajcount;

  std::vector<long long> elapsed_core_runtimes;
  std::vector<long long> user_core_runtimes;
  std::vector<long long> elapsed_epilogue_runtimes;
  std::vector<long long> user_epilogue_runtimes; 
  
  
#endif

public:

  MetaEngine(Network* network, RunConfig* runconfig) : 
    network(network), runconfig(runconfig),
    time_tick(runconfig->getTimeTick()), 
    max_time(runconfig->getMaxTime()), 
    sample_count(runconfig->getSampleCount()), 
    statdist_trajcount(runconfig->getStatDistTrajCount()),
    discrete_time(runconfig->isDiscreteTime()), 
    thread_count(runconfig->getThreadCount()) {

#ifdef MPI_COMPAT

  MPI_Init(NULL, NULL);
  // Get the number of processes
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get the rank of the process
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  global_sample_count = sample_count;
  global_statdist_trajcount = statdist_trajcount;
  
  if (world_rank == 0) {
    sample_count = (sample_count / world_size) + (sample_count % world_size);
    statdist_trajcount = (statdist_trajcount / world_size) + (statdist_trajcount % world_size);
  } else {
    sample_count = sample_count / world_size;
    statdist_trajcount = statdist_trajcount / world_size;
  }
  
  // Get the name of the processor
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);
  unsigned long int sec= time(NULL);

  std::cout << sec << " " << processor_name << ":" << (world_rank+1) << "/" << world_size 
            << " (" << sample_count << "/" << global_sample_count 
            << ", " << statdist_trajcount << "/" << global_statdist_trajcount << ")"
            << std::endl;
            
#else 
  unsigned long int sec= time(NULL);
  std::cout << sec << " localhost:1/1 (" << sample_count << "/" << sample_count << ", " << statdist_trajcount << "/" << statdist_trajcount << ")" << std::endl;
#endif
      
    }
  ~MetaEngine() {
    
#ifdef MPI_COMPAT
  MPI_Finalize();
#endif
  
  }
  static void init();
  static void loadUserFuncs(const char* module);

  long long getElapsedCoreRunTime() const {return elapsed_core_runtime;}
  long long getUserCoreRunTime() const {return user_core_runtime;}

  long long getElapsedEpilogueRunTime() const {return elapsed_epilogue_runtime;}
  long long getUserEpilogueRunTime() const {return user_epilogue_runtime;}

  long long getElapsedStatDistRunTime() const {return elapsed_statdist_runtime;}
  long long getUserStatDistRunTime() const {return user_statdist_runtime;}

#ifdef MPI_COMPAT
  int getWorldRank() const { return world_rank; }
  int getWorldSize() const { return world_size; }
  std::vector<long long> getUserCoreRuntimes() const { return user_core_runtimes; }
  std::vector<long long> getElapsedCoreRuntimes() const { return elapsed_core_runtimes; }
  std::vector<long long> getUserEpilogueRuntimes() const { return user_epilogue_runtimes; }
  std::vector<long long> getElapsedEpilogueRuntimes() const { return elapsed_epilogue_runtimes; }
  #endif


  bool converges() const {return fixpoints.size() > 0;}
  const STATE_MAP<NetworkState_Impl, unsigned int>& getFixpoints() const {return fixpoints;}

  const std::map<double, STATE_MAP<NetworkState_Impl, double> > getStateDists() const;
  const STATE_MAP<NetworkState_Impl, double> getNthStateDist(int nn) const;
  const STATE_MAP<NetworkState_Impl, double> getAsymptoticStateDist() const;

  Cumulator* getMergedCumulator() {
    return merged_cumulator; 
  }

  const std::map<double, std::map<Node *, double> > getNodesDists() const;
  const std::map<Node*, double> getNthNodesDist(int nn) const;
  const std::map<Node*, double> getAsymptoticNodesDist() const;

  const std::map<double, double> getNodeDists(Node * node) const;
  double getNthNodeDist(Node * node, int nn) const;
  double getAsymptoticNodeDist(Node * node) const;
  
  const std::map<unsigned int, std::pair<NetworkState, double> > getFixPointsDists() const;
  int getMaxTickIndex() const {return merged_cumulator->getMaxTickIndex();} 
  const double getFinalTime() const;

  void display(std::ostream& output_probtraj, std::ostream& output_statdist, std::ostream& output_fp, bool hexfloat = false) const;
  void displayStatDist(std::ostream& output_statdist, bool hexfloat = false) const;
  void displayProbTraj(std::ostream& output_probtraj, bool hexfloat = false) const;
  void displayFixpoints(std::ostream& output_fp, bool hexfloat = false) const;
  void displayFixpoints(FixedPointDisplayer* displayer) const;
  void displayAsymptotic(std::ostream& output_asymptprob, bool hexfloat = false, bool proba = true) const;

  void displayProbTraj(ProbTrajDisplayer* displayer) const;
  void displayStatDist(StatDistDisplayer* output_statdist) const;

  void display(ProbTrajDisplayer* probtraj_displayer, std::ostream& output_statdist, std::ostream& output_fp, bool hexfloat = false) const;
  void display(ProbTrajDisplayer* probtraj_displayer, StatDistDisplayer* statdist_displayer, FixedPointDisplayer* fp_displayer) const;
};

#endif
