/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOFLOWGRAPH_NEW_TARGET_SELECTOR_HPP
#define DIABLOFLOWGRAPH_NEW_TARGET_SELECTOR_HPP

#include "diabloflowgraph_transformation.hpp"

/* These classes return basic blocks to which other functions can jump / fallthrough directly. This can be used to randomly select (or create) a target BBL for obfuscation
 * transformations (like for the false paths in opaque predicates, fallthrough paths in branch functions, etc).
 * No edges are added. BBLs can be existing, correct basic blocks, so they should not be broken. If they are broken, they are broken by these generators.
*/

class NewTargetSelector : public Transformation {
  static constexpr t_const_string _name = "newtargetselector";
public:
  NewTargetSelector();
  // TODO: overload canTransform and doTransform with a list of potential 'next' BBLs that we can mess up :-)
  virtual bool canTransform(const t_bbl* bbl) const = 0;
  virtual t_bbl* doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) = 0;
  virtual t_const_string name() const { return _name; }
};

class InfiniteLoopSelector : public NewTargetSelector {
  static constexpr t_const_string _name = "newtargetselector:infiniteloop";
public:
  InfiniteLoopSelector();
  virtual bool canTransform(const t_bbl*) const;
  virtual t_bbl* doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual t_const_string name() const { return _name; }
};

class NewTargetInSameFunctionSelector : public NewTargetSelector {
  static constexpr t_const_string _name = "newtargetselector:samefunction";
public:
  NewTargetInSameFunctionSelector();
  virtual bool canTransform(const t_bbl*) const;
  virtual t_bbl* doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual t_const_string name() const { return _name; }
};

/* TODO: disassembly-thwarting for i386 */


void InstallGenericNewTargetHandlers();
void DestroyGenericNewTargetHandlers();

/* need_to_fall_through = true means that the function requiring a target needs to fall through to the returned BBL. If there
 * is already an (IP_)FALLTHROUGH edge, add a new BBL with a direct jump to the target, and return that instead. */
t_bbl* SelectTargetFor(t_bbl* bbl, t_randomnumbergenerator* rng, bool need_to_fall_through = false);

#endif /* DIABLOFLOWGRAPH_NEW_TARGET_SELECTOR_HPP */
