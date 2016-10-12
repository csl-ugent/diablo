/* calls all peephole optimizations consecutively */
void Amd64PeepHoles(t_cfg * cfg);

/* individual peephole optimizations */
void Amd64PeepHoleStack(t_cfg * cfg);
void Amd64PeepHoleIdempotent(t_cfg * cfg);
void Amd64PeepHoleRedundantPushes(t_cfg * cfg);
void Amd64PeepHoleShorterEquivalent(t_cfg * cfg);
void Amd64PeepHoleAddSub(t_cfg * cfg);
void Amd64PeepHoleFramePointerRemoval(t_cfg * cfg);
void Amd64PeepHoleAddSubESP(t_cfg * cfg);

/* vim: set shiftwidth=2 foldmethod=marker : */
