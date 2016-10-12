/* calls all peephole optimizations consecutively */
void I386PeepHoles(t_cfg * cfg);

/* individual peephole optimizations */
void I386PeepHoleStack(t_cfg * cfg);
void I386PeepHoleIdempotent(t_cfg * cfg);
void I386PeepHoleRedundantPushes(t_cfg * cfg);
void I386PeepHoleShorterEquivalent(t_cfg * cfg);
void I386PeepHoleAddSub(t_cfg * cfg);
void I386PeepHoleFramePointerRemoval(t_cfg * cfg);
void I386PeepHoleAddSubESP(t_cfg * cfg);

/* vim: set shiftwidth=2 foldmethod=marker : */
