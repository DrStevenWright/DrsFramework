// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsXrArbiter/DrsXrArbiter.h"
#include "DrsXrArbiter/DrsXrArbiterTypes.h"
#include "DrsXrArbiter/DrsXrConfigText.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

using Drs::XrArbiter::FConfigText;

namespace
{
	// The entries in DrsXrPlatform's Config/DefaultDrsXrPlatform.ini.
	const TArray<FString> DefaultVendorEntries =
	{
		TEXT("(Id=\"Meta\",Keywords=(\"Meta\",\"Oculus\",\"Quest\"),MarkerModule=\"OculusXRHMD\")"),
		TEXT("(Id=\"Pico\",Keywords=(\"Pico\"),MarkerModule=\"PICOOpenXRHMD\")"),
	};

	FDrsXrManifestLocator MakeLocator(EDrsXrManifestLocator kind, const FString& name, const FString& path)
	{
		FDrsXrManifestLocator locator;
		locator.Locator = kind;
		locator.Name = name;
		locator.Path = path;
		return locator;
	}

	FDrsXrRuntime MakeRuntime(const TCHAR* name, const TCHAR* vendor = TEXT(""))
	{
		FDrsXrRuntime runtime;
		runtime.Name = name;
		runtime.Vendor = vendor;
		runtime.Manifest.Add(MakeLocator(EDrsXrManifestLocator::ProgramFiles, FString(), FString(name) + TEXT(".json")));
		return runtime;
	}

	/** Chooses among SteamVR, Oculus and Pico, with the given ones installed and running. */
	FName Choose(const TSet<FName>& installed, const TSet<FName>& running)
	{
		const TArray<FDrsXrRuntime> runtimes = { MakeRuntime(TEXT("SteamVR")), MakeRuntime(TEXT("Oculus"), TEXT("Meta")), MakeRuntime(TEXT("PicoConnect"), TEXT("Pico")) };
		FString manifest;
		const FDrsXrRuntime* selected = Drs::XrArbiter::ChooseRuntime(runtimes,
			[&installed](const FDrsXrRuntime& runtime) { return installed.Contains(runtime.Name) ? runtime.Manifest[0].Path : FString(); },
			[&running](const FDrsXrRuntime& runtime) { return running.Contains(runtime.Name); },
			manifest);
		return selected ? selected->Name : NAME_None;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrConfigTextTest, "Drs.XrPlatform.Arbiter.ConfigText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrConfigTextTest::RunTest(const FString& parameters)
{
	const TOptional<FConfigText> text = FConfigText::Parse(TEXT(" ( Name = \"A \\\"quoted\\\" (name)\", Bare = Token , List=(\"x\", y), Nested=((K=1),(K=2)), Empty=() ) "));
	if (!TestTrue(TEXT("Parses"), text.IsSet()))
	{
		return false;
	}
	TestEqual(TEXT("Quoted with escapes and parentheses"), text->GetString(TEXT("Name")), FString(TEXT("A \"quoted\" (name)")));
	TestEqual(TEXT("Bare token, trimmed"), text->GetString(TEXT("bare")), FString(TEXT("Token")));
	TestEqual(TEXT("List"), text->GetStrings(TEXT("List")), TArray<FString>{ TEXT("x"), TEXT("y") });
	TestEqual(TEXT("Nested structs"), text->GetItems(TEXT("Nested")).Num(), 2);
	TestEqual(TEXT("Nested field"), text->GetItems(TEXT("Nested"))[1].GetString(TEXT("K")), FString(TEXT("2")));
	TestTrue(TEXT("Empty list"), text->GetStrings(TEXT("Empty")).IsEmpty());
	TestTrue(TEXT("Missing field"), text->Find(TEXT("Missing")) == nullptr);
	TestEqual(TEXT("A plain value counts as a list of one"), text->GetStrings(TEXT("Bare")).Num(), 1);

	TestEqual(TEXT("Top-level bare value"), FConfigText::Parse(TEXT("SteamVR"))->Text, FString(TEXT("SteamVR")));

	AddExpectedError(TEXT("Can't parse the config value"), EAutomationExpectedErrorFlags::Contains, 3);
	TestFalse(TEXT("Unclosed group"), FConfigText::Parse(TEXT("(Name=\"A\"")).IsSet());
	TestFalse(TEXT("Unclosed quote"), FConfigText::Parse(TEXT("(Name=\"A)")).IsSet());
	TestFalse(TEXT("Trailing text"), FConfigText::Parse(TEXT("(Name=A) extra")).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrArbiterVendorsTest, "Drs.XrPlatform.Arbiter.Vendors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrArbiterVendorsTest::RunTest(const FString& parameters)
{
	const TArray<FDrsXrVendor> vendors = Drs::XrArbiter::ParseVendors(DefaultVendorEntries);
	if (!TestEqual(TEXT("Vendor count"), vendors.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("Id"), vendors[0].Id, FName(TEXT("Meta")));
	TestEqual(TEXT("Keywords"), vendors[0].Keywords.Num(), 3);
	TestEqual(TEXT("Marker module"), vendors[1].MarkerModule, FName(TEXT("PICOOpenXRHMD")));

	// The identities a Quest 3 reported in the DrsXrProbe runs.
	const FName meta(TEXT("Meta"));
	TestEqual(TEXT("Link system name"), Drs::XrArbiter::MatchVendor(vendors, { TEXT("Meta Quest 3") }), meta);
	TestEqual(TEXT("SteamVR system name"), Drs::XrArbiter::MatchVendor(vendors, { TEXT("SteamVR/OpenXR : oculus") }), meta);
	TestEqual(TEXT("Quest brand and manufacturer"), Drs::XrArbiter::MatchVendor(vendors, { TEXT("oculus"), TEXT("Oculus") }), meta);
	TestEqual(TEXT("Pico"), Drs::XrArbiter::MatchVendor(vendors, { TEXT("PICO 4") }), FName(TEXT("Pico")));
	TestEqual(TEXT("SteamVR with a lighthouse headset"), Drs::XrArbiter::MatchVendor(vendors, { TEXT("SteamVR/OpenXR : lighthouse") }), FName());
	TestEqual(TEXT("No identity"), Drs::XrArbiter::MatchVendor(vendors, {}), FName());

	AddExpectedError(TEXT("which has no Id"), EAutomationExpectedErrorFlags::Contains, 1);
	TestTrue(TEXT("An entry without an Id is skipped"), Drs::XrArbiter::ParseVendors({ TEXT("(Keywords=(\"X\"))") }).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrArbiterRuntimesTest, "Drs.XrPlatform.Arbiter.Runtimes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrArbiterRuntimesTest::RunTest(const FString& parameters)
{
	const TArray<FDrsXrRuntime> runtimes = Drs::XrArbiter::ParseRuntimes({
		TEXT("(Name=\"SteamVR\",Manifest=((Locator=UriHandler,Name=\"steamvr\",Path=\"../../steamxr_win64.json\"),(Locator=SteamLibrary,Path=\"steamapps/common/SteamVR/steamxr_win64.json\")),RunningProcesses=(\"vrmonitor.exe\"))"),
		TEXT("(Name=\"Oculus\",Vendor=\"Meta\",Manifest=((Locator=EnvironmentVariable,Name=\"OculusBase\",Path=\"Support/oculus-runtime/oculus_openxr_64.json\")),RunningProcesses=(\"oculus-platform-runtime.exe\"))"),
	});
	if (!TestEqual(TEXT("Runtime count"), runtimes.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("SteamVR names no vendor"), runtimes[0].Vendor, FName());
	TestEqual(TEXT("Locators, in order"), runtimes[0].Manifest.Num(), 2);
	TestEqual(TEXT("Locator kind"), runtimes[0].Manifest[1].Locator, EDrsXrManifestLocator::SteamLibrary);
	TestEqual(TEXT("Locator name"), runtimes[0].Manifest[0].Name, FString(TEXT("steamvr")));
	TestEqual(TEXT("Running processes"), runtimes[0].RunningProcesses, TArray<FString>{ TEXT("vrmonitor.exe") });
	TestEqual(TEXT("Oculus implies Meta"), runtimes[1].Vendor, FName(TEXT("Meta")));

	AddExpectedError(TEXT("not in the runtime table"), EAutomationExpectedErrorFlags::Contains, 1);
	const TArray<FDrsXrRuntime> ordered = Drs::XrArbiter::OrderRuntimes(runtimes, { TEXT("Oculus"), TEXT("Unknown"), TEXT("SteamVR"), TEXT("Oculus") });
	if (TestEqual(TEXT("Ordered count"), ordered.Num(), 2))
	{
		TestEqual(TEXT("Priority order"), ordered[0].Name, FName(TEXT("Oculus")));
	}
	TestTrue(TEXT("Runtimes not in the priority are left out"), Drs::XrArbiter::OrderRuntimes(runtimes, { TEXT("SteamVR") }).Num() == 1);

	AddExpectedError(TEXT("Unknown manifest locator"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("needs a Name and a valid Manifest"), EAutomationExpectedErrorFlags::Contains, 1);
	TestTrue(TEXT("A runtime without a valid locator is skipped"), Drs::XrArbiter::ParseRuntimes({ TEXT("(Name=\"X\",Manifest=((Locator=Nowhere,Path=\"x.json\")))") }).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrArbiterChooseRuntimeTest, "Drs.XrPlatform.Arbiter.ChooseRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrArbiterChooseRuntimeTest::RunTest(const FString& parameters)
{
	const FName steamVr(TEXT("SteamVR"));
	const FName oculus(TEXT("Oculus"));
	const FName pico(TEXT("PicoConnect"));

	// Meta's Link software runs from boot, so Oculus always counts as running.
	TestEqual(TEXT("A running SteamVR wins over a running Oculus"), Choose({ steamVr, oculus }, { steamVr, oculus }), steamVr);
	TestEqual(TEXT("Oculus wins while SteamVR is only installed"), Choose({ steamVr, oculus }, { oculus }), oculus);
	TestEqual(TEXT("A running runtime wins over an earlier installed one"), Choose({ steamVr, pico }, { pico }), pico);
	TestEqual(TEXT("With none running, the first installed one"), Choose({ oculus, pico }, {}), oculus);
	TestEqual(TEXT("A running runtime that isn't installed doesn't count"), Choose({ oculus }, { steamVr }), oculus);
	TestEqual(TEXT("None installed"), Choose({}, { steamVr }), FName());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrArbiterExportTest, "Drs.XrPlatform.Arbiter.EditorExport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrArbiterExportTest::RunTest(const FString& parameters)
{
	// Project Settings saves the tables with ExportText, so the parser must read what it writes.
	FDrsXrVendor vendor;
	vendor.Id = TEXT("Vive");
	vendor.Keywords = { TEXT("HTC"), TEXT("Vive \"Pro\"") };
	vendor.MarkerModule = TEXT("ViveHMD");
	FString vendorText;
	FDrsXrVendor::StaticStruct()->ExportText(vendorText, &vendor, nullptr, nullptr, PPF_None, nullptr);

	const TArray<FDrsXrVendor> vendors = Drs::XrArbiter::ParseVendors({ vendorText });
	if (TestEqual(*FString::Printf(TEXT("Vendor round trip of %s"), *vendorText), vendors.Num(), 1))
	{
		TestEqual(TEXT("Id"), vendors[0].Id, vendor.Id);
		TestEqual(TEXT("Keywords"), vendors[0].Keywords, vendor.Keywords);
		TestEqual(TEXT("Marker module"), vendors[0].MarkerModule, vendor.MarkerModule);
	}

	FDrsXrRuntime runtime = MakeRuntime(TEXT("Oculus"), TEXT("Meta"));
	runtime.Manifest.Add(MakeLocator(EDrsXrManifestLocator::EnvironmentVariable, TEXT("OculusBase"), TEXT("Support/oculus-runtime/oculus_openxr_64.json")));
	runtime.RunningProcesses = { TEXT("oculus-platform-runtime.exe") };
	FString runtimeText;
	FDrsXrRuntime::StaticStruct()->ExportText(runtimeText, &runtime, nullptr, nullptr, PPF_None, nullptr);

	const TArray<FDrsXrRuntime> runtimes = Drs::XrArbiter::ParseRuntimes({ runtimeText });
	if (TestEqual(*FString::Printf(TEXT("Runtime round trip of %s"), *runtimeText), runtimes.Num(), 1))
	{
		TestEqual(TEXT("Name"), runtimes[0].Name, runtime.Name);
		TestEqual(TEXT("Vendor"), runtimes[0].Vendor, runtime.Vendor);
		if (TestEqual(TEXT("Locators"), runtimes[0].Manifest.Num(), 2))
		{
			TestEqual(TEXT("Locator kind"), runtimes[0].Manifest[1].Locator, runtime.Manifest[1].Locator);
			TestEqual(TEXT("Locator name"), runtimes[0].Manifest[1].Name, runtime.Manifest[1].Name);
			TestEqual(TEXT("Locator path"), runtimes[0].Manifest[1].Path, runtime.Manifest[1].Path);
		}
		TestEqual(TEXT("Running processes"), runtimes[0].RunningProcesses, runtime.RunningProcesses);
	}

	// A multi-vendor runtime exports its empty vendor as None.
	FString steamText;
	FDrsXrRuntime steamVr = MakeRuntime(TEXT("SteamVR"));
	FDrsXrRuntime::StaticStruct()->ExportText(steamText, &steamVr, nullptr, nullptr, PPF_None, nullptr);
	const TArray<FDrsXrRuntime> steamRuntimes = Drs::XrArbiter::ParseRuntimes({ steamText });
	if (TestEqual(TEXT("SteamVR round trip"), steamRuntimes.Num(), 1))
	{
		TestTrue(TEXT("No vendor"), steamRuntimes[0].Vendor.IsNone());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrsXrArbiterDefaultConfigTest, "Drs.XrPlatform.Arbiter.DefaultConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDrsXrArbiterDefaultConfigTest::RunTest(const FString& parameters)
{
	// The tables as GConfig gives them, from DrsXrPlatform's Config/DefaultDrsXrPlatform.ini and any project override.
	const FDrsXrVendor* meta = Drs::XrArbiter::FindVendor(TEXT("Meta"));
	if (TestNotNull(TEXT("Meta is in the vendor table"), meta))
	{
		TestEqual(TEXT("Meta's marker module"), meta->MarkerModule, FName(TEXT("OculusXRHMD")));
	}
	TestNotNull(TEXT("Pico is in the vendor table"), Drs::XrArbiter::FindVendor(TEXT("Pico")));

	TArray<FName> runtimeNames;
	for (const FDrsXrRuntime& runtime : Drs::XrArbiter::GetPrioritisedRuntimes())
	{
		runtimeNames.Add(runtime.Name);
	}
	TestEqual(TEXT("Runtime priority"), runtimeNames, TArray<FName>{ TEXT("SteamVR"), TEXT("Oculus"), TEXT("PicoStreaming"), TEXT("PicoConnect") });

	// MetaXR loads as normal in the editor, so its marker module identifies it.
	if (FModuleManager::Get().IsModuleLoaded(TEXT("OculusXRHMD")))
	{
		TestEqual(TEXT("The running vendor plugin"), Drs::XrArbiter::GetXrPluginType(), FName(TEXT("Meta")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
