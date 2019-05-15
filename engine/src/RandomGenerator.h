/* 
   MaBoSS (Markov Boolean Stochastic Simulator)
   Copyright (C) 2011-2018 Institut Curie, 26 rue d'Ulm, Paris, France
   
   MaBoSS is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   MaBoSS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA 
*/

/*
   Module:
     RandomGenerator.h

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
 
   Date:
     January-March 2011
*/

#ifndef _RANDOMGENERATOR_H_
#define _RANDOMGENERATOR_H_

// DO NOT define USE_DUMMY_RANDOM: except for profiling purpose: this flag has been introduced to get an estimation of random number generation time
//#define USE_DUMMY_RANDOM

//#define RANDOM_TRACE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include "maboss-config.h"

class RandomGenerator {

  static size_t generated_number_count;

 protected:
  static void incrGeneratedNumberCount() {generated_number_count++;}

 public:
  virtual std::string getName() const = 0;

  virtual bool isPseudoRandom() const = 0;

  virtual unsigned int generateUInt32() = 0;

  virtual double generate() = 0;

  virtual void setSeed(int seed) { }

  static size_t getGeneratedNumberCount() {return generated_number_count;}

  virtual ~RandomGenerator() {}
};

class PhysicalRandomGenerator : public RandomGenerator {
  int fd;

 public:
  PhysicalRandomGenerator() {
    fd = open("/dev/urandom", O_RDONLY);
    assert(fd >= 0);
  }

  bool isPseudoRandom() const {
    return false;
  }

  std::string getName() const {
    return "physical";
  }

  unsigned int generateUInt32() {
    incrGeneratedNumberCount();
#ifdef USE_DUMMY_RANDOM
    return ~0U/2;
#endif
    unsigned int result;
    int ret = read(fd, &result, sizeof(result));
    assert(ret == sizeof(result));
#ifdef RANDOM_TRACE
    std::cout << result << '\n';
#endif
    return result;
  }

  virtual double generate() {
    double result = ((double)generateUInt32())/~0U; // fixed this 2014-10-17, but I think I added /2 because it did not work
#ifdef RANDOM_TRACE
    std::cout << result << '\n';
#endif
    return result;
  }

  ~PhysicalRandomGenerator() {
    if (fd >= 0) {
      close(fd);
    }
  }
};

class StandardRandomGenerator : public RandomGenerator {

  int seed;
#ifdef HAS_RAND48_T
  drand48_data data;
#endif

 public:
  StandardRandomGenerator(int seed) : seed(seed) {
#ifdef HAS_RAND48_T
    memset(&data, 0, sizeof(data));
    srand48_r(seed, &data);
#elif defined(HAS_RAND48)
    srand48(seed);
#else
    srand(seed);
#endif
  }

  bool isPseudoRandom() const {
    return true;
  }

  std::string getName() const {
    return "standard";
  }

  unsigned int generateUInt32() {
    incrGeneratedNumberCount();
#ifdef USE_DUMMY_RANDOM
    return ~0U/2;
#endif
#ifdef HAS_RAND48_T
    long result;
    lrand48_r(&data, &result);
#ifdef RANDOM_TRACE
    std::cout << (unsigned int)result << '\n';
#endif
    return (unsigned int)result;
#elif defined(HAS_RAND48)
    return lrand48();
#else
    return rand();
#endif
  }

  virtual double generate() {
    incrGeneratedNumberCount();
#ifdef USE_DUMMY_RANDOM
    return 0.5;
#endif
#ifdef HAS_RAND48_T
    double result;
    drand48_r(&data, &result);
#ifdef RANDOM_TRACE
    std::cout << result << '\n';
#endif
    return result;
#elif defined(HAS_RAND48)
    return drand48();
#else
    return (double(rand()) / RAND_MAX);
#endif
  }

  virtual void setSeed(int seed) {
    this->seed = seed;
#ifdef HAS_RAND48_T
    srand48_r(seed, &data);
#elif defined(HAS_RAND48)
    srand48(seed);
#else
    srand(seed);
#endif
  }
};


class GLibCRandomGenerator : public RandomGenerator
{
  #define SIZE_R 344
  #define GLIBCRAND_MAX 2147483647
  int seed;
  int n;
  int r[SIZE_R];

  void glibc_srand(int seed) {
    int i;
    r[0] = seed;
    for (i=1; i<31; i++) {
        r[i] = (16807LL * r[i-1]) % GLIBCRAND_MAX;
        if (r[i] < 0) {
        r[i] += GLIBCRAND_MAX;
        }
    }
    for (i=31; i<34; i++) {
        r[i] = r[i-31];
    }
    for (i=34; i<344; i++) {
        r[i] = r[i-31] + r[i-3];
    }
    n = 0;
  }

  unsigned int glibc_rand() {
    unsigned int x = r[n%344] = r[(n+313%344)] + r[(n+341)%344];
    n = (n+1)%344;
    return x >> 1;
  }

 public:
  GLibCRandomGenerator(int seed) : seed(seed) {
    glibc_srand(seed);
  }

  bool isPseudoRandom() const {
    return true;
  }

  std::string getName() const {
    return "glibc";
  }

  unsigned int generateUInt32() {
    incrGeneratedNumberCount();
#ifdef USE_DUMMY_RANDOM
    return ~0U/2;
#endif
    return glibc_rand();
  }

  virtual double generate() {
    incrGeneratedNumberCount();
#ifdef USE_DUMMY_RANDOM
    return 0.5;
#endif
    return (double(glibc_rand()) / GLIBCRAND_MAX);
  }

  virtual void setSeed(int seed) {
    this->seed = seed;
    glibc_srand(seed);
  }

};

class RandomGeneratorFactory {

public:
  enum Type {
    STANDARD = 1,
    PHYSICAL
  };

private:
  Type type;

public:
  RandomGeneratorFactory(Type type) : type(type) { }

  RandomGenerator* generateRandomGenerator(int seed) const {
    switch(type) {
    case STANDARD:
#ifndef WINDOWS
      return new StandardRandomGenerator(seed);
#else 
      return new GLibCRandomGenerator(seed);
#endif
    case PHYSICAL:
      return new PhysicalRandomGenerator();
    default:
      abort();
      return NULL;
    }
  }

  std::string getName() const {
    switch(type) {
    case STANDARD:
      return "standard";
    case PHYSICAL:
      return "physical";
    default:
      abort();
      return NULL;
    }
  }

  bool isPseudoRandom() const {
    switch(type) {
    case STANDARD:
      return true;
    case PHYSICAL:
      return false;
    default:
      abort();
      return false;
    }
  }

  bool isThreadSafe() const {
    switch(type) {
    case STANDARD:
#ifdef HAS_RAND48_T
      return true;
#else
      return false;
#endif
    case PHYSICAL:
      return true;
    default:
      abort();
      return false;
    }
  }
};

#endif
