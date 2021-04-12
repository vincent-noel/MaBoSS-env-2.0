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
     PopCumulator.h

   Authors:
     Vincent Noël <vincent.noel@curie.fr>
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
   Date:
     March 2021
*/

#ifndef _POPCUMULATOR_H_
#define _POPCUMULATOR_H_

#define USE_NEXT_OPT

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <assert.h>

#ifdef PYTHON_API
#define NO_IMPORT_ARRAY
#define PY_ARRAY_UNIQUE_SYMBOL MABOSS_ARRAY_API
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>
#endif

static bool POP_COMPUTE_ERRORS = true;

#define HD_BUG

#include "PopProbaDist.h"

class Network;
template <class S> class ProbTrajDisplayer;

class PopCumulator {

  struct TickValue {
    double tm_slice;
    double TH;
    double tm_slice_square;
    TickValue() {tm_slice = 0.; TH = 0.; tm_slice_square = 0.;}
    TickValue(double tm_slice, double TH) : tm_slice(tm_slice), TH(TH), tm_slice_square(0.0) { }
  };

  struct LastTickValue {
    double tm_slice;
    double TH;
    LastTickValue() : tm_slice(0.0), TH(0.0) {}
    LastTickValue(double tm_slice, double TH) : tm_slice(tm_slice), TH(TH) {}
  };

  class PopCumulMap {
    STATE_MAP<PopNetworkState, TickValue> mp;

  public:
    size_t size() const {
      return mp.size();
    }

    void incr(const PopNetworkState& state, double tm_slice, double TH) {
      STATE_MAP<PopNetworkState, TickValue>::iterator iter = mp.find(state);
      if (iter == mp.end()) {
	mp[state] = TickValue(tm_slice, tm_slice * TH);
      } else {
	(*iter).second.tm_slice += tm_slice;
	(*iter).second.TH += tm_slice * TH;
      }
    }

    void cumulTMSliceSquare(const PopNetworkState& state, double tm_slice) {
      STATE_MAP<PopNetworkState, TickValue>::iterator iter = mp.find(state);
      assert(iter != mp.end());
      (*iter).second.tm_slice_square += tm_slice * tm_slice;
    }
    
    void add(const PopNetworkState& state, const TickValue& tick_value) {
      STATE_MAP<PopNetworkState, TickValue>::iterator iter = mp.find(state);
      if (iter == mp.end()) {
	mp[state] = tick_value;
      } else {
	TickValue& to_tick_value = (*iter).second;
	to_tick_value.tm_slice += tick_value.tm_slice;
	if (POP_COMPUTE_ERRORS) {
	  to_tick_value.tm_slice_square += tick_value.tm_slice_square;
	}
	to_tick_value.TH += tick_value.TH;
      }
    }

    class Iterator {
    
      const PopCumulMap& cumul_map;
      STATE_MAP<PopNetworkState, TickValue>::const_iterator iter, end;

    public:
      Iterator(const PopCumulMap& cumul_map) : cumul_map(cumul_map) {
	iter = cumul_map.mp.begin();
	end = cumul_map.mp.end();
      }
	
      void rewind() {
	iter = cumul_map.mp.begin();
      }

      bool hasNext() const {
	return iter != end;
      }

      void next(PopNetworkState& state, TickValue& tick_value) {
	state = (*iter).first;
	tick_value = (*iter).second;
	++iter;
      }
	
#ifdef USE_NEXT_OPT
      const PopNetworkState& next2(TickValue& tick_value) {
	tick_value = (*iter).second;
	return (*iter++).first;
      }
#endif
	
      void next(TickValue& tick_value) {
	tick_value = (*iter).second;
	++iter;
      }
    };

    Iterator iterator() {return Iterator(*this);}
    Iterator iterator() const {return Iterator(*this);}
  };

#ifdef HD_BUG
  class HDPopCumulMap {
    STATE_MAP<PopNetworkState, double> mp;

  public:
    void incr(const PopNetworkState& fullstate, double tm_slice) {
      STATE_MAP<PopNetworkState, double>::iterator iter = mp.find(fullstate);
      if (iter == mp.end()) {
	mp[fullstate] = tm_slice;
      } else {
	(*iter).second += tm_slice;
      }
    }

    void add(const PopNetworkState& fullstate, double tm_slice) {
      STATE_MAP<PopNetworkState, double>::iterator iter = mp.find(fullstate);
      if (iter == mp.end()) {
	mp[fullstate] = tm_slice;
      } else {
	(*iter).second += tm_slice;
      }
    }

    class Iterator {
    
      const HDPopCumulMap& hd_cumul_map;
      STATE_MAP<PopNetworkState, double>::const_iterator iter, end;

    public:
      Iterator(const HDPopCumulMap& hd_cumul_map) : hd_cumul_map(hd_cumul_map) {
	iter = hd_cumul_map.mp.begin();
	end = hd_cumul_map.mp.end();
      }
	
      void rewind() {
	iter = hd_cumul_map.mp.begin();
      }

      bool hasNext() const {
	return iter != end;
      }

      void next(PopNetworkState& state, double& tm_slice) {
	state = (*iter).first;
	tm_slice = (*iter).second;
	++iter;
      }
	
#ifdef USE_NEXT_OPT
      const PopNetworkState& next2(double& tm_slice) {
	tm_slice = (*iter).second;
	return (*iter++).first;
      }
#endif

      void next(double& tm_slice) {
	tm_slice = (*iter).second;
	++iter;
      }
    };

    Iterator iterator() {return Iterator(*this);}
    Iterator iterator() const {return Iterator(*this);}
  };
#endif
  RunConfig* runconfig;
  double time_tick;
  unsigned int sample_count;
  unsigned int sample_num;
  double last_tm;
  int tick_index;
  std::vector<double> H_v;
  std::vector<double> TH_v;
  std::vector<MAP<unsigned int, double> > HD_v;
  std::vector<double> TH_square_v;
  unsigned int maxcols;
  int max_size;
  int max_tick_index;
  NetworkState_Impl output_mask;
  std::vector<PopCumulMap> cumul_map_v;
#ifdef HD_BUG
  std::vector<HDPopCumulMap> hd_cumul_map_v;
#endif
  std::vector<PopProbaDist> proba_dist_v;
  PopProbaDist curtraj_proba_dist;
  STATE_MAP<PopNetworkState, LastTickValue> last_tick_map;
  bool tick_completed;

  PopCumulMap& get_map() {
    assert((size_t)tick_index < cumul_map_v.size());
    return cumul_map_v[tick_index];
  }

  PopCumulMap& get_map(unsigned int nn) {
    assert(nn < cumul_map_v.size());
    return cumul_map_v[nn];
  }

  const PopCumulMap& get_map(unsigned int nn) const {
    assert(nn < cumul_map_v.size());
    return cumul_map_v[nn];
  }

#ifdef HD_BUG
  HDPopCumulMap& get_hd_map() {
    assert((size_t)tick_index < hd_cumul_map_v.size());
    return hd_cumul_map_v[tick_index];
  }

  HDPopCumulMap& get_hd_map(unsigned int nn) {
    assert(nn < hd_cumul_map_v.size());
    return hd_cumul_map_v[nn];
  }

  const HDPopCumulMap& get_hd_map(unsigned int nn) const {
    assert(nn < hd_cumul_map_v.size());
    return hd_cumul_map_v[nn];
  }
#endif

  double cumultime(int at_tick_index = -1) {
    if (at_tick_index < 0) {
      at_tick_index = tick_index;
    }
    return at_tick_index * time_tick;
  }

  bool incr(const PopNetworkState& state, double tm_slice, double TH, const PopNetworkState& fullstate) {
    if (tm_slice == 0.0) {
      return true;
    }
    curtraj_proba_dist.incr(fullstate, tm_slice);

    if (tick_index >= max_size) {
      return false;
    }
    tick_completed = false;
    PopCumulMap& mp = get_map();
    mp.incr(state, tm_slice, TH);
#ifdef HD_BUG
    HDPopCumulMap& hd_mp = get_hd_map();
    hd_mp.incr(fullstate, tm_slice);
#endif

    STATE_MAP<PopNetworkState, LastTickValue>::iterator last_tick_iter = last_tick_map.find(state);
    if (last_tick_iter == last_tick_map.end()) {
      last_tick_map[state] = LastTickValue(tm_slice, tm_slice * TH);
    } else {
      (*last_tick_iter).second.tm_slice += tm_slice;
      (*last_tick_iter).second.TH += tm_slice * TH;
    }

    return true;
  }

  void check() const;

  void add(unsigned int where, const PopCumulMap& add_cumul_map);
#ifdef HD_BUG
  void add(unsigned int where, const HDPopCumulMap& add_hd_cumul_map);
#endif
  
public:

  PopCumulator(RunConfig* runconfig, double time_tick, double max_time, unsigned int sample_count) :
    runconfig(runconfig), time_tick(time_tick), sample_count(sample_count), sample_num(0), last_tm(0.), tick_index(0) {
#ifdef USE_STATIC_BITSET
    output_mask.set();
#elif defined(USE_BOOST_BITSET) || defined(USE_DYNAMIC_BITSET)
    // EV: 2020-10-23
    //output_mask.resize(MAXNODES);
    output_mask.resize(Network::getMaxNodeSize());
    output_mask.set();
#else
    output_mask = ~0ULL;
#endif
    max_size = (int)(max_time/time_tick)+2;
    max_tick_index = max_size;
    cumul_map_v.resize(max_size);
#ifdef HD_BUG
    hd_cumul_map_v.resize(max_size);
#endif
    if (POP_COMPUTE_ERRORS) {
      TH_square_v.resize(max_size);
      for (int nn = 0; nn < max_size; ++nn) {
	TH_square_v[nn] = 0.;
      }
    }
    proba_dist_v.resize(sample_count);
    tick_completed = false;
  }

  void rewind() {
    if (last_tm) {
      computeMaxTickIndex();
    }

    tick_index = 0;
    last_tm = 0.;
    last_tick_map.clear();
    curtraj_proba_dist.clear();
    tick_completed = false;
  }

  void next() {
    if (tick_index < max_size) {
      STATE_MAP<PopNetworkState, LastTickValue>::const_iterator begin = last_tick_map.begin();
      STATE_MAP<PopNetworkState, LastTickValue>::const_iterator end = last_tick_map.end();
      PopCumulMap& mp = get_map();
      double TH = 0.0;
      while (begin != end) {
  // PopNetworkState state = (*begin).first;
	const PopNetworkState& state = (*begin).first;
  double tm_slice = (*begin).second.tm_slice;
  TH += (*begin).second.TH;
  if (POP_COMPUTE_ERRORS) {
    mp.cumulTMSliceSquare(state, tm_slice);
  }
	++begin;
      }
      if (POP_COMPUTE_ERRORS) {
	TH_square_v[tick_index] += TH * TH;
      }
    }
    ++tick_index;
    tick_completed = true;
    last_tick_map.clear();
  }

  void cumul(const PopNetworkState& network_state, double tm, double TH) {
#ifdef USE_DYNAMIC_BITSET
    PopNetworkState fullstate(network_state);
#else
    PopNetworkState fullstate(network_state);
#endif
    PopNetworkState state(fullstate & output_mask);
    
    double time_1 = cumultime(tick_index+1);
    if (tm < time_1) {
      incr(state, tm - last_tm, TH, fullstate);
      last_tm = tm;
      return;
    }

    if (!incr(state, time_1 - last_tm, TH, fullstate)) {
      last_tm = tm;
      return;
    }
    next();
    for (; cumultime(tick_index+1) < tm; next()) {
      if (!incr(state, time_tick, TH, fullstate)) {
	last_tm = tm;
	return;
      }
    }
      
    incr(state, tm - cumultime(), TH, fullstate);
    last_tm = tm;
  }

  void setOutputMask(const NetworkState_Impl& output_mask) {
    this->output_mask = output_mask;
  }

  void displayPopProbTraj(PopNetwork* network, unsigned int refnode_count, ProbTrajDisplayer<PopNetworkState>* displayer) const;

  void computeMaxTickIndex();
  int getMaxTickIndex() const { return max_tick_index;} 

  void epilogue(PopNetwork* network, const PopNetworkState& reference_state);
  void trajectoryEpilogue();

  unsigned int getSampleCount() const {return sample_count;}

  static PopCumulator* mergePopCumulators(RunConfig* runconfig, std::vector<PopCumulator*>& cumulator_v);
  
  
#ifdef PYTHON_API

  PyObject* getNumpyStatesDists(PopNetwork* network) const;
  // PyObject* getNumpyLastStatesDists(Network* network) const;;
  std::vector<PopNetworkState> getStates(PopNetwork* network) const;
  // std::vector<NetworkState_Impl> getLastStates() const;
  // PyObject* getNumpyNodesDists(Network* network) const;
  // PyObject* getNumpyLastNodesDists(Network* network) const;
  // std::vector<Node*> getNodes(Network* network) const;
  
#endif
};

#endif