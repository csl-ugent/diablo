#ifndef META_API_DYNSYM_H
#define META_API_DYNSYM_H

t_section *CallDynamicSymbol(t_cfg *cfg, t_string name, t_ins *ins, t_bbl *call_site, t_bbl *return_site);

#endif /* META_API_DYNSYM_H */
