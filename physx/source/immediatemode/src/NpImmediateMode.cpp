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

#include "PxImmediateMode.h"
#include "PxBroadPhase.h"
#include "../../lowleveldynamics/src/DyBodyCoreIntegrator.h"
#include "../../lowleveldynamics/src/DyContactPrep.h"
#include "../../lowleveldynamics/src/DyCorrelationBuffer.h"
#include "../../lowleveldynamics/src/DyConstraintPrep.h"
#include "../../lowleveldynamics/src/DySolverControl.h"
#include "../../lowleveldynamics/src/DySolverContext.h"
#include "../../lowlevel/common/include/collision/PxcContactMethodImpl.h"
#include "../../lowleveldynamics/src/DyTGSContactPrep.h"
#include "../../lowleveldynamics/src/DyTGS.h"
#include "../../lowleveldynamics/src/DyConstraintPartition.h"
#include "../../lowleveldynamics/src/DyPGS.h"
#include "../../lowleveldynamics/shared/DyCpuGpuArticulation.h"
#include "GuPersistentContactManifold.h"
#include "NpConstraint.h"
#include "common/PxProfileZone.h"

#include "../../lowleveldynamics/include/DyFeatherstoneArticulation.h"

#include "../../lowlevel/common/include/utils/PxcScratchAllocator.h"

using namespace physx;
using namespace Dy;
using namespace immediate;

void immediate::PxConstructSolverBodies(const PxRigidBodyData* inRigidData, PxSolverBodyData* outSolverBodyData, PxU32 nbBodies, const PxVec3& gravity, PxReal dt, bool gyroscopicForces)
{
	PX_ASSERT((size_t(inRigidData) & 0xf) == 0);
	PX_ASSERT((size_t(outSolverBodyData) & 0xf) == 0);

	for(PxU32 a=0; a<nbBodies; a++)
	{
		const PxRigidBodyData& rigidData = inRigidData[a];
		PxVec3 lv = rigidData.linearVelocity, av = rigidData.angularVelocity;
		Dy::bodyCoreComputeUnconstrainedVelocity(gravity, dt, rigidData.linearDamping, rigidData.angularDamping, 1.0f, rigidData.maxLinearVelocitySq, rigidData.maxAngularVelocitySq, lv, av, false);
		Dy::copyToSolverBodyData(lv, av, rigidData.invMass, rigidData.invInertia, rigidData.body2World, -rigidData.maxDepenetrationVelocity, rigidData.maxContactImpulse, 
			PX_INVALID_NODE, PX_MAX_F32, outSolverBodyData[a], 0, dt, gyroscopicForces);
	}
}

void immediate::PxConstructStaticSolverBody(const PxTransform& globalPose, PxSolverBodyData& solverBodyData)
{
	PX_ASSERT((size_t(&solverBodyData) & 0xf) == 0);

	const PxVec3 zero(0.0f);
	Dy::copyToSolverBodyData(zero, zero, 0.0f, zero, globalPose, -PX_MAX_F32, PX_MAX_F32, PX_INVALID_NODE, PX_MAX_F32, solverBodyData, 0, 0.0f, false);
}

void immediate::PxIntegrateSolverBodies(PxSolverBodyData* solverBodyData, PxSolverBody* solverBody, PxVec3* linearMotionVelocity, PxVec3* angularMotionState, PxU32 nbBodiesToIntegrate, PxReal dt)
{
	PX_ASSERT((size_t(solverBodyData) & 0xf) == 0);
	PX_ASSERT((size_t(solverBody) & 0xf) == 0);

	for(PxU32 i=0; i<nbBodiesToIntegrate; ++i)
		Dy::integrateCore(linearMotionVelocity[i], angularMotionState[i], solverBody[i], solverBodyData[i], dt, 0);
}

namespace
{
	// PT: local structure to provide a ctor for the PxArray below, I don't want to make it visible to the regular PhysX code
	struct immArticulationJointCore : Dy::ArticulationJointCore
	{
		immArticulationJointCore() : Dy::ArticulationJointCore(PxTransform(PxIdentity), PxTransform(PxIdentity))
		{
		}
	};

	// PT: this class makes it possible to call the FeatherstoneArticulation protected functions from here.
	class immArticulation : public FeatherstoneArticulation
	{
		public:
														immArticulation(const PxArticulationDataRC& data);
														~immArticulation();

		PX_FORCE_INLINE	void							immSolveInternalConstraints(PxReal dt, PxReal invDt, PxReal elapsedTime, bool velocityIteration, bool isTGS)
														{
															// PT: TODO: revisit the TGS coeff (PX-4516)
															FeatherstoneArticulation::solveInternalConstraints(dt, dt, invDt, velocityIteration, isTGS, elapsedTime, isTGS ? 0.7f : DY_ARTICULATION_PGS_BIAS_COEFFICIENT, false, false); //  pass correct flag value - PX-4744
														}

		PX_FORCE_INLINE	void							immComputeUnconstrainedVelocitiesTGS(PxReal dt, PxReal totalDt, PxReal invDt, PxReal /*invTotalDt*/, const PxVec3& gravity, PxReal invLengthScale)
														{
															mArticulationData.setDt(totalDt);

															FeatherstoneArticulation::computeUnconstrainedVelocitiesInternal(gravity, invLengthScale); // pass perIterationGravity flag here? ->  PX-4744

															setupInternalConstraints(mArticulationData.getLinks(), mArticulationData.getLinkCount(), 
																mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE, mArticulationData, dt, totalDt, invDt, true);
														}

		PX_FORCE_INLINE	void							immComputeUnconstrainedVelocities(PxReal dt, const PxVec3& gravity, PxReal invLengthScale)
														{
															mArticulationData.setDt(dt);

															FeatherstoneArticulation::computeUnconstrainedVelocitiesInternal(gravity, invLengthScale);
															const PxReal invDt = 1.0f/dt;
															setupInternalConstraints(mArticulationData.getLinks(), mArticulationData.getLinkCount(),
																mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE, mArticulationData, dt, dt, invDt, false);
														}

						void							allocate(const PxU32 nbLinks);
						PxU32							addLink(const PxU32 parent, const PxArticulationLinkDataRC& data);
			
						void							complete();

						PxArray<Dy::ArticulationLink>		mLinks;
						PxArray<PxsBodyCore>				mBodyCores;
						PxArray<immArticulationJointCore>	mArticulationJointCores;
						PxArticulationFlags					mFlags;	// PT: PX-1399. Stored in Dy::ArticulationCore for retained mode.

						// PT: quick and dirty version, to improve later
						struct immArticulationLinkDataRC : PxArticulationLinkDataRC
						{
							PxArticulationLinkCookie	parent;
							PxU32						userID;
						};
						PxArray<immArticulationLinkDataRC>	mTemp;
						PxArray<Cm::SpatialVectorF>			mTempDeltaV;

						bool							mImmDirty;
						bool							mJCalcDirty;
		private:
						void							initJointCore(Dy::ArticulationJointCore& core, const PxArticulationJointDataRC& inboundJoint);
	};

	PxU32 createHeaders(PxU32 nbConstraints, const PxU32* PX_RESTRICT constraintsPerPartition, PxU32 maxBatchPartition,
						PxConstraintBatchHeader* PX_RESTRICT outBatchHeaders, const PxSolverConstraintDesc* PX_RESTRICT outOrderedConstraintDescs)
	{
		PxU32 numHeaders = 0;
		PxU32 currentPartition = 0;
		PxU32 maxJ = nbConstraints == 0 ? 0 : constraintsPerPartition[0];

		//printf("\nnbConstraints: %d\n", nbConstraints);

		for (PxU32 a = 0; a < nbConstraints;)
		{
			PxConstraintBatchHeader& header = outBatchHeaders[numHeaders++];
			header.startIndex = a;

			PxU32 loopMax = PxMin(maxJ - a, 4u);
			PxU16 j = 0;
			if (loopMax > 0)
			{
				j = 1;
				const PxSolverConstraintDesc& desc = outOrderedConstraintDescs[a];

				if(isArticulationConstraint(desc))
					loopMax = 1;

				if (currentPartition < maxBatchPartition)
				{
					for (; j < loopMax && desc.constraintType == outOrderedConstraintDescs[a + j].constraintType
						&& !isArticulationConstraint(outOrderedConstraintDescs[a + j]); ++j);
				}
				header.stride = j;
				header.constraintType = desc.constraintType;
			}
			if (maxJ == (a + j) && maxJ != nbConstraints)
			{
				currentPartition++;
				maxJ = constraintsPerPartition[currentPartition];
			}
			a += j;
		}
		return numHeaders;
	}
}

static PxU32 batchConstraints(	const PxSolverConstraintDesc* solverConstraintDescs, PxU32 nbConstraints, PxU8* solverBodies, PxU32 nbBodies,
								PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
								PxArticulationHandle* articulations, PxU32 nbArticulations, PxU32 stride, PxU32 maxPartitions, bool)
{
	PX_ASSERT((size_t(solverBodies) & 0xf) == 0);

	if(!nbConstraints)
		return 0;

	// PT: used to dump data for UTs. Please keep that code around.
/*	if(0)
	{
		FILE* fp = fopen("d:\\tmp\\dump.txt", "w");
		if(fp)
		{
			for(PxU32 i=0;i<nbConstraints;i++)
			{
				const PxU32 indexA = PxU32(PxU64(reinterpret_cast<PxU8*>(solverConstraintDescs[i].bodyA) - solverBodies)/stride);
				const PxU32 indexB = PxU32(PxU64(reinterpret_cast<PxU8*>(solverConstraintDescs[i].bodyB) - solverBodies)/stride);
				fprintf(fp, "%d, %d,\n", indexA, indexB);
			}
			fclose(fp);
		}
	}*/

	const ConstraintPartitionIn in(	solverBodies, nbBodies, stride,
									reinterpret_cast<Dy::FeatherstoneArticulation**>(articulations), nbArticulations,
									solverConstraintDescs, nbConstraints, maxPartitions, true);	// PT: crashes with false!

	// PT: this is really a waste of space for what we use it for. It should be allocated on-the-fly or maybe use indices
	// within the source buffer, which would also be cheaper to copy/reshuffle.
	PxSolverConstraintDesc* overflowConstraintDescriptors = PX_ALLOCATE(PxSolverConstraintDesc, nbConstraints, "overflowConstraintDescriptors");

	PxArray<PxU32> constraintsPerPartition;
	constraintsPerPartition.reserve(128);

	ConstraintPartitionOut out(outOrderedConstraintDescs, overflowConstraintDescriptors, &constraintsPerPartition);

	// PT: maxPartition usually well below 32 but some scenes go far beyond (I've seen a 60....)
	const PxU32 maxPartition = partitionContactConstraints(out, in);
	PX_UNUSED(maxPartition);
	//printf("maxPartition: %d\n", maxPartition);

	//printf("mNumOverflowConstraints: %d\n", out.mNumOverflowConstraints);

	PX_FREE(overflowConstraintDescriptors);

	// PT: we probably cannot reuse the code from the SDK here because it's hardcoded for Dy types in constraintLengthOver16,
	// while imm mode used e.g. PxSolverConstraintDesc::eCONTACT_CONSTRAINT there.
	const PxU32 nbHeaders = createHeaders(nbConstraints, constraintsPerPartition.begin(), maxPartition, outBatchHeaders, outOrderedConstraintDescs);

#ifdef MISSING_CALL
	if(tgs)
	{
		::processOverflowConstraints(reinterpret_cast<PxU8*>(solverBodies), sizeof(PxTGSSolverBodyVel), nbBodies,
			//Dy::ArticulationSolverDesc* articulationDescs, PxU32 numArticulations,
			NULL, 0,
			outOrderedConstraintDescs, out.mNumOverflowConstraints/* ? nbConstraints : 0*/);

//		processOverflowConstraints(reinterpret_cast<PxU8*>(mContext.mSolverBodyVelPool.begin() + mObjects.solverBodyOffset+1), sizeof(PxTGSSolverBodyVel),
//			mCounts.bodies, mThreadContext.getArticulations().begin(), mThreadContext.getArticulations().size(), contactDescBegin,
//			mThreadContext.mHasOverflowPartitions ? mThreadContext.mConstraintsPerPartition[0] : 0);
	}
#endif
	return nbHeaders;
}

PxU32 immediate::PxBatchConstraints(const PxSolverConstraintDesc* solverConstraintDescs, PxU32 nbConstraints, PxSolverBody* solverBodies, PxU32 nbBodies,
									PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
									PxArticulationHandle* articulations, PxU32 nbArticulations)
{
	return batchConstraints(solverConstraintDescs, nbConstraints, reinterpret_cast<PxU8*>(solverBodies), nbBodies,
							outBatchHeaders, outOrderedConstraintDescs, articulations, nbArticulations,
							sizeof(PxSolverBody), PX_MAX_U32, false);
}

PxU32 immediate::PxBatchConstraintsTGS(const PxSolverConstraintDesc* solverConstraintDescs, PxU32 nbConstraints, PxTGSSolverBodyVel* solverBodies, PxU32 nbBodies,
	PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
	PxArticulationHandle* articulations, PxU32 nbArticulations)
{
	// PT: SDK code uses 64 for TGS but in immediate mode it makes the caterpillar track scene explode, most likely because mNumOverflowConstraints = 1024.
	// I think the missing bit is the call to processOverflowConstraints, that we do for TGS in the SDK but not here in immediate mode. Using PX_MAX_U32
	// instead, in immediate mode, fixes the explosion.
	//const PxU32 maxPartitions = 64;
	const PxU32 maxPartitions = PX_MAX_U32;
	return batchConstraints(solverConstraintDescs, nbConstraints, reinterpret_cast<PxU8*>(solverBodies), nbBodies,
							outBatchHeaders, outOrderedConstraintDescs, articulations, nbArticulations,
							sizeof(PxTGSSolverBodyVel), maxPartitions, true);
}

bool immediate::PxCreateContactConstraints(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxSolverContactDesc* contactDescs,
	PxConstraintAllocator& allocator, PxReal invDt, PxReal bounceThreshold, PxReal frictionOffsetThreshold, 
	PxReal correlationDistance, PxSpatialVector* ZV)
{
	PX_ASSERT(invDt > 0.0f && PxIsFinite(invDt));
	PX_ASSERT(bounceThreshold < 0.0f);
	PX_ASSERT(frictionOffsetThreshold > 0.0f);
	PX_ASSERT(correlationDistance > 0.0f);

	Dy::CorrelationBuffer cb;

	PxU32 currentContactDescIdx = 0;

	const PxReal dt = 1.0f / invDt;

	for(PxU32 i=0; i<nbHeaders; ++i)
	{
		Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];
		if (batchHeader.stride == 4)
		{
			const PxSolverContactDesc* ctc = contactDescs + currentContactDescIdx;
			const PxU32 totalContacts =	  ctc[0].numContacts
										+ ctc[1].numContacts
										+ ctc[2].numContacts
										+ ctc[3].numContacts;

			if (totalContacts <= 64)
			{
				state = Dy::createFinalizeSolverContacts4(cb,
					contactDescs + currentContactDescIdx,
					invDt,
					dt,
					bounceThreshold,
					frictionOffsetThreshold,
					correlationDistance,
					allocator);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			Cm::SpatialVectorF* Z = reinterpret_cast<Cm::SpatialVectorF*>(ZV);
			for(PxU32 a=0; a<batchHeader.stride; ++a)
			{
				Dy::createFinalizeSolverContacts(contactDescs[currentContactDescIdx + a], cb, invDt, dt, bounceThreshold, 
					frictionOffsetThreshold, correlationDistance, allocator, Z);
			}
		}

		if(contactDescs[currentContactDescIdx].desc->constraint)
		{
			PxU8 type = *contactDescs[currentContactDescIdx].desc->constraint;

			if(type == DY_SC_TYPE_STATIC_CONTACT)
			{
				//Check if any block of constraints is classified as type static (single) contact constraint.
				//If they are, iterate over all constraints grouped with it and switch to "dynamic" contact constraint
				//type if there's a dynamic contact constraint in the group.
				for(PxU32 c=1; c<batchHeader.stride; ++c)
				{
					if (*contactDescs[currentContactDescIdx + c].desc->constraint == DY_SC_TYPE_RB_CONTACT)
					{
						type = DY_SC_TYPE_RB_CONTACT;
						break;
					}
				}
			}

			batchHeader.constraintType = type;
		}

		currentContactDescIdx += batchHeader.stride;
	}
	return true;
}

bool immediate::PxCreateJointConstraints(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxSolverConstraintPrepDesc* jointDescs, 
	PxConstraintAllocator& allocator, PxSpatialVector* /* ZV */, PxReal dt, PxReal invDt)
{
	PX_ASSERT(dt > 0.0f);
	PX_ASSERT(invDt > 0.0f && PxIsFinite(invDt));

	
	PxU32 currentDescIdx = 0;
	for(PxU32 i=0; i<nbHeaders; ++i)
	{
		Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		PxU8 type = DY_SC_TYPE_BLOCK_1D;
		if (batchHeader.stride == 4)
		{
			PxU32 totalRows = 0;
			PxU32 maxRows = 0;
			bool batchable = true;
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				if (jointDescs[currentDescIdx + a].numRows == 0)
				{
					batchable = false;
					break;
				}
				totalRows += jointDescs[currentDescIdx + a].numRows;
				maxRows = PxMax(maxRows, jointDescs[currentDescIdx + a].numRows);
			}

			if (batchable)
			{
				state = Dy::setupSolverConstraint4
					(jointDescs + currentDescIdx,
					dt, invDt, totalRows,
					allocator, maxRows, false);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			type = DY_SC_TYPE_RB_1D;
			for(PxU32 a=0; a<batchHeader.stride; ++a)
			{
				// PT: TODO: And "isExtended" is already computed in Dy::ConstraintHelper::setupSolverConstraint
				PxSolverConstraintDesc& desc = *jointDescs[currentDescIdx + a].desc;
				const bool isExtended = desc.linkIndexA != PxSolverConstraintDesc::RIGID_BODY || desc.linkIndexB != PxSolverConstraintDesc::RIGID_BODY;
				if(isExtended)
					type = DY_SC_TYPE_EXT_1D;

				Dy::ConstraintHelper::setupSolverConstraint(jointDescs[currentDescIdx + a], allocator, dt, invDt);
			}
		}

		batchHeader.constraintType = type;
		currentDescIdx += batchHeader.stride;
	}

	return true;
}

template<class LeafTestT, class ParamsT>
static bool PxCreateJointConstraintsWithShadersT(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, ParamsT* params, PxSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, PxSpatialVector* Z, PxReal dt, PxReal invDt)
{
	Px1DConstraint allRows[Dy::MAX_CONSTRAINT_ROWS * 4];

	//Runs shaders to fill in rows...

	PxU32 currentDescIdx = 0;

	for(PxU32 i=0; i<nbHeaders; i++)
	{
		Px1DConstraint* rows = allRows;
		Px1DConstraint* rows2 = allRows;

		PxU32 maxRows = 0;
		PxU32 nbToPrep = MAX_CONSTRAINT_ROWS;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		for(PxU32 a=0; a<batchHeader.stride; a++)
		{
			PxSolverConstraintPrepDesc& desc = jointDescs[currentDescIdx + a];

			PxConstraintSolverPrep prep;
			const void* constantBlock;
			const bool useExtendedLimits = LeafTestT::getData(params, currentDescIdx + a, &prep, &constantBlock);
			PX_ASSERT(prep);

			PX_ASSERT(rows2 + nbToPrep <= allRows + MAX_CONSTRAINT_ROWS*4);
			setupConstraintRows(rows2, nbToPrep);
			rows2 += nbToPrep;

			desc.invMassScales.linear0 = desc.invMassScales.linear1 = desc.invMassScales.angular0 = desc.invMassScales.angular1 = 1.0f;
			desc.body0WorldOffset = PxVec3(0.0f);

			PxVec3p unused_cA2w, unused_cB2w;
			//TAG:solverprepcall
			const PxU32 constraintCount = prep(rows,
				desc.body0WorldOffset,
				Dy::MAX_CONSTRAINT_ROWS,
				desc.invMassScales,
				constantBlock,
				desc.bodyFrame0, desc.bodyFrame1, 
				useExtendedLimits,
				unused_cA2w, unused_cB2w);

			nbToPrep = constraintCount;
			maxRows = PxMax(constraintCount, maxRows);

			desc.rows = rows;
			desc.numRows = constraintCount;
			rows += constraintCount;
		}

		PxCreateJointConstraints(&batchHeader, 1, jointDescs + currentDescIdx, allocator, Z, dt, invDt);

		currentDescIdx += batchHeader.stride;
	}
	return true; //KS - TODO - do some error reporting/management...
}

namespace
{
	class PxConstraintAdapter
	{
	public:
		static PX_FORCE_INLINE bool getData(PxConstraint** constraints, PxU32 i, PxConstraintSolverPrep* prep, const void** constantBlock)
		{
			NpConstraint* npConstraint = static_cast<NpConstraint*>(constraints[i]);
			const Sc::ConstraintCore& core = npConstraint->getCore();

			if (npConstraint->isDirty())
			{
				core.getPxConnector()->prepareData();
				npConstraint->markClean();
			}

			*prep = core.getPxConnector()->getPrep();
			*constantBlock = core.getPxConnector()->getConstantBlock();
			return core.getFlags() & PxConstraintFlag::eENABLE_EXTENDED_LIMITS;
		}
	};
}

bool immediate::PxCreateJointConstraintsWithShaders(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxConstraint** constraints, PxSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, PxReal dt, PxReal invDt, PxSpatialVector* Z)
{
	return PxCreateJointConstraintsWithShadersT<PxConstraintAdapter>(batchHeaders, nbHeaders, constraints, jointDescs, allocator, Z, dt, invDt);
}

bool immediate::PxCreateJointConstraintsWithImmediateShaders(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxImmediateConstraint* constraints, PxSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, PxReal dt, PxReal invDt, PxSpatialVector* Z)
{
	class immConstraintAdapter
	{
	public:
		static PX_FORCE_INLINE bool getData(PxImmediateConstraint* constraints_, PxU32 i, PxConstraintSolverPrep* prep, const void** constantBlock)
		{
			const PxImmediateConstraint& ic = constraints_[i];
			*prep = ic.prep;
			*constantBlock = ic.constantBlock;
			return false;
		}
	};

	return PxCreateJointConstraintsWithShadersT<immConstraintAdapter>(batchHeaders, nbHeaders, constraints, jointDescs, allocator, Z, dt, invDt);
}

/*static*/ PX_FORCE_INLINE bool PxIsZero(const PxSolverBody* bodies, PxU32 nbBodies)
{
	for(PxU32 i=0; i<nbBodies; i++)
	{
		if(	!bodies[i].linearVelocity.isZero() ||
			!bodies[i].angularState.isZero())
			return false;
	}
	return true;
}

void immediate::PxSolveConstraints(const PxConstraintBatchHeader* batchHeaders, PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
	const PxSolverBody* solverBodies, PxVec3* linearMotionVelocity, PxVec3* angularMotionVelocity, PxU32 nbSolverBodies, PxU32 nbPositionIterations, PxU32 nbVelocityIterations,
	float dt, float invDt, PxU32 nbSolverArticulations, PxArticulationHandle* solverArticulations, PxSpatialVector* /* pxZ */, PxSpatialVector* pxDeltaV)
{
#ifdef TO_REVISIT
	if(0)
	{
		// PT:
		// - problem 1: why do we need bodyDataList? => this is actually the same solverBodyArray from the SolverContext so we already encountered that one.
		// Can be null because we don't do "writeBacks" here.
		// - problem 2: the articulation descs
		// - problem 3: spatial vector (same problem as for reusing saveMotionVelocities)
		// Honestly I don't understand:
		// - PxSpatialVector / SpatialVector / SpatialVectorF / UnAlignedSpatialVector / SpatialVectorV
		SolverIslandParams params;
		params.positionIterations		= nbPositionIterations;
		params.velocityIterations		= nbVelocityIterations;
		params.bodyListStart			= solverBodies;
		params.bodyDataList				= NULL;
		params.bodyListSize				= nbSolverBodies;
		params.articulationListStart	= NULL;
		params.articulationListSize		= 0;
		params.constraintList			= solverConstraintDescs;
		params.constraintBatchHeaders	= batchHeaders;
		params.numConstraintHeaders		= nbBatchHeaders;
		params.dt						= dt;
		params.invDt					= invDt;
		params.thresholdStream			= NULL;
		params.thresholdStreamLength	= 0;
		params.outThresholdPairs		= NULL;
		params.Z						= NULL;//pxZ;
		params.deltaV					= NULL;//pxDeltaV;

		Cm::SpatialVector* spatialVectors = PX_ALLOCATE(Cm::SpatialVector, nbSolverBodies, "");
		params.motionVelocityArray	= spatialVectors;
		for(PxU32 i=0;i<nbSolverBodies;i++)
		{
			spatialVectors[i].linear = linearMotionVelocity[i];
			spatialVectors[i].angular = angularMotionVelocity[i];
		}

		SolverCoreGeneral solver(false);
		//solver.solveV_Blocks(params);

/*				params.headersPerPartition = mThreadContext.mConstraintsPerPartition.begin();
				params.nbPartitions = mThreadContext.mConstraintsPerPartition.size();
				const PxU32 unrollSize = 8;
				const PxU32 denom = PxMax(1u, (mThreadContext.mMaxPartitions*unrollSize));
				const PxU32 MaxTasks = getTaskManager()->getCpuDispatcher()->getWorkerCount();
				// PT: small improvement: if there's no contacts, use the number of bodies instead.
				// That way the integration work still benefits from multiple tasks.
				const PxU32 numWorkItems = mThreadContext.numContactConstraintBatches ? mThreadContext.numContactConstraintBatches : mIslandContext.mCounts.bodies;
				const PxU32 idealThreads = (numWorkItems+denom-1)/denom;
				const PxU32 numTasks = PxMax(1u, PxMin(idealThreads, MaxTasks));
				
				if(numTasks > 1)
				{
					const PxU32 idealBatchSize = PxMax(unrollSize, idealThreads*unrollSize/(numTasks*2));
					params.batchSize = idealBatchSize; //assigning ideal batch size for the solver to grab work at. Only needed by the multi-threaded island solver.
*/

		params.headersPerPartition;
		params.nbPartitions;
		params.batchSize;
		solver.solveVParallelAndWriteBack(params, NULL, NULL);

		for(PxU32 i=0;i<nbSolverBodies;i++)
		{
			linearMotionVelocity[i] = spatialVectors[i].linear;
			angularMotionVelocity[i] = spatialVectors[i].angular;
		}

		PX_FREE(spatialVectors);

		return;
	}
#endif

	PX_ASSERT(nbPositionIterations > 0);
	PX_ASSERT(nbVelocityIterations > 0);
	PX_ASSERT(PxIsZero(solverBodies, nbSolverBodies)); //Ensure that solver body velocities have been zeroed before solving
	PX_ASSERT((size_t(solverBodies) & 0xf) == 0);

	const Dy::SolveBlockMethod* solveTable = Dy::gVTableSolveBlock;
	const Dy::SolveBlockMethod* solveConcludeTable = Dy::gVTableSolveConcludeBlock;
	const Dy::SolveWriteBackBlockMethod* solveWritebackTable = Dy::gVTableSolveWriteBackBlock;

	Dy::SolverContext cache;
	cache.solverBodyArray = NULL;
	cache.mThresholdStreamIndex = 0;
	cache.mThresholdStreamLength = 0xFFFFFFF;
		
	PX_ASSERT(nbPositionIterations > 0);
	PX_ASSERT(nbVelocityIterations > 0);

	Cm::SpatialVectorF* deltaV = reinterpret_cast<Cm::SpatialVectorF*>(pxDeltaV);
	cache.deltaV = deltaV;

	Dy::FeatherstoneArticulation** articulations = reinterpret_cast<Dy::FeatherstoneArticulation**>(solverArticulations);

	struct PGS
	{
		static PX_FORCE_INLINE void solveArticulationInternalConstraints(float dt_, float invDt_, PxU32 nbSolverArticulations_, Dy::FeatherstoneArticulation** solverArticulations_, bool velIter_)
		{
			while(nbSolverArticulations_--)
			{
				immArticulation* immArt = static_cast<immArticulation*>(*solverArticulations_++);
				immArt->immSolveInternalConstraints(dt_, invDt_, 0.0f, velIter_, false);
			}
		}

		static PX_FORCE_INLINE void runIter(const PxConstraintBatchHeader* batchHeaders_, PxU32 nbBatchHeaders_, const PxSolverConstraintDesc* solverConstraintDescs_,
											PxU32 nbSolverArticulations_, Dy::FeatherstoneArticulation** articulations_,
											const Dy::SolveBlockMethod* solveTable_, Dy::SolverContext& solverCache_, float dt_, float invDt_, bool doFriction, bool velIter_)
		{
			solverCache_.doFriction = doFriction;
			for(PxU32 a=0; a<nbBatchHeaders_; ++a)
			{
				const PxConstraintBatchHeader& batch = batchHeaders_[a];
				solveTable_[batch.constraintType](solverConstraintDescs_ + batch.startIndex, batch.stride, solverCache_);
			}

			solveArticulationInternalConstraints(dt_, invDt_, nbSolverArticulations_, articulations_, velIter_);
		}
	};

	cache.isPositionIteration = true;
	for(PxU32 i=nbPositionIterations; i>1; --i)
		PGS::runIter(batchHeaders, nbBatchHeaders, solverConstraintDescs, nbSolverArticulations, articulations, solveTable, cache, dt, invDt, i <= 3, false);
	PGS::runIter(batchHeaders, nbBatchHeaders, solverConstraintDescs, nbSolverArticulations, articulations, solveConcludeTable, cache, dt, invDt, true, false);

	//Save motion velocities...
	for(PxU32 a=0; a<nbSolverBodies; a++)
	{
		linearMotionVelocity[a] = solverBodies[a].linearVelocity;
		angularMotionVelocity[a] = solverBodies[a].angularState;
	}

	for(PxU32 a=0; a<nbSolverArticulations; a++)
		FeatherstoneArticulation::saveVelocity(reinterpret_cast<Dy::FeatherstoneArticulation*>(solverArticulations[a]), deltaV);

	cache.isPositionIteration = false;
	for(PxU32 i=nbVelocityIterations; i>1; --i)
		PGS::runIter(batchHeaders, nbBatchHeaders, solverConstraintDescs, nbSolverArticulations, articulations, solveTable, cache, dt, invDt, true, true);
	PGS::runIter(batchHeaders, nbBatchHeaders, solverConstraintDescs, nbSolverArticulations, articulations, solveWritebackTable, cache, dt, invDt, true, true);
}

static void createCache(Gu::Cache& cache, PxGeometryType::Enum geomType0, PxGeometryType::Enum geomType1, PxCacheAllocator& allocator)
{	
	if(gEnablePCMCaching[geomType0][geomType1])
	{
		if(geomType0 <= PxGeometryType::eCONVEXMESH && geomType1 <= PxGeometryType::eCONVEXMESH)
		{
			if(geomType0 == PxGeometryType::eSPHERE || geomType1 == PxGeometryType::eSPHERE)
			{
				Gu::PersistentContactManifold* manifold = PX_PLACEMENT_NEW(allocator.allocateCacheData(sizeof(Gu::SpherePersistentContactManifold)), Gu::SpherePersistentContactManifold)();
				cache.setManifold(manifold);
			}
			else
			{
				Gu::PersistentContactManifold* manifold = PX_PLACEMENT_NEW(allocator.allocateCacheData(sizeof(Gu::LargePersistentContactManifold)), Gu::LargePersistentContactManifold)();
				cache.setManifold(manifold);
			}
			cache.getManifold().clearManifold();
		}
		else
		{
			//ML: raised 1 to indicate the manifold is multiManifold which is for contact gen in mesh/height field
			//cache.manifold = 1;
			cache.setMultiManifold(NULL);
		}
	}
	else
	{
		//cache.manifold =  0;
		cache.mCachedData = NULL;
		cache.mManifoldFlags = 0;
	}
}

bool immediate::PxGenerateContacts(	const PxGeometry* const * geom0, const PxGeometry* const * geom1, const PxTransform* pose0, const PxTransform* pose1, PxCache* contactCache, PxU32 nbPairs,
									PxContactRecorder& contactRecorder, PxReal contactDistance, PxReal meshContactMargin, PxReal toleranceLength, PxCacheAllocator& allocator)
{
	PX_ASSERT(meshContactMargin > 0.0f);
	PX_ASSERT(toleranceLength > 0.0f);
	PX_ASSERT(contactDistance > 0.0f);
	PxContactBuffer contactBuffer;

	const Gu::NarrowPhaseParams params(contactDistance, meshContactMargin, toleranceLength);

	PxTransform32 transform0;
	PxTransform32 transform1;
	for(PxU32 i=0; i<nbPairs; i++)
	{
		contactBuffer.count = 0;
		PxGeometryType::Enum type0 = geom0[i]->getType();
		PxGeometryType::Enum type1 = geom1[i]->getType();

		const PxGeometry* tempGeom0;
		const PxGeometry* tempGeom1;

		const bool bSwap = type0 > type1;
		if(bSwap)
		{
			PxSwap(type0, type1);

			tempGeom1 = geom0[i];
			tempGeom0 = geom1[i];

			transform1 = pose0[i];
			transform0 = pose1[i];
		}
		else
		{
			tempGeom0 = geom0[i];
			tempGeom1 = geom1[i];

			transform0 = pose0[i];
			transform1 = pose1[i];
		}

		//Now work out which type of PCM we need...

		Gu::Cache& cache = static_cast<Gu::Cache&>(contactCache[i]);

		const bool needsMultiManifold = type1 > PxGeometryType::eCONVEXMESH;

		if(needsMultiManifold)
		{
			Gu::MultiplePersistentContactManifold multiManifold;

			if(cache.isMultiManifold())
			{
				multiManifold.fromBuffer(cache.mCachedData);
			}
			else
			{
				multiManifold.initialize();
			}
			cache.setMultiManifold(&multiManifold);

			//Do collision detection, then write manifold out...
			g_PCMContactMethodTable[type0][type1](*tempGeom0, *tempGeom1, transform0, transform1, params, cache, contactBuffer, NULL);

			const PxU32 size = (sizeof(Gu::MultiPersistentManifoldHeader) +
				multiManifold.mNumManifolds * sizeof(Gu::SingleManifoldHeader) +
				multiManifold.mNumTotalContacts * sizeof(Gu::CachedMeshPersistentContact));

			PxU8* buffer = allocator.allocateCacheData(size);

			multiManifold.toBuffer(buffer);

			cache.setMultiManifold(buffer);
		}
		else
		{
			//Allocate the type of manifold we need again...
			Gu::PersistentContactManifold* oldManifold = NULL;

			if(cache.isManifold())
				oldManifold = &cache.getManifold();

			//Allocates and creates the PCM...
			createCache(cache, type0, type1, allocator);

			//Copy PCM from old to new manifold...
			if(oldManifold)
			{
				Gu::PersistentContactManifold& manifold = cache.getManifold();
				manifold.mRelativeTransform = oldManifold->mRelativeTransform;
				manifold.mQuatA = oldManifold->mQuatA;
				manifold.mQuatB = oldManifold->mQuatB;
				manifold.mNumContacts = oldManifold->mNumContacts;
				manifold.mNumWarmStartPoints = oldManifold->mNumWarmStartPoints;
				manifold.mAIndice[0] = oldManifold->mAIndice[0]; manifold.mAIndice[1] = oldManifold->mAIndice[1];
				manifold.mAIndice[2] = oldManifold->mAIndice[2]; manifold.mAIndice[3] = oldManifold->mAIndice[3];
				manifold.mBIndice[0] = oldManifold->mBIndice[0]; manifold.mBIndice[1] = oldManifold->mBIndice[1];
				manifold.mBIndice[2] = oldManifold->mBIndice[2]; manifold.mBIndice[3] = oldManifold->mBIndice[3];
				PxMemCopy(manifold.mContactPoints, oldManifold->mContactPoints, sizeof(Gu::PersistentContact)*manifold.mNumContacts);
			}

			g_PCMContactMethodTable[type0][type1](*tempGeom0, *tempGeom1, transform0, transform1, params, cache, contactBuffer, NULL);
		}

		if(contactBuffer.count)
		{
			if(bSwap)
			{
				for(PxU32 a=0; a<contactBuffer.count; a++)
					contactBuffer.contacts[a].normal = -contactBuffer.contacts[a].normal;
			}

			//Record this contact pair...
			contactRecorder.recordContacts(contactBuffer.contacts, contactBuffer.count, i);
		}
	}
	return true;
}

immArticulation::immArticulation(const PxArticulationDataRC& data) :
	FeatherstoneArticulation(this),
	mFlags					(data.flags),
	mImmDirty				(true),
	mJCalcDirty				(true)
{
	// PT: TODO: we only need the flags here, maybe drop the solver desc?
	getSolverDesc().initData(NULL, &mFlags);
}

immArticulation::~immArticulation()
{
}

void immArticulation::initJointCore(Dy::ArticulationJointCore& core, const PxArticulationJointDataRC& inboundJoint)
{
	core.init(inboundJoint.parentPose, inboundJoint.childPose);

	core.jCalcUpdateFrames =  true;

	const PxU32* binP = reinterpret_cast<const PxU32*>(inboundJoint.targetPos);
	const PxU32* binV = reinterpret_cast<const PxU32*>(inboundJoint.targetVel);

	for(PxU32 i=0; i<PxArticulationAxis::eCOUNT; i++)
	{
		core.setLimit(PxArticulationAxis::Enum(i), inboundJoint.limits[i]);
		core.setDrive(PxArticulationAxis::Enum(i), inboundJoint.drives[i]);
		core.setMaxJointVelocity(inboundJoint.maxJointVelocity[i]);

		// See Sc::ArticulationJointCore::setTargetP and Sc::ArticulationJointCore::setTargetV
		if(binP[i]!=0xffffffff)
		{
			core.targetP[i] = inboundJoint.targetPos[i];
		}
		if(binV[i]!=0xffffffff)
		{
			core.targetV[i] = inboundJoint.targetVel[i];
		}
		core.armature[i] = inboundJoint.armature[i];
		core.jointPos[i] = inboundJoint.jointPos[i];
		core.jointVel[i] = inboundJoint.jointVel[i];
		core.motion[i] = PxU8(inboundJoint.motion[i]);
	}

	core.setFrictionCoefficient(inboundJoint.frictionCoefficient);
	core.setJointType(inboundJoint.type);
}

void immArticulation::allocate(const PxU32 nbLinks)
{
	mLinks.reserve(nbLinks);
	mBodyCores.resize(nbLinks);
	mArticulationJointCores.resize(nbLinks);
}

PxU32 immArticulation::addLink(const PxU32 parentIndex, const PxArticulationLinkDataRC& data)
{
	PX_ASSERT(data.pose.p.isFinite());
	PX_ASSERT(data.pose.q.isFinite());

	mImmDirty = true;
	mJCalcDirty = true;

	// Replicate ArticulationSim::addBody
	addBody();

	const PxU32 index = mLinks.size();

	const PxTransform& bodyPose = data.pose;
	//
	PxsBodyCore* bodyCore = &mBodyCores[index];
	{
		// PT: this function inits everything but we only need a fraction of the data there for articulations
		bodyCore->init(	bodyPose, data.inverseInertia, data.inverseMass, 0.0f, 0.0f,
						data.linearDamping, data.angularDamping,
						data.maxLinearVelocitySq, data.maxAngularVelocitySq, PxActorType::eARTICULATION_LINK);

		// PT: TODO: consider exposing all used data to immediate mode API (PX-1398)
//		bodyCore->maxPenBias			= -1e32f;	// <= this one is related to setMaxDepenetrationVelocity
//		bodyCore->linearVelocity		= PxVec3(0.0f);
//		bodyCore->angularVelocity		= PxVec3(0.0f);
//		bodyCore->linearVelocity		= PxVec3(0.0f, 10.0f, 0.0f);
//		bodyCore->angularVelocity		= PxVec3(0.0f, 10.0f, 0.0f);
		bodyCore->cfmScale = data.cfmScale;
	}

/*	PX_ASSERT((((index==0) && (joint == 0)) && (parent == 0)) ||
				(((index!=0) && joint) && (parent && (parent->getArticulation() == this))));*/

	// PT: TODO: add ctors everywhere
	ArticulationLink& link = mLinks.insert();

	// void BodySim::postActorFlagChange(PxU32 oldFlags, PxU32 newFlags)
	bodyCore->disableGravity	= data.disableGravity;
	link.bodyCore				= bodyCore;
	link.mPathToRootStartIndex	= 0;
	link.mPathToRootCount		= 0;
	link.mChildrenStartIndex	= 0xffffffff;
	link.mNumChildren			= 0;

	const bool isRoot = parentIndex==0xffffffff;
	if(!isRoot)
	{
		link.parent = parentIndex;
		link.inboundJoint = &mArticulationJointCores[index];

		ArticulationLink& parentLink = mLinks[parentIndex];

		if(parentLink.mChildrenStartIndex == 0xffffffff)
			parentLink.mChildrenStartIndex = index;

		parentLink.mNumChildren++;

		initJointCore(*link.inboundJoint, data.inboundJoint);
	}
	else
	{
		link.parent = DY_ARTICULATION_LINK_NONE;
		link.inboundJoint = NULL;
	}
	
	return index;
}

void immArticulation::complete()
{
	// Based on Sc::ArticulationSim::checkResize()

	if(!mImmDirty)
		return;
	mImmDirty = false;

	const PxU32 linkSize = mLinks.size();
	setupLinks(linkSize, const_cast<ArticulationLink*>(mLinks.begin()));
	jcalc<true>(mArticulationData);
	mJCalcDirty = false;
	initPathToRoot();

	mTempDeltaV.resize(linkSize);
}

PxArticulationCookie immediate::PxBeginCreateArticulationRC(const PxArticulationDataRC& data)
{
	// PT: we create the same class as before under the hood, we just don't tell users yet. Returning a void pointer/cookie
	// means we can prevent them from using the articulation before it's fully completed. We do this because we're going to
	// delay the link creation, so we don't want them to call PxAddArticulationLink and expect the link to be here already.
	void* memory = PxAlignedAllocator<64>().allocate(sizeof(immArticulation), PX_FL);
	PX_PLACEMENT_NEW(memory, immArticulation(data));
	return memory;
}

PxArticulationLinkCookie immediate::PxAddArticulationLink(PxArticulationCookie articulation, const PxArticulationLinkCookie* parent, const PxArticulationLinkDataRC& data)
{
	if(!articulation)
		return PxCreateArticulationLinkCookie();

	immArticulation* immArt = reinterpret_cast<immArticulation*>(articulation);

	const PxU32 id = immArt->mTemp.size();

	// PT: TODO: this is the quick-and-dirty version, we could try something smarter where we don't just batch everything like barbarians
	immArticulation::immArticulationLinkDataRC tmp;
	static_cast<PxArticulationLinkDataRC&>(tmp) = data;
	tmp.userID = id;
	tmp.parent = parent ? *parent : PxCreateArticulationLinkCookie();
	immArt->mTemp.pushBack(tmp);

	// WARNING: cannot be null, snippet uses null for regular rigid bodies (non articulation links)
	return PxCreateArticulationLinkCookie(articulation, id);
}

PxArticulationHandle immediate::PxEndCreateArticulationRC(PxArticulationCookie articulation, PxArticulationLinkHandle* handles, PxU32 bufferSize)
{
	if(!articulation)
		return NULL;

	immArticulation* immArt = reinterpret_cast<immArticulation*>(articulation);

	PxU32 nbLinks = immArt->mTemp.size();
	if(nbLinks!=bufferSize)
		return NULL;

	immArticulation::immArticulationLinkDataRC* linkData = immArt->mTemp.begin();

	{
		struct _{ bool operator()(const immArticulation::immArticulationLinkDataRC& data1, const immArticulation::immArticulationLinkDataRC& data2) const
		{
			if(!data1.parent.articulation)
				return true;
			if(!data2.parent.articulation)
				return false;

			return data1.parent.linkId < data2.parent.linkId;
		}};
		PxSort(linkData, nbLinks, _());
	}

	PxMemSet(handles, 0, sizeof(PxArticulationLinkHandle)*nbLinks);

	immArt->allocate(nbLinks);

	while(nbLinks--)
	{
		const PxU32 userID = linkData->userID;
		PxU32 parentID = linkData->parent.linkId;
		
		if(parentID != 0xffffffff)
			parentID = handles[parentID].linkId;
		const PxU32 realID = immArt->addLink(parentID, *linkData);

		handles[userID] = PxArticulationLinkHandle(immArt, realID);

		linkData++;
	}

	immArt->complete();
	return immArt;
}

void immediate::PxReleaseArticulation(PxArticulationHandle articulation)
{
	if(!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	immArt->~immArticulation();
	PxAlignedAllocator<64>().deallocate(articulation);
}

PxArticulationCache* immediate::PxCreateArticulationCache(PxArticulationHandle articulation)
{
	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	immArt->complete();

	return FeatherstoneArticulation::createCache(immArt->getDofs(), immArt->getBodyCount());
}

void immediate::PxCopyInternalStateToArticulationCache(PxArticulationHandle articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag)
{
	immArticulation* immArt = static_cast<immArticulation *>(articulation);
	immArt->copyInternalStateToCache(cache, flag, false);	
}

void immediate::PxApplyArticulationCache(PxArticulationHandle articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag)
{
	bool shouldWake = false;
	immArticulation* immArt = static_cast<immArticulation *>(articulation);
	immArt->applyCache(cache, flag, shouldWake);
}

void immediate::PxReleaseArticulationCache(PxArticulationCache& cache)
{
	PxcScratchAllocator* scratchAlloc = reinterpret_cast<PxcScratchAllocator*>(cache.scratchAllocator);
	PX_DELETE(scratchAlloc);
	cache.scratchAllocator = NULL;

	PX_FREE(cache.scratchMemory);

	PxArticulationCache* ptr = &cache;
	PX_FREE(ptr);
}

void immediate::PxComputeUnconstrainedVelocities(PxArticulationHandle articulation, const PxVec3& gravity, PxReal dt, PxReal invLengthScale)
{
	if(!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	immArt->complete();
	if(immArt->mJCalcDirty)
	{
		immArt->mJCalcDirty = false;
		immArt->jcalc<true>(immArt->mArticulationData);
	}
	immArt->immComputeUnconstrainedVelocities(dt, gravity, invLengthScale);
}

void immediate::PxUpdateArticulationBodies(PxArticulationHandle articulation, PxReal dt)
{
	if(!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);

	FeatherstoneArticulation::updateBodies(immArt, immArt->mTempDeltaV.begin(), dt, true);
}

void immediate::PxComputeUnconstrainedVelocitiesTGS(PxArticulationHandle articulation, const PxVec3& gravity, PxReal dt,
													PxReal totalDt, PxReal invDt, PxReal invTotalDt, PxReal invLengthScale)
{
	if (!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	immArt->complete();
	if(immArt->mJCalcDirty)
	{
		immArt->mJCalcDirty = false;
		immArt->jcalc<true>(immArt->mArticulationData);
	}
	immArt->immComputeUnconstrainedVelocitiesTGS(dt, totalDt, invDt, invTotalDt, gravity, invLengthScale);
}

void immediate::PxUpdateArticulationBodiesTGS(PxArticulationHandle articulation, PxReal dt)
{
	if (!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);

	FeatherstoneArticulation::updateBodies(immArt, immArt->mTempDeltaV.begin(), dt, false);
}

static void copyLinkData(PxArticulationLinkDerivedDataRC& data, const immArticulation* immArt, PxU32 index)
{
	data.pose				= immArt->mBodyCores[index].body2World;
//	data.linearVelocity		= immArt->mBodyCores[index].linearVelocity;
//	data.angularVelocity	= immArt->mBodyCores[index].angularVelocity;
	const Cm::SpatialVectorF& velocity = immArt->getArticulationData().getMotionVelocity(index);
	data.linearVelocity		= velocity.bottom;
	data.angularVelocity	= velocity.top;
}

static PX_FORCE_INLINE const immArticulation* getFromLink(const PxArticulationLinkHandle& link, PxU32& index)
{
	if(!link.articulation)
		return NULL;

	const immArticulation* immArt = static_cast<const immArticulation*>(link.articulation);
	index = link.linkId;

	if(index>=immArt->mLinks.size())
		return NULL;

	return immArt;
}

bool immediate::PxGetLinkData(const PxArticulationLinkHandle& link, PxArticulationLinkDerivedDataRC& data)
{
	PxU32 index;
	const immArticulation* immArt = getFromLink(link, index);
	if(!immArt)
		return false;

	copyLinkData(data, immArt, index);

	return true;
}

PxU32 immediate::PxGetAllLinkData(const PxArticulationHandle articulation, PxArticulationLinkDerivedDataRC* data)
{
	if(!articulation)
		return 0;

	const immArticulation* immArt = static_cast<const immArticulation*>(articulation);

	const PxU32 nb = immArt->mLinks.size();
	if(data)
	{
		for(PxU32 i=0;i<nb;i++)
			copyLinkData(data[i], immArt, i);
	}

	return nb;
}

bool immediate::PxGetMutableLinkData(const PxArticulationLinkHandle& link , PxArticulationLinkMutableDataRC& data)
{
	PxU32 index;
	const immArticulation* immArt = getFromLink(link, index);
	if(!immArt)
		return false;

	data.inverseInertia			= immArt->mBodyCores[index].inverseInertia;
	data.inverseMass			= immArt->mBodyCores[index].inverseMass;
	data.linearDamping			= immArt->mBodyCores[index].linearDamping;
	data.angularDamping			= immArt->mBodyCores[index].angularDamping;
	data.maxLinearVelocitySq	= immArt->mBodyCores[index].maxLinearVelocitySq;
	data.maxAngularVelocitySq	= immArt->mBodyCores[index].maxAngularVelocitySq;
	data.cfmScale				= immArt->mBodyCores[index].cfmScale;
	data.disableGravity			= immArt->mBodyCores[index].disableGravity!=0;

	return true;
}

bool immediate::PxSetMutableLinkData(const PxArticulationLinkHandle& link , const PxArticulationLinkMutableDataRC& data)
{
	PxU32 index;
	immArticulation* immArt = const_cast<immArticulation*>(getFromLink(link, index));
	if(!immArt)
		return false;

	immArt->mBodyCores[index].inverseInertia		= data.inverseInertia;			// See Sc::BodyCore::setInverseInertia
	immArt->mBodyCores[index].inverseMass			= data.inverseMass;				// See Sc::BodyCore::setInverseMass
	immArt->mBodyCores[index].linearDamping			= data.linearDamping;			// See Sc::BodyCore::setLinearDamping
	immArt->mBodyCores[index].angularDamping		= data.angularDamping;			// See Sc::BodyCore::setAngularDamping
	immArt->mBodyCores[index].maxLinearVelocitySq	= data.maxLinearVelocitySq;		// See Sc::BodyCore::setMaxLinVelSq
	immArt->mBodyCores[index].maxAngularVelocitySq	= data.maxAngularVelocitySq;	// See Sc::BodyCore::setMaxAngVelSq
	immArt->mBodyCores[index].cfmScale				= data.cfmScale;				// See Sc::BodyCore::setCfmScale
	immArt->mBodyCores[index].disableGravity		= data.disableGravity;			// See BodySim::postActorFlagChange

	return true;
}

bool immediate::PxGetJointData(const PxArticulationLinkHandle& link, PxArticulationJointDataRC& data)
{
	PxU32 index;
	const immArticulation* immArt = getFromLink(link, index);
	if(!immArt)
		return false;

	const Dy::ArticulationJointCore& core = immArt->mArticulationJointCores[index];

	data.parentPose				= core.parentPose;
	data.childPose				= core.childPose;
	data.frictionCoefficient	= core.frictionCoefficient;
	data.type					= PxArticulationJointType::Enum(core.jointType);
	for(PxU32 i=0;i<PxArticulationAxis::eCOUNT;i++)
	{
		data.motion[i]		= PxArticulationMotion::Enum(PxU8(core.motion[i]));
		data.limits[i]		= core.limits[i];
		data.drives[i]		= core.drives[i];
		data.targetPos[i]	= core.targetP[i];
		data.targetVel[i]	= core.targetV[i];
		data.armature[i]	= core.armature[i];
		data.jointPos[i]	= core.jointPos[i];
		data.jointVel[i]	= core.jointVel[i];
		data.maxJointVelocity[i] = core.maxJointVelocity[i];
	}
	return true;
}

static bool samePoses(const PxTransform& pose0, const PxTransform& pose1)
{
	return (pose0.p == pose1.p) && (pose0.q == pose1.q);
}

// PT: this is not super efficient if you only want to change one parameter. We could consider adding individual, atomic accessors (but that would
// bloat the API) or flags to validate the desired parameters.
bool immediate::PxSetJointData(const PxArticulationLinkHandle& link, const PxArticulationJointDataRC& data)
{
	PxU32 index;
	immArticulation* immArt = const_cast<immArticulation*>(getFromLink(link, index));
	if(!immArt)
		return false;

	Dy::ArticulationJointCore& core = immArt->mArticulationJointCores[index];

	// PT: poses read by jcalc in ArticulationJointCore::setJointFrame. We need to set ArticulationJointCoreDirtyFlag::eFRAME for this.
	{
		if(!samePoses(core.parentPose, data.parentPose))
		{
			core.setParentPose(data.parentPose);	// PT: also sets ArticulationJointCoreDirtyFlag::eFRAME
			immArt->mJCalcDirty = true;
		}

		if(!samePoses(core.childPose, data.childPose))
		{
			core.setChildPose(data.childPose);		// PT: also sets ArticulationJointCoreDirtyFlag::eFRAME
			immArt->mJCalcDirty = true;
		}
	}

	// PT: joint type read by jcalc in computeMotionMatrix, called from ArticulationJointCore::setJointFrame
	if(core.jointType!=PxU8(data.type))
	{
		core.setJointType(data.type);
		immArt->mJCalcDirty = true;
	}

	// PT: TODO: do we need to recompute jcalc for these?
	core.frictionCoefficient	= data.frictionCoefficient;

	for(PxU32 i=0;i<PxArticulationAxis::eCOUNT;i++)
	{
		// PT: we don't need to recompute jcalc for these
		core.limits[i]	= data.limits[i];
		core.drives[i]	= data.drives[i];
		core.maxJointVelocity[i] = data.maxJointVelocity[i];

		core.jointPos[i] = data.jointPos[i];
		core.jointVel[i] = data.jointVel[i];

		// PT: joint motion read by jcalc in computeJointDof. 
		if(core.motion[i]!=data.motion[i])
		{
			core.setMotion(PxArticulationAxis::Enum(i), data.motion[i]);	
			immArt->mJCalcDirty = true;
		}

		// PT: targetP read by jcalc
		if(core.targetP[i] != data.targetPos[i])
		{
			core.setTargetP(PxArticulationAxis::Enum(i), data.targetPos[i]);
			immArt->mJCalcDirty = true;
		}

		// PT: targetV read by jcalc
		if(core.targetV[i] != data.targetVel[i])
		{
			core.setTargetV(PxArticulationAxis::Enum(i), data.targetVel[i]);
			immArt->mJCalcDirty = true;
		}


		if(core.armature[i] != data.armature[i])
		{
			core.setArmature(PxArticulationAxis::Enum(i), data.armature[i]);
		}
	}

	return true;
}

void immediate::PxConstructSolverBodiesTGS(const PxRigidBodyData* inRigidData, PxTGSSolverBodyVel* outSolverBodyVel, 
	PxTGSSolverBodyTxInertia* outSolverBodyTxInertia, PxTGSSolverBodyData* outSolverBodyData, PxU32 nbBodies, const PxVec3& gravity, 
	PxReal dt, bool gyroscopicForces)
{
	for (PxU32 a = 0; a<nbBodies; a++)
	{
		const PxRigidBodyData& rigidData = inRigidData[a];
		PxVec3 lv = rigidData.linearVelocity, av = rigidData.angularVelocity;
		Dy::bodyCoreComputeUnconstrainedVelocity(gravity, dt, rigidData.linearDamping, rigidData.angularDamping, 1.0f, rigidData.maxLinearVelocitySq, rigidData.maxAngularVelocitySq, lv, av, false);

		Dy::copyToSolverBodyDataStep(lv, av, rigidData.invMass, rigidData.invInertia, rigidData.body2World, -rigidData.maxDepenetrationVelocity, rigidData.maxContactImpulse, PX_INVALID_NODE,
			PX_MAX_F32, rigidData.maxAngularVelocitySq, 0, false, outSolverBodyVel[a], outSolverBodyTxInertia[a], outSolverBodyData[a], dt, gyroscopicForces);
	}
}

void immediate::PxConstructStaticSolverBodyTGS(const PxTransform& globalPose, PxTGSSolverBodyVel& solverBodyVel, PxTGSSolverBodyTxInertia& solverBodyTxInertia, PxTGSSolverBodyData& solverBodyData)
{
	const PxVec3 zero(0.0f);
	Dy::copyToSolverBodyDataStep(zero, zero, 0.0f, zero, globalPose, -PX_MAX_F32, PX_MAX_F32, PX_INVALID_NODE, PX_MAX_F32, PX_MAX_F32, 0, true, solverBodyVel, solverBodyTxInertia, solverBodyData, 0.0f, false);
}

bool immediate::PxCreateContactConstraintsTGS(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxTGSSolverContactDesc* contactDescs,
	PxConstraintAllocator& allocator, PxReal invDt, PxReal invTotalDt, PxReal bounceThreshold, PxReal frictionOffsetThreshold, PxReal correlationDistance)
{
	PX_ASSERT(invDt > 0.0f && PxIsFinite(invDt));
	PX_ASSERT(bounceThreshold < 0.0f);
	PX_ASSERT(frictionOffsetThreshold > 0.0f);
	PX_ASSERT(correlationDistance > 0.0f);

	Dy::CorrelationBuffer cb;

	PxU32 currentContactDescIdx = 0;

	// PT: we call it this way in the snippet:
	// PxCreateContactConstraintsTGS(..., invStepDt, invDt, ...);
	// with:
	// const PxReal dt = 1.f / 60.f;
	// const PxReal invDt = 60.f;
	// const PxReal stepDt = dt/PxReal(nbPositionIterations);
	// const PxReal invStepDt = invDt * PxReal(nbPositionIterations);
	//
	// So this is a bit confusing here because "invDt" is not the same for the calling code and now:
	// invStepDt => invDt
	// invDt => invTotalDt
	//
	// Thus:
	// bias = invTotalDt/invDt (in function) = invDt/invStepDt (calling code) = invDt/(invDt * PxReal(nbPositionIterations)) = 1/nbPositionIterations
	// Which is the same as what we used for bias inside the SDK (non immediate mode)

	const PxReal biasCoefficient = 2.f*PxSqrt(invTotalDt/invDt);
	const PxReal totalDt = 1.f/invTotalDt;
	const PxReal dt = 1.f / invDt;

	for (PxU32 i = 0; i < nbHeaders; ++i)
	{
		Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];
		if (batchHeader.stride == 4)
		{
			PxU32 totalContacts = contactDescs[currentContactDescIdx].numContacts + contactDescs[currentContactDescIdx + 1].numContacts +
				contactDescs[currentContactDescIdx + 2].numContacts + contactDescs[currentContactDescIdx + 3].numContacts;

			if (totalContacts <= 64)
			{
				state = Dy::createFinalizeSolverContacts4Step(cb,
					contactDescs + currentContactDescIdx,
					invDt,
					totalDt,
					invTotalDt,
					dt,
					bounceThreshold,
					frictionOffsetThreshold,
					correlationDistance,
					biasCoefficient,
					allocator);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				Dy::createFinalizeSolverContactsStep(contactDescs[currentContactDescIdx + a], cb, invDt, invTotalDt, totalDt, dt, bounceThreshold,
					frictionOffsetThreshold, correlationDistance, biasCoefficient, allocator);
			}
		}

		if(contactDescs[currentContactDescIdx].desc->constraint)
		{
			PxU8 type = *contactDescs[currentContactDescIdx].desc->constraint;

			if (type == DY_SC_TYPE_STATIC_CONTACT)
			{
				//Check if any block of constraints is classified as type static (single) contact constraint.
				//If they are, iterate over all constraints grouped with it and switch to "dynamic" contact constraint
				//type if there's a dynamic contact constraint in the group.
				for (PxU32 c = 1; c < batchHeader.stride; ++c)
				{
					if (*contactDescs[currentContactDescIdx + c].desc->constraint == DY_SC_TYPE_RB_CONTACT)
					{
						type = DY_SC_TYPE_RB_CONTACT;
						break;
					}
				}
			}

			batchHeader.constraintType = type;
		}

		currentContactDescIdx += batchHeader.stride;
	}
	return true;
}

bool immediate::PxCreateJointConstraintsTGS(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, 
	PxTGSSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, PxReal dt, PxReal totalDt, PxReal invDt,
	PxReal invTotalDt, PxReal lengthScale)
{
	PX_ASSERT(dt > 0.0f);
	PX_ASSERT(invDt > 0.0f && PxIsFinite(invDt));

	const PxReal biasCoefficient = 2.f*PxSqrt(dt/totalDt);

	PxU32 currentDescIdx = 0;
	for (PxU32 i = 0; i < nbHeaders; ++i)
	{
		Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		PxU8 type = DY_SC_TYPE_BLOCK_1D;
		if (batchHeader.stride == 4)
		{
			PxU32 totalRows = 0;
			PxU32 maxRows = 0;
			bool batchable = true;
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				if (jointDescs[currentDescIdx + a].numRows == 0)
				{
					batchable = false;
					break;
				}
				totalRows += jointDescs[currentDescIdx + a].numRows;
				maxRows = PxMax(maxRows, jointDescs[currentDescIdx + a].numRows);
			}

			if (batchable)
			{
				state = Dy::setupSolverConstraintStep4
				(jointDescs + currentDescIdx,
					dt, totalDt, invDt, invTotalDt, totalRows,
					allocator, maxRows, lengthScale, biasCoefficient, false);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			type = DY_SC_TYPE_RB_1D;
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				// PT: TODO: And "isExtended" is already computed in Dy::ConstraintHelper::setupSolverConstraint
				PxSolverConstraintDesc& desc = *jointDescs[currentDescIdx + a].desc;
				const bool isExtended = desc.linkIndexA != PxSolverConstraintDesc::RIGID_BODY || desc.linkIndexB != PxSolverConstraintDesc::RIGID_BODY;
				if (isExtended)
					type = DY_SC_TYPE_EXT_1D;

				Dy::setupSolverConstraintStep(jointDescs[currentDescIdx + a], allocator, dt, totalDt, invDt, invTotalDt, lengthScale, biasCoefficient);
			}
		}

		batchHeader.constraintType = type;
		currentDescIdx += batchHeader.stride;
	}

	return true;
}

template<class LeafTestT, class ParamsT>
static bool PxCreateJointConstraintsWithShadersTGS_T(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, ParamsT* params, PxTGSSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, PxReal dt, PxReal totalDt, PxReal invDt, PxReal invTotalDt, PxReal lengthScale)
{
	Px1DConstraint allRows[Dy::MAX_CONSTRAINT_ROWS * 4];

	//Runs shaders to fill in rows...

	PxU32 currentDescIdx = 0;

	for (PxU32 i = 0; i<nbHeaders; i++)
	{
		Px1DConstraint* rows = allRows;
		Px1DConstraint* rows2 = allRows;

		PxU32 maxRows = 0;
		PxU32 nbToPrep = MAX_CONSTRAINT_ROWS;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		for (PxU32 a = 0; a<batchHeader.stride; a++)
		{
			PxTGSSolverConstraintPrepDesc& desc = jointDescs[currentDescIdx + a];

			PxConstraintSolverPrep prep;
			const void* constantBlock;
			const bool useExtendedLimits = LeafTestT::getData(params, currentDescIdx + a, &prep, &constantBlock);
			PX_ASSERT(prep);

			PX_ASSERT(rows2 + nbToPrep <= allRows + MAX_CONSTRAINT_ROWS*4);
			setupConstraintRows(rows2, nbToPrep);
			rows2 += nbToPrep;

			desc.invMassScales.linear0 = desc.invMassScales.linear1 = desc.invMassScales.angular0 = desc.invMassScales.angular1 = 1.0f;
			desc.body0WorldOffset = PxVec3(0.0f);

			//TAG:solverprepcall
			const PxU32 constraintCount = prep(rows,
				desc.body0WorldOffset,
				Dy::MAX_CONSTRAINT_ROWS,
				desc.invMassScales,
				constantBlock,
				desc.bodyFrame0, desc.bodyFrame1,
				useExtendedLimits,
				desc.cA2w, desc.cB2w);

			nbToPrep = constraintCount;
			maxRows = PxMax(constraintCount, maxRows);

			desc.rows = rows;
			desc.numRows = constraintCount;
			rows += constraintCount;
		}

		PxCreateJointConstraintsTGS(&batchHeader, 1, jointDescs + currentDescIdx, allocator, dt, totalDt, invDt,
			invTotalDt, lengthScale);

		currentDescIdx += batchHeader.stride;
	}
	return true; //KS - TODO - do some error reporting/management...
}

bool immediate::PxCreateJointConstraintsWithShadersTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, PxConstraint** constraints, PxTGSSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt,
	const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal lengthScale)
{
	return PxCreateJointConstraintsWithShadersTGS_T<PxConstraintAdapter>(batchHeaders, nbBatchHeaders, constraints, jointDescs, allocator, dt, totalDt, invDt, invTotalDt, lengthScale);
}

bool immediate::PxCreateJointConstraintsWithImmediateShadersTGS(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxImmediateConstraint* constraints, PxTGSSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, PxReal dt, PxReal totalDt, PxReal invDt, PxReal invTotalDt, PxReal lengthScale)
{
	class immConstraintAdapter
	{
	public:
		static PX_FORCE_INLINE bool getData(PxImmediateConstraint* constraints_, PxU32 i, PxConstraintSolverPrep* prep, const void** constantBlock)
		{
			const PxImmediateConstraint& ic = constraints_[i];
			*prep = ic.prep;
			*constantBlock = ic.constantBlock;
			return false;
		}
	};

	return PxCreateJointConstraintsWithShadersTGS_T<immConstraintAdapter>(batchHeaders, nbHeaders, constraints, jointDescs, allocator, dt, totalDt, invDt, invTotalDt, lengthScale);
}

void immediate::PxSolveConstraintsTGS(const PxConstraintBatchHeader* batchHeaders, PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
	PxTGSSolverBodyVel* solverBodies, PxTGSSolverBodyTxInertia* txInertias, PxU32 nbSolverBodies, PxU32 nbPositionIterations, PxU32 nbVelocityIterations,
	float dt, float invDt, PxU32 nbSolverArticulations, PxArticulationHandle* solverArticulations, PxSpatialVector* /* pxZ */, PxSpatialVector* pxDeltaV)
{
	PX_ASSERT(nbPositionIterations > 0);
	PX_ASSERT(nbVelocityIterations > 0);

	const Dy::TGSSolveBlockMethod* solveTable = Dy::g_SolveTGSMethods;
	const Dy::TGSSolveConcludeMethod* solveConcludeTable = Dy::g_SolveConcludeTGSMethods;
	const Dy::TGSWriteBackMethod* writebackTable = Dy::g_WritebackTGSMethods;

	Dy::SolverContext cache;
	cache.solverBodyArray = NULL;
	cache.mThresholdStreamIndex = 0;
	cache.mThresholdStreamLength = 0xFFFFFFF;

	Cm::SpatialVectorF* deltaV = reinterpret_cast<Cm::SpatialVectorF*>(pxDeltaV);

	cache.deltaV = deltaV;
	cache.doFriction = true;

	Dy::FeatherstoneArticulation** articulations = reinterpret_cast<Dy::FeatherstoneArticulation**>(solverArticulations);

	struct TGS
	{
		static PX_FORCE_INLINE void solveArticulationInternalConstraints(float dt_, float invDt_, PxU32 nbSolverArticulations_, Dy::FeatherstoneArticulation** solverArticulations_,
			PxReal elapsedTime, bool velIter_)
		{
			while(nbSolverArticulations_--)
			{
				immArticulation* immArt = static_cast<immArticulation*>(*solverArticulations_++);
				immArt->immSolveInternalConstraints(dt_, invDt_, elapsedTime, velIter_, true);
			}
		}
	};

	const PxReal invTotalDt = 1.0f/(dt*nbPositionIterations);

	PxReal elapsedTime = 0.0f;

	cache.isPositionIteration = true;
	while(nbPositionIterations--)
	{
		TGS::solveArticulationInternalConstraints(dt, invDt, nbSolverArticulations, articulations, elapsedTime, false);

		for(PxU32 a=0; a<nbBatchHeaders; ++a)
		{
			const PxConstraintBatchHeader& batch = batchHeaders[a];
			if(nbPositionIterations)
				solveTable[batch.constraintType](batch, solverConstraintDescs, txInertias, -PX_MAX_F32, elapsedTime, cache);
			else
				solveConcludeTable[batch.constraintType](batch, solverConstraintDescs, txInertias, elapsedTime, cache);
		}

		{
			for(PxU32 j=0; j<nbSolverBodies; ++j)
				Dy::integrateCoreStep(solverBodies[j], txInertias[j], dt);

			for(PxU32 j=0; j<nbSolverArticulations; ++j)
			{
				immArticulation* immArt = static_cast<immArticulation*>(solverArticulations[j]);
				immArt->recordDeltaMotion(immArt->getSolverDesc(), dt, deltaV, invTotalDt);
			}
		}

		elapsedTime += dt;
	}

	for (PxU32 a=0; a<nbSolverArticulations; a++)
	{
		immArticulation* immArt = static_cast<immArticulation*>(articulations[a]);
		immArt->saveVelocityTGS(immArt, invTotalDt);
	}

	cache.isPositionIteration = false;
	while(nbVelocityIterations--)
	{
		TGS::solveArticulationInternalConstraints(dt, invDt, nbSolverArticulations, articulations, elapsedTime, true);

		for(PxU32 a=0; a<nbBatchHeaders; ++a)
		{
			const PxConstraintBatchHeader& batch = batchHeaders[a];
			solveTable[batch.constraintType](batch, solverConstraintDescs, txInertias, 0.0f, elapsedTime, cache);
			if(!nbVelocityIterations)
				writebackTable[batch.constraintType](batch, solverConstraintDescs, &cache);
		}
	}
}

void immediate::PxIntegrateSolverBodiesTGS(PxTGSSolverBodyVel* solverBody, const PxTGSSolverBodyTxInertia* txInertia, PxTransform* poses, PxU32 nbBodiesToIntegrate, PxReal /*dt*/)
{
	for(PxU32 i = 0; i < nbBodiesToIntegrate; ++i)
	{
		solverBody[i].angularVelocity = txInertia[i].sqrtInvInertia * solverBody[i].angularVelocity;
		poses[i].p = txInertia[i].body2WorldP;
		poses[i].q = (txInertia[i].deltaBody2WorldQ * poses[i].q).getNormalized();
	}
}


#include "PxvGlobals.h"
#include "PxPhysXGpu.h"
#include "BpBroadPhase.h"
#include "PxsHeapMemoryAllocator.h"
#include "PxsKernelWrangler.h"
#include "PxsMemoryManager.h"

PX_COMPILE_TIME_ASSERT(sizeof(Bp::FilterGroup::Enum)==sizeof(PxBpFilterGroup));

PX_IMPLEMENT_OUTPUT_ERROR

PxBpFilterGroup	physx::PxGetBroadPhaseStaticFilterGroup()
{
	return Bp::getFilterGroup_Statics();
}

PxBpFilterGroup	physx::PxGetBroadPhaseDynamicFilterGroup(PxU32 id)
{
	return Bp::getFilterGroup_Dynamics(id, false);
}

PxBpFilterGroup	physx::PxGetBroadPhaseKinematicFilterGroup(PxU32 id)
{
	return Bp::getFilterGroup_Dynamics(id, true);
}

namespace
{
	// PT: the Bp::BroadPhase API is quite confusing and the file cannot be included from everywhere
	// so let's have a user-friendly wrapper for now.
	class ImmCPUBP : public PxBroadPhase, public PxBroadPhaseRegions, public PxUserAllocated
	{
		public:
											ImmCPUBP(const PxBroadPhaseDesc& desc);
		virtual								~ImmCPUBP();

		virtual	bool						init(const PxBroadPhaseDesc& desc);

		// PxBroadPhase
		virtual	void						release()																	PX_OVERRIDE	PX_FINAL;
		virtual	PxBroadPhaseType::Enum		getType()															const	PX_OVERRIDE	PX_FINAL;
		virtual	void						getCaps(PxBroadPhaseCaps& caps)										const	PX_OVERRIDE	PX_FINAL;
		virtual	PxBroadPhaseRegions*		getRegions()																PX_OVERRIDE	PX_FINAL;
		virtual	PxAllocatorCallback*		getAllocator()																PX_OVERRIDE;
		virtual	PxU64						getContextID()														const	PX_OVERRIDE	PX_FINAL;
		virtual	void						setScratchBlock(void* scratchBlock, PxU32 size)								PX_OVERRIDE	PX_FINAL;
		virtual	void						update(const PxBroadPhaseUpdateData& updateData, PxBaseTask* continuation)	PX_OVERRIDE	PX_FINAL;
		virtual	void						fetchResults(PxBroadPhaseResults& results)									PX_OVERRIDE	PX_FINAL;
		//~PxBroadPhase

		// PxBroadPhaseRegions
		virtual	PxU32						getNbRegions()																											const	PX_OVERRIDE	PX_FINAL;
		virtual	PxU32						getRegions(PxBroadPhaseRegionInfo* userBuffer, PxU32 bufferSize, PxU32 startIndex)										const	PX_OVERRIDE	PX_FINAL;
		virtual	PxU32						addRegion(const PxBroadPhaseRegion& region, bool populateRegion, const PxBounds3* boundsArray, const float* contactDistance)	PX_OVERRIDE	PX_FINAL;
		virtual	bool						removeRegion(PxU32 handle)																										PX_OVERRIDE	PX_FINAL;
		virtual	PxU32						getNbOutOfBoundsObjects()																								const	PX_OVERRIDE	PX_FINAL;
		virtual	const PxU32*				getOutOfBoundsObjects()																									const	PX_OVERRIDE	PX_FINAL;
		//~PxBroadPhaseRegions

				Bp::BroadPhase*				mBroadPhase;
				PxcScratchAllocator			mScratchAllocator;
				Bp::BpFilter				mFilters;
				PxArray<PxBroadPhasePair>	mCreatedPairs;
				PxArray<PxBroadPhasePair>	mDeletedPairs;

				const PxU64					mContextID;
				void*						mAABBManager;

				void						releaseBP();
	};
}

///////////////////////////////////////////////////////////////////////////////

ImmCPUBP::ImmCPUBP(const PxBroadPhaseDesc& desc) :
	mBroadPhase	(NULL),
	mFilters	(desc.mDiscardKinematicVsKinematic, desc.mDiscardStaticVsKinematic),
	mContextID	(desc.mContextID),
	mAABBManager(NULL)
{
}

ImmCPUBP::~ImmCPUBP()
{
	releaseBP();
}

void ImmCPUBP::releaseBP()
{
	PX_RELEASE(mBroadPhase);
}

bool ImmCPUBP::init(const PxBroadPhaseDesc& desc)
{
	if(!desc.isValid())
		return outputError<PxErrorCode::eINVALID_PARAMETER>(__LINE__, "PxCreateBroadPhase: invalid broadphase descriptor");

	const PxU32 maxNbRegions = 0;
	const PxU32 maxNbBroadPhaseOverlaps = 0;
	const PxU32 maxNbStaticShapes = 0;
	const PxU32 maxNbDynamicShapes = 0;

	// PT: TODO: unify creation of CPU and GPU BPs (PX-2542)
	mBroadPhase = Bp::BroadPhase::create(desc.mType, maxNbRegions, maxNbBroadPhaseOverlaps, maxNbStaticShapes, maxNbDynamicShapes, desc.mContextID);
	return mBroadPhase!=NULL;
}

///////////////////////////////////////////////////////////////////////////////

void ImmCPUBP::release()
{
	if(mAABBManager)
	{
		outputError<PxErrorCode::eINVALID_OPERATION>(__LINE__, "ImmCPUBP::release: AABB manager is still present, release the AABB manager first");
		return;
	}

	PX_DELETE_THIS;
}

PxBroadPhaseType::Enum ImmCPUBP::getType() const
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->getType();
}

void ImmCPUBP::getCaps(PxBroadPhaseCaps& caps) const
{
	PX_ASSERT(mBroadPhase);
	mBroadPhase->getCaps(caps);
}

PxBroadPhaseRegions* ImmCPUBP::getRegions()
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->getType() == PxBroadPhaseType::eMBP ? this : NULL;
}

PxAllocatorCallback* ImmCPUBP::getAllocator()
{
	return PxGetAllocatorCallback();
}

PxU64 ImmCPUBP::getContextID() const
{
	return mContextID;
}

void ImmCPUBP::setScratchBlock(void* scratchBlock, PxU32 size)
{
	if(scratchBlock && size)
		mScratchAllocator.setBlock(scratchBlock, size);
}

void ImmCPUBP::update(const PxBroadPhaseUpdateData& updateData, PxBaseTask* continuation)
{
	PX_PROFILE_ZONE("ImmCPUBP::update", mContextID);

	PX_ASSERT(mBroadPhase);

	// PT: convert PxBroadPhaseUpdateData to Bp::BroadPhaseUpdateData. Main differences is the two undocumented bools
	// added for the GPU version. For now we just set them to true, which may not be the fastest but it should always
	// be correct.
	// TODO: revisit this / get rid of the bools in the low-level API (PX-2835)

	const Bp::BroadPhaseUpdateData defaultUpdateData(
		updateData.mCreated, updateData.mNbCreated,
		updateData.mUpdated, updateData.mNbUpdated,
		updateData.mRemoved, updateData.mNbRemoved,
		updateData.mBounds,
		reinterpret_cast<const Bp::FilterGroup::Enum*>(updateData.mGroups),
		updateData.mDistances, updateData.mCapacity,
		mFilters,
		true, true);

	// PT: preBroadPhase & fetchBroadPhaseResults are only needed for the GPU BP.
	// The PxBroadPhase API hides this from users and gives them an easier API that
	// deals with these differences under the hood.

	mBroadPhase->preBroadPhase(defaultUpdateData);	// ### could be skipped for CPU BPs

	// PT: BP UPDATE CALL
	mBroadPhase->update(&mScratchAllocator, defaultUpdateData, continuation);

	mBroadPhase->fetchBroadPhaseResults();	// ### could be skipped for CPU BPs
}

void ImmCPUBP::fetchResults(PxBroadPhaseResults& results)
{
	PX_PROFILE_ZONE("ImmCPUBP::fetchResults", mContextID);
	PX_ASSERT(mBroadPhase);

	// PT: TODO: flags to skip the copies (PX-2929)
	if(0)
	{
		results.mCreatedPairs = reinterpret_cast<const PxBroadPhasePair*>(mBroadPhase->getCreatedPairs(results.mNbCreatedPairs));
		results.mDeletedPairs = reinterpret_cast<const PxBroadPhasePair*>(mBroadPhase->getDeletedPairs(results.mNbDeletedPairs));
	}
	else
	{
		struct Local
		{
			static void copyPairs(PxArray<PxBroadPhasePair>& pairs, PxU32 nbPairs, const Bp::BroadPhasePair* bpPairs)
			{
				pairs.resetOrClear();
				const PxBroadPhasePair* src = reinterpret_cast<const PxBroadPhasePair*>(bpPairs);
				PxBroadPhasePair* dst = Cm::reserveContainerMemory(pairs, nbPairs);
				PxMemCopy(dst, src, sizeof(PxBroadPhasePair)*nbPairs);
			}
		};

		{
			PX_PROFILE_ZONE("copyPairs", mContextID);
			{
				PxU32 nbCreatedPairs;
				const Bp::BroadPhasePair* createdPairs = mBroadPhase->getCreatedPairs(nbCreatedPairs);
				Local::copyPairs(mCreatedPairs, nbCreatedPairs, createdPairs);
			}
			{
				PxU32 nbDeletedPairs;
				const Bp::BroadPhasePair* deletedPairs = mBroadPhase->getDeletedPairs(nbDeletedPairs);
				Local::copyPairs(mDeletedPairs, nbDeletedPairs, deletedPairs);
			}
		}

		results.mNbCreatedPairs = mCreatedPairs.size();
		results.mNbDeletedPairs = mDeletedPairs.size();
		results.mCreatedPairs = mCreatedPairs.begin();
		results.mDeletedPairs = mDeletedPairs.begin();
	}

	// PT: TODO: this function got introduced in the "GRB merge" (CL 20888255) for the SAP but wasn't necessary before,
	// and isn't necessary for the other BPs (even the GPU one). That makes no sense and should probably be removed.
	//mBroadPhase->deletePairs();

	// PT: similarly this is only needed for the SAP. This is also called at the exact same time as "deletePairs" so
	// I'm not sure why we used 2 separate functions. It just bloats the API for no reason.
	mBroadPhase->freeBuffers();
}

///////////////////////////////////////////////////////////////////////////////

// PT: the following calls are just re-routed to the LL functions. This should only be available/needed for MBP.

PxU32 ImmCPUBP::getNbRegions()	const
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->getNbRegions();
}

PxU32 ImmCPUBP::getRegions(PxBroadPhaseRegionInfo* userBuffer, PxU32 bufferSize, PxU32 startIndex)	const
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->getRegions(userBuffer, bufferSize, startIndex);
}

PxU32 ImmCPUBP::addRegion(const PxBroadPhaseRegion& region, bool populateRegion, const PxBounds3* boundsArray, const float* contactDistance)
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->addRegion(region, populateRegion, boundsArray, contactDistance);
}

bool ImmCPUBP::removeRegion(PxU32 handle)
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->removeRegion(handle);
}

PxU32 ImmCPUBP::getNbOutOfBoundsObjects()	const
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->getNbOutOfBoundsObjects();
}

const PxU32* ImmCPUBP::getOutOfBoundsObjects()	const
{
	PX_ASSERT(mBroadPhase);
	return mBroadPhase->getOutOfBoundsObjects();
}

///////////////////////////////////////////////////////////////////////////////

#if PX_SUPPORT_GPU_PHYSX
namespace
{
	class ImmGPUBP : public ImmCPUBP, public PxAllocatorCallback
	{
		public:
												ImmGPUBP(const PxBroadPhaseDesc& desc);
		virtual									~ImmGPUBP();

		// PxAllocatorCallback
		virtual void*							allocate(size_t size, const char* /*typeName*/, const char* filename, int line)	PX_OVERRIDE	PX_FINAL;
		virtual void							deallocate(void* ptr)															PX_OVERRIDE	PX_FINAL;
		//~PxAllocatorCallback

		// PxBroadPhase
		virtual	PxAllocatorCallback*			getAllocator()	PX_OVERRIDE	PX_FINAL;
		//~PxBroadPhase

		// ImmCPUBP
		virtual	bool							init(const PxBroadPhaseDesc& desc)	PX_OVERRIDE	PX_FINAL;
		//~ImmCPUBP

				PxPhysXGpu*						mPxGpu;
				PxsMemoryManager*				mMemoryManager;
				PxsKernelWranglerManager*		mGpuWranglerManagers;
				PxsHeapMemoryAllocatorManager*	mHeapMemoryAllocationManager;
	};
}
#endif

///////////////////////////////////////////////////////////////////////////////

#if PX_SUPPORT_GPU_PHYSX
ImmGPUBP::ImmGPUBP(const PxBroadPhaseDesc& desc) :
	ImmCPUBP					(desc),
	mPxGpu						(NULL),
	mMemoryManager				(NULL),
	mGpuWranglerManagers		(NULL),
	mHeapMemoryAllocationManager(NULL)
{
}

ImmGPUBP::~ImmGPUBP()
{
	releaseBP();	// PT: must release the BP first, before the base dtor is called
	PX_DELETE(mHeapMemoryAllocationManager);
	PX_DELETE(mMemoryManager);
	//PX_RELEASE(mPxGpu);
	PxvReleasePhysXGpu(mPxGpu);
	mPxGpu = NULL;
}

void* ImmGPUBP::allocate(size_t size, const char* /*typeName*/, const char* filename, int line)
{
	PX_ASSERT(mMemoryManager);
	PxVirtualAllocatorCallback* cb = mMemoryManager->getHostMemoryAllocator();
	return cb->allocate(size, 0, filename, line);
}

void ImmGPUBP::deallocate(void* ptr)
{
	PX_ASSERT(mMemoryManager);
	PxVirtualAllocatorCallback* cb = mMemoryManager->getHostMemoryAllocator();
	cb->deallocate(ptr);
}

PxAllocatorCallback* ImmGPUBP::getAllocator()
{
	return this;
}

bool ImmGPUBP::init(const PxBroadPhaseDesc& desc)
{
	PX_ASSERT(desc.mType==PxBroadPhaseType::eGPU);

	if(!desc.isValid())
		return outputError<PxErrorCode::eINVALID_PARAMETER>(__LINE__, "PxCreateBroadPhase: invalid broadphase descriptor");

	PxCudaContextManager* contextManager = desc.mContextManager;

	// PT: one issue with PxvGetPhysXGpu is that it creates the whole PxPhysXGpu object, not just the BP. Questionable coupling there.

	//mPxGpu = PxCreatePhysXGpu();

	mPxGpu = PxvGetPhysXGpu(true);
	if(!mPxGpu)
		return false;

	const PxU32 gpuComputeVersion = 0;

	// PT: what's the difference between the "GPU memory manager" and the "GPU heap memory allocator manager" ?
	mMemoryManager = mPxGpu->createGpuMemoryManager(contextManager);
	if(!mMemoryManager)
		return false;

	mGpuWranglerManagers = mPxGpu->getGpuKernelWranglerManager(contextManager);
	if(!mGpuWranglerManagers)
		return false;

	PxGpuDynamicsMemoryConfig gpuDynamicsConfig;
	gpuDynamicsConfig.foundLostPairsCapacity = desc.mFoundLostPairsCapacity;

	mHeapMemoryAllocationManager = mPxGpu->createGpuHeapMemoryAllocatorManager(gpuDynamicsConfig.heapCapacity, mMemoryManager, gpuComputeVersion);
	if(!mHeapMemoryAllocationManager)
		return false;

	// PT: we currently do not expose PxGpuBroadPhaseDesc for the standalone BP,
	// as the API does not expose environment IDs there either.
	PxGpuBroadPhaseDesc defaultGpuBPDesc;
	mBroadPhase = mPxGpu->createGpuBroadPhase(defaultGpuBPDesc, mGpuWranglerManagers, contextManager, gpuComputeVersion, gpuDynamicsConfig, mHeapMemoryAllocationManager, desc.mContextID);
	return mBroadPhase!=NULL;
}
#endif

///////////////////////////////////////////////////////////////////////////////

// PT: TODO: why don't we even have a PxBroadPhaseDesc in the main Px API by now? (PX-2933)
// The BP parameters are scattered in PxSceneDesc/PxSceneLimits/etc
// The various BP-related APIs are particularly messy.
PxBroadPhase* physx::PxCreateBroadPhase(const PxBroadPhaseDesc& desc)
{
	ImmCPUBP* immBP;
	if(desc.mType == PxBroadPhaseType::eGPU)
#if PX_SUPPORT_GPU_PHYSX
		immBP = PX_NEW(ImmGPUBP)(desc);
#else
		return NULL;
#endif
	else
		immBP = PX_NEW(ImmCPUBP)(desc);

	if(!immBP->init(desc))
	{
		PX_DELETE(immBP);
		return NULL;
	}
	return immBP;
}




namespace
{
	// TODO: user-data? (PX-2934)
	// TODO: aggregates? (PX-2935)
	// TODO: do we really need the bitmaps in this wrapper anyway?

	class HighLevelBroadPhaseAPI : public PxAABBManager, public PxUserAllocated
	{
		public:
										HighLevelBroadPhaseAPI(PxBroadPhase& broadphase);
		virtual							~HighLevelBroadPhaseAPI();

		// PxAABBManager
		virtual	void					release()				PX_OVERRIDE	PX_FINAL	{ PX_DELETE_THIS;		}

		virtual	void					addObject(PxU32 index, const PxBounds3& bounds, PxBpFilterGroup group, float distance)	PX_OVERRIDE	PX_FINAL;
		virtual	void					removeObject(PxU32 index)																PX_OVERRIDE	PX_FINAL;
		virtual	void					updateObject(PxU32 index, const PxBounds3* bounds, const float* distance)				PX_OVERRIDE	PX_FINAL;
		virtual	void					update(PxBaseTask* continuation)														PX_OVERRIDE	PX_FINAL;
		virtual	void					fetchResults(PxBroadPhaseResults& results)												PX_OVERRIDE	PX_FINAL;

		virtual	PxBroadPhase&			getBroadPhase()			PX_OVERRIDE	PX_FINAL	{ return mBroadPhase;	}
		virtual	const PxBounds3*		getBounds()		const	PX_OVERRIDE	PX_FINAL	{ return mBounds;		}
		virtual	const float*			getDistances()	const	PX_OVERRIDE	PX_FINAL	{ return mDistances;	}
		virtual	const PxBpFilterGroup*	getGroups()		const	PX_OVERRIDE	PX_FINAL	{ return mGroups;		}
		virtual	PxU32					getCapacity()	const	PX_OVERRIDE	PX_FINAL	{ return mCapacity;		}
		//~PxAABBManager

				void					reserveSpace(PxU32 nbTotalBounds);

				PxBroadPhase&			mBroadPhase;
				PxBounds3*				mBounds;
				float*					mDistances;
				PxBpFilterGroup*		mGroups;
				PxU32					mCapacity;	// PT: same capacity for all the above buffers

				// PT: TODO: pinned? (PX-2936)
				PxBitMap				mAddedHandleMap;		// PT: indexed by BoundsIndex
				PxBitMap				mRemovedHandleMap;		// PT: indexed by BoundsIndex
				PxBitMap				mUpdatedHandleMap;		// PT: indexed by BoundsIndex

				// PT: TODO: pinned? (PX-2936)
				PxArray<PxU32>			mAddedHandles;
				PxArray<PxU32>			mUpdatedHandles;
				PxArray<PxU32>			mRemovedHandles;

				const PxU64				mContextID;
	};
}

HighLevelBroadPhaseAPI::HighLevelBroadPhaseAPI(PxBroadPhase& broadphase) :
	mBroadPhase	(broadphase),
	mBounds		(NULL),
	mDistances	(NULL),
	mGroups		(NULL),
	mCapacity	(0),
	mContextID	(broadphase.getContextID())
{
	ImmCPUBP& baseBP = static_cast<ImmCPUBP&>(broadphase);
	PX_ASSERT(!baseBP.mAABBManager);
	baseBP.mAABBManager = this;
}

HighLevelBroadPhaseAPI::~HighLevelBroadPhaseAPI()
{
	PxAllocatorCallback* allocator = mBroadPhase.getAllocator();
	if(mDistances)
	{
		allocator->deallocate(mDistances);
		mDistances = NULL;
	}
	if(mGroups)
	{
		allocator->deallocate(mGroups);
		mGroups = NULL;
	}
	if(mBounds)
	{
		allocator->deallocate(mBounds);
		mBounds = NULL;
	}

	ImmCPUBP& baseBP = static_cast<ImmCPUBP&>(mBroadPhase);
	baseBP.mAABBManager = NULL;
}

void HighLevelBroadPhaseAPI::reserveSpace(PxU32 nbEntriesNeeded)
{
	PX_PROFILE_ZONE("HighLevelBroadPhaseAPI::reserveSpace", mContextID);
	PX_ASSERT(mCapacity<nbEntriesNeeded);	// PT: otherwise don't call this function

	// PT: allocate more than necessary to minimize the amount of reallocations
	nbEntriesNeeded = PxNextPowerOfTwo(nbEntriesNeeded);

	// PT: use the allocator provided by the BP, in case we need CUDA-friendly buffers
	PxAllocatorCallback* allocator = mBroadPhase.getAllocator();

	{
		// PT: for bounds we always allocate one more entry to ensure safe SIMD loads
		PxBounds3* newBounds = reinterpret_cast<PxBounds3*>(allocator->allocate(sizeof(PxBounds3)*(nbEntriesNeeded+1), "HighLevelBroadPhaseAPI::mBounds", PX_FL));
		if(mCapacity && mBounds)
			PxMemCopy(newBounds, mBounds, sizeof(PxBounds3)*mCapacity);
		for(PxU32 i=mCapacity;i<nbEntriesNeeded;i++)
			newBounds[i].setEmpty();	// PT: maybe we could skip this for perf
		if(mBounds)
			allocator->deallocate(mBounds);
		mBounds = newBounds;
	}

	{
		PxBpFilterGroup* newGroups = reinterpret_cast<PxBpFilterGroup*>(allocator->allocate(sizeof(PxBpFilterGroup)*nbEntriesNeeded, "HighLevelBroadPhaseAPI::mGroups", PX_FL));
		if(mCapacity && mGroups)
			PxMemCopy(newGroups, mGroups, sizeof(PxBpFilterGroup)*mCapacity);
		for(PxU32 i=mCapacity;i<nbEntriesNeeded;i++)
			newGroups[i] = PX_INVALID_BP_FILTER_GROUP;	// PT: maybe we could skip this for perf
		if(mGroups)
			allocator->deallocate(mGroups);
		mGroups = newGroups;
	}

	{
		float* newDistances = reinterpret_cast<float*>(allocator->allocate(sizeof(float)*nbEntriesNeeded, "HighLevelBroadPhaseAPI::mDistances", PX_FL));
		if(mCapacity && mDistances)
			PxMemCopy(newDistances, mDistances, sizeof(float)*mCapacity);
		for(PxU32 i=mCapacity;i<nbEntriesNeeded;i++)
			newDistances[i] = 0.0f;	// PT: maybe we could skip this for perf
		if(mDistances)
			allocator->deallocate(mDistances);
		mDistances = newDistances;
	}

	mAddedHandleMap.resize(nbEntriesNeeded);
	mRemovedHandleMap.resize(nbEntriesNeeded);

	mCapacity = nbEntriesNeeded;
}

// PT: TODO: version with internal index management? (PX-2942)

// PT: TODO: batched version?
void HighLevelBroadPhaseAPI::addObject(PxBpIndex index, const PxBounds3& bounds, PxBpFilterGroup group, float distance)
{
	PX_ASSERT(group != PX_INVALID_BP_FILTER_GROUP);	// PT: we use group == PX_INVALID_BP_FILTER_GROUP to mark removed/invalid entries

	const PxU32 nbEntriesNeeded = index + 1;
	if(mCapacity<nbEntriesNeeded)
		reserveSpace(nbEntriesNeeded);

	mBounds[index] = bounds;
	mGroups[index] = group;
	mDistances[index] = distance;

	if(mRemovedHandleMap.test(index))
		mRemovedHandleMap.reset(index);
	else	// PT: for case where an object in the BP gets removed and then we re-add same frame (we don't want to set the add bit in this case)
		mAddedHandleMap.set(index);
}

// PT: TODO: batched version?
void HighLevelBroadPhaseAPI::removeObject(PxBpIndex index)
{
	PX_ASSERT(index < mCapacity);
	PX_ASSERT(mGroups[index] != PX_INVALID_BP_FILTER_GROUP);

	if(mAddedHandleMap.test(index))		// PT: if object had been added this frame...
		mAddedHandleMap.reset(index);	// PT: ...then simply revert the previous operation locally (it hasn't been passed to the BP yet).
	else
		mRemovedHandleMap.set(index);	// PT: else we need to remove it from the BP

	mBounds[index].setEmpty();
	mGroups[index] = PX_INVALID_BP_FILTER_GROUP;
	mDistances[index] = 0.0f;
}

// PT: TODO: batched version?
void HighLevelBroadPhaseAPI::updateObject(PxBpIndex index, const PxBounds3* bounds, const float* distance)
{
	PX_ASSERT(index < mCapacity);

	mUpdatedHandleMap.growAndSet(index);

	if(bounds)
		mBounds[index] = *bounds;
	if(distance)
		mDistances[index] = *distance;
}

namespace
{
	struct HandleTest_Add
	{
		static PX_FORCE_INLINE void processEntry(HighLevelBroadPhaseAPI& bp, PxU32 handle)
		{
			PX_ASSERT(bp.mGroups[handle] != PX_INVALID_BP_FILTER_GROUP);
			bp.mAddedHandles.pushBack(handle);
		}
	};

	struct HandleTest_Update
	{
		static PX_FORCE_INLINE void processEntry(HighLevelBroadPhaseAPI& bp, PxU32 handle)
		{
			// PT: TODO: revisit the logic here (PX-2937)

			PX_ASSERT(!bp.mRemovedHandleMap.test(handle));	// a handle may only be updated and deleted if it was just added.

			if(bp.mAddedHandleMap.test(handle))				// just-inserted handles may also be marked updated, so skip them
				return;

			PX_ASSERT(bp.mGroups[handle] != PX_INVALID_BP_FILTER_GROUP);

			bp.mUpdatedHandles.pushBack(handle);
		}
	};

	struct HandleTest_Remove
	{
		static PX_FORCE_INLINE void processEntry(HighLevelBroadPhaseAPI& bp, PxU32 handle)
		{
			PX_ASSERT(bp.mGroups[handle] == PX_INVALID_BP_FILTER_GROUP);
			bp.mRemovedHandles.pushBack(handle);
		}
	};
}

template<class FunctionT, class ParamsT>
static void iterateBitmap(const PxBitMap& bitmap, ParamsT& params)
{
	const PxU32* bits = bitmap.getWords();
	if(bits)
	{
		const PxU32 lastSetBit = bitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = bits[w]; b; b &= b-1)
			{
				const PxU32 index = PxU32(w<<5|PxLowestSetBit(b));

				FunctionT::processEntry(params, index);
			}
		}
	}
}

/*static void shuffle(PxArray<PxU32>& handles)
{
	PxU32 nb = handles.size();
	PxU32* data = handles.begin();
	for(PxU32 i=0;i<nb*10;i++)
	{
		PxU32 id0 = rand() % nb;
		PxU32 id1 = rand() % nb;
		PxSwap(data[id0], data[id1]);
	}
}*/

void HighLevelBroadPhaseAPI::update(PxBaseTask* continuation)
{
	PX_PROFILE_ZONE("HighLevelBroadPhaseAPI::update", mContextID);

	{
		PX_PROFILE_ZONE("resetOrClear", mContextID);
		mAddedHandles.resetOrClear();
		mUpdatedHandles.resetOrClear();
		mRemovedHandles.resetOrClear();
	}

	{
		{
			PX_PROFILE_ZONE("iterateBitmap added", mContextID);
			iterateBitmap<HandleTest_Add>(mAddedHandleMap, *this);
		}
		{
			PX_PROFILE_ZONE("iterateBitmap updated", mContextID);
			iterateBitmap<HandleTest_Update>(mUpdatedHandleMap, *this);
		}
		{
			PX_PROFILE_ZONE("iterateBitmap removed", mContextID);
			iterateBitmap<HandleTest_Remove>(mRemovedHandleMap, *this);
		}
	}

	// PT: call the low-level BP API
	{
		PX_PROFILE_ZONE("BP update", mContextID);
/*		if(1)	// Suffle test
		{
			shuffle(mAddedHandles);
			shuffle(mUpdatedHandles);
			shuffle(mRemovedHandles);
		}*/

		const PxBroadPhaseUpdateData updateData(
			mAddedHandles.begin(), mAddedHandles.size(),
			mUpdatedHandles.begin(), mUpdatedHandles.size(),
			mRemovedHandles.begin(), mRemovedHandles.size(),
			mBounds, mGroups, mDistances,
			mCapacity);

		mBroadPhase.update(updateData, continuation);
	}

	{
		PX_PROFILE_ZONE("clear bitmaps", mContextID);
		mAddedHandleMap.clear();
		mRemovedHandleMap.clear();
		mUpdatedHandleMap.clear();
	}
}

void HighLevelBroadPhaseAPI::fetchResults(PxBroadPhaseResults& results)
{
	PX_PROFILE_ZONE("HighLevelBroadPhaseAPI::fetchResults", mContextID);
	mBroadPhase.fetchResults(results);
}

PxAABBManager* physx::PxCreateAABBManager(PxBroadPhase& bp)
{
	// PT: make sure we cannot link a bp to multiple managers
	ImmCPUBP& baseBP = static_cast<ImmCPUBP&>(bp);
	if(baseBP.mAABBManager)
		return NULL;

	return PX_NEW(HighLevelBroadPhaseAPI)(bp);
}
