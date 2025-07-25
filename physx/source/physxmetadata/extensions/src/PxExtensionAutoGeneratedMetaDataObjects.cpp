// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2025 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

// This code is auto-generated by the PhysX Clang metadata generator.  Do not edit or be
// prepared for your edits to be quietly ignored next time the clang metadata generator is
// run.  You can find the most recent version of clang metadata generator by contacting
// Chris Nuernberger <chrisn@nvidia.com> or Dilip or Adam.
// The source code for the generate was at one time checked into:
// physx/PhysXMetaDataGenerator/llvm/tools/clang/lib/Frontend/PhysXMetaDataAction.cpp
#include "foundation/PxPreprocessor.h"
#if PX_LINUX && PX_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#include "PxExtensionMetaDataObjects.h"
#if PX_LINUX && PX_CLANG
#pragma clang diagnostic pop
#endif

#include "PxMetaDataCppPrefix.h"
#include "extensions/PxExtensionsAPI.h"
using namespace physx;
void setPxJoint_Actors( PxJoint* inObj, PxRigidActor * inArg0, PxRigidActor * inArg1 ) { inObj->setActors( inArg0, inArg1 ); }
void getPxJoint_Actors( const PxJoint* inObj, PxRigidActor *& inArg0, PxRigidActor *& inArg1 ) { inObj->getActors( inArg0, inArg1 ); }
void setPxJoint_LocalPose( PxJoint* inObj, PxJointActorIndex::Enum inIndex, PxTransform inArg ){ inObj->setLocalPose( inIndex, inArg ); }
PxTransform getPxJoint_LocalPose( const PxJoint* inObj, PxJointActorIndex::Enum inIndex ) { return inObj->getLocalPose( inIndex ); }
PxTransform getPxJoint_RelativeTransform( const PxJoint* inObj ) { return inObj->getRelativeTransform(); }
PxVec3 getPxJoint_RelativeLinearVelocity( const PxJoint* inObj ) { return inObj->getRelativeLinearVelocity(); }
PxVec3 getPxJoint_RelativeAngularVelocity( const PxJoint* inObj ) { return inObj->getRelativeAngularVelocity(); }
void setPxJoint_BreakForce( PxJoint* inObj, PxReal inArg0, PxReal inArg1 ) { inObj->setBreakForce( inArg0, inArg1 ); }
void getPxJoint_BreakForce( const PxJoint* inObj, PxReal& inArg0, PxReal& inArg1 ) { inObj->getBreakForce( inArg0, inArg1 ); }
void setPxJoint_ConstraintFlags( PxJoint* inObj, PxConstraintFlags inArg){ inObj->setConstraintFlags( inArg ); }
PxConstraintFlags getPxJoint_ConstraintFlags( const PxJoint* inObj ) { return inObj->getConstraintFlags(); }
void setPxJoint_InvMassScale0( PxJoint* inObj, PxReal inArg){ inObj->setInvMassScale0( inArg ); }
PxReal getPxJoint_InvMassScale0( const PxJoint* inObj ) { return inObj->getInvMassScale0(); }
void setPxJoint_InvInertiaScale0( PxJoint* inObj, PxReal inArg){ inObj->setInvInertiaScale0( inArg ); }
PxReal getPxJoint_InvInertiaScale0( const PxJoint* inObj ) { return inObj->getInvInertiaScale0(); }
void setPxJoint_InvMassScale1( PxJoint* inObj, PxReal inArg){ inObj->setInvMassScale1( inArg ); }
PxReal getPxJoint_InvMassScale1( const PxJoint* inObj ) { return inObj->getInvMassScale1(); }
void setPxJoint_InvInertiaScale1( PxJoint* inObj, PxReal inArg){ inObj->setInvInertiaScale1( inArg ); }
PxReal getPxJoint_InvInertiaScale1( const PxJoint* inObj ) { return inObj->getInvInertiaScale1(); }
PxConstraint * getPxJoint_Constraint( const PxJoint* inObj ) { return inObj->getConstraint(); }
void setPxJoint_Name( PxJoint* inObj, const char * inArg){ inObj->setName( inArg ); }
const char * getPxJoint_Name( const PxJoint* inObj ) { return inObj->getName(); }
PxScene * getPxJoint_Scene( const PxJoint* inObj ) { return inObj->getScene(); }
inline void * getPxJointUserData( const PxJoint* inOwner ) { return inOwner->userData; }
inline void setPxJointUserData( PxJoint* inOwner, void * inData) { inOwner->userData = inData; }
 PxJointGeneratedInfo::PxJointGeneratedInfo()
	: Actors( "Actors", "actor0", "actor1", setPxJoint_Actors, getPxJoint_Actors)
	, LocalPose( "LocalPose", setPxJoint_LocalPose, getPxJoint_LocalPose)
	, RelativeTransform( "RelativeTransform", getPxJoint_RelativeTransform)
	, RelativeLinearVelocity( "RelativeLinearVelocity", getPxJoint_RelativeLinearVelocity)
	, RelativeAngularVelocity( "RelativeAngularVelocity", getPxJoint_RelativeAngularVelocity)
	, BreakForce( "BreakForce", "force", "torque", setPxJoint_BreakForce, getPxJoint_BreakForce)
	, ConstraintFlags( "ConstraintFlags", setPxJoint_ConstraintFlags, getPxJoint_ConstraintFlags)
	, InvMassScale0( "InvMassScale0", setPxJoint_InvMassScale0, getPxJoint_InvMassScale0)
	, InvInertiaScale0( "InvInertiaScale0", setPxJoint_InvInertiaScale0, getPxJoint_InvInertiaScale0)
	, InvMassScale1( "InvMassScale1", setPxJoint_InvMassScale1, getPxJoint_InvMassScale1)
	, InvInertiaScale1( "InvInertiaScale1", setPxJoint_InvInertiaScale1, getPxJoint_InvInertiaScale1)
	, Constraint( "Constraint", getPxJoint_Constraint)
	, Name( "Name", setPxJoint_Name, getPxJoint_Name)
	, Scene( "Scene", getPxJoint_Scene)
	, UserData( "UserData", setPxJointUserData, getPxJointUserData )
{}
 PxJointGeneratedValues::PxJointGeneratedValues( const PxJoint* inSource )
		:RelativeTransform( getPxJoint_RelativeTransform( inSource ) )
		,RelativeLinearVelocity( getPxJoint_RelativeLinearVelocity( inSource ) )
		,RelativeAngularVelocity( getPxJoint_RelativeAngularVelocity( inSource ) )
		,ConstraintFlags( getPxJoint_ConstraintFlags( inSource ) )
		,InvMassScale0( getPxJoint_InvMassScale0( inSource ) )
		,InvInertiaScale0( getPxJoint_InvInertiaScale0( inSource ) )
		,InvMassScale1( getPxJoint_InvMassScale1( inSource ) )
		,InvInertiaScale1( getPxJoint_InvInertiaScale1( inSource ) )
		,Constraint( getPxJoint_Constraint( inSource ) )
		,Name( getPxJoint_Name( inSource ) )
		,Scene( getPxJoint_Scene( inSource ) )
		,UserData( inSource->userData )
{
	PX_UNUSED(inSource);
	getPxJoint_Actors( inSource, Actors[0], Actors[1] );
		for ( PxU32 idx = 0; idx < static_cast<PxU32>( physx::PxJointActorIndex::COUNT ); ++idx )
		LocalPose[idx] = getPxJoint_LocalPose( inSource, static_cast< PxJointActorIndex::Enum >( idx ) );
	getPxJoint_BreakForce( inSource, BreakForce[0], BreakForce[1] );
}
void setPxRackAndPinionJoint_Ratio( PxRackAndPinionJoint* inObj, float inArg){ inObj->setRatio( inArg ); }
float getPxRackAndPinionJoint_Ratio( const PxRackAndPinionJoint* inObj ) { return inObj->getRatio(); }
const char * getPxRackAndPinionJoint_ConcreteTypeName( const PxRackAndPinionJoint* inObj ) { return inObj->getConcreteTypeName(); }
 PxRackAndPinionJointGeneratedInfo::PxRackAndPinionJointGeneratedInfo()
	: Ratio( "Ratio", setPxRackAndPinionJoint_Ratio, getPxRackAndPinionJoint_Ratio)
	, ConcreteTypeName( "ConcreteTypeName", getPxRackAndPinionJoint_ConcreteTypeName)
{}
 PxRackAndPinionJointGeneratedValues::PxRackAndPinionJointGeneratedValues( const PxRackAndPinionJoint* inSource )
		:PxJointGeneratedValues( inSource )
		,Ratio( getPxRackAndPinionJoint_Ratio( inSource ) )
		,ConcreteTypeName( getPxRackAndPinionJoint_ConcreteTypeName( inSource ) )
{
	PX_UNUSED(inSource);
}
void setPxGearJoint_GearRatio( PxGearJoint* inObj, float inArg){ inObj->setGearRatio( inArg ); }
float getPxGearJoint_GearRatio( const PxGearJoint* inObj ) { return inObj->getGearRatio(); }
const char * getPxGearJoint_ConcreteTypeName( const PxGearJoint* inObj ) { return inObj->getConcreteTypeName(); }
 PxGearJointGeneratedInfo::PxGearJointGeneratedInfo()
	: GearRatio( "GearRatio", setPxGearJoint_GearRatio, getPxGearJoint_GearRatio)
	, ConcreteTypeName( "ConcreteTypeName", getPxGearJoint_ConcreteTypeName)
{}
 PxGearJointGeneratedValues::PxGearJointGeneratedValues( const PxGearJoint* inSource )
		:PxJointGeneratedValues( inSource )
		,GearRatio( getPxGearJoint_GearRatio( inSource ) )
		,ConcreteTypeName( getPxGearJoint_ConcreteTypeName( inSource ) )
{
	PX_UNUSED(inSource);
}
void setPxD6Joint_Motion( PxD6Joint* inObj, PxD6Axis::Enum inIndex, PxD6Motion::Enum inArg ){ inObj->setMotion( inIndex, inArg ); }
PxD6Motion::Enum getPxD6Joint_Motion( const PxD6Joint* inObj, PxD6Axis::Enum inIndex ) { return inObj->getMotion( inIndex ); }
PxReal getPxD6Joint_TwistAngle( const PxD6Joint* inObj ) { return inObj->getTwistAngle(); }
PxReal getPxD6Joint_Twist( const PxD6Joint* inObj ) { return inObj->getTwist(); }
PxReal getPxD6Joint_SwingYAngle( const PxD6Joint* inObj ) { return inObj->getSwingYAngle(); }
PxReal getPxD6Joint_SwingZAngle( const PxD6Joint* inObj ) { return inObj->getSwingZAngle(); }
void setPxD6Joint_DistanceLimit( PxD6Joint* inObj, const PxJointLinearLimit & inArg){ inObj->setDistanceLimit( inArg ); }
PxJointLinearLimit getPxD6Joint_DistanceLimit( const PxD6Joint* inObj ) { return inObj->getDistanceLimit(); }
void setPxD6Joint_LinearLimit( PxD6Joint* inObj, const PxJointLinearLimit & inArg){ inObj->setLinearLimit( inArg ); }
PxJointLinearLimit getPxD6Joint_LinearLimit( const PxD6Joint* inObj ) { return inObj->getLinearLimit(); }
void setPxD6Joint_TwistLimit( PxD6Joint* inObj, const PxJointAngularLimitPair & inArg){ inObj->setTwistLimit( inArg ); }
PxJointAngularLimitPair getPxD6Joint_TwistLimit( const PxD6Joint* inObj ) { return inObj->getTwistLimit(); }
void setPxD6Joint_SwingLimit( PxD6Joint* inObj, const PxJointLimitCone & inArg){ inObj->setSwingLimit( inArg ); }
PxJointLimitCone getPxD6Joint_SwingLimit( const PxD6Joint* inObj ) { return inObj->getSwingLimit(); }
void setPxD6Joint_PyramidSwingLimit( PxD6Joint* inObj, const PxJointLimitPyramid & inArg){ inObj->setPyramidSwingLimit( inArg ); }
PxJointLimitPyramid getPxD6Joint_PyramidSwingLimit( const PxD6Joint* inObj ) { return inObj->getPyramidSwingLimit(); }
void setPxD6Joint_Drive( PxD6Joint* inObj, PxD6Drive::Enum inIndex, PxD6JointDrive inArg ){ inObj->setDrive( inIndex, inArg ); }
PxD6JointDrive getPxD6Joint_Drive( const PxD6Joint* inObj, PxD6Drive::Enum inIndex ) { return inObj->getDrive( inIndex ); }
void setPxD6Joint_DrivePosition( PxD6Joint* inObj, const PxTransform & inArg){ inObj->setDrivePosition( inArg ); }
PxTransform getPxD6Joint_DrivePosition( const PxD6Joint* inObj ) { return inObj->getDrivePosition(); }
const char * getPxD6Joint_ConcreteTypeName( const PxD6Joint* inObj ) { return inObj->getConcreteTypeName(); }
void setPxD6Joint_AngularDriveConfig( PxD6Joint* inObj, PxD6AngularDriveConfig::Enum inArg){ inObj->setAngularDriveConfig( inArg ); }
PxD6AngularDriveConfig::Enum getPxD6Joint_AngularDriveConfig( const PxD6Joint* inObj ) { return inObj->getAngularDriveConfig(); }
 PxD6JointGeneratedInfo::PxD6JointGeneratedInfo()
	: Motion( "Motion", setPxD6Joint_Motion, getPxD6Joint_Motion)
	, TwistAngle( "TwistAngle", getPxD6Joint_TwistAngle)
	, Twist( "Twist", getPxD6Joint_Twist)
	, SwingYAngle( "SwingYAngle", getPxD6Joint_SwingYAngle)
	, SwingZAngle( "SwingZAngle", getPxD6Joint_SwingZAngle)
	, DistanceLimit( "DistanceLimit", setPxD6Joint_DistanceLimit, getPxD6Joint_DistanceLimit)
	, LinearLimit( "LinearLimit", setPxD6Joint_LinearLimit, getPxD6Joint_LinearLimit)
	, TwistLimit( "TwistLimit", setPxD6Joint_TwistLimit, getPxD6Joint_TwistLimit)
	, SwingLimit( "SwingLimit", setPxD6Joint_SwingLimit, getPxD6Joint_SwingLimit)
	, PyramidSwingLimit( "PyramidSwingLimit", setPxD6Joint_PyramidSwingLimit, getPxD6Joint_PyramidSwingLimit)
	, Drive( "Drive", setPxD6Joint_Drive, getPxD6Joint_Drive)
	, DrivePosition( "DrivePosition", setPxD6Joint_DrivePosition, getPxD6Joint_DrivePosition)
	, ConcreteTypeName( "ConcreteTypeName", getPxD6Joint_ConcreteTypeName)
	, AngularDriveConfig( "AngularDriveConfig", setPxD6Joint_AngularDriveConfig, getPxD6Joint_AngularDriveConfig)
{}
 PxD6JointGeneratedValues::PxD6JointGeneratedValues( const PxD6Joint* inSource )
		:PxJointGeneratedValues( inSource )
		,TwistAngle( getPxD6Joint_TwistAngle( inSource ) )
		,Twist( getPxD6Joint_Twist( inSource ) )
		,SwingYAngle( getPxD6Joint_SwingYAngle( inSource ) )
		,SwingZAngle( getPxD6Joint_SwingZAngle( inSource ) )
		,DistanceLimit( getPxD6Joint_DistanceLimit( inSource ) )
		,LinearLimit( getPxD6Joint_LinearLimit( inSource ) )
		,TwistLimit( getPxD6Joint_TwistLimit( inSource ) )
		,SwingLimit( getPxD6Joint_SwingLimit( inSource ) )
		,PyramidSwingLimit( getPxD6Joint_PyramidSwingLimit( inSource ) )
		,DrivePosition( getPxD6Joint_DrivePosition( inSource ) )
		,ConcreteTypeName( getPxD6Joint_ConcreteTypeName( inSource ) )
		,AngularDriveConfig( getPxD6Joint_AngularDriveConfig( inSource ) )
{
	PX_UNUSED(inSource);
		for ( PxU32 idx = 0; idx < static_cast<PxU32>( physx::PxD6Axis::eCOUNT ); ++idx )
		Motion[idx] = getPxD6Joint_Motion( inSource, static_cast< PxD6Axis::Enum >( idx ) );
		for ( PxU32 idx = 0; idx < static_cast<PxU32>( physx::PxD6Drive::eCOUNT ); ++idx )
		Drive[idx] = getPxD6Joint_Drive( inSource, static_cast< PxD6Drive::Enum >( idx ) );
}
PxReal getPxDistanceJoint_Distance( const PxDistanceJoint* inObj ) { return inObj->getDistance(); }
void setPxDistanceJoint_MinDistance( PxDistanceJoint* inObj, PxReal inArg){ inObj->setMinDistance( inArg ); }
PxReal getPxDistanceJoint_MinDistance( const PxDistanceJoint* inObj ) { return inObj->getMinDistance(); }
void setPxDistanceJoint_MaxDistance( PxDistanceJoint* inObj, PxReal inArg){ inObj->setMaxDistance( inArg ); }
PxReal getPxDistanceJoint_MaxDistance( const PxDistanceJoint* inObj ) { return inObj->getMaxDistance(); }
void setPxDistanceJoint_Tolerance( PxDistanceJoint* inObj, PxReal inArg){ inObj->setTolerance( inArg ); }
PxReal getPxDistanceJoint_Tolerance( const PxDistanceJoint* inObj ) { return inObj->getTolerance(); }
void setPxDistanceJoint_Stiffness( PxDistanceJoint* inObj, PxReal inArg){ inObj->setStiffness( inArg ); }
PxReal getPxDistanceJoint_Stiffness( const PxDistanceJoint* inObj ) { return inObj->getStiffness(); }
void setPxDistanceJoint_Damping( PxDistanceJoint* inObj, PxReal inArg){ inObj->setDamping( inArg ); }
PxReal getPxDistanceJoint_Damping( const PxDistanceJoint* inObj ) { return inObj->getDamping(); }
void setPxDistanceJoint_DistanceJointFlags( PxDistanceJoint* inObj, PxDistanceJointFlags inArg){ inObj->setDistanceJointFlags( inArg ); }
PxDistanceJointFlags getPxDistanceJoint_DistanceJointFlags( const PxDistanceJoint* inObj ) { return inObj->getDistanceJointFlags(); }
const char * getPxDistanceJoint_ConcreteTypeName( const PxDistanceJoint* inObj ) { return inObj->getConcreteTypeName(); }
 PxDistanceJointGeneratedInfo::PxDistanceJointGeneratedInfo()
	: Distance( "Distance", getPxDistanceJoint_Distance)
	, MinDistance( "MinDistance", setPxDistanceJoint_MinDistance, getPxDistanceJoint_MinDistance)
	, MaxDistance( "MaxDistance", setPxDistanceJoint_MaxDistance, getPxDistanceJoint_MaxDistance)
	, Tolerance( "Tolerance", setPxDistanceJoint_Tolerance, getPxDistanceJoint_Tolerance)
	, Stiffness( "Stiffness", setPxDistanceJoint_Stiffness, getPxDistanceJoint_Stiffness)
	, Damping( "Damping", setPxDistanceJoint_Damping, getPxDistanceJoint_Damping)
	, DistanceJointFlags( "DistanceJointFlags", setPxDistanceJoint_DistanceJointFlags, getPxDistanceJoint_DistanceJointFlags)
	, ConcreteTypeName( "ConcreteTypeName", getPxDistanceJoint_ConcreteTypeName)
{}
 PxDistanceJointGeneratedValues::PxDistanceJointGeneratedValues( const PxDistanceJoint* inSource )
		:PxJointGeneratedValues( inSource )
		,Distance( getPxDistanceJoint_Distance( inSource ) )
		,MinDistance( getPxDistanceJoint_MinDistance( inSource ) )
		,MaxDistance( getPxDistanceJoint_MaxDistance( inSource ) )
		,Tolerance( getPxDistanceJoint_Tolerance( inSource ) )
		,Stiffness( getPxDistanceJoint_Stiffness( inSource ) )
		,Damping( getPxDistanceJoint_Damping( inSource ) )
		,DistanceJointFlags( getPxDistanceJoint_DistanceJointFlags( inSource ) )
		,ConcreteTypeName( getPxDistanceJoint_ConcreteTypeName( inSource ) )
{
	PX_UNUSED(inSource);
}
const char * getPxFixedJoint_ConcreteTypeName( const PxFixedJoint* inObj ) { return inObj->getConcreteTypeName(); }
 PxFixedJointGeneratedInfo::PxFixedJointGeneratedInfo()
	: ConcreteTypeName( "ConcreteTypeName", getPxFixedJoint_ConcreteTypeName)
{}
 PxFixedJointGeneratedValues::PxFixedJointGeneratedValues( const PxFixedJoint* inSource )
		:PxJointGeneratedValues( inSource )
		,ConcreteTypeName( getPxFixedJoint_ConcreteTypeName( inSource ) )
{
	PX_UNUSED(inSource);
}
PxReal getPxPrismaticJoint_Position( const PxPrismaticJoint* inObj ) { return inObj->getPosition(); }
PxReal getPxPrismaticJoint_Velocity( const PxPrismaticJoint* inObj ) { return inObj->getVelocity(); }
void setPxPrismaticJoint_Limit( PxPrismaticJoint* inObj, const PxJointLinearLimitPair & inArg){ inObj->setLimit( inArg ); }
PxJointLinearLimitPair getPxPrismaticJoint_Limit( const PxPrismaticJoint* inObj ) { return inObj->getLimit(); }
void setPxPrismaticJoint_PrismaticJointFlags( PxPrismaticJoint* inObj, PxPrismaticJointFlags inArg){ inObj->setPrismaticJointFlags( inArg ); }
PxPrismaticJointFlags getPxPrismaticJoint_PrismaticJointFlags( const PxPrismaticJoint* inObj ) { return inObj->getPrismaticJointFlags(); }
const char * getPxPrismaticJoint_ConcreteTypeName( const PxPrismaticJoint* inObj ) { return inObj->getConcreteTypeName(); }
 PxPrismaticJointGeneratedInfo::PxPrismaticJointGeneratedInfo()
	: Position( "Position", getPxPrismaticJoint_Position)
	, Velocity( "Velocity", getPxPrismaticJoint_Velocity)
	, Limit( "Limit", setPxPrismaticJoint_Limit, getPxPrismaticJoint_Limit)
	, PrismaticJointFlags( "PrismaticJointFlags", setPxPrismaticJoint_PrismaticJointFlags, getPxPrismaticJoint_PrismaticJointFlags)
	, ConcreteTypeName( "ConcreteTypeName", getPxPrismaticJoint_ConcreteTypeName)
{}
 PxPrismaticJointGeneratedValues::PxPrismaticJointGeneratedValues( const PxPrismaticJoint* inSource )
		:PxJointGeneratedValues( inSource )
		,Position( getPxPrismaticJoint_Position( inSource ) )
		,Velocity( getPxPrismaticJoint_Velocity( inSource ) )
		,Limit( getPxPrismaticJoint_Limit( inSource ) )
		,PrismaticJointFlags( getPxPrismaticJoint_PrismaticJointFlags( inSource ) )
		,ConcreteTypeName( getPxPrismaticJoint_ConcreteTypeName( inSource ) )
{
	PX_UNUSED(inSource);
}
PxReal getPxRevoluteJoint_Angle( const PxRevoluteJoint* inObj ) { return inObj->getAngle(); }
PxReal getPxRevoluteJoint_Velocity( const PxRevoluteJoint* inObj ) { return inObj->getVelocity(); }
void setPxRevoluteJoint_Limit( PxRevoluteJoint* inObj, const PxJointAngularLimitPair & inArg){ inObj->setLimit( inArg ); }
PxJointAngularLimitPair getPxRevoluteJoint_Limit( const PxRevoluteJoint* inObj ) { return inObj->getLimit(); }
void setPxRevoluteJoint_DriveVelocity( PxRevoluteJoint* inObj, PxReal inArg){ inObj->setDriveVelocity( inArg ); }
PxReal getPxRevoluteJoint_DriveVelocity( const PxRevoluteJoint* inObj ) { return inObj->getDriveVelocity(); }
void setPxRevoluteJoint_DriveForceLimit( PxRevoluteJoint* inObj, PxReal inArg){ inObj->setDriveForceLimit( inArg ); }
PxReal getPxRevoluteJoint_DriveForceLimit( const PxRevoluteJoint* inObj ) { return inObj->getDriveForceLimit(); }
void setPxRevoluteJoint_DriveGearRatio( PxRevoluteJoint* inObj, PxReal inArg){ inObj->setDriveGearRatio( inArg ); }
PxReal getPxRevoluteJoint_DriveGearRatio( const PxRevoluteJoint* inObj ) { return inObj->getDriveGearRatio(); }
void setPxRevoluteJoint_RevoluteJointFlags( PxRevoluteJoint* inObj, PxRevoluteJointFlags inArg){ inObj->setRevoluteJointFlags( inArg ); }
PxRevoluteJointFlags getPxRevoluteJoint_RevoluteJointFlags( const PxRevoluteJoint* inObj ) { return inObj->getRevoluteJointFlags(); }
const char * getPxRevoluteJoint_ConcreteTypeName( const PxRevoluteJoint* inObj ) { return inObj->getConcreteTypeName(); }
 PxRevoluteJointGeneratedInfo::PxRevoluteJointGeneratedInfo()
	: Angle( "Angle", getPxRevoluteJoint_Angle)
	, Velocity( "Velocity", getPxRevoluteJoint_Velocity)
	, Limit( "Limit", setPxRevoluteJoint_Limit, getPxRevoluteJoint_Limit)
	, DriveVelocity( "DriveVelocity", setPxRevoluteJoint_DriveVelocity, getPxRevoluteJoint_DriveVelocity)
	, DriveForceLimit( "DriveForceLimit", setPxRevoluteJoint_DriveForceLimit, getPxRevoluteJoint_DriveForceLimit)
	, DriveGearRatio( "DriveGearRatio", setPxRevoluteJoint_DriveGearRatio, getPxRevoluteJoint_DriveGearRatio)
	, RevoluteJointFlags( "RevoluteJointFlags", setPxRevoluteJoint_RevoluteJointFlags, getPxRevoluteJoint_RevoluteJointFlags)
	, ConcreteTypeName( "ConcreteTypeName", getPxRevoluteJoint_ConcreteTypeName)
{}
 PxRevoluteJointGeneratedValues::PxRevoluteJointGeneratedValues( const PxRevoluteJoint* inSource )
		:PxJointGeneratedValues( inSource )
		,Angle( getPxRevoluteJoint_Angle( inSource ) )
		,Velocity( getPxRevoluteJoint_Velocity( inSource ) )
		,Limit( getPxRevoluteJoint_Limit( inSource ) )
		,DriveVelocity( getPxRevoluteJoint_DriveVelocity( inSource ) )
		,DriveForceLimit( getPxRevoluteJoint_DriveForceLimit( inSource ) )
		,DriveGearRatio( getPxRevoluteJoint_DriveGearRatio( inSource ) )
		,RevoluteJointFlags( getPxRevoluteJoint_RevoluteJointFlags( inSource ) )
		,ConcreteTypeName( getPxRevoluteJoint_ConcreteTypeName( inSource ) )
{
	PX_UNUSED(inSource);
}
void setPxSphericalJoint_LimitCone( PxSphericalJoint* inObj, const PxJointLimitCone & inArg){ inObj->setLimitCone( inArg ); }
PxJointLimitCone getPxSphericalJoint_LimitCone( const PxSphericalJoint* inObj ) { return inObj->getLimitCone(); }
PxReal getPxSphericalJoint_SwingYAngle( const PxSphericalJoint* inObj ) { return inObj->getSwingYAngle(); }
PxReal getPxSphericalJoint_SwingZAngle( const PxSphericalJoint* inObj ) { return inObj->getSwingZAngle(); }
void setPxSphericalJoint_SphericalJointFlags( PxSphericalJoint* inObj, PxSphericalJointFlags inArg){ inObj->setSphericalJointFlags( inArg ); }
PxSphericalJointFlags getPxSphericalJoint_SphericalJointFlags( const PxSphericalJoint* inObj ) { return inObj->getSphericalJointFlags(); }
const char * getPxSphericalJoint_ConcreteTypeName( const PxSphericalJoint* inObj ) { return inObj->getConcreteTypeName(); }
 PxSphericalJointGeneratedInfo::PxSphericalJointGeneratedInfo()
	: LimitCone( "LimitCone", setPxSphericalJoint_LimitCone, getPxSphericalJoint_LimitCone)
	, SwingYAngle( "SwingYAngle", getPxSphericalJoint_SwingYAngle)
	, SwingZAngle( "SwingZAngle", getPxSphericalJoint_SwingZAngle)
	, SphericalJointFlags( "SphericalJointFlags", setPxSphericalJoint_SphericalJointFlags, getPxSphericalJoint_SphericalJointFlags)
	, ConcreteTypeName( "ConcreteTypeName", getPxSphericalJoint_ConcreteTypeName)
{}
 PxSphericalJointGeneratedValues::PxSphericalJointGeneratedValues( const PxSphericalJoint* inSource )
		:PxJointGeneratedValues( inSource )
		,LimitCone( getPxSphericalJoint_LimitCone( inSource ) )
		,SwingYAngle( getPxSphericalJoint_SwingYAngle( inSource ) )
		,SwingZAngle( getPxSphericalJoint_SwingZAngle( inSource ) )
		,SphericalJointFlags( getPxSphericalJoint_SphericalJointFlags( inSource ) )
		,ConcreteTypeName( getPxSphericalJoint_ConcreteTypeName( inSource ) )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxJointLimitParametersRestitution( const PxJointLimitParameters* inOwner ) { return inOwner->restitution; }
inline void setPxJointLimitParametersRestitution( PxJointLimitParameters* inOwner, PxReal inData) { inOwner->restitution = inData; }
inline PxReal getPxJointLimitParametersBounceThreshold( const PxJointLimitParameters* inOwner ) { return inOwner->bounceThreshold; }
inline void setPxJointLimitParametersBounceThreshold( PxJointLimitParameters* inOwner, PxReal inData) { inOwner->bounceThreshold = inData; }
inline PxReal getPxJointLimitParametersStiffness( const PxJointLimitParameters* inOwner ) { return inOwner->stiffness; }
inline void setPxJointLimitParametersStiffness( PxJointLimitParameters* inOwner, PxReal inData) { inOwner->stiffness = inData; }
inline PxReal getPxJointLimitParametersDamping( const PxJointLimitParameters* inOwner ) { return inOwner->damping; }
inline void setPxJointLimitParametersDamping( PxJointLimitParameters* inOwner, PxReal inData) { inOwner->damping = inData; }
 PxJointLimitParametersGeneratedInfo::PxJointLimitParametersGeneratedInfo()
	: Restitution( "Restitution", setPxJointLimitParametersRestitution, getPxJointLimitParametersRestitution )
	, BounceThreshold( "BounceThreshold", setPxJointLimitParametersBounceThreshold, getPxJointLimitParametersBounceThreshold )
	, Stiffness( "Stiffness", setPxJointLimitParametersStiffness, getPxJointLimitParametersStiffness )
	, Damping( "Damping", setPxJointLimitParametersDamping, getPxJointLimitParametersDamping )
{}
 PxJointLimitParametersGeneratedValues::PxJointLimitParametersGeneratedValues( const PxJointLimitParameters* inSource )
		:Restitution( inSource->restitution )
		,BounceThreshold( inSource->bounceThreshold )
		,Stiffness( inSource->stiffness )
		,Damping( inSource->damping )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxJointLinearLimitValue( const PxJointLinearLimit* inOwner ) { return inOwner->value; }
inline void setPxJointLinearLimitValue( PxJointLinearLimit* inOwner, PxReal inData) { inOwner->value = inData; }
 PxJointLinearLimitGeneratedInfo::PxJointLinearLimitGeneratedInfo()
	: Value( "Value", setPxJointLinearLimitValue, getPxJointLinearLimitValue )
{}
 PxJointLinearLimitGeneratedValues::PxJointLinearLimitGeneratedValues( const PxJointLinearLimit* inSource )
		:PxJointLimitParametersGeneratedValues( inSource )
		,Value( inSource->value )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxJointLinearLimitPairUpper( const PxJointLinearLimitPair* inOwner ) { return inOwner->upper; }
inline void setPxJointLinearLimitPairUpper( PxJointLinearLimitPair* inOwner, PxReal inData) { inOwner->upper = inData; }
inline PxReal getPxJointLinearLimitPairLower( const PxJointLinearLimitPair* inOwner ) { return inOwner->lower; }
inline void setPxJointLinearLimitPairLower( PxJointLinearLimitPair* inOwner, PxReal inData) { inOwner->lower = inData; }
 PxJointLinearLimitPairGeneratedInfo::PxJointLinearLimitPairGeneratedInfo()
	: Upper( "Upper", setPxJointLinearLimitPairUpper, getPxJointLinearLimitPairUpper )
	, Lower( "Lower", setPxJointLinearLimitPairLower, getPxJointLinearLimitPairLower )
{}
 PxJointLinearLimitPairGeneratedValues::PxJointLinearLimitPairGeneratedValues( const PxJointLinearLimitPair* inSource )
		:PxJointLimitParametersGeneratedValues( inSource )
		,Upper( inSource->upper )
		,Lower( inSource->lower )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxJointAngularLimitPairUpper( const PxJointAngularLimitPair* inOwner ) { return inOwner->upper; }
inline void setPxJointAngularLimitPairUpper( PxJointAngularLimitPair* inOwner, PxReal inData) { inOwner->upper = inData; }
inline PxReal getPxJointAngularLimitPairLower( const PxJointAngularLimitPair* inOwner ) { return inOwner->lower; }
inline void setPxJointAngularLimitPairLower( PxJointAngularLimitPair* inOwner, PxReal inData) { inOwner->lower = inData; }
 PxJointAngularLimitPairGeneratedInfo::PxJointAngularLimitPairGeneratedInfo()
	: Upper( "Upper", setPxJointAngularLimitPairUpper, getPxJointAngularLimitPairUpper )
	, Lower( "Lower", setPxJointAngularLimitPairLower, getPxJointAngularLimitPairLower )
{}
 PxJointAngularLimitPairGeneratedValues::PxJointAngularLimitPairGeneratedValues( const PxJointAngularLimitPair* inSource )
		:PxJointLimitParametersGeneratedValues( inSource )
		,Upper( inSource->upper )
		,Lower( inSource->lower )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxJointLimitConeYAngle( const PxJointLimitCone* inOwner ) { return inOwner->yAngle; }
inline void setPxJointLimitConeYAngle( PxJointLimitCone* inOwner, PxReal inData) { inOwner->yAngle = inData; }
inline PxReal getPxJointLimitConeZAngle( const PxJointLimitCone* inOwner ) { return inOwner->zAngle; }
inline void setPxJointLimitConeZAngle( PxJointLimitCone* inOwner, PxReal inData) { inOwner->zAngle = inData; }
 PxJointLimitConeGeneratedInfo::PxJointLimitConeGeneratedInfo()
	: YAngle( "YAngle", setPxJointLimitConeYAngle, getPxJointLimitConeYAngle )
	, ZAngle( "ZAngle", setPxJointLimitConeZAngle, getPxJointLimitConeZAngle )
{}
 PxJointLimitConeGeneratedValues::PxJointLimitConeGeneratedValues( const PxJointLimitCone* inSource )
		:PxJointLimitParametersGeneratedValues( inSource )
		,YAngle( inSource->yAngle )
		,ZAngle( inSource->zAngle )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxJointLimitPyramidYAngleMin( const PxJointLimitPyramid* inOwner ) { return inOwner->yAngleMin; }
inline void setPxJointLimitPyramidYAngleMin( PxJointLimitPyramid* inOwner, PxReal inData) { inOwner->yAngleMin = inData; }
inline PxReal getPxJointLimitPyramidYAngleMax( const PxJointLimitPyramid* inOwner ) { return inOwner->yAngleMax; }
inline void setPxJointLimitPyramidYAngleMax( PxJointLimitPyramid* inOwner, PxReal inData) { inOwner->yAngleMax = inData; }
inline PxReal getPxJointLimitPyramidZAngleMin( const PxJointLimitPyramid* inOwner ) { return inOwner->zAngleMin; }
inline void setPxJointLimitPyramidZAngleMin( PxJointLimitPyramid* inOwner, PxReal inData) { inOwner->zAngleMin = inData; }
inline PxReal getPxJointLimitPyramidZAngleMax( const PxJointLimitPyramid* inOwner ) { return inOwner->zAngleMax; }
inline void setPxJointLimitPyramidZAngleMax( PxJointLimitPyramid* inOwner, PxReal inData) { inOwner->zAngleMax = inData; }
 PxJointLimitPyramidGeneratedInfo::PxJointLimitPyramidGeneratedInfo()
	: YAngleMin( "YAngleMin", setPxJointLimitPyramidYAngleMin, getPxJointLimitPyramidYAngleMin )
	, YAngleMax( "YAngleMax", setPxJointLimitPyramidYAngleMax, getPxJointLimitPyramidYAngleMax )
	, ZAngleMin( "ZAngleMin", setPxJointLimitPyramidZAngleMin, getPxJointLimitPyramidZAngleMin )
	, ZAngleMax( "ZAngleMax", setPxJointLimitPyramidZAngleMax, getPxJointLimitPyramidZAngleMax )
{}
 PxJointLimitPyramidGeneratedValues::PxJointLimitPyramidGeneratedValues( const PxJointLimitPyramid* inSource )
		:PxJointLimitParametersGeneratedValues( inSource )
		,YAngleMin( inSource->yAngleMin )
		,YAngleMax( inSource->yAngleMax )
		,ZAngleMin( inSource->zAngleMin )
		,ZAngleMax( inSource->zAngleMax )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxSpringStiffness( const PxSpring* inOwner ) { return inOwner->stiffness; }
inline void setPxSpringStiffness( PxSpring* inOwner, PxReal inData) { inOwner->stiffness = inData; }
inline PxReal getPxSpringDamping( const PxSpring* inOwner ) { return inOwner->damping; }
inline void setPxSpringDamping( PxSpring* inOwner, PxReal inData) { inOwner->damping = inData; }
 PxSpringGeneratedInfo::PxSpringGeneratedInfo()
	: Stiffness( "Stiffness", setPxSpringStiffness, getPxSpringStiffness )
	, Damping( "Damping", setPxSpringDamping, getPxSpringDamping )
{}
 PxSpringGeneratedValues::PxSpringGeneratedValues( const PxSpring* inSource )
		:Stiffness( inSource->stiffness )
		,Damping( inSource->damping )
{
	PX_UNUSED(inSource);
}
inline PxReal getPxD6JointDriveForceLimit( const PxD6JointDrive* inOwner ) { return inOwner->forceLimit; }
inline void setPxD6JointDriveForceLimit( PxD6JointDrive* inOwner, PxReal inData) { inOwner->forceLimit = inData; }
inline PxD6JointDriveFlags getPxD6JointDriveFlags( const PxD6JointDrive* inOwner ) { return inOwner->flags; }
inline void setPxD6JointDriveFlags( PxD6JointDrive* inOwner, PxD6JointDriveFlags inData) { inOwner->flags = inData; }
 PxD6JointDriveGeneratedInfo::PxD6JointDriveGeneratedInfo()
	: ForceLimit( "ForceLimit", setPxD6JointDriveForceLimit, getPxD6JointDriveForceLimit )
	, Flags( "Flags", setPxD6JointDriveFlags, getPxD6JointDriveFlags )
{}
 PxD6JointDriveGeneratedValues::PxD6JointDriveGeneratedValues( const PxD6JointDrive* inSource )
		:PxSpringGeneratedValues( inSource )
		,ForceLimit( inSource->forceLimit )
		,Flags( inSource->flags )
{
	PX_UNUSED(inSource);
}
