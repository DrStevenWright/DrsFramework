// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "DrsBase/DrsEnumToString.h"

#include "Templates/IsUEnumClass.h"
#include "Traits/IsTEnumAsByte.h"
#include <type_traits>

namespace Drs::Private
{
	/**
	 * Converts a vote ID to a string for debug output:
	 * - FName: the name, or "Anonymous" for None.
	 * - UENUM enum class, or TEnumAsByte: the value name, without the enum prefix.
	 * - Other enums: the underlying number.
	 * - Anything else: LexToString, which must have an overload for the type.
	 */
	template <typename VoteIdType>
	FString VoteIdToString(const VoteIdType& voteId)
	{
		if constexpr (std::is_same_v<VoteIdType, FName>)
		{
			return voteId.IsNone() ? FString(TEXT("Anonymous")) : voteId.ToString();
		}
		else if constexpr (TIsUEnumClass<VoteIdType>::Value || TIsTEnumAsByte_V<VoteIdType>)
		{
			return Drs::EnumToString(voteId);
		}
		else if constexpr (std::is_enum_v<VoteIdType>)
		{
			return LexToString(static_cast<std::underlying_type_t<VoteIdType>>(voteId));
		}
		else
		{
			static_assert(requires { LexToString(voteId); }, "TDrsVotableState needs a LexToString overload for its VoteIdType.");
			return LexToString(voteId);
		}
	}
}

/**
 * A boolean state that several voters can hold. It is true while any vote is cast and turns false once every vote is
 * withdrawn. Each voter casts and withdraws with its own unique ID, which can be any type that TSet accepts: usually
 * an FName or a UENUM.
 */
template <typename VoteIdType>
class TDrsVotableState
{
public:
	/** Casts a vote. Returns whether this vote changed the state (to true). */
	bool CastVote(const VoteIdType& voteId)
	{
		const bool wasVoted = IsVoted();
		Votes.Add(voteId);
		return !wasVoted;
	}

	/** Withdraws a vote. Returns whether this changed the state (to false). */
	bool WithdrawVote(const VoteIdType& voteId)
	{
		if (!IsVoted())
		{
			return false;
		}
		Votes.Remove(voteId);
		return !IsVoted();
	}

	/** Casts or withdraws a vote. Returns whether the state changed. */
	bool SetVoteCast(const VoteIdType& voteId, bool isCast)
	{
		return isCast ? CastVote(voteId) : WithdrawVote(voteId);
	}

	/** Whether any vote is cast. */
	bool IsVoted() const { return Votes.Num() > 0; }

	/** Whether a given vote is cast. */
	bool WasVoteCast(const VoteIdType& voteId) const { return Votes.Contains(voteId); }

	int32 GetVoteCount() const { return Votes.Num(); }

	/** The cast votes, for debugging ("..." when there are none). */
	FString GetVotesString() const
	{
		if (!IsVoted())
		{
			return TEXT("..."); // Not "None", which reads like a default FName.
		}
		return FString::JoinBy(Votes, TEXT(", "), [](const VoteIdType& voteId) { return Drs::Private::VoteIdToString(voteId); });
	}

private:
	TSet<VoteIdType> Votes;
};
