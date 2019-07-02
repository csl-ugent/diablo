#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_SLICE_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_SLICE_H

#define SLICECOMPARE_NE 2
#define SLICECOMPARE_GT 1
#define SLICECOMPARE_EQ 0
#define SLICECOMPARE_LT -1

class Slice;
struct FactoringPossibility;
struct FactoredPath;
typedef std::function<bool(Slice *slice)> F_SliceIsMaster;
typedef std::function<bool(Slice *slice)> F_SliceIsSlave;

typedef std::map<size_t, t_reg> t_vreg_to_reg_map;
typedef std::pair<size_t, t_reg> t_vreg_to_reg_map_item;

struct VirtualizationResult {
  t_vreg_to_reg_map input_vregs;
  t_vreg_to_reg_map internal_vregs;
  t_vreg_to_reg_map output_vregs;
};

struct SliceCompare
{
	bool operator() (const Slice* left, const Slice* right) const;
};

class Slice {
public:
	Slice(F_SliceIsMaster p_IsMaster, F_SliceIsSlave p_IsSlave, bool p_is_sequence) {
		static t_uint64 global_uid = 0;

		IsMaster = p_IsMaster;
		IsSlave = p_IsSlave;

		cache_invalidated = false;
		invalidated = false;
		parent_slice = NULL;
		dont_destroy = false;
		uid = global_uid++;
		is_sequence = p_is_sequence;
		combine_data_fixed = false;
    procstate_before = NULL;

		null_before = NullRegs;
		constant_before = NullRegs;
		tag_before = NullRegs;
		nonzero_before = NullRegs;
    overwritten_registers = NullRegs;

		min_order = 10000000;
		max_order = 0;
		//DEBUG(("creating slice %p", this));
	};

	~Slice();

	void FillWithBbl(t_ins *ins);
  void FillWithSequence(t_ins *ins);
	void FillWithSequenceAddr(t_ins *ins);
  void FillWithDefs(t_ins *ins);
	void AddInstruction(t_ins *ins);
	void SetBaseInstruction(t_ins *ins) {
		base_instruction = ins;

		/* source information */
		BblSourceLocation(Bbl(), origin_function, origin_object, origin_archive);
	}
	t_bool RescheduleBbl();

	int Compare(Slice *b, size_t& nr_match, int max_len = 0);
	std::string Print(size_t slice_size = 0);
	bool Overlaps(Slice *b);

	t_bool CompareExact(Slice *b, size_t& nr_match, int max_len = 0);

	size_t Size() { return NrInstructions(); }
	t_ins *Get(int i);
	t_ins *GetR(int i);
  t_ins *Get(int i, size_t slice_size);

	void AssociateWithIns(t_ins *ins);
	void DissociateFromIns(t_ins *ins);

	void Mark() {
		BblMark(INS_BBL(base_instruction));
	}
	bool IsMarked() {
		return BblIsMarked(INS_BBL(base_instruction));
	}

  t_bbl *Bbl() const {
    return INS_BBL(base_instruction);
  }
	bool IsExecuted() const {
		return BBL_EXEC_COUNT(Bbl()) > 0;
	}
	int Id() const;

  std::vector<t_ins *> Instructions() {
		return elements;
  }

	size_t NrInstructions();

	void Finalize() {
		std::reverse(elements.begin(), elements.end());
	}

  bool ContainsInstruction(t_ins *ins, size_t slice_size);

	void Invalidate() {
		if (!invalidated)
		{
			//DEBUG(("invalidating %p with base @I", this, base_instruction));
			//DEBUG(("%s", Print().c_str()));
			invalidated = true;
			InvalidateAllRelatedSlices();

			if (parent_slice)
				parent_slice->Invalidate();
		}
	}

	int RegionId() {
		return BBL_FACTORING_REGION_ID(Bbl());
	}

	bool IsInvalidated() {
		return invalidated;
	}

	void SetParentSlice(Slice *slice) {
		parent_slice = slice;
	}

	Slice *ParentSlice() {
		return parent_slice;
	}

	void ProtectFromDestruction() {
		dont_destroy = true;
	}

	void UnprotectFromDestruction() {
		dont_destroy = false;
	}

	void InvalidateAllRelatedSlices();

	void UpdateInstructions();

	std::vector<FactoredPath> CalculateFactoredPaths(t_regset live_registers);

  void FixCombineResults();
  void UnfixCombineResults();
	void PrecalculateCombineData(size_t slice_size);
  bool CanProduceZeroInRegister(t_reg reg, bool& already, bool& mov, bool& create);
  bool CanProduceNonzeroInRegister(t_reg reg, bool& already, bool& mov, bool& create);

	/* these need to be accessible from outside the class */
	F_SliceIsMaster IsMaster;
	t_ins *base_instruction;
	bool dont_destroy;
	t_uint64 uid;
	int group_id;
	std::vector<t_ins *> elements;
	t_uint32 my_index;

  /* used for register equalisation */
  t_regset regeq_live_out;
  VirtualizationResult regeq_virtual;
  t_uint8 regeq_nr_mapped_regs;

	bool cache_invalidated;
	bool is_sequence;
	bool combine_data_fixed;
	t_regset dead_through;
	t_regset nonzero_before;
	t_regset constant_before;
	t_regset null_before;
	t_regset tag_before;
  t_regset live_after;
  t_regset overwritten_registers;
	t_gpregisters can_contain_z;
	t_gpregisters can_contain_nz;

	/* source file location */
	FunctionUID origin_function;
	SourceFileUID origin_object;
	SourceArchiveUID origin_archive;

	int min_order;
	int max_order;

	std::set<t_arm_ins *> address_before;
	std::set<t_arm_ins *> address_after;

  t_regset dead_before;

private:
	F_SliceIsSlave IsSlave;
	bool invalidated;
	Slice *parent_slice;
	t_procstate *procstate_before;
};

Slice *CalculateSliceForInstruction(t_ins *ins, F_SliceIsMaster IsMaster, F_SliceIsSlave IsSlave, bool sequence, bool sequence_without_addrprod);
void InsSliceFree(Slice *val);
void InsInvalidateSlices(t_ins *ins);
t_uint32 SlicePathCount(Slice *slice);

/* dynamic members */
extern SliceSet created_slices_global;
INS_DYNAMIC_MEMBER_GLOBAL_BODY(slice, SLICE, Slice, Slice*, {*valp=NULL;}, {InsSliceFree(*valp);}, {*valp=NULL;});
INS_DYNAMIC_MEMBER_GLOBAL(slice_id, SLICE_ID, SliceId, int, -1);
INS_DYNAMIC_MEMBER_GLOBAL(mark, MARK, Mark, bool, false);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_SLICE_H */
