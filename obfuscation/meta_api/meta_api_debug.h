#ifndef META_API_DEBUG_H
#define META_API_DEBUG_H

extern bool force_dots;

void DumpDots(t_cfg *cfg, std::string prefix, int uid);
void DumpDotsF(FunctionSet functions_to_draw, std::string prefix, int uid);

#endif /* META_API_DEBUG_H */
