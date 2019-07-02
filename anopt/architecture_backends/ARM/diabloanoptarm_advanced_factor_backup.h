#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_BACKUP_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_BACKUP_H

void QueueForBackup(t_bbl *landing_site, t_reg reg, TransformedSliceInformation *info);
AddedInstructionInfo BackupRegisters(t_bbl *factored_bbl, t_regset new_live, bool added_trampoline, BblSet& modified);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_BACKUP_H */
