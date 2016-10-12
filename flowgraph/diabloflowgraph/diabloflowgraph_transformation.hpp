/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOFLOWGRAPH_TRANSFORMATION_HPP
#define DIABLOFLOWGRAPH_TRANSFORMATION_HPP
#include "diabloflowgraph.hpp"

#include <vector>
#include <string>

class Transformation {
public:
  virtual t_const_string name() const = 0;
  virtual void dumpStats(const std::string& prefix) {}
  virtual bool transformationIsAvailableInRandomizedList() { return true; }
  
  Transformation() {}
  virtual ~Transformation() {}
};

struct BBLTransformation : public Transformation {
  BBLTransformation();
  virtual t_const_string name() const { return "bbl_transformation"; }
  virtual bool canTransform(const t_bbl* bbl) const = 0;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) = 0;
};

struct FunctionTransformation : public Transformation {
  FunctionTransformation();
  virtual t_const_string name() const { return "function_transformation"; }
  virtual bool canTransform(const t_function* fun) const = 0;
  virtual bool doTransform(t_function* fun, t_randomnumbergenerator * rng) = 0;
};
#define DIABLOFLOWGRAPH_TRANSFORMATION_CLASS

std::vector<Transformation*> GetTransformationsForType(t_const_string type, bool is_for_randomized_list = false);
void RegisterTransformationType(Transformation* transfo, t_const_string type);

template<class T>
T* GetRandomTypedTransformationForType(t_const_string type, t_randomnumbergenerator* rng, bool is_for_randomized_list = false) {
  auto transformations = GetTransformationsForType(type, is_for_randomized_list);

  if (transformations.size() == 0)
    return nullptr;

  auto transformation_raw = transformations.at(RNGGenerateWithRange(rng, 0, transformations.size() - 1));
  auto transformation = dynamic_cast<T*>(transformation_raw);

  ASSERT(transformation, ("Expected that '%s' was a transformation, but it wasn't!", transformation_raw->name()));

  return transformation;
}

template<class T>
std::vector<T*> GetAllTypedTransformationsForType(t_const_string type, bool is_for_randomized_list = false) {
  std::vector<T*> vec;

  auto transformations = GetTransformationsForType(type);

  for (auto transformation_raw: transformations) {
    if (auto transformation = dynamic_cast<T*>(transformation_raw)) {
      vec.push_back(transformation);
    }
  }

  return vec;
}

struct MaybeSaveRegister {
  t_reg reg;
  bool  save;
  MaybeSaveRegister(t_reg reg, bool save) : reg(reg), save(save) {}
};

class ArchitectureInfo {
protected:
  t_regset possibleRegisters;
public:
  ArchitectureInfo();

  virtual t_reg getGenericRandomRegister(t_randomnumbergenerator* rng) = 0;
  virtual MaybeSaveRegister getRandomRegister(t_ins* before, t_regset not_in, t_randomnumbergenerator* rng);
  virtual MaybeSaveRegister getRandomRegisterCustomLiveness(t_regset live, t_regset not_in, t_randomnumbergenerator* rng);

  virtual void saveOrRestoreRegisters(const std::vector<MaybeSaveRegister>& regs, t_bbl* bbl, bool push, bool prepend=false) = 0; // prepend is a hack for ARM function flattening, TODO: make unneeded!

  virtual void appendUnconditionalBranchInstruction(t_bbl* bbl) = 0;
  virtual t_bbl* splitBasicBlockWithJump(t_bbl* bbl, t_ins* ins, bool before);

  /* Possibly (non)advanced detection whether or not the function returns something related to the PC, like a get_pc_thunk. This function should get its target sub-architecture
     from the function argument if required */
  virtual bool functionReturnsPC(const t_function* fun) { return false; }
};

/* ARM can possibly have different architectureInfos, depending on whether the BBL we are using as context is in ARM or Thumb2 mode... */
struct ArchitectureInfoWrapper {
  virtual ArchitectureInfo* getArchitectureInfo(t_bbl* bbl_for) = 0; // TODO: use smart pointers here
};
/* We used to get this from our ObfuscationTransformation, but by making it global some legacy, non-class-based code can also easily access it */
void SetArchitectureInfoWrapper(ArchitectureInfoWrapper* w);
ArchitectureInfoWrapper *GetArchitectureInfoWrapper();
ArchitectureInfo* GetArchitectureInfo(t_bbl* bbl_for);

#endif
