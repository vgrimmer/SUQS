#include "SuqsQuestState.h"

#include "SuqsObjectiveState.h"
#include "SuqsTaskState.h"

void USuqsQuestState::Initialise(FSuqsQuest* Def, USuqsPlayState* Root)
{
	// We always build quest state from the master quest definition
	// Then when we restore, we do it into this structure.
	// That means there's never a chance that the state doesn't match the definitions (breaking changes to quests will
	// have to be handled another way)

	// Quest definitions are static data so it's OK to keep this (it's owned by parent)
	QuestDefinition = Def;
	PlayState = Root;
	Status = ESuqsQuestStatus::NotStarted;
	FastTaskLookup.Empty();

	for (const auto& ObjDef : Def->Objectives)
	{
		auto Obj = NewObject<USuqsObjectiveState>(this);
		Obj->Initialise(&ObjDef, this, Root);
		Objectives.Add(Obj);

		for (auto Task : Obj->Tasks)
		{
			FastTaskLookup.Add(Task->GetIdentifier(), Task);
		}
	}
	
	
}

void USuqsQuestState::Tick(float DeltaTime)
{
	for (auto& Objective : Objectives)
	{
		Objective->Tick(DeltaTime);
	}
	
}


USuqsTaskState* USuqsQuestState::FindTask(const FName& Identifier) const
{
	return FastTaskLookup.FindChecked(Identifier);
}


void USuqsQuestState::SetBranchActive(FName Branch, bool bActive)
{
	int Changes;
	if (bActive)
		Changes = ActiveBranches.AddUnique(Branch);
	else
		Changes = ActiveBranches.Remove(Branch);

	if (Changes > 0)
		NotifyObjectiveStatusChanged();
}

bool USuqsQuestState::IsBranchActive(FName Branch)
{
	return ActiveBranches.Contains(Branch);
}

void USuqsQuestState::NotifyObjectiveStatusChanged()
{
	// TODO propagate state
	// If not completed or failed, figure out what objective should be active now
	// Re-scan objectives to see what we should do now
}
