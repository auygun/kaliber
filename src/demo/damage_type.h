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
  // Enemy units (waves and boss adds).
  kEnemyType_LightSkull,
  kEnemyType_DarkSkull,
  kEnemyType_Tank,
  kEnemyType_Bug,
  kEnemyType_Unit_Last = kEnemyType_Bug,
  // Boss.
  kEnemyType_PowerUp,
  kEnemyType_Boss,
  kEnemyType_Max
};

enum SpeedType {
  kSpeedType_Invalid = -1,
  kSpeedType_Slow,
  kSpeedType_Fast,
  kSpeedType_Max
};

#endif  // DAMAGE_TYPE_H
