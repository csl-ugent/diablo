#ifndef ARM_ADVANCED_FACTORING_H
#define ARM_ADVANCED_FACTORING_H

void MarkSlicesInBbl(t_bbl *, int (*)(t_ins *), void (*)(t_ins *, int));
void MarkSliceForIns(t_ins *, int (*)(t_ins *), void (*)(t_ins *, int));
void RescheduleBblForSlice(t_bbl *bbl, void (*)(t_ins *, int));
void RescheduleBblForSequence(t_bbl *bbl, int (*)(t_ins *));
void CanonicalizeBbl(t_bbl *bbl, t_uint16 (*)(t_ins *));
void FreeDagForBbl(t_bbl *);

#endif /* ARM_ADVANCED_FACTORING_H */
