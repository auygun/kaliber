#ifndef DAMAGE_TYPE_H
#define DAMAGE_TYPE_H

enum DamageType {
  kDamageType_Invalid = -1,
  kDamageType_Green,
  kDamageType_Blue,
  kDamageType_Any,
  kDamageType_Max
};

enum EnemyType {
  kEnemyType_Invalid = -1,
  kEnemyType_LightSkull,
  kEnemyType_DarkSkull,
  kEnemyType_Tank,
  kEnemyType_Bug,
  kEnemyType_Max
};

#endif  // DAMAGE_TYPE_H
