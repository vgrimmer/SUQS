#include "Misc/AutomationTest.h"
#include "Engine.h"
#include "SuqsProgression.h"
#include "SuqsTaskState.h"


const FString TimeLimitQuestJson = R"RAWJSON([
	{
		"Name": "Q_TimeLimits",
		"Identifier": "Q_TimeLimits",
		"bPlayerVisible": true,
		"Title": "NSLOCTEXT(\"[TestQuests]\", \"TimeLimitQuestTitle\", \"Quest With Time Limit\")",
		"DescriptionWhenActive": "NSLOCTEXT(\"[TestQuests]\", \"TimeLimitQuestDesc\", \"A quest with a time limit\")",
		"DescriptionWhenCompleted": "",
		"AutoAccept": false,
		"PrerequisiteQuests": [],
		"PrerequisiteQuestFailures": [],
		"Objectives": [
			{
				"Identifier": "O_Single",
				"Title": "NSLOCTEXT(\"[TestQuests]\", \"TimeLimitO1\", \"First objective, single task\")",
				"DescriptionWhenActive": "",
				"DescriptionWhenCompleted": "",
				"bSequentialTasks": true,
				"bAllMandatoryTasksRequired": true,
				"Branch": "None",
				"Tasks": [
					{
						"Identifier": "T_Single",
						"Title": "NSLOCTEXT(\"[TestQuests]\", \"TSingleDesc\", \"Single time limit task\")",
						"bMandatory": true,
						"TargetNumber": 1,
						"TimeLimit": 100
					}
				]
			},
			{
				"Identifier": "O_SingleButNotFirst",
				"Title": "NSLOCTEXT(\"[TestQuests]\", \"TimeLimitO2\", \"Objective with 2 sequential tasks, later one is timed\")",
				"DescriptionWhenActive": "",
				"DescriptionWhenCompleted": "",
				"bSequentialTasks": true,
				"bAllMandatoryTasksRequired": true,
				"Branch": "None",
				"Tasks": [
					{
						"Identifier": "T_NonTimeLimited",
						"Title": "NSLOCTEXT(\"[TestQuests]\", \"TNotTimeLimited\", \"This is not time limited\")",
						"bMandatory": true,
						"TargetNumber": 1,
						"TimeLimit": 0
					},
					{
						"Identifier": "T_SecondTimeLimited",
						"Title": "NSLOCTEXT(\"[TestQuests]\", \"TTimeLimited\", \"This is time limited\")",
						"bMandatory": true,
						"TargetNumber": 1,
						"TimeLimit": 50
					},
					{
						"Identifier": "T_NonTimeLimited2",
						"Title": "NSLOCTEXT(\"[TestQuests]\", \"TNotTimeLimited\", \"This is not time limited\")",
						"bMandatory": true,
						"TargetNumber": 1,
						"TimeLimit": 0
					}
				]
			},
			{
				"Identifier": "O_MultipleTimeLimitsNonSequential",
				"Title": "NSLOCTEXT(\"[TestQuests]\", \"DummyTitle\", \"Ignore this for test\")",
				"DescriptionWhenActive": "",
				"DescriptionWhenCompleted": "",
				"bSequentialTasks": false,
				"bAllMandatoryTasksRequired": true,
				"Branch": "None",
				"Tasks": [
					{
						"Identifier": "T_NonSequential1",
						"Title": "NSLOCTEXT(\"[TestQuests]\", \"TNonSeqNonMandTitle\", \"Non-sequential time limited task, not mandatory\")",
						"bMandatory": false,
						"TargetNumber": 1,
						"TimeLimit": 20
					},
					{
						"Identifier": "T_NonSequential2",
						"Title": "NSLOCTEXT(\"[TestQuests]\", \"TNonSeqNonMandTitle\", \"Non-sequential time limited task, not mandatory\")",
						"bMandatory": false,
						"TargetNumber": 1,
						"TimeLimit": 30
					},
					{
						"Identifier": "T_NonSequential3",
						"Title": "NSLOCTEXT(\"[TestQuests]\", \"TNonSeqMandTitle\", \"Non-sequential time limited task, the only mandatory\")",
						"bMandatory": true,
						"TargetNumber": 1,
						"TimeLimit": 50
					}
				]
			}
		]
	},
])RAWJSON";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestQuestTimeLimitSimple, "SUQSTest.QuestTimeLimitSimple",
                                 EAutomationTestFlags::EditorContext |
                                 EAutomationTestFlags::ClientContext |
                                 EAutomationTestFlags::ProductFilter)

bool FTestQuestTimeLimitSimple::RunTest(const FString& Parameters)
{
	USuqsProgression* Progression = NewObject<USuqsProgression>();
	UDataTable* QuestTable = NewObject<UDataTable>();
	QuestTable->RowStruct = FSuqsQuest::StaticStruct();
	QuestTable->CreateTableFromJSONString(TimeLimitQuestJson);

	Progression->QuestDataTables.Add(QuestTable);

	TestTrue("Accept quest OK", Progression->AcceptQuest("Q_TimeLimits"));
	TestTrue("Quest should be incomplete", Progression->IsQuestIncomplete("Q_TimeLimits"));

	// Manually tick
	Progression->Tick(10);
	TestFalse("Quest should not be failed", Progression->IsQuestFailed("Q_TimeLimits"));
	// Ticking past where later objectives would fail, but should not fail because only first objective should tick
	// and that task has a longer timeout
	Progression->Tick(60); // total 70
	TestFalse("Quest should not be failed", Progression->IsQuestFailed("Q_TimeLimits"));
	TestFalse("Inactive tasks should not be failed", Progression->IsTaskFailed("Q_TimeLimits", "T_SecondTimeLimited"));
	// Now tick beyond first task time limit
	Progression->Tick(30.1); // total 100.1
	TestTrue("Quest should have failed due to timeout", Progression->IsQuestFailed("Q_TimeLimits"));
	TestTrue("Objective should have failed due to timeout", Progression->IsObjectiveFailed("Q_TimeLimits", "O_Single"));
	TestTrue("Task should have failed due to timeout", Progression->IsTaskFailed("Q_TimeLimits", "T_Single"));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestQuestTimeLimitNotFirstTask, "SUQSTest.QuestTimeLimitNotFirstTask",
                                 EAutomationTestFlags::EditorContext |
                                 EAutomationTestFlags::ClientContext |
                                 EAutomationTestFlags::ProductFilter)

bool FTestQuestTimeLimitNotFirstTask::RunTest(const FString& Parameters)
{
	USuqsProgression* Progression = NewObject<USuqsProgression>();
	UDataTable* QuestTable = NewObject<UDataTable>();
	QuestTable->RowStruct = FSuqsQuest::StaticStruct();
	QuestTable->CreateTableFromJSONString(TimeLimitQuestJson);

	Progression->QuestDataTables.Add(QuestTable);

	TestTrue("Accept quest OK", Progression->AcceptQuest("Q_TimeLimits"));
	auto T = Progression->GetTaskState("Q_TimeLimits", "T_SecondTimeLimited");
	float OrigTimeLeft = T->GetTimeRemaining();
	// Tick a little and confirm it doesn't change the timeout
	Progression->Tick(10);
	TestEqual("Ticking while objective inactive should not have changed second objective", T->GetTimeRemaining(), OrigTimeLeft);
	// Complete first objective so we can start on the next
	TestTrue("Completing task should have worked", Progression->CompleteTask("Q_TimeLimits", "T_Single"));
	TestEqual("First task of second objective should be current", Progression->GetNextMandatoryTask("Q_TimeLimits")->GetIdentifier(), FName("T_NonTimeLimited"));
	// this task isn't time limited, and objective is sequential, so again ticking should not affect the time limited task
	Progression->Tick(10);
	TestEqual("Ticking while time limited task isn't next shouldn't reduce time left", T->GetTimeRemaining(), OrigTimeLeft);
	// Complete the non-timelimited task
	TestTrue("Completing task should have worked", Progression->CompleteTask("Q_TimeLimits", "T_NonTimeLimited"));
	// OK time limited task should be active now, and should reduce
	Progression->Tick(10);
	TestTrue("Second task should be reducing time limit now", T->GetTimeRemaining() < (OrigTimeLeft - 9.9));
	TestFalse("Quest should not be failed yet", Progression->IsQuestFailed("Q_TimeLimits"));
	Progression->Tick(50); // Total 60 now
	TestEqual("Second task should bhave run out of time", T->GetTimeRemaining(), 0.f);
	TestTrue("Quest should have failed now", Progression->IsQuestFailed("Q_TimeLimits"));

	// Reset so we can test having completed within time limit
	T->Reset();
	TestEqual("Time limit should be back", T->GetTimeRemaining(), OrigTimeLeft);
	TestFalse("Quest should not be failed anymore", Progression->IsQuestFailed("Q_TimeLimits"));
	Progression->Tick(15); // Tick a little within limit
	float TimeRemainingAtCompletion = T->GetTimeRemaining();
	TestEqual("Time remaining should be correct", TimeRemainingAtCompletion, T->GetTimeLimit() - 15);
	// Complete the tiem limited task
	T->Complete();
	// Same objective should still be active, but now moved on to last task
	// now tick again, and make sure this didn't affect the completed task
	Progression->Tick(50);
	TestEqual("Time remaining should not have changed", T->GetTimeRemaining(), TimeRemainingAtCompletion);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestQuestTimeLimitMultipleSimultaneousTasks, "SUQSTest.QuestTimeLimitMultipleSimultaneousTasks",
                                 EAutomationTestFlags::EditorContext |
                                 EAutomationTestFlags::ClientContext |
                                 EAutomationTestFlags::ProductFilter)

bool FTestQuestTimeLimitMultipleSimultaneousTasks::RunTest(const FString& Parameters)
{
	return true;
}