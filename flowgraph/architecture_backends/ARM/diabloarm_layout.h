/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOARM_LAYOUT_DEFINES
#define DIABLOARM_LAYOUT_DEFINES
#define NODE_CHAIN(node) 	(((t_chain_node*)node)->chain)
#define T_CHAIN_NODE(node)      ((t_chain_node*)node)
#define POOLENTRY_FIRSTINS(pool_entry)	(pool_entry->inslist->ins)
#endif

#ifndef DIABLOARM_LAYOUT_TYPEDEFS
#define DIABLOARM_LAYOUT_TYPEDEFS
typedef struct _t_data_pool_entry t_data_pool_entry;
typedef struct _t_data_pool t_data_pool;
typedef struct _t_chain_node t_chain_node;
#endif

#ifdef DIABLOARM_TYPES
#ifndef DIABLOARM_LAYOUT_TYPES
#define DIABLOARM_LAYOUT_TYPES
struct _t_data_pool_entry
{
  t_uint32 nr_ins;
  t_inslist * inslist;
  struct _t_data_pool_entry * next;
};

struct _t_data_pool
{
  t_bbl * bbl;
  t_data_pool_entry * list;
  t_data_pool_entry * end;
  t_int32 nr_entries;
};

struct _t_chain_node
{
  t_node node;
  t_bbl * chain;
  struct _t_chain_node *next_marked;
  t_uint32 nr_edges_out;
  t_uint32 nr_edges_in;
};



typedef struct _t_cluster t_cluster;
typedef struct _t_clustercollection t_clustercollection;
typedef struct _t_clusterchain t_clusterchain;
typedef struct _t_bbllist t_bbllist;
typedef struct _t_chaininfo t_chaininfo;

struct _t_cluster
{
  t_clusterchain ** chains;
  t_uint32 array_size;
  t_uint32 nchains;
};

struct _t_clustercollection
{
  t_cluster ** clusters;
  t_uint32 array_size;
  t_uint32 nclusters;
};

struct _t_clusterchain
{
  t_bbl * head;

  /* contains the predecessor/successor clusters
   * (the clusters from which a CB(N)Z instruction jumps to this chain)
   * (the clusters to which a CB(N)Z instruction inside this chain jumps) */
  t_clustercollection * linked_clusters;
};

struct _t_bbllist
{
  t_bbl ** bbls;
  t_uint32 array_size;
  t_uint32 nbbls;
};

struct _t_chaininfo
{
  t_bbl * chain;
  t_address lowest;
};

#endif
#endif

#ifdef DIABLOARM_FUNCTIONS
#ifndef DIABLOARM_LAYOUT_FUNCTIONS
#define DIABLOARM_LAYOUT_FUNCTIONS
void ArmCfgLayout(t_cfg * cfg, t_uint32 mode);
void ArmCreateChains(t_cfg * cfg, t_chain_holder *ch);
void ArmSortChains(t_chain_holder *ch);
void ArmAddressProducers(t_object *obj, t_cfg * cfg, t_uint32 mode);
void ArmRelocate(t_cfg * cfg, t_uint32 mode);
void ArmGenerateFloatProducers(t_object *obj);
void GenerateInstructionsForConstProdIfPossible(t_arm_ins *ins, t_int32 val);
void ArmValidateChainsForThumbBranches(t_chain_holder * ch);
void ArmClusterCollectionAddCluster(t_clustercollection * cc, t_cluster * c);
void ArmClusterAddChain(t_cluster * c, t_clusterchain * chain);
t_clustercollection * ArmClusterChains(t_cfg * cfg, t_chain_holder * ch);
t_clustercollection * ArmSingleCluster(t_cfg * cfg, t_chain_holder * ch);
t_clustercollection * ArmClusterCollectionCreate();
t_cluster * ArmClusterCreate();
t_clusterchain * ArmClusterChainCreate(t_bbl * chain_head);
t_cluster * ArmClusterMerge(t_cluster * a, t_cluster * b);
void ArmClusterCollectionRemoveDuplicates(t_clustercollection * cc);
void ArmClusterRemoveLinkToCluster(t_cluster * c, t_cluster * link);
t_clustercollection * ArmClusterCollectionCreateSized(t_uint32 n);
void ArmClusterPrint(t_cluster * c);
void ArmClusterCollectionDestroy(t_clustercollection * cc, t_bool everything);
void ArmClusterDestroy(t_cluster * c);
void ArmClusterChainDestroy(t_clusterchain * cc);
void ArmClusterChainBblsByAddress(t_clustercollection * cc, t_chain_holder * ch);
void ArmBblListAddBbl(t_bbllist * bl, t_bbl * b);

/* dynamic members of BBL's */
BBL_DYNAMIC_MEMBER_GLOBAL(cluster, CLUSTER, Cluster, t_cluster *, NULL);
#endif
#endif
