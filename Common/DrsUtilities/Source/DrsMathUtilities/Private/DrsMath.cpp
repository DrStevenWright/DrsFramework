// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMath.h"

FQuat Drs::Math::ToYawOnly(const FQuat& rotation)
{
	return FRotator(0.0, rotation.Rotator().Yaw, 0.0).Quaternion();
}
