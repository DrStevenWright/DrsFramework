// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsMath.h"

FQuat Drs::Math::ToYawOnly(const FQuat& Rotation)
{
	return FRotator(0.0, Rotation.Rotator().Yaw, 0.0).Quaternion();
}
