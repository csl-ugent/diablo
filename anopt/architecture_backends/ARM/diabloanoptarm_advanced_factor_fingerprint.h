#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_FINGERPRINT_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_FINGERPRINT_H

#define InstructionFingerprintNull 0

template<typename T>
class InstructionFingerprintC {
public:
  static T GetFingerprint(t_ins *ins);
};

typedef t_uint16 InstructionFingerprintType;
typedef InstructionFingerprintC<InstructionFingerprintType> InstructionFingerprint;
typedef std::vector<InstructionFingerprintType> InstructionFingerprintVector;

#include "diabloanoptarm_advanced_factor_fingerprint.cpp"

/* dynamic members */
INS_DYNAMIC_MEMBER_GLOBAL(fingerprint, FINGERPRINT, Fingerprint, InstructionFingerprintType, InstructionFingerprintNull);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_FINGERPRINT_H */
