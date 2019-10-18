/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "attestation.h"
#include "attestation_area.h"
#include <code_mobility.h>
#include <algorithm>

using namespace Attestation;
using namespace std;

t_const_string AttestationAnnotationInfo::AID_string = NULL;
t_const_string AttestationAnnotationInfo::output_name = NULL;

static void AddAttestation(t_cfg* cfg)
{
  /* We don't need to attest mobile objects */
  if (CodeMobilityTransformer::IsMobileObject(CFG_OBJECT(cfg)))
    return;

  STATUS(START, ("Adding Attestation"));

  t_object* obj = CFG_OBJECT(cfg);
  const t_section* unified = OBJECT_CODE(obj)[0];
  set<string> attestators;

  /* Parse AID */
  t_string tmp_modify = const_cast<t_string>(AttestationAnnotationInfo::AID_string);
  t_uint64 lower_AID = strtoull(tmp_modify + 16, NULL, 16);
  char overwritten = tmp_modify[16];
  tmp_modify[16] = '\0';
  t_uint64 higher_AID = strtoull(tmp_modify, NULL, 16);
  tmp_modify[16] = overwritten;

  /* Create a file to contain metrics and print its header */
  t_const_string fname = StringConcat2(AttestationAnnotationInfo::output_name, ".attestation_metrics");
  FILE* f_metric = fopen(fname, "w+");
  Free(fname);
  fprintf(f_metric, "#region_idx,nr_of_bytes_in_ads,nr_of_blocks,nr_of_guarded_bytes\n");

  /* For every attestator find the symbols and fill in the ADS */
  t_uint64 attestator_id = 0;
  for(auto& pair : Attestator::attestators)
  {
    /* Unpack information about attestator */
    t_const_string name = pair.first.c_str();
    Attestator* attestator = &(pair.second);
    const t_symbol* base_address_sym = attestator->base_address_sym;

    /* Add relocation so the base_address variable will contain the address of the .text section */
    RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(obj),
        AddressNullForObject(obj),
        SYMBOL_BASE(base_address_sym),
        SYMBOL_OFFSET_FROM_START(base_address_sym),
        T_RELOCATABLE(unified),
        AddressNullForObject(obj),
        FALSE,
        NULL,
        NULL,
        NULL,
        "R00A00+\\l*w\\s0000$");

    /* Add dynamic relative relocation on the base_address variable if necessary */
    if ((OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC) || (OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC))
      DiabloBrokerCall ("AddDynamicRelativeRelocation", obj, SYMBOL_BASE(base_address_sym), SYMBOL_OFFSET_FROM_START(base_address_sym));

    /* Go over all regions for the attestator and count and index them all */
    Region* region;
    AttestationAnnotationInfo *info;
    vector<AttestationAnnotationInfo*> attestation_annots;
    CFG_FOREACH_ATTESTATION_REGION(cfg, region, info)
    {
      if ((region->bbls.size() == 0) || (info->attestators.find(attestator) == info->attestators.end()))
        continue;

      info->index = attestation_annots.size();
      attestation_annots.push_back(info);
    }

    /* If there are no areas, return */
    if (attestation_annots.empty())
      continue;

    /* Use a dynamically-sized vector to hold all areas */
    attestator->areas.resize(attestation_annots.size());
    vector<Area>& areas = attestator->areas;

    const t_bbl* chain = T_BBL(SECTION_TMP_BUF(unified));
    const t_bbl* bbl;
    CHAIN_FOREACH_BBL(chain, bbl)
    {
      Region *region;
      const AttestationAnnotationInfo *info;
      BBL_FOREACH_ATTESTATION_REGION(bbl, region, info)
      {
        /* Only do this if the region has the right attestator */
        if (info->attestators.find(attestator) == info->attestators.end())
          continue;

        const t_uint32 index = info->index;
        Area& area = areas[index];

        /* Set the region_idx of the area */
        area.region_idx = region->idx;

        /* If the area isn't alive at the moment, initialize a new block */
        if (area.alive == NO)
        {
          area.alive = YES;
          area.InitializeBlock(AddressSub(BBL_CADDRESS(bbl), SECTION_CADDRESS(unified)));
        }
        else
        {
          /* Liveness has been set to PERHAPS, set it to YES */
          area.alive = YES;
        }
      }

      /* Areas that are alive (= YES) will be set to PERHAPS. Those that are still at PERHAPS haven't
       * been renewed and thus are dead. We will finalize the current block.
       */
      for (auto& area : areas)
      {
        if (area.alive == YES)
        {
          area.alive = PERHAPS;
        }
        else if (area.alive == PERHAPS)
        {
          area.FinalizeBlock(AddressSub(BBL_CADDRESS(bbl), SECTION_CADDRESS(unified)));
          area.alive = NO;
        }
      }
    }

    /* Finishing up: all areas still alive at the end of the chain (these areas have a liveness of PERHAPS)
     * must be finalized, and the areas must be written to the ADS section. We will also generate a file that
     * contains the labels of all the regions that are to be attestated at startup.
     */
    t_uint16 id = 0;
    t_const_string tmpstr = StringConcat2("startup_labels_", name);
    t_section* ads = T_SECTION(SYMBOL_BASE(attestator->blob_sym));/* The ADS subsection */
    FILE* fp = fopen(tmpstr, "w");
    Free(tmpstr);
    for (size_t iii = 0; iii < areas.size(); iii++)
    {
      auto area = areas[iii];
      if (area.alive == PERHAPS)
      {
        area.FinalizeBlock(AddressSub(AddressAdd(BBL_CADDRESS(bbl), BBL_CSIZE(bbl)), SECTION_CADDRESS(unified)));
      }

      if (!area.empty())
      {
        auto annot = attestation_annots[iii];
        annot->successfully_applied = TRUE;/* If the area is not empty, the annotation was successfully applied */
        area.WriteAreaToADS(ads, id);
        area.PrintMetric(f_metric);

        /* Check if this are has to be attested at startup */
        if (dynamic_cast<RemoteAttestationAnnotationInfo*>(annot) != NULL)
        {
          RemoteAttestationAnnotationInfo* remote = dynamic_cast<RemoteAttestationAnnotationInfo*>(annot);
          if (remote->at_startup)
            fprintf(fp, "%u\n", id);
        }

        id++;
      }
    }
    fclose(fp);

    /* Fill in the beginning of the ADS */
    SectionSetData64(ads, AddressNullForObject(obj), lower_AID);
    SectionSetData64(ads, AddressNewForObject(obj, sizeof(t_uint64)), higher_AID);
    SectionSetData64(ads, AddressNewForObject(obj, 2 * sizeof(t_uint64)), attestator_id++);
    SectionSetData32(ads, AddressNewForObject(obj, 3 * sizeof(t_uint64)), id);/* Total areas */

    /* If we're dealing with a code guard attestator, we should reserve space for the checksums */
    if (!attestator->area_names.empty())
      attestator->ReserveChecksumSpace();

    /* Dump the ADS in binary format */
    tmpstr = StringConcat2("ads_", name);
    fp = fopen(tmpstr, "w");
    Free(tmpstr);
    fwrite(SECTION_DATA(ads), sizeof(char), SECTION_CSIZE(ads), fp);
    fclose(fp);
  }
  fclose(f_metric);

  STATUS(STOP, ("Adding Attestation"));
}

/* This function will adapt all the attestator callsites so that they take the ID of the area
 * they have to attest as an argument. This is far from perfect. If the area with what we think now
 * to be ID 1 ends up empty it won't be placed into the ADS at all and the area with what's currently
 * ID 2 ends up in the ADS with ID 1. We would want to only call this function and insert the constant
 * producers creating the ID argument after we know if any area might be empty, but this happens at
 * layout time, by which time it's too late to insert any instructions. Therefore we rely on the
 * current messy system. Luckily most attestators never attestate more as 1 merged region.
 */
static void AdaptAttestatorCalls(t_cfg* cfg)
{
  Region *region;
  AttestatorAnnotationInfo *info;
  CFG_FOREACH_ATTESTATOR_REGION(cfg, region, info)
  {
    /* The default argument for the inserted call is 0, so in this case there's no need to adapt the call */
    if (info->area_id == 0)
      continue;

    /* We assume there will be exactly 1 call in this region, that to the attestator */
    for (auto bbl : region->bbls)
    {
      t_cfg_edge* edge;
      BBL_FOREACH_PRED_EDGE(bbl, edge)
        if (CfgEdgeTestCategoryOr(edge, ET_CALL))
          break;

      if (edge != NULL)
      {
        /* Insert a constant producer that produces the area ID right before the call */
        t_arm_ins* tmp;
        ArmMakeInsForIns(Mov, Before, tmp, T_ARM_INS(BBL_INS_LAST(bbl)), ArmBblIsThumb(bbl), ARM_REG_R0, ARM_REG_NONE, 0, ARM_CONDITION_AL);
        ArmMakeConstantProducer(tmp, info->area_id);

        break;
      }
    }
  }
}

static void setFalse(t_bool* b)
{
  *b = FALSE;
}

void AttestationInit(t_object* obj, t_const_string AID_string, t_const_string output_name)
{
  /* Initialize global variables and install broker calls */
  AttestationAnnotationInfo::AID_string = AID_string;
  AttestationAnnotationInfo::output_name = output_name;
  DiabloBrokerCallInstall("AfterCodeLayoutFixed", "t_cfg* cfg", (void*)AddAttestation, FALSE);
  DiabloBrokerCallInstall("AfterObjectAssembled", "t_object* obj", (void*)Attestator::CalculateChecksums, FALSE);
  DiabloBrokerCallInstall("DetermineAddressProducersOptimization", "t_bool* optimize", (void*)setFalse, TRUE);

  /* Create and associate the regions with their attestators, and fill in the argument for the calls to the attestator */
  Attestator::ResolveSymbols(obj);
  Attestator::AssociateRegionsWithAttestators(OBJECT_CFG(obj));
  AdaptAttestatorCalls(OBJECT_CFG(obj));
}

Attestator::OrderedMap Attestator::attestators;

Attestator* Attestator::Create(string& label)
{
  return &(attestators[label]);
}

Attestator* Attestator::Create(string& label, vector<string>& regions, t_uint32* area_id)
{
  Attestator* ret = Create(label);

  /* Sort the regions vector (alphabetically) and create the area_name */
  sort(regions.begin(), regions.end());
  string area_name;
  for (auto& region : regions)
    area_name += "#" + region + "#";

  /* Get the location of the area_name in the vector (which is its ID) or insert it if necessary */
  auto& area_names = ret->area_names;
  auto it = find(area_names.begin(), area_names.end(), area_name);
  if (it == area_names.end())
  {
    area_names.push_back(area_name);
    *area_id = area_names.size() - 1;
  }
  else
    *area_id = it - area_names.begin();

  return ret;
}

void Attestator::ResolveSymbols(t_object* obj)
{
  for(auto& pair : attestators)
  {
    /* Unpack information about attestator */
    t_const_string name = pair.first.c_str();
    Attestator* attestator = &(pair.second);

    /* Get the symbols for the variables */
    t_const_string tmpstr = StringConcat2("base_address_", name);
    attestator->base_address_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), tmpstr);
    Free(tmpstr);

    tmpstr = StringConcat2("data_structure_blob_", name);
    attestator->blob_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), tmpstr);
    Free(tmpstr);

    tmpstr = StringConcat2("checksum_", name);
    attestator->checksum_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), tmpstr);
    Free(tmpstr);

    ASSERT(attestator->base_address_sym && attestator->blob_sym && attestator->checksum_sym, ("Some symbols required for attestation (attestator: %s) were found not to be present in the binary. Are you sure the necessary files were linked in?", name));
  }
}

void Attestator::AssociateRegionsWithAttestators(t_cfg* cfg)
{
  for(auto& pair : attestators)
  {
    Attestator* attestator = &(pair.second);

    /* For every expanded area, get or create the annotation info and add it to the right regions */
    for (auto& area_name : attestator->area_names)
    {
      auto& area_info = CodeGuardAttestationAnnotationInfo::annotations[area_name];

      /* In case it wasn't present yet in the map, we have to create the area_info (and corresponding merged region) ourselves */
      if (area_info == nullptr)
      {
        area_info = new CodeGuardAttestationAnnotationInfo();
        BblSet bbls;

        Region *region;
        CodeGuardAnnotationInfo *info;
        CFG_FOREACH_CODEGUARD_REGION(cfg, region, info)
        {
          /* If this region is part of the area, add its BBL's to those of the merged region */
          if (area_name.find("#" + info->label + "#") != string::npos)
            bbls.insert(region->bbls.begin(), region->bbls.end());
        }

        new Region(cfg, NULL, bbls, area_info);
      }

      /* Add the attestator to the area */
      area_info->attestators.insert(attestator);
    }
  }
}

typedef uint64_t result_t;
static result_t result;
static uint32_t s1;
static uint32_t s2;

static void hash_block(const uint8_t* base, size_t size)
{
  for (size_t n = 0; n < size; n++)
  {
    s1 = (s1 + base[n]) % 65521;
    s2 = (s2 + s1) % 65521;
  }
  result = (s2 << 16) | s1;
}

void Attestator::CalculateChecksums(t_object* obj)
{
  /* We don't need to calculate the checksums for mobile objects */
  if (CodeMobilityTransformer::IsMobileObject(obj))
    return;

  void* text_base = SECTION_DATA(OBJECT_CODE(obj)[0]);
  for(auto& pair : attestators)
  {
    Attestator* attestator = &(pair.second);

    /* We only have to calculate the checksums for code guard attestators that are actually covering any regions */
    if (!attestator->area_names.empty() && !attestator->areas.empty())
    {
      /* As we're already past the point where the sections are rebuilt from their subsections, we have
       * to modify the parent section itself to fill in the checksums. This requires some juggling with addresses.
       */
      t_section* checksum_subsec = T_SECTION(SYMBOL_BASE(attestator->checksum_sym));
      t_section* data_sec = SECTION_PARENT_SECTION(checksum_subsec);
      t_address offset = AddressAdd(AddressSub(SECTION_CADDRESS(checksum_subsec), SECTION_CADDRESS(data_sec)), SYMBOL_OFFSET_FROM_START(attestator->checksum_sym));

      for (const auto& area : attestator->areas)
      {
        /* Calculate the checksum for this area */
        result = 0;
        s1 = 1;
        s2 = 0;
        area.CalculateChecksum(text_base, hash_block, 0);

        /* Fill in the actual checksum */
        SectionSetData64(data_sec, offset, result);
        offset = AddressAdd(offset, attestator->checksum_size);
      }
    }
  }
}

void Attestator::ReserveChecksumSpace()
{
  /* Resize the checksum section */
  checksum_size = SectionGetData32 (T_SECTION(SYMBOL_BASE(checksum_sym)), SYMBOL_OFFSET_FROM_START(checksum_sym));
  t_section* checksum_subsec = T_SECTION(SYMBOL_BASE(checksum_sym));
  SECTION_SET_DATA(checksum_subsec, Realloc (SECTION_DATA(checksum_subsec), checksum_size * this->areas.size()));
  SECTION_SET_CSIZE(checksum_subsec, checksum_size * this->areas.size());
}
